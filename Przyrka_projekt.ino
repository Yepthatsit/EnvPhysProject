#include <LiquidCrystal_I2C.h>
#include <dht11.h>
#include <Arduino.h>
#include <SensirionI2CSen5x.h>
#include <Wire.h>
#include "wiring_private.h"
#include <TinyGPS++.h>
#include <SD.h>

#define MAXBUF_REQUIREMENT 48
#define DHT11PIN 5 
#define SERIAL_RX_BUFFER_SIZE 256

#if (defined(I2C_BUFFER_LENGTH) &&       \          
     (I2C_BUFFER_LENGTH >= MAXBUF_REQUIREMENT)) || \
    (defined(BUFFER_LENGTH) && BUFFER_LENGTH >= MAXBUF_REQUIREMENT)
#define USE_PRODUCT_INFO
#endif

String Filename;
TinyGPSPlus gps;
Uart Serial2(&sercom3, 7, 6, SERCOM_RX_PAD_3, UART_TX_PAD_2); // do gpsa
int displaymode = 0;
void SERCOM3_Handler()
{
  Serial2.IrqHandler();
}

SensirionI2CSen5x sen5x;
dht11 DHT11;

LiquidCrystal_I2C lcd(0x27,20,4); 
void setup() {
  analogReadResolution(12);
  lcd.init();
  // Print a message to the LCD.
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("Inicjalizacja...");



  // put your setup code here, to run once:
  Serial1.begin(9600);
  while (!Serial1) {
        delay(100);
    }
  Serial.begin(9600);
    //GPS
  Serial2.begin(9600);
  while (!Serial2) {
        delay(100);
    }
  

  pinPeripheral(7, PIO_SERCOM_ALT); // RX
  pinPeripheral(6, PIO_SERCOM_ALT);  // TX
    //sensirion
    Wire.begin();

    sen5x.begin(Wire);
 

    uint16_t error;
    char errorMessage[256];
    error = sen5x.deviceReset();
    if (error) {
        Serial.print("Error trying to execute deviceReset(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    }
    float tempOffset = 0.0;
    error = sen5x.setTemperatureOffsetSimple(tempOffset);
    if (error) {
        Serial.print("Error trying to execute setTemperatureOffsetSimple(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    } else {
        Serial.print("Temperature Offset set to ");
        Serial.print(tempOffset);
        Serial.println(" deg. Celsius (SEN54/SEN55 only");
    }
    error = sen5x.startMeasurement();
    if (error) {
        Serial.print("Error trying to execute startMeasurement(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    }
  //DHT11.check
  int chk = DHT11.read(DHT11PIN);         
  Serial.print("Stan sensora: ");
  switch (chk)
  {
    case DHTLIB_OK: 
		Serial.print("OK\t"); 
		break;
    case DHTLIB_ERROR_CHECKSUM: 
		Serial.println("Błąd sumy kontrolnej"); 
		break;
    case DHTLIB_ERROR_TIMEOUT: 
		Serial.println("Koniec czasu oczekiwania - brak odpowiedzi"); 
		break;
    default: 
		Serial.println("Nieznany błąd"); 
		break;
  }

    //winsen
    byte qacommand[] = {0xFF, 0x01, 0x78, 0x04, 0x00, 0x00, 0x00, 0x83};
    Serial1.write(qacommand,sizeof(qacommand));
    const byte qcommand[] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
    Serial1.write(qcommand,sizeof(qcommand));
    byte buffer[9];
    if (Serial1.available() > 0) {
        Serial1.readBytes(buffer,9);
    }
  // pierwszy pomiar gps
  // Otawcie pliku, zapisanie nagłówka
  if(SD.begin()){
    Filename = createFileName();
  //Serial.print(Filename);
    File datafile = SD.open(Filename,FILE_WRITE);
    datafile.println("Pm1p0,Pm2p5,Pm4p0,Pm10p0,NO2ppm,No2ppmRes,TGS,Humidity,Temperature,Lat,Long,Alt,TimeAndDate");
    datafile.close();
  }else{
    Serial.println("Karta nie załadowana");
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  uint16_t error;
    char errorMessage[256];

    delay(3000); //between mesurments

    // Read Measurement
    float massConcentrationPm1p0;
    float massConcentrationPm2p5;
    float massConcentrationPm4p0;
    float massConcentrationPm10p0;
    float ambientHumidity;
    float ambientTemperature;
    float vocIndex;
    float noxIndex;
    
    float Temperature;
    float Humidity;

    const int numBytes = 9;       // Number of bytes to read
    byte buffer[numBytes];
    int bytesRead = 0;

    error = sen5x.readMeasuredValues( // pomiar sensirionem
        massConcentrationPm1p0, massConcentrationPm2p5, massConcentrationPm4p0,
        massConcentrationPm10p0, ambientHumidity, ambientTemperature, vocIndex,
        noxIndex);
    if (error) {
    Serial.print("Error trying to execute readMeasuredValues(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
        
    }
    
    //dht 11
    Temperature = (float)DHT11.temperature;
    Humidity = (float)DHT11.humidity; 
    //Winsen musi być w q/a mode bo się rozjeżdża
    const byte qcommand[] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
    Serial1.write(qcommand,sizeof(qcommand));
    if (Serial1.available() > 0) {
        Serial1.readBytes(buffer,numBytes);
    }
    
    float resolution;
    switch(buffer[5]){
      case 0:
        resolution = 1;
      case 2:
        resolution = 0.01;
      case 1:
        resolution = 0.1;
    }
    float wvalue;
    wvalue = (buffer[2]*256+buffer[3])*resolution;



    // TGS
    float value = analogRead(A0);
    float readvalue = value*5/4096;

  // GPS
  while (Serial2.available() > 0) {
    char c = Serial2.read();
    gps.encode(c);
    Serial.print(c);
    // Check if both time and date have been updated
    if (gps.time.isUpdated() && gps.date.isUpdated()) {
      Serial.print("\n");
      displayTimeAndDate();
      break; // Stop execution after receiving the update
      
    }
  }
  updatelcd(massConcentrationPm1p0, massConcentrationPm2p5, massConcentrationPm4p0, massConcentrationPm10p0, wvalue, readvalue, Humidity, Temperature);
  AddDataToFile(massConcentrationPm1p0, massConcentrationPm2p5, massConcentrationPm4p0, massConcentrationPm10p0, wvalue,resolution, readvalue, Humidity, Temperature);
  // wypis na port szeregowy
  Serial.print("SensirionPm1p0:");
  Serial.print(massConcentrationPm1p0);
  Serial.print("\t");
  Serial.print("SensirionPm2p5:");
  Serial.print(massConcentrationPm2p5);
  Serial.print("\t");
  Serial.print("SensirionPm4p0:");
  Serial.print(massConcentrationPm4p0);
  Serial.print("\t");
  Serial.print("SensirionPm10p0:");
  Serial.print(massConcentrationPm10p0);
  Serial.print("\t");
  Serial.print("Humidity:");
  Serial.print(Humidity);
  Serial.print("\t");
  Serial.print("Temperature:");
  Serial.print(Temperature);
  Serial.print("\t");
  Serial.print("NO2ppm:");
  Serial.print(wvalue);
  Serial.print("\t");
  Serial.print("NO2ppmRes:");
  Serial.print(resolution,4);
  Serial.print("\t");
  Serial.print("TGS:");
  //Serial.print((5/readvalue -1)*500 );
  //Serial.print("\t");
  Serial.print(readvalue,4);
  Serial.print("\t");
  //Serial.print(Serial2.readStringUntil('\r\n'));
  //for (int i = 0; i < numBytes; i++) {
  //          Serial.print(buffer[i], HEX);
  //          Serial.print(" ");
  //      }
  Serial.print("\n");
  //if (isnan(ambientHumidity)) { //masa ifór ,sprawdzająca czy cokolwiek zmierzył dla temperatury i wilgotności ()
        //    Serial.print("n/a");
       // } else {
        //    Serial.print(ambientHumidity);
      //  }
      //  Serial.print("\t");
      //  Serial.print("AmbientTemperature:");
      //  if (isnan(ambientTemperature)) {
      //      Serial.print("n/a");
       // } else {
      //      Serial.print(ambientTemperature);
      //  }
       // Serial.print("\t");
       // Serial.print("VocIndex:");
      //  if (isnan(vocIndex)) {
      //      Serial.print("n/a");
      //  } else {
       //     Serial.print(vocIndex);
       // }
       // Serial.print("\t");
       // Serial.print("NoxIndex:");
       // if (isnan(noxIndex)) {
       //     Serial.println("n/a");
       // } else {
      //      Serial.println(noxIndex);
       // }

        /// DHT 11 prawdopodobnie powyższe do wywalenia

}

void updatelcd(float pm1p0 , float pm2p5, float pm4p0, float pm10p0,float win, float TGS, float humidity, float temp){
  switch(displaymode){
    case 0:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("PM1.0: ");
      lcd.print(pm1p0);
      lcd.setCursor(0, 1);
      lcd.print("PM2.5: ");
      lcd.print(pm2p5);
      displaymode++;
      break;
    case 1:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("PM4.0: ");
      lcd.print(pm4p0);
      lcd.setCursor(0, 1);
      lcd.print("PM10.0: ");
      lcd.print(pm10p0);
      displaymode++;
      break;
    case 2:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("NO2 ppm: ");
      lcd.print(win);
      lcd.setCursor(0, 1);
      lcd.print("TGS: ");
      lcd.print(TGS);
      displaymode++;
      break;
    case 3:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Temp: ");
      lcd.print(temp);
      lcd.print(" C");
      lcd.setCursor(0, 1);
      lcd.print("HUM: ");
      lcd.print(humidity);
      lcd.print(" %");
      displaymode = 0;
      break;
  }
}

void displayTimeAndDate() {
  Serial.print("Date: ");
  Serial.print(gps.date.month());
  Serial.print("/");
  Serial.print(gps.date.day());
  Serial.print("/");
  Serial.println(gps.date.year());

  Serial.print("Time: ");
  Serial.print(gps.time.hour());
  Serial.print(":");
  Serial.print(gps.time.minute());
  Serial.print(":");
  Serial.println(gps.time.second());
  // Display Latitude and Longitude
  Serial.print("Latitude: ");
  Serial.print(gps.location.lat(), 6);  
  Serial.print(", Longitude: ");
  Serial.println(gps.location.lng(), 6);  

    // Display Altitude
  Serial.print("Altitude: ");
  Serial.print(gps.altitude.meters());  
  Serial.println(" meters");
}
String createFileName() {
  // Extract date components
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("TEST GPS");
  delay(1000);
  while (Serial2.available() > 0) {
    char c = Serial2.read();
    gps.encode(c);

    // Check if both time and date have been updated
    if (gps.time.isUpdated() && gps.date.isUpdated()) {
      displayTimeAndDate();
      break; // Stop execution after receiving the update
    }
  }
  int month = gps.date.month();
  int day = gps.date.day();
  int year = gps.date.year();
  
  // Extract time components
  int hour = gps.time.hour();
  int minute = gps.time.minute();
  int second = gps.time.second();
  
    // Format date as DDMM
  String datePart = padZero(String(day)) + padZero(String(month));

  // Format time as mm
  String timePart = padZero(String(minute));

  // Generate a random 2-character suffix for uniqueness
  String suffix = getRandomSuffix();

  // Create the final filename
  String filename = datePart + timePart + suffix + ".csv";

  return filename;
  
}
//Pm1p0,Pm2p5,Pm4p0,Pm10p0,NO2ppm,No2ppmRes,TGS,Humidity,Temperature,Lat,Long,Alt,TimeAndDate
void AddDataToFile(float pm1p0 , float pm2p5, float pm4p0, float pm10p0,float NO2ppm,float NO2ppmRes, float TGS, float humidity, float temp){
  float sensorData[] = {pm1p0,pm2p5,pm4p0,pm10p0,NO2ppm,NO2ppmRes,TGS,humidity,temp};
  String Dataline = String();
  //int arraySize = sizeof(sensorData) / sizeof(sensorData[0]);
  for (int i= 0; i<9;i++){
    Dataline += String(sensorData[i],4);
    Dataline += ",";
  }
  float latitude = gps.location.lat();
    float longitude = gps.location.lng();
    float altitude = gps.altitude.meters();
    String fullDate =  String(gps.date.day()) + "/" + String(gps.date.month()) + "/" + String(gps.date.year());
    String fullTime = String(gps.time.hour()) + ":" + String(gps.time.minute()) + ":" + String(gps.time.second());

    // Create a CSV formatted string
    Dataline += String(latitude) + ",";  // Latitude
    Dataline += String(longitude) + ",";       // Longitude
    Dataline += String(altitude) + ",";        // Altitude
    Dataline += fullDate + "-";                // Date
    Dataline += fullTime  ;
    File file = SD.open(Filename,FILE_WRITE);
    file.println(Dataline);
    file.close();

}
String getRandomSuffix() {
  randomSeed(analogRead(A0) + millis()); // Seed the random generator
  char suffix[3];
  sprintf(suffix, "%02X", random(0, 256)); // Generate hexadecimal suffix (00 to FF)
  return String(suffix);
}

// Helper to pad single-digit numbers with a leading zero
String padZero(String input) {
  return (input.length() < 2) ? "0" + input : input;
}