//Copyright (c) 2019 aNo研 プリンを見守る技術
//https://github.com/anoken/purin_wo_mimamoru_gijutsu/


#include "esp_camera.h"
#include <WiFi.h>
#include <ssl_client.h>
#include <WiFiClientSecure.h>

const char* ssid = "your_ssid";
const char* passwd = "your_passwd";
const char* host = "notify-api.line.me";
const char* token = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXX";

#define CAMERA_MODEL_M5STACK_WIDE
//"camera_pins.h"は、スケッチ例 ESP32>Camera>CameraWebServer から取得
#include "camera_pins.h"

void setup() {
  Serial.begin(115200);
  Serial.println();
  setup_camera();			//M5Cameraの設定
  setup_wifi();				//Wifiの設定
  delay(1000);
}

void loop() {
  uint32_t data_len = 0;
  camera_fb_t * fb = esp_camera_fb_get();
  Serial.printf("image size%d[byte]:width%d,height%d,format%d\r\n", fb->len, fb->width, fb->height, fb->format);
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  sendLineNotify(fb->buf, fb->len);			//LINE通知

  esp_camera_fb_return(fb);
  delay(100);
}

//Line通知
void sendLineNotify(uint8_t* image_data, size_t image_sz) {
  WiFiClientSecure client;
  if (!client.connect(host, 443))   return;
  int httpCode = 404;
  size_t image_size = image_sz;
  String boundary = "----purin_alert--";
  String body = "--" + boundary + "\r\n";
  String message = "プリンが取られました！！！";
  body += "Content-Disposition: form-data; name=\"message\"\r\n\r\n" + message + " \r\n";
  if (image_data != NULL && image_sz > 0 ) {
    image_size = image_sz;
    body += "--" + boundary + "\r\n";
    body += "Content-Disposition: form-data; name=\"imageFile\"; filename=\"image.jpg\"\r\n";
    body += "Content-Type: image/jpeg\r\n\r\n";
  }
  String body_end = "--" + boundary + "--\r\n";
  size_t body_length = body.length() + image_size + body_end.length();
  String header = "POST /api/notify HTTP/1.1\r\n";
  header += "Host: notify-api.line.me\r\n";
  header += "Authorization: Bearer " + String(token) + "\r\n";
  header += "User-Agent: " + String("M5Stack") + "\r\n";
  header += "Connection: close\r\n";
  header += "Cache-Control: no-cache\r\n";
  header += "Content-Length: " + String(body_length) + "\r\n";
  header += "Content-Type: multipart/form-data; boundary=" + boundary + "\r\n\r\n";
  client.print( header + body);
  Serial.print(header + body);

  bool Success_h = false;
  uint8_t line_try = 3;
  while (!Success_h && line_try-- > 0) {
    if (image_size > 0) {
      size_t BUF_SIZE = 1024;
      if ( image_data != NULL) {
        uint8_t *p = image_data;
        size_t sz = image_size;
        while ( p != NULL && sz) {
          if ( sz >= BUF_SIZE) {
            client.write( p, BUF_SIZE);
            p += BUF_SIZE; sz -= BUF_SIZE;
          } else {
            client.write( p, sz);
            p += sz; sz = 0;
          }
        }
      }
      client.print("\r\n" + body_end);
      Serial.print("\r\n" + body_end);

      while ( client.connected() && !client.available()) delay(10);
      if ( client.connected() && client.available() ) {
        String resp = client.readStringUntil('\n');
        httpCode    = resp.substring(resp.indexOf(" ") + 1, resp.indexOf(" ", resp.indexOf(" ") + 1)).toInt();
        Success_h   = (httpCode == 200);
        Serial.println(resp);
      }
      delay(10);
    }
  }
  client.stop();
}



void setup_wifi() {		 // WiFi network
   Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, passwd);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
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

