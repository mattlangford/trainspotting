#include <SPI.h>
#include <SD.h>
#include "UUID.h"
#include <MKRIMU.h>

const int PRECISION = 7;
String random_string;

String mag_datafile_name() {
  return "mag" + random_string + ".csv";
}
String imu_datafile_name() { 
  return "imu" + random_string + ".csv";
}

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  const int chip_select = SDCARD_SS_PIN;
  Serial.print("Initializing SD card on pin ");
  Serial.print(chip_select);
  Serial.print("...");
  if (!SD.begin(chip_select)) {
    Serial.println("Card failed, or not present");
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
  for (int i = 0; i < 4; i++)
    random_string += random(0, 9);

  Serial.print("Initializing IMU data file ");
  Serial.print(imu_datafile_name());
  Serial.print("...");
  File imu_datafile = SD.open(imu_datafile_name(), FILE_WRITE);
  if (!imu_datafile) {
    Serial.println("error!");
    while (1);
  }
  imu_datafile.println("time,x,y,z");
  imu_datafile.close();
  Serial.println("data file initialized.");

  digitalWrite(LED_BUILTIN, HIGH);
}

unsigned int count = 0;
unsigned int last_reset = 0;

struct Sample {
  unsigned int time;
  float x;
  float y;
  float z;
};

constexpr unsigned int SAMPLE_RATE = 100; // hz
constexpr unsigned int NUM_SECONDS = 10; // s
constexpr unsigned int SAMPLE_COUNT = SAMPLE_RATE * NUM_SECONDS;
unsigned int next_sample_index = 0;
Sample samples[SAMPLE_COUNT];

void loop() {
  if (IMU.accelerationAvailable()) {
    Sample& sample = samples[next_sample_index++];
    samples[next_sample_index].time = millis();
    
  }

  if (next_sample_index < SAMPLE_COUNT) {
    return;
  }
  digitalWrite(LED_BUILTIN, LOW);

  File datafile = SD.open(imu_datafile_name(), FILE_WRITE);
  if (!datafile) {
    Serial.println("error opening imu data file.");
    delay(1000);
    return;
  }

  unsigned int read_at = millis() - 1000 * NUM_SECONDS;
  for (unsigned int i = 0; i < SAMPLE_COUNT; ++i) {
    datafile.print(read_at);
    datafile.print(',');
    datafile.print(samples[i].x, PRECISION);
    datafile.print(',');
    datafile.print(samples[i].y, PRECISION);
    datafile.print(',');
    datafile.println(samples[i].z, PRECISION);

    read_at += 1000 / SAMPLE_RATE;
  }

  datafile.close();
  next_sample_index = 0;
  digitalWrite(LED_BUILTIN, HIGH);
}









