//Copyright (c) 2019 aNo研 プリンを見守る技術
//https://github.com/anoken/purin_wo_mimamoru_gijutsu/

#include <M5StickC.h>
#include <WiFi.h>
#include "SPIFFS.h"
#include "AudioFileSourceSPIFFS.h"
#include "AudioFileSourceID3.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"
AudioGeneratorMP3 *mp3;
AudioFileSourceSPIFFS *file;
AudioOutputI2S *out;
AudioFileSourceID3 *id3;
int mp3_do = 0;
void spiffs_mp3_init() {    //MP3 loading From SPIFFS
  SPIFFS.begin();
  Serial.printf("Sample MP3 play...\n");
  file = new AudioFileSourceSPIFFS("/sound.mp3");
  id3 = new AudioFileSourceID3(file);
  out = new AudioOutputI2S(0, 1);
  out->SetOutputModeMono(true);
  out->SetGain(2.0);
  mp3 = new AudioGeneratorMP3();
  mp3->begin(id3, out);
  mp3_do = 1;
}
void setup() {
  Serial.begin(115200);
  delay(1000);
  spiffs_mp3_init();
}
void loop() {        //Play MP3
  if (mp3_do) {
    if (mp3->isRunning()) {
      if (!mp3->loop()) {
        mp3->stop();
        mp3_do = 0;
      }
    }
  }
}
