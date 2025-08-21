/*
 * sd_card_task.c
 *
 *  Created on: May 30, 2025
 *      Author: ashish
 */

#include "main.h"
#include "spi.h"
#include "gpio.h"
#include "sd_card_task.h"

#define SD_DUMMY_BYTE             0xFF
#define SD_START_BLOCK_TOKEN      0xFE
#define SD_CMD_LENGTH             6
#define SD_INIT_CLOCK_CYCLES      10
#define SD_BLOCK_SIZE             512
#define SD_CMD_RESPONSE_ATTEMPTS  10
#define SD_DATA_TOKEN_WAIT        100000
#define SD_CRC_DUMMY_BYTE         0xFF
#define SD_IDLE_STATE             0x01
#define SD_READY_STATE            0x00
#define SD_CMD8_CHECK_PATTERN     0x1AA
#define SD_CMD0_CRC               0x95
#define SD_CMD8_CRC               0x87
#define SD_APP_CMD_INDICATOR      0x01
#define SD_HC_CAPABILITY_FLAG     0x40000000
#define SD_DATA_ACCEPTED_TOKEN    0x05

extern SPI_HandleTypeDef hspi1;

static inline void sd_cs_select() {
	HAL_GPIO_WritePin(SD_CARD_CS_GPIO_Port, SD_CARD_CS_Pin, RESET);
}

static inline void sd_cs_deselect() {
	HAL_GPIO_WritePin(SD_CARD_CS_GPIO_Port, SD_CARD_CS_Pin, SET);
}

//Send single byte over SPI static
void SPI_SendByte(uint8_t data) {
	HAL_SPI_Transmit(&hspi1, &data, 1, HAL_MAX_DELAY);
}

// Receive single byte
static uint8_t SPI_ReceiveByte(void) {
	uint8_t data = SD_DUMMY_BYTE;
	HAL_SPI_TransmitReceive(&hspi1, &data, &data, 1, HAL_MAX_DELAY);
	return data;
}

// Send command to SD card
static uint8_t SD_SendCommand(uint8_t cmd, uint32_t arg, uint8_t crc) {
	uint8_t buf[SD_CMD_LENGTH];

	buf[0] = cmd;
	buf[1] = (arg >> 24);
	buf[2] = (arg >> 16);
	buf[3] = (arg >> 8);
	buf[4] = (arg >> 0);
	buf[5] = crc;

	sd_cs_select();
	SPI_SendByte(SD_DUMMY_BYTE);
// Delay before command
	HAL_SPI_Transmit(&hspi1, buf, SD_CMD_LENGTH, HAL_MAX_DELAY);

// Wait for response (expect < 0x80)
	for (int i = 0; i < SD_CMD_RESPONSE_ATTEMPTS; i++) {
		uint8_t response = SPI_ReceiveByte();
		if ((response & 0x80) == 0)
			return response;
	}

	return 0xFF; // Timeout
}

sdcard_status_t SD_Init(void) {
	sd_cs_deselect();
	for (int i = 0; i < SD_INIT_CLOCK_CYCLES; i++)
		SPI_SendByte(SD_DUMMY_BYTE); // 80 clocks

	uint8_t response = SD_SendCommand(CMD0, 0, SD_CMD0_CRC);
	if (response != SD_IDLE_STATE)
		return SDCARD_ERROR;

	response = SD_SendCommand(CMD8, SD_CMD8_CHECK_PATTERN, SD_CMD8_CRC);
	if (response != SD_IDLE_STATE)
		return SDCARD_ERROR;

// Wait for card to be ready
	do
	{
		SD_SendCommand(CMD55, 0, SD_APP_CMD_INDICATOR);
		response = SD_SendCommand(ACMD41, SD_HC_CAPABILITY_FLAG, SD_APP_CMD_INDICATOR);
	} while (response != SD_READY_STATE);

	response = SD_SendCommand(CMD58, 0, SD_APP_CMD_INDICATOR);
	if (response != SD_READY_STATE) {
		return SDCARD_ERROR;
	}

	sd_cs_deselect();
	SPI_SendByte(SD_DUMMY_BYTE);
	return SDCARD_OK;
}

sdcard_status_t SD_ReadBlock(uint32_t blockAddr, uint8_t *buffer) {
	if (SD_SendCommand(CMD17, blockAddr, SD_APP_CMD_INDICATOR) != SD_READY_STATE) {
		sd_cs_deselect();
		return SDCARD_ERROR;
	}

// Wait for data token (0xFE)
	for (int i = 0; i < SD_DATA_TOKEN_WAIT; i++) {
		if (SPI_ReceiveByte() == SD_START_BLOCK_TOKEN) break;
	}

// Use DMA to receive 512 bytes
	if (HAL_SPI_Receive_DMA(&hspi1, buffer, SD_BLOCK_SIZE) != HAL_OK) {
		sd_cs_deselect();
		return SDCARD_ERROR;
	}

	while (HAL_SPI_GetState(&hspi1) != HAL_SPI_STATE_READY) {
		;
	}

	SPI_ReceiveByte(); // Discard CRC SPI_ReceiveByte();

	sd_cs_deselect();
	SPI_SendByte(SD_DUMMY_BYTE);
	return SDCARD_OK;
}

sdcard_status_t SD_WriteBlock(uint32_t blockAddr, const uint8_t *buffer) {
	if (SD_SendCommand(CMD24, blockAddr, SD_APP_CMD_INDICATOR) != SD_READY_STATE) {
		sd_cs_deselect();
		return SDCARD_ERROR;
	}

	SPI_SendByte(SD_START_BLOCK_TOKEN);

// Use DMA to send 512 bytes
	if (HAL_SPI_Transmit_DMA(&hspi1, (uint8_t *)buffer, SD_BLOCK_SIZE) != HAL_OK)
	{
		sd_cs_deselect();
		return SDCARD_ERROR;
	}

	while (HAL_SPI_GetState(&hspi1) != HAL_SPI_STATE_READY) {
		;
	}

	SPI_SendByte(SD_CRC_DUMMY_BYTE); // Dummy CRC
	SPI_SendByte(SD_CRC_DUMMY_BYTE);

	uint8_t response = SPI_ReceiveByte();
	if ((response & 0x1F) != SD_DATA_ACCEPTED_TOKEN) {
		sd_cs_deselect();
		return SDCARD_ERROR;
	}

	while (SPI_ReceiveByte() == 0x00) {
		;
	}

	sd_cs_deselect();
	SPI_SendByte(SD_DUMMY_BYTE);
	return SDCARD_OK;
}
