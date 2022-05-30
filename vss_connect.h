#ifndef VSS_CONNECT_H_
#define VSS_CONNECT_H_

#include <WinInet.h>

int connectToEDB(void** data, const char* p_url, DWORD buf_size);
static int retrieveSpec(HINTERNET h_url, void* data, DWORD buf_size);

#endif