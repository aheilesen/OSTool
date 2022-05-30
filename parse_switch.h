#ifndef PARSES_WITCH_H_
#define PARSES_WITCH_H_

#include <Windows.h>

#include "andrewll.h"

#define NUM_LOC_6605		39

#define VAR_STR_LENGTH		100
#define PLUG			22997159
#define COVER			82303552

int skipToSwitches(char** c);
int getSwitchLoc(const char* line);
int getPartNum(const char* line);
int getSWQty(const char* line);
int getVarString(char* buf, const char* line);
int parseCSV(LL** pSwitchList, const Variant* var_list, int num_var,
             WORD res_ID);
int getLine(char* dest, int dest_size, char** src);
int insertNewSW(LL* sw_list, int loc, int pn, const char* buf, int qty);
int processCSVLine(int* loc, int* pn, int* qty, char* buf, const char* line);
int checkVarString(const Variant* var_list, int num_var, const char* sw_vars);
int removeSW(LL* sw_list, int loc, int pn);
void freeSWLink(void* link);

#endif