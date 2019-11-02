/*
 * cmd.c
 *
 *  Created on: 2 Sep 2019
 *      Author: 12592503
 */
/*!
**  @addtogroup cmd_module CMD module documentation
**  @{
*/
#include "cmd.h"

#include "Flash.h"
#include "packet.h"
#include "RTC.h"
#include "types.h"
#include "analog.h"
//#include "SPI.h"

/*
 * Tower software version V1.0
 */
const uint8_t TOWER_VERSION_H = 1;
const uint8_t TOWER_VERISON_L = 0;

static uint16union_t volatile *TowerNumber;
static uint16union_t volatile *TowerMode;



// holds TAnalogInput for each channel in an array
TAnalogInput Analog_Input[ANALOG_NB_INPUTS];

/*!
 * @brief Set up the tower number and mode flash allocation.
 * @note Requires the flash module to be started.
 */
bool CMD_Init()
{
  bool allocNumber = Flash_AllocateVar((volatile void **) &TowerNumber, sizeof(uint16union_t));
  bool allocMode = Flash_AllocateVar((volatile void **) &TowerMode, sizeof(uint16union_t));
  if (allocNumber == 1 && allocMode == 1)
    {
      if (TowerNumber->l == 0xFFFF)
	{
	  Flash_Write16((uint16_t volatile *) TowerNumber, CMD_ID);
	}
      if (TowerMode->l == 0xFFFF)
	{
	  Flash_Write16((uint16_t volatile *) TowerMode, 0x01);
	}
      return 1;
    }
  return 0;
}

/*!
 * @brief Send the special startup values.
 * @return bool TRUE if the operation succeeded.
 */
bool CMD_GetStartupValues()
{
  if (!Packet_Put(CMD_TX_TOWER_STARTUP, 0x0, 0x0, 0x0))
    {
      return 0;
    }
  if (!CMD_TowerVersion())
    {
      return 0;
    }
  if (!Packet_Put(CMD_TX_TOWER_NUMBER, 1, TowerNumber->s.Lo, TowerNumber->s.Hi))
    {
      return 0;
    }
  if (!Packet_Put(CMD_TX_TOWER_MODE, 0x1, TowerMode->s.Lo, TowerMode->s.Hi))
    {
      return 0;
    }
  if (!Packet_Put(CMD_TX_PROTOCOL_MODE, 0x01, 0, 0))
    {
      return 0;
    }
  return 1;
}

/*!
 * @brief Programs a byte of flash at the specified offset.
 * @param offset Offset of the byte from the start of the sector.
 * @param data The byte to write.
 * @note An offset greater than 7 will erase the sector.
 * @return bool TRUE if the operation succeeded.
 */
bool CMD_FlashProgramByte(const uint8_t offset, const uint8_t data)
{
  if (offset > 8)// Change 8 to FLASH_DATA_SIZE for dynamicness
    {
      return 0;
    }
  if (offset == 8)// Change 8 to FLASH_DATA_SIZE for dynamicness
    {
      return Flash_Erase();
    }
  uint8_t *address = (uint8_t *)(FLASH_DATA_START + offset);
  return Flash_Write8(address, data);
}

/*!
 * @brief Read a byte of the flash and send it over the UART.
 * @param offset Offset of the byte from the start of the sector.
 * @note An offset past the end of the flash will fail.
 * @return bool TRUE if the operation succeeded.
 */
bool CMD_FlashReadByte(const uint8_t offset)
{
  if (offset > (FLASH_DATA_SIZE - 1))
  {
  	return 0;
  }
  uint8_t data = _FB(FLASH_DATA_START + offset);
  return Packet_Put(CMD_TX_READ_BYTE, offset, 0x0, data);
}

/*!
 * @brief check the protocol mode of data being sent.
 * @param offset Offset of the byte from the start of the sector.
 *
 * @return bool TRUE if the operation succeeded.
 */
bool CMD_ProtocolMode(const uint8_t mode, bool isSync)
{
  while (isSync)
    return Packet_Put(CMD_TX_TOWER_MODE, mode, isSync, 0x0);
}

/*!
 * @brief Saves the tower number to a buffer.
 * @param mode Getting or setting.
 * @param lsb Least significant byte of the Tower number
 * @param msb Most significant byte of the Tower number
 * @return bool TRUE if the operation succeeded.
 */
bool CMD_TowerNumber(uint8_t mode, uint8_t lsb, uint8_t msb)
{
  if (mode == CMD_TOWER_NUMBER_GET)
    {
      return Packet_Put(CMD_TX_TOWER_NUMBER, 1, TowerNumber->s.Lo, TowerNumber->s.Hi);
    }
  else if (mode == CMD_TOWER_NUMBER_SET)
    {
      uint16union_t temp;
      temp.s.Hi = msb;
      temp.s.Lo = lsb;
      return Flash_Write16((uint16_t volatile *) TowerNumber, temp.l);
    }
  return 0;
}

/*!
 * @brief Set the tower mode and save to flash.
 * @param mode the mode to set to.
 * @param lsb The least significant byte.
 * @param msb The most significant byte.
 * @return bool TRUE if the operation succeeded.
 */
bool CMD_TowerMode(const uint8_t mode, const uint8_t lsb, const uint8_t msb)
{
  if (mode == CMD_TOWER_MODE_GET)
    {
      return Packet_Put(CMD_TX_TOWER_MODE, 0x1, TowerMode->s.Lo, TowerMode->s.Hi);
    }
  else if (mode == CMD_TOWER_NUMBER_SET)
    {
      uint16union_t temp;
      temp.s.Hi = msb;
      temp.s.Lo = lsb;
      return Flash_Write16((uint16_t volatile *) TowerMode, temp.l);
    }
  return 0;
}

/*!
 * @brief Sends the startup packet to the computer.
 * @return bool TRUE if the operation succeeded.
 */
bool CMD_StartupPacket()
{
  return Packet_Put(CMD_TX_TOWER_STARTUP, 0x0, 0x0, 0x0);
}

/*!
 * @brief Sends the tower version to the computer.
 * @return bool TRUE if the operation succeeded.
 */
bool CMD_TowerVersion()
{
  return Packet_Put(CMD_TX_TOWER_VERSION, 'v', TOWER_VERSION_H, TOWER_VERISON_L);
}

/*!
 * @brief Sends the time to the PC.
 * @param hours The count of hours which havs occurred.
 * @param minutes The count of minutes which has occurred.
 * @param seconds The number of seconds which has occurred.
 * @return bool TRUE if the operation succeeded.
 */
bool CMD_SendTime(const uint8_t hours, const uint8_t minutes, const uint8_t seconds)
{
  return Packet_Put(CMD_TX_TIME, hours, minutes, seconds);
}

/*!
 * @brief Set the time of the RTC.
 * @param hours The count of hours which has occurred.
 * @param minutes The count of minutes which has occurred.
 * @param seconds The number of seconds which has occurred.
 * @return bool TRUE if the operation succeeded.
 */
bool CMD_SetTime(const uint8_t hours, const uint8_t minutes, const uint8_t seconds)
{
  if (hours > 23 || minutes > 59 || seconds > 59)
    {
      return 0;
    }
  RTC_Set(hours, minutes, seconds);
  return 1;
}

/*!
 * @brief check the value of the analog signal.
 * @param hours The count of hours which has occurred.
 * @param minutes The count of minutes which has occurred.
 * @param seconds The number of seconds which has occurred.
 * @return bool TRUE if the operation succeeded.
 */
bool CMD_AnalogValue(const uint8_t channelNb)
{
  if (channelNb > 1 | channelNb < 0)
    {
      return 1;
    }
  return Packet_Put(CMD_RX_ANALOG_INPUT, channelNb, Analog_Input[channelNb].value.s.Lo, Analog_Input[channelNb].value.s.Hi);
}

/*  END OF COMMAND MODULE */
/*!
** @}
*/


