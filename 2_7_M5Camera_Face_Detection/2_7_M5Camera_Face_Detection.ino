//Copyright (c) 2019 aNo研 プリンを見守る技術
//https://github.com/anoken/purin_wo_mimamoru_gijutsu/

#include <WiFi.h>
#include <WebServer.h>
#include <WiFiClient.h>
#include "esp_camera.h"

#define CAMERA_MODEL_M5STACK_WIDE
//"camera_pins.h"は、スケッチ例 ESP32>Camera>CameraWebServer から取得
#include "camera_pins.h"

//wifiの設定
const char* ssid = "your_ssid";
const char* passwd = "your_passwd";

//esp-whoのライブラリ
#include "fb_gfx.h"
#include "fd_forward.h"
#include "dl_lib.h"
#include "fr_forward.h"

//カメラ画像のバッファ
camera_fb_t *fb = NULL;
//画像処理のバッファ
dl_matrix3du_t *image_matrix = NULL;
//顔検出のパラメータ
static mtmn_config_t mtmn_config = {0};

//webサーバ
WebServer server(80);

void setup() {
  Serial.begin(115200);
  Serial.println();
  setup_camera();
  setup_wifi();

  // 1. 顔検出の設定パラメータを入力
  mtmn_config.min_face = 80;
  mtmn_config.pyramid = 0.7;
  mtmn_config.p_threshold.score = 0.6;
  mtmn_config.p_threshold.nms = 0.7;
  mtmn_config.r_threshold.score = 0.7;
  mtmn_config.r_threshold.nms = 0.7;
  mtmn_config.r_threshold.candidate_number = 4;
  mtmn_config.o_threshold.score = 0.7;
  mtmn_config.o_threshold.nms = 0.4;
  mtmn_config.o_threshold.candidate_number = 1;

  //webサーバの起動
  server.on("/", HTTP_GET, handle_jpg_stream);
  server.onNotFound(handleNotFound);
  server.begin();
  delay(100);
}

void loop() {
  //webサーバのループ処理
  server.handleClient();
}

void handle_jpg_stream(void) {
  WiFiClient client = server.client();
  String response = "HTTP/1.1 200 OK\r\n";
  response += "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";
  server.sendContent(response);

  uint8_t * _jpg_buf = NULL;
  size_t _jpg_buf_len = 0;

  while (1)  {
    //画像取得
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      break;
    }


    //RGB画像データのメモリ確保
    image_matrix = dl_matrix3du_alloc(1, fb->width, fb->height, 3);
    if (!image_matrix) {
      esp_camera_fb_return(fb);
      Serial.println("dl_matrix3du_alloc failed");
      return;
    }
    //
    uint8_t * out_buf;
    size_t out_len, out_width, out_height;
    out_buf = image_matrix->item;
    out_len = fb->width * fb->height * 3;
    out_width = fb->width;
    out_height = fb->height;

    //画像データをRGBデータへ変換
    uint32_t res = fmt2rgb888(fb->buf, fb->len, fb->format, out_buf);
    if (true != res)        {
      Serial.printf(  "fmt2rgb888 failed, fb: %d\n", fb->len);
      dl_matrix3du_free(image_matrix);
      return;
    }

    //顔検出の実行
    box_array_t *net_boxes = face_detect(image_matrix, &mtmn_config);
    int face_id = 0;

    if (net_boxes) {
      Serial.printf("DETECTED:\n");
      draw_face_boxes(image_matrix, net_boxes, face_id);

      if (!fmt2jpg(out_buf, out_len, out_width, out_height, PIXFORMAT_RGB888, 90, &_jpg_buf, &_jpg_buf_len)) {
        Serial.printf(  "fmt2jpg failed");
      }
      free(net_boxes->box);
      free(net_boxes->landmark);
      free(net_boxes);
    }
    else {
      _jpg_buf_len = fb->len;
      _jpg_buf = fb->buf;
    }
    if (!client.connected())
      break;

    response = "--frame\r\n";
    response += "Content-Type: image/jpeg\r\n\r\n";
    server.sendContent(response);
    //webサーバへ画像の送信
    client.write((const char *)_jpg_buf, _jpg_buf_len);
    server.sendContent("\r\n");
    if (!client.connected()) break;
    dl_matrix3du_free(image_matrix);
    if (fb) {
      esp_camera_fb_return(fb);
      fb = NULL;
      _jpg_buf = NULL;
    } else if (_jpg_buf) {
      free(_jpg_buf);
      _jpg_buf = NULL;
    }
  }
}

static void draw_face_boxes(dl_matrix3du_t *image_matrix, box_array_t *boxes, int face_id) {
  int x, y, w, h, i;
  uint32_t color = 0x000000FF;  //FACE_COLOR_RED
  fb_data_t fb;
  fb.width = image_matrix->w;
  fb.height = image_matrix->h;
  fb.data = image_matrix->item;
  fb.bytes_per_pixel = 3;
  fb.format = FB_BGR888;
  for (i = 0; i < boxes->len; i++) {    // rectangle box
    x = (int)boxes->box[i].box_p[0];
    y = (int)boxes->box[i].box_p[1];
    w = (int)boxes->box[i].box_p[2] - x + 1;
    h = (int)boxes->box[i].box_p[3] - y + 1;
    fb_gfx_drawFastHLine(&fb, x, y, w, color);
    fb_gfx_drawFastHLine(&fb, x, y + h - 1, w, color);
    fb_gfx_drawFastVLine(&fb, x, y, h, color);
    fb_gfx_drawFastVLine(&fb, x + w - 1, y, h, color);
  }
}


void handleNotFound() {
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

