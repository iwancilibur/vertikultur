int sensorM=34;

void setup() {
pinMode(sensorM,INPUT);
Serial.begin(115200);
}

void loop() {
int AmbilSensorMM=analogRead(sensorM);
//int AmbilSensorM=map(AmbilSensorMM,1024,0,0,100);
Serial.print("Data Sensor M :");
Serial.println(AmbilSensorMM);
delay(100);
}
