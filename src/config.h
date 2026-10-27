#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <string.h>
#include <WiFi.h>
#include <lv_games.h>

typedef struct {
    int screen_light;
    String WiFi_scan_one;
    String WiFi_scan_all;
    bool WiFi_button_flag;
    char WiFi_name[64];
    char WiFi_password[64];       // 自己拷一份,别直接指向 LVGL textarea 的内部缓冲
    bool WiFi_connect_status;
    bool WiFi_connect_flag;
    int WiFi_connect_timeout;
}cfg_set;

typedef struct {
    bool time_flag;
    bool weather_flag;
}cfg_data;

typedef struct {
    String tx_data;               // 用 String 存,避免指向临时变量/textarea 缓冲
    String rx_data;
}cfg_serial;

typedef struct {
    String question_text;
    String answer_text;
    String answer_url;
    bool answer_flag;
    bool ask_flag;
}cfg_xiaozhi;

typedef struct {
    char song_name[128];          // 63 字节会切断长中文文件名(UTF-8 每字 3 字节)
    String song_list;
    const char* song_options;
    bool play_flag;
    bool start_flag;
    bool pause_flag;
    int volume_value;
}cfg_music;

extern cfg_set Set;
extern cfg_data Data;
extern cfg_serial Uart;
extern cfg_music Music;
extern cfg_xiaozhi Xiaozhi;

#endif