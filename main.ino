#define BLYNK_TEMPLATE_ID "TMPL3b9YYfNaN"
#define BLYNK_TEMPLATE_NAME "Smart Feeding Bottle"
#define BLYNK_AUTH_TOKEN "rDI3qWNwwq75DzCJQmzDTrwxvux0zlHP"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <LiquidCrystal_I2C.h>
#define TdsSensorPin 35
#define VREF 3.3 // analog reference voltage(Volt) of the ADC
#define SCOUNT 30 // sum of sample point
#define BLYNK_PRINT Serial
int analogBuffer[SCOUNT]; // store the analog value in the array, read from ADC
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0;
int copyIndex = 0;
float averageVoltage = 0;
float tdsValue = 0;
float temperature = 25; // current temperature for compensation
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "Galaxy S23 FE";
char pass[] = "12@123456";
BlynkTimer timer;
OneWire oneWire(4); // DS18B20 data pin connected to digital pin 2
DallasTemperature sensors(&oneWire);
LiquidCrystal_I2C lcd(0x27, 16, 2);
// median filtering algorithm
int getMedianNum(int bArray[], int iFilterLen){
int bTab[iFilterLen];
for (byte i = 0; i<iFilterLen; i++)
bTab[i] = bArray[i];
int i, j, bTemp;
for (j = 0; j < iFilterLen - 1; j++) {
for (i = 0; i < iFilterLen - j - 1; i++) {
if (bTab[i] > bTab[i + 1]) {
bTemp = bTab[i];
}
}
}
if ((iFilterLen & 1) > 0){
bTemp = bTab[(iFilterLen - 1) / 2];
}
else {
bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
}
return bTemp;
}
void setup(){
Serial.begin(9600);
pinMode(TdsSensorPin, INPUT);
sensors.begin(); // Initialize the DS18B20 sensor
WiFi.begin(ssid, pass);
while (WiFi.status() != WL_CONNECTED) {
delay(1000);
Serial.println("Connecting to WiFi...");
}
Serial.println("Connected to WiFi");
// Connect to Blynk
Blynk.begin(auth, ssid, pass);
while (!Blynk.connected()) {
Serial.println("Connecting to Blynk...");
delay(1000);
}
Serial.println("Connected to Blynk");
lcd.init();
lcd.backlight();
}
void loop(){
// Read temperature from DS18B20 sensor
sensors.requestTemperatures(); // Send the command to get temperatures
float dsTemperature = sensors.getTempCByIndex(0); // Get temperature in Celsius
temperature = dsTemperature;
static unsigned long analogSampleTimepoint = millis();
if(millis()-analogSampleTimepoint > 40U){ //every 40 milliseconds,read the analog value from the ADC
analogSampleTimepoint = millis();
analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin); //read the analog value and store into the buffer
analogBufferIndex++;
if(analogBufferIndex == SCOUNT){
analogBufferIndex = 0;
}
}
static unsigned long printTimepoint = millis();
if(millis()-printTimepoint > 40U){
printTimepoint = millis();
for(copyIndex=0; copyIndex<SCOUNT; copyIndex++){
analogBufferTemp[copyIndex] = analogBuffer[copyIndex];
// read the analog value more stable by the median filtering algorithm, and convert to voltage value
averageVoltage = getMedianNum(analogBufferTemp,SCOUNT) * (float)VREF / 4096.0;
//temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0));
float compensationCoefficient = 1.0+0.02*(temperature-25.0);
//temperature compensation
float compensationVoltage=averageVoltage/compensationCoefficient;
//convert voltage value to tds value
tdsValue=(133.42*compensationVoltage*compensationVoltage*compensationVoltage - 255.86*compensationVoltage*compensationVoltage + 857.39*compensationVoltage)*0.5;
}
}
// Print both TDS and DS18B20 temperature readings
Serial.print("TDS Value: ");
Serial.print(tdsValue); // You need to calculate tdsValue based on analogBuffer readings
Serial.print(" ppm\t"); // Assuming the unit of TDS is parts per million
Serial.print("Temperature: ");
Serial.print(dsTemperature);
Serial.println(" °C");
lcd.clear();
lcd.setCursor(0, 0);
lcd.print("Temp: ");
lcd.print(dsTemperature);
lcd.print(" (C)");
lcd.setCursor(0, 1);
if(tdsValue < 1000){
lcd.print("Water: ");
}else{
lcd.print("Milk: ");
}
lcd.print(tdsValue);
lcd.print("ppm");
Blynk.virtualWrite(V0, dsTemperature);
Blynk.virtualWrite(V1, tdsValue);
delay(1000); // Adjust delay if needed to control the rate of printing
Blynk.run();
}
