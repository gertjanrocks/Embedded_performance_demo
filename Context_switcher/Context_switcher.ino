/*
This example shows the potential use of context switching.

This example is based on the context switching library of Artem Boldariev
https://github.com/arbv/avr-context

*/

#include <avr/wdt.h>
#include <avr/sleep.h>

#include "avrcontext_arduino.h"

//Declarations for analog input
int sensorPin_1 = A0;   // select the input pin for first analog input
int sensorPin_2 = A0;   // select the input pin for second analog inpu

#define AVERAGE_COUNT 600

double average_buf_1[AVERAGE_COUNT];
double average_buf_2[AVERAGE_COUNT];

int analog_function();

//// Global variables

extern "C" {
avr_context_t *volatile current_task_ctx;   // current task context
}
static size_t current_task_num;             // current task index
static avr_context_t dummy_ctx;             // never going to be used
static avr_context_t tasks[2];              // contexts for tasks
static uint8_t analog_stack[128];         // stack for the analog_task

//Function declaration
static void analog_task(void *);


// This function starts system timer. 
void start_system_timer(void)
{
  TCB2.CCMP = 0x1F3F; // 1ms with TCB_CLKSEL_CLKDIV2_gc 
  TCB2.CTRLB = 0; // interrupt mode
  TCB2.CTRLA = TCB_CLKSEL_CLKDIV2_gc | TCB_ENABLE_bm;
  TCB2.INTCTRL =  TCB_CAPT_bm; 
}

void setup(void)
{
    // Setup serial communication
    Serial.begin(115200);
    // Enable builtin LED
    pinMode(LED_BUILTIN, OUTPUT);

    // Initialise dummy context.
     avr_getcontext(&dummy_ctx);

    // Initialise the first task.
    // To put it simply: we hijack the current execution context and
    // make it schedulable.
    current_task_num = 0;
    current_task_ctx = &tasks[0];

    // Initialise the second task.
    avr_getcontext(&tasks[1]);
    avr_makecontext(&tasks[1],
                    (void*)&analog_stack[0], sizeof(analog_stack),
                    &dummy_ctx,
                    analog_task, NULL);
    // start scheduling
    start_system_timer();
    // after returning from this function
    // loop() gets executed (as usual).
}

// This code blinks the LED // Task 1
void loop(void)
{
    while(true){
      digitalWrite(LED_BUILTIN, HIGH);
      delay(100);
      digitalWrite(LED_BUILTIN, LOW);
      delay(100);
    }   
}

// This function is the second task body. // Task 2
// It reads out the analog sensor.
static void analog_task(void *)
{
    static int analog_value = 0;
    while (true){
      analog_value = analog_function();
      Serial.print("Analog value: ");
      Serial.println(analog_value);
      // delay(100);
    }
}

static void switch_task(void)
{
    current_task_num = current_task_num == 0 ? 1 : 0;
    current_task_ctx = &tasks[current_task_num];
}

// System Timer Interrupt System Routine.
//
// Please keep in mind that ISR_NAKED attribute is important, because
// we have to save the current task execution context without changing
// it.
ISR(TCB2_INT_vect, ISR_NAKED)
{
    // save the context of the current task
    AVR_SAVE_CONTEXT_GLOBAL_POINTER(
        "cli\n", // disable interrupts during task switching
        current_task_ctx);
    switch_task(); // switch to the other task.
    TCB2.INTFLAGS = TCB_CAPT_bm; // Rest the timer
    // restore the context of the task to which we have just switched.
    AVR_RESTORE_CONTEXT_GLOBAL_POINTER(current_task_ctx);
    asm volatile("reti\n"); // return from the interrupt and activate the restored context.
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
