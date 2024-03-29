#pragma once
//using namespace std;

/*#ifdef __cplusplus
#include <Arduino.h>
#include <HardwareSerial.h>

extern HardwareSerial Serial;
#endif //__cplusplus*/


//#include "esp_http_client.h" // JCDS

#ifndef COMMON_DISPLAY_H
#define COMMON_DISPLAY_H


#ifdef __cplusplus

#include <WString.h>
#include <Arduino.h>
#include <ArduinoJson.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include <esp_wifi.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"

#include "esp_sntp.h"
#include "lwip/apps/sntp.h"
#include "time.h"
#include "lgfx.hpp"

// LVGL includes
#include "FT6236.h"

// Display interface UI include
#include "UI/ui.h"

#include <uuid/log.h>

#define SSID "Freebox-2.4G"
#define PASS "lafreeboxajcestmagique"

#define ARDUINOJSON_ENABLE_STD_STRING 1
#define MAX_HTTP_OUTPUT_BUFFER 512

/************************************************************************************
                             Declare some variables
**************************************************************************************/

static bool wifiConnected = false;

static const char *TAG = "EMS-ESP";

#define UPDATE_DISPLAY_PERIOD 10000
#define UPDATE_DISPLAY_DATE_TIME 500


//esp_http_client_handle_t client;

/************************************************************************************
                                   Declare some functions
**************************************************************************************/
void displayInit(void);
void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data);
void touch_init();
//static void updateValues(void *pvParameters);
//esp_err_t client_event_get_handler(esp_http_client_event_handle_t evt);
//static void rest_get();
//static void rest_post();



/*Change to your screen resolution*/
static const uint16_t screenWidth = 480;
static const uint16_t screenHeight = 320;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * screenHeight / 5];

//#include <ArduinoJson.h>
//#include <ArduinoJson.h>

//extern String currentTemp;       // La variable est déclarée mais non réservée en mémoire
//extern HardwareSerial Serial;
//extern JsonDocument doc;
//extern char currentTemp[10];
//extern char currentMode[10];

#endif //__cplusplus

extern float currentTemp;
extern const char *currentMode;
extern char *Hc1_Mode;

extern char currentDate[30];
extern char currentTime[10];

#endif // COMMON_DISPLAY_H