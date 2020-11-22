#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL_scancode.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

/* #include "TUM_Ball.h" */
/* #include "TUM_Draw.h" */
/* #include "TUM_Font.h" */
/* #include "TUM_Event.h" */
/* #include "TUM_Sound.h" */
/* #include "TUM_Utils.h" */
#include "TUM_FreeRTOS_Utils.h"
/* #include "TUM_Print.h" */

/* #include "AsyncIO.h" */

#define mainGENERIC_PRIORITY (tskIDLE_PRIORITY)
#define mainGENERIC_STACK_SIZE ((unsigned short)2560)

/*
The priorities of the tasks.
The task, which actually counts the time is the
most important one, so it gets the highest priority.
The task which reads the user input is the second most
important. Therefor it gets the second highest priority
level. The dask which should display the time is the one
with the lowest priority. Still: None of this tasks
is of priority 0, to avoid conflicts with the idle task.
*/
#define displayTask_PRIORITY (1)
#define inputTask_PRIORITY (2)
#define timeTask_PRIORITY (3)

/*
Chosen size for the input. It was chosen abritary. This can
be done, because we only evaluate the first character inputted.
*/
#define INPUT_SIZE (16)

/*
Needed handles for the tasks. Not further used.
*/
static TaskHandle_t displayHandle = NULL;
static TaskHandle_t inputHandle = NULL;
static TaskHandle_t timeHandle = NULL;

/*
The periods of the tasks. For all tasks the periode
was set 100 ticks. This value was chosen through
experimenting and abritary. One could calculate
the actual periode needed through analyzing the
worst case execution time, but this was out of the
scope for this exercise. The periodes of the tasks
doesn't differ, because there is no real need for it
in my eyes. If one wants the stop watch to be even more
accurate, one could lower the periode of the timeTask
even further. But this wouldn't change the displayed result,
because the displayTask is still executed with the same periode.
If one also wants to display more accurate results, one need to
lower the periode of the display task also.
*/
static portTickType displayPeriode = 100;
static portTickType inputPeriode = 100;
static portTickType timePeriode = 100;

/*
Variables which keep track of the recorded time.
lastRecord is to get the actual time difference
from the current tickCount to the last time the stop
watch counted.
*/
static portTickType stopWatchTime = 0;
static portTickType lastRecord = 0;

/*
Enums to model the state machine and make
things a bit more convinient.
*/
enum{running, stop, clear};
static int currentState = stop;

/*
Array to store user input.
*/
static char input[INPUT_SIZE];

/*
Task body for the task which should take care
of the displaying. Nothing special here, only
the current time is printed. To transfor the
ticks (which is the recorded unit) into actual
seconds or milliseconds, one can use the variable
portTick_Rate_MS. To be honest I'm not sure if I
have to multiply the variable or divide by the variable,
because in my case, it has the value 1. I'm dividing
by 1000, so I can get my results in seconds (inlcusive
floating point).
*/
void displayTaskBody() {
    portTickType lastTimeAwake;
    while(1) {
        lastTimeAwake = xTaskGetTickCount();
        printf("%f\n", (stopWatchTime * portTICK_RATE_MS) / 1000.0);

        /*
        Ensure that the task releases a new job with its periode.
        It is the same for the other tasks and not commented for
        those again.
        */
        vTaskDelayUntil(&lastTimeAwake, displayPeriode);
    }
}

/*
Task body for the task which should evaluate the input
of the user. First the input is got with fgets and put
into the storage variable (input). Then the first char
is evaluated.
r: Running. The stopwatch starts or resumes the counting.
s: Stop. The stopwatch stops the counting, but does not reset it.
c: Clear: The stopwatch clears the counting. It does not start or stop it.
r and s changes the states of the machine. The state "clear" is modeled
implicit, because this is just the reset of a variable.
*/
void inputTaskBody() {
    portTickType lastTimeAwake;
    while(1) {
        lastTimeAwake = xTaskGetTickCount();
        fgets(input, sizeof input, stdin);
        switch(input[0]) {
            case 'r':
                currentState = running;
                break;
            case 's':
                currentState = stop;
                break;
            case 'c':
                stopWatchTime = 0;
                input[0] = 'x';
                break;
        }

        /*
        Even is the time is cleared while the stopwatch is
        running, the system stays in the running state.
        The last time the time is recorded happens here,
        because I want the stopwatch to start as close to
        the time the user input happens as possible.
        */
        if(currentState == running) {
            lastRecord = xTaskGetTickCount();
        }

        vTaskDelayUntil(&lastTimeAwake, inputPeriode);
    }
}

/*
The body of the task which keeps track of the time.
It checks if the current state is "running", if so,
it gets the different between the last time a time
was recorded till the current time. This resulting
value is in ticks. It then just adds the value to the
recorded time.
*/
void timeTaskBody() {
    portTickType lastTimeAwake;
    while(1) {
        lastTimeAwake = xTaskGetTickCount();

        switch(currentState) {
            case running:
                stopWatchTime += lastTimeAwake - lastRecord;
                lastRecord = xTaskGetTickCount();
                break;
        }

        vTaskDelayUntil(&lastTimeAwake, timePeriode);
    }
}

/*
The main function. The three relevant tasks are created and then the scheduler is started.
*/
int main(int argc, char *argv[])
{
    xTaskCreate(displayTaskBody, "Display Task", mainGENERIC_STACK_SIZE * 2, NULL, displayTask_PRIORITY, &displayHandle);
    xTaskCreate(inputTaskBody, "Input Task", mainGENERIC_STACK_SIZE * 2, NULL, inputTask_PRIORITY, &inputHandle);
    xTaskCreate(timeTaskBody, "Time Task", mainGENERIC_STACK_SIZE * 2, NULL, timeTask_PRIORITY, &timeHandle);
    vTaskStartScheduler();

    return EXIT_SUCCESS;
}

// cppcheck-suppress unusedFunction
__attribute__((unused)) void vMainQueueSendPassed(void)
{
    /* This is just an example implementation of the "queue send" trace hook. */
}

// cppcheck-suppress unusedFunction
__attribute__((unused)) void vApplicationIdleHook(void)
{
#ifdef __GCC_POSIX__
    struct timespec xTimeToSleep, xTimeSlept;
    /* Makes the process more agreeable when using the Posix simulator. */
    xTimeToSleep.tv_sec = 1;
    xTimeToSleep.tv_nsec = 0;
    nanosleep(&xTimeToSleep, &xTimeSlept);
#endif
}
