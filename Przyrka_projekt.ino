#include <dht11.h>
#include <Arduino.h>
#include <SensirionI2CSen5x.h>
#include <Wire.h>


#define MAXBUF_REQUIREMENT 48
#define DHT11PIN 6 

#if (defined(I2C_BUFFER_LENGTH) &&                 \
     (I2C_BUFFER_LENGTH >= MAXBUF_REQUIREMENT)) || \
    (defined(BUFFER_LENGTH) && BUFFER_LENGTH >= MAXBUF_REQUIREMENT)
#define USE_PRODUCT_INFO
#endif
SensirionI2CSen5x sen5x;
dht11 DHT11;
void setup() {
  // put your setup code here, to run once:
  Serial1.begin(9600);
  while (!Serial1) {
        delay(100);
    }
  Serial.begin(115200);
    while (!Serial) {
        delay(100);
    }
    //sensirion
    Wire.begin();

    sen5x.begin(Wire);
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
}

void loop() {
  // put your main code here, to run repeatedly:
  uint16_t error;
    char errorMessage[256];

    delay(1000); //between mesurments

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

    error = sen5x.readMeasuredValues( // pomiar sensirionem
        massConcentrationPm1p0, massConcentrationPm2p5, massConcentrationPm4p0,
        massConcentrationPm10p0, ambientHumidity, ambientTemperature, vocIndex,
        noxIndex);
    
    Temperature = (float)DHT11.temperature;
    Humidity = (float)DHT11.humidity; 

    
    if (error) {
        Serial.print("Error trying to execute readMeasuredValues(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
        
    }

  Serial.print("MassConcentrationPm1p0:");
  Serial.print(massConcentrationPm1p0);
  Serial.print("\t");
  Serial.print("MassConcentrationPm2p5:");
  Serial.print(massConcentrationPm2p5);
  Serial.print("\t");
  Serial.print("MassConcentrationPm4p0:");
  Serial.print(massConcentrationPm4p0);
  Serial.print("\t");
  Serial.print("MassConcentrationPm10p0:");
  Serial.print(massConcentrationPm10p0);
  Serial.print("\t");
  Serial.print("Humidity:");
  Serial.print(Humidity);
  Serial.print("\t");
  Serial.print("Temperature:");
  Serial.print(Temperature);
  Serial.print("\t");
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
