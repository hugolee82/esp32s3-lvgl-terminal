#include "data.h"
#include "wifiuser.h"
#include "task.h"
#include "lvgl.h"
#include "ui/src/ui.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>

struct tm timeinfo;

//获取天气
String url = "https://api.seniverse.com/v3/weather/now.json"; // 获取当天天气
String key = "你的key";
String locations = "武汉";
String language = "zh-Hans";
String unit = "c";
String url_2 = "https://api.seniverse.com/v3/weather/daily.json"; // 获取未来天气
String start = "0";
String days = "7";
#define get_days 7
String city,temp,text,humi,last_update;
String data,date;
const char* week_weather[7]={"周日","周一","周二","周三","周四","周五","周六"};

// 从接口返回的 "2026-10-27" 算星期几(0=周日,1=周一...)。
// 这样天气就不再依赖 NTP 有没有对时成功。
static int weekday_from_date(const String &s)
{
    int y = 1970, m = 1, d = 1;
    sscanf(s.c_str(), "%d-%d-%d", &y, &m, &d);
    if(m < 1 || m > 12) m = 1;
    static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if(m < 3) y -= 1;
    int w = (y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;
    return w < 0 ? w + 7 : w;
}

void weather_get()
{
    // 两次请求改成串行:同时开两个 TLS 会话峰值内存很高,
    // 第一个 http.end() 之后再发第二个,能省一整套 mbedTLS 上下文。
    HTTPClient http;
    http.begin(url+"?key="+key+"&location="+locations+"&language="+language+"&unit="+unit);
    int httpCode = http.GET();
    if(httpCode != HTTP_CODE_OK)
    {
        // 非 200 时正文里通常带着原因(例如 key 过期:{"status":"Your membership has been expired..."})
        // 一定要打出来,否则界面只是不刷新,完全看不出是接口的问题
        String err = http.getString();
        Serial.printf("[Weather] 今日天气请求失败 code=%d body=%s\n", httpCode, err.c_str());
        http.end();
        return;
    }
    String response = http.getString();
    http.end();

    HTTPClient http_2;
    http_2.begin(url_2+"?key="+key+"&location="+locations+"&language="+language+"&unit="+unit+"&start="+start+"&days="+days);
    int httpCode_2 = http_2.GET();
    if(httpCode_2 != HTTP_CODE_OK)
    {
        String err_2 = http_2.getString();
        Serial.printf("[Weather] 未来天气请求失败 code=%d body=%s\n", httpCode_2, err_2.c_str());
        http_2.end();
        return;
    }
    String response_2 = http_2.getString();
    http_2.end();

    // 创建 DynamicJsonDocument 对象
    DynamicJsonDocument doc(1024);
    DynamicJsonDocument doc_2(4096);   // 7 天的日预报,512*7 太紧
    // 解析失败就直接放弃,别用空数据把界面刷白
    if(deserializeJson(doc, response))
    {
        Serial.println("[Weather] 今日 JSON 解析失败");
        return;
    }
    if(deserializeJson(doc_2, response_2))
    {
        Serial.println("[Weather] 预报 JSON 解析失败");
        return;
    }
    // 从解析后的 JSON 文档中获取值
    city = doc["results"][0]["location"]["name"].as<String>();
    temp = doc["results"][0]["now"]["temperature"].as<String>();
    text = doc["results"][0]["now"]["text"].as<String>();
    humi = doc["results"][0]["now"]["humidity"].as<String>();
    last_update = doc["results"][0]["last_update"].as<String>();

    data = "";
    for(int i = 0; i < get_days; ++i)
    {
        int wd = weekday_from_date(doc_2["results"][0]["daily"][i]["date"].as<String>());
        if(i == 0) date = "今天";
        else date = week_weather[wd];
        data +=
        date+
        " "+doc_2["results"][0]["daily"][i]["text_day"].as<String>()+
        " "+doc_2["results"][0]["daily"][i]["low"].as<String>()+
        "~"+doc_2["results"][0]["daily"][i]["high"].as<String>()+
        " "+doc_2["results"][0]["daily"][i]["precip"].as<String>()+
        "\n";
    }

    LVGL_LOCK();
    lv_label_set_text_fmt( ui_LabelLocation, "地点:%s", city.c_str());
    lv_label_set_text_fmt(ui_LabelTemp, "%s", temp.c_str());
    lv_label_set_text_fmt( ui_LabelHumidity, "%s%s", humi.c_str(), "%");
    lv_label_set_text_fmt( ui_LabelWeathertoday, "%s", text.c_str());
    lv_label_set_text_fmt( ui_LabelLastupdata, "上次更新时间:%s", last_update.c_str());
    lv_roller_set_options( ui_RollerFutureweather, data.c_str(), LV_ROLLER_MODE_NORMAL );
    LVGL_UNLOCK();
}

void time_get()
{
    configTime(28800, 0,"ntp1.aliyun.com","ntp2.aliyun.com", "ntp3.aliyun.com");
}

void data_update()
{
    // 兜底:WiFi_task 改成按需创建后,开机不再有常驻任务去补连WiFi。
    // data_task 每秒都跑,这里发现已连上就补一次(内部有 time_flag 保护,不会重复对时)。
    if(!Data.time_flag && WiFi.status() == WL_CONNECTED)
    {
        WiFi_on_connected();
    }

    // 天气只要求联网,不再被 getLocalTime() 成功与否卡住。
    static uint32_t last_weather_ms = 0;
    if(Data.weather_flag && WiFi.status() == WL_CONNECTED)
    {
        Data.weather_flag = 0;
        last_weather_ms = millis();
        weather_get();
    }
    // 联网状态下每小时自动刷新一次天气(原来只有连上那一刻和手动按钮)
    if(WiFi.status() == WL_CONNECTED && millis() - last_weather_ms >= 60UL * 60UL * 1000UL)
    {
        Data.weather_flag = 1;
    }

    if(!Data.time_flag) return;

    // getLocalTime() 默认超时 5000ms:在 1 秒循环里会让 data_task 卡成 6 秒一次
    if(getLocalTime(&timeinfo, 50))
    {
        const char* week[7]={"星期日","星期一","星期二","星期三","星期四","星期五","星期六"};
        int today = (timeinfo.tm_year + 1900) * 10000 + (timeinfo.tm_mon + 1) * 100 + timeinfo.tm_mday;

        LVGL_LOCK();
        lv_label_set_text_fmt(ui_Label3,"%02d:%02d:%02d",timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec);
        lv_label_set_text_fmt(ui_Label4,"%d/%02d/%02d %s",timeinfo.tm_year+1900,timeinfo.tm_mon+1,timeinfo.tm_mday,week[timeinfo.tm_wday]);

        // 日历/日期 label 只在跨天时更新一次,别每秒重设(会触发日历重绘)
        static int last_day = -1;
        if(today != last_day)
        {
            last_day = today;
            lv_label_set_text_fmt(ui_LabelCalendartoday,"%d年%02d月%02d日 %s",timeinfo.tm_year+1900,timeinfo.tm_mon+1,timeinfo.tm_mday,week[timeinfo.tm_wday]);
            lv_calendar_set_today_date(ui_Calendar1,timeinfo.tm_year+1900,timeinfo.tm_mon+1,timeinfo.tm_mday);
            lv_calendar_set_showed_date(ui_Calendar1,timeinfo.tm_year+1900,timeinfo.tm_mon+1);
        }
        LVGL_UNLOCK();
    }
}