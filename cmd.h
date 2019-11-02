/*! @file cmd.h
 *
 *  @brief command macros and functions called in main
 *
 *
 *  @author Abel Queipo 12592503
 *  @date  2 Sep 2019
 */

/*!
**  @addtogroup cmd_module CMD module documentation
**  @{
*/
#ifndef CMD_H
#define CMD_H

#include "types.h"

/*****************************************
 * Packets Transmitted from Tower to PC
 */

/*!
 * Command sent upon startup
 */
#define CMD_TX_TOWER_STARTUP 0x04

/*!
 * Command Macro which sends the result of a flash read operation
 */
#define CMD_TX_READ_BYTE 0x08

/*!
 * Command Macro which sends the tower version to the PC.
 */
#define CMD_TX_TOWER_VERSION 0x09

/*!
 * Command Macro which starts protocol mod
 */
#define CMD_TX_PROTOCOL_MODE 0x0A

/*!
 * Command Macro which sends the tower number to the PC.
 */
#define CMD_TX_TOWER_NUMBER 0x0B

/*!
 * Command Macro which sends the current time of the RTC
 */
#define CMD_TX_TIME 0x0C

/*!
 * Command Macro which sends the tower mode to the PC application
 */
#define CMD_TX_TOWER_MODE 0x0D

/*****************************************
 * Packets Transmitted from PC to Tower
 */

 /*!
  * Command Macro sent through upon startup
  */
#define CMD_RX_STARTUP_VALUES 0x04

/*!
 * Command Macro to Program a byte of flash
 */
#define CMD_RX_PROGRAM_BYTE 0x07

/*!
 * Command Macro to Read a byte of flash
 */
#define CMD_RX_READ_BYTE 0x08

/*!
 * Command Macro to get the version of the Tower software.
 */
#define CMD_RX_GET_VERSION 0x09

/*!
 * Command Macro to start protocol mode
 */
#define CMD_RX_PROTOCOL_MODE 0x0A

/*!
 * Command Macro to get or set the Student ID associated with
 * the Tower software.
 */
#define CMD_RX_TOWER_NUMBER 0x0B

/*!
 * Command Macro to set the time of the RTC
 */
#define CMD_RX_SET_TIME 0x0C

/*!
 * Command Macro to get or set the tower mode
 */
#define CMD_RX_TOWER_MODE 0x0D

/*
 * Command Macro to get analog inpu
 */
#define CMD_RX_ANALOG_INPUT 0x50

/*!
 * Packet parameter 1 to get tower number.
 */
#define CMD_TOWER_NUMBER_GET 1

/*!
 * Packet parameter 1 to set tower number.
 */
#define CMD_TOWER_NUMBER_SET 2

/*!
 * Packet parameter 1 to get tower mode.
 */
#define CMD_TOWER_MODE_GET 1

/*!
 * Packet parameter 1 to set tower mode.
 */
#define CMD_TOWER_MODE_SET 2

/*!
 * The lower 2 bytes of 12011146.
 */
#define CMD_ID 0x09C7



/*!
 * @brief Set up the tower number and mode flash allocation.
 * @note Requires the flash module to be started.
 */
bool CMD_Init();

/*!
 * @brief Send the special startup values.
 * @return bool TRUE if the operation succeeded.
 */
bool CMD_GetStartupValues();

/*!
 * @brief Programs a byte of flash at the specified offset.
 * @param offset Offset of the byte from the start of the sector.
 * @param data The byte to write.
 * @note An offset greater than 7 will erase the sector.
 * @return bool TRUE if the operation succeeded.
 */
bool CMD_FlashProgramByte(const uint8_t offset, const uint8_t data);

/*!
 * @brief check the protocol mode of data being sent.
 * @param offset Offset of the byte from the start of the sector.
 *
 * @return bool TRUE if the operation succeeded.
 */
bool CMD_ProtocolMode(const uint8_t mode, bool isSync);

/*!
 * @brief Read a byte of the flash and send it over the UART.
 * @param offset Offset of the byte from the start of the sector.
 * @note An offset past the end of the flash will fail.
 * @return bool TRUE if the operation succeeded.
 */

/*!
 * @brief Saves the tower number to a buffer.
 * @param mode Getting or setting.
 * @param lsb Least significant byte of the Tower number
 * @param msb Most significant byte of the Tower number
 * @return bool TRUE if the operation succeeded.
 */
bool CMD_TowerNumber(uint8_t mode, uint8_t lsb, uint8_t msb);

/*!
 * @brief Set the tower mode and save to flash.
 * @param mode the mode to set to.
 * @param lsb The least significant byte.
 * @param msb The most significant byte.
 * @return bool TRUE if the operation succeeded.
 */
bool CMD_TowerMode(const uint8_t mode, const uint8_t lsb, const uint8_t msb);

/*!
 * @brief Sends the startup packet to the computer.
 * @return bool TRUE if the operation succeeded.
 */
bool CMD_StartupPacket();

/*!
 * @brief Sends the tower version to the computer.
 * @return bool TRUE if the operation succeeded.
 */
bool CMD_TowerVersion();

/*!
 * @brief Sends the time to the PC.
 * @param hours The count of hours which havs occurred.
 * @param minutes The count of minutes which has occurred.
 * @param seconds The number of seconds which has occurred.
 * @return bool TRUE if the operation succeeded.
 */
bool CMD_SendTime(const uint8_t hours, const uint8_t minutes, const uint8_t seconds);

/*!
 * @brief Set the time of the RTC.
 * @param hours The count of hours which has occurred.
 * @param minutes The count of minutes which has occurred.
 * @param seconds The number of seconds which has occurred.
 * @return bool TRUE if the operation succeeded.
 */
bool CMD_SetTime(const uint8_t hours, const uint8_t minutes, const uint8_t seconds);

/*!
 * @brief check the value of the analog signal.
 * @param hours The count of hours which has occurred.
 * @param minutes The count of minutes which has occurred.
 * @param seconds The number of seconds which has occurred.
 * @return bool TRUE if the operation succeeded.
 */
bool CMD_AnalogValue(const uint8_t channelNb);

#endif /* SOURCES_CMD_H_ */
/*!
** @}
*/

