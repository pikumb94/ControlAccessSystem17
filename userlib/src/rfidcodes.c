#include "rfidcodes.h"

#if USERLIB_USE_RFIDCODES || defined(__DOXYGEN__)


int8_t codeIndex(RfidCodes *validCodes, MifareUID *code){
  int8_t i=0;

  while(i<validCodes->size){
    if(equalCodes(code,&validCodes->validId[i]))
      return i;
    i++;
  }

  return -1;

}


bool equalCodes(MifareUID *code1, MifareUID *code2){

  uint8_t i=0;
  bool flag=true;
  if(code1->size!=code2->size){
      return false;
  }

  while(i<code1->size && flag){
    if(code1->bytes[i]!=code2->bytes[i])
      flag=false;
    i++;
  }
  return flag;




}



#endif
