/**
 * @file Packethandler.h
 * @brief file containg functions for handling packets
 * @author Adhil
 * @date  29-092023
 * @note
*/

#ifndef _JSON_HANDLER_H
#define _JSON_HANDLER_H

/**************************************INCLUDES***************************/
#include <stdint.h>
#include <stdbool.h>
#include "cJSON.h"
/**************************************MACROS*****************************/

/**************************************TYPEDEFS***************************/
typedef enum __eJsonDataType
{
    NUMBER,
    STRING,
    ARRAY,
    OBJECT,
}_eJsonDataType;

/***********************************FUNCTION DECLARATION******************/

bool AddItemtoJsonObject(cJSON **pcJsonHandle, _eJsonDataType JsondataType, const char *pcKey, 
                    void *pcValue, uint8_t ucLen);
bool ParseRxData(uint8_t *pData,const char *pckey, uint8_t ucLen, uint32_t *pucData);
#endif