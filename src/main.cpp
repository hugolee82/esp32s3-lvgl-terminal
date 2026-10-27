#include <lvgl.h>
#include <TFT_eSPI.h>
#include <CST816S.h>
#include "config.h"
#include "ui/src/ui.h" 
#include "wifiuser.h"
#include "data.h"
#include "sduser.h"
#include "music.h"
#include "xiaozhi_ai.h"
#include "xiaozhi.h"
#include "task.h"

static const uint16_t screenWidth  = 320;
static const uint16_t screenHeight = 240;

static lv_disp_draw_buf_t draw_buf;
// 缓冲区从 1/10 屏提到 1/5 屏,刷新次数更少,滚动/动画更顺
static lv_color_t buf[ screenWidth * screenHeight / 5 ];

CST816S touch(3, 18, 8, 46); // sda, scl, rst, irq
TFT_eSPI tft = TFT_eSPI(screenWidth, screenHeight); /* TFT instance */

/* Display flushing */
void my_disp_flush( lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p )
{
    uint32_t w = ( area->x2 - area->x1 + 1 );
    uint32_t h = ( area->y2 - area->y1 + 1 );

    tft.startWrite();
    tft.setAddrWindow( area->x1, area->y1, w, h );
    tft.pushColors( ( uint16_t * )&color_p->full, w * h, true );
    tft.endWrite();

    lv_disp_flush_ready( disp_drv );
}

// 触摸调试计数:每秒往串口打一行。
// 改成 1 可以重新打开这些日志(排查幽灵触摸/自己滑动时用)。
#define TOUCH_DEBUG 0

static uint32_t touch_press_cnt = 0;      // 有效按压次数
static uint32_t touch_drop_cnt  = 0;      // 被丢掉的异常上报次数
static uint32_t touch_log_tick  = 0;

/*Read the touchpad*/
void my_touchpad_read( lv_indev_drv_t * indev_driver, lv_indev_data_t * data )
{
    data->state = LV_INDEV_STATE_REL;     // 默认松开

    if (touch.available())
    {
        uint8_t  points = touch.data.points;
        uint16_t raw_x  = touch.data.x;
        uint16_t raw_y  = touch.data.y;

        // CST816S 库在 I2C 读失败时会保留上一次的(甚至是未初始化的)数据,
        // 手指抬起时上报的 points 也是 0。这些都不是有效按压,必须丢掉:
        // 否则 LVGL 会在随机位置收到一次"幽灵点击"——
        // 点落在音乐开关上就是音乐反复暂停,落在 roller 上就是列表自己乱滑。
        // 旋转后 raw_x 是短边(0~239),raw_y 是长边(0~319)
        if (points > 0 && points <= 5 && raw_x < 240 && raw_y < 320)
        {
            data->state = LV_INDEV_STATE_PR;

            /*Set the coordinates*/
            data->point.x = 320 - raw_y;
            data->point.y = raw_x;
            touch_press_cnt++;
        }
        else
        {
            touch_drop_cnt++;
#if TOUCH_DEBUG
            // 每秒最多打印一条被丢掉的上报,方便确认是不是真的在乱报点
            static uint32_t last_drop_log = 0;
            if (millis() - last_drop_log > 1000)
            {
                last_drop_log = millis();
                Serial.printf("[Touch] 丢弃异常上报 points=%u x=%u y=%u\n", points, raw_x, raw_y);
            }
#endif
        }
    }

#if TOUCH_DEBUG
    if (millis() - touch_log_tick >= 1000)
    {
        touch_log_tick = millis();
        if (touch_press_cnt || touch_drop_cnt)
        {
            Serial.printf("[Touch] 有效按压=%lu 丢弃异常=%lu\n",
                          (unsigned long)touch_press_cnt, (unsigned long)touch_drop_cnt);
        }
        touch_press_cnt = 0;
        touch_drop_cnt  = 0;
    }
#endif
}

void lvgl_setup()
{
    Serial.begin( 115200 ); /* prepare for possible serial debug */

    lv_init();
    tft.begin();          /* TFT init */
    tft.setRotation( 3 ); /* Landscape orientation, flipped */

    /*Set the touchscreen calibration data,
     the actual data for your display can be acquired using
     the Generic -> Touch_calibrate example from the TFT_eSPI library*/
    touch.begin();

    lv_disp_draw_buf_init( &draw_buf, buf, NULL, screenWidth * screenHeight / 5 );

    /*Initialize the display*/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init( &disp_drv );
    /*Change the following line to your display resolution*/
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register( &disp_drv );

    /*Initialize the (dummy) input device driver*/
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init( &indev_drv );
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register( &indev_drv );
}

void setup()
{
    lvgl_setup();
    SD_init();
    music_init();
    xiaozhi_init();

    ui_init();
    task_init();
    // 放最后:WiFi_init() 现在不阻塞,但即使以后改回阻塞式,
    // 此时 LVGL 任务已经在跑,界面也不会黑屏好几秒。
    WiFi_init();
}

void loop()
{

}