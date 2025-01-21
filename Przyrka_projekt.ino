#include <LiquidCrystal_I2C.h>
#include <dht11.h>
#include <Arduino.h>
#include <SensirionI2CSen5x.h>
#include <Wire.h>
#include "wiring_private.h"



#define MAXBUF_REQUIREMENT 48
#define DHT11PIN 5 
#define SoftwareTX 4
#define SoftwareRX 5

#if (defined(I2C_BUFFER_LENGTH) &&       \          
     (I2C_BUFFER_LENGTH >= MAXBUF_REQUIREMENT)) || \
    (defined(BUFFER_LENGTH) && BUFFER_LENGTH >= MAXBUF_REQUIREMENT)
#define USE_PRODUCT_INFO
#endif

Uart Serial2(&sercom3, 7, 6, SERCOM_RX_PAD_3, UART_TX_PAD_2); // do gpsa
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
  lcd.print("Initializing...");



  // put your setup code here, to run once:
  Serial1.begin(9600);
  while (!Serial1) {
        delay(100);
    }
  Serial.begin(9600);
    while (!Serial) {
        delay(100);
    }
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

}

void loop() {
  // put your main code here, to run repeatedly:
  uint16_t error;
    char errorMessage[256];

    delay(2000); //between mesurments

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
  Serial.print(resolution);
  Serial.print("\t");
  Serial.print("TGSRs:");
  Serial.print((5/readvalue -1)*500 );
  Serial.print("\t");
  Serial.print(readvalue);
  Serial.print("\t");
  Serial.print(Serial2.readStringUntil('\n'));
  //for (int i = 0; i < numBytes; i++) {
   //         Serial.print(buffer[i], HEX);
    //        Serial.print(" ");
   //     }
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
