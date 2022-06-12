#ifndef PARSEVSS_H_
#define PARSEVSS_H_

#include <stdio.h>
#include "ost_data.h"

#define FIRST_VSS_LINE 14

// VSS file functions
//int skipToVariantsFile(FILE* fp, fpos_t* fpos);
//int countLinesFile(FILE* fp);
//int processVssLineFile(FILE* fp, struct variant* var);
struct variant* parseVssFile(const char* file_path, int* num_var);

// VSS buffer functions
//int skipToVariantsBuffer(char** cur_pos);
//int countLinesBuffer(char* buf);
// void processVssLineBuffer(char** buf_pos, struct variant* var);
struct variant* parseVssBuffer(char* buf_pos, int* num_var);

#endif