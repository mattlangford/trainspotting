#include <SPI.h>
#include <SD.h>
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
    ;  // wait for serial port to connect. Needed for native USB port only
  }

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  const int chip_select = SDCARD_SS_PIN;
  Serial.print("Initializing SD card on pin ");
  Serial.print(chip_select);
  Serial.print("...");
  if (!SD.begin(chip_select)) {
    Serial.println("Card failed, or not present");
    while (1)
      ;
  }
  Serial.println("card initialized.");

  Serial.print("Initializing IMU... ");
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1)
      ;
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
    while (1)
      ;
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

constexpr unsigned int SAMPLE_COUNT = 1024;  // should be a power of 2, 100hz sample rate
unsigned int next_sample_index = 0;
unsigned int last_sample_start_time = 0;
Sample samples[SAMPLE_COUNT];

void loop() {
  if (next_sample_index < SAMPLE_COUNT) {
    if (IMU.accelerationAvailable()) {
      Sample& sample = samples[next_sample_index++];
      sample.time = millis();
      IMU.readAcceleration(sample.x, sample.y, sample.z);
    }
    return;
  }

  digitalWrite(LED_BUILTIN, LOW);

  unsigned int now = millis();
  Serial.print("Writing data at ");
  Serial.print(now);
  Serial.print("ms... ");

  File datafile = SD.open(imu_datafile_name(), O_WRONLY | O_APPEND);
  if (!datafile) {
    Serial.println("error opening imu data file.");
    delay(1000);
    return;
  }

  unsigned int time_window = 0;
  for (unsigned int i = 0; i < next_sample_index; ++i) {
    Sample& sample = samples[i];
    datafile.print(sample.time);
    datafile.print(',');
    datafile.print(sample.x, PRECISION);
    datafile.print(',');
    datafile.print(sample.y, PRECISION);
    datafile.print(',');
    datafile.println(sample.z, PRECISION);

    if (i > 0) {
      // Fudge the time window a bit for the sake of printouts
      time_window += (1 + 1 * i == 1) * (samples[i].time - samples[i - 1].time);
    }
  }

  Serial.print("Wrote ");
  Serial.print(next_sample_index);
  Serial.print(" samples, ");
  Serial.print(time_window);
  Serial.print("ms of data, or ");
  Serial.print(1000 * next_sample_index / time_window);
  Serial.println("hz");


  datafile.close();
  next_sample_index = 0;
  digitalWrite(LED_BUILTIN, HIGH);
}
