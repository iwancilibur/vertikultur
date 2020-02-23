#include <EEPROM.h>
#include <Wire.h>
#include <Adafruit_ADS1015.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <NewPing.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>

//WIFI CONNECTION
const char* ssid = "WIRA";
const char* password = "12345678";
const char* mqtt_server = "192.168.0.2";
WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

//FLOWMETER
byte statusLed    = 19;
byte sensorInterrupt = 4;  // PIN UNTUK FLOWMETER di pin GPIO4
byte sensorPin       = 4;
float calibrationFactor = 4.5;
volatile byte pulseCount;  
float flowRate;
unsigned int flowMilliLitres;
unsigned long totalMilliLitres;
unsigned long oldTime;

//PH
Adafruit_ADS1115 ads;
float Voltage,phair;
int kalibrasi;

//SUHU DS18B20
#define ONE_WIRE_BUS 5  //pin 5
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensorSuhu(&oneWire);
float suhuSekarang;

//ULTRASONIC 1
#define TRIGGER_PIN1 32 // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN1 33 // Arduino pin tied to echo pin on the ultrasonic sensor.
#define MAX_DISTANCE 50 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.
NewPing sonar1(TRIGGER_PIN1, ECHO_PIN1, MAX_DISTANCE); // NewPing setup of pins and maximum distance.

//ULTRASONIC 2
#define TRIGGER_PIN2 25 // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN2 263 // Arduino pin tied to echo pin on the ultrasonic sensor.
#define MAX_DISTANCE 50 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.
NewPing sonar2(TRIGGER_PIN2, ECHO_PIN2, MAX_DISTANCE); // NewPing setup of pins and maximum distance.

//WATER LEVEL
int waterlevel=34;

//RELAY
int R1=13;
int R2=12;
int R3=14;
int R4=27;

void setup()
{ 
  // Initialize a serial connection for reporting values to the host
  Serial.begin(115200);
  ads.begin();
  sensorSuhu.begin();
  pinMode(waterlevel,INPUT);
  pinMode(R1,OUTPUT);
  pinMode(R2,OUTPUT);
  pinMode(R3,OUTPUT);
  pinMode(R4,OUTPUT);

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  
  // Set up the status LED line as an output
  pinMode(statusLed, OUTPUT);
  digitalWrite(statusLed, HIGH);  // We have an active-low LED attached
  
  pinMode(sensorPin, INPUT);
  digitalWrite(sensorPin, HIGH);
  pulseCount        = 0;
  flowRate          = 0.0;
  flowMilliLitres   = 0;
  totalMilliLitres  = 0;
  oldTime           = 0;
  attachInterrupt(sensorInterrupt, pulseCounter, FALLING);
  EEPROM.put(1,0); 
  delay(1000);
// Initiaize the value of totalMilliLiters 
  if (EEPROM.read(0) != 0xFF){            
      EEPROM.put(1,totalMilliLitres);  
} else {
      EEPROM.get(1,totalMilliLitres);
}
    Serial.println("Startup EPROM Value  :");
    Serial.print("Output Liquid Quantity: ");        
    Serial.println(totalMilliLitres);
}

void loop()
{
    if (!client.connected()) {
    reconnect();
  }
  client.loop();
   if((millis() - oldTime) > 1000)    // Only process counters once per second
  { 
    // GET DATA SENSOR
    /*/--------------------------------------------------------------------/*/
    //SENSOR FLOW METER
      detachInterrupt(sensorInterrupt);
      flowRate = ((1000.0 / (millis() - oldTime)) * pulseCount) / calibrationFactor;
      oldTime = millis();
      flowMilliLitres = (flowRate / 60) * 1000;
      // Add the millilitres passed in this second to the cumulative total
      totalMilliLitres += flowMilliLitres;
      unsigned int frac;

    //SENSOR PH
      int16_t adc0;  // we read from the ADC, we have a sixteen bit integer as a result
      adc0 = ads.readADC_SingleEnded(0);
      kalibrasi=3;
      Voltage =(adc0 * 0.125)-1366;
      phair = (1023 - Voltage) / 73.07;
      phair=phair+kalibrasi;

     //SENSOR SUHU DS18B20
      suhuSekarang = ambilSuhu();

     //SENSOR ULTRASONIC
     int jarak1 = sonar1.ping_cm();
     int jarak2 = sonar2.ping_cm();

     //SENSOR WATER LEVEL
     int ambilwaterlevel=analogRead(waterlevel);
     
    /*/--------------------------------------------------------------------/*/
     
    //POST TO SERIAL 
    /*/--------------------------------------------------------------------/*/
    // FLOW METER PRINT
    Serial.print("Flow rate: ");
    Serial.print(int(flowRate));  // Print the integer part of the variable
    Serial.print("L/min");
    Serial.print("|");       // Print tab space
    Serial.print("Total mL: ");        
    Serial.print(totalMilliLitres);
    Serial.print(" | ");
    Serial.print("PH :"); 
    Serial.print(phair);
    Serial.print(" | ");
    Serial.print("SUHU :");
    Serial.print(suhuSekarang);
    Serial.print(" | ");
    Serial.print("ULTRASONIC 1 :");
    Serial.print(jarak1);
    Serial.print(" | ");
    Serial.print("ULTRASONIC 2 :");
    Serial.print(jarak2);
    Serial.print(" | ");
    Serial.print("WATER LEVEL :");
    Serial.print(ambilwaterlevel);
    
    Serial.println();
   /*/--------------------------------------------------------------------/*/
   
    EEPROM.put(1,totalMilliLitres);           //  Was read value  EEPROM.get(1,totalMilliLitres);
    // Reset the pulse counter so we can start incrementing again
    pulseCount = 0;
    // Enable the interrupt again now that we've finished sending output
    attachInterrupt(sensorInterrupt, pulseCounter, FALLING);

    //POST TO CLOUD
     /*/--------------------------------------------------------------------/*/
    char flowRateString[8];
    char totalMilliLitreString[8];
    char phairString[8];
    char suhuSekarangString[8];
    char jarak1String[8];
    char jarak2String[8];
    char ambilwaterlevelString[8];
    
    dtostrf(flowRate, 1, 2, flowRateString);
    dtostrf(totalMilliLitres, 1, 2, totalMilliLitreString);
    dtostrf(phair, 1, 2, phairString);
    dtostrf(suhuSekarang, 1, 2, suhuSekarangString);
    dtostrf(jarak1, 1, 2, jarak1String);
    dtostrf(jarak2, 1, 2, jarak2String);
    dtostrf(ambilwaterlevel, 1, 2, ambilwaterlevelString);
    
    client.publish("flowRate", flowRateString);
    client.publish("totalMilliLitres", totalMilliLitreString);
    client.publish("phair", phairString);
    client.publish("suhuSekarang", suhuSekarangString);
    client.publish("jarak1", jarak1String);
    client.publish("jarak2", jarak2String);
    client.publish("ambilwaterlevel", ambilwaterlevelString);
     /*/--------------------------------------------------------------------/*/
  }
}

void pulseCounter()
{
  // Increment the pulse counter
  pulseCount++;
}

float ambilSuhu()
{
   sensorSuhu.requestTemperatures();
   float suhu = sensorSuhu.getTempCByIndex(0);
   return suhu;   
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  
  if (String(topic) == "R1") {
    Serial.print("Changing output to ");
    if(messageTemp == "on"){
      Serial.println("R1 on");
      digitalWrite(R1, HIGH);
    }
    else if(messageTemp == "off"){
      Serial.println("R1 off");
      digitalWrite(R1, LOW);
    }
  }

  if (String(topic) == "R2") {
    Serial.print("Changing output to ");
    if(messageTemp == "on"){
      Serial.println("R2 on");
      digitalWrite(R2, HIGH);
    }
    else if(messageTemp == "off"){
      Serial.println("R2 off");
      digitalWrite(R2, LOW);
    }
  }
if (String(topic) == "R3") {
    Serial.print("Changing output to ");
    if(messageTemp == "on"){
      Serial.println("R3 on");
      digitalWrite(R3, HIGH);
    }
    else if(messageTemp == "off"){
      Serial.println("R3 off");
      digitalWrite(R3, LOW);
    }
  }

  if (String(topic) == "R4") {
    Serial.print("Changing output to ");
    if(messageTemp == "on"){
      Serial.println("R4 on");
      digitalWrite(R4, HIGH);
    }
    else if(messageTemp == "off"){
      Serial.println("R4 off");
      digitalWrite(R4, LOW);
    }
  }
  
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      // Subscribe
      client.subscribe("R1");
      client.subscribe("R2");
      client.subscribe("R3");
      client.subscribe("R4");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
