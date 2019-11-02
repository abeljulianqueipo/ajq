/*! @file
 *
 *  @brief Routines for setting up the FlexTimer module (FTM) on the TWR-K70F120M.
 *
 *  This contains the functions for operating the FlexTimer module (FTM).
 *
 *  @author Abel Queipo 12592503, Yann Clair 13698257
 *  @date 2015-09-04
 *
 *
 *  @addtogroup FTM_module FTM module documentation
 *  @{
*/
#include "FTM.h"
#include <MK70F12.h>
#include <strings.h>

#define PIT_CHANNEL_COUNT 8

static TFTMChannel const* TimerCache[PIT_CHANNEL_COUNT] = {0};

/*! @brief Sets up the FTM before first use.
 *
 *  Enables the FTM as a free running 16-bit counter.
 *  @return bool - TRUE if the FTM was successfully initialized.
 */
bool FTM_Init()
{
  SIM_SCGC6 |= SIM_SCGC6_FTM0_MASK;

  FTM0_CNTIN = ~FTM_CNTIN_INIT_MASK; // Free-running counter
  FTM0_MOD = FTM_MOD_MOD_MASK;     // Modulo value
  FTM0_CNT = ~FTM_CNT_COUNT_MASK;  // Counter value
  FTM0_SC |= FTM_SC_CLKS(FIXED_FREQ_CLK);  // enable fixed frequency clock

  //Setting up NVIC for FTM
  // Vector=78 IRQ=62
  // NVIC non-IPR=1 IPR=15
  // Clear any pending interrupts on FTM
  NVICICPR1 = (1 << 30); // 62mod32 = 30
  // Enable interrupts from the FTM
  NVICISER1 = (1 << 30);
}


/*! @brief Sets up a timer channel.
 *
 *  @param aFTMChannel is a structure containing the parameters to be used in setting up the timer channel.
 *    channelNb is the channel number of the FTM to use.
 *    delayCount is the delay count (in module clock periods) for an output compare event.
 *    timerFunction is used to set the timer up as either an input capture or an output compare.
 *    ioType is a union that depends on the setting of the channel as input capture or output compare:
 *      outputAction is the action to take on a successful output compare.
 *      inputDetection is the type of input capture detection.
 *    callbackFunction is a pointer to a user callback function.
 *    callbackArguments is a pointer to the user arguments to use with the user callback function.
 *  @return bool - TRUE if the timer was set up successfully.
 *  @note Assumes the FTM has been initialized.
 */
bool FTM_Set(const TFTMChannel* const aFTMChannel)
{
  // checking if channel is within range
  if (aFTMChannel->channelNb >= PIT_CHANNEL_COUNT)
    {
      return 0;
    }

  // making sure it only supports simple output compare
  if (aFTMChannel->timerFunction != TIMER_FUNCTION_OUTPUT_COMPARE)
    {
      return 0;
    }

  // If there is no user function;
  if (!aFTMChannel->callbackFunction)
    {
      return 0;
    }

  TimerCache[aFTMChannel->channelNb] = aFTMChannel;

}

/*! @brief Starts a timer if set up for output compare.
 *
 *  @param aFTMChannel is a structure containing the parameters to be used in setting up the timer channel.
 *  @return bool - TRUE if the timer was started successfully.
 *  @note Assumes the FTM has been initialized.
 */
bool FTM_StartTimer(const TFTMChannel* const aFTMChannel)
{
  if (TimerCache[aFTMChannel->channelNb] != aFTMChannel)
    {
      return 0;
    }
  FTM0_CnSC(aFTMChannel->channelNb) = (FTM_CnSC_MSA_MASK | FTM_CnSC_CHIE_MASK);
  FTM0_CnV(aFTMChannel->channelNb) = FTM0_CNT + aFTMChannel->delayCount;

}


/*! @brief Interrupt service routine for the FTM.
 *
 *  If a timer channel was set up as output compare, then the user callback function will be called.
 *  @note Assumes the FTM has been initialized.
 */
void __attribute__ ((interrupt)) FTM0_ISR(void)
    {
      uint32_t status = FTM0_STATUS;
      for (size_t i = 0; i < PIT_CHANNEL_COUNT; i++)
	{
	  if (TimerCache[i] && (status & (1 << i)))
	      {
	        FTM0_CnSC(i) &= ~FTM_CnSC_CHF_MASK;
	        (TimerCache[i]->callbackFunction)(TimerCache[i]->callbackArguments);
	      }
	}
    }

/*  END FTM Module */
/*!
** @}
*/


