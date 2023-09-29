/**
 * @file Packethandler.h
 * @brief file containg functions for handling packets
 * @author Adhil
 * @date  29-092023
 * @note
*/


/**************************************INCLUDES***************************/
#include "JsonHandler.h"
/**************************************MACROS*****************************/

/**************************************TYPEDEFS***************************/

/***********************************FUNCTION DEFINITION*******************/

/**
 * @brief function to add json object to json
 * @param pcJsonHandle - Json object handle
 * @param pcKey - Key name
 * @param pcValue - value
 * @param ucLen - value length
 * @return true or false
*/
bool AddItemtoJsonObject(cJSON **pcJsonHandle, _eJsonDataType JsondataType, const char *pcKey, 
                    void *pcValue, uint8_t ucLen)
{
    uint8_t ucIndex = 0;
    cJSON *pcData = NULL;
    bool bRetVal = false;

    if (*pcJsonHandle && pcKey && pcValue)
    {
        switch(JsondataType)
        {
            case NUMBER: 
                         cJSON_AddNumberToObject(*pcJsonHandle, pcKey, *((uint16_t*)(pcValue)));
                         break;
            case STRING: cJSON_AddStringToObject(*pcJsonHandle, pcKey, (char* )pcValue);
                         break;
            case OBJECT:    
                         break;
            case ARRAY: pcData = cJSON_AddArrayToObject(*pcJsonHandle, "data");

                        if (pcData)
                        {
                            for (ucIndex = 0; ucIndex < ucLen; ucIndex++)
                            {
                                cJSON_AddItemToArray(pcData, cJSON_CreateNumber(*((uint16_t*)pcValue+ucIndex)));
                            }
                        }
                         break;
            default:
                    break;                          
        }
        bRetVal = true;
    }

    return bRetVal;
}
