#include <FreeRTOS_SAMD21.h>
#include <SPI.h>
#include <SD.h>
#include <MKRIMU.h>

//**************************************************************************
TaskHandle_t handle_writer_task;
TaskHandle_t handle_monitor_task;
TaskHandle_t handle_poll_imu;
StreamBufferHandle_t stream_data;

String random_string;
String datafile_name() { return random_string + ".csv"; }

//**************************************************************************

struct Sample {
  unsigned int time;
  float a_x;
  float a_y;
  float a_z;

  float m_x;
  float m_y;
  float m_z;

  float temperature;
};

constexpr size_t SAMPLE_BUFFER_SIZE = 128;
constexpr size_t SAMPLE_BUFFER_SIZE_BYTES = SAMPLE_BUFFER_SIZE * sizeof(Sample);

size_t dropped_samples = 0;

//**************************************************************************

static void thread_poll_imu(void  * /* pvParameters */) {
  Sample sample;

  TickType_t previous_wake_time = xTaskGetTickCount();
  
  while (1) {
    vTaskDelayUntil(&previous_wake_time, pdMS_TO_TICKS(10));

    sample.time = millis();
    IMU.readAcceleration(sample.a_x, sample.a_y, sample.a_z);
    //IMU.readMagneticField(sample.m_x, sample.m_y, sample.m_z);
    sample.temperature = IMU.readTemperature();

    if (xStreamBufferSend(stream_data, &sample, sizeof(Sample), pdMS_TO_TICKS(10)) != sizeof(Sample)) {
      dropped_samples++;
      Serial.print("Failed to write! (dropped=");
      Serial.print(dropped_samples);
      Serial.print(")");
      Serial.print(". Buffer size ");
      Serial.print(xStreamBufferBytesAvailable(stream_data));
      Serial.println();

      // Try again later
      vTaskDelayUntil(&previous_wake_time, pdMS_TO_TICKS(100));
    }
  }

  // Shouldn't reach here
  Serial.println("Thread thread_sd_write: Deleting");
  vTaskDelete(NULL);
}

//**************************************************************************

static void thread_sd_write(void  * /* pvParameters */) {
  Serial.println("Thread thread_sd_write: Started");

  File datafile = SD.open(datafile_name(), O_WRONLY | O_APPEND);
  if (!datafile) {
    Serial.println("error opening imu data file.");

    // delete ourselves.
    // Have to call this or the system crashes when you reach the end bracket and then get scheduled.
    Serial.println("Thread thread_sd_write: Deleting");
    vTaskDelete(NULL);
  }
  
  datafile.println("time,acc_x,acc_y,acc_z,mag_x,mag_y,mag_z,temp");

  while (1) {
    static Sample samples[SAMPLE_BUFFER_SIZE];
    const size_t read_bytes = xStreamBufferReceive(stream_data, (void*)samples, sizeof(samples), portMAX_DELAY);
    const size_t num_samples = read_bytes / sizeof(Sample);
    for (size_t i = 0; i < num_samples; ++i) {
      Sample& sample = samples[i];

      const int PRECISION = 7;
      datafile.print(sample.time);
      datafile.print(',');
      datafile.print(sample.a_x, PRECISION);
      datafile.print(',');
      datafile.print(sample.a_y, PRECISION);
      datafile.print(',');
      datafile.print(sample.a_z, PRECISION);
      datafile.print(',');
      datafile.print(sample.m_x, PRECISION);
      datafile.print(',');
      datafile.print(sample.m_y, PRECISION);
      datafile.print(',');
      datafile.print(sample.m_z, PRECISION);
      datafile.print(',');
      datafile.print(sample.temperature, PRECISION);
      datafile.println();
    }
    datafile.flush();
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

//*****************************************************************

void task_monitor(void * /* pvParameters */) {
  Serial.println("Task Monitor: Started");

  while (1) {
    static char ptrTaskList[400];  // temporary string buffer for task stats

    vTaskDelay(pdMS_TO_TICKS(10000));  // print every 10 seconds

    Serial.flush();
    Serial.println("");
    Serial.println("****************************************************");
    Serial.print("Time: ");
    Serial.print(millis());
    Serial.print("ms (tick=");
    Serial.print(xTaskGetTickCount());
    Serial.println(")");
    Serial.print("Free Heap: ");
    Serial.print(xPortGetFreeHeapSize());
    Serial.println(" bytes");

    Serial.print("Min Heap: ");
    Serial.print(xPortGetMinimumEverFreeHeapSize());
    Serial.println(" bytes");
    Serial.flush();

    Serial.println("****************************************************");
    Serial.println("Task            ABS             %Util");
    Serial.println("****************************************************");

    vTaskGetRunTimeStats(ptrTaskList);  // save stats to char array
    Serial.println(ptrTaskList);        // prints out already formatted stats
    Serial.flush();

    Serial.println("****************************************************");
    Serial.println("Task            State   Prio    Stack   Num     Core");
    Serial.println("****************************************************");

    vTaskList(ptrTaskList);       // save stats to char array
    Serial.println(ptrTaskList);  // prints out already formatted stats
    Serial.flush();

    Serial.println("****************************************************");

    Serial.print("Buffer usage: ");
    size_t usage = xStreamBufferBytesAvailable(stream_data);
    Serial.print(usage);
    Serial.print("/");
    Serial.print(usage + xStreamBufferSpacesAvailable(stream_data));
    Serial.print(" bytes.");
    Serial.println();
    
    Serial.print("Dropped samples: ");
    Serial.println(dropped_samples);

    Serial.println("****************************************************");
    Serial.flush();
  }

  // delete ourselves.
  // Have to call this or the system crashes when you reach the end bracket and then get scheduled.
  Serial.println("Task Monitor: Deleting");
  vTaskDelete(NULL);
}

//*****************************************************************

size_t get_num_files(File root) {
  size_t count = 0;
  while (true) {

    File entry =  root.openNextFile();
    if (! entry) {
      // no more files
      break;
    }
    count++;

    entry.close();
  }
  return count;
}

//*****************************************************************

void setup() {
  Serial.begin(115200);

  delay(1000);  // prevents usb driver crash on startup, do not omit this

  Serial.println("");
  Serial.println("******************************");
  Serial.println("        Program start         ");
  Serial.println("******************************");
  Serial.flush();

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
  
  random_string = String(get_num_files(SD.open("/")));

  Serial.print("Initializing IMU data file ");
  Serial.print(datafile_name());
  Serial.print("...");
  if (!SD.open(datafile_name(), FILE_WRITE)) {
    Serial.println("error!");
    while (1);
  }
  Serial.println("Data file initialized.");

  // Set the led the rtos will blink when we have a fatal rtos error
  // RTOS also Needs to know if high/low is the state that turns on the led.
  // Error Blink Codes:
  //    3 blinks - Fatal Rtos Error, something bad happened. Think really hard about what you just changed.
  //    2 blinks - Malloc Failed, Happens when you couldn't create a rtos object.
  //               Probably ran out of heap.
  //    1 blink  - Stack overflow, Task needs more bytes defined for its stack!
  //               Use the taskMonitor thread to help gauge how much more you need
  vSetErrorLed(LED_BUILTIN, HIGH);

  // sets the serial port to print errors to when the rtos crashes
  // if this is not set, serial information is not printed by default
  vSetErrorSerial(&Serial);

  // Create stream buffer for sharing data between the threads
  stream_data = xStreamBufferCreate(SAMPLE_BUFFER_SIZE_BYTES, sizeof(Sample));

  xTaskCreate(thread_poll_imu, "IMU Poll", 128, nullptr, tskIDLE_PRIORITY + 3, &handle_poll_imu);
  xTaskCreate(task_monitor, "Task Monitor", 256, nullptr, tskIDLE_PRIORITY + 2, &handle_monitor_task);
  xTaskCreate(thread_sd_write, "SD Writer", 256, nullptr, tskIDLE_PRIORITY + 1, &handle_writer_task);
  
  // Start the RTOS, this function will never return and will schedule the tasks.
  vTaskStartScheduler();

  // error scheduler failed to start
  // should never get here
  while (1) {
    Serial.println("Scheduler Failed!");
    Serial.flush();
    delay(1000);
  }
}

//*****************************************************************
void loop() {
  // Optional commands, can comment/uncomment below
  Serial.print(".");  //print out dots in terminal, we only do this when the RTOS is in the idle state
  Serial.flush();
  delay(100);  //delay is interrupt friendly, unlike vNopDelayMS
}