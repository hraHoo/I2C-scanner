#include <Arduino.h>

/* --------------------------------------
// i2c_scanner
//
// Version 1
//    This program (or code that looks like it)
//    can be found in many places.
//    For example on the Arduino.cc forum.
//    The original author is not know.
// Version 2, Juni 2012, Using Arduino 1.0.1
//     Adapted to be as simple as possible by Arduino.cc user Krodal
// Version 3, Feb 26  2013
//    V3 by louarnold
// Version 4, March 3, 2013, Using Arduino 1.0.3
//    by Arduino.cc user Krodal.
//    Changes by louarnold removed.
//    Scanning addresses changed from 0...127 to 1...119,
//    according to the i2c scanner by Nick Gammon
//    http://www.gammon.com.au/forum/?id=10896
// Version 5, March 28, 2013
//    As version 4, but address scans now to 127.
//    A sensor seems to use address 120.
// Version 6, November 27, 2015.
//    Added waiting for the Leonardo serial communication.
//
//
// Version 7, september 17, 2021.
//    Added more waiting for serial communication to let ESP8266 and ESP32 settle.
//    Added chip recognize prosesses. Chips that now is identified: 
//	- BMP085 or BMP180
//      - BMP280
//      - BME280
//      - BME680
//      - CSS811 or AMS IAQ-CORE
//      - HDC1000
//      - HDC1050
//      - MPU9250 or MPU6050
//      - SHT30
//      - SGP30
//	- VEML60xx
//	- SI1145
//	- VL53Lxx
//	- AS3935
//	- BH1750
//	- PCF8574
//	- OLED display
//
//  Version 7.1 February 14 2022
//	- DS2482 I2C to 1-Wire bridge
//	- MS5837 temperature and pressure sensor
//	- AHTxx sensor family
//	- LC709203 sensor
//	- MAX17043 fuel gauge (lipo battery monitor)
//
//  Version 7.2 December 11 2022
//	- ENS160 Air Quality Sensor
//
//
//
// This sketch tests the standard 7-bit addresses
// Devices with higher bit address might not be seen properly.
*/

#define VERSION 7.2

#include <Wire.h>

//#define SDA_pin 4								//esp8266 GPIO4 - D2 | esp32 GPIO21             
//#define SCL_pin 5								//esp8266 GPIO5 - D1 | esp32 GPIO22

#define SDA_pin 21								//esp8266 GPIO4 - D2 | esp32 GPIO21             
#define SCL_pin 22								//esp8266 GPIO5 - D1 | esp32 GPIO22

#define BOSCH_REG_CHIPID              0xD0
#define CCS811_HW_ID                  0x81
#define HDC1000_Device_ID             0x1000
#define HDC10xx_Device_ID             0x1050
#define VEML6075_Device_ID            0x0026

#define CCS811_HW_ID_REG              0x20
#define HDC10xx_MANUFACTURER_ID_REG   0xFE
#define HDC10xx_Device_ID_REG         0xFF
#define VEML6075_Device_ID_REG        0x0C

// if no serial port then use led to indicate I2C presense
const int ledPin =  17;								//LED_BUILTIN;


uint8_t scanAddress = 0;


uint8_t  Read8(uint8_t reg, uint8_t _devAddr)
{
  Wire.beginTransmission(_devAddr);
  Wire.write(reg);
  Wire.endTransmission();

  Wire.requestFrom(_devAddr, 1);
  uint32_t timeout = millis() + 500;       					// Don't hang here for more than 300ms
  while (Wire.available() < 1) {
      if ((millis() - timeout) > 0) {
          return 0;
      }
  }
  return Wire.read();
}

uint16_t Read16(uint8_t reg, uint8_t _devAddr)
{
  uint8_t msb, lsb;

  Wire.beginTransmission(_devAddr);
  Wire.write(reg);
  Wire.endTransmission();

  Wire.requestFrom(_devAddr, 2);
  uint32_t timeout = millis() + 500;       					// Don't hang here for more than 300ms
  while (Wire.available() < 2) {
      if ((millis() - timeout) > 0) {
        Serial.println("no id");
          return 0;
      }
  }
  msb = Wire.read();
  lsb = Wire.read();
  return (uint16_t) msb<<8 | lsb;
}


String deviceChip(uint8_t address){

  if(address >= 0x70 && address <= 0x77){                                       // 0x70 - 0x77    TCA9548
    if(address == 0x76 || address == 0x77){					// possible bosch sensor
      uint8_t chip_id = Read8(BOSCH_REG_CHIPID, address);                       // get chip ID
      //Serial.print("\nChip ID: ");
      //Serial.println(chip_id, HEX);
      if(chip_id == 0x55){ return "chip = BMP085 or BMP180"; }                  // 0x77          BMP180    chip ID 0x55
      else if(chip_id == 0x58){ return "chip = BMP280"; }                       // 0x76 / 0x77   BMP280    chip ID 0x58
      else if(chip_id == 0x56 || chip_id == 0x57){                              // 0x76 / 0x77   BMP280    chip ID 0x56 or 0x57 (Sample)
        return "chip = BMP280 (Sample)";
      }
      else if(chip_id == 0x60){ return "chip = BME280"; }                       // 0x76 / 0x77   BME280    chip ID 0x60
      else if(chip_id == 0x61){ return "chip = BME680"; }                       // 0x76 / 0x77   BME680    chip ID 0x61
      else { return "MS5611, MS5837 or TCA9548"; }                              // 0x76 / 0x77   MS5611    0x70 - 0x77    TCA9548
    }
    else { return "Most likely TCA9548"; }                                      // 0x70 - 0x77    TCA9548
  }
  
  if(address == 0x5A || address == 0x5B){                                       // CCS811 air quality from Sciosense
    uint8_t chip_id = Read8(CCS811_HW_ID_REG, address);                         // get chip ID
    //Serial.print("\nChip ID: ");
    //Serial.println(chip_id, HEX);
    if(chip_id == CCS811_HW_ID){ return "chip = CCS811"; }                      // CCS811    chip ID 0x81
    else{ return "unknown"; }
  }
  
  if(address >= 0x40 && address <= 0x43){                                       // possible HDC1000 or SHT2x
    uint16_t chip_id = Read16(HDC10xx_Device_ID, address);                      // get chip ID
    //Serial.print("\nChip ID: ");
    //Serial.println(chip_id, HEX);
    if(chip_id == HDC10xx_Device_ID){ return "chip = HDC1050 or HDC1080"; }     // 0x40          HDC1080   chip ID 0x1050
    if(chip_id == HDC1000_Device_ID){ return "chip = HDC1000"; }                // 0x40 - 0x43   HDC1000   chip ID 0x1000
    else{ return "maybe SHT2x"; }                                               // 0x40 and no ID for HDC10x0   SHT2x
  }

  if(address == 0x3C){ return "device: OLED display"; }                         // 0x3C          OLED display

  if(address >= 0x38 && address <= 0x3F){                                       // possible PCF8574
    uint16_t chip_id = Read16(HDC10xx_Device_ID, address);                      // get chip ID
    //Serial.print("\nChip ID: ");
    //Serial.println(chip_id, HEX);
    if(address == 0x23){ return "chip = Most likely BH1750 can be PCF8574"; }   // 0x20   BH1750 default address
    if(address == 0x38){ return "chip = Most likely AHTxx can be PCF8574"; }    // 0x38 AHT10 AHT20 AHT21
    if(address == 0x39){ return "chip = Most likely AHT10 can be PCF8574"; }    // 0x39 AHT10
    else{ return "Most likely PCF8574"; }                                       // 0x20 - 0x27 PCF8574
  }
  if(address == 0x68 || address == 0x69){                                       // 0x68 / 0x69   MPU9250
    return "chip = MPU9250 or MPU6050"; 
  }
  
  if(address == 0x10){ return "chip = VEML60xx"; }                              // VEML60xx Sensors from Vishay
  if(address >= 0x01 && address <= 0x03){ return "chip = AS3935"; }             // 0x01 - 0x03   AS3935    default 0x03
  if(address >= 0x18 && address <= 0x21){ return "chip = DS2482"; }             // 0x18 / 0x21   DS2482
  if(address == 0x23 || address == 0x5c){ return "chip = BH1750"; }             // 0x23 / 0x5C   BH1750    ADDR=1->0x5C
  if(address == 0x29){ return "chip = VL53Lxx"; }                               // 0x29          VL53Lxx
  if(address == 0x32){ return "chip = Fuel gauge MAX17043"; }                  	// 0x32          MAX17043
  if(address == 0x44){ return "chip = SHT3x"; }                                 // 0x44          SHT3x
  if(address == 0x52 || address == 0x53){ return "chip = ENS160"; }             // 0x52 / 0x53   ENS160  ADDR = 1->0x53  CS = 1 -> I2C
  if(address == 0x58){ return "chip = SGP30"; }                                 // 0x58          SGP30
  if(address == 0x60){ return "chip = SI1145 od SI5351"; }                      // 0x60          SI1145 or SI5351
  if(address == 0x0B){ return "chip = LC709203"; }                              // 0x0B          LC709203

  
  return "unknown";
}


void blinkNow(){
  digitalWrite(ledPin, LOW);
  delay(200);
  digitalWrite(ledPin, HIGH);
}

void setup()
{
  Wire.begin();
  //Wire.begin(SDA_pin, SCL_pin);

  Serial.begin(115200);
  while (!Serial);
  delay(300);
  Serial.print("\nI2C Scanner veriaon: ");
  Serial.println(VERSION);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
}


void loop()
{    
  byte error;
  int nDevices;

  Serial.println("Scanning...");

  nDevices = 0;
  for(scanAddress = 1; scanAddress < 127; scanAddress++ ) 
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(scanAddress);
    error = Wire.endTransmission();

    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");
      if (scanAddress<16) 
        Serial.print("0");
      Serial.print(scanAddress,HEX);
      Serial.print(" Hex, ");
      Serial.print(scanAddress);
      Serial.print(" Dec.\t");
      Serial.println(deviceChip(scanAddress));
      nDevices++;
      blinkNow();
    }
    else if (error==4) 
    {
      Serial.print("Unknow error at address 0x");
      if (scanAddress<16) 
        Serial.print("0");
      Serial.println(scanAddress,HEX);
    }    
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("done\n");

  delay(5000);									// wait 5 seconds for next scan
}
