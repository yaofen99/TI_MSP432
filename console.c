/*
 * Copyright (c) 2016-2019, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ======== console.c ========
 */
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "includes.h"

/* POSIX Header files */
#include <pthread.h>
#include <semaphore.h>

/* Driver Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/UART.h>

/* Driver configuration */
#include "ti_drivers_config.h"

/* Console display strings */
const char consoleDisplay[]   = "\fConsole (h for help)\r\n";
const char helpPrompt[]       = "Valid Commands\r\n"                  \
                                "--------------\r\n"                  \
                                "h: help\r\n"                         \
                                "q: quit and shutdown UART\r\n"       \
                                "c: clear the screen\r\n"             \
                                "t: display current temperature\r\n";
const char byeDisplay[]       = "Bye! Hit button1 to start UART again\r\n";
const char tempStartDisplay[] = "Current temp = ";
const char tempMidDisplay[]   = "C (";
const char tempEndDisplay[]   = "F)\r\n";
const char cleanDisplay[]     = "\f";
const char userPrompt[]       = "> ";
const char readErrDisplay[]   = "Problem read UART.\r\n";

/* Used to determine whether to have the thread block */
volatile bool uartEnabled = true;
sem_t semConsole;

/* Temperature written by the temperature thread and read by console thread */
extern volatile float temperatureC;
extern volatile float temperatureF;

/* Mutex to protect the reading/writing of the float temperature */
pthread_mutex_t temperatureMutex;


/* Display Header files */
#include <ti/display/Display.h>
/* Display Driver Handle */
Display_Handle displayHandle;

void Display_uart_init(void)
{
    Display_Params displayParams;

    Display_init();

    /* Configure & open Display driver */
    Display_Params_init(&displayParams);
    displayParams.lineClearMode = DISPLAY_CLEAR_BOTH;
    displayHandle = Display_open(Display_Type_UART, &displayParams);
    if (displayHandle == NULL) {
        Display_printf(displayHandle, 0, 0, "Error creating displayHandle\n");
        while (1);
    }
}

/* Used itoa instead of sprintf to help minimize the size of the stack */
static void itoa(int n, char s[]);
static UART_Handle uart;
/*
 *  ======== gpioButtonFxn ========
 *  Callback function for the GPIO interrupt on CONFIG_GPIO_BUTTON_1.
 *  There is no debounce logic here since we are just looking for
 *  a button push. The uartEnabled variable protects use against any
 *  additional interrupts cased by the bouncing of the button.
 */
void gpioButtonFxn(uint_least8_t index)
{

    /* If disabled, enable and post the semaphore */
    if (uartEnabled == false) {
        uartEnabled = true;
        sem_post(&semConsole);
    }
}

void Console_Print( const char *buffer)
{
    while(*buffer)
        UART_write(uart, buffer++, 1);
}

/*
 *  ======== simpleConsole ========
 *  Handle the user input. Currently this console does not handle
 *  user back-spaces or other "hard" characters.
 */
void simpleConsole(UART_Handle uart)
{
    char cmd;
    int status;
    char tempStr[8];
    int localTemperatureC;
    int localTemperatureF;

    UART_write(uart, consoleDisplay, sizeof(consoleDisplay));

    /* Loop until read fails or user quits */
    while (1) {
        UART_write(uart, userPrompt, sizeof(userPrompt));
        status = UART_read(uart, &cmd, sizeof(cmd));
        if (status == 0) {
            UART_write(uart, readErrDisplay, sizeof(readErrDisplay));
            cmd = 'q';
        }

        switch (cmd) {
            case 't':
                UART_write(uart, tempStartDisplay, sizeof(tempStartDisplay));
                /*
                 *  Make sure we are accessing the global float temperature variables
                 *  in a thread-safe manner.
                 */
//                pthread_mutex_lock(&temperatureMutex);
                localTemperatureC = 1;// (int)temperatureC;
                localTemperatureF =  2;//(int)temperatureF;
//                pthread_mutex_unlock(&temperatureMutex);

                itoa((int)localTemperatureC, tempStr);
                UART_write(uart, tempStr, strlen(tempStr));
                UART_write(uart, tempMidDisplay, sizeof(tempMidDisplay));
                itoa((int)localTemperatureF, tempStr);
                UART_write(uart, tempStr, strlen(tempStr));
                UART_write(uart, tempEndDisplay, sizeof(tempEndDisplay));
                Console_Print("abcd\r\n");
                break;

            case 'c':
                UART_write(uart, cleanDisplay, sizeof(cleanDisplay));
                break;
            case 'q':
                UART_write(uart, byeDisplay, sizeof(byeDisplay));
                return;
            case 'h':
            default:
                UART_write(uart, helpPrompt, sizeof(helpPrompt));
                break;
        }
    }
}

/*
 *  ======== consoleThread ========
 */

void *consoleThread(void *arg0)
{
    UART_Params uartParams;
    int retc;

//    /* Configure the button pin */
//    GPIO_setConfig(CONFIG_BUTTON_1, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);
//
//    /* install Button callback and enable it */
//    GPIO_setCallback(CONFIG_BUTTON_1, gpioButtonFxn);
//    GPIO_enableInt(CONFIG_BUTTON_1);
//
//    retc = sem_init(&semConsole, 0, 0);
//    if (retc == -1) {
//        while (1);
//    }

    UART_init();
    Display_uart_init();
#ifdef NORMAL_DEBUG
    DEBUG_printf("Project Initialized! ! \n");
#endif

//   使用console时用
//    /*
//     *  Initialize the UART parameters outside the loop. Let's keep
//     *  most of the defaults (e.g. baudrate = 115200) and only change the
//     *  following.
//     */
//    UART_Params_init(&uartParams);
//    uartParams.writeDataMode  = UART_DATA_BINARY;
//    uartParams.readDataMode   = UART_DATA_BINARY;
//    uartParams.readReturnMode = UART_RETURN_FULL;
//
//    /* Loop forever to start the console */
//    while (1) {
//        if (uartEnabled == false) {
//            retc = sem_wait(&semConsole);
//            if (retc == -1) {
//                while (1);
//            }
//        }
//
//        /* Create a UART for the console */
//        uart = UART_open(CONFIG_UART_1, &uartParams);
//        if (uart == NULL) {
//            while (1);
//        }
//
//        simpleConsole(uart);
//
//        /*
//         * Since we returned from the console, we need to close the UART.
//         * The Power Manager will go into a lower power mode when the UART
//         * is closed.
//         */
//        UART_close(uart);
//        uartEnabled = false;
//    }
}

/*
 * The following function is from good old K & R.
 */
static void reverse(char s[])
{
    int i, j;
    char c;

    for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

/*
 * The following function is from good old K & R.
 */
static void itoa(int n, char s[])
{
    int i, sign;

    if ((sign = n) < 0)  /* record sign */
        n = -n;          /* make n positive */
    i = 0;
    do {       /* generate digits in reverse order */
        s[i++] = n % 10 + '0';   /* get next digit */
    } while ((n /= 10) > 0);     /* delete it */
    if (sign < 0)
         s[i++] = '-';
    s[i] = '\0';
    reverse(s);
}


