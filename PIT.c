/*! @file
 * PIT.c
 *  @brief Routines for controlling Periodic Interrupt Timer (PIT) on the TWR-K70F120M.
 *
 *  @date 3 Sept 2019
 *  @author Abel Queipo 12592503, Yann Clair 13698257
 *
 *  @addtogroup pit_module PIT module documentation
 *  @{
 */

#include "PIT.h"
#include "MK70F12.h"

#define NS_IN_1_SEC 1000000000

#define NANO_SECONDS_IN_A_SECOND 1000000000
#define NANO_SECONDS_IN_10_MS 10000000

#define VOLTAGE_CHANNEL 1
#define CURRENT_CHANNEL 2

static void (*Callback)(void *);
static void *Arguments;
static bool Initialised = 0;
static uint32_t Clock;

int16_t Meter_Voltage;
int16_t Meter_Current;

/*! @brief Sets up the PIT before first use.
 *
 *  Enables the PIT and freezes the timer when debugging.
 *  @param moduleClk The module clock rate in Hz.
 *  @param userFunction is a pointer to a user callback function.
 *  @param userArguments is a pointer to the user arguments to use with the user callback function.
 *  @return bool - TRUE if the PIT was successfully initialized.
 *  @note Assumes that moduleClk has a period which can be expressed as an integral number of nanoseconds.
 */
bool PIT_Init(const uint32_t moduleClk, void (*userFunction)(void*), void* userArguments)
{
  Callback = userFunction;
  Arguments = userArguments;
  Clock = moduleClk;

  Initialised = 1;

  SIM_SCGC6 |= SIM_SCGC6_PIT_MASK; // enable PIT clock gate control

  PIT_MCR = PIT_MCR_FRZ_MASK;  // allows timers to freeze during debug session

  PIT_TCTRL0 |= PIT_TCTRL_TIE_MASK; // Timer Interrupt Enable

  // Setting up NVIC for PIT see K70 manual pg 97, 99
  // Vector=84, IRQ=68
  // NVIC non-IPR=2 IPR=17
  // Clear any pending interrupts on PIT
  NVICICPR2 = (1 << 4);
  // Enable Interrupts for PIT0
  NVICISER2 = (1 << 4);
}

/*! @brief Sets the value of the desired period of the PIT.
 *
 *  @param period The desired value of the timer period in nanoseconds.
 *  @param restart TRUE if the PIT is disabled, a new value set, and then enabled.
 *                 FALSE if the PIT will use the new value after a trigger event.
 *  @note The function will enable the timer and interrupts for the PIT.
 */
void PIT_Set(const uint32_t period, const bool restart)
{
  uint32_t Hz = NS_IN_1_SEC / period;
  uint32_t cycleCount = Clock / Hz;
  uint32_t Trigger = cycleCount - 1;

  PIT_LDVAL0 = PIT_LDVAL_TSV(Trigger);

  if (restart)
    {
      PIT_TCTRL0 &= ~PIT_TCTRL_TEN_MASK; // reset all bits in register
      PIT_TCTRL0 |= PIT_TCTRL_TEN_MASK; // Timer Enable TEN in register
    }
}

/*! @brief Enables or disables the PIT.
 *
 *  @param enable - TRUE if the PIT is to be enabled, FALSE if the PIT is to be disabled.
 */
void PIT_Enable(const bool enable)
{
  if (enable)
    {
      PIT_TCTRL0 |= PIT_TCTRL_TEN_MASK;  // Enable PIT0
    }
  else
    {
      PIT_TCTRL0 &= ~PIT_TCTRL_TEN_MASK;  // Disable PIT0
    }
}

/*! @brief Interrupt service routine for the PIT.
 *
 *  The periodic interrupt timer has timed out.
 *  The user callback function will be called.
 *  @note Assumes the PIT has been initialized.
 */
void __attribute__ ((interrupt)) PIT_ISR(void)
    {
      if (!Initialised)
	{
	  return;
	}
      (*Callback)(Arguments);
      PIT_TFLG0 = PIT_TFLG_TIF_MASK;

      Analog_Get(VOLTAGE_CHANNEL, &Meter_Voltage);
      Analog_Get(CURRENT_CHANNEL, &Meter_Current);
    }
/*  END pit  */
/*!
** @}
*/
