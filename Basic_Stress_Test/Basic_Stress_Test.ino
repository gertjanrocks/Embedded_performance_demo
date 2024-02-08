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

void setup() {
  // Setup serial communication
  Serial.begin(9600);
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
}

// loop = main function on other platforms
void loop() {
  int analog_value = 0;
  static int analog_enable_counter = 0;
  bool analog_enabled = false;
   
   //Timing values
  long start_time = 0;
  long LED_on_time = 0;
  long LED_off_time = 0;
  long analog_time = 0;

  start_time = micros();                    // reset the timing value
  digitalWrite(LED_BUILTIN, HIGH);          // turn the LED on (HIGH is the voltage level)
  delay(100);                               // wait for 100mSec
  LED_on_time = micros() - start_time;      // store the time the LED was on

  start_time = micros();                    // reset the timing value
  digitalWrite(LED_BUILTIN, LOW);           // turn the LED off by making the voltage LOW
  delay(100);                               // wait for 100 mSec
  LED_off_time = micros() - start_time;      // store the time the LED was off

  start_time = micros();                    // reset the timing value 
  if (analog_enable_counter > 10){          // first 10 iterations do not do the calculations
    analog_value = analog_function();
    analog_enabled = true;
  }
  analog_enable_counter++;
  if (analog_enable_counter > 20) analog_enable_counter = 0; //Restart counter after 20 iterations.
  analog_time = micros() - start_time;      // store the time the analog read function

  //Print info to terminal

  Serial.println(((String)"LED_on_time: "+ LED_on_time + " LED_off_time: "+ LED_off_time + " Analog_time: "+ analog_time + " Analog value: " + analog_value + " Calculation: " + ((analog_enabled == 1) ? "on" : "off")));

}

// This function reads out the analog pin, calculates a moving average and return the sum of both values
int analog_function() {
  static int counter_index = 0;
  double sensor_value_1 = 0;
  double sensor_value_2 = 0;

  //Read out the sensors // Stress the processor by adding a tan() function
  average_buf_1[counter_index] = analogRead(sensorPin_1);
  average_buf_2[counter_index] = analogRead(sensorPin_2);

  //Set the next counter_index
  counter_index++; 
  if (counter_index >= AVERAGE_COUNT) counter_index = 0;

  //Calculate average
  for (int x=0; x < AVERAGE_COUNT; x++) {
    // Stress the processor by adding a tan() function
    sensor_value_1 += tan(average_buf_1[x]);
    sensor_value_2 += tan(average_buf_2[x]);
  }
  sensor_value_1 = (sensor_value_1 / AVERAGE_COUNT);
  sensor_value_2 = (sensor_value_2 / AVERAGE_COUNT);

  //Return the sum of both values
  return (int)((sensor_value_1 + sensor_value_2) * 10000);
}
