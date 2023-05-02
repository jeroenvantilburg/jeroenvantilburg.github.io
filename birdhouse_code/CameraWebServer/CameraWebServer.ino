#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <driver/rtc_io.h> // needed for rtc_gpio_isolate

//
// WARNING!!! PSRAM IC required for UXGA resolution and high JPEG quality
//            Ensure ESP32 Wrover Module or other board with PSRAM is selected
//            Partial images will be transmitted if image exceeds buffer size
//

// Select camera model
//#define CAMERA_MODEL_WROVER_KIT // Has PSRAM
//#define CAMERA_MODEL_ESP_EYE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_PSRAM // Has PSRAM
//#define CAMERA_MODEL_M5STACK_V2_PSRAM // M5Camera version B Has PSRAM
//#define CAMERA_MODEL_M5STACK_WIDE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_ESP32CAM // No PSRAM
#define CAMERA_MODEL_AI_THINKER // Has PSRAM
//#define CAMERA_MODEL_TTGO_T_JOURNAL // No PSRAM

#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */

#include "camera_pins.h"

#define FLASH_GPIO_NUM 4
#define IR_LED_pin 2

const char* ssid = "XXXXXXXXXXXXXXX";
const char* password = "XXXXXXXXXXXXX";

String serverName = "raspberrypi.home";      // REPLACE WITH YOUR Raspberry Pi IP ADDRESS
String uploadPath = "/esp32/upload.php";     // The default uploadPath should be upload.php
String intervalPath = "/esp32/interval.txt"; // The default intervalPath should be interval.txt
const int serverPort = 80;
WiFiClient client;

RTC_DATA_ATTR int bootCount = 0;

int timerInterval = -1;    // time between each HTTP POST image. START WITH -1 IN CASE OF NO CONNECTION
unsigned long previousMillis = 0;   // last time image was sent

void startCameraServer();

void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}

void getIntervalFromServer() {
  HTTPClient http;
  //http.begin("http://raspberrypi.home/esp32/interval.txt"); //Specify the URL
  http.begin(String("http://") + serverName + intervalPath); // Specify the URL
  //Serial.println("http://" + serverName + intervalPath);
  int httpCode = http.GET();             // Make the request
  if (httpCode > 0) { //Check for the returning code
    String payload = http.getString();
    Serial.println("HTTP code  : " + httpCode);
    Serial.println("Interval(s):  " + payload);
    timerInterval = payload.toInt();
  } else {
    Serial.println("Error on HTTP request");
  }
  http.end(); //Free the resources  
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  //Increment boot number and print it every reboot
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));
  print_wakeup_reason();

  // Make sure to switch off flash light
  pinMode(FLASH_GPIO_NUM, OUTPUT);
  //digitalWrite(FLASH_GPIO_NUM, HIGH);
  //delay(200);
  digitalWrite(FLASH_GPIO_NUM, LOW);
  
  // set GPIO 2 for the IR LED
  pinMode(IR_LED_pin, OUTPUT);

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  
  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1); // flip it back
    s->set_brightness(s, 1); // up the brightness just a bit
    s->set_saturation(s, -2); // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  s->set_framesize(s, FRAMESIZE_QVGA);

#if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  getIntervalFromServer();
  if( timerInterval < 0 ) {

    digitalWrite(IR_LED_pin, HIGH); // Switch on IR LED
    startCameraServer();

    Serial.print("Camera Ready! Use 'http://");
    Serial.print(WiFi.localIP());
    Serial.println("' to connect");
  } else {
    sendPhoto();
    esp_sleep_enable_timer_wakeup(timerInterval * uS_TO_S_FACTOR);
    Serial.println("Setup ESP32 to sleep for every " + String(timerInterval) + " seconds");
    delay(1000);
    Serial.println("Going to sleep now");
    Serial.flush(); 
    digitalWrite(FLASH_GPIO_NUM, LOW); // Extra measure to switch off flash light
    rtc_gpio_isolate(GPIO_NUM_4);  // See: https://forum.arduino.cc/t/esp32-onboard-led-flash-lighting-up-in-deep-sleep/1039467/4
    esp_deep_sleep_start();
    Serial.println("This will never be printed");
  }  
}

void loop() {
  getIntervalFromServer();
  if( timerInterval > 0 ) {
    Serial.println("Time interval changed to positive " + String(timerInterval) + " seconds");
    Serial.println("Restarting ESP");
    digitalWrite(IR_LED_pin, LOW); // Switch off IR LED
    ESP.restart();
  }
  delay(5000);
}

String sendPhoto() {
  String getAll;
  String getBody;

  digitalWrite(IR_LED_pin, HIGH); // Switch on IR LED

  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();
  digitalWrite(IR_LED_pin, LOW); // Switch off IR LED

  if(!fb) {
    Serial.println("Camera capture failed");
    delay(1000);
    ESP.restart();
  }
  
  Serial.println("Connecting to server: " + serverName);

  if (client.connect(serverName.c_str(), serverPort)) {
    Serial.println("Connection successful!");    
    String head = "--RandomNerdTutorials\r\nContent-Disposition: form-data; name=\"imageFile\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--RandomNerdTutorials--\r\n";

    uint32_t imageLen = fb->len;
    uint32_t extraLen = head.length() + tail.length();
    uint32_t totalLen = imageLen + extraLen;
  
    client.println("POST " + uploadPath + " HTTP/1.1");
    client.println("Host: " + serverName);
    client.println("Content-Length: " + String(totalLen));
    client.println("Content-Type: multipart/form-data; boundary=RandomNerdTutorials");
    client.println();
    client.print(head);
  
    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
    for (size_t n=0; n<fbLen; n=n+1024) {
      if (n+1024 < fbLen) {
        client.write(fbBuf, 1024);
        fbBuf += 1024;
      }
      else if (fbLen%1024>0) {
        size_t remainder = fbLen%1024;
        client.write(fbBuf, remainder);
      }
    }   
    client.print(tail);
    
    esp_camera_fb_return(fb);
    
    int timoutTimer = 10000;
    long startTimer = millis();
    boolean state = false;
    
    while ((startTimer + timoutTimer) > millis()) {
      Serial.print(".");
      delay(100);      
      while (client.available()) {
        char c = client.read();
        if (c == '\n') {
          if (getAll.length()==0) { state=true; }
          getAll = "";
        }
        else if (c != '\r') { getAll += String(c); }
        if (state==true) { getBody += String(c); }
        startTimer = millis();
      }
      if (getBody.length()>0) { break; }
    }
    Serial.println();
    client.stop();
    Serial.println(getBody);
  }
  else {
    getBody = "Connection to " + serverName +  " failed.";
    Serial.println(getBody);
  }
  return getBody;
}
