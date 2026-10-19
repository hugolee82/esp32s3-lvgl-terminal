#include "config.h"

cfg_set Set = {
    .screen_light = 100,
    .WiFi_scan_one = "",
    .WiFi_scan_all = "",
    .WiFi_button_flag = true,
    .WiFi_name = {'\0'},
    .WiFi_password = "" ,
    .WiFi_connect_status = false,
    .WiFi_connect_flag = false,
    .WiFi_connect_timeout = 10 * 5
};

cfg_data Data = {
    .time_flag = false,
    .weather_flag = false
};

cfg_serial Uart = {
    .tx_data = "",
    .rx_data = ""
};

cfg_music Music = {
    .song_name = {'\0'},
    .song_list = "",
    .song_options = "",
    .play_flag = false,
    .start_flag = false,
    .pause_flag = false,
    .volume_value = 16
};