/*
 * This library provides support to verify if a code matches with a valid code stored in memory.
 *
 *
 *
 */

#ifndef _RFIDCODES_H_
#define _RFIDCODES_H_

#include "hal.h"
#include "userconf.h"
#include "mfrc522.h"

#if USERLIB_USE_RFIDCODES || defined(__DOXYGEN__)


#define MAX_ID_NUM 20

typedef struct RfidCodes{
    MifareUID validId[MAX_ID_NUM];
    uint8_t size;
}RfidCodes;



#ifdef __cplusplus
extern "C" {
#endif
     int8_t codeIndex(RfidCodes *validCodes, MifareUID *code);
     bool equalCodes(MifareUID *code1, MifareUID *code2);

#ifdef __cplusplus
}
#endif
#endif
#endif
