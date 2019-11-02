/*! @file
 *  packet.c
 *  @brief Packet Source File which configures the packets of the PC and Tower to establish proper communication
 *
 *  @date 5 Aug 2019
 *  @author Abel Queipo 1259 2503, Yann Clair 1369 8257
 *
 *  @addtogroup Packet_module Packet module documentation
**  @{
 */

#include "packet.h"
#include "types.h"
#include "UART.h"

static uint8_t Position = 0;

//static uint8_t Checksum;

//uint8_t Packet_Command,
//	Packet_Parameter1,
//	Packet_Parameter2,
//	Packet_Parameter3,
//	Checksum;

/* calculates theoretical XOR checksum to confirm if received checksum matches
 * assumes Packet_Init has been called
 * output: unsigned int(8-bit)
 */

TPacket Packet;

uint8_t PacketTest(void)
{
  uint8_t calc_checksum = Packet_Command ^ Packet_Parameter1 ^ Packet_Parameter2 ^ Packet_Parameter3; // XOR packet to calculate the checksum
  uint8_t ret_val = calc_checksum == Packet_Checksum;

}


/* initialises the packets by calling required initialisation routine(s) (UART_INIT)
 * input: integer, baudRate - the required baud rate
 * input: integer, moduleClk - the specified module clock frequency in Hz
 * output: boolean - true if the module was successfully initialised
 */
bool Packet_Init(const uint32_t baudRate, const uint32_t moduleClk)
{
  UART_Init(baudRate, moduleClk);  // Initialising the Baud Rate
}

/* attempts to receive a full packet from the FIFO
 * assumes Packet_Init has been called
 * output: boolean - true if a full packet has been received, in phase
 */
bool Packet_Get(void)
{
  uint8_t uartData;  // initialising a temporary 8-bit integer for the UART data
  if (!UART_InChar(&uartData))  // Checking if there is any data in the FIFO, if not return 0
  {
      return 0;
  }
	switch (Position)
	{
	case 0:	// Command Byte of Packet
		Packet_Command = uartData;
		Position++;
		return 0;
	case 1:	// Parameter 1 byte of packet
		Packet_Parameter1 = uartData;
		Position++;
		return 0;
	case 2:	// Parameter 2 byte of packet
		Packet_Parameter2 = uartData;
		Position++;
		return 0;
	case 3:	// Parameter 3 byte of packet
		Packet_Parameter3 = uartData;
		Position++;
		return 0;
	case 4:	// Checksum byte of packet
		Packet_Checksum = uartData;
		if (PacketTest())  // XORing the Checksum
		{
			Position = 0;  // if checksum XOR is successful, reset the packet position to 0
			return 1;
		}
                // If XORing is unsuccessful, moves each byte across for re-alignment
		Packet_Command = Packet_Parameter1;
		Packet_Parameter1 = Packet_Parameter2;
		Packet_Parameter2 = Packet_Parameter3;
		Packet_Parameter3 = Packet_Checksum;
		return 0;
	default:
                //reset the counter
		Position = 0;
		return 0;
	}
  return 0;
}


/* sends a packet to the transmit FIFO
 * assumes Packet_Init has been called
 * assumes packet is in phase
 * input: integer, command
 * input: integer, parameter1
 * input: integer, parameter2
 * input: integer, parameter3
 * output: boolean - true if valid packet was sent. always true, this function is only called for valid packets
 */
bool Packet_Put(const uint8_t command, const uint8_t parameter1, const uint8_t parameter2, const uint8_t parameter3)
{
	//enter critical
	if (!UART_OutChar(command)) // if command byte is not valid, failed to put command byte successfully
	{
		return 0;           // return 0 - failed to put packet
	}
	if (!UART_OutChar(parameter1)) // if parameter1 byte is not valid, failed to put p1 byte successfully
	{
		return 0;              // return 0 - failed to put packet
	}
	if (!UART_OutChar(parameter2)) // if parameter2 byte is not valid, failed to put p2 byte successfully
	{
		return 0;              // return 0 - failed to put packet
	}
	if (!UART_OutChar(parameter3)) // if parameter3 byte is not valid, failed to put p3 byte successfully
	{
		return 0;              // return 0 - failed to put packet
	}
	if (!UART_OutChar(command ^ parameter1 ^ parameter2 ^ parameter3)) // re-evaluating the checksum (logical XOR)
	{
		return 0;              // return 0 - failed to put packet
	}
	return 1; // packet is valid, return 1 - success
}


/* END packet */
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
