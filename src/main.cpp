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

static const uint16_t screenWidth  = 320;
static const uint16_t screenHeight = 240;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[ screenWidth * screenHeight / 10 ];

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

/*Read the touchpad*/
void my_touchpad_read( lv_indev_drv_t * indev_driver, lv_indev_data_t * data )
{
    if (touch.available())
    {
        data->state = LV_INDEV_STATE_PR;
        
        /*Set the coordinates*/
        data->point.x = 320-touch.data.y;
        data->point.y = touch.data.x;
    }
    else
    {
        data->state = LV_INDEV_STATE_REL;
    }
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

    lv_disp_draw_buf_init( &draw_buf, buf, NULL, screenWidth * screenHeight / 10 );

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

void lvgl_task(void *pt);
void WiFi_task(void *pt);
void data_task(void *pt);
void serial_task(void *pt);
void music_task(void *pt);
void xiaozhi_task(void *pt);

void setup()
{
    lvgl_setup();

    SD_init();
    music_init();
    ui_init();
    WiFi_init();
    xiaozhi_init();

    xTaskCreatePinnedToCore(lvgl_task, "lvgl_task", 1024*15, NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(WiFi_task, "WiFi_task", 1024*8, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(data_task, "data_task", 1024*8, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(serial_task, "serial_task", 1024*8, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(music_task, "music_task", 1024*5, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(xiaozhi_task, "xiaozhi_task", 1024*5, NULL, 1, NULL, 0);
}

void loop()
{

}

void lvgl_task(void *pt)
{
    while(1)
    {
        lv_timer_handler();  // LVGL 核心刷新
        vTaskDelay(5);       
    }
}

void WiFi_task(void *pt)
{
    while(1)
    {
        WiFi_update();
        vTaskDelay(50);
    }
}

void data_task(void *pt)
{
    while(1)
    {
        data_update();
        vTaskDelay(1000);
    }
}

void serial_task(void *pt)
{
    while(1)
    {
        if(Serial.available()) 
        {
        String RX = Serial.readStringUntil('\n');
        Uart.rx_data = RX.c_str();
        lv_textarea_add_text(ui_TextAreaRX, Uart.rx_data);
        }
    }
}

void music_task(void *pt)
{
    while(1)
    {
        music_update();
        vTaskDelay(5);
    }
}

void xiaozhi_task(void *pt)
{
    while(1)
    {
        xiaozhi_update();
        vTaskDelay(5);
    }
}