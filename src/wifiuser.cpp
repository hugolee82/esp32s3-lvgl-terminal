#include "wifiuser.h"
#include "lvgl.h"
#include "ui/src/ui.h"
#include <WiFi.h>
#include "data.h"

void WiFi_update() 
{    
    if(Set.WiFi_connect_flag)
    {
        WiFi_connect();
        Set.WiFi_connect_flag = 0;
    }
    else
    {
        // 修复加载的组件在连接完成后会残留文件问题
        if(!lv_obj_has_flag(ui_SpinnerWiFi, LV_OBJ_FLAG_HIDDEN))
        {
            lv_obj_add_flag(ui_SpinnerWiFi, LV_OBJ_FLAG_HIDDEN);
        }
        lv_obj_invalidate(ui_SpinnerWiFi);
        lv_obj_invalidate(lv_obj_get_parent(ui_SpinnerWiFi));
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }

    if(Set.WiFi_button_flag == 1)
    {
        lv_roller_set_options(ui_RollerWiFi, "正在扫描附近WiFi...\n", LV_ROLLER_MODE_NORMAL);
        WiFi_scan();
        Set.WiFi_button_flag = 0;
    }
    else
    {
        lv_roller_set_options(ui_RollerWiFi, Set.WiFi_scan_all.c_str(), LV_ROLLER_MODE_NORMAL);
    }
}

void WiFi_scan()
{
    int n = WiFi.scanNetworks();
    if (n == 0) 
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
        // 修复WiFi列表第一项没有读取到的问题
        strcpy(Set.WiFi_name, WiFi.SSID(0).c_str());
    }
    if(!Set.WiFi_connect_status)WiFi_init(); 
}

void WiFi_connect()
{
    WiFi.begin(Set.WiFi_name, Set.WiFi_password);
    for(int i = 0; i < Set.WiFi_connect_timeout; ++i)
    {
        uint32_t connect_status = WiFi.status();
        // Serial.println(connect_status);
        if(connect_status == WL_CONNECTED)
        {
        	// Serial.println("success");
        	Set.WiFi_connect_status = 1;
        	break;
        }
        else Set.WiFi_connect_status = 0;
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void WiFi_init()
{
    WiFi.mode(WIFI_STA);
    WiFi.begin();
    for(int i = 0; i < Set.WiFi_connect_timeout; ++i)
    {
        uint32_t connect_status = WiFi.status();
        // Serial.println(connect_status);
        if(connect_status == WL_CONNECTED)
        {
        	// Serial.println("success");
        	Set.WiFi_connect_status = 1;
        	break;
        }
        else Set.WiFi_connect_status = 0;
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}