/*! @file
 * RTC.c
 *  @brief Routines for controlling the Real Time Clock (RTC) on the TWR-K70F120M.
 *
 *  @date 3 Sept 2019
 *  @author Abel Queipo 12592503, Yann Clair 13698257
 */
/*!
**  @addtogroup rtc_module RTC module documentation
**  @{
*/

#include "MK70F12.h"
#include <stdint.h>
#include <types.h>
#include "RTC.h"
#include "LEDs.h"


static void (*Callback)(void *);
static void *Arguments;
static bool Initialised = 0;

/*! @brief Initializes the RTC before first use.
 *
 *  Sets up the control register for the RTC and locks it.
 *  Enables the RTC and sets an interrupt every second.
 *  @param userFunction is a pointer to a user callback function.
 *  @param userArguments is a pointer to the user arguments to use with the user callback function.
 *  @return bool - TRUE if the RTC was successfully initialized.
 */
bool RTC_Init(void (*userFunction)(void*), void* userArguments)
{
  Callback = userFunction;
  Arguments = userArguments;

  SIM_SCGC6 |= SIM_SCGC6_RTC_MASK;
  // enable 18pF load capacitors

  RTC_CR |= RTC_CR_SC16P_MASK;
  RTC_CR |= RTC_CR_SC2P_MASK;

  RTC_CR |= RTC_CR_OSCE_MASK;  // enable the oscillator

  for (int i = 0; i < 5000000; i++){;}  // wait for 32.768kHz osc to stabilise

  RTC_IER |= RTC_IER_TSIE_MASK;  //Sets the interrupt
  RTC_IER &= ~RTC_IER_TAIE_MASK;  //Disable Time Alarm Interrupt
  RTC_IER &= ~RTC_IER_TOIE_MASK;  //Time Overflow Interrupt Interrupt
  RTC_IER &= ~RTC_IER_TIIE_MASK;  //Time Invalid Interrupt

  //  clear the error if the invalid timer flag is set
  if (RTC_SR & RTC_SR_TIF_MASK)
    {
      RTC_TSR = 0;
    }

  RTC_LR &= ~RTC_LR_CRL_MASK;  // Lock Control Register
  RTC_SR |= RTC_SR_TCE_MASK; // Initialise Timer Control

  // NVIC stuff
  // vector address 53 = 0x83, IRQ = 67, non-IPR = 2, IPR = 16
  // 67 % 32 = 3
  NVICISER2 = (1 << 3);
  NVICICPR2 = (1 << 3);

  Initialised = 1;
  return 1;

}

/*! @brief Sets the value of the real time clock.
 *
 *  @param hours The desired value of the real time clock hours (0-23).
 *  @param minutes The desired value of the real time clock minutes (0-59).
 *  @param seconds The desired value of the real time clock seconds (0-59).
 *  @note Assumes that the RTC module has been initialized and all input parameters are in range.
 */
void RTC_Set(const uint8_t hours, const uint8_t minutes, const uint8_t seconds)
{
  uint32_t timeSeconds = ((hours % 24) * 3600) + ((minutes % 60) * 60) + (seconds % 60);  // hours, minutes, seconds
  RTC_SR &= ~RTC_SR_TCE_MASK;
  RTC_TSR = timeSeconds;
  RTC_SR |= RTC_SR_TCE_MASK;
}

/*! @brief Gets the value of the real time clock.
 *
 *  @param hours The address of a variable to store the real time clock hours.
 *  @param minutes The address of a variable to store the real time clock minutes.
 *  @param seconds The address of a variable to store the real time clock seconds.
 *  @note Assumes that the RTC module has been initialized.
 */
void RTC_Get(uint8_t* const hours, uint8_t* const minutes, uint8_t* const seconds)
{
  uint32_t currentTime = RTC_TSR;
  *hours = currentTime / 3600;
  *minutes = currentTime / 60 % 60;
  *seconds = currentTime % 60;
}

/*! @brief Interrupt service routine for the RTC.
 *
 *  The RTC has incremented one second.
 *  The user callback function will be called.
 *  @note Assumes the RTC has been initialized.
 */
void __attribute__ ((interrupt)) RTC_ISR(void)
    {
       // if code runs at 0x00;
       if (Initialised == 0)
	 {
	   return;
	 }
       (*Callback)(Arguments);
    }


/*  END rtc  */
/*!
** @}
*/






