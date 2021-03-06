/*
 * Lua RTOS, Lora WAN driver for RN2483
 *
 * Copyright (C) 2015 - 2016
 * IBEROXARXA SERVICIOS INTEGRALES, S.L. & CSS IBÉRICA, S.L.
 * 
 * Author: Jaume Olivé (jolive@iberoxarxa.com / jolive@whitecatboard.org)
 * 
 * All rights reserved.  
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and this
 * permission notice and warranty disclaimer appear in supporting
 * documentation, and that the name of the author not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * The author disclaim all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 */

#include "luartos.h"

#ifndef LORA_H
#define LORA_H

#if LUA_USE_LORA

#include <sys/driver.h>

// Lora errors
#define LORA_ERR_KEYS_NOT_CONFIGURED                (DRIVER_EXCEPTION_BASE(LORA_DRIVER_ID) |  1)
#define LORA_ERR_JOIN_DENIED                        (DRIVER_EXCEPTION_BASE(LORA_DRIVER_ID) |  2)
#define LORA_ERR_UNEXPECTED_RESPONSE                (DRIVER_EXCEPTION_BASE(LORA_DRIVER_ID) |  3)
#define LORA_ERR_NOT_JOINED                         (DRIVER_EXCEPTION_BASE(LORA_DRIVER_ID) |  4)
#define LORA_ERR_NOT_SETUP                          (DRIVER_EXCEPTION_BASE(LORA_DRIVER_ID) |  5)
#define LORA_ERR_NO_MEM					            (DRIVER_EXCEPTION_BASE(LORA_DRIVER_ID) |  6)
#define LORA_ERR_ABP_EXPECTED			            (DRIVER_EXCEPTION_BASE(LORA_DRIVER_ID) |  7)
#define LORA_ERR_CANT_SETUP				            (DRIVER_EXCEPTION_BASE(LORA_DRIVER_ID) |  8)
#define LORA_ERR_TRANSMISSION_FAIL_ACK_NOT_RECEIVED (DRIVER_EXCEPTION_BASE(LORA_DRIVER_ID) |  9)
#define LORA_ERR_INVALID_ARGUMENT                   (DRIVER_EXCEPTION_BASE(LORA_DRIVER_ID) | 10)

// Lora Mac set commands
#define LORA_MAC_SET_DEVADDR		0
#define LORA_MAC_SET_DEVEUI			1
#define LORA_MAC_SET_APPEUI			2
#define LORA_MAC_SET_NWKSKEY		3
#define LORA_MAC_SET_APPSKEY		4
#define LORA_MAC_SET_APPKEY			5
#define LORA_MAC_SET_DR				6
#define LORA_MAC_SET_ADR			7
#define LORA_MAC_SET_LINKCHK        8

// Lora Mac get commands
#define LORA_MAC_GET_DEVADDR	   20
#define LORA_MAC_GET_DEVEUI		   21
#define LORA_MAC_GET_APPEUI		   22
#define LORA_MAC_GET_DR			   23
#define LORA_MAC_GET_ADR		   24
#define LORA_MAC_GET_LINKCHK       25

typedef void (lora_rx)(int port, char *payload);

driver_error_t *lora_setup(int band);
driver_error_t *lora_mac_set(const char command, const char *value);
driver_error_t *lora_mac_get(const char command, char **value);
driver_error_t *lora_join();
driver_error_t *lora_tx(int cnf, int port, const char *data);

void lora_set_rx_callback(lora_rx *callback);

#endif

#endif /* LORA_H */

