/*
Show the basic task scheduler
  
*/

#include <Arduino.h>
#include "scheduler.h"

os_task_t blinkingLED_task;
os_task_t AnalogRead_task;
os_task_id_t blinkingLED_taskID, AnalogRead_taskID;
os_functions_pointers_t function_pointers;

//Functions
os_task_return_codes_t blinkingLED_callback(os_event_t event);
os_task_return_codes_t AnalogRead_callback(os_event_t event);

//Declarations for analog input
int sensorPin_1 = A0;   // select the input pin for first analog input
int sensorPin_2 = A0;   // select the input pin for second analog inpu

#define AVERAGE_COUNT 600

double average_buf_1[AVERAGE_COUNT];
double average_buf_2[AVERAGE_COUNT];


void stdio_function( const char *fmt);
int analog_function();

//******************************************************************************
ISR(TCB2_INT_vect){
  TCB2.INTFLAGS = TCB_CAPT_bm;
  os_timer_count();
}
//******************************************************************************
void setup_timer_B(){
// compilation from diff sources
TCB2.CCMP = 0x9C3;  // 10ms with TCB_CLKSEL_CLKTCA_gc
//TCB2.CCMP = 0x9C3F; // 5ms with TCB_CLKSEL_CLKDIV2_gc 
// TCB2.CCMP = 0x1F3F; // 1ms with TCB_CLKSEL_CLKDIV2_gc 
TCB2.CTRLB = 0; // interrupt mode
// TCB2.CTRLA = TCB_CLKSEL_CLKDIV2_gc | TCB_ENABLE_bm;
TCB2.CTRLA = TCB_CLKSEL_CLKTCA_gc | TCB_ENABLE_bm;  // use TCA 250kHz clock
TCB2.INTCTRL =  TCB_CAPT_bm; 
}

void setup() {
  // Setup serial communication
  Serial.begin(115200);
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);

  //Init the schedular
  //Add functions pointers necessery for interrupt
	function_pointers.stdio = &stdio_function;
	//Add structure to os
	os_add_function_pointers(&function_pointers);

	//Call init
	os_init();

	//Add BlinkingLED task
	blinkingLED_task.task_cb = &blinkingLED_callback;
	blinkingLED_task.task_name = "BlinkingLED";
	os_add_task(blinkingLED_task);
	//Add AnalogRead task
	AnalogRead_task.task_cb = &AnalogRead_callback;
	AnalogRead_task.task_name = "AnalogRead";
	os_add_task(AnalogRead_task);

	//Begin timer for 10ms timing
  setup_timer_B();

  //Start os should never return
	os_main();

}

/**
 * Function used to output stdio
 */
void stdio_function( const char *fmt) {
	// os_log(os_log_level_all, "stdio_function\n\r");
  Serial.write(fmt);
}

void loop() {
  //Empty OS-Main handels the task
}

/**
 * Task 1
 * @param event
 * @return
 */
os_task_return_codes_t blinkingLED_callback(os_event_t event) {
	os_msg_t msg;
	static os_timer_id_t timer;
  static bool LED_Status = false;

	switch (event) {
	case os_event_init:
		//Save task id
		blinkingLED_taskID = os_current_task_id();
		//Log the task id
		os_log("Task %d is initialized\n\r", os_current_task_id());
		//Subscribe for timer event
		os_subscribe_for_event(os_event_timer, os_current_task_id());
		//Create timer for 100 milliseconds
		timer = os_timer_add(100, os_timer_repeat);
		// Start the timer
		os_timer_start(timer);
		break;

	case os_event_timer:
    if (LED_Status == true) {
      digitalWrite(LED_BUILTIN, HIGH);
      LED_Status = false;
    } else {
      digitalWrite(LED_BUILTIN, LOW);
      LED_Status = true;
    }
		break;

	default:
		break;
	}
	return os_task_succeed; //Succeed
}


/**
 * Task 2
 * @param event
 * @return
 */
os_task_return_codes_t AnalogRead_callback(os_event_t event) {
	os_msg_t msg;
	static os_timer_id_t timer = OS_MAXIMUM_TIMERS;
  static int analog_value = 0;

	switch (event) {
	case os_event_init:
		//Save task id
		AnalogRead_taskID = os_current_task_id();
		//Log the task id
		os_log("Task %d is initialized\n\r", os_current_task_id());
		//Subscribe for idle
		os_subscribe_for_event(os_event_idle, os_current_task_id());
		//Subscribe for timer event
		os_subscribe_for_event(os_event_timer, os_current_task_id());
		//Create timer for 100 milliseconds
		timer = os_timer_add(100, os_timer_repeat);
    // Start the timer
		os_timer_start(timer);
		break;

  case os_event_idle:
    analog_value = analog_function();
    break;

  case os_event_timer:
    Serial.println((String)" Analog value: " + analog_value );
		break;

	default:
		break;
	}
	return os_task_succeed; //Succeed
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