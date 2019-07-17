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
#include "ESP8266FtpServer.h"
//Simple FTP Server for using esp8266/esp32 SPIFFs
//https://github.com/nailbuster/esp8266FTPServer

FtpServer ftpSrv;   // in ESP8266FtpServer.h

void FTP_server_task(void* param) {  //FTP Server Thread
  ftpSrv.begin("esp32", "esp32");//username, password
  while (1) {
    vTaskDelay(10);
    ftpSrv.handleFTP();
  }
}
void setup() {
  Serial.begin(115200);
  Serial.println();
  setup_camera();
  setup_wifi();

  if (!SPIFFS.begin(true)) {    		//SPIFFS 開始
    Serial.println("SPIFFS Mount Failed");
  }
  delay(1000);
  xTaskCreatePinnedToCore(FTP_server_task, "FTP_server_task",
                          4096, NULL, 1, NULL, 1);
}
void loop() {
  const char * path = "/test.jpg";
  capture_camera(path);
  delay(1000);
}

void setup_wifi() {      // WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

}


void capture_camera(const char * path) {	//SPIFFSへの画像保存
  uint32_t data_len = 0;
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }
  Serial.printf("image size%d[byte]:width%d,height%d,format%d\r\n", fb->len, fb->width, fb->height, fb->format);

  File file = SPIFFS.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("- failed to open file for writing");
    return ;
  }
  file.write((const uint8_t *)fb->buf, fb->len);
  file.close();

  esp_camera_fb_return(fb);
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


void listDir(fs::FS &fs, const char * dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\r\n", dirname);
  File root = fs.open(dirname);
  if (!root) {
    Serial.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println(" - not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels) {
        listDir(fs, file.name(), levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("\tSIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}
