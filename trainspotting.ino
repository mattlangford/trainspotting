#include <SPI.h>
#include <SD.h>
#include "UUID.h"
#include <MKRIMU.h>

const int PRECISION = 5;
String random_string;

String mag_datafile_name() {
  return "mag_" + random_string + ".csv";
}
String imu_datafile_name() { 
  return "imu_" + random_string + ".csv";
}

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  bool success = true;

  pinMode(LED_BUILTIN, OUTPUT);

  const int chip_select = SDCARD_SS_PIN;
  Serial.print("Initializing SD card on pin ");
  Serial.print(chip_select);
  Serial.print("...");

  // see if the card is present and can be initialized:
  if (!SD.begin(chip_select)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    while (1);
  }
  Serial.println("card initialized.");

  Serial.print("Initializing IMU... ");
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }
  Serial.println("IMU initialized.");

  // Seed uuid with IMU data
  while (!IMU.accelerationAvailable()) {}
  float x, y, z;
  IMU.readMagneticField(x, y, z);
  randomSeed(x + pow(y, 2) + pow(z, 3));
  for (int i = 0; i < 3; i++)
    random_string += random(0, 9);

  Serial.print("Initializing IMU data file ");
  Serial.print(imu_datafile_name());
  Serial.print("...");
  File imu_datafile = SD.open(imu_datafile_name(), FILE_WRITE);
  // if the file is available, write to it:
  if (imu_datafile) {
    imu_datafile.println("time,x,y,z");
    imu_datafile.close();
    Serial.println("data file initialized.");
  } else {
    success = false;
    Serial.println("error!");
  }

  Serial.print("Initializing MAG data file ");
  Serial.print(mag_datafile_name());
  Serial.print("...");
  File mag_datafile = SD.open(mag_datafile_name(), FILE_WRITE);
  // if the file is available, write to it:
  if (mag_datafile) {
    mag_datafile.println("time,x,y,z");
    mag_datafile.close();
    Serial.println("data file initialized.");
  } else {
    success = false;
    Serial.println("error!");
  }

  if (success) {
    digitalWrite(LED_BUILTIN, HIGH);
  }
}

void loop() {
  if (IMU.magneticFieldAvailable()) {
    float x, y, z;
    IMU.readMagneticField(x, y, z);

    File datafile = SD.open(mag_datafile_name(), FILE_WRITE);
    if (datafile) {
      datafile.print(millis());
      datafile.print(',');
      datafile.print(x, PRECISION);
      datafile.print(',');
      datafile.print(y, PRECISION);
      datafile.print(',');
      datafile.println(z, PRECISION);
      datafile.close();
    } else {
      static uint32_t last_print = 0;
      if (millis() - last_print > 1000) {
        Serial.println("error opening mag data file.");
        last_print = millis();
      }
      digitalWrite(LED_BUILTIN, LOW);
    }
  }
  if (IMU.accelerationAvailable()) {
    float x, y, z;
    IMU.readAcceleration(x, y, z);

    File datafile = SD.open(imu_datafile_name(), FILE_WRITE);
    if (datafile) {
      datafile.print(millis());
      datafile.print(',');
      datafile.print(x, PRECISION);
      datafile.print(',');
      datafile.print(y, PRECISION);
      datafile.print(',');
      datafile.println(z, PRECISION);
      datafile.close();
    } else {
      static uint32_t last_print = 0;
      if (millis() - last_print > 1000) {
        Serial.println("error opening imu data file.");
        last_print = millis();
      }
      digitalWrite(LED_BUILTIN, LOW);
    }

  }
}









