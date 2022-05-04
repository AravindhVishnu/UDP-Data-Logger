#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <xdc/std.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>
#include <xdc/runtime/Types.h>
#include <xdc/cfg/global.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/family/arm/a8/Mmu.h>
#include <ti/sysbios/timers/dmtimer/Timer.h>
#include <ti/board/board.h>
#include <ti/drv/uart/UART.h>
#include <ti/drv/uart/UART_stdio.h>
#include "udp_data_logger.h"
#include "network/network.h"

static void init();
static void myTimerTask();
static void myIsr();

static UInt32 timerTaskCnt = 0;
static Semaphore_Handle semTimer;

//----------------------------------------------------------------------------
int main()
{
    init();
    BIOS_start();
    return 0;
}

//----------------------------------------------------------------------------
void init()
{
    Board_initCfg boardCfg = BOARD_INIT_PINMUX_CONFIG |
                             BOARD_INIT_MODULE_CLOCK  |
                             BOARD_INIT_UART_STDIO;

    Board_STATUS status = Board_init(boardCfg);
    if (status != BOARD_SOK)
    {
        UART_printf("\nBoard init failed\n");
        BIOS_exit(0);
    }
    UART_printf("\nBoard init complete\n");

    networkInit();
    UART_printf("\nNetwork init complete\n");

    Error_Block eb;
    Semaphore_Params semaphoreParams;

    Semaphore_Params_init(&semaphoreParams);
    semaphoreParams.mode = Semaphore_Mode_BINARY;
    Error_init(&eb);
    semTimer = Semaphore_create(0, &semaphoreParams, &eb);
    if (semTimer == NULL)
    {
        UART_printf("\nSemaphore creation failed\n");
        BIOS_exit(0);
    }

    Task_Params taskParams;
    Task_Handle taskHandle;

    Error_init(&eb);
    Task_Params_init(&taskParams);
    taskParams.instance->name = "Timer";
    taskParams.priority = 3;
    taskHandle = Task_create(myTimerTask, &taskParams, &eb);
    if (taskHandle == NULL)
    {
        UART_printf("\nTimer task creation failed\n");
        BIOS_exit(0);
    }

    Timer_Params timerParams;
    Timer_Handle myTimer;
    Types_FreqHz freq;

    Error_init(&eb);
    Timer_Params_init(&timerParams);
    timerParams.period = 100000;
    timerParams.periodType = Timer_PeriodType_MICROSECS;
    timerParams.startMode = Timer_StartMode_AUTO;
    timerParams.runMode = Timer_RunMode_CONTINUOUS;
    timerParams.extFreq.hi = 0;
    timerParams.extFreq.lo = 24000000;
    myTimer = Timer_create(1, (Timer_FuncPtr)myIsr, &timerParams, &eb);  /* DM Timer 3 has Timer ID 1 */
    if (myTimer == NULL)
    {
        UART_printf("\nTimer creation failed\n");
        System_abort("Timer create failed");
    }

    Timer_getFreq(myTimer, &freq);
    if (freq.lo != timerParams.extFreq.lo)
    {
        UART_printf("\nError, incorrect timer frequency\n");
        System_abort("Error, incorrect timer frequency");
    }

    UART_printf("\nSystem init complete\n");
}

//----------------------------------------------------------------------------
void myIsr()
{
    Semaphore_post(semTimer);
}

//----------------------------------------------------------------------------
void myTimerTask()
{
    while(1)
    {
        Semaphore_pend(semTimer, BIOS_WAIT_FOREVER);
        udpSend();
        ++timerTaskCnt;
    }
}
