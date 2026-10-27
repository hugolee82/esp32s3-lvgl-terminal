#ifndef WIFIUSER_H
#define WIFIUSER_H

#include "config.h"

void WiFi_scan();
void WiFi_update();
void WiFi_connect();
void WiFi_init();
void WiFi_on_connected();   // 连上后的一次性初始化(对时+拉天气),data_task 也会兜底调用

#endif