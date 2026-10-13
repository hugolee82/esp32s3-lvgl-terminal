#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <string.h>
#include <WiFi.h>

typedef struct {
    int screen_light;
    String WiFi_scan_one;
    String WiFi_scan_all;
    bool WiFi_button_flag;
    char WiFi_name[64];
    const char *WiFi_password;
    bool WiFi_connect_status;
    bool WiFi_connect_flag;
    int WiFi_connect_timeout;
}cfg_set;

typedef struct {
    bool time_flag;
    bool weather_flag;
}cfg_data;

extern cfg_set Set;
extern cfg_data Data;

#endif