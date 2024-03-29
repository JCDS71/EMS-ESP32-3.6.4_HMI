#include "common_Display.h"

LGFX tft;

//Week Days
String weekDays[7]={"Dimanche", "Lundi", "Mardi", "Mercredi", "Jeudi", "Vendredi", "Samedi"};

//Month names
String months[12]={"Janvier", "Fevrier", "Mars", "Avril", "Mai", "Juin", "Juillet", "Aout", "Septembre", "Octobre", "Novembre", "Decembre"};

char currentDate[30];
char currentTime[10];

// Allocate the JSON document
//JsonDocument doc;

/* #ifndef FS_BUFFER_SIZE
#define FS_BUFFER_SIZE 8192 // JCDS
#endif */

float currentTemp;
const char *currentMode;
char *Hc1_Mode = "auto";

const char *datetime;
const char *dateTimeRaw;

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;


//DynamicJsonDocument  doc = DynamicJsonDocument(FS_BUFFER_SIZE);;


/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.writePixels((lgfx::rgb565_t *)&color_p->full, w * h);
  tft.endWrite();

  lv_disp_flush_ready(disp);
}

void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
  int pos[2] = {0, 0};

  ft6236_pos(pos);
  if (pos[0] > 0 && pos[1] > 0)
  {
    data->state = LV_INDEV_STATE_PR;
    //    data->point.x = tft.width()-pos[1];
    //    data->point.y = pos[0];
    data->point.x = tft.width() - pos[1];
    data->point.y = pos[0];
    //Serial.printf("x-%d,y-%d\n", data->point.x, data->point.y);
  }
  else
  {
    data->state = LV_INDEV_STATE_REL;
  }
}

void touch_init()
{
  // I2C init
  Wire.begin(SDA_FT6236, SCL_FT6236);
  byte error, address;

  Wire.beginTransmission(i2c_touch_addr);
  error = Wire.endTransmission();

  if (error == 0)
  {
    Serial.print("I2C device found at address 0x");
    Serial.print(i2c_touch_addr, HEX);
    Serial.println("  !");
  }
  else if (error == 4)
  {
    Serial.print("Unknown error at address 0x");
    Serial.println(i2c_touch_addr, HEX);
  }
}


void displayInit(void)
{
  Serial.begin(115200); /* prepare for possible serial debug */
  //Buzzer
  pinMode(20, OUTPUT);
  digitalWrite(20, LOW);

  tft.begin();        /* TFT init */
  tft.setRotation(1); /* Landscape orientation, flipped */
  tft.fillScreen(TFT_BLACK);
  delay(500);
  pinMode(LCD_BL, OUTPUT);
  digitalWrite(LCD_BL, HIGH);

  touch_init();

  lv_init();
  lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * screenHeight / 5);

  /*Initialize the display*/
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  /*Change the following line to your display resolution*/
  disp_drv.hor_res = screenWidth;
  disp_drv.ver_res = screenHeight;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  /*Initialize the (dummy) input device driver*/
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register(&indev_drv);

  ui_init();
  Serial.println("************************************************Setup done*******************************************************");

  // Wifi connection
  ESP_ERROR_CHECK( nvs_flash_init() );
  //wifi_connection();

  vTaskDelay(5000 / portTICK_PERIOD_MS);
  //printf("WIFI was initiated ...........\n\n");

 /*  if (wifiConnected)
  {
    xTaskCreate(&updateValues, "updateValues", 4096, NULL, 5, NULL);
    //xTaskCreate(&getTimeNow, "getTimeNow", 4096, NULL, 10, NULL);
  } */
}

/* esp_err_t client_event_get_handler(esp_http_client_event_handle_t evt)
{
  DeserializationError error;

    switch (evt->event_id)
    {
    case HTTP_EVENT_ON_DATA:
        //printf("HTTP_EVENT_ON_DATA: %d\n", evt->data_len);
        
        if (!esp_http_client_is_chunked_response(evt->client))
        {
          //printf("NOT CHUNKED :\n%.*s", evt->data_len, (char*)evt->data);
          ESP_LOGI(TAG, "NOT CHUNKED :\n%.*s", evt->data_len, (char*)evt->data);
        }
        //printf("\n");
       
        error = deserializeJson(doc, evt->data);

        if (error)
        {
            ESP_LOGE(TAG, "DeserializeJson() failed: %s", error.c_str());
            //printf("DeserializeJson() failed: ");
            //printf(error.c_str());
        }
        //Serial.println();

        currentTemp = doc["hc1 current room temperature"].as<float>();
        currentMode = doc["hc1 mode"];
        
        //doc["hc1 current room temperature"].as<String>().toCharArray(temp, 10);
        
        //ESP_LOGI(TAG, "Current room temp : %.1f", currentTemp);
        //ESP_LOGI(TAG, "Current mode : %s", currentMode);
        Serial.print("HC1 : ");
        Serial.println(currentTemp,1);
        Serial.print("MODE : ");
        Serial.println(currentMode);
       break;
    case HTTP_EVENT_ON_CONNECTED:
        //ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_ERROR:
        //ESP_LOGI(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_FINISH:
        //ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
        break;
    case HTTP_EVENT_DISCONNECTED:
        //ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
        break;

    default:
        break;
    }
    return ESP_OK;
} */

/* static void rest_get()
{
  // Http client Get Method
  esp_http_client_set_url(client, "http://192.168.1.185/api/thermostat/info");
  esp_http_client_set_method(client, HTTP_METHOD_GET);
  esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }
}

static void rest_post()
{
  char post_data[50];
  sprintf(post_data, "{\"cmd\":\"mode\", \"data\":\"%s\"}", Hc1_Mode);

  esp_http_client_set_url(client, "http://192.168.1.185/api/thermostat");
  esp_http_client_set_method(client, HTTP_METHOD_POST);
  esp_http_client_set_post_field(client, post_data, strlen(post_data));
  esp_http_client_set_header(client, "Content-Type", "application/json");

  esp_err_t err = esp_http_client_perform(client);
  if (err == ESP_OK) {
      ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %d",
              esp_http_client_get_status_code(client),
              esp_http_client_get_content_length(client));
  } else {
      ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
  }
} */

/* static void updateValues(void *pvParameters)
{
  while(1)
  {
    esp_http_client_config_t config_get = {
    .url = "http://192.168.1.185/api/thermostat",
    .cert_pem = NULL,
    .event_handler = client_event_get_handler};
      
    client = esp_http_client_init(&config_get);

    // Set values to Rest API by POST if changed
    if(currentMode && Hc1_Mode)
    {
      //ESP_LOGI(TAG, "HC1_MODE = %s current Mode = %s", Hc1_Mode, currentMode);
      if(strcmp(Hc1_Mode, currentMode) != 0)
      {
        ESP_LOGI(TAG, "**************  POST **********************");
        rest_post();
      }
    }

    // Get values from Rest API
    rest_get();
    esp_http_client_cleanup(client);

    // Update temperature display
    lv_event_send(ui_Label_Celsius, LV_EVENT_REFRESH, NULL);

    // Update current Mode (Auto, Day Night)
    if(strcmp(currentMode, "night") == 0)
    {
      //lv_event_send(ui_ButtonNight, LV_EVENT_CLICKED, NULL);
      lv_obj_add_state(ui_ButtonNight, LV_STATE_CHECKED);
    }
    if(strcmp(currentMode, "day") == 0)
    {
      //lv_event_send(ui_ButtonDay, LV_EVENT_CLICKED, NULL);
      lv_obj_add_state(ui_ButtonDay, LV_STATE_CHECKED);
    }
    if(strcmp(currentMode, "auto") == 0)
    {
      //lv_event_send(ui_ButtonAuto, LV_EVENT_CLICKED, NULL);
      lv_obj_add_state(ui_ButtonAuto, LV_STATE_CHECKED);
    }

    ESP_LOGI(TAG, "waiting for update\n");

    vTaskDelay(UPDATE_DISPLAY_PERIOD / portTICK_PERIOD_MS);
  }
} */