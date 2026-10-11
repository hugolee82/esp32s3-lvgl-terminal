#include "wifiuser.h"
#include "lvgl.h"
#include "ui/src/ui.h"
#include <WiFi.h>

void WiFi_update()
{
    static int last_status = -1;
    if(Set.WiFi_connect_stauts != last_status)
    {
        last_status = Set.WiFi_connect_stauts;
        if(last_status)
            lv_label_set_text_fmt(ui_LabelWiFIstate, "已连接 %s", WiFi.SSID().c_str());
        else
            lv_label_set_text(ui_LabelWiFIstate, "未连接");
    }

    if(Set.WiFi_connect_stauts && WiFi.status() != WL_CONNECTED)
        Set.WiFi_connect_stauts = 0;

    if(Set.WiFi_connect_flag)WiFi_connect();

    if(Set.WiFi_button_flag)
    {
         WiFi_scan();
        lv_roller_set_options(ui_RollerWiFi, "正在扫描附近WiFi...\n", 
        LV_ROLLER_MODE_NORMAL);
    }
    else
    {
        lv_roller_set_options(ui_RollerWiFi, Set.WiFi_scan_all.c_str(),
        LV_ROLLER_MODE_NORMAL);
    }
}

void WiFi_scan()
{
    int n = WiFi.scanNetworks();
    if (n == 0) 
    {
        lv_roller_set_options(ui_RollerWiFi, "附近无可用WiFi\n", 
        LV_ROLLER_MODE_NORMAL);
    } 
    else
    {
        for (int i = 0; i < n; ++i)
        {
            Set.WiFi_scan_one = WiFi.SSID(i) + "\n";
            Set.WiFi_scan_all += Set.WiFi_scan_one;
        }
        Set.WiFi_button_flag = 0;
    }
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
            Set.WiFi_connect_stauts = 1;
            break;
        }
        else Set.WiFi_connect_stauts = 0;
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    lv_obj_add_flag(ui_SpinnerWiFi, LV_OBJ_FLAG_HIDDEN);
    Set.WiFi_connect_flag = 0;
}

void WiFi_init()
{
    WiFi.begin();
    for(int i = 0; i < Set.WiFi_connect_timeout; ++i)
    {
        uint32_t connect_status = WiFi.status();
        Serial.println(connect_status);
        if(connect_status == WL_CONNECTED)
        {
            Serial.println("success");
            Set.WiFi_connect_stauts = 1;
            break;
        }
        else Set.WiFi_connect_stauts = 0;
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}