//Copyright (c) 2019 aNo研 プリンを見守る技術 
//https://github.com/anoken/purin_wo_mimamoru_gijutsu/

#include "esp_camera.h"
#include <WiFi.h>
#include "FS.h"
#include "SPIFFS.h"
const char* ssid = "your_ssid";
const char* password = "your_passwd";

//"camera_pins.h"は、スケッチ例 ESP32>Camera>CameraWebServer から取得
#define CAMERA_MODEL_M5STACK_WIDE
#include "camera_pins.h"

void setup() {
  Serial.begin(115200);
  Serial.println();
  if (!SPIFFS.begin(true)) {            //SPIFFSの開始
    Serial.println("SPIFFS Mount Failed");
  }
  setup_camera();                       //M5Cameraの設定
}
void loop() {
  capture_camera();                     //M5Cameraの撮影
  delay(1000);
}

void setup_camera() {		         //M5Cameraの設定
  camera_config_t config;

  //M5Camera GPIO 
  config.ledc_channel = LEDC_CHANNEL_0;	/*!< LEDC channel generating XCLK  */
  config.ledc_timer = LEDC_TIMER_0;	    /*!< LEDC timer generating XCLK  */
  config.pin_d0 = Y2_GPIO_NUM;	 	      /*!< GPIO pin D0 line */
  config.pin_d1 = Y3_GPIO_NUM;	 	      /*!< GPIO pin D1 line */
  config.pin_d2 = Y4_GPIO_NUM;	 	      /*!< GPIO pin D2 line */
  config.pin_d3 = Y5_GPIO_NUM;	 	      /*!< GPIO pin D3 line */
  config.pin_d4 = Y6_GPIO_NUM;	 	      /*!< GPIO pin D4 line */
  config.pin_d5 = Y7_GPIO_NUM;	 	      /*!< GPIO pin D5 line */
  config.pin_d6 = Y8_GPIO_NUM;	 	      /*!< GPIO pin D6 line */
  config.pin_d7 = Y9_GPIO_NUM;	 	      /*!< GPIO pin D7 line */
  config.pin_xclk = XCLK_GPIO_NUM;	    /*!< GPIO pin XCLK line */
  config.pin_pclk = PCLK_GPIO_NUM;	    /*!< GPIO pin PCLK line */
  config.pin_vsync = VSYNC_GPIO_NUM;	  /*!< GPIO pin VSYNC line */
  config.pin_href = HREF_GPIO_NUM;	    /*!< GPIO pin HREF line */
  config.pin_sscb_sda = SIOD_GPIO_NUM;	/*!< GPIO pin SDA line */
  config.pin_sscb_scl = SIOC_GPIO_NUM;	/*!< GPIO pin SCL line */
  config.pin_pwdn = PWDN_GPIO_NUM;	    /*!< GPIO pin power down line */
  config.pin_reset = RESET_GPIO_NUM;	  /*!< GPIO pin reset line */
  config.xclk_freq_hz = 20000000;	      /*!< Frequency of XCLK signal*/

  //M5Camera Image Format
  config.pixel_format = PIXFORMAT_JPEG;	/*!< Format of the pixel data*/
  config.frame_size = FRAMESIZE_VGA;	  /*!<imageSize*/
  config.jpeg_quality = 6;		          /*!< JPEG Quality range of 0-63 */
  config.fb_count = 1;	                /*!< Number of frame buffers to be allocated.  */

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
  sensor_t * s = esp_camera_sensor_get();
}


void capture_camera() {
  uint32_t data_len = 0;
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }
  Serial.printf("image size%d[byte]:width%d,height%d,format%d\r\n", fb->len, fb->width, fb->height, fb->format);
  const char *path="/test.jpg";
  File file = SPIFFS.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("- failed to open file for writing");
    return ;
  }
  file.write((const uint8_t *)fb->buf, fb->len);
  file.close();
  esp_camera_fb_return(fb);
}
