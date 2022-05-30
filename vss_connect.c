////////////////////////////////////////////////////////////////////////////////
// vss_connect.c                                                              //
//                                                                            //
// This TU contains data and functions used to download a truck spec from our //
// database, called EDB. It achieves this in two steps:                       //
//                                                                            //
// 1) An internet handle to the EDB URL is acquired through a series of       //
//    WinInet function calls.                                                 //
// 2) The webpage data is downloaded and stored into a temporary buffer       //
//    using the InternetReadFile() function, which uses the aforementioned    //
//    internet handle.                                                        //
//                                                                            //
// The internet handles (of type HINTERNET) are similar to other Windows      //
// handle types, but differ in that they need to be closed with a             //
// WinInet function called InternetCloseHandle(). The WinInet functions       //
// abstract away low-level HTTP protocols and behave like other Win32 API     //
// functions, making development of internet-connected applications easier.   //
//                                                                            //
// The retrieveSpec() function, which calls InternetReadFile(), reads 64 KiB  //
// of data at a time. A spec is around ~192 KiB so it needs to be called a    //
// few times before the spec is completely downloaded.                        //
////////////////////////////////////////////////////////////////////////////////

#include <Windows.h>
#include <strsafe.h>
#include <tchar.h>	// for _itot_s()

#include "vss_connect.h"

////////////////////////////////////////////////////////////////////////////////
// connectToEDB                                                               //
//                                                                            //
// Calls a series of WinInet functions to retrieve a HINTERNET (internet      //
// handle) that can be used to call InternetReadFile(). First, the function   //
// checks to see if a connection can be established to the EDB URL provided.  //
// This will fail if the computer is not connected to Volvo's internal        //
// network.                                                                   //
//                                                                            //
// InternetOpen() returns a HINTERNET that is used to call                    //
// InternetOpenUrl(), which also returns a HINTERNET. This latter HINTERNET   //
// is passed to retrieveSpec(). If at any point an error is encountered, the  //
// internet handles are closed with InternetCloseHandle(). They are also      //
// closed at the end of the function. This is a requirement of using the      //
// WinInet functions per official documentation.                              //
//                                                                            //
// This function takes a pointer to pointer named 'data', which points to the //
// address of the buffer in which the spec is stored. Once a spec is          //
// successfully acquired, the 'data' argument provided to connectToEDB()      //
// will point to an address of memory on the heap. It's freed in              //
// parseVssBuffer(), in the WM_COMMAND processing in ostool.c.                //
//                                                                            //
// The parameter buf_size is the size of the destination buffer and is        //
// provided to ensure the end of the buffer is not written past.              //
////////////////////////////////////////////////////////////////////////////////

int connectToEDB(void** data, const char* p_url, DWORD buf_size)
{
	HINTERNET h_open = NULL;
	HINTERNET h_url = NULL;

	if (!InternetCheckConnectionA("https://edb.volvo.net/edb2/index.htm",
	                              FLAG_ICC_FORCE_CONNECTION, 0))
		return -1;

	h_open = InternetOpenA("CE_SW_TOOL", INTERNET_OPEN_TYPE_PRECONFIG,
	                       NULL, NULL, 0);

	if (!h_open)
		return -2;

	// At this point h_open is a valid handle
	h_url = InternetOpenUrlA(h_open, p_url, NULL, 0,
	                         INTERNET_FLAG_RESYNCHRONIZE |
	                         INTERNET_FLAG_NO_UI | INTERNET_FLAG_SECURE, 0);

	if (!h_url) {
		InternetCloseHandle(h_open);
		return -3;
	}

	// At this point, both h_open and h_url are valid handles

	if ((*data = malloc(buf_size)) == NULL) {
		InternetCloseHandle(h_url);
		InternetCloseHandle(h_open);
		return -4;
	}

	// At this point, both h_open and h_url are still valid handles.
	// 'data' is pointing to memory on the heap

	if (retrieveSpec(h_url, *data, buf_size)) {
		InternetCloseHandle(h_url);
		InternetCloseHandle(h_open);
		free(*data);
		return -5;
	}

	InternetCloseHandle(h_url);
	InternetCloseHandle(h_open);

	// At this point, the handles have been closed.
	// 'data' is pointing to memory on the heap. It will be up to
	// the application to free it.
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
// retrieveSpec                                                               //
//                                                                            //
// Helper function for connectToEDB(). This function makes repeated calls to  //
// InternetReadFile() until the entire spec is downloaded. It downloads 64    //
// KiB at a time.                                                             //
//                                                                            //
// 'h_url' is a HINTERNET defined in the connectToEDB() function, and         //
// 'buf_size' provides the size of the destination buffer. This is used to    //
// ensure writes aren't made past the end of the buffer. If the buffer is     //
// too small to hold a spec, the function returns -1 to indicate an error.    //
// Both 'h_url' and 'data' are checked for validity in connectToEDB() before  //
// they are used here.                                                        //
//                                                                            //
// When this function returns successfully, the destination buffer will       //
// contain a full truck spec downloaded from the EDB database.                //
//                                                                            //
// Possible future improvement: InternetReadFile() is called                  //
// synchronously. If the connection is interrupted before InternetReadFile()  //
// returns, it could freeze the application. The download is quite small      //
// (around 192 KiB), so a connection loss during a download is unlikely.      //
// Additionally, the program does not perform a critical function or have     //
// safety implications, so having to force close it in an unlikely scenario   //
// is not a serious issue. Still, it's high on the priority list of future    //
// fixes because it's suboptimal design.                                      //
////////////////////////////////////////////////////////////////////////////////

static int retrieveSpec(HINTERNET h_url, void* data, DWORD buf_size)
{
	BOOL f_read_ok = FALSE;

	DWORD bytes_read = 0;
	DWORD bytes_accum = 0;
	DWORD min_read = 0;

	DWORD bytes_per_call = 65536;

	if (buf_size < bytes_per_call + 1)		// + 1 for EOF marker
		return -1;

	do {
		// This macro ensures writes aren't made past the end of the
		// buffer. The -1 accounts for the EOF marker
		min_read = min(bytes_per_call, buf_size - bytes_accum - 1);

		f_read_ok =
		InternetReadFile(h_url, (LPBYTE)(data) + bytes_accum, min_read,
		                 &bytes_read);

		if (!f_read_ok)
			return -2;

		bytes_accum += bytes_read;

	} while (bytes_read);

	// Add EOF marker
	*((char*)(data) + bytes_accum) = '~';

	return 0;
}