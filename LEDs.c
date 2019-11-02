/*! @file
 *  LEDs.c
 *
 *  @brief Initialising LED pins as outputs
 *
 *  @author Abel Queipo 12592503, Yann Clair 13698257
 *  @date 19 Aug 2019
 *
 *  @addtogroup LEDs_module LEDs module documentation
**  @{
 */

// new types
#include "types.h"
#include "LEDs.h"
#include "MK70F12.h"



/*! @brief Sets up the LEDs before first use.
 *
 *  @return bool - TRUE if the LEDs were successfully initialized.
 */
bool LEDs_Init(void)
{
  // Initialise Port A pins as GPIOs
  PORTA_PCR10 |= PORT_PCR_MUX(1);
  PORTA_PCR11 |= PORT_PCR_MUX(1);
  PORTA_PCR28 |= PORT_PCR_MUX(1);
  PORTA_PCR29 |= PORT_PCR_MUX(1);

  // Turn off all LEDs
   GPIOA_PSOR |= LED_ORANGE;
   GPIOA_PSOR |= LED_YELLOW;
   GPIOA_PSOR |= LED_GREEN;
   GPIOA_PSOR |= LED_BLUE;

  // Set GPIOs as outputs
  GPIOA_PDDR |= LED_ORANGE;  //sets led line as an output
  GPIOA_PDDR |= LED_YELLOW;
  GPIOA_PDDR |= LED_GREEN;
  GPIOA_PDDR |= LED_BLUE;

  // Turn port A on
  SIM_SCGC5 |= SIM_SCGC5_PORTA_MASK;



  return 1;
}

/*! @brief Turns an LED on.
 *
 *  @param color The color of the LED to turn on.
 *  @note Assumes that LEDs_Init has been called.
 */
void LEDs_On(const TLED color)
{
  GPIOA_PCOR |= color;
}

/*! @brief Turns off an LED.
 *
 *  @param color THe color of the LED to turn off.
 *  @note Assumes that LEDs_Init has been called.
 */
void LEDs_Off(const TLED color)
{
  GPIOA_PSOR |= color;
}

/*! @brief Toggles an LED.
 *
 *  @param color THe color of the LED to toggle.
 *  @note Assumes that LEDs_Init has been called.
 */
void LEDs_Toggle(const TLED color)
{
  GPIOA_PTOR |= color;
}

/* END LEDs */
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
