/* 
  Scheduler example. Based on RocksOS
  https://github.com/gertjanrocks/rocksOS/

 * -----------------------------------------------------------------------------
 * Author: Gertjan Rocks
 * Web:    www.gertjanrocks.com
 * Mail:   gertjanrocks@outlook.com
 * -----------------------------------------------------------------------------

 */

#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include <Arduino.h>
/* -----------------*
 * Type definitions *
 * -----------------*
 */

typedef enum {
	os_failed = -2,				//! Os has failed OS should stop working
	os_init_failed,				//! Init has failed OS should not start
	os_init_succeed,			//! Init is successful OS will start
	os_succesful				//! Return never
} os_return_codes_t;

typedef enum {
	os_task_succeed, //! The called task is succeed and only needs be to called when necessary
	os_task_failed,          //! The called task has failed stops running the OS
	os_task_not_registerd		//!The task is not registered for the event
} os_task_return_codes_t;

typedef enum {
	os_event_msg_pending, 		//! Message is pending
	os_event_init,		//! Os is started and calls each task one time on init
	os_event_idle,	//! No messages and if activated timer events are pending
	os_event_timer,
	os_nmbr_of_events
} os_event_t;

typedef struct {
	uint16_t os_msg_id;	//! Message ID of the message
	uint16_t data;			//! Data along with the message
} os_msg_t;

typedef struct {
	const char* task_name;	//! Task name
	os_task_return_codes_t (*task_cb)(os_event_t); //! Callback function for the task
	uint32_t identifier; //! User identifier, is not used by the os. Can be changed and read during a running os
} os_task_t;

typedef uint16_t os_task_id_t;  //! The task id assigned to a tasks

typedef enum {
	os_timer_repeat,	//! Timer will auto restart after timer was ended
	os_timer_one_shot //! Timer needs a start command after finishing
} os_timer_type_t;

typedef uint16_t os_timer_id_t;

typedef struct {
	os_task_t task;
	os_task_id_t _this;
	bool event_subscribe[os_nmbr_of_events];
} os_task_admin_t;

typedef struct {
	void (*stdio)( const char *fmt);
} os_functions_pointers_t;

/* ----------------------*
 * Function declarations *
 * ----------------------*
 */

extern int os_add_task(os_task_t new_task);
extern os_task_id_t os_current_task_id(void);
extern int os_subscribe_for_event(os_event_t event, os_task_id_t task_id);

extern int os_post_msg(os_msg_t msg, os_task_id_t dest_task_id);
extern int os_retrieve_msg(os_msg_t *msg);

extern void os_timer_count(void);
os_timer_id_t os_timer_add(uint32_t timer_value_ms, os_timer_type_t timer_type);
extern void os_timer_stop(os_timer_id_t timer_id);
extern void os_timer_start(os_timer_id_t timer_id);
extern void os_timer_new_value(os_timer_id_t timer_id, uint32_t timer_value_ms);
extern uint16_t os_timer_counter_step_setting(void);

extern void os_log(const char *fmt, ...);

extern os_return_codes_t os_init(void);
extern os_return_codes_t os_main(void);

extern os_return_codes_t os_add_function_pointers(os_functions_pointers_t* os_functions);

/* -------------------------------*
 * Constant and macro definitions *
 * -------------------------------*
 */

/** SIZE settings **/
#define OS_MAXIMUM_TASKS	(4)		//! Maximum number of tasks
#define OS_MSG_QUEUE_SIZE	(4)		//! Size of the message queue for each prio
#define OS_MAXIMUM_TIMERS	(4)	//! Number of availble timers
#define OS_COUNTER_TIME		(10)	//! Every OS_COUNTER_TIME ms the os_timer_count will be called

/* -------------------------------*
 * "Private" Functions *
 * -------------------------------*
 */

extern os_return_codes_t os_task_init(void);
extern os_task_return_codes_t os_run_task(os_task_id_t task_id, os_event_t event);
extern os_task_id_t os_nmbr_of_tasks(void);

extern void os_set_error(void);

extern os_return_codes_t os_msg_init(void);
extern os_task_id_t os_msg_pending(void);

extern os_return_codes_t os_timer_init(void) ;
extern void os_timer_check_timers(void) ;
extern os_task_id_t os_timer_pending(void);


/* -----------------*
 * Queue related items *
 * -----------------*
 */

typedef struct
{
   void* buffer;             //!< \private
   uint16_t capacity;        //!< \private
   uint16_t elementSize;     //!< \private
   uint16_t currentSize;     //!< \private
   int in;                   //!< \private
   int out;                  //!< \private
}os_queue_t;

void os_queue_init(os_queue_t *queue, void* buffer, uint16_t bufferSize, uint16_t elementSize);
void os_queue_add(os_queue_t *queue, const void* element);
void os_queue_remove(os_queue_t *queue, void* element);
uint16_t os_queue_size(os_queue_t *queue);
uint16_t os_queue_capacity(os_queue_t *queue);
bool os_queue_isFull(os_queue_t *queue);
bool os_queue_isEmpty(os_queue_t *queue);

#endif /* SCHEDULER_H_ */