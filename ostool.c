////////////////////////////////////////////////////////////////////////////////
// ostool.c                                                                   //
//                                                                            //
// The main translation unit for the CE (Customer Engineering) dash switch    //
// Visualizer application. Contains the WinMain entry point and main window   //
// procedure.                                                                 //
//                                                                            //
// The WM_CREATE processing in the main window procedure creates the three    //
// main child windows for the application: the banner, list view and cab      //
// view windows. WM_COMMAND processing calls functions that parse the spec    //
// and resource files.                                                        //
//                                                                            //
// Static data declared in the main window procedure is shared between the    //
// main window and child windows. This is achieved by storing the address of  //
// this data in the 'USERDATA' section of each window. This 'state' data is   //
// stored in a structure of type struct _tag_STATE_DATA (see ost_data.h).     //
//                                                                            //
// Besides the lengthy window procedure, this TU contains functions that      //
// create custom fonts from resources, load bitmaps from resources and        //
// create memory device contexts, free dynamically allocated memory, notify   //
// the user of errors, and perform a few other miscellaneous tasks.           //
//                                                                            //
// Each function (other than the window procedures) has a comment header      //
// that explains the purpose of the function and provides additional info. I  //
// tried to write code that is understandable without many comments, but      //
// there are inline comments throughout as well. There are some places where  //
// possible future improvements are noted (as seen below).                    //
//                                                                            //
// Possible future improvements: comb through all of the code and ensure the  //
// following is done:                                                         //
//                                                                            //
// 1) Use const whenever passing a pointer to data that is not modified.      //
// 2) Use static for functions that aren't used outside of their TU.          //
// 3) Initialize all variables.                                               //
// 4) Check for unnecessary header inclusions                                 //
// 5) Improve consistency of struct variable declarations (sometimes I used   //
//    a typedef'd type, sometimes I used struct <type>; sometimes I used a    //
//    typedef'd pointer to a struct, and other times I used struct <type>*).  //
//                                                                            //
// Additionally, I'd like to have the program compile with no warnings when   //
// using /Wall (all warnings enabled). As of ver 1.00 it compiles with no     //
// warnings when using /W4 (level 4 warnings enabled). Level 4 is higher      //
// than the default, but there are plenty of warnings shown with the all      //
// warnings setting that aren't shown with level 4. The vast majority of      //
// these warnings are either the compiler stating padding insertions or the   //
// result of a bug in winioctl.h.                                             //
////////////////////////////////////////////////////////////////////////////////

#include <Windows.h>
#include <windowsx.h>	// for GET_X_LPARAM and GET_Y_LPARAM macros

#include "resource.h"

#include "ostool.h"
#include "vss_connect.h"    // for internet retrieval of VSS spec

const char g_title[] = "CE Dash Visualizer";

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
                   _In_ PSTR pCmdLine, _In_ int nCmdShow)
{
	char app_name[] = "CE_SW_VIS";
	HANDLE h_accel;
	HWND hwnd;
	MSG msg;

	WNDCLASSA wndclass = { 0 };
	wndclass.style          = 0;
	wndclass.lpfnWndProc    = WindowProc;
	wndclass.cbClsExtra     = 0;
	wndclass.cbWndExtra     = 0;
	wndclass.hInstance      = hInstance;
	wndclass.hIcon          = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor        = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground  = GetStockObject(WHITE_BRUSH);
	wndclass.lpszMenuName   = NULL;
	wndclass.lpszClassName  = app_name;

	if (!RegisterClassA(&wndclass)) {
		MessageBoxA(NULL, "Failed to register window class!",
			app_name, MB_ICONERROR);
		return -1;
	}

	hwnd = CreateWindowA(app_name, g_title,
	                     WS_OVERLAPPEDWINDOW,
	                     CW_USEDEFAULT, CW_USEDEFAULT,
	                     1453, 929,
	                     NULL, NULL, hInstance, NULL);

	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);

	h_accel = LoadAcceleratorsA(hInstance, "VSS_ACCEL");

	while (GetMessageA(&msg, NULL, 0, 0)) {
		if (!TranslateAccelerator(hwnd, h_accel, &msg)) {
			TranslateMessage(&msg);
			DispatchMessageA(&msg);
		}
	}

	// Clear C4100 compiler warning
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(pCmdLine);

	return (int)(msg.wParam);
}

LRESULT CALLBACK WindowProc
(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HINSTANCE h_instance;

	// VSS file variables
	static struct variant* var_list;
	static int num_var;

	static HWND hwnd_banner;
	static HWND hwnd_list_view;
	static HWND hwnd_cab_view;

	static STATE_DATA state_data = { 0 };

	HDC hdc;

	// Window sizing variables
	UINT dpi;
	RECT win_rect = { 0 };
	RECT desk_rect = { 0 };

	// HATCH4 must be last. Add any new bitmaps at the end of the list but
	// before HATCH4. This is because it's loaded into memory differently
	// in the loadBitmaps() function.
	static SW_BITMAP sw_bitmaps[] = {
		NULL, NULL, "P",               // 0
		NULL, NULL, "C",               // 1
		NULL, NULL, "D",               // 2
		NULL, NULL, "E",               // 3
		NULL, NULL, "M",               // 4
		NULL, NULL, "ABFGHIJKL",       // 5
		NULL, NULL, "DASH",            // 6
		NULL, NULL, "TRUCK",           // 7 - 64  x 78
		NULL, NULL, "MAINTITLE",       // 8 - 388 x 78
		NULL, NULL, "BUTTON_ICONS",    // 9
		NULL, NULL, "TOGGLES",         // 10
		NULL, NULL, "RIGHTARROW",      // 11
		NULL, NULL, "HATCH4",
	};

	switch (message) {

	case WM_CREATE:
		
		hdc = GetDC(hwnd);

		state_data.num_bitmaps = sizeof(sw_bitmaps) / sizeof(sw_bitmaps[0]);

		// Initialize button_data struct declared in ost_shared.c
		if (getOldButtonProc(hwnd))
			return -1;

		if (createMemoryDCs(hdc, sw_bitmaps, state_data.num_bitmaps)) {
			ReleaseDC(hwnd, hdc);
			return -1;
		}
		ReleaseDC(hwnd, hdc);
		
		h_instance = ((LPCREATESTRUCTA)lParam)->hInstance;

		if (loadBitmaps(h_instance, sw_bitmaps, state_data.num_bitmaps))
		{
			deleteMemoryDCs(sw_bitmaps, state_data.num_bitmaps);
			return -1;
		}

		if (selectBitmaps(sw_bitmaps, state_data.num_bitmaps)) {
			deleteMemoryDCs(sw_bitmaps, state_data.num_bitmaps);
			destroyBitmaps( sw_bitmaps, state_data.num_bitmaps);
			return -1;
		}
		
		SetClassLongPtrA(hwnd, GCLP_HBRBACKGROUND,
		                (LONG_PTR)CreateSolidBrush(RGB(240, 240, 240)));

		// Set starting zone 4 panel to the 10-switch panel
		state_data.src_bitmap_pos[0] = 3;

		///////////////////////////////////////////////////////////////
		// Prepare CREATE_DATA structure

		state_data.p_sw_list = NULL;
		state_data.p_bitmaps = sw_bitmaps;
		state_data.num_bitmaps = state_data.num_bitmaps;

		// Load Novum font resources
		loadNovumFont(h_instance, "NOVUM",     "BINFONT");
		loadNovumFont(h_instance, "NOVUM_MED", "BINFONT");

		// Create Novum fonts
		state_data.h_font_title = createVSSFont(hwnd, "Volvo Novum Medium", 18);
		state_data.h_font_text  = createVSSFont(hwnd, "Volvo Novum",        12);


		if (createChildWindows(h_instance, hwnd, &hwnd_banner,
		                       &hwnd_list_view, &hwnd_cab_view,
		                       &state_data))
		{
			deleteMemoryDCs(sw_bitmaps, state_data.num_bitmaps);
			destroyBitmaps( sw_bitmaps, state_data.num_bitmaps);
			return -1;
		}

		// WINDOW SIZING
		dpi = GetDpiForWindow(hwnd);

		// Calculate initial size of window
		win_rect.left = 0;
		win_rect.right = 1453;
		win_rect.top = 0;
		win_rect.bottom = 929;
		AdjustWindowRectExForDpi(&win_rect, WS_OVERLAPPEDWINDOW,
		                         FALSE, 0, dpi);

		// Position window in center of work area
		// SystemParametersInfoForDpi is not meant to be used with
		// SPI_GETWORKAREA
		SystemParametersInfoA(SPI_GETWORKAREA, 0, &desk_rect, 0);

		MoveWindow(hwnd,
		           ((desk_rect.right - desk_rect.left) -
		           (win_rect.right - win_rect.left)) / 2,
		           ((desk_rect.bottom - desk_rect.top) -
		           (win_rect.bottom - win_rect.top)) / 2,
		           win_rect.right - win_rect.left,
		           win_rect.bottom - win_rect.top, FALSE);

		// Set focus to VSS # edit control
		SendMessageA(hwnd_banner, WM_SETFOCUSEDIT, 1, 0);

		return 0;

	// Possible future improvement: this program does not move or resize
	// the child windows. If the main window size is changed, the child
	// windows will either be hidden or aligned completely to the left.
	// My users have 1080p displays so this is not a huge issue; however,
	// it would be best to account for the possibility of different
	// resolutions.
	case WM_SIZE:
		MoveWindow(hwnd_banner, 0, 0, LOWORD(lParam), 88, TRUE);
		MoveWindow(hwnd_list_view, 32, 120, 384, 777, TRUE);
		MoveWindow(hwnd_cab_view, 448, 120, 973, 777, TRUE);
		return 0;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {          // Accelerator or control ID
		case BTN_ID_FILE:
		case BTN_ID_ARROW: {

			// This buffer stores the title bar text when a COS retrieval
			// has been performed or a file has been opened. +14 includes
			// space fora VSS number (VSS-##-######) and terminating null.
			char title_text[MAX_PATH + 14];
			rsize_t title_size = MAX_PATH + 14;

			// Clear everything
			SendMessageA(hwnd, WM_COMMAND, MAKEWPARAM(BTN_ID_CLEAR, 0), 0);

			if (LOWORD(wParam) == BTN_ID_FILE) {      // Spec from file
				OPENFILENAMEA ofn;
				char file_path[MAX_PATH];
				if (!getFileInfo(hwnd, &ofn, file_path, MAX_PATH)) {

					/* If a user closes an open file dialog, the return
					 * value will be zero. Since the return value for an
					 * error is also zero, a call to CommDlgExtendedError()
					 * is necessary to determine if an error actually occurred
					 * for a return value of zero. CommDlgExtendedError()
					 * will return a value greater than zero if an error
					 * occurred (see msdn article for  CommDlgExtendedError()
					 * for different values it can return). */

					if (CommDlgExtendedError()) {
						MessageBoxA(hwnd, "Could not open file!",
						            "Error", MB_ICONERROR);
						SendMessageA(hwnd_banner, WM_SETFOCUSEDIT, 1, 0);
					}
					return 0;
				}

				// parseVssFile closes the file
				var_list = parseVssFile(ofn.lpstrFile, &num_var);
				if (var_list == NULL) {
					MessageBoxA(hwnd, "Couldn't parse VSS file...",
					            "Error!", MB_ICONERROR);
					SendMessageA(hwnd_banner, WM_SETFOCUSEDIT, 1, 0);
					return 0;
				}

				// !
				// At this point, var_list points to memory allocated on heap
				// !

				// Set title bar text to display the opened file
				strncpy_s(title_text, title_size, g_title, strlen(g_title));
				strncat_s(title_text, title_size, " - ", 3);
				strncat_s(title_text, title_size, file_path, strlen(file_path));
				SetWindowTextA(hwnd, title_text);
			}
			else {      // Spec retrieved from EDB

				// Internet spec retrieval variables
				char* vss_buf = NULL;
				DWORD buf_size = 200000;

				const char* url     = ((struct vss_search_info*)lParam)->url;
				const char* vss_num = ((struct vss_search_info*)lParam)->vss_num;

				SetCursor(LoadCursor(NULL, IDC_WAIT));

				// lParam points to a structure with the url
				// connectToEDB closes h_open and h_url handles
				if (connectToEDB(&vss_buf, url, buf_size)) {
					MessageBoxA(NULL, "Error connecting to EDB",
					            "Error", MB_ICONERROR);
					SendMessageA(hwnd_banner, WM_SETFOCUSEDIT, 1, 0);
					return 0;
				}

				// !
				// At this point, vss_buf points to memory allocated on heap
				// !

				// parseVssBuffer frees vss_buf
				var_list = parseVssBuffer(vss_buf, &num_var);
				if (var_list == NULL) {
					MessageBoxA(hwnd, "Error downloading VSS spec!\n\n"
					            "Make sure the VSS number was entered correctly.",
					            "VSS Error", MB_ICONERROR);
					SendMessageA(hwnd_banner, WM_SETFOCUSEDIT, 1, 0);
					return 0;
				}

				// !
				// At this point, var_list points to memory allocated on heap
				// !

				// Set title bar text to display the VSS # retrieved
				strncpy_s(title_text, title_size, g_title, strlen(g_title));
				strncat_s(title_text, title_size, " - ", 3);
				strncat_s(title_text, title_size, vss_num, strlen(vss_num));
				SetWindowTextA(hwnd, title_text);
			}

			// !
			// At this point, var_list points to memory on the heap.
			// It won't be freed until another spec is loaded, the
			// clear button is clicked, or the program is exited.
			// !

			// Parse standard product switch data file
			if (parseCSV(&(state_data.p_sw_list), var_list, num_var,
						 IDR_CSV3)) {
				MessageBoxA(hwnd, "SP CSV Error!", "Error!",
							MB_ICONERROR);
				SendMessageA(hwnd, WM_COMMAND, MAKEWPARAM(BTN_ID_CLEAR, 0), 0);
				return 0;
			}

			// Parse CA switch data file
			int pcsv;
			char pc_buf[50] = { 0 };
			if ((pcsv = parseCSV(&(state_data.p_sw_list), var_list, num_var,
			                     IDR_CSV4)) != 0) {
				wsprintfA(pc_buf, "CA CSV Error! (%d)", pcsv);
				MessageBoxA(hwnd, pc_buf, "Error!", MB_ICONERROR);
				SendMessageA(hwnd, WM_COMMAND, MAKEWPARAM(BTN_ID_CLEAR, 0), 0);
				return 0;
			}

			getSrcBitmapPos(state_data.p_sw_list,
							state_data.src_bitmap_pos);

			state_data.src_bitmap_pos[0] = getSwPanel(var_list, num_var);

			notifyConflicts(state_data.p_sw_list);
			notifyPanel(state_data.p_sw_list, state_data.src_bitmap_pos[0]);

			// This call is required to set the input focus to the VSS # edit
			// control if notifyConflicts displays a message box
			SendMessageA(hwnd_banner, WM_SETFOCUSEDIT, 1, 0);
			
			InvalidateRect(hwnd, NULL, FALSE);
			return 0;
		}

		case BTN_ID_HELP:
			DialogBoxA(NULL, "AbtDlg", hwnd, (DLGPROC)AboutDlgProc);
			SendMessageA(hwnd_banner, WM_SETFOCUSEDIT, 1, 0);
			return 0;

		case BTN_ID_CLEAR:
			freeMemory(&var_list, &(state_data.p_sw_list));
			clearSrcBitmapPos(state_data.src_bitmap_pos);

			// Clear list box
			SendMessageA(hwnd_list_view, WM_COMMAND, wParam, lParam);

			// Clear highlight square
			SendMessageA(hwnd_cab_view, WM_DRAWHIGHLIGHT, 0, -1);

			// Reset title bar text
			SetWindowTextA(hwnd, "CE Dash Visualizer");

			SendMessageA(hwnd_banner, WM_SETFOCUSEDIT, 1, 0);

			InvalidateRect(hwnd, NULL, FALSE);
			return 0;

		//////////////////////////////////////////////////////////////////////////
		// KEYBOARD ACCELERATORS

		case ID_ACCEL_CLR:
			// List view window proc does not use lParam, so it's OK to
			// pass 0 here
			SendMessageA(hwnd, WM_COMMAND, MAKEWPARAM(BTN_ID_CLEAR, 0), 0);
			break;
		case ID_ACCEL_DESEL:
			SendMessageA(hwnd, WM_LBUTTONDOWN, 0, 0);
			break;
		case ID_ACCEL_EDIT:
			SendMessageA(hwnd_banner, WM_SETFOCUSEDIT, 1, 0);
			break;
		case ID_ACCEL_HELP:
			SendMessageA(hwnd, WM_COMMAND, MAKEWPARAM(BTN_ID_HELP, 0), 0);
			break;
		case ID_ACCEL_LGND:
			SendMessageA(hwnd_cab_view, WM_COMMAND,
			             MAKEWPARAM(TOG_ID_LEGEND, 0), 0);
			break;
		case ID_ACCEL_OPEN:
			SendMessageA(hwnd, WM_COMMAND, MAKEWPARAM(BTN_ID_FILE, 0), 0);
			break;

		}
		return 0;

	case WM_LBUTTONDOWN:
		SendMessageA(hwnd, WM_CLEARHIGHLIGHT, 0, -1);
		return 0;

	// Custom message - see description in ost_shared.h
	case WM_CLEARHIGHLIGHT:
		SendMessageA(hwnd_list_view, WM_CLEARHIGHLIGHT, 0,  0);
		SendMessageA(hwnd_cab_view,  WM_DRAWHIGHLIGHT,  0, -1);
		return 0;

	// Custom message - see description in ost_shared.h
	case WM_DRAWHIGHLIGHT:
		if (wParam == -1) {
			SendMessageA(hwnd, WM_CLEARHIGHLIGHT, 0, 0);
			return 0;
		}

		int hi_pos = getHighlightPos(state_data.p_sw_list, (int)wParam);

		if (hi_pos < 0) {
			SendMessageA(hwnd, WM_CLEARHIGHLIGHT, 0, 0);
			return 0;
		}

		// Switch numbers jump from 30 to 35. First subtraction is
		// to deal with zero-index
		hi_pos--;
		if (hi_pos > 29)
			hi_pos -= 4;

		SendMessageA(hwnd_cab_view, WM_DRAWHIGHLIGHT, wParam, hi_pos);
		return 0;
	
	// Free all memory dynamically reserved by the program
	case WM_DESTROY:
		DeleteObject((HGDIOBJ)SetClassLongPtrA(hwnd, GCLP_HBRBACKGROUND,
					     (LONG_PTR)GetStockObject(WHITE_BRUSH)));

		if (state_data.h_font_title)
			DeleteObject(state_data.h_font_title);
		if (state_data.h_font_text)
			DeleteObject(state_data.h_font_text);

		freeMemory(&var_list, &(state_data.p_sw_list));
		deleteMemoryDCs(sw_bitmaps, state_data.num_bitmaps);
		destroyBitmaps( sw_bitmaps, state_data.num_bitmaps);
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProcA(hwnd, message, wParam, lParam);
}

////////////////////////////////////////////////////////////////////////////////
// freeMemory                                                                 //
//                                                                            //
// In this program there is some memory that needs to be allocated and freed  //
// each time a spec is processed. This memory consists of the variant list    //
// and the switch list.                                                       //
//                                                                            //
// The variant list is an array of struct variant objects allocated on the    //
// heap. The switch list is a linked list of struct sw_link objects, and is   //
// also allocated on the heap. Both of these structs are declared in the      //
// ost_data.h header file.                                                    //
//                                                                            //
// This function is called every time WM_COMMAND is processed with the        //
// LOWORD(wParam) == BTN_ID_CLEAR, which is the ID for the button that clears //
// the data on the screen. This happens at the beginning of WM_COMMAND        //
// processing for a new spec, if an error occurs while parsing a CSV file,    //
// or the clear button is clicked / hotkey is pressed.                        //
//                                                                            //
// It is also called in the WM_DESTROY processing.                            //
////////////////////////////////////////////////////////////////////////////////

void freeMemory(Variant** p_variant, LL** p_switch_list)
{
	free(*p_variant);
	*p_variant = NULL;

	if (*p_switch_list)
		LL_Destroy(*p_switch_list);
	*p_switch_list = NULL;
}

////////////////////////////////////////////////////////////////////////////////
// AboutDlgProc                                                               //
//                                                                            //
// Dialog procedure for the Help window dialog. All it does worth noting is   //
// call centerDialog() in the WM_INITDIALOG message processing to center the  //
// help dialog window in the main window.                                     //
////////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK AboutDlgProc
(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_INITDIALOG:
		centerDialog(hDlg);
		return TRUE;

	case WM_COMMAND:
		EndDialog(hDlg, TRUE);
		return TRUE;
	}

	// Clear C4100 compiler warning
	UNREFERENCED_PARAMETER(wParam);
	UNREFERENCED_PARAMETER(lParam);

	return FALSE;
}

////////////////////////////////////////////////////////////////////////////////
// createVSSFont                                                              //
//                                                                            //
// Once the Novum font resources are prepared (see loadNovumFont() below),    //
// this function creates the font so it can be selected into DCs and used in  //
// GDI functions in the program.                                              //
//                                                                            //
// If CreateFont() encounters an error, the function will return NULL. This   //
// may happen if loadNovumFont can't load the font resource.                  //
//                                                                            //
// Possible future improvement: like loadNovumFont(), the error for this      //
// function is not checked. This results to an ugly fallback condition (the   //
// bitmap System font being used). Checking for a NULL return value would     //
// allow for a nicer font to be used in case of an error.                     //
////////////////////////////////////////////////////////////////////////////////

HFONT createVSSFont(HWND hwnd, char* font_name, int height)
{
	HDC hdc = GetDC(hwnd);

	HFONT h_font =
	CreateFontA(-MulDiv(height, GetDeviceCaps(hdc, LOGPIXELSY), 72),
	            0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
	            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
	            DEFAULT_PITCH | FF_DONTCARE, font_name);

	ReleaseDC(hwnd, hdc);
	return h_font;
}

////////////////////////////////////////////////////////////////////////////////
// loadNovumFont                                                              //
//                                                                            //
// Loads the Novum font resource into memory, and calls AddFontMemResourceEX  //
// which allows the application to create a font with it. The font will only  //
// be visible to this application.                                            //
//                                                                            //
// The MSDN documentation for AddFontMemResourceEx states that the system     //
// will unload the fonts from memory if the process does not call             //
// RemoveFontMemResourceEx.                                                   //
//                                                                            //
// Possible future improvement: this function returns negative integers to    //
// indicate an error occurred while loading the font. However, the caller     //
// doesn't use this return value to check whether it was successful. If for   //
// some reason the fonts aren't loaded properly, the application will fall    //
// back to the default device context font, which is the bitmap System font.  //
// It might be desirable to fall back to a prettier font. This error seems    //
// quite unlikely since the font is loaded from a resource which is embedded  //
// in the executable.                                                         //
////////////////////////////////////////////////////////////////////////////////

int loadNovumFont(HINSTANCE h_instance, char* res_name, char* res_type)
{
	HRSRC h_rsrc_novum;
	HGLOBAL h_res_novum;
	LPVOID p_novum;
	HANDLE h_font_novum;
	DWORD novum_size;
	DWORD num_fonts = 0;

	if ((h_rsrc_novum = FindResourceA(h_instance, res_name, res_type)) == NULL)
		return -1;
	if ((h_res_novum = LoadResource(h_instance, h_rsrc_novum)) == NULL)
		return -2;
	if ((p_novum = LockResource(h_res_novum)) == NULL)
		return -3;
	if ((novum_size = SizeofResource(h_instance, h_rsrc_novum)) == 0)
		return -4;

	h_font_novum = AddFontMemResourceEx(p_novum, novum_size, NULL, &num_fonts);
	if (!h_font_novum)
		return -5;

	return 0;
}

////////////////////////////////////////////////////////////////////////////////
// centerDialog                                                               //
//                                                                            //
// Centers the help dialog so that it always appears in the middle of the     //
// main window. It determines where the dialog should be placed in terms of   //
// client coordinates on the main window, converts those to screen            //
// coordinates, and uses MoveWindow to position the dialog.                   //
////////////////////////////////////////////////////////////////////////////////

void centerDialog(HWND hDlg)
{
	HWND hwndParent = GetParent(hDlg);

	RECT wRectDlg;
	RECT rectParent;

	POINT ptDlg = {0, 0};

	GetWindowRect(hDlg, &wRectDlg);         // Screen coordinates
	GetClientRect(hwndParent, &rectParent);

	ptDlg.x = rectParent.right  / 2 - ((wRectDlg.right -  wRectDlg.left) / 2);
	ptDlg.y = rectParent.bottom / 2 - ((wRectDlg.bottom - wRectDlg.top)  / 2);
	
	ClientToScreen(hwndParent, &ptDlg);     /* MoveWindow uses
	                                           screen coordinates */

	MoveWindow(hDlg, ptDlg.x, ptDlg.y, wRectDlg.right - wRectDlg.left,
	           wRectDlg.bottom - wRectDlg.top, FALSE);
}

////////////////////////////////////////////////////////////////////////////////
// clearSrcBitmapPos                                                          //
//                                                                            //
// The src_bitmap_pos array in the state data structure stores integral       //
// values used to calculate offsets used when BitBlting the various switch    //
// location bitmaps to the final dash bitmap. This function resets the        //
// values in that array to zero, which is used to select the bitmaps that     //
// contain no switches.                                                       //
//                                                                            //
// This is used when the current view is "cleared", so an empty, "default"    //
// dash is displayed.                                                         //
//                                                                            //
// The first value in the array (at index 0) stores the offset for the zone 4 //
// panel. The default zone 4 panel is the 10-switch panel, which is           //
// represented with the value 3.                                              //
////////////////////////////////////////////////////////////////////////////////

void clearSrcBitmapPos(int* p_src_bitmap_pos)
{
	int i;

	if (!p_src_bitmap_pos)
		return;

	// Setting i = 1 is intentional
	for (i = 1; i < 14; i++)
		p_src_bitmap_pos[i] = 0;

	*p_src_bitmap_pos = 3;
}

////////////////////////////////////////////////////////////////////////////////
// getSrcBitmapPos                                                            //
//                                                                            //
// Populates the src_bitmap_pos array. This array of integers is a member of  //
// the state_data array declared as a static variable in the main window      //
// procedure. The array stores vertical offsets used to select the bitmaps    //
// when drawing the dash. This function uses the previously-populated         //
// switch list to determine these offsets.                                    //
//                                                                            //
// Each index of the array corresponds to section in the dash, each of which  //
// has a bitmap that contains all switch configurations. These bitmaps        //
// contain a column (or columns) of images, one row of which is selected when //
// drawing the dash. The value stored in each index is multiplied by the      //
// height of these sub-images within each bitmap when performing the BitBlt   //
// operations. This function generates the value stored in each index.        //
//                                                                            //
// To make the code simpler, the sub-images are stored in ascending order as  //
// if counting in binary, with the least significant bit on the left. For     //
// example, for a panel that contains a row of three switches, the topmost    //
// section of the bitmap would contain the image for no switches, the next    //
// section would show just the leftmost switch, then the middle switch, then  //
// the leftmost two switches, then just the third, etc etc...                 //
////////////////////////////////////////////////////////////////////////////////

int getSrcBitmapPos(LL* sw_list, int* p_src_bitmap_pos)
{
	LL_elem* p_sw_elem;

	UINT index = 0;
	UINT shift = 0;
	UINT position;

	if ((p_sw_elem = sw_list->head) == NULL)
		return -1;

	while (p_sw_elem) {
		position = ((SW_link*)(p_sw_elem->data))->loc - 1;

		// Last four switch positions are not continuous.
		// Switches jump from position 30 to position 35
		if (position > 29)
			position -= 4;

		if      (position <  2) { index = 5;  shift = 0;  }    // Group A
		else if (position <  4) { index = 6;  shift = 2;  }    // Group B
		else if (position < 10) { index = 1;  shift = 4;  }    // Group C
		else if (position < 15) { index = 2;  shift = 10; }    // Group D
		else if (position < 20) { index = 3;  shift = 15; }    // Group E
		else if (position < 22) { index = 7;  shift = 20; }    // Group F
		else if (position < 23) { index = 12; shift = 22; }    // Group G
		else if (position < 25) { index = 8;  shift = 23; }    // Group H
		else if (position < 27) { index = 9;  shift = 25; }    // Group I
		else if (position < 28) { index = 13; shift = 27; }    // Group J
		else if (position < 30) { index = 10; shift = 28; }    // Group K
		else if (position < 33) { index = 4;  shift = 30; }    // Group M
		else                    { index = 11; shift = 32; }    // Group L

		// The bitmaps for each section of the dash contain images of
		// switch configurations in ascending order counting in binary.
		// This allowed for the simple logic below to decide the offset
		// to use when BitBlting from the source bitmap for each
		// section of the dash to the destination.
		p_src_bitmap_pos[index] |= 1 << (position - shift);

		p_sw_elem = p_sw_elem->next;
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
// getFileInfo                                                                //
//                                                                            //
// This is a simple function that populates a OPENFILENAME structure, which   //
// windows uses with the GetOpenFileName function to open a file selection    //
// dialog.                                                                    //
//                                                                            //
// Possible future improvement: right now it is very barebones - it doesn't   //
// even tell the user what types of files are supported. It also doesn't      //
// support unicode characters in file paths.                                  //
////////////////////////////////////////////////////////////////////////////////

BOOL getFileInfo(HWND hwnd, OPENFILENAMEA* pofn, char* pFilePath,
                 int pathLength)
{
	if (pFilePath == NULL || pofn == NULL)
		return FALSE;
	*pFilePath = '\0';

	ZeroMemory(pofn, sizeof(OPENFILENAMEA));

	pofn->lStructSize       = sizeof(OPENFILENAMEA);
	pofn->hwndOwner         = hwnd;
	pofn->hInstance         = 0;
	pofn->lpstrFilter       = NULL;
	pofn->lpstrCustomFilter = NULL;
	pofn->nMaxCustFilter    = 0;
	pofn->nFilterIndex      = 0;
	pofn->lpstrFile         = pFilePath;
	pofn->nMaxFile          = pathLength;
	pofn->lpstrFileTitle    = NULL;
	pofn->nMaxFileTitle     = 0;
	pofn->lpstrInitialDir   = NULL;
	pofn->lpstrTitle        = NULL;
	pofn->Flags             = 0;
	pofn->nFileOffset       = 0;
	pofn->nFileExtension    = 0;
	pofn->lpstrDefExt       = NULL;
	pofn->lCustData         = 0;
	pofn->lpfnHook          = NULL;
	pofn->lpTemplateName    = NULL;
	pofn->FlagsEx           = 0;

	return GetOpenFileNameA(pofn);
}

////////////////////////////////////////////////////////////////////////////////
// loadBitmaps                                                                //
//                                                                            //
// Loads the bitmap resources into memory. If an error occurs while loading   //
// the bitmaps, the program is aborted.                                       //
////////////////////////////////////////////////////////////////////////////////

int loadBitmaps(HINSTANCE hInstance, P_SW_BITMAP p_sw_bitmap, int num_bitmaps)
{
	int i = 0;

	for (i = 0; i < num_bitmaps - 1; i++) {
		p_sw_bitmap[i].h_bitmap = LoadBitmapA(hInstance, p_sw_bitmap[i].str);
		if (!(p_sw_bitmap[i].h_bitmap)) {
			destroyBitmaps(p_sw_bitmap, num_bitmaps);
			return -(i + 1);
		}
	}

	// LoadImage was used for the hatch mark bitmap because it's monochrome.
	// When I used LoadBitmap instead, BitBlting the hatch mark bitmap
	// into a memory DC did not result in the hatch marks using the text
	// color of that DC. Even though the hatch mark is saved monochrome bitmap,
	// it was treated as a color bitmap when using LoadBitmap. When using
	// LoadImage with the LR_MONOCHROME argument, the system recognized it as a
	// monochrome bitmap.
	if ((p_sw_bitmap[num_bitmaps - 1].h_bitmap =
		LoadImageA(hInstance, p_sw_bitmap[num_bitmaps - 1].str,
		           IMAGE_BITMAP, 0, 0, LR_MONOCHROME)) == NULL)
	{
		destroyBitmaps(p_sw_bitmap, num_bitmaps);
		return -(num_bitmaps - 1);
	}

	return 0;
}

////////////////////////////////////////////////////////////////////////////////
// destroyBitmaps                                                             //
//                                                                            //
// Frees the bitmaps from memory. It's only called when exiting the program,  //
// whether that's because an error occurred or the program exited normally.   //
////////////////////////////////////////////////////////////////////////////////

void destroyBitmaps(P_SW_BITMAP p_sw_bitmap, int num_bitmaps)
{
	int i;

	for (i = 0; i < num_bitmaps; i++)
		if (p_sw_bitmap[i].h_bitmap)
			DeleteObject(p_sw_bitmap[i].h_bitmap);
}

////////////////////////////////////////////////////////////////////////////////
// createMemoryDCs                                                            //
//                                                                            //
// Create the memory device contexts that the bitmaps are selected into. If   //
// an error occurs, the program is aborted.                                   //
////////////////////////////////////////////////////////////////////////////////

int createMemoryDCs(HDC hdc, P_SW_BITMAP p_sw_bitmap, int num_dcs)
{
	for (int i = 0; i < num_dcs; i++)
		if ((p_sw_bitmap[i].hdc_mem = CreateCompatibleDC(hdc)) == NULL) {
			deleteMemoryDCs(p_sw_bitmap, num_dcs);
			return -1;
		}

	return 0;
}

////////////////////////////////////////////////////////////////////////////////
// deleteMemoryDCs                                                            //
//                                                                            //
// Deletes the memory DCs created in createMemoryDCs(). It's only called      //
// when exiting the program, whether that's because an error occurred or the  //
// program exited normally.                                                   //
////////////////////////////////////////////////////////////////////////////////

void deleteMemoryDCs(P_SW_BITMAP p_sw_bitmap, int num_dcs)
{
	int i;

	// The hdc_mem member of the p_sw_bitmap structure is NULL when initialized
	// and is assigned NULL if an error occurred when creating the memory DC in
	// the createMemoryDCs function, so DeleteDC is only called on valid DCs.
	// There is not much information provided for DeleteDC in the offical
	// documentation. It may be perfectly fine to call DeleteDC with a NULL
	// argument, which means checking for NULL as I did here is unnecessary.
	for (i = 0; i < num_dcs; i++)
		if (p_sw_bitmap[i].hdc_mem)
			DeleteDC(p_sw_bitmap[i].hdc_mem);
}

////////////////////////////////////////////////////////////////////////////////
// selectBitmaps                                                              //
//                                                                            //
// Once the bitmap resources are loaded into memory and their memory DCs      //
// have been created, the bitmaps need to be selected into their memory DCs.  //
// If an error occurs, the program is aborted.                                //
////////////////////////////////////////////////////////////////////////////////

int selectBitmaps(P_SW_BITMAP p_sw_bitmap, int num_bitmaps)
{
	HGDIOBJ hobj;

	for (int i = 0; i < num_bitmaps; i++) {
		hobj = SelectObject(p_sw_bitmap[i].hdc_mem,
				    p_sw_bitmap[i].h_bitmap);

		if (hobj == NULL)
			return -1;
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
// getSwPanel                                                                 //
//                                                                            //
// The trucks have four different options for the rightmost switch panel      //
// (also referred to as the zone-4 panel):                                    //
// 0: Cubby (no switches)                                                     //
// 1: 2-switch panel                                                          //
// 2: 6-switch panel                                                          //
// 3: 10-switch panel                                                         //
//                                                                            //
// This function determines which one of the zone-4 panels the spec has. The  //
// program needs to know this so it can draw the correct panel, and also      //
// detect whether the spec is calling for a switch that will have no          //
// location because of the panel selected.                                    //
//                                                                            //
// In the state data structure declared as a static variable in the main      //
// window procedure, there is an array member call src_bitmap_pos that        //
// stores the position of each bitmap used to draw the dash. By 'position'    //
// I mean an integral value multiplied by the vertical dimension of each      //
// bitmap section. The first member of this array is where the getSwPanel     //
// return value is stored.                                                    //
////////////////////////////////////////////////////////////////////////////////

int getSwPanel(struct variant* var_list, int num_var)
{
	int i;

	for (i = 0; i < num_var; i++) {
		if (strstr(var_list[i].idvar6, "W7D")) {
			if (strchr(var_list[i].symbol, '0'))
				return 3;
			else if (strchr(var_list[i].symbol, '6'))
				return 2;
			else if (strchr(var_list[i].symbol, '2'))
				return 1;
			else
				return 0;
		}
	}

	// The variant that describes the panel (part of the W7D family) should
	// be in every spec. Regardless, use the 10-switch panel as a default
	// incase the W7D isn't found.
	return 3;
}

////////////////////////////////////////////////////////////////////////////////
// createChildWindows                                                         //
//                                                                            //
// This function handles the WNDCLASS structure definition, RegisterClass     //
// call, and CreateWindow call for each of the 'main' child windows.          //
// The main intent of this function is to remove clutter from the WM_CREATE   //
// message handling in the main window procedure.                             //
//                                                                            //
// This program is organized into three child windows: the banner, list view, //
// and cab view windows. The banner is the horizontal section at the top      //
// containing the program logo, edit control and buttons. The list view       //
// window is a columnar window on the left which displays a list of switches  //
// in a spec. The cab view window is to the right of the list view window,    //
// and displays the dash.                                                     //
//                                                                            //
// If an error occurs, the program is aborted.                                //
////////////////////////////////////////////////////////////////////////////////

int createChildWindows
(HINSTANCE h_instance, HWND hwnd_parent, HWND* p_hwnd_banner,
 HWND* p_hwnd_list_view, HWND* p_hwnd_cab_view, P_STATE_DATA p_state_data)
{
	////////////////////////////////////////////////////////////////
	// Create banner child window

	WNDCLASSA banner_class = { 0 };
	banner_class.style = 0;
	banner_class.lpfnWndProc = bannerProc;
	banner_class.cbClsExtra = 0;
	banner_class.cbWndExtra = 0;
	banner_class.hInstance = h_instance;
	banner_class.hIcon = NULL;
	banner_class.hCursor = LoadCursor(NULL, IDC_ARROW);
	banner_class.hbrBackground = GetStockObject(WHITE_BRUSH);
	banner_class.lpszMenuName = NULL;
	banner_class.lpszClassName = "banner_class";

	RegisterClassA(&banner_class);

	*p_hwnd_banner =
	CreateWindowA("banner_class", NULL, WS_CHILD | WS_VISIBLE,
	              0, 0, 0, 0, hwnd_parent, NULL, h_instance, p_state_data);

	if (!(*p_hwnd_banner))
		return -1;

	////////////////////////////////////////////////////////////////
	// Create list child window

	WNDCLASSA list_view_class = { 0 };
	list_view_class.style = 0;
	list_view_class.lpfnWndProc = listViewProc;
	list_view_class.cbClsExtra = 0;
	list_view_class.cbWndExtra = 0;
	list_view_class.hInstance = h_instance;
	list_view_class.hIcon = NULL;
	list_view_class.hCursor = LoadCursor(NULL, IDC_ARROW);
	list_view_class.hbrBackground = GetStockObject(WHITE_BRUSH);
	list_view_class.lpszMenuName = NULL;
	list_view_class.lpszClassName = "list_view_class";

	RegisterClassA(&list_view_class);

	*p_hwnd_list_view =
	CreateWindowA("list_view_class", NULL, WS_CHILD | WS_VISIBLE,
	              0, 0, 0, 0, hwnd_parent, NULL, h_instance, p_state_data);

	if (!(*p_hwnd_list_view))
		return -2;

	////////////////////////////////////////////////////////////////
	// Create cab view child window

	WNDCLASSA cab_view_class = { 0 };
	cab_view_class.style = 0;
	cab_view_class.lpfnWndProc = cabViewProc;
	cab_view_class.cbClsExtra = 0;
	cab_view_class.cbWndExtra = 0;
	cab_view_class.hInstance = h_instance;
	cab_view_class.hIcon = NULL;
	cab_view_class.hCursor = LoadCursor(NULL, IDC_ARROW);
	cab_view_class.hbrBackground = GetStockObject(WHITE_BRUSH);
	cab_view_class.lpszMenuName = NULL;
	cab_view_class.lpszClassName = "cab_view_class";

	RegisterClassA(&cab_view_class);

	*p_hwnd_cab_view =
	CreateWindowA("cab_view_class", NULL, WS_CHILD | WS_VISIBLE,
	              0, 0, 0, 0, hwnd_parent, NULL, h_instance, p_state_data);

	if (!(*p_hwnd_cab_view))
		return -3;

	return 0;
}

////////////////////////////////////////////////////////////////////////////////
// getHighlightPos                                                            //
//                                                                            //
// When a user clicks on a switch in the list view, the program draws a       //
// retangle around the selected switch in the cab view. In this program, that //
// is referred to a 'highlight', or 'highlight rectangle'. In order to do     //
// this, it needs to know the position in the dash that the clicked switch    //
// occupies. That position is what this function determines.                  //
//                                                                            //
// The switches are displayed with a list box control, in the same order they //
// were added to the linked list of switches. This function takes the index   //
// of the selected switch in the list box, traverses the linked list <index>  //
// number of times, and returns the location of the switch.                   //
//                                                                            //
// If an error occurs, the previous highlight rectangle is cleared, and a     //
// new one is not drawn.                                                      //
////////////////////////////////////////////////////////////////////////////////

int getHighlightPos(LL* sw_list, int index)
{
	LL_elem* p_temp_elem;
	int i;

	if (!sw_list)
		return -1;

	p_temp_elem = sw_list->head;
	if (!p_temp_elem)
		return -2;

	for (i = 0; i < index; i++) {
		p_temp_elem = p_temp_elem->next;

		if (!p_temp_elem)
			return -3;
	}

	return ((SW_link*)(p_temp_elem->data))->loc;
}

////////////////////////////////////////////////////////////////////////////////
// notifyConflicts                                                            //
//                                                                            //
// It is possible for a spec to contain codes that call for two or more       //
// switches to be in the same location. This function traverses a completed   //
// list and identifies those conflicts. Since the switches are inserted into  //
// the linked list in ascending order (of location), this is trivial to do.   //
//                                                                            //
// The function calls MessageBox before the dash is drawn to alert the user   //
// of the conflict. The message box displays the variants used to call out    //
// each switch, so the user knows where to start when determining what to     //
// change. A separate message box is presented for every conflicting          //
// pair in the list as they are encountered.                                  //
//                                                                            //
// Possible future improvement: Use one message box or dialog window, instead //
// of one message box for each conflicting pair.                              //
////////////////////////////////////////////////////////////////////////////////

static void notifyConflicts(LL* sw_list)
{
	LL_elem* sw_link;
	char buf[1024] = { 0 };
	char fmt[] = "Error: multiple switches in location %d!\n\n"
	             "Switch #1: %s\n" "Variant string: %s\n\n"
	             "Switch #2: %s\n" "Variant string: %s";

	char sw_1_desc[50] = { 0 };
	char sw_2_desc[50] = { 0 };

	strcpy_s(buf, 1024, fmt);

	if (!sw_list)
		return;

	sw_link = sw_list->head;

	while (sw_link) {
		if (!(sw_link->next))
			return;
		if (((struct sw_link*)(sw_link->data))->loc ==
			((struct sw_link*)(sw_link->next->data))->loc) {

			// Get description of each switch
			getSWDesc(sw_1_desc, 50, ((struct sw_link*)(sw_link->data))->pn);
			getSWDesc(sw_2_desc, 50,
					  ((struct sw_link*)(sw_link->next->data))->pn);

			wsprintfA(buf, fmt, ((struct sw_link*)(sw_link->data))->loc,
					  sw_1_desc,
					  ((struct sw_link*)(sw_link->data))->vars,
					  sw_2_desc,
					  ((struct sw_link*)(sw_link->next->data))->vars);

			MessageBoxA(NULL, buf, "Switch Conflict", MB_ICONERROR);
		}
		sw_link = sw_link->next;
	}
}

////////////////////////////////////////////////////////////////////////////////
// notifyPanel                                                                //
//                                                                            //
// It is possible for a spec to contain codes that call for switches in       //
// zone 4 while having a zone 4 panel that doesn't contain locations for      //
// these switches. This function traverses a completed list and identifies    //
// those conflicts. Zone 4 is comprised of switch locations 21-30. If the     //
// list calls for a switch in this position, and the zone 4 panel is not      //
// suitable, this function calls MessageBox for each incompatibility          //
// before the dash is drawn to alert the user of the conflict.                //
//                                                                            //
// Like the notifyConflicts() function, this function displays the variants   //
// used to call out each switch, so the user knows what to change.            //
//                                                                            //
// Possible future improvement: Use one message box or dialog window, instead //
// of one message box for each conflict.                                      //
////////////////////////////////////////////////////////////////////////////////

static void notifyPanel(LL* sw_list, unsigned panel)
{
	LL_elem* sw_link;

	char buf[1024] = { 0 };
	char fmt[] = "Error: Zone 4 panel conflict!\n"
	             "Switch cannot be placed in loc %d.\n\n"
	             "Switch: %s\n" "Variant string: %s\n"
	             "Zone 4 Panel: %s\n";

	char sw_desc[50] = { 0 };
	char sz_panel[9] = { 0 };

	if (!sw_list)
		return;

	if (panel > 2)        // 3 signifies the full 10-switch panel
		return;

	strcpy_s(buf, 1024, fmt);

	switch (panel) {
	case 0:  strcpy_s(sz_panel, 9, "UADASWPA"); break;
	case 1:  strcpy_s(sz_panel, 9, "ADASWP2");  break;
	case 2:  strcpy_s(sz_panel, 9, "ADASWP6");  break;
	default: strcpy_s(sz_panel, 9, "?");        break;
	}

	sw_link = sw_list->head;

	while (sw_link) {
		int loc = ((struct sw_link*)(sw_link->data))->loc;

		switch (loc) {
		case 21: case 22: case 26: case 27: break;   // always conflict
		case 24: case 25: case 29: case 30:
			if (panel == 2) {
				sw_link = sw_link->next;
				continue;
			}
			break;
		case 23: case 28:
			if (panel != 0) {
				sw_link = sw_link->next;
				continue;
			}
			break;
		default:
			sw_link = sw_link->next;
			continue;
		}

		// Get description of switch
		getSWDesc(sw_desc, 50, ((struct sw_link*)(sw_link->data))->pn);

		wsprintfA(buf, fmt, loc, sw_desc,
					 ((struct sw_link*)(sw_link->data))->vars, sz_panel);

		MessageBoxA(NULL, buf, "Switch Conflict", MB_ICONERROR);

		sw_link = sw_link->next;
	}
}