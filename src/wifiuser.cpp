#include "wifiuser.h"
#include "lvgl.h"
#include "ui/src/ui.h"
#include <WiFi.h>
#include "data.h"

void WiFi_update()
{
    static int last_status = -1;
    if(Set.WiFi_connect_status != last_status)
    {
        last_status = Set.WiFi_connect_status;
        if(last_status)
            lv_label_set_text_fmt(ui_LabelWiFIstate, "已连接 %s", WiFi.SSID().c_str());
        else
            lv_label_set_text(ui_LabelWiFIstate, "未连接");
    }

    if(Set.WiFi_connect_status && WiFi.status() != WL_CONNECTED)
        Set.WiFi_connect_status = 0;

    if(Set.WiFi_connect_flag)WiFi_connect();

    if(Set.WiFi_button_flag)
    {
        lv_roller_set_options(ui_RollerWiFi, "正在扫描附近WiFi...\n",
        LV_ROLLER_MODE_NORMAL);
        WiFi_scan();
        Set.WiFi_button_flag = 0;  // 无条件清零，防止扫不到时死循环重扫
    }
    else
    {
        lv_roller_set_options(ui_RollerWiFi, Set.WiFi_scan_all.c_str(),
        LV_ROLLER_MODE_NORMAL);
    }
}

void WiFi_scan()
{
    if(WiFi.status() != WL_CONNECTED)
    {
        WiFi.disconnect(false, false);  // 停掉进行中的连接尝试，否则驱动忙，扫描必然失败
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    Set.WiFi_scan_all = "";  // 先清空，避免重复叠加
    int n = WiFi.scanNetworks();
    Serial.printf("scanNetworks = %d\n", n);
    if (n <= 0)  // 0、-1、-2 都算没扫到（-2 = 驱动忙/模式未启动）
    {
        Set.WiFi_scan_all = "附近无可用WiFi\n";
    }
    else
    {
        for (int i = 0; i < n; ++i)
        {
            Set.WiFi_scan_one = WiFi.SSID(i) + "\n";
            Set.WiFi_scan_all += Set.WiFi_scan_one;
        }
        strcpy(Set.WiFi_name, WiFi.SSID(0).c_str());  // 取列表第一项，而不是当前已连接的SSID
    }
    // 注意：这里不能调 WiFi_init()——WiFi.begin() 内部会触发 esp_wifi_connect()，
    // 让驱动进入“连接中”状态，下一次扫描就会失败。开机自动重连由 main.cpp 的 WiFi_task 负责。
}

void WiFi_connect()
{
    WiFi.begin(Set.WiFi_name, Set.WiFi_password);
    for(int i = 0; i < Set.WiFi_connect_timeout; ++i)
    {
        uint32_t connect_status = WiFi.status();
        Serial.println(connect_status);
        if(connect_status == WL_CONNECTED)
        {
            Serial.println("success");
            Set.WiFi_connect_status = 1;
            break;
        }
        else Set.WiFi_connect_status = 0;
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    if(Set.WiFi_connect_status)
    {
        if(!Data.time_flag){time_get(), Data.time_flag = 1;}
        if(!Data.weather_flag)Data.weather_flag = 1;
    }
    lv_obj_add_flag(ui_SpinnerWiFi, LV_OBJ_FLAG_HIDDEN);
    Set.WiFi_connect_flag = 0;
}

void WiFi_init()
{
    WiFi.mode(WIFI_STA);  // 显式设为STA模式，确保扫描可用
    WiFi.begin();
    for(int i = 0; i < Set.WiFi_connect_timeout; ++i)
    {
        uint32_t connect_status = WiFi.status();
        Serial.println(connect_status);
        if(connect_status == WL_CONNECTED)
        {
            Serial.println("success");
            Set.WiFi_connect_status = 1;
            break;
        }
        else Set.WiFi_connect_status = 0;
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    if(Set.WiFi_connect_status)
    {
        if(!Data.time_flag){time_get(), Data.time_flag = 1;}
        if(!Data.weather_flag)Data.weather_flag = 1;
    }
}