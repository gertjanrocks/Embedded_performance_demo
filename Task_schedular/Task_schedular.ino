/*
Show the basic task scheduler
  
*/

#include <Arduino.h>
#include "scheduler.h"

os_task_t task1;
os_task_t task2;
os_task_t task3;
os_task_id_t taskID_1, taskID_2, taskID_3;
os_functions_pointers_t function_pointers;

//Functions
os_task_return_codes_t task1_callback(os_event_t event);
os_task_return_codes_t task2_callback(os_event_t event);
os_task_return_codes_t task3_callback(os_event_t event);
void stdio_function( const char *fmt);

//******************************************************************************
ISR(TCB2_INT_vect){
  TCB2.INTFLAGS = TCB_CAPT_bm;
  os_timer_count();
  // Serial.println( "ISR");
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

	//Add task 1
	task1.task_cb = &task1_callback;
	task1.task_name = "task1";
	os_add_task(task1);
	//Add task 2
	task2.task_cb = &task2_callback;
	task2.task_name = "task2";
	os_add_task(task2);
	//Add task 3
	task3.task_cb = &task3_callback;
	task3.task_name = "task3";
	os_add_task(task3);

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
os_task_return_codes_t task1_callback(os_event_t event) {
	os_msg_t msg;
	static os_timer_id_t timer;

	switch (event) {
	case os_event_init:
		//Save task id
		taskID_1 = os_current_task_id();
		//Log the task id
		os_log("Task %d is initialized\n\r", os_current_task_id());
		//Subscribe for timer event
		os_subscribe_for_event(os_event_timer, os_current_task_id());
		//Create timer for 10 seconds = 10000 milliseconds
		timer = os_timer_add(10000, os_timer_repeat);
		// Start the timer
		os_timer_start(timer);
		break;

	case os_event_timer:
		os_log("Timer send message from %d to %d\n\r",os_current_task_id(),taskID_2);
		msg.os_msg_id = taskID_1;
		msg.data = 0x55;
		//Post message to task2
		os_post_msg(msg, taskID_2);
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
os_task_return_codes_t task2_callback(os_event_t event) {
	os_msg_t msg;
	static os_timer_id_t timer = OS_MAXIMUM_TIMERS;

	switch (event) {
	case os_event_init:
		//Save task id
		taskID_2 = os_current_task_id();
		//Log the task id
		os_log("Task %d is initialized\n\r", os_current_task_id());
		//Subscribe for message pending
		os_subscribe_for_event(os_event_msg_pending, os_current_task_id());
		//Subscribe for timer event
		os_subscribe_for_event(os_event_timer, os_current_task_id());
		//Create timer for 1 seconds = 1000 milliseconds
		timer = os_timer_add(1000, os_timer_one_shot);
		break;

	case os_event_msg_pending:
		//Retrieve message
		os_retrieve_msg(&msg);
		os_log("Pending message = %d, %x\n\r",msg.os_msg_id,msg.data);
		if((msg.os_msg_id == taskID_1) && (msg.data == 0x55)){
			// Start the timer
			os_timer_start(timer);
		}
		break;

	case os_event_timer:
		os_log("Timer send message from %d to %d\n\r",os_current_task_id(),taskID_3);
		msg.os_msg_id = taskID_2;
		msg.data = 0xAA;
		//Post message to task3
		os_post_msg(msg, taskID_3);
		break;
	default:
		break;
	}
	return os_task_succeed; //Succeed
}

/**
 * Task 3
 * @param event
 * @return
 */
os_task_return_codes_t task3_callback(os_event_t event) {
	os_msg_t msg;
	static bool msgReceived = false;

	switch (event) {
	case os_event_init:
		//Save task id
		taskID_3 = os_current_task_id();
		//Log the task id
		os_log("Task %d is initialized\n\r", os_current_task_id());
		//Subscribe for message pending
		os_subscribe_for_event(os_event_msg_pending, os_current_task_id());
		//Subscribe for idle
		os_subscribe_for_event(os_event_idle, os_current_task_id());
		break;

	case os_event_msg_pending:
		//Retrieve message
		os_retrieve_msg(&msg);
		os_log("Pending message = %d, %x\n\r", msg.os_msg_id, msg.data);
		if((msg.os_msg_id == taskID_2) && (msg.data == 0xAA)){
			msgReceived = true;
		}
		break;
	case os_event_idle:
		if(msgReceived){
			os_log("Task %d in idle\n\r", os_current_task_id());
			msgReceived = false;
		}
		break;

	default:
		break;
	}
	return os_task_succeed; //Succeed
}

