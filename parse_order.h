#ifndef _PARSE_ORDER_H_
#define _PARSE_ORDER_H_

#include "ost_data.h"

int GetLineBuffer(const char* src, char* dest, int dest_size);
struct variant* parseOrderBuffer(char* buf, int* num_var);

// Macros
#define GetIDVAR6(line, dest)  GetProperty((line),(dest),5,IDVAR6_LENGTH)
#define GetFamDesc(line, dest) GetProperty((line),(dest),1,FAM_DESC_LENGTH)
#define GetSymbol(line, dest)  GetProperty((line),(dest),1,SYMBOL_LENGTH)
#define GetVarDesc(line, dest) GetProperty((line),(dest),1,VAR_DESC_LENGTH)

#endif