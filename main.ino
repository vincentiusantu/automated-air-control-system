#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <DHT.h>

//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert your network credentials
#define WIFI_SSID ""
#define WIFI_PASSWORD ""

// Insert Firebase project API Key
#define API_KEY ""

// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "" 

#define DHT_PIN 13

//Define Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;

//sensor and get data from firebase
DHT dht(DHT_PIN, DHT11);

int level_state = 0;

// pin dc motor
int motor1Pin1 = 26; 
int motor1Pin2 = 25; 
int enable1Pin = 33;

// PWM properties
const int freq = 20000;
const int pwmChannel = 0;
const int resolution = 8;

void setup(){
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // pin motor
  pinMode(motor1Pin1, OUTPUT);
  pinMode(motor1Pin2, OUTPUT);
  pinMode(enable1Pin, OUTPUT);
  
  // configure PWM functionalitites
  ledcSetup(pwmChannel, freq, resolution);
  
  // attach the channel to the GPIO to be controlled
  ledcAttachPin(enable1Pin, pwmChannel);
}

void loop(){
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  int dutyCycle = 0;

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read data from DHT sensor");
  } 
  else {
    // Serial.print("Humidity: ");
    // Serial.print(humidity);
    // Serial.print("%  Temperature: ");
    // Serial.print(temperature);
    // Serial.println("°C");

    // Send data to Firebase
    Firebase.RTDB.setFloat(&fbdo, "/data-sensor/hum", humidity);
    Firebase.RTDB.setFloat(&fbdo, "/data-sensor/temp", temperature);
    
    //getting data from Firebase
    if (Firebase.RTDB.getInt(&fbdo, "/data-actuator/level")){
      level_state = fbdo.intData();
      //Serial.print("succesfull"+fbdo.dataPath()+""+level_state);
    
      if (level_state == 3){
        dutyCycle = 225;
        digitalWrite(motor1Pin1, HIGH);
        digitalWrite(motor1Pin2, LOW); 
        ledcWrite(pwmChannel, dutyCycle);
        Serial.print("Strong\n");
      }
      else if (level_state == 2){
        dutyCycle = 200;
        digitalWrite(motor1Pin1, HIGH);
        digitalWrite(motor1Pin2, LOW); 
        ledcWrite(pwmChannel, dutyCycle);
        Serial.print("Normal\n");
      }
      else if (level_state == 1){
        dutyCycle = 180;
        digitalWrite(motor1Pin1, HIGH);
        digitalWrite(motor1Pin2, LOW); 
        ledcWrite(pwmChannel, dutyCycle);
        Serial.print("Low\n");
      }
      else if(level_state == 0) {
        dutyCycle = 0;
        digitalWrite(motor1Pin1, HIGH);
        digitalWrite(motor1Pin2, LOW); 
        ledcWrite(pwmChannel, dutyCycle);
        Serial.print("Off\n");
      }
    }
    else {
      Serial.print("Failed\n");
    }
  }
  delay(5000); // Delay for 5 seconds before sending the next data
}


