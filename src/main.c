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

#define displayTask_PRIORITY (1)
#define inputTask_PRIORITY (2)
#define timeTask_PRIORITY (3)

#define INPUT_SIZE (16)

static TaskHandle_t displayHandle = NULL;
static TaskHandle_t inputHandle = NULL;
static TaskHandle_t timeHandle = NULL;

static portTickType displayPeriode = 100;
static portTickType inputPeriode = 100;
static portTickType timePeriode = 100;

static portTickType stopWatchTime = 0;
static portTickType lastRecord = 0;

enum{running, stop, clear};
static int currentState = stop;

static char input[INPUT_SIZE];

void displayTaskBody() {
    portTickType lastTimeAwake;
    while(1) {
        lastTimeAwake = xTaskGetTickCount();
        printf("%f\n", (stopWatchTime * portTICK_RATE_MS) / 1000.0);
        vTaskDelayUntil(&lastTimeAwake, displayPeriode);
    }
}

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

        if(currentState == running) {
            lastRecord = xTaskGetTickCount();
        }

        vTaskDelayUntil(&lastTimeAwake, inputPeriode);
    }
}

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
