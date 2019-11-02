/*! @file
 *  Flash.c
 *
 *  @brief Routines for erasing and writing to flash
 *
 *  @author  Abel Queipo 12592503, Yann Clair 13698257
 *  @date 25 Aug 2019
 *
 *  @addtogroup Flash_module Flash module documentation
**  @{
 */

#include "types.h"
#include "Flash.h"
#include "MK70F12.h"

#include <string.h>

#define SECTOR_SIZE 4095

#define FLASH_EMPTY 0
#define FLASH_ALLOCATED 1


//0 is unallocated, 1 is allocated
static uint8_t AllocationMap[FLASH_DATA_SIZE] = { FLASH_EMPTY };

/*
 * Flash Commands
 */

//Read 1s Section
#define FLASH_CMD_RD1SEC 0x01LU

//Erase Sector
#define FLASH_CMD_ERSSCR 0x09LU

#define FLASH_CMD_PGM8 0x07LU

/* @brief Wait for the CCIF register to be set to 1.
 *
 */
void WaitCCIF()
{
	while (!(FTFE_FSTAT & FTFE_FSTAT_CCIF_MASK))
		;
}

/*! @brief Set CCIF and the wait for it to be set.
 * Used to start a flash command and wait for it to complete
 *
 */
void SetCCIF()
{
	FTFE_FSTAT = FTFE_FSTAT_CCIF_MASK;
	WaitCCIF();
}

bool MGSTAT0Error()
{
	return FTFE_FSTAT & FTFE_FSTAT_MGSTAT0_MASK;
}

/*!
 * @brief Handle the error registers and resets them
 * @return TRUE if success
 */
bool HandleRegisters()
{
  uint8_t fstat_buffer = FTFE_FSTAT;
  //Command Complete Interrupt Flag. 0 = in progress, 1 = idle
  bool CCIFset = ((fstat_buffer & FTFE_FSTAT_CCIF_MASK) == FTFE_FSTAT_CCIF_MASK);

  //FTFE Read Collision Error Flag. 1 = collision detected
  bool RDCOLERRset = ((fstat_buffer & FTFE_FSTAT_RDCOLERR_MASK)
			== FTFE_FSTAT_RDCOLERR_MASK);
  //Flash Access Error Flag. 1 = error
  bool ACCERRset = ((fstat_buffer & FTFE_FSTAT_ACCERR_MASK)
      == FTFE_FSTAT_ACCERR_MASK);

  //Flash Protection Violation Flag. 1 = violated
  bool FPVIOLset = ((fstat_buffer & FTFE_FSTAT_FPVIOL_MASK)
      == FTFE_FSTAT_FPVIOL_MASK);

  //Memory Controller Command Completion Status Flag
  bool MGSTAT0set = ((fstat_buffer & FTFE_FSTAT_MGSTAT0_MASK)
			== FTFE_FSTAT_MGSTAT0_MASK);

  return 1;
}

/*! @brief Initializes the flash modules.
 *
 *  @return BOOL - TRUE if the Flash was setup successfully.
 */
bool Flash_Init()
{
  // Connect the busses and clocks
  SIM_SCGC3 = SIM_SCGC3_NFC_MASK;

  //Wait for the flash module to start up
  WaitCCIF();
  return 1;
}

/*! @brief Makes space for a non-volatile variable in the Flash memory.
 *
 *  @param variable is the address of a pointer to a variable that is to be allocated space in Flash memory.
 *  @param size The size, in bytes, of the variable that is to be allocated space in the Flash memory. Valid values are 1, 2 and 4.
 *  @return BOOL - TRUE if the variable was allocated space in the Flash memory.
 *  @note Assumes Flash has been initialized.
 */
bool Flash_AllocateVar(volatile void ** variable, const uint8_t size)
{
  //The number of sequential unallocated bytes.
  size_t sequential = 0;

  //Location of the allocation in the array.
  size_t location = 0;
  //loop over allocation map.
  for (size_t i = 0; i < FLASH_DATA_SIZE; i++)
    {
      if (AllocationMap[i] == 0)
	{
	  sequential++;
	}
      else
	{
	  sequential = 0;
	}
      //Assign *variable to an aligned memory address
      if (i % size == 0)
	{
	  location = i;
	}
      /* If there are the correct number of sequential bytes free
       * 	 * and the position is aligned
      */
      if (sequential >= size && ((i + 1) % size == 0))
	{
	  //Iterate over allocation map and mark the bytes as allocated
	  for (size_t j = location; j <= i; j++)
	    {
	      AllocationMap[j] = FLASH_ALLOCATED;
	    }
	  *variable = (void*) (FLASH_DATA_START + location);
	  return 1;
	}
    }
  return 0;
}

/*! @brief Erases flash and then writes phrase
 *	@return TRUE if success
 */
bool WritePhrase(const uint64_t phrase)
{
  WaitCCIF();
  bool result = Flash_Erase();
  if (result != 1)
    {
      return 0;
    }

  uint32_8union_t flashStart;
  flashStart.l = FLASH_DATA_START;

  FTFE_FCCOB0 = FLASH_CMD_PGM8; // defines the FTFE command to write
  FTFE_FCCOB1 = flashStart.s.b; // sets flash address[23:16] to 128
  FTFE_FCCOB2 = flashStart.s.c; // sets flash address[15:8] to 0
  FTFE_FCCOB3 = (flashStart.s.d & 0xF8);

  uint8_t *data = (uint8_t *) &phrase;

  //Order of these is reversed and switched so that the allocation mapping works
  FTFE_FCCOB4 = data[3];
  FTFE_FCCOB5 = data[2];
  FTFE_FCCOB6 = data[1];
  FTFE_FCCOB7 = data[0];
  FTFE_FCCOB8 = data[7];
  FTFE_FCCOB9 = data[6];
  FTFE_FCCOBA = data[5];
  FTFE_FCCOBB = data[4];

  SetCCIF();

  return HandleRegisters();
}

void ReadPhrase(uint64_t * const phrase)
{
  WaitCCIF();
  *phrase = _FP(FLASH_DATA_START);
}

/*! @brief erases a sector
 *  @param address the start address we are erasing from
 *  @return BOOL - TRUE if successful
 */
static bool EraseSector(const uint32_t address)
{ // reset our phrase to all 1s. this kills the phrase.
  uint32_8union_t flashStart;
  flashStart.l = FLASH_DATA_START;

  FTFE_FCCOB0 = FLASH_CMD_ERSSCR; // defines the FTFE command to erase
  FTFE_FCCOB1 = flashStart.s.b; // sets flash address[23:16] to 128
  FTFE_FCCOB2 = flashStart.s.c; // sets flash address[15:8] to 0
  FTFE_FCCOB3 = (flashStart.s.d & 0xF0); // sets flash address[7:0] to 0

  SetCCIF();
  return 1; // also control will return here once the CCIF flag says it's done

}

/*! @brief Puts a 32-bit integer to Flash.
 *
 *  @param address The address of the data.
 *  @param data The 32-bit data to write.
 *  @return BOOL - TRUE if Flash was written successfully, FALSE if address is not aligned to a 4-byte boundary or if there is a programming error.
 *  @note Assumes Flash has been initialized.
 */
bool Flash_Write32(uint32_t volatile * const address, const uint32_t data)
{
  size_t index = (size_t) address - FLASH_DATA_START;
  if (index >= FLASH_DATA_SIZE || index < 0)
    {
      //Out of range.
      return 0;
    }
  if (index % 4 != 0)
    {
      //not aligned
      return 0;
    }
  index /= 4;
  uint64_t tempPhrase;
  ReadPhrase(&tempPhrase);
  uint32_t *psuedoArray = (uint32_t *) &tempPhrase;
  psuedoArray[index] = data;
  WritePhrase(tempPhrase);
  return 1;
}

/*! @brief Puts a 16-bit integer to Flash.
 *
 *  @param address The address of the data.
 *  @param data The 16-bit data to write.
 *  @return BOOL - TRUE if Flash was written successfully, FALSE if address is not aligned to a 2-byte boundary or if there is a programming error.
 *  @note Assumes Flash has been initialized.
 */
bool Flash_Write16(uint16_t volatile * const address, const uint16_t data)
{
  size_t index = (size_t) address - FLASH_DATA_START;
  if (index >= FLASH_DATA_SIZE || index < 0)
    {
      //Out of range.
      return 0;
    }
  if (index % 2 != 0)
    {
      //not aligned
      return 0;
    }
  index /= 2;
  uint64_t tempPhrase;
  ReadPhrase(&tempPhrase);
  uint16_t *psuedoArray = (uint16_t *) &tempPhrase;
  psuedoArray[index] = data;
  WritePhrase(tempPhrase);
  return 1;
}

/*! @brief Puts an 8-bit integer to Flash.
 *
 *  @param address The address of the data.
 *  @param data The 8-bit data to write.
 *  @return BOOL - TRUE if Flash was written successfully, FALSE if there is a programming error.
 *  @note Assumes Flash has been initialized.
 */
bool Flash_Write8(uint8_t volatile * const address, const uint8_t data)
{
  size_t index = (size_t) address - FLASH_DATA_START;
  if (index >= FLASH_DATA_SIZE || index < 0)
    {
      //Out of range.
      return 0;
    }
  uint64_t tempPhrase;
  ReadPhrase(&tempPhrase);
  uint8_t *psuedoArray = (uint8_t *) &tempPhrase;
  psuedoArray[index] = data;
  WritePhrase(tempPhrase);
  return 1;
}

/*! @brief Erases the entire Flash sector.
 *
 *  @return BOOL - TRUE if the Flash "data" sector was erased successfully.
 *  @note Assumes Flash has been initialized.
 */
bool Flash_Erase(void)
{
  WaitCCIF();
  EraseSector(FLASH_DATA_START);
  //SetCCIF();
  //todo: set only once
// Only do this if you want the allocation to clear too.
//	memset(allocationMap, 0, FLASH_DATA_SIZE);
  return HandleRegisters();
}

/* END flash */
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

