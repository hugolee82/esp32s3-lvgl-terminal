#ifndef TASK_H
#define TASK_H

#include "config.h"
#include <freertos/semphr.h>

void task_init();
void WiFiTask_create();
void serialTask_create();
void xiaozhiTask_create();
void WiFiTask_delete();
void serialTask_delete();
void xiaozhiTask_delete();


void lvgl_task(void *pt);
void WiFi_task(void *pt);
void data_task(void *pt);
void serial_task(void *pt);
void music_task(void *pt);
void xiaozhi_task(void *pt);

// LVGL 不是线程安全的:lcd 任务在 core1 跑 lv_timer_handler,
// 而 WiFi/data/music/xiaozhi/serial 任务在 core0 直接改对象。
// 所有非 LCD 任务里的 lv_* 调用都要用这把递归锁包起来。
// 注意:不要把 WiFi_scan()/weather_get()/audio 这类阻塞调用包进锁里,
// 否则 LVGL 会跟着卡住不刷新。
extern SemaphoreHandle_t lvgl_mutex;
#define LVGL_LOCK()   xSemaphoreTakeRecursive(lvgl_mutex, portMAX_DELAY)
#define LVGL_UNLOCK() xSemaphoreGiveRecursive(lvgl_mutex)

#endif