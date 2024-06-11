#include <ESP8266HTTPClient.h>
#include <FastLED.h>
#include <WiFiManager.h>
#include <EEPROM.h>

// FastLed
#define NUM_LEDS 7
#define DATA_PIN D7
#define LED_TYPE WS2812B
#define BRIGHTNESS 2

// Glucose level value
#define GETTING_LOW 80
#define LOW 60
#define VERY_LOW 50
#define GETTING_HIGH 150
#define HIGH 200
#define VERY_HIGH 300

// Dexcom Share API base urls
// US URL
//#define DEXCOM_BASE_URL "https://share2.dexcom.com/ShareWebServices/Services/"

// Outside US URL
#define DEXCOM_BASE_URL "https://shareous1.dexcom.com/ShareWebServices/Services/"

// Dexcom Share API endpoints
#define DEXCOM_LOGIN_ID_ENDPOINT "General/LoginPublisherAccountById"
#define DEXCOM_AUTHENTICATE_ENDPOINT "General/AuthenticatePublisherAccount"
#define DEXCOM_GLUCOSE_READINGS_ENDPOINT "Publisher/ReadPublisherLatestGlucoseValues"

#define DEXCOM_APPLICATION_ID "d89443d2-327c-4a6f-89e5-496bbb0317db"

struct DexcomConfig {
  unsigned long initialized; // Value must be 123456
  char userName[60];
  char password[30];
};

// Constants
const unsigned long timerDelay = 5 * 60 * 1000; // 5 minutes timer.

// Global variables
unsigned long lastTime = 0;
String account_id;
CRGB leds[NUM_LEDS];
int lgl = 0;
bool blink = false;
CRGB color;
DexcomConfig config;
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

String post_request(String json, String endPoint) {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.begin(client, DEXCOM_BASE_URL + endPoint);
  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.POST(json);
  Serial.println(httpResponseCode);
  if (httpResponseCode > 0) {
    String response = http.getString();
    http.end();
    return response;
  }
  Serial.println("error on sending POST");
  return "";
}

void init_account_id() {
  String json = String("{") +
                "\"accountName\":" + "\"" + config.userName + "\"," +
                "\"password\":" + "\"" + config.password + "\"," +
                "\"applicationId\":" + "\"" + DEXCOM_APPLICATION_ID +
                "\"}";
  account_id = post_request(json, DEXCOM_AUTHENTICATE_ENDPOINT);
  Serial.println("Account id: " + account_id);
}

String get_session_id() {
  String json = String("{") +
           "\"accountId\":" + account_id + "," +
           "\"password\":" + "\"" + config.password + "\"," +
           "\"applicationId\":" + "\"" + DEXCOM_APPLICATION_ID +
           "\"}";
  String session_id = post_request(json, DEXCOM_LOGIN_ID_ENDPOINT);
  session_id = session_id.substring(1, session_id.length() - 1);
  Serial.println("Session id: " + session_id);
  return session_id;
}

int get_latest_glucose_level() {
  String session_id = get_session_id();
  String latestGlucoseLevel = post_request("{}", DEXCOM_GLUCOSE_READINGS_ENDPOINT + String("?sessionId=" + session_id + "&minutes=20&maxCount=1"));
  Serial.println("Latest glucose level: " + latestGlucoseLevel);
  int index = latestGlucoseLevel.indexOf("\"Value\":");
  if (index > 0) {
    latestGlucoseLevel = latestGlucoseLevel.substring(index + 8, latestGlucoseLevel.indexOf(',', index));
    int lgl = latestGlucoseLevel.toInt();
    return lgl;
  }
  return -1;
}

void connect_to_WIFI() {
  WiFiManager wm;
  
  //set config save notify callback
  wm.setSaveConfigCallback(saveConfigCallback);

  // Load Dexcom userName and password from EEPROM
  EEPROM.begin(1024);
  EEPROM.get(0, config);
  if (config.initialized != 123456) { // Check whether config values were already saved
    wm.resetSettings();
    strcpy(config.userName, "");
    strcpy(config.password, "");
  }

  // Custom parameters
  WiFiManagerParameter dexcomUserName("userName", "Dexcom user name", config.userName, 60);
  WiFiManagerParameter dexcomPassword("password", "Dexcom password", config.password, 30);
  wm.addParameter(&dexcomUserName);
  wm.addParameter(&dexcomPassword);

  if(!wm.autoConnect("AutoConnectAP")) {
    Serial.println("Failed to connect");
  } else {
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }

  //save the custom parameters
  if (shouldSaveConfig) {
    if (strcmp(config.userName, dexcomUserName.getValue()) != 0 || strcmp(config.password, dexcomPassword.getValue()) != 0) {
      strcpy(config.userName, dexcomUserName.getValue());
      strcpy(config.password, dexcomPassword.getValue());
      config.initialized = 123456;
      EEPROM.put(0, config);
      EEPROM.commit();
    }
  }

  EEPROM.end();
  
  // Dexcom account ID
  init_account_id();
  if (account_id.indexOf("AccountPasswordInvalid") != -1) {
    wm.resetSettings();
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial);

  // Connect to WiFi
  connect_to_WIFI();
  
  // FastLED
  FastLED.addLeds<LED_TYPE, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
}

void loop() {
  // Check latest glucose level every timerDelay
  if (lastTime == 0 || ((millis() - lastTime) > timerDelay)) {
    lgl = get_latest_glucose_level();
    Serial.println("Latest glucose level (int): " + String(lgl));
    blink = false;
    if (lgl > 0) {
      if (lgl < VERY_LOW) {
        color = CRGB::Red;
        blink = true;
      } else if (lgl < LOW) {
        color = CRGB::Red;
      } else if (lgl < GETTING_LOW) {
        color = CRGB::Orange;
      } else if (lgl < GETTING_HIGH) {
        color = CRGB::Green;
      } else if (lgl < HIGH) {
        color = CRGB::Blue;
      } else if (lgl < VERY_HIGH) {
        color = CRGB::Purple;
      } else {
        color = CRGB::Purple;
        blink = true;
      }
    } else {
      Serial.println("No glucose level value available");
      color = CRGB::White;
    }
    fill_solid(leds, NUM_LEDS, color);
    FastLED.show();
    lastTime = millis();
  }
  if (blink) {
    FastLED.delay(1000);
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    FastLED.delay(1000);
  }
  fill_solid(leds, NUM_LEDS, color);
  FastLED.show();
  delay(100);
}
