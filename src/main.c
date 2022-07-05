/****** Note *****/
/* After Program Run will get outputs in
 * two ways:
 * 1. in console terminal
 * 2. in two text files
 * 		1. filename.txt --> contains total no.of transmitted, blocked, received messages in every range
 * 		2. Random_Period.txt --> contains random period generation in each range
 */
/****************/

/******* Include Libraries *******/
#include <stdio.h>						// standard input output library
#include <stdlib.h>						// standard library
#include <string.h>						// string library to use (strlen, strcat, .....)
#include "diag/trace.h"
#include <time.h>						// to get time noe for random generation numbers
#include <math.h>						// for using any math expression
#include "FreeRTOS.h"						// FreeRTOS library
#include "task.h"						// Task library
#include "queue.h"						// queue library
#include "timers.h"						// timer library
#include "semphr.h"						// semaphore library
/**********************************/

#define CCM_RAM __attribute__((section(".ccmram")))

/******* Define Global Variables *******/
static void CallBack_Timer1(TimerHandle_t xTimer);
static void CallBack_Timer2(TimerHandle_t xTimer);
static void CallBack_Timer3(TimerHandle_t xTimer);

static TimerHandle_t xTimer1 = NULL;
static TimerHandle_t xTimer2 = NULL;
static TimerHandle_t xTimer3 = NULL;

BaseType_t xTimer1Started;
BaseType_t xTimer2Started;
BaseType_t xTimer3Started;

TaskHandle_t HandleSender1 = NULL;
TaskHandle_t HandleSender2 = NULL;
TaskHandle_t HandleReceiver = NULL;

int no_received_messages = 0;
int no_transmitted_messages = 0;
int no_blocked_messages = 0;

QueueHandle_t MyQueue;
SemaphoreHandle_t MySemapore_Sender = NULL;
SemaphoreHandle_t MySemapore_Reciever = NULL;

char msg_Send[30];
const int Lower_bound_array[] = {50, 80, 110, 140, 170, 200};
const int Upper_bound_array[] = {150, 200, 250, 300, 350, 400};
int Lower_bound = 50;
int Upper_bound = 150;
int Random_period_sender1 = 100;
int Random_period_sender2 = 100;
/**********************************/

/******* Useful Functions we Implemented *******/
TickType_t length_of_number(int n)
{
	TickType_t count = 0;
    while (n)
    {
        count++;
        n /= 10;
    }
    return count;
}
TickType_t str_len(char *str)
{
	TickType_t count = 0;
    while (str[count])
        count++;
    return count;
}
void str_cat_num(char *str, TickType_t n)
{
	TickType_t count = length_of_number(n);
	TickType_t count2 = str_len(str);
    for (TickType_t i = count2 + count -1; i >= count2 ; i--)
    {
        str[i] = n % 10 + '0';
        n /= 10;
    }
    str[count + count2] = '\0';
}
void Send_message_via_Queue(QueueHandle_t Queue_Handle, char *msg, char *msg_to_send)
{
    sprintf(msg, msg_to_send);
    if (xQueueSend(Queue_Handle, msg, (TickType_t)0))
    {
//        printf("\"%s\" is sent\n", msg_to_send);
        no_transmitted_messages++;
    }
    else
    {
//        printf("\"%s\" doesn't sent\n", msg_to_send);
        no_blocked_messages++;
    }
}
char *str_cat(char *str)
{
    unsigned char count = 0;
    while (str[count])
        count++;
    if (str[count - 1] != '9')
    {
        str[count - 1]++;
    }
    else if (str[count - 1] == '9')
    {
        if (str[count - 2] >= '0' && str[count - 2] < '9')
        {
            str[count - 2]++;
            str[count - 1] = '0';
        }
        else if (str[count - 2] == '9' && str[count - 3] == ' ')
        {
            str[count] = '0';
            str[count - 1] = '0';
            str[count - 2] = '1';
        }
        else if (str[count - 2] == '9' && str[count - 3] != ' ')
        {
            str[count - 3]++;
            str[count - 2] = '0';
            str[count - 1] = '0';
        }
        else if (str[count - 2] == ' ')
        {
            str[count] = '0';
            str[count - 1] = '1';
        }
    }
    return str;
}
/**********************************/

/******* Our Sender and Receiver Tasks *******/
void MySender1(void *p)
{
    char msg_to_send[30] = "Time is ";
    while (1)
    {
    	str_cat_num(msg_to_send, xTaskGetTickCount());
    	if(xSemaphoreTake(MySemapore_Sender,portMAX_DELAY))
    	{
    		Send_message_via_Queue(MyQueue, msg_Send, msg_to_send);
        	msg_to_send[8] = '\0';
        	xSemaphoreGive(MySemapore_Sender);
    	}
        vTaskDelay(pdMS_TO_TICKS(Random_period_sender1));
    }
}
void MySender2(void *p)
{
    char msg_to_send[30] = "Time is ";
    while (1)
    {
    	str_cat_num(msg_to_send, xTaskGetTickCount());
    	if(xSemaphoreTake(MySemapore_Sender,portMAX_DELAY))
    	{
    		Send_message_via_Queue(MyQueue, msg_Send, msg_to_send);
        	msg_to_send[8] = '\0';
        	xSemaphoreGive(MySemapore_Sender);
    	}
        vTaskDelay(pdMS_TO_TICKS(Random_period_sender2));
    }
}
void MyReceiver(void *p)
{
    char buffer[30];
    int test = 0;
    while (1)
    {
    	if(xSemaphoreTake(MySemapore_Reciever,portMAX_DELAY))
    	{
    		if (xQueueReceive(MyQueue, buffer, (TickType_t)1000))
    		{
//    			printf("Data Received is \"%s\"\n", buffer);
    			no_received_messages++;
    		}
    		else
//    			printf("Queue is empty\n");
        	xSemaphoreGive(MySemapore_Reciever);
    	}
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
/**********************************/

/******* Reset Function *******/
void Reset_Function()
{
	FILE *file;
	file = fopen("filename.txt","aw");
	fprintf(file,"Range From %d : %d\n", Lower_bound,Upper_bound);
	fprintf(file,"Total no.of Transmitted messages are: %d\n", no_transmitted_messages);
	fprintf(file,"Total no.of Blocked messages are: %d\n", no_blocked_messages);
	fprintf(file,"Total no.of Received messages are: %d\n", no_received_messages);
	fprintf(file,"------------------\n");
	fclose(file);

	printf("Total no.of Transmitted messages are: %d\n", no_transmitted_messages);
	printf("Total no.of Blocked messages are: %d\n", no_blocked_messages);
	printf("Total no.of Received messages are: %d\n", no_received_messages);
	printf("------------------\n");
	no_transmitted_messages = 0;
	no_blocked_messages = 0;
	no_received_messages = 0;

	xQueueReset(MyQueue);
	static int index = 0;
	if(index == 5)
	{
		printf("Game Over\n");
		exit(0);
	}

	Lower_bound = Lower_bound_array[++index];
	Upper_bound = Upper_bound_array[index];
	Random_period_sender1 = Lower_bound + rand()%(Upper_bound-Lower_bound);
	Random_period_sender2 = Lower_bound + rand()%(Upper_bound-Lower_bound);

	FILE *file2;
	file2 = fopen("Random_Period.txt","aw");
	fprintf(file2,"---------------\n");
	fclose(file2);
}
/**********************************/

/******* Out Main Function *******/
int main(int argc, char *argv[])
{
	trace_puts("--------------------------------\n\tStart of Program\n--------------------------------\n");

    time_t t;
    srand((unsigned)time(&t));

	xTaskCreate(MySender1, "Sender1", 200, (void *)0, tskIDLE_PRIORITY, &HandleSender1);
    xTaskCreate(MySender2, "Sender2", 200, (void *)0, tskIDLE_PRIORITY, &HandleSender2);
    xTaskCreate(MyReceiver, "Receiver", 200, (void *)0, tskIDLE_PRIORITY, &HandleReceiver);

    MyQueue = xQueueCreate(2, sizeof(msg_Send));
    vSemaphoreCreateBinary(MySemapore_Sender);
    vSemaphoreCreateBinary(MySemapore_Reciever);

    xTimer1 = xTimerCreate("Timer1", (pdMS_TO_TICKS(100)), pdTRUE, (void *)0, CallBack_Timer1);   // periodic timer1 after 2s
    xTimer2 = xTimerCreate("Timer2", (pdMS_TO_TICKS(100)), pdTRUE, (void *)1, CallBack_Timer2); // periodic timer2 every 1s
    xTimer3 = xTimerCreate("Timer3", (pdMS_TO_TICKS(100)), pdTRUE, (void *)1, CallBack_Timer3);  // periodic timer2 every 1s


    if ((xTimer1 != NULL) && (xTimer2 != NULL) && (xTimer3 != NULL))
    {
        xTimer1Started = xTimerStart(xTimer1, 0);
        xTimer2Started = xTimerStart(xTimer2, 0);
        xTimer3Started = xTimerStart(xTimer3, 0);
    }

    if (xTimer1Started == pdPASS && xTimer2Started == pdPASS && xTimer2Started == pdPASS)
        vTaskStartScheduler();
    else
    	trace_puts("the start command could not be sent to the timer command queue\n");
    return 0;
}
/**********************************/

#pragma GCC diagnostic pop

/******* CallBack Functions For Timers *******/
static void CallBack_Timer1(TimerHandle_t xTimer)
{
    static unsigned char control = 0;
    if (control++ % 2 == 0)
        vTaskSuspend(HandleSender1);
    else
        vTaskResume(HandleSender1);
	Random_period_sender1 = Lower_bound + rand()%(Upper_bound-Lower_bound + 1);
//    printf("Random Period Sender1: %d\n",Random_period_sender1);
	FILE *file;
	file = fopen("Random_Period.txt","aw");
	fprintf(file,"%d\t%d\n", Random_period_sender1,Random_period_sender2);
	fclose(file);
    xTimerChangePeriod(xTimer,pdMS_TO_TICKS(Random_period_sender1),(TickType_t)0);
    xSemaphoreGive(MySemapore_Sender);
}
static void CallBack_Timer2(TimerHandle_t xTimer)
{
    static unsigned char control = 0;
    if (control++ % 2 == 0)
        vTaskSuspend(HandleSender2);
    else
        vTaskResume(HandleSender2);
	Random_period_sender2 = Lower_bound + rand()%(Upper_bound-Lower_bound+1);
    xTimerChangePeriod(xTimer,pdMS_TO_TICKS(Random_period_sender2),(TickType_t)0);
    xSemaphoreGive(MySemapore_Sender);
}
static void CallBack_Timer3(TimerHandle_t xTimer)
{
    static unsigned char control = 0;
    if (control++ % 2 == 0)
        vTaskSuspend(HandleReceiver);
    else
        vTaskResume(HandleReceiver);
    if(no_received_messages >= 500)
    	Reset_Function();
	xSemaphoreGive(MySemapore_Reciever);
}
/**********************************/

/******* Some Functions in Demo Project *******/
void vApplicationMallocFailedHook(void)
{
    /* Called if a call to pvPortMalloc() fails because there is insufficient
    free memory available in the FreeRTOS heap.  pvPortMalloc() is called
    internally by FreeRTOS API functions that create tasks, queues, software
    timers, and semaphores.  The size of the FreeRTOS heap is set by the
    configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */
    for (;;)
        ;
}
void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName)
{
    (void)pcTaskName;
    (void)pxTask;

    /* Run time stack overflow checking is performed if
    configconfigCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
    function is called if a stack overflow is detected. */
    for (;;)
        ;
}
void vApplicationIdleHook(void)
{
    volatile size_t xFreeStackSpace;

    /* This function is called on each cycle of the idle task.  In this case it
    does nothing useful, other than report the amout of FreeRTOS heap that
    remains unallocated. */
    xFreeStackSpace = xPortGetFreeHeapSize();

    if (xFreeStackSpace > 100)
    {
        /* By now, the kernel has allocated everything it is going to, so
        if there is a lot of heap remaining unallocated then
        the value of configTOTAL_HEAP_SIZE in FreeRTOSConfig.h can be
        reduced accordingly. */
    }
}
void vApplicationTickHook(void)
{
}
StaticTask_t xIdleTaskTCB CCM_RAM;
StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE] CCM_RAM;
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize)
{
    /* Pass out a pointer to the StaticTask_t structure in which the Idle task's
    state will be stored. */
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

    /* Pass out the array that will be used as the Idle task's stack. */
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;

    /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
    Note that, as the array is necessarily of type StackType_t,
    configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}
static StaticTask_t xTimerTaskTCB CCM_RAM;
static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH] CCM_RAM;
/* configUSE_STATIC_ALLOCATION and configUSE_TIMERS are both set to 1, so the
application must provide an implementation of vApplicationGetTimerTaskMemory()
to provide the memory that is used by the Timer service task. */
void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize)
{
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
/**********************************/
