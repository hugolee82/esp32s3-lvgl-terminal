#include "task.h"
#include "ui/src/ui.h" 
#include "wifiuser.h"
#include "data.h"
#include "sduser.h"
#include "music.h"
#include "xiaozhi_ai.h"
#include "xiaozhi.h"

TaskHandle_t WiFiHandle = nullptr;
TaskHandle_t serialHandle = nullptr;
TaskHandle_t xiaozhiHandle = nullptr;

SemaphoreHandle_t lvgl_mutex = nullptr;

void task_init()
{
    // 必须在任何会调 LVGL 的任务启动前创建
    lvgl_mutex = xSemaphoreCreateRecursiveMutex();

    xTaskCreatePinnedToCore(lvgl_task, "lvgl_task", 1024*30, NULL, 2, NULL, 1);
    // data_task 里要做 HTTPS(weather_get),TLS 握手很吃栈,8KB 偏小
    xTaskCreatePinnedToCore(data_task, "data_task", 1024*12, NULL, 1, NULL, 0);
    // 音频要持续喂 I2S,优先级提到和 lvgl 一样,别被网络任务拖出爆音
    xTaskCreatePinnedToCore(music_task, "music_task", 1024*10, NULL, 2, NULL, 0);
}

void WiFiTask_create()
{
    if(WiFiHandle == nullptr)
    {
        xTaskCreatePinnedToCore(WiFi_task, "WiFi_task", 1024*8, NULL, 1, &WiFiHandle, 0);
    }
}

void serialTask_create()
{
    if(serialHandle == nullptr)
    {
        xTaskCreatePinnedToCore(serial_task, "serial_task", 1024*8, NULL, 1, &serialHandle, 0);
    }
}

void xiaozhiTask_create()
{
    if(xiaozhiHandle == nullptr)
    {
        // 重置库内部状态(ws_state/have_question 等),避免上次退出时残留的会话
        // 让这一次 xiaozhi_listen() 认为"还在识别中"而不重新开麦。
        // 彻底关掉旧 WebSocket 需要库对外提供 xiaozhi_stop(),见 xiaozhiTask_delete 的注释。
        xiaozhi_init();
        // WebSocket + 百度 token 的 HTTPS + JSON 都在这个任务里,10KB 偏小
        xTaskCreatePinnedToCore(xiaozhi_task, "xiaozhi_task", 1024*16, NULL, 1, &xiaozhiHandle, 0);
    }
}

void WiFiTask_delete()
{
    if(WiFiHandle != nullptr)
    {
        Set.WiFi_button_flag = true;   // 下次进设置页重新扫描
        vTaskDelete(WiFiHandle);
        WiFiHandle = nullptr;
    }
}

void serialTask_delete()
{
    if(serialHandle != nullptr)
    {
        vTaskDelete(serialHandle);
        serialHandle = nullptr;
    }
}

void xiaozhiTask_delete()
{
    if(xiaozhiHandle != nullptr)
    {
        Xiaozhi.ask_flag = false;
        // 不要清 answer_flag:回答可能还在播,清掉后 music_update 会把它当成
        // "用户关掉了音乐"直接 pause,回答就断在半句。让它自然播完再归零。
        LVGL_LOCK();
        if(lv_obj_has_state(ui_SwitchSpeak, LV_STATE_CHECKED))
        {
            lv_obj_clear_state(ui_SwitchSpeak, LV_STATE_CHECKED);
            lv_label_set_text(ui_LabelSpeak, "OFF");
        }
        LVGL_UNLOCK();

        // 注意:这里只是删任务,库里的 WebSocket/ASR 会话还在。
        // 彻底释放需要在 baidu-xiaozhi 库里加一个 xiaozhi_stop()
        // (内部调 xiaozhi_stop_asr()),然后在这里 vTaskDelete 之前调用。
        vTaskDelete(xiaozhiHandle);
        xiaozhiHandle = nullptr;
    }
}

void lvgl_task(void *pt)
{
    while(1)
    {
        LVGL_LOCK();
        lv_timer_handler();  // LVGL 核心刷新
        LVGL_UNLOCK();
        vTaskDelay(5);
    }
}

void WiFi_task(void *pt)
{
    while(1)
    {
        WiFi_update();
        vTaskDelay(50);
    }
}

void data_task(void *pt)
{
    while(1)
    {
        data_update();
        vTaskDelay(1000);
    }
}

void serial_task(void *pt)
{
    Serial.setTimeout(50);   // 默认 1000ms,半行数据会把任务卡住 1 秒

    while(1)
    {
        if(Serial.available())
        {
            String RX = Serial.readStringUntil('\n');
            Uart.rx_data = RX;   // 拷贝一份,Uart.rx_data 是 String,不再悬空
            LVGL_LOCK();
            lv_textarea_add_text(ui_TextAreaRX, Uart.rx_data.c_str());
            LVGL_UNLOCK();
        }
        vTaskDelay(10);   // 别忙等,否则和 lvgl_task 抢 CPU
    }
}

void music_task(void *pt)
{
    while(1)
    {
        music_update();
        vTaskDelay(5);
    }
}

void xiaozhi_task(void *pt)
{
    while(1)
    {
        xiaozhi_update();
        vTaskDelay(5);
    }
}