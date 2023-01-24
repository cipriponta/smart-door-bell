// -----------------------------------------------------------
//
// Project Name: smart-door-bell
// Author: Ponta Ciprian
//
// -----------------------------------------------------------

// --- Include Statements
#include <WiFi.h>
#include <HTTPClient.h>
#include <UrlEncode.h>
#include "credentials.h"

// --- Define Statements
#define SERIAL_BAUD_RATE                (115200)

#define LOLIN32_LITE_BUILTIN_LED        (22)
#define LOLIN32_LITE_BUILTIN_LED_ON     (LOW)
#define LOLIN32_LITE_BUILTIN_LED_OFF    (HIGH)

#define WIFI_CONNECTION_TIME            (5)

#define INTERRUPT_PIN                   (34)

// --- Data Structures
typedef struct
{
  const char *phoneNumber;
  const char *apiKey;
  const char *message;
  const char *apiUrl;
  const char *wifiSsid;
  const char *wifiPassword;
}credentials_t;

// --- Global Variables
volatile boolean interruptDetected = false;

// --- Function Prototypes
boolean connectToWifi(credentials_t credentials);
void IRAM_ATTR pinISR();
void sendAlert();

// --- Setup and Loop Functions
void setup() 
{
  Serial.begin(SERIAL_BAUD_RATE);

  pinMode(LOLIN32_LITE_BUILTIN_LED, OUTPUT);
  digitalWrite(LOLIN32_LITE_BUILTIN_LED, LOLIN32_LITE_BUILTIN_LED_OFF);

  pinMode(INTERRUPT_PIN, INPUT);
  attachInterrupt(INTERRUPT_PIN, pinISR, FALLING);

  // A credentials header needs to be created in order to 
  // initialize your personal data
  credentials_t credentials = 
  {
    .phoneNumber = PHONE_NUMBER,
    .apiKey = API_KEY,
    .message = MESSAGE,
    .apiUrl = API_URL,
    .wifiSsid = WIFI_SSID,
    .wifiPassword = WIFI_PASSWORD
  };

  if(false == connectToWifi(credentials))
  {
    digitalWrite(LOLIN32_LITE_BUILTIN_LED, LOLIN32_LITE_BUILTIN_LED_ON);
  }
  else
  {
    digitalWrite(LOLIN32_LITE_BUILTIN_LED, LOLIN32_LITE_BUILTIN_LED_OFF);
  }
}

void loop() 
{
  // Mini Scheduler
  static uint8_t wifiCounter = 0; 

  if(wifiCounter == 10)
  {
    if(!WiFi.isConnected())
    {
      digitalWrite(LOLIN32_LITE_BUILTIN_LED, LOLIN32_LITE_BUILTIN_LED_ON);
      Serial.println("WiFi not connected, attempting reconnection");
      WiFi.reconnect();
    }
    else
    {
      digitalWrite(LOLIN32_LITE_BUILTIN_LED, LOLIN32_LITE_BUILTIN_LED_OFF);
    }
    
    wifiCounter = 0;
  }

  if(true == interruptDetected)
  {
    sendAlert();
    interruptDetected = false;
  }

  wifiCounter++;

  delay(1000);
}

// --- Function Definitions
boolean connectToWifi(credentials_t credentials)
{
  Serial.print("Connecting to WiFi");
  WiFi.begin(credentials.wifiSsid, credentials.wifiPassword);
  
  uint8_t u8TimeoutCounter = WIFI_CONNECTION_TIME;

  while(u8TimeoutCounter)
  {
    if(WiFi.isConnected())
    {
      Serial.println();
      Serial.println("Connected to WiFi");
      Serial.print("ESP32 IP: ");
      Serial.println(WiFi.localIP());
      return true;
    }

    Serial.print(".");
    delay(1000);

    u8TimeoutCounter--;
  }

  Serial.println();
  Serial.println("WiFi connection failed");
  return false;
}

void IRAM_ATTR pinISR()
{
  interruptDetected = true;
}

void sendAlert()
{
  String url = String(API_URL) + String("?phone=") + String(PHONE_NUMBER) + 
               String("&text=") + urlEncode(MESSAGE) + String("&apikey=") + String(API_KEY);
  HTTPClient http;

  http.begin(url);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int httpResponse = http.POST(url);

  if(200 == httpResponse)
  {
    Serial.println("Message sent succesfully");
  }
  else 
  {
    Serial.print("HTTP Error: ");
    Serial.println(httpResponse);
  }
  
  http.end();
}