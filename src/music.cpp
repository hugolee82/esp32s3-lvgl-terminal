#include "config.h"
#include "music.h"
#include "task.h"
#include "ui/src/ui.h"   // 播完复位音乐开关要用 ui_StartMusic
#include <driver/i2s.h>
#include <strings.h>   // strcasecmp
#include <Audio.h>

#define I2S_DOUT      39
#define I2S_BCLK      40
#define I2S_LRC       41

// 一首播完之后怎么办:
//   0 = 停下来,并把界面上的音乐开关复位(常规播放器行为)
//   1 = 单曲循环,无缝从头放(不需要重新打开文件,不会有空档)
#define MUSIC_LOOP_ONE_SONG   0

// 必须显式指定 I2S_NUM_1:
// ESP32-audioI2S 2.3.0 起构造函数默认端口改成了 I2S_NUM_0,
// 而小智麦克风库固定占用 I2S_NUM_0,不指定会互相顶掉(I2S 驱动装不上,收音/放音同时失效)
Audio audio(false, 3, I2S_NUM_1);

static int      last_volume    = -1;      // 上次真正写进去的音量
static bool     eof_flag       = false;   // 一首播完(音频库回调里置位)
static uint32_t reconnect_tick = 0;       // 重新打开文件的节流时间点
static uint8_t  open_fail_cnt  = 0;       // 连续打开失败次数(坏文件/被拔卡)

// ESP32-audioI2S 的回调:MP3 播完时会在 audio.loop() 里被调用,
// 和 music_update() 在同一个任务里,所以这里只是置个标志,真正的处理放在 music_update()
void audio_eof_mp3(const char *info)
{
    eof_flag = true;
    Serial.printf("[Music] 播完: %s\n", info ? info : "");
}

void music_init()
{
    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio.setVolume(16); // 0...21
    // 库的 setBalance() 范围是 -16...16,传 100 会被截成 16 = 右声道完全静音。
    // 0 才是左右平衡(如果板子只接了一个声道,也一样能正常出声)。
    audio.setBalance(0);
    music_read();
}

void music_read()
{
    Music.song_list = "";   // 重新扫描时先清空,避免列表一次比一次长
    File root = SD.open("/");
    if (!root)
    {
        Serial.println("[Music] SD 根目录打不开");
        return;
    }
    File file = root.openNextFile();
    bool found_first = false;
    while (file)
    {
        if (!file.isDirectory())
        {
            const char *filename = file.name();
            int len = strlen(filename);
            const char *mp3_ext = ".mp3";
            // strcasecmp: 大写 .MP3 也能识别
            if ((len > 4) && (filename[0] != '.') && (strcasecmp(mp3_ext, &filename[len - 4]) == 0))
            {
                // 第一首 mp3 作为默认选中的歌(原逻辑用 cnt==1,根目录第一个不是 mp3 时歌名就为空)
                if (!found_first)
                {
                    memset(Music.song_name, 0, sizeof(Music.song_name));
                    strncpy(Music.song_name, filename, sizeof(Music.song_name) - 1);
                    found_first = true;
                }
                Music.song_list += filename;
                Music.song_list += "\n";
            }
        }
        file = root.openNextFile();
    }
    Music.song_options = Music.song_list.c_str();
}

void music_update()
{
    audio.loop();

    // 只有音量真的变了才写。原来每 5ms 调一次 setVolume,没必要
    if(Music.volume_value != last_volume)
    {
        last_volume = Music.volume_value;
        audio.setVolume(Music.volume_value);
    }

    // 1) 一首播完:停掉播放请求 + 复位界面开关。
    //    原来的写法是 "没在播 -> start_flag=1",播完以后会立刻重新打开同一首歌,
    //    每 5ms 一次,听起来就是音乐反复停一下再从头开始。
    if(eof_flag)
    {
        eof_flag = false;
#if MUSIC_LOOP_ONE_SONG
        Music.start_flag = 1;               // 重新放一遍
#else
        Music.play_flag  = 0;
        Music.pause_flag = 0;
        Music.start_flag = 0;
        Serial.println("[Music] 已停止,开关复位");
        LVGL_LOCK();
        if(lv_obj_has_state(ui_StartMusic, LV_STATE_CHECKED))
        {
            lv_obj_clear_state(ui_StartMusic, LV_STATE_CHECKED);
        }
        LVGL_UNLOCK();
#endif
    }

    // 2) 有播放 / 切歌请求
    if(Music.play_flag == 1 && Music.start_flag)
    {
        Music.start_flag = 0;
        Music.pause_flag = 0;
        if(Xiaozhi.answer_flag == 1)
        {
            Serial.println("[Music] 播放小智回答");
            if(!audio.connecttohost(Xiaozhi.answer_url.c_str()))
            {
                Serial.println("[Music] 回答音频打开失败");
            }
            Music.play_flag = 0;            // 回答放完就结束,由下面 else 分支收尾
        }
        else
        {
            Serial.printf("[Music] 播放: %s\n", Music.song_name);
            if(audio.connecttoFS(SD, Music.song_name))
            {
                open_fail_cnt = 0;
#if MUSIC_LOOP_ONE_SONG
                audio.setFileLoop(true);    // 无缝循环,不用重新打开文件
#endif
            }
            else if(++open_fail_cnt >= 3)
            {
                // 文件坏了/卡被拔了:原来会每 500ms 无限重试,串口一直刷日志。
                // 连续失败 3 次就停下来并复位开关。
                open_fail_cnt = 0;
                Music.play_flag  = 0;
                Music.pause_flag = 0;
                Serial.printf("[Music] 连续打开失败,已停止: %s\n", Music.song_name);
                LVGL_LOCK();
                if(lv_obj_has_state(ui_StartMusic, LV_STATE_CHECKED))
                {
                    lv_obj_clear_state(ui_StartMusic, LV_STATE_CHECKED);
                }
                LVGL_UNLOCK();
            }
            else
            {
                Serial.printf("[Music] 文件打不开: %s\n", Music.song_name);
                reconnect_tick = millis();  // 失败后 500ms 再试,别疯狂重连
            }
        }
        return;
    }

    // 3) 开关:开 -> 播放,关 -> 暂停
    if(Music.play_flag == 1)
    {
        if(audio.isRunning())
        {
            Music.pause_flag = 0;
        }
        else if(Music.pause_flag)
        {
            // 暂停 -> 继续
            if(audio.pauseResume()) Music.pause_flag = 0;
        }
        else if(millis() - reconnect_tick >= 500)
        {
            // 本来该响却没响(刚开机 / 上次打开失败 / 被别处停掉):
            // 重新请求一次,带 500ms 节流
            reconnect_tick = millis();
            Music.start_flag = 1;
        }
    }
    else
    {
        // 播放 -> 暂停
        if(!Music.pause_flag && audio.isRunning() && Xiaozhi.answer_flag == 0)
        {
            if(audio.pauseResume()) Music.pause_flag = 1;
        }
        if(!audio.isRunning() && Xiaozhi.answer_flag == 1)
        {
            Xiaozhi.answer_flag = 0;
        }
    }
}
