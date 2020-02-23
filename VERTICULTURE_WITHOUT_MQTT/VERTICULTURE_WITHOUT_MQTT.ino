#include <EEPROM.h>
#include <Wire.h>
#include <Adafruit_ADS1015.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <NewPing.h>

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
