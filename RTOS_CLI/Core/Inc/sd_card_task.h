/*
 * sd_card_task.h
 *
 *  Created on: May 30, 2025
 *      Author: ashish
 */

#ifndef INC_SD_CARD_TASK_H_
#define INC_SD_CARD_TASK_H_

#define SD_SECTOR_SIZE	(512)

// SD Card SPI Mode Commands
typedef enum {
    // Basic commands
    CMD0  = 0x40, // GO_IDLE_STATE: Reset the card. Must be sent first after power-on. CRC must be 0x95.
    CMD1  = 0x41, // SEND_OP_COND: Initialize MMC cards (not used for SD cards anymore).
    CMD8  = 0x48, // SEND_IF_COND: Check SD version and voltage range (send 0x1AA as argument). CRC must be 0x87.
    CMD9  = 0x49, // SEND_CSD: Read the Card-Specific Data (CSD) register.
    CMD10 = 0x4A, // SEND_CID: Read the Card Identification (CID) register.
    CMD12 = 0x4C, // STOP_TRANSMISSION: Stop a multiple block read/write operation.
    CMD13 = 0x4D, // SEND_STATUS: Ask the card for its current status.

    // Data transfer block length
    CMD16 = 0x50, // SET_BLOCKLEN: Set the block size (must be 512 for SDHC/SDXC; can be ignored).

    // Read operations
    CMD17 = 0x51, // READ_SINGLE_BLOCK: Read one 512-byte block from the given address.
    CMD18 = 0x52, // READ_MULTIPLE_BLOCK: Read multiple blocks starting from a given address.

    // Write operations
    CMD24 = 0x58, // WRITE_BLOCK: Write a single 512-byte block.
    CMD25 = 0x59, // WRITE_MULTIPLE_BLOCK: Write multiple blocks in sequence.

    // App command wrapper
    CMD55 = 0x77, // APP_CMD: Signals the next command is an application-specific command (ACMD).

    // Card configuration
    CMD58 = 0x7A, // READ_OCR: Read the OCR (Operating Conditions Register) to get voltage and card type.

    // Application-specific commands (must be preceded by CMD55)
    ACMD41 = 0x69, // SD_SEND_OP_COND: Initialize SD card and exit idle state. (Used instead of CMD1)
    ACMD22 = 0x56, // SEND_NUM_WR_BLOCKS: Get number of written write blocks.
    ACMD23 = 0x57  // SET_WR_BLK_ERASE_COUNT: Define number of pre-erased blocks before write.
} SD_Command;

typedef enum
{
	SDCARD_OK,
	SDCARD_ERROR,
	SDCARD_TIMEOUT,
	SDCARD_BUSY,
	SDCARD_NO_RESPONSE
}sdcard_status_t;

sdcard_status_t SD_Init(void);
void print_sd_card_info();
sdcard_status_t SD_WriteBlock(uint32_t blockAddr, const uint8_t *buffer);
sdcard_status_t SD_ReadBlock(uint32_t blockAddr, uint8_t *buffer) ;


#endif /* INC_SD_CARD_TASK_H_ */
