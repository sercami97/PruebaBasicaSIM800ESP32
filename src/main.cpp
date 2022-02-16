#include <Arduino.h>
#include "SIM800L.h"

#define SIM800_RST_PIN 6

const char APN[] = "TM";
// const char APN[] = "internet.comcel.com.co";
const char URL[] = "http://eco.agromakers.org/api/r_v?id=2021041229&tramo=1,1,1,1,1,1,1,1,1,1,1;2,2,2,2,2,2,2,2,2,2,2";

SIM800L* sim800l;

#define transistor_sim 33   //Pin del transistor


// void setupModule();
void setupModule() {
    // Wait until the module is ready to accept AT commands
  while(!sim800l->isReady()) {
    Serial.println(F("Problem to initialize AT command, retry in 1 sec"));
    delay(1000);
  }
  Serial.println(F("Setup Complete!"));

  // Go into low power mode
  bool normalPowerMode = sim800l->setPowerMode(NORMAL);
  if(normalPowerMode) {
    Serial.println(F("Module in Normal power mode"));
  } else {
    Serial.println(F("Failed to switch module to NORMAL power mode"));
  }


  // Wait for the GSM signal
  uint8_t signal = sim800l->getSignal();
  while(signal <= 0) {
    delay(1000);
    signal = sim800l->getSignal();
  }
  Serial.print(F("Signal OK (strenght: "));
  Serial.print(signal);
  Serial.println(F(")"));
  delay(1000);

  uint8_t contNetwork = 0;
  // Wait for operator network registration (national or roaming network)
  NetworkRegistration network = sim800l->getRegistrationStatus();
  while(network != REGISTERED_HOME && network != REGISTERED_ROAMING && contNetwork < 5) {
    delay(1000);
    network = sim800l->getRegistrationStatus();
    contNetwork++;
  }
  if(network != REGISTERED_HOME && network != REGISTERED_ROAMING){
    Serial.println(F("Network registration Failed"));
  }else{
    Serial.println(F("Network registration OK"));
  }
  delay(1000);

  // Setup APN for GPRS configuration
  bool success = sim800l->setupGPRS(APN);
  while(!success) {
    success = sim800l->setupGPRS(APN);
    delay(5000);
  }
  Serial.println(F("GPRS config OK"));
}

void setup() {  
  // Initialize Serial Monitor for debugging
  Serial.begin(115200);
  while(!Serial);

  // Initialize the hardware Serial1
  Serial2.begin(9600);
  delay(1000);

  pinMode(transistor_sim, OUTPUT);
  delay(100);
  digitalWrite(transistor_sim, HIGH);
  delay(100);

  delay(10000);
   
  // Initialize SIM800L driver with an internal buffer of 200 bytes and a reception buffer of 512 bytes, debug disabled
  // sim800l = new SIM800L((Stream *)&Serial1, SIM800_RST_PIN, 200, 512);

  // Equivalent line with the debug enabled on the Serial
  sim800l = new SIM800L((Stream *)&Serial2, NULL, 200, 512, (Stream *)&Serial);

  delay(5000);

  // Setup module for GPRS communication
  setupModule();
}
 
void loop() {
  // Establish GPRS connectivity (5 trials)
  bool connected = false;
  for(uint8_t i = 0; i < 5 && !connected; i++) {
    delay(1000);
    connected = sim800l->connectGPRS();
  }

  // Check if connected, if not reset the module and setup the config again
  if(connected) {
    Serial.print(F("GPRS connected with IP "));
    Serial.println(sim800l->getIP());
  } else {
    Serial.println(F("GPRS not connected !"));
    Serial.println(F("Reset the module."));
    sim800l->reset();
    setupModule();
    return;
  }

  Serial.println(F("Start HTTP GET..."));

  // Do HTTP GET communication with 10s for the timeout (read)
  uint16_t rc = sim800l->doGet(URL, 10000);
   if(rc == 200) {
    // Success, output the data received on the serial
    Serial.print(F("HTTP GET successful ("));
    Serial.print(sim800l->getDataSizeReceived());
    Serial.println(F(" bytes)"));
    Serial.print(F("Received : "));
    Serial.println(sim800l->getDataReceived());
  } else {
    // Failed...
    Serial.print(F("HTTP GET error "));
    Serial.println(rc);
  }

  // Close GPRS connectivity (5 trials)
  bool disconnected = sim800l->disconnectGPRS();
  for(uint8_t i = 0; i < 5 && !connected; i++) {
    delay(1000);
    disconnected = sim800l->disconnectGPRS();
  }
  
  if(disconnected) {
    Serial.println(F("GPRS disconnected !"));
  } else {
    Serial.println(F("GPRS still connected !"));
  }

  // Go into low power mode
  bool lowPowerMode = sim800l->setPowerMode(MINIMUM);
  if(lowPowerMode) {
    Serial.println(F("Module in low power mode"));
  } else {
    Serial.println(F("Failed to switch module to low power mode"));
  }

  // End of program... wait...
  while(1);
}

