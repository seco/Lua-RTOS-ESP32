#ifndef LUA_RTOS_LUARTOS_H_
#define LUA_RTOS_LUARTOS_H_

#include "sdkconfig.h"

/*
 *
 * UART
 *
 */

// Use console?
#ifdef CONFIG_USE_CONSOLE
#define USE_CONSOLE CONFIG_USE_CONSOLE
#else
#define USE_CONSOLE 1
#endif

// Get the UART assigned to the console
#if CONFIG_LUA_RTOS_CONSOLE_UART0
#define CONSOLE_UART 1
#endif

#if CONFIG_LUA_RTOS_CONSOLE_UART1
#define CONSOLE_UART 2
#endif

#if CONFIG_LUA_RTOS_CONSOLE_UART2
#define CONSOLE_UART 3
#endif

// Get the console baud rate
#if CONFIG_LUA_RTOS_CONSOLE_BR_57600
#define CONSOLE_BR 57600
#endif

#if CONFIG_LUA_RTOS_CONSOLE_BR_115200
#define CONSOLE_BR 115200
#endif

// Get the console buffer length
#ifdef CONFIG_LUA_RTOS_CONSOLE_BUFFER_LEN
#define CONSOLE_BUFFER_LEN CONFIG_LUA_RTOS_CONSOLE_BUFFER_LEN
#else
#define CONSOLE_BUFFER_LEN 1024
#endif

#ifndef CONSOLE_UART
#define CONSOLE_UART 1
#endif

#ifndef CONSOLE_BR
#define CONSOLE_BR 115200
#endif

// SPIFFS?
#define SPIFFS_ERASE_SIZE 4096
#define SPIFFS_LOG_PAGE_SIZE 256

#if CONFIG_LUA_RTOS_USE_SPIFFS
#define USE_SPIFFS CONFIG_LUA_RTOS_USE_SPIFFS
#else
#define USE_SPIFFS 0
#endif

#ifdef CONFIG_LUA_RTOS_SPIFFS_LOG_BLOCK_SIZE
#define SPIFFS_LOG_BLOCK_SIZE CONFIG_LUA_RTOS_SPIFFS_LOG_BLOCK_SIZE
#else
#define SPIFFS_LOG_BLOCK_SIZE 8192
#endif

#ifdef CONFIG_LUA_RTOS_SPIFFS_BASE_ADDR
#define SPIFFS_BASE_ADDR CONFIG_LUA_RTOS_SPIFFS_BASE_ADDR
#else
#define SPIFFS_BASE_ADDR 180000
#endif

#ifdef CONFIG_LUA_RTOS_SPIFFS_SIZE
#define SPIFFS_SIZE CONFIG_LUA_RTOS_SPIFFS_SIZE
#else
#define SPIFFS_SIZE 524288
#endif

// LoRa WAN
#if CONFIG_LUA_RTOS_USE_LMIC
#define USE_LMIC 1
#else
#define USE_LMIC 0
#endif

#if CONFIG_LUA_RTOS_LORAWAN_RADIO_SX1276
	#define CFG_sx1276_radio 1
#else
	#if CONFIG_LUA_RTOS_LORAWAN_RADIO_SX1272
		#define CFG_sx1272_radio 1
	#else
		#define CFG_sx1276_radio 1
	#endif
#endif

#if CONFIG_LUA_RTOS_LORAWAN_BAND_EU868
	#define CFG_eu868 1
#else
	#if CONFIG_LUA_RTOS_LORAWAN_BAND_US915
		#define CFG_us915 1
	#else
		#define CFG_eu868 1
	#endif
#endif

#ifdef CONFIG_LUA_RTOS_LMIC_SPI
#define LMIC_SPI CONFIG_LUA_RTOS_LMIC_SPI
#else
#define LMIC_SPI 3
#endif

#ifdef CONFIG_LUA_RTOS_LMIC_RST
#define LMIC_RST CONFIG_LUA_RTOS_LMIC_RST
#else
#define LMIC_RST 27
#endif

#ifdef CONFIG_LUA_RTOS_LMIC_CS
#define LMIC_CS CONFIG_LUA_RTOS_LMIC_CS
#else
#define LMIC_CS 5
#endif

#ifdef CONFIG_LUA_RTOS_LMIC_DIO0
#define LMIC_DIO0 CONFIG_LUA_RTOS_LMIC_DIO0
#else
#define LMIC_DIO0 26
#endif

#ifdef CONFIG_LUA_RTOS_LMIC_DIO1
#define LMIC_DIO1 CONFIG_LUA_RTOS_LMIC_DIO1
#else
#define LMIC_DIO1 25
#endif

#ifdef CONFIG_LUA_RTOS_LMIC_DIO2
#define LMIC_DIO2 CONFIG_LUA_RTOS_LMIC_DIO2
#else
#define LMIC_DIO2 33
#endif

// SD Card / FAT
#if CONFIG_LUA_RTOS_USE_FAT
#define USE_FAT 1
#define USE_SD 1
#else
#define USE_FAT 0
#define USE_SD 0
#endif

#ifdef CONFIG_LUA_RTOS_SD_SPI
#define SD_SPI CONFIG_LUA_RTOS_SD_SPI
#else
#define SD_SPI 2
#endif

#ifdef CONFIG_LUA_RTOS_SD_CS
#define SD_CS CONFIG_LUA_RTOS_SD_CS
#else
#define SD_CS 15
#endif

#define THREAD_LOCAL_STORAGE_POINTER_ID 0

#endif
