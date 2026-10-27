#include "xiaozhi.h"
#include "xiaozhi_ai.h"
#include "task.h"
#include "ui/src/ui.h"

void xiaozhi_update()
{
    if(Xiaozhi.ask_flag == 1)
    {
        // 回答播放期间不 poll、不重启 ASR:
        // 原来的写法在 answer_flag == 1 时仍然调 xiaozhi_listen(),而库里的
        // xiaozhi_listen() 只要 ws_state == 0 就会重新走一遍 start_asr
        // (HTTPS 拿百度 token + 连 WebSocket),5ms 一次会变成请求风暴。
        // 同时开关关掉后也不 poll,麦克风就不再往百度推流。
        if(Xiaozhi.answer_flag == 0)
        {
            xiaozhi_loop();   // 处理 WebSocket 收包 / 按节奏推送麦克风数据

            // 启动识别失败时给 300ms 退避,别每 5ms 重试一次百度接口
            static uint32_t last_listen_try = 0;
            if(millis() - last_listen_try >= 300)
            {
                last_listen_try = millis();
                if(xiaozhi_listen())
                {
                    //获得问题文本
                    Xiaozhi.question_text = xiaozhi_question();
                    LVGL_LOCK();
                    lv_textarea_set_text(ui_TextAreaQuestion, Xiaozhi.question_text.c_str());
                    LVGL_UNLOCK();
                }
            }

            //检测是否合成音频答案
            if(xiaozhi_speak())
            {
                // xiaozhi_answer(0) 返回答案文本, xiaozhi_answer(1) 返回TTS音频URL
                Xiaozhi.answer_text = xiaozhi_answer(0);
                LVGL_LOCK();
                lv_textarea_set_text(ui_TextAreaAnswer, Xiaozhi.answer_text.c_str());
                // 回答播完后不再继续放歌,同时把音乐开关的UI状态复位
                if(lv_obj_has_state(ui_StartMusic, LV_STATE_CHECKED))
                {
                    lv_obj_clear_state(ui_StartMusic, LV_STATE_CHECKED);
                }
                LVGL_UNLOCK();

                Xiaozhi.answer_url = xiaozhi_answer(1);
                Music.play_flag = 1;
                Music.start_flag = 1;
                Xiaozhi.answer_flag = 1;
            }
        }
    }
}
