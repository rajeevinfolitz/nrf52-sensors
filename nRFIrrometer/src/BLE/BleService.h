/**
 * @file BleService.h
 * @brief File containing Visense service related handling
 * @author
 * @see BleService.c
 * @date 27-09-2023
*/

/**************************************INCLUDES******************************/
#include "BleHandler.h"

/***************************************MACROS*******************************/

/**************************************TYPEDEFS******************************/

/*************************************FUNCTION DECLARATION*******************/
int VisenseSensordataNotify(uint8_t *pucSensorData, uint16_t unLen);
void BleSensorDataNotify(const struct bt_gatt_attr *attr, uint16_t value);
bool IsNotificationenabled();