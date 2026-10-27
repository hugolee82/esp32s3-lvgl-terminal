#include "wifiuser.h"
#include "lvgl.h"
#include "ui/src/ui.h"
#include <WiFi.h>
#include "data.h"
#include "task.h"

// 连上之后的一次性初始化
// 漏了它的后果:Data.time_flag 永远是 false → data_update() 整个 if 不成立
// → 时钟不走、天气也不会去请求
void WiFi_on_connected()
{
    if(!Data.time_flag)
    {
        time_get();          // 启动 NTP 对时
        Data.time_flag = 1;
    }
    if(!Data.weather_flag) Data.weather_flag = 1;   // 让 data_task 去拉一次天气
}

void WiFi_update()
{
    static String last_scan;    // 上次真正写进 roller 的列表内容
    static String last_state;   // 上次写进状态 label 的文字

    if(Set.WiFi_connect_flag)
    {
        WiFi_connect();
        Set.WiFi_connect_flag = 0;
    }
    else
    {
        // 修复加载的组件在连接完成后会残留文件问题。
        // 只在"刚隐藏"的那一次重绘一次;原来每 250ms 无条件 invalidate 父对象,
        // 等于把整个设置页反复全屏重绘,既费 CPU 又占 SPI。
        if(!lv_obj_has_flag(ui_SpinnerWiFi, LV_OBJ_FLAG_HIDDEN))
        {
            LVGL_LOCK();
            lv_obj_add_flag(ui_SpinnerWiFi, LV_OBJ_FLAG_HIDDEN);
            lv_obj_invalidate(ui_SpinnerWiFi);
            lv_obj_invalidate(lv_obj_get_parent(ui_SpinnerWiFi));
            LVGL_UNLOCK();
        }
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }

    // 状态一律以 WiFi.status() 为准。
    // 原来的写法用了只升不降的 Set.WiFi_connect_status,路由器掉线后
    // 界面还一直显示"已连接",顺带导致 WiFi_scan() 里的自动重连也不触发。
    // 注意:不要在这里动 ui_ImgButtonWiFi 的 CHECKED 状态,
    // 它的选中态是跟面板显隐绑定的,由 ui_event_ImgButtonWiFi 自己管
    Set.WiFi_connect_status = (WiFi.status() == WL_CONNECTED);
    String state = Set.WiFi_connect_status ? ("已连接 " + WiFi.SSID()) : String("未连接");
    if(state != last_state)   // 内容没变就别重设 label,避免字符串堆分配 + 重绘
    {
        last_state = state;
        LVGL_LOCK();
        lv_label_set_text(ui_LabelWiFIstate, state.c_str());
        LVGL_UNLOCK();
    }

    if(Set.WiFi_button_flag == 1)
    {
        LVGL_LOCK();
        lv_roller_set_options(ui_RollerWiFi, "正在扫描附近WiFi...\n", LV_ROLLER_MODE_NORMAL);
        LVGL_UNLOCK();
        last_scan = "正在扫描附近WiFi...\n";
        WiFi_scan();
        Set.WiFi_button_flag = 0;
    }
    else if(Set.WiFi_scan_all != last_scan)
    {
        // 只在列表内容真的变了才重设。
        // 原来每 50ms 调一次 lv_roller_set_options,它会把 roller 的选中项/滚动位置
        // 一遍遍重置回第 0 项,看起来就是"roller 自己在反复滑动"。
        last_scan = Set.WiFi_scan_all;
        LVGL_LOCK();
        lv_roller_set_options(ui_RollerWiFi, Set.WiFi_scan_all.c_str(), LV_ROLLER_MODE_NORMAL);
        LVGL_UNLOCK();
    }
}

void WiFi_scan()
{
    Set.WiFi_scan_all = "";   // 每次扫描前先清空,否则列表会一次比一次长
    int n = WiFi.scanNetworks();
    if (n == 0)
    {
        Set.WiFi_scan_all = "附近无可用WiFi\n";
    }
    else
    {
        for (int i = 0; i < n; ++i)
        {
            if(WiFi.SSID(i).length() == 0) continue;   // 隐藏热点没名字,跳过
            Set.WiFi_scan_one = WiFi.SSID(i) + "\n";
            Set.WiFi_scan_all += Set.WiFi_scan_one;
        }
        // 修复WiFi列表第一项没有读取到的问题
        // 用 strncpy 限长,原来 strcpy 遇到超过 63 字符的 SSID 会写爆 WiFi_name[64]
        strncpy(Set.WiFi_name, WiFi.SSID(0).c_str(), sizeof(Set.WiFi_name) - 1);
        Set.WiFi_name[sizeof(Set.WiFi_name) - 1] = '\0';
    }
    // 用真实连接状态判断,重连本身不阻塞(见 WiFi_init)
    if(WiFi.status() != WL_CONNECTED) WiFi_init();
}

void WiFi_connect()
{
    // 没取到 SSID 就别调 begin() 了,否则会把滚轮里的提示文字当热点名,静默失败
    if(Set.WiFi_name[0] == '\0')
    {
        Serial.println("[WiFi] 没选到WiFi名称,放弃连接");
        Set.WiFi_connect_status = 0;
        return;
    }

    Serial.printf("[WiFi] 开始连接 SSID=[%s] 密码长度=%u\n",
                  Set.WiFi_name, (unsigned)strlen(Set.WiFi_password));

    WiFi.begin(Set.WiFi_name, Set.WiFi_password);
    for(int i = 0; i < Set.WiFi_connect_timeout; ++i)
    {
        uint32_t connect_status = WiFi.status();
        if(connect_status == WL_CONNECTED)
        {
        	Set.WiFi_connect_status = 1;
        	Serial.printf("[WiFi] 连接成功 IP=%s\n", WiFi.localIP().toString().c_str());
        	break;
        }
        else Set.WiFi_connect_status = 0;
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    // 3=WL_CONNECTED 6=WL_DISCONNECTED(密码错/信号差) 4=WL_CONNECT_FAILED
    if(!Set.WiFi_connect_status) Serial.printf("[WiFi] 连接失败,最后状态=%d\n", WiFi.status());
    else WiFi_on_connected();
}

void WiFi_init()
{
    WiFi.mode(WIFI_STA);
    // 不带参数:用上次存进 NVS 的 SSID/密码重连。
    // 这里不再忙等(原来最多阻塞 5 秒):连接结果由 data_task 的兜底逻辑
    // 或 WiFi_update() 轮询发现,UI 不会因为等 WiFi 而卡住。
    WiFi.begin();
    Set.WiFi_connect_status = (WiFi.status() == WL_CONNECTED);
    if(Set.WiFi_connect_status) WiFi_on_connected();
}