#include <Wire.h>
#include <Adafruit_ADS1015.h>
 
Adafruit_ADS1115 ads;
float Voltage,phair;
int kalibrasi;

void setup() {
  ads.begin();
  Serial.begin(115200);
}

void loop() {
   int16_t adc0;  // we read from the ADC, we have a sixteen bit integer as a result
   adc0 = ads.readADC_SingleEnded(0);
   kalibrasi=3;
   Voltage =(adc0 * 0.125)-1366;
   phair = (1023 - Voltage) / 73.07;
   phair=phair+kalibrasi;
   Serial.println(phair);
   delay(1000);
}
