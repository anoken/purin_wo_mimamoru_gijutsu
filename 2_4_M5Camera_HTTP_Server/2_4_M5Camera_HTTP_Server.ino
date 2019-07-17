//Copyright (c) 2019 aNo研 プリンを見守る技術
//https://github.com/anoken/purin_wo_mimamoru_gijutsu/

#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include <WiFiClient.h>

const char* ssid = "your_ssid";
const char* passwd = "your_passwd";

#define CAMERA_MODEL_M5STACK_WIDE
//"camera_pins.h"は、スケッチ例 ESP32>Camera>CameraWebServer から取得
#include "camera_pins.h"

WebServer server(80);


void setup() {
  Serial.begin(115200);
  Serial.println();
  setup_camera();
  setup_wifi();

  server.on("/", HTTP_GET, handle_jpg_stream);	//HTTPサーバ起動
  server.on("/jpg", HTTP_GET, handle_jpg);
  server.onNotFound(handleNotFound);
  server.begin();

}

void loop() {
  server.handleClient();			//HTTPサーバ処理

}

void handle_jpg(void) {
  WiFiClient client = server.client();
  camera_fb_t * fb = esp_camera_fb_get();
  printf("should %d, print a image, len: \r\n", fb->len);
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }
  if (!client.connected())  {
    Serial.println("fail ... \n");
    return;
  }
  String response = "HTTP/1.1 200 OK\r\n";
  response += "Content-disposition: inline; filename=m5camera.jpg\r\n";
  response += "Content-type: image/jpeg\r\n\r\n";
  server.sendContent(response);
  client.write((const char *)fb->buf, fb->len);
  esp_camera_fb_return(fb);
  delay(100);
}

void handle_jpg_stream(void) {
  WiFiClient client = server.client();
  String response = "HTTP/1.1 200 OK\r\n";
  response += "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";
  server.sendContent(response);

  while (1)  {
    camera_fb_t * fb = esp_camera_fb_get();
    if (!client.connected())
      break;
    response = "--frame\r\n";
    response += "Content-Type: image/jpeg\r\n\r\n";
    server.sendContent(response);

    client.write((const char *)fb->buf, fb->len);
    server.sendContent("\r\n");
    if (!client.connected())
      break;

    esp_camera_fb_return(fb);

  }
}

void handleNotFound()
{
  String message = "Server is running!\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  server.send(200, "text/plain", message);
}



void setup_wifi() {
  // We start by connecting to a WiFi network
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
