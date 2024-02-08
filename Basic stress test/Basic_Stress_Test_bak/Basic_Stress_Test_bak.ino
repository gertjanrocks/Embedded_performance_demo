/*
 Stress testing the basic main loop
 Features are added to show proof that the main loop has its limits
  
*/

#include <Arduino.h>
#include <EEPROM.h>

//Declarations for analog input
int sensorPin_1 = A0;   // select the input pin for first analog input
int sensorPin_2 = A0;   // select the input pin for second analog inpu

#define AVERAGE_COUNT 600

double average_buf_1[AVERAGE_COUNT];
double average_buf_2[AVERAGE_COUNT];

//Functions

int analog_function();
void write_to_eeprom(int value);
int read_from_eeprom();

void setup() {
  // with 16MHz oscillator, divide by 16 to get a 1MHz CPU clock:
  // _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, CLKCTRL_PDIV_16X_gc | CLKCTRL_PEN_bm);
  // Setup serial communication
  Serial.begin(9600);
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
}

// loop = main function on other platforms
void loop() {
  int analog_value = 0;
  int EEPROM_value = 0;

  //Timing values
  long old_time_value = millis();
Serial.println("new"); 
  digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
  delay(100);                      // wait for a second
  Serial.println(millis() - old_time_value); // Display time starting at the beginning of the loop
  digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
  delay(100);                      // wait for a second
  Serial.println(millis() - old_time_value); // Display time starting at the beginning of the loop
  old_time_value = millis();
  //Read previous stored data
  EEPROM_value = read_from_eeprom();
  //Write value to serial
  Serial.println(EEPROM_value);
  //Read analog data

  //TODO SET FUNCTION ON AFTER 10 SEC
  analog_value = analog_function();
  //Store analog data 
  write_to_eeprom (analog_value);
Serial.println(millis() - old_time_value); // Display time starting at the beginning of the loop
  old_time_value = millis();
  //Read previous stored data
  EEPROM_value = read_from_eeprom();
  double test = tan(EEPROM_value);
  test = atan(test);
  test = tan(test);
  test = atan(test);
  test = tan(test);
  test = atan(test);
  test = tan(test);
  test = atan(test);
  Serial.println(test, 4);

Serial.println(millis() - old_time_value); // Display time starting at the beginning of the loop
}

// This function reads out the analog pin, calculates a moving average and return the sum of both values
int analog_function() {
  static int counter_index = 0;
  double sensor_value_1 = 0;
  double sensor_value_2 = 0;

  //Read out the sensors
  average_buf_1[counter_index] = tan((double)analogRead(sensorPin_1));
  average_buf_2[counter_index] = tan((double)analogRead(sensorPin_2));

  //Set the next counter_index
  counter_index++; 
  if (counter_index >= AVERAGE_COUNT) counter_index = 0;

  //Calculate average
  for (int x=0; x < AVERAGE_COUNT; x++) {
    sensor_value_1 += tan(average_buf_1[x]);
    sensor_value_2 += tan(average_buf_2[x]);
  }
  sensor_value_1 = (sensor_value_1 / AVERAGE_COUNT);
  sensor_value_2 = (sensor_value_2 / AVERAGE_COUNT);

  //Return the sum of both values
  return (int)((sensor_value_1 + sensor_value_2) * 10000);
}

// Write value to EEPROM
void write_to_eeprom(int value) {
  //eeprom is divided in bytes so split up int 16-bit to 2x 8 bit
  byte byte_1 = (byte)(value  >> 8) ; //bitshift down
  byte byte_2 = (byte)(value & 0x00FF); // Only keep lower byte

  //Store bytes in EEPROM on adress 0 and adress 1
  EEPROM.write(0, byte_1);
  EEPROM.write(1, byte_2);
}

int read_from_eeprom() {
  //Data is stored on adress 0 and adress 1
  byte byte_1 = EEPROM.read(0);
  byte byte_2 = EEPROM.read(1);
  //Combine the data and return the value
  return (byte_1 << 8)  + byte_2;
}