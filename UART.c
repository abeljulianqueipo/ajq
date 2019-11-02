/*! @file
 * UART.c
 *  @brief initialises UART communication
 *  UART.c Source File initialises the baud rate and module clock for serial communication to the MCU.
 *  It also holds the functions which transmit/receive data from/to the UART. The file also has a polling function which will poll the
 *  UART status register to receive and/or transmit one character.
 *
 *  @date 16 Sept 2019
 *  @author Abel Queipo 1259 2503, Yann Clair 1369 8257
 *
 *  @addtogroup UART_module UART module documentation
**  @{
 */
#include "UART.h"
#include "FIFO.h"
#include "types.h"
#include "MK70F12.h"

static TFIFO RxFIFO, TxFIFO; //local variables for the two FIFO's one for receive and one for transmit.

/*! initialise UART by setting desired ports on
 *  input: integer, baudRate - desired baud rate
 *  input: integer, moduleClk - required module clock frequency in Hz
 *  output: boolean - true if correctly initialised.
 */
bool UART_Init(const uint32_t baudRate, const uint32_t moduleClk)
{
  //UART setup
  //uint16_t divisor = moduleClk/(16*baudRate); // gives us our required divisor; we use 12 bits for this divisor so 16 bit integer
  					    // our divisor is 34.1333.. the decimal is ignored because computer we can add that in manually using fine adjust.


  SIM_SCGC4 |= SIM_SCGC4_UART2_MASK; // UART2 Clock gate control

  SIM_SCGC5 |= SIM_SCGC5_PORTE_MASK; // Enable 	clock gate control for PORTE.

  PORTE_PCR16 |= PORT_PCR_MUX(3);    // Set pin control register 16 PORTE bits 8 & 9, to enable the multiplexer alternative 3 (Transmitter).

  PORTE_PCR17 |= PORT_PCR_MUX(3);    // Set pin control register 17 PORTE bits 8 & 9, to enable the multiplexer alternative 3 (Receiver).




  UART2_C2 |= UART_C2_TCIE_MASK;  //Transmission Complete Interrupt Enable
  UART2_C2 |= UART_C2_RIE_MASK;  // Receiver Full Interrupt or DMA Transfer Enable

  UART2_C2 |= UART_C2_RE_MASK;  // Enable UART2 receive.
  UART2_C2 |= UART_C2_TE_MASK;  // Enable UART2 transmit.


  //Set the requested baud rate

  //UART2_BDH = (divisor & 0x1F00) >> 8;    // ANDing our divisor with this hex number will give us our most significant byte, what we want for BDH, ignoring 3 most significant bits.
  				          // we also shift over by 8 bits as divisor is 16 bits long but BDH is only 8
  //UART2_BDL = (divisor & 0x00FF); 	  // ANDing our divisor with this hex number will ignore the first byte and only look at the second byte, which is what we want for BDL.

  //UART2_C4 &= ~UART_C4_BRFA_MASK;	  //Clear all the bits in the BRFA mask in control register 4 - resetting the control register in-case of unwanted data stored
  //UART2_C4 |= (0x04 & UART_C4_BRFA_MASK); // turns on bit 2 in the BRFA mask, which is 0100(base 2) the value we wanted for our BRFA (4/32)

  // Setting requested Baud Rate
  uint16union_t setting;
  setting.l = (uint16_t)(moduleClk/(baudRate * 16));
  UART2_BDH |= (uint8_t)(setting.s.Hi & 0x1F);
  UART2_BDL = (uint8_t)(setting.s.Lo);

  uint8_t fineAdjust = (uint8_t)((moduleClk * 2) / baudRate) % 32;
  UART2_C4 = (fineAdjust & 0x1F);

  //Initialize the FIFO buffers
  FIFO_Init(&RxFIFO);
  FIFO_Init(&TxFIFO);

  // Setting up NVIC for PIT see K70 manual pg 97, 99
  // Vector=65, IRQ=49
  // NVIC non-IPR=1 IPR=12
  // Clear any pending interrupts on PIT
  NVICICPR1 = NVIC_ICPR_CLRPEND(1 << 17);
  // Enable Interrupts for PIT0
  NVICISER1 = NVIC_ISER_SETENA(1 << 17);

  return 1;
}


/* get one character from the receive FIFO but only if it is not empty
 * assumes UART_Init has been called
 * input: integer, dataPtr - a memory location to store the retreived character
 * output: boolean - true if the receive FIFO returned a character
 */
bool UART_InChar(uint8_t * const dataPtr)
{
  return FIFO_Get(&RxFIFO, dataPtr);  // returns 0 if FIFO is empty, else points to the FIFO to receive data
}

/* put one character in the transmit FIFO if it isnt full yet
 * assumes UART_Init has been called
 * input: integer, data - the character to be transmitted
 * output: boolean - true if the data was successfully placed in the transmit FIFO
 */
bool UART_OutChar(const uint8_t data)
{
  if (FIFO_Put(&TxFIFO, data))
    {
      UART2_C2 |= UART_C2_TIE_MASK;
      return 1;
    }
  return 0;  // returns 0 if FIFO is full, else transmits data

}

/*! @brief Interrupt service routine for the UART.
 *
 *  @note Assumes the transmit and receive FIFOs have been initialized.
 */
void __attribute__ ((interrupt)) UART_ISR(void)
    {
      if (UART2_S1 & UART_S1_TDRE_MASK)
	{
	  if (FIFO_Get(&TxFIFO, &UART2_D) == 0)
	    {
	      UART2_C2 &= ~UART_C2_TIE_MASK;
	    }
	}
      if (UART2_S1 & UART_S1_RDRF_MASK)
	{
	  FIFO_Put(&RxFIFO, UART2_D);
	}
    }

/* polls the UART status register to attempt send/receive one character
 * assumes UART_Init has been called
 */
//void UART_Poll(void)

//{
//  if (UART2_S1 & UART_S1_TDRE_MASK) // if the status register and the TDRE flag is enabled ie new character in transmit FIFO
//    {
//	FIFO_Get(&TxFIFO, (uint8_t *)&UART2_D); // run FIFO_Get; pass the location of the transmitting FIFO and the UART2 Data register
//    }
//    if (UART2_S1 & UART_S1_RDRF_MASK) //also if the RDRF flag is enabled ie new character in receive FIFO
//    {
//	FIFO_Put(&RxFIFO, UART2_D); //run FIFO_Put; passing the location of the recieving FIFO and the UART2 Data register
//    }
//}
/* END UART */
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

