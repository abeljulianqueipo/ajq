/*! @file
 *  FIFO.c
    @brief FIFO source file to store the data to be received/transmitted using a struct

 *
 *  @date 5 Aug 2019
 *  @author Abel Queipo 1259 2503, Yann Clair 1369 8257
 *
 *  @addtogroup FIFO_module FIFO module documentation
**  @{
 */
#include "FIFO.h"
#include "PE_Types.h"
#include "CPU.h"

/* initialise the FIFO before its first run
 * input: struct TFIFO pointer, FIFO - the location of the FIFO to be initialised
*/
bool FIFO_Init(TFIFO * const fifo)
{
  fifo->Start = 0;
  fifo->End = 0;
  fifo->NbBytes = 0;
}


/* stores one charatcer into the FIFO
 * assumes FIFO_Init has been called
 * input: struct TFIFO pointer, FIFO - location of a FIFO struct, data will be stored here
 * input: integer, data - the character of information to store
 * output: boolean - true if data is successfully stored in IFFO
*/
bool FIFO_Put(TFIFO * const fifo, const uint8_t data)
{
  EnterCritical();
  if (fifo->NbBytes >= FIFO_SIZE)  // FIFO is Full
  {
      ExitCritical();
      return 0;
  }
  fifo->Buffer[fifo->End] = data;  // store data into the FIFO Buffer end index
  fifo->NbBytes++;                 // Increase Number of bytes in FIFO
  fifo->End++;                     // Reset the END byte of the FIFO
  if (fifo->End >= FIFO_SIZE)      // If END index is outside of specified FIFO size
  {
      fifo->End = 0;               // Reset END index of FIFO to 0
  }
  ExitCritical();
  return 1;
}


/* get one character out of the FIFO
 * assumes FIFO_Init has been called
 * input: struct TFIFO pointer, FIFO - location of FIFO that holds data to be retrieved
 * input: integer pointer, dataPtr - memory location that will hold the retreived data
 * output: boolean - true if data is successfully retrieved from the FIFO
 */
bool FIFO_Get(TFIFO * const fifo, uint8_t * const dataPtr)
{
  EnterCritical();
  if (fifo->NbBytes == 0)               // FIFO is Empty
  {
      ExitCritical();
      return 0;
  }
  *dataPtr = fifo->Buffer[fifo->Start]; // Point to the oldest data stored in the FIFO buffer (the START)
  fifo->Start++;                        // Move on to next byte to retrieve
  fifo->NbBytes--;                      // Decrease number of bytes in FIFO
  if (fifo->Start >= FIFO_SIZE)         // if FIFO is OUTSIDE of specified FIFO size
  {
      fifo->Start = 0;                  // Reset starting index of FIFO to 0
  }
  ExitCritical();
  return 1;
}

/* END FIFO */
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
