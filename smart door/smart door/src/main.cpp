#define BLYNK_TEMPLATE_ID "TMPL5aVy0za3i"
#define BLYNK_TEMPLATE_NAME "Smart door"
#define BLYNK_AUTH_TOKEN "2mFRriQ_6tHTa81CwLg6MrZSbTK04alV"

#include <Arduino.h>
#include <Keypad.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SPI.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h> // Ensure this is included for secure WiFi client
#include <time.h> // Include the time library for time functions

// Azure IoT client




BlynkTimer timer;

// Azure IoT Hub connection details
// Azure IoT Hub details
const char* mqttServer = "myiotdevices.azureiotcentral.com"; // Replace with your IoT Hub name
const int mqttPort = 8883; // Azure uses TLS
const char* deviceId = "smart_doors";
const char* mqttUser = "myiotdevices.azureiotcentral.com/smart_doors/?api-version=2021-04-12";
const char* mqttPassword = "WiSlsPRgXxvmZbe9KjD9F6MGx4fLE8wABklCtU22C1I="; // Replace with SAS token

WiFiClientSecure espClient;
PubSubClient client(espClient);
// Initialize WiFi and Azure IoT Hub
//WiFiClientSecure client;
//iothub_client iotHubClient;




// Define ILI9341 Display Pins
#define TFT_CS    5
#define TFT_DC    2
#define TFT_RST   4
#define TFT_MOSI  23
#define TFT_SCK   18

// Virtual Pins
#define VPIN_BUTTON V34
#define VPIN_PIN_DISPLAY V2
#define VPIN_PIR_POWER V3
#define VPIN_PIR_MOTION V4
#define VPIN_DOORSTATUS V1

String generatedPin = "";
unsigned long pinExpiryTime = 0;

unsigned long lastBlinkTime = 0;
unsigned long blinkInterval = 100; // Interval for blinking in milliseconds

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

bool intruderAlertActive = false;  // Flag to track intruder alert status
bool isLocked = true;
bool pir = false;

// Define Keypad
const byte ROWS = 4; // 4 rows
const byte COLS = 4; // 4 columns
char keys[ROWS][COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {32, 33, 25, 26};
byte colPins[COLS] = {27, 14, 12, 13};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Servo Motor (Simulated Lock)
Servo doorLock;
#define SERVO_PIN 21



// Buzzer
#define BUZZER_PIN 19

// RGB LED Pins
#define LED_RED 16
#define LED_GREEN 17
#define LED_BLUE 15
#define LED_YELLOW 0  // Define the pin for the yellow LED

// PIR Sensor
#define PIR_SENSOR 22

// Password Setup
String password = "1234";
String inputPassword = "";
bool doorUnlocked = false;




//Display functions
void displayMessage(String msg) {
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 20);
  tft.print(msg);
} 

void welcomeMsg() {
  tft.fillScreen(ILI9341_WHITE); // Set background to white
  tft.setTextColor(ILI9341_BLACK); // Set text color to black
  tft.setTextSize(2); // Each char: ~12px wide, 16px tall
  tft.setCursor(30, 50);
  tft.print("OTP");
  tft.setCursor(30,70);
  tft.print("smart Lock");
  tft.print("\n");
  tft.setCursor(30,100);
  tft.print("Set up WiFi");
  tft.setCursor(30,120);
  tft.print("to use the device");
  delay(4000);
  tft.setTextColor(ILI9341_WHITE); // Set text color to black
} 

void wifiList(String msg) {
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextSize(2);
  tft.setCursor(20, 40);
  tft.print(msg);
} 

String selectedSSID = "";  // Store selected WiFi network
String wifiPassword = "";  // Store entered WiFi password

bool wifiConnected = false;  // Lock unlock function until WiFi is connected

//function to set up wifi connection
void setupWiFi() {

  
  welcomeMsg();
  delay(1000);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();  // Ensure fresh scan
  delay(1000);

  while (true) {  // Infinite loop to keep retrying if connection fails
    selectedSSID = "";  // Reset selectedSSID before scanning

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();  // Ensure fresh scan
    delay(1000);

    // Scanning WiFi networks
    while (selectedSSID == "") {  // Loop until user selects a network
      displayMessage("Scanning WiFi...");
      int numNetworks = WiFi.scanNetworks();

      Serial.print("Number of networks found: ");
      Serial.println(numNetworks);


      if (numNetworks == 0) {
        displayMessage("No Networks Found");
        delay(2000);
        continue;  // Exit if no networks
      }

      // Display available networks
      displayMessage("Select Network:");
      for (int i = 0; i < numNetworks; i++) {
        tft.setTextSize(2);
        tft.setCursor(20, 40 + (i * 20));  // Adjust cursor position for each network
        tft.print(String(i + 1) + ". " + WiFi.SSID(i));
      }

      unsigned long startTime = millis();
      // Let user select network
      char key;
      while (millis() - startTime < 10000) {  // Wait for 10 seconds for user input
        key = keypad.getKey();
        if (key >= '1' && key <= '9') {
          //doorLock.write(0);
          tone(BUZZER_PIN, 500, 10);
          int index = key - '1';
          if (index < numNetworks) {
            selectedSSID = WiFi.SSID(index);
            displayMessage("Selected: \n" + selectedSSID);
            delay(2000);
            break;
          }
        }
      }

      if (selectedSSID == "") {
        displayMessage("No Network Selected");
        delay(2000);
      }
    }

    // Ask user to enter password
    displayMessage("Enter Password:");
    wifiPassword = "";
    while (true) {
      char key = keypad.getKey();
      if (key) {
        //doorLock.write(0);
        tone(BUZZER_PIN, 500, 10);
        if (key == '#') break;  // Press '#' to confirm password
        else if (key == '*') {
          wifiPassword = "";
          displayMessage("Cleared!");
          displayMessage("Enter Password:");
        } else {
          wifiPassword += key;
          displayMessage("*");  // Mask input
        }
      }
    }

    // Connect to WiFi
    displayMessage("Connecting...");
    WiFi.begin(selectedSSID.c_str(), wifiPassword.c_str());

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 5) {
      displayMessage("Connecting...");
      delay(1000);
      attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      displayMessage("Connected! IP: \n" + WiFi.localIP().toString());
      wifiConnected = true;
      digitalWrite(LED_GREEN, HIGH);


      // Initialize Blynk
      Blynk.begin(BLYNK_AUTH_TOKEN, selectedSSID.c_str(), wifiPassword.c_str());
      if (Blynk.connected()) {
        tft.setTextSize(2);
        tft.setCursor(5, 60 );
        tft.print("Connected to Blynk!");
        Serial.println("Blynk connected");

        Blynk.virtualWrite(VPIN_PIR_POWER, LOW);
      }else{
        tft.setTextSize(2);
        tft.setCursor(5, 60 );
        tft.print("Blynk did not connect");
        Serial.println("Blynk did not connect");
        //Blynk.connect(); // Attempts to reconnect with last known credentials
      }
      delay(2000); 

      

      break;  // Exit the loop if connected
    } else {
      displayMessage("Failed! Trying Again...");
      wifiConnected = false;
      selectedSSID = "";
      delay(2000);  // Wait before retrying the network scan
    }
  }
}


//function to generate OTP
void generateRandomPin() {
  generatedPin = String(random(1000, 9999)); // Generate a 4-digit random PIN
  pinExpiryTime = millis() + 10000; // Set expiry time to 10 seconds from now
  Blynk.virtualWrite(VPIN_PIN_DISPLAY, generatedPin); // Send PIN to Blynk
  //displayMessage("PIN: " + generatedPin);
}



BLYNK_WRITE(VPIN_BUTTON) {
  generateRandomPin(); // Generate a new PIN when the button is pressed
}

void checkPinExpiry() {
  if (millis() > pinExpiryTime && generatedPin != "") {
    generatedPin = ""; // Invalidate the PIN after expiry
    Blynk.virtualWrite(VPIN_PIN_DISPLAY, "Expired"); // Notify Blynk
    //displayMessage("PIN Expired");
    delay(2000);
  }
}



void lockDoor() {
  Blynk.virtualWrite(VPIN_DOORSTATUS, LOW);
  isLocked=true;
  doorLock.attach(SERVO_PIN);
  doorLock.write(0);
  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_BLUE, LOW);
  tone(BUZZER_PIN, 1000, 200);
  displayMessage("Door Locked");
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(20, 50);
  tft.print("Enter Password:");
}

bool justActivate = false;
//activate and deactivate PIR sensor
BLYNK_WRITE(VPIN_PIR_POWER) {
  int pinValue = param.asInt();
  if (pinValue == HIGH) {
    digitalWrite(PIR_SENSOR, LOW); // Turn on PIR sensor
    pir = true;
    justActivate = true;
    digitalWrite(PIR_SENSOR, HIGH); // Turn on PIR sensor
    Serial.println("PIR Sensor Powered ON");
    tft.fillScreen(ILI9341_BLACK);
    tft.setTextSize(2);
    tft.setCursor(10, 20);
    tft.print("You are\nfully protected\nMotion detection\nactivated.");
    delay(3000);
    lockDoor();
  } else {
    pir = false;
    digitalWrite(PIR_SENSOR, LOW); // Turn off PIR sensor
    Serial.println("PIR Sensor Powered OFF");
    tft.fillScreen(ILI9341_BLACK);
    tft.setTextSize(2);
    tft.setCursor(10, 20);
    tft.print("You are not\nfully protected\nMotion detection\ndeactivated!.");
    delay(3000);
    lockDoor();
  }
}


void unlockDoor() {
  doorLock.attach(SERVO_PIN);
  doorLock.write(90);
  Blynk.virtualWrite(VPIN_DOORSTATUS, HIGH);
  isLocked = false;
  //doorLock.attach(SERVO_PIN);
  
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_BLUE, LOW);
  tone(BUZZER_PIN, 2000, 200);

  displayMessage("Door Unlocked");
}

void wrongPassword() {
  doorLock.write(0);
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_BLUE, HIGH);
  tone(BUZZER_PIN, 1000, 500);
}

void alertIntruder() {
  intruderAlertActive = true;  // Set alert active

  for (int i = 0; i < 10; i++) {  // Repeat 10 times
      if (!intruderAlertActive) break;  // Stop if the flag is reset

      Blynk.virtualWrite(VPIN_PIR_MOTION, "Wrong OTP!!!");
      displayMessage("Intruder!!!");
      digitalWrite(LED_RED, HIGH);
      digitalWrite(LED_GREEN, LOW);
      digitalWrite(LED_BLUE, LOW);
      tone(BUZZER_PIN, 1000, 500);  // Buzzer ON for 500ms
      delay(1000);  // Wait 1 second before the next beep
  }

  // Stop alert after 10 beeps
  noTone(BUZZER_PIN);
  digitalWrite(LED_RED, LOW);
  intruderAlertActive = false;
  lockDoor();
}

void stopIntruderAlert() {
  intruderAlertActive = false;  // Set alert active
  digitalWrite(LED_RED, LOW);  // Turn off Red LED
  noTone(BUZZER_PIN);  // Stop buzzer
}



//remotely lock and unlock door
BLYNK_WRITE(VPIN_DOORSTATUS) {
  int pinValue = param.asInt();
  if (pinValue == HIGH) {
    unlockDoor();
    tft.fillScreen(ILI9341_BLACK);
    tft.setTextSize(2);
    tft.setCursor(20, 20);
    tft.print("Door unlocked\nremotely.");
    delay(5000);
    lockDoor();
    pinValue == LOW;
  } else {
    lockDoor();
    tft.fillScreen(ILI9341_BLACK);
    tft.setTextSize(2);
    tft.setCursor(20, 20);
    tft.print("Door locked\nremotely.");
    delay(3000);
  }
}

int attemptCount = 0;  // Track wrong password attempts

void checkPassword() {
  if (!wifiConnected) {
    displayMessage("Connect to WiFi First!");  // Block unlock
    return;
  }

    if (inputPassword == generatedPin) {
      unlockDoor();
      if(VPIN_DOORSTATUS == HIGH){
        unlockDoor();
      }
      stopIntruderAlert();
      delay(5000);
      lockDoor();
      attemptCount = 0;
    } else {
        attemptCount++;  // Increase wrong attempt count
        displayMessage("Wrong Password!");
        wrongPassword();  // Flash LED / Buzzer (short alert)

        if (attemptCount >= 3) {
            alertIntruder();  // Activate full intruder alert
        }
    }
    inputPassword = "";  // Reset password input
}



void detectMotion() {
  if(pir == true){
    //lockDoor();
    if (digitalRead(PIR_SENSOR) == HIGH) {
      displayMessage("Motion Detected!");
      digitalWrite(LED_RED, LOW);
      digitalWrite(LED_GREEN, LOW);
      digitalWrite(LED_BLUE, HIGH);
      tone(BUZZER_PIN, 600, 300);
      delay(300);
      Blynk.virtualWrite(VPIN_PIR_MOTION, "Motion detected");
      
    }else{
      Blynk.virtualWrite(VPIN_PIR_MOTION, "Safe");
      //lockDoor();
    }
  }else{
    digitalWrite(PIR_SENSOR, LOW);
    //lockDoor();
  }

}


void blinkLed() {
  if (wifiConnected) {
    if (millis() - lastBlinkTime >= blinkInterval) {
      lastBlinkTime = millis();
      digitalWrite(LED_YELLOW, !digitalRead(LED_YELLOW)); // Toggle LED state
    }
  } else {
    digitalWrite(LED_YELLOW, LOW); // Turn off LED if not connected
  }
}

void forceLock(){
  while (!wifiConnected) {
    doorLock.attach(SERVO_PIN);
    doorLock.write(0);  

  }
}


// Function to connect to Blynk
void connectBlynk() {
  if(WiFi.status() == !WL_CONNECTED){
    tft.fillScreen(ILI9341_BLACK);
    tft.setTextSize(2);
    tft.setCursor(5, 60 );
    tft.print("Wifi disconnected,\nreconnecting...");
    Serial.println("wifi disconnected, reconnecting...");
    delay(3000);
    setupWiFi();

  }else if (WiFi.status() == WL_CONNECTED && !Blynk.connected()) {
    tft.fillScreen(ILI9341_BLACK);
    tft.setTextSize(2);
    tft.setCursor(5, 60 );
    tft.print("Blynk disconnected,\nreconnecting...");
    Serial.println("Blynk disconnected, reconnecting...");
    Blynk.connect(); // Attempts to reconnect with last known credentials
    int attempts = 0;
    while (!Blynk.connected() && attempts < 10) { // Timeout after ~10s
      delay(500);
      Serial.print(".");
      attempts++;
    }
    if (Blynk.connected()) {
      tft.fillScreen(ILI9341_BLACK);
      tft.setTextSize(2);
      tft.setCursor(5, 60 );
      tft.print("Blynk reconnected");
      Serial.println("\nBlynk reconnected");
      delay(3000);
      lockDoor();
    } else {
      WiFi.mode(WIFI_STA);
      WiFi.disconnect();
      delay(1000);
      Serial.println("\nBlynk connection failed, restarting...");
      ESP.restart(); // Reset ESP32 if Blynk fails persistently
    }
  }
}

void lockUnlock(){
char key = keypad.getKey();
  if (key) {
    // doorLock.attach(SERVO_PIN);
    // doorLock.write(0);
    tone(BUZZER_PIN, 500, 10);
      if (key == '#') {
        //doorLock.write(0),

          if (inputPassword == ""){
            tft.fillScreen(ILI9341_BLACK);
            tft.setTextSize(2);
            tft.setCursor(20, 60 );
            tft.print("Please enter\npassword.");
            delay(3000);
            lockDoor();
          }else{
            checkPassword();
          }
      } else if (key == '*') {
          inputPassword = "";
          displayMessage("Cleared!");
          delay(1000);
          lockDoor();
          Blynk.virtualWrite(VPIN_PIR_MOTION, "Safe");
      } else if (key == 'A'){
        if (WiFi.status() == WL_CONNECTED && Blynk.connected()) {
          tft.fillScreen(ILI9341_BLACK);
          tft.setTextSize(2);
          tft.setCursor(20, 60 );
          tft.print("Wifi and Blynk\n   still connected.");
          delay(3000);
          lockDoor();
        }else if(WiFi.status() == WL_CONNECTED && !Blynk.connected()){
          tft.fillScreen(ILI9341_BLACK);
          tft.setTextSize(2);
          tft.setCursor(20, 60 );
          tft.print("Blynk is \n   disconnected.");
          delay(3000);
          lockDoor();
        }else if(WiFi.status() == !WL_CONNECTED){
          tft.fillScreen(ILI9341_BLACK);
          tft.setTextSize(2);
          tft.setCursor(20, 60 );
          tft.print("Wifi is \n   disconnected.");
          delay(3000);
          lockDoor();
          setupWiFi();
        }
        //connectBlynk();
        lockDoor();
      }
       else {
        doorLock.write(0),
          inputPassword += key;
          displayMessage(inputPassword);
      }
  }
}

void setup() {
  doorLock.attach(SERVO_PIN);
  doorLock.write(0);  

  Serial.begin(115200);
  tft.begin();
  tft.setRotation(0);
  tft.fillScreen(ILI9341_BLACK);
  
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  pinMode(PIR_SENSOR, INPUT);
  
  pinMode(LED_YELLOW, OUTPUT);
 



  
  //selectNetwork(); // Let user choose network and enter password

    // Wait until WiFi is connected before proceeding
    while (!wifiConnected) {
      digitalWrite(LED_RED, LOW);
      digitalWrite(LED_GREEN, LOW);
      digitalWrite(LED_BLUE, HIGH);
      doorLock.attach(SERVO_PIN);
      doorLock.write(0);  
      setupWiFi();
    }

    lockDoor();

}

void loop() { 

  if(VPIN_DOORSTATUS == LOW){
    doorLock.attach(SERVO_PIN);
    doorLock.write(0);
  }

  forceLock();
  lockUnlock();


  if (WiFi.status() == WL_CONNECTED && !Blynk.connected()) {
    connectBlynk();
  }

  if(Blynk.connected()){
    Blynk.run();
  }

  timer.run();
  checkPinExpiry();
  blinkLed();

  detectMotion();

    // Check WiFi connection status
    if (WiFi.status() != WL_CONNECTED) {
      wifiConnected = false;
    } else {
      wifiConnected = true;
    }


}

