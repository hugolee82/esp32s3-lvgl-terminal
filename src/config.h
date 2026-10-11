#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <string.h>

typedef struct {
    int screen_light;
    String WiFi_scan_one;
    String WiFi_scan_all;
    bool WiFi_button_flag;
    char WiFi_name[64];
    const char *WiFi_password;
    bool WiFi_connect_stauts;
    bool WiFi_connect_flag;
    int WiFi_connect_timeout;
}cfg_set;

extern cfg_set Set;

#endif