/* ###################################################################
**     Filename    : main.c
**     Project     : Lab3
**     Processor   : MK70FN1M0VMJ12
**     Version     : Driver 01.01
**     Compiler    : GNU C Compiler
**     Date/Time   : 2015-07-20, 13:27, # CodeGen: 0
**     Abstract    :
**         Main module.
**         This module contains user's application code.
**     Settings    :
**     Contents    :
**         No public methods
**
**      Created on: 5 Aug 2019
**      Author: Abel Queipo 1259 2503
**      	Yann Clair 1369 8257**
** ###################################################################*/
/*!
** @file main.c
** @version 3.0
** @brief
**         Main module.
**         This module contains user's application code.
*/         
/*!
**  @addtogroup main_module main module documentation
**  @{
*/         
/* MODULE main */


// CPU module - contains low level hardware initialization routines
#include "Cpu.h"
#include "Events.h"
#include "PE_Types.h"
#include "PE_Error.h"
#include "PE_Const.h"
#include "IO_Map.h"
#include "UART.h"
#include "packet.h"
#include "LEDs.h"
#include "Flash.h"
#include "types.h"
#include "cmd.h"
#include "RTC.h"
#include "FTM.h"
#include "PIT.h"
#include "FIFO.h"
// Analog functions
#include "analog.h"
// OS
#include "OS.h"

volatile uint16union_t* NvTowerNumber;
volatile uint16union_t* NvTowerMode;

// Defining the version no. as global hexidecimal values
uint8_t VersionInteger = 0x01;
uint8_t VersionDecimal = 0x00;

// Defining two unsigned 8-bit variables which results in one 16-bit value
// Last 4 digits of student no. = 2503 which is 0x09C7 in hexadecimal
uint8_t TowerNumberHigh = 0xC7; // LSB for parameter 2 of packet
uint8_t TowerNumberLow = 0x09;  // MSB for parameter 3 of packet

static uint16_t TowerNb = 0x09C7;

const uint8_t PACKET_ACK_MASK = 0x80;  // Packet Acknowledgement Mask

volatile bool isSynchronous;

// holds TAnalogInput for each channel in an array
TAnalogInput Analog_Input[ANALOG_NB_INPUTS];


// ----------------------------------------
// Thread set up
// ----------------------------------------
// Arbitrary thread stack size - big enough for stacking of interrupts and OS use.
#define THREAD_STACK_SIZE 100
#define NB_ANALOG_CHANNELS 2

// Thread stacks
OS_THREAD_STACK(InitModulesThreadStack, THREAD_STACK_SIZE); /*!< The stack for the LED Init thread. */
static uint32_t AnalogThreadStacks[NB_ANALOG_CHANNELS][THREAD_STACK_SIZE] __attribute__ ((aligned(0x08)));

// ----------------------------------------
// Thread priorities
// 0 = highest priority
// ----------------------------------------
const uint8_t ANALOG_THREAD_PRIORITIES[NB_ANALOG_CHANNELS] = {1, 2};

/*! @brief Data structure used to pass Analog configuration to a user thread
 *
 */
typedef struct AnalogThreadData
{
  OS_ECB* semaphore;
  uint8_t channelNb;
} TAnalogThreadData;

/*! @brief Analog thread configuration data
 *
 */
static TAnalogThreadData AnalogThreadData[NB_ANALOG_CHANNELS] =
{
  {
    .semaphore = NULL,
    .channelNb = 0
  },
  {
    .semaphore = NULL,
    .channelNb = 1
  }
};

void LPTMRInit(const uint16_t count)
{
  // Enable clock gate to LPTMR module
  SIM_SCGC5 |= SIM_SCGC5_LPTIMER_MASK;

  // Disable the LPTMR while we set up
  // This also clears the CSR[TCF] bit which indicates a pending interrupt
  LPTMR0_CSR &= ~LPTMR_CSR_TEN_MASK;

  // Enable LPTMR interrupts
  LPTMR0_CSR |= LPTMR_CSR_TIE_MASK;
  // Reset the LPTMR free running counter whenever the 'counter' equals 'compare'
  LPTMR0_CSR &= ~LPTMR_CSR_TFC_MASK;
  // Set the LPTMR as a timer rather than a counter
  LPTMR0_CSR &= ~LPTMR_CSR_TMS_MASK;

  // Bypass the prescaler
  LPTMR0_PSR |= LPTMR_PSR_PBYP_MASK;
  // Select the prescaler clock source
  LPTMR0_PSR = (LPTMR0_PSR & ~LPTMR_PSR_PCS(0x3)) | LPTMR_PSR_PCS(1);

  // Set compare value
  LPTMR0_CMR = LPTMR_CMR_COMPARE(count);

  // Initialize NVIC
  // see p. 91 of K70P256M150SF3RM.pdf
  // Vector 0x65=101, IRQ=85
  // NVIC non-IPR=2 IPR=21
  // Clear any pending interrupts on LPTMR
  NVICICPR2 = NVIC_ICPR_CLRPEND(1 << 21);
  // Enable interrupts from LPTMR module
  NVICISER2 = NVIC_ISER_SETENA(1 << 21);

  //Turn on LPTMR and start counting
  LPTMR0_CSR |= LPTMR_CSR_TEN_MASK;
}

void __attribute__ ((interrupt)) LPTimer_ISR(void)
{
  //OS_ISREnter();

  // Clear interrupt flag
  LPTMR0_CSR |= LPTMR_CSR_TCF_MASK;

  // Signal the analog channels to take a sample
  for (uint8_t analogNb = 0; analogNb < NB_ANALOG_CHANNELS; analogNb++)
    {
      Analog_Get(channelNb);
      if (isSynchronous)
	{
	  Packet_Put(CMD_RX_ANALOG_INPUT, channelNb, Analog_Input[channelNb].value.s.Lo, Analog_Input[channelNb].value.s.Hi);
	}
      else
	{
	  if (Analog_Input[channelNb].value.l != Analog_Input[channelNb].oldValue.l)
	    Packet_Put(CMD_RX_ANALOG_INPUT, channelNb, Analog_Input[channelNb].value.s.Lo, Analog_Input[channelNb].value.s.Lo);
	}
    }


  //OS_ISRExit();
}

/*! @brief Initialises modules.
 *
 */
static void InitModulesThread(void* pData)
{
  // Analog
  (void)Analog_Init(CPU_BUS_CLK_HZ);

  // Generate the global analog semaphores
  //for (uint8_t analogNb = 0; analogNb < NB_ANALOG_CHANNELS; analogNb++)
    //AnalogThreadData[analogNb].semaphore = OS_SemaphoreCreate(0);

  // Initialise the low power timer to tick every 10 ms
  LPTMRInit(10);

  // We only do this once - therefore delete this thread
  //OS_ThreadDelete(OS_PRIORITY_SELF);
}

/*! @brief Samples a value on an ADC channel and sends it to the corresponding DAC channel.
 *
 */
/*
void AnalogLoopbackThread(void* pData)
{
  // Make the code easier to read by giving a name to the typecast'ed pointer
  #define analogData ((TAnalogThreadData*)pData)

  for (;;)
  {
    int16_t analogInputValue;

    //(void)OS_SemaphoreWait(analogData->semaphore, 0);
    // Get analog sample
    Analog_Get(analogData->channelNb, &analogInputValue);
    // Put analog sample
    Analog_Put(analogData->channelNb, analogInputValue);
  }
}
*/


/*
 *  Toggles Blue LED LOW
 */
void BlueLedOff(void *arguments)
{
  LEDs_Off(LED_BLUE);
}

// 1 second Timer for FTM Channel
const static TFTMChannel PacketTimer = {0, 24414, TIMER_FUNCTION_OUTPUT_COMPARE, TIMER_OUTPUT_DISCONNECT, &BlueLedOff, (void *)0};



/*!
 * @brief Handle incoming packets
 */
void PacketHandle()
{
  bool error = 1;
  uint8_t data;
  //mask out the ack, otherwise it goes to default
  switch (Packet_Command & ~PACKET_ACK_MASK)
  {
    case CMD_RX_STARTUP_VALUES:
      error = !CMD_GetStartupValues();
      break;

    case CMD_RX_PROGRAM_BYTE:
      error = !CMD_FlashProgramByte(Packet_Parameter1, Packet_Parameter3);
      break;

    case CMD_RX_READ_BYTE:
      error = !CMD_FlashReadByte(Packet_Parameter1);
      break;

    case CMD_RX_GET_VERSION:
      error = !CMD_TowerVersion();
      break;

    case CMD_RX_TOWER_NUMBER:
      error = !CMD_TowerNumber(Packet_Parameter1, Packet_Parameter2, Packet_Parameter3);
      break;

    case CMD_RX_TOWER_MODE:
      error = !CMD_TowerMode(Packet_Parameter1, Packet_Parameter2, Packet_Parameter3);
    break;
    case CMD_RX_SET_TIME:
      error = !CMD_SetTime(Packet_Parameter1, Packet_Parameter2, Packet_Parameter3);
      default:
	break;
  }

  if (Packet_Command & PACKET_ACK_MASK)
    {
      uint8_t maskedPacket = 0;
      if (error)
	{
	  maskedPacket = Packet_Command & ~PACKET_ACK_MASK;
	}
      else
	{
	  maskedPacket = Packet_Command | PACKET_ACK_MASK;
	}
      Packet_Put(maskedPacket, Packet_Parameter1, Packet_Parameter2, Packet_Parameter3);
	}
}

/*! @brief User callback function for RTC
 *
 *  @param arguments Pointer to the user argument to use with the user callback function
 *  @note Assumes RTC interrupt has occurred
 */
void RtcCallback(void *arguments)
{
  uint8_t h, m, s;
  RTC_Get(&h, &m, &s);
  CMD_SendTime(h, m, s);
  LEDs_Toggle(LED_YELLOW);
}

/*! @brief User callback function for PIT
 *
 *  @param arguments Pointer to the user argument to use with the user callback function
 *  @note Assumes PIT interrupt has occurred
 */
void PitCallback(void *arguments)
{
  LEDs_Toggle(LED_GREEN);
  AnalogIn();
}

/*lint -save  -e970 Disable MISRA rule (6.3) checking. */
int main(void)
/*lint -restore Enable MISRA rule (6.3) checking. */
{
  /* Write your local variable definition here */

  /*** Processor Expert internal initialization. DON'T REMOVE THIS CODE!!! ***/

  PE_low_level_init();

  /*** End of Processor Expert internal initialization.                    ***/

    __DI();
  //EnterCritical();

  LEDs_Init();

  PIT_Init(CPU_BUS_CLK_HZ, &PitCallback, (void *)0);
  PIT_Set(500000000, 0);
  PIT_Enable(1);

  Packet_Init(115200, CPU_BUS_CLK_HZ);
  // Initialising UART
  UART_Init(115200, CPU_BUS_CLK_HZ);
  Flash_Init();
  CMD_Init();

  FTM_Init();
  FTM_Set(&PacketTimer);

  // Initialise RTC last
  RTC_Init(&RtcCallback, (void *)0);

    LEDs_On(LED_ORANGE);
  //if (Flash_AllocateVar((void* )&NvTowerNumber, sizeof(*NvTowerNumber)))
    //{
      //Flash_Write16((uint16_t* )NvTowerNumber, TowerNb);
    //}

  //if (Flash_AllocateVar((void* )&NvTowerMode, sizeof(*NvTowerMode)))
    //{
      //Flash_Write16((uint16_t* )NvTowerMode, 1);
    //}
  //todo: review above code - may be overwriting
  //ExitCritical();



  __EI();
  //OS_ERROR error;

  //OS_Init(CPU_CORE_CLK_HZ, true);


  //sending over the 'tower startup', 'tower version' and 'tower number' at startup
  CMD_GetStartupValues();

  /* Write your code here */
  for (;;)
  {
      //UART_Poll(); // poll the UART
      if(Packet_Get()) // if we receive a full packet
      {
	 LEDs_On(LED_BLUE);  // Toggle LED HIGH when packet is sent through
	 FTM_StartTimer(&PacketTimer);  // Update Timer Setting
 	 PacketHandle(); // handle the packet
      }
      // Start multithreading - never returns!

  }
  //OS_Start();

  /*** Don't write any code pass this line, or it will be deleted during code generation. ***/
  /*** RTOS startup code. Macro PEX_RTOS_START is defined by the RTOS component. DON'T MODIFY THIS CODE!!! ***/
  #ifdef PEX_RTOS_START
    PEX_RTOS_START();                  /* Startup of the selected RTOS. Macro is defined by the RTOS component. */
  #endif
  /*** End of RTOS startup code.  ***/
  /*** Processor Expert end of main routine. DON'T MODIFY THIS CODE!!! ***/
  for(;;){}
  /*** Processor Expert end of main routine. DON'T WRITE CODE BELOW!!! ***/
} /*** End of main routine. DO NOT MODIFY THIS TEXT!!! ***/

/* END main */
/*!
** @}
*/
/*
** ###################################################################
**
**     This file was created by Processor Expert 10.5 [05.21]
**     for the Freescale Kinetis series of microcontrollers.
**
** ###################################################################
*/
