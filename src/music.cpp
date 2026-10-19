#include "config.h"
#include "music.h"
#include <driver/i2s.h>
#include <Audio.h>

#define I2S_DOUT       39
#define I2S_BCLK      40
#define I2S_LRC       41

Audio audio;

void music_init()
{
    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio.setVolume(16); // 0...21
    audio.setBalance(100);
    music_read();
}

void music_read()
{
    File root = SD.open("/");
    File file = root.openNextFile();
    while (file)
    {
        static int cnt = 0;
        if(cnt <= 1)cnt++;
        if (!file.isDirectory())
        {
            const char *filename = file.name();
            int8_t len = strlen(filename);
            const char *mp3_ext = ".mp3";
            if ((filename[0] != '.') && (strcmp(mp3_ext, &filename[len - 4]) == 0))
            {
                if(cnt == 1)strcpy(Music.song_name, filename);
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
    audio.setVolume(Music.volume_value); 
    if(Music.play_flag == 1)
    {
        // 暂停->播放
        if(Music.pause_flag && !audio.isRunning())
        {
            audio.pauseResume();
            Music.pause_flag = 0;
        
        }
        // 切歌
        if(!audio.isRunning() && Music.start_flag == 0)
        {
            Music.start_flag = 1; 
        }
        if(Music.start_flag)
        {
            audio.connecttoFS(SD, Music.song_name);
            Music.start_flag = 0;
        }
        
        if(!audio.isRunning() && Music.start_flag == 0)
        {
			Music.start_flag = 1; 
        }
    } 
    else 
    {
        // 播放->暂停
        if(!Music.pause_flag && audio.isRunning())
        {
        audio.pauseResume();
        Music.pause_flag = 1;
        }
    }
}