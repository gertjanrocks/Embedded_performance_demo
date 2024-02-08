/* 
  Scheduler example. Based on RocksOS
  https://github.com/gertjanrocks/rocksOS/

 * -----------------------------------------------------------------------------
 * Author: Gertjan Rocks
 * Web:    www.gertjanrocks.com
 * Mail:   gertjanrocks@outlook.com
 * -----------------------------------------------------------------------------

 */

#include "scheduler.h"
#include <stdio.h>
#include <stdarg.h>

/* -----------------*
 * Type definitions *
 * -----------------*
 */

typedef struct {
	uint64_t start_value;
	uint32_t value;
	os_timer_type_t type;
	os_task_id_t task_id;
	bool used;
} os_timer_entry_t;

typedef struct {
	os_msg_t msg;
	os_task_id_t destination;
} os_msg_queue_entry_t;

typedef struct {
	os_msg_queue_entry_t queue_buffer[OS_MSG_QUEUE_SIZE];
	os_queue_t queue;
} os_msg_queue_t;

/* ---------------------*
 * File-scope variables *
 * ---------------------*
 */

static os_task_admin_t os_task_admin[OS_MAXIMUM_TASKS];
static os_task_id_t os_latest_added_task;
static os_task_id_t task_running;

static bool os_error;
static bool os_functions_done = false;

static os_msg_queue_t os_msg_buffer;
static os_msg_queue_entry_t msg_pending;

//Timer entries
static os_timer_entry_t os_timers[OS_MAXIMUM_TIMERS];
//Queue to store the pending task_ids
static os_task_id_t	os_timer_pending_queue_buffer[OS_MAXIMUM_TIMERS];
static os_queue_t os_timer_pending_queue;

//unsigned 64 bit timer @ 1 milliseconds can overflow in 584942417 years! Take care of this!
static uint64_t os_timer_counter;

//Public for OS
os_functions_pointers_t* os_functions_pointer;

/**
 * Function will initialize every necessary things for the os
 * @return
 */
os_return_codes_t os_init(void) {
	os_return_codes_t os_returnValue = os_init_succeed;
	//Clear os error;
	os_error = false;
	//Init the os message layer
	if (os_returnValue == os_init_succeed) {
		if (os_msg_init() != os_init_succeed) {
			os_returnValue = os_init_failed;
			os_log("Error on message initialize\n\r");
		}
	}
	//Init the task layer
	if (os_returnValue == os_init_succeed) {
		if (os_task_init() != os_init_succeed) {
			os_returnValue = os_init_failed;
			os_log("Error on task initialize\n\r");
		}
	}

	//Init the timer layer
	if (os_returnValue == os_init_succeed) {
		if (os_timer_init() != os_init_succeed) {
			os_returnValue = os_init_failed;
			os_log("Error on timer initialize\n\r");
		}
	}

  os_log("\n\r\n\rRocksOS - 2017 www.gertjanrocks.com \n\r\n\r");
	return os_returnValue;
}

/**
 * main function for the os should only return on a error
 * @return
 */
os_return_codes_t os_main(void) {
	static os_task_id_t last_checked_task_id;
	os_task_id_t task_id;
	os_task_return_codes_t returnValue;

	//Check functions pointers
	if(!os_functions_done){
		os_set_error();
		os_log("Error functions pointers not done\n\r");
	}

	//init all the task
	if(!os_error){
		for (task_id = 0; task_id < os_nmbr_of_tasks(); task_id++){
			os_log("Send init event to %d\n\r",task_id);
			returnValue = os_run_task(task_id,os_event_init);
		}
	}
	//Run the OS while main loop until an error occurs
	while(!os_error){
		//Check if message is pending
		task_id = os_msg_pending();
		//Check if message is valid
		if(task_id < OS_MAXIMUM_TASKS){
			os_log("Message pending for %d\n\r",task_id);
			returnValue = os_run_task(task_id, os_event_msg_pending);
			if(returnValue == os_task_failed){
				os_log("Error on message pending for %d\n\r",task_id);
			}
		}

		//Check all timers
		os_timer_check_timers();
		//Check if timer task id is pending
		task_id = os_timer_pending();
		//If task id pending run task
		if(task_id < OS_MAXIMUM_TASKS){
			os_log("Timer event for %d\n\r",task_id);
			returnValue = os_run_task(task_id, os_event_timer);
			if(returnValue == os_task_failed){
				os_log("Error on timer event for %d\n\r",task_id);
			}
		}

		//Cycle trough task for the idle event
		if(last_checked_task_id < os_nmbr_of_tasks()){
			last_checked_task_id++;
		} else {
			last_checked_task_id = 0;
		}
		//Set function to check and run if necessary
		returnValue = os_run_task(last_checked_task_id, os_event_idle);
		if(returnValue == os_task_failed){
			os_log("Error on idle for %d\n\r",task_id);
		}
	}
	return os_failed;
}

/**
 * Set the OS error on true
 */
void os_set_error(void) {
	os_error = true;
	os_log ("Error reported for OS!!\n\r");
}

/**
 * add the necessary functions pointers to the os
 * @param os_functions
 * @return
 */
os_return_codes_t os_add_function_pointers(os_functions_pointers_t* os_functions){
	os_return_codes_t os_returnValue = os_init_succeed;
	//Check the necessary functions

	os_functions_pointer = os_functions;
	//Check disable irq function
	if (os_returnValue == os_init_succeed) {
		if (os_functions_pointer->stdio == NULL) {
			os_returnValue = os_init_failed;
		}
	}

	if(os_returnValue == os_init_succeed){
		os_functions_done = true;
	}
	return os_returnValue;
}

/**
 * initialse the os task layer
 * @return
 */
os_return_codes_t os_task_init(void) {
	os_return_codes_t returnValue = os_init_failed;

	//Clear all task entries
	for (os_task_id_t task = 0; task < OS_MAXIMUM_TASKS; task++) {
		os_task_admin[task].task.task_cb = NULL;
		os_task_admin[task]._this = task;
		//Clear the event subscription
		for (int event = 0; event < os_nmbr_of_events; event++) {
			os_task_admin[task].event_subscribe[event] = false;
		}
		os_task_admin[task].task.identifier = 0;
	}
	//Clear the variable
	os_latest_added_task = 0;
	task_running = OS_MAXIMUM_TASKS;
	//Init is successful
	returnValue = os_init_succeed;
	return returnValue;
}

/**
 * add a task to the local task table
 * @param new_task
 * @return -1 if error on adding
 */
int os_add_task(os_task_t new_task) {
	if (os_task_admin[os_latest_added_task].task.task_cb == NULL) {
		//Every event is auto subscribed to the os_event_init
		os_task_admin[os_latest_added_task].event_subscribe[os_event_init] =
				true;
		//Add task to the table
		os_task_admin[os_latest_added_task++].task = new_task;
		if (os_latest_added_task >= OS_MAXIMUM_TASKS) {
			os_log("Error on adding task\n\r");
			os_set_error();
			return -1;
		}
	} else {
		os_log( "Error on adding task\n\r");
		os_set_error();
		return -1;
	}
	return 0;
}

/**
 * subscribe a task id for a os event
 * @param  event   event to subscribed
 * @param  task_id task id which want to subscribe to the event
 * @return         returns 0 if it was not possible to subscribe for the event
 */
int os_subscribe_for_event(os_event_t event, os_task_id_t task_id) {
	os_task_admin[task_id].event_subscribe[event] = true;
	return 1;
}

/**
 *	return the running task id
 * @return
 */
extern os_task_id_t os_current_task_id(void){
	return task_running;
}


/**
 * run a task with a given event
 * @param task_id
 * @param event
 * @return the task return code
 */
os_task_return_codes_t os_run_task(os_task_id_t task_id, os_event_t event) {
	//Check if task id registered for the event
	if (os_task_admin[task_id].event_subscribe[event]) {
		if (os_task_admin[task_id].task.task_cb != NULL) {
			task_running = os_task_admin[task_id]._this;
			return os_task_admin[task_id].task.task_cb(event);
		} else {
			return os_task_failed;
		}
	} else {
		return os_task_not_registerd;
	}
}

/**
 * return the number of task
 */
os_task_id_t os_nmbr_of_tasks(void){
	return os_latest_added_task;
}

/**
 * handle the log function
 */
void os_log( const char *fmt, ...) {
    va_list arg;
    const char _buff[128];
    //Write the log
    va_start(arg, fmt);
    vsprintf (_buff, fmt, arg);
    va_end(arg);

    os_functions_pointer->stdio(_buff);
}


/**
 * initialise the message layer
 * @return
 */
os_return_codes_t os_msg_init(void) {
	os_return_codes_t returnValue;

	//Initialize the queues 
		os_queue_init(&os_msg_buffer.queue,
				&os_msg_buffer.queue_buffer,
				(sizeof(os_msg_buffer.queue_buffer)
						/ sizeof(os_msg_buffer.queue_buffer[0])),
				sizeof(os_msg_buffer.queue_buffer[0]));

	returnValue = os_init_succeed;

	return returnValue;
}

/**
 * Check if a message is pending
 * @return task id of message pending
 */
os_task_id_t os_msg_pending(void) {
	os_task_id_t msg_task;

	//Set the message task to the maximum if not a message is found
	msg_task = OS_MAXIMUM_TASKS;
	//Check if queue is not empty
	if (!os_queue_isEmpty(&os_msg_buffer.queue)) {
		//Get the message from the queue
		os_queue_remove(&os_msg_buffer.queue, &msg_pending);
		//Get the task id from the message
		msg_task = msg_pending.destination;
	}

	return msg_task;
}

/**
 * retreive the message pending
 * @param msg
 * @return
 */
int os_retrieve_msg(os_msg_t *msg)
{
	if(msg_pending.destination == os_current_task_id()){
		*msg = msg_pending.msg;
		return 1;
	} else {
		os_set_error();
	}
	return 0;
}

/**
 * post msg to queue
 * @param msg
 * @param dest_task_id
 * @param prio
 * @return
 */
int os_post_msg(os_msg_t msg, os_task_id_t dest_task_id)
{
	os_msg_queue_entry_t new_entry;
	new_entry.msg = msg;
	new_entry.destination = dest_task_id;
	os_queue_add(&os_msg_buffer.queue, &new_entry);
	return 0;
}

/**
 * queue initialize
 * @param queue
 * @param buffer
 * @param bufferSize
 * @param elementSize
 */
void os_queue_init(os_queue_t *queue, void* buffer, uint16_t bufferSize,
		uint16_t elementSize) {
	queue->buffer = buffer;
	queue->capacity = bufferSize;
	queue->elementSize = elementSize;
	queue->currentSize = 0;
	queue->in = 0;
	queue->out = 0;

}

/**
 * add element to queue
 * @param queue
 * @param element
 */
void os_queue_add(os_queue_t *queue, const void* element) {

	(void) memcpy(queue->buffer + queue->in * queue->elementSize, element,
			queue->elementSize);
	++queue->currentSize;
	++queue->in;
	queue->in %= queue->capacity;

}

/**
 * remove element from queue
 * @param queue
 */
void os_queue_remove(os_queue_t *queue, void* element) {

	if (queue->currentSize > 0) {
		(void) memcpy(element, queue->buffer + queue->out * queue->elementSize,
				queue->elementSize);
		--queue->currentSize;
		++queue->out;
		queue->out %= queue->capacity;
	}
}

/**
 * returns queue size
 * @param queue
 * @return
 */
uint16_t os_queue_size(os_queue_t *queue) {
	return queue->currentSize;
}

/**
 * return the queue capacity
 * @param queue
 * @return
 */
uint16_t os_queue_capacity(os_queue_t *queue) {
	return queue->capacity;
}

/**
 * return true if queue is full
 * @param queue
 * @return
 */
bool os_queue_isFull(os_queue_t *queue) {
	bool full;
	full = queue->currentSize == queue->capacity;
	return full;
}

/**
 * return true is queue is empty
 * @param queue
 * @return
 */
bool os_queue_isEmpty(os_queue_t *queue) {
	bool empty;
	empty = queue->currentSize == 0;
	return empty;
}

/**
 * initialize the timer functions
 * @return  os_init_succeed if succeed
 */
os_return_codes_t os_timer_init(void) {
	os_return_codes_t returnValue = os_init_succeed;
	//Clear os timer counter
	//Start at one so zero can be used to disable timer
	os_timer_counter = 1;
	//Clear and disable all entries
	for (os_timer_id_t id = 0; id < OS_MAXIMUM_TIMERS; id++) {
		os_timers[id].start_value = 0;
		os_timers[id].task_id = OS_MAXIMUM_TASKS;
		os_timers[id].value = 0;
		os_timers[id].used = false;
	}
	//Init the timer pending queue
	os_queue_init(&os_timer_pending_queue, &os_timer_pending_queue_buffer,
		 (sizeof(os_timer_pending_queue_buffer)	/ sizeof(os_timer_pending_queue_buffer[0])),
		 sizeof(os_timer_pending_queue_buffer[0]));
	return returnValue;
}

/**
 * Must be called every OS_COUNTER_TIME to count
 */
void os_timer_count(void) {
	os_timer_counter += OS_COUNTER_TIME;
}

/**
 * Add a timer with default settings. The function will return a unique timer ID which can be used to
 * start or stop the timer
 * @param timer_value_ms
 * @param timer_type
 * @return
 */
os_timer_id_t os_timer_add(uint32_t timer_value_ms, os_timer_type_t timer_type) {
	os_timer_id_t free_slot;
	//Set free slot to max to find empty slot
	free_slot = OS_MAXIMUM_TIMERS;
	//Search for empty timer slot
	for (os_timer_id_t id = 0; id < OS_MAXIMUM_TIMERS; id++) {
		if (!os_timers[id].used) {
			free_slot = id;
			break;
		}
	}

	if (free_slot >= OS_MAXIMUM_TIMERS) {
		os_log("Not enough room to add timer!\n\r");
		os_set_error();
	} else {
		//Add the data to the new timer
		os_timers[free_slot].task_id = os_current_task_id();
		os_timers[free_slot].type = timer_type;
		os_timers[free_slot].value = timer_value_ms;
		os_timers[free_slot].used = true;
	}

	return free_slot;
}

/**
 * Stop a timer. A timer can be stoped even when not running
 * @param timer_id
 */
void os_timer_stop(os_timer_id_t timer_id) {
	//Check if timer is in use
	if (os_timers[timer_id].used) {
		//Set start value to zero to disable timer
		os_timers[timer_id].start_value = 0;
	}
}

/**
 * Start or restart the timer
 * @param timer_id
 */
void os_timer_start(os_timer_id_t timer_id) {
	//Check if timer is in use
	if (os_timers[timer_id].used) {
		os_timers[timer_id].start_value = os_timer_counter;
	}
}

/**
 * Set a new timer value. This is only possible when the timer is stopped
 * @param timer_id
 * @param timer_value_ms
 */
void os_timer_new_value(os_timer_id_t timer_id, uint32_t timer_value_ms) {
	//Check is timer is in use and if timer is stopped
	if((os_timers[timer_id].used) && (os_timers[timer_id].start_value == 0)){
		os_timers[timer_id].value = timer_value_ms;
	}
}

/**
 * Call this function from the OS main loop to check the timers
 */
void os_timer_check_timers(void) {
	//Check all timers if in use
	for(os_timer_id_t id = 0; id < OS_MAXIMUM_TIMERS; id++){
		//Check for in use and for started
		if((os_timers[id].used) && (os_timers[id].start_value > 0)){
			//Check if timer has occurred
			if(os_timer_counter >= (os_timers[id].start_value + os_timers[id].value)){
				//Add the timer to the timer queue
				os_queue_add(&os_timer_pending_queue, &os_timers[id].task_id);
				if(os_timers[id].type == os_timer_repeat){
					os_timer_start(id);
				} else {
					os_timer_stop(id);
				}
			}
		}
	}
}

/**
 * This functions can be called to check is a timer is pending
 * @return  task id of pending timer
 */
os_task_id_t os_timer_pending(void) {
	os_task_id_t pending_task;
	//Set task is to max
	pending_task = OS_MAXIMUM_TASKS;
	//Check if the queue is not empty
	if (!os_queue_isEmpty(&os_timer_pending_queue)) {
		//Get the task id from the queue
		os_queue_remove(&os_timer_pending_queue, &pending_task);
	}
	return pending_task;
}

/**
 * This function will return the counter setting in ms which needs to be used
 * to initialize the calling counter for os_timer_count();
 * @return
 */
uint16_t os_timer_counter_step_setting(void){
	return OS_COUNTER_TIME;
}

