////////////////////////////////////////////////////////////////////////////////
// banner.c                                                                   //
//                                                                            //
// This TU contains data and functions used by the banner child window, one   //
// of three child windows to the main parent window. The banner is drawn at   //
// the top of the application client area. It contains the program logo, a    //
// search box to enter a VSS number for query, and four buttons. The banner   //
// occupies the full width of the client area.                                //
//                                                                            //
// The logo consists of a small bitmap of a Volvo truck, and the program name //
// written in Broad Pro font, a Volvo brand font. These resources are         //
// available for download from Volvo's website. Because the Broad Pro font is //
// only used for the title, it was simpler to draw it as a pre-generated      //
// bitmap  instead of creating a font and using one of the GDI text           //
// functions to draw it.                                                      //
//                                                                            //
// The edit control is based on the built-in "edit" window class. A simple    //
// subclass procedure handles processing for the enter key and alerts the     //
// parent window (the banner window) when it's losing focus. The edit         //
// control features a bitmap of a magnifying glass as a clue for users that   //
// it's used for searching. It responds to mouse hovering by shading the      //
// enclosing rectangle, as well as the magnifying glass, blue. It also        //
// checks the user input to ensure the VSS number entered is valid.           //
//                                                                            //
// The arrow button next to the edit control is used to submit a VSS number   //
// to the application. The arrow button is drawn with a subtle gray           //
// background to indicate that the input does not represent a valid           //
// VSS number, which disables the button. When the application detects that   //
// the input comprises a valid VSS number, the gray square is removed. If     //
// the button is enabled, the arrow turns blue when the mouse hovers over     //
// it, and is offset during a click. While the edit control has the input     //
// focus, the enter key simulates a mouse click.                              //
//                                                                            //
// The other three buttons are more similar. They are always enabled. They    //
// draw their text in blue while the mouse cursor is hovering over them, and  //
// their text and icons are offset during a click. The buttons provide the    //
// following functions: clearing the screen, opening a file with a spec, and  //
// opening the Help dialog.                                                   //
////////////////////////////////////////////////////////////////////////////////

#include <windowsx.h>       // GET_X_LPARAM & GET_X_LPARAM macros
#include "banner.h"
#include "ost_shared.h"

// State for the four buttons (mouse hovering and clicking)
static struct button_state button_states[4];

// Variable that stores the enabled / disabled arrow button state
static BOOL arrow_enabled;

static char* btn_text[] = {
	"Clear",
	"Open File",
	"Help",
	"",      // Arrow button has no text
};

// Sublass window procedure for the VSS number entry edit control
static WNDPROC oldEditProc;

LRESULT CALLBACK bannerProc(HWND hwnd, UINT message, WPARAM wParam,
                            LPARAM lParam)
{
	static P_STATE_DATA p_data;

	static HWND hwnd_edit;
	static HWND hwnd_arrow;    // handle to the arrow button

	static RECT edit_rect = { 448, 12, 704, 60 };
	static BOOL f_in_edit = FALSE;

	static int x_banner, y_banner;

	static struct vss_search_info vsi = { 0 };

	POINT cursor_pos = { 0 };

	PAINTSTRUCT ps;
	HDC hdc;

	// For WM_DRAWITEM processing
	HDC hdc_btn;
	RECT rect_btn;
	HFONT h_font;
	HDC hdc_mem;

	switch (message) {
	case WM_CREATE:
		SetWindowLongPtrA(hwnd, GWLP_USERDATA,
		                  (LONG_PTR)((LPCREATESTRUCT)lParam)->lpCreateParams);

		p_data = (P_STATE_DATA)GetWindowLongPtrA(hwnd, GWLP_USERDATA);

		// Create banner push buttons
		createVSSButton(hwnd, NULL, 999, 12, 116, 48, BTN_ID_CLEAR,
		                &(button_states[0]));

		createVSSButton(hwnd, NULL, 1131, 12, 162, 48, BTN_ID_FILE,
		                &(button_states[1]));

		createVSSButton(hwnd, NULL, 1309, 12, 112, 48, BTN_ID_HELP,
		                &(button_states[2]));

		// Create edit control
		hwnd_edit =
		CreateWindowA("edit", NULL, WS_CHILD | WS_VISIBLE | ES_UPPERCASE,
		              496, 20, 206, 34, hwnd, (HMENU)EDIT_ID,
		              (HINSTANCE)GetWindowLongPtrA(hwnd, GWLP_HINSTANCE), NULL);

		// Subclass edit control
		oldEditProc =
		(WNDPROC)SetWindowLongPtrA(hwnd_edit, GWLP_WNDPROC,
		                           (LONG_PTR)vssEditProc);

		SendMessageA(hwnd_edit, EM_SETLIMITTEXT, 13, 0);
		SendMessageA(hwnd_edit, WM_SETFONT, (WPARAM)p_data->h_font_title, TRUE);

		hwnd_arrow =
		createVSSButton(hwnd, NULL, 712, 16, 40, 40, BTN_ID_ARROW,
		                &(button_states[3]));

		return 0;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {  // Control ID
		case BTN_ID_CLEAR:
			SetWindowTextA(hwnd_edit, NULL);  // fall through
		case BTN_ID_FILE:
		case BTN_ID_HELP:
			SendMessageA(GetParent(hwnd), WM_COMMAND, wParam, lParam);
			break;

		case BTN_ID_ARROW:
			if (arrow_enabled) {

				// Store VSS # from edit control into buf_vss
				if (!getVssFromEdit(hwnd_edit, vsi.vss_num, 14)) {
					MessageBoxA(NULL, "Error executing getVssFromEdit",
						"Error", MB_ICONERROR);
					break;
				}

				// Store EDB URL into buf_url
				if (genVssURL(vsi.url, vsi.vss_num, 200) < 0) {
					MessageBoxA(NULL, "Error executing genVssURL",
						"Error", MB_ICONERROR);
					break;
				}

				// Disable search button
				arrow_enabled = FALSE;
				InvalidateRect(hwnd_arrow, NULL, FALSE);

				// Without this UpdateWindow call, the arrow button would
				// not be redrawn until the spec retrieval had completed
				UpdateWindow(hwnd_arrow);

				// Initiate spec retrieval
				SendMessageA(GetParent(hwnd), WM_COMMAND,
				             MAKEWPARAM(BTN_ID_ARROW, 0), (LPARAM)&vsi);

				// Clear edit box and restore cusor to arrow (it was set to
				// the wait animation in the WM_COMMAND processing in ostool.c)
				SetWindowTextA(hwnd_edit, NULL);
				SetCursor(LoadCursorA(NULL, IDC_ARROW));
			}
			break;

		case EDIT_ID:
			switch (HIWORD(wParam)) {
			case EN_CHANGE: {
				BOOL old_arr_en = arrow_enabled;
				size_t len = 0;

				ZeroMemory(vsi.vss_num, 14);

				*(vsi.vss_num) = 14;

				SendMessageA(hwnd_edit, EM_GETLINE, 0, (LPARAM)vsi.vss_num);

				// EM_GETLINE does not append a null character
				*(vsi.vss_num + 13) = '\0';
				len = strlen(vsi.vss_num);
				arrow_enabled = (checkInput(vsi.vss_num, len) < 0 ? FALSE : TRUE);

				if (arrow_enabled != old_arr_en)
					InvalidateRect(hwnd_arrow, NULL, FALSE);
				break;
			}
			}
		}
		return 0;

	case WM_SETFOCUSEDIT:
		if (wParam)
			SetFocus(hwnd_edit);
		InvalidateRect(hwnd, &edit_rect, FALSE);
		return 0;

	case WM_MOUSEMOVE:
		cursor_pos.x = GET_X_LPARAM(lParam);
		cursor_pos.y = GET_Y_LPARAM(lParam);

		if (f_in_edit != PtInRect(&edit_rect, cursor_pos)) {
			f_in_edit = PtInRect(&edit_rect, cursor_pos);
			InvalidateRect(hwnd, &edit_rect, FALSE);
		}
		return 0;

	case WM_SETCURSOR:
		if (f_in_edit)
			SetCursor(LoadCursorA(NULL, IDC_IBEAM));
		else
			SetCursor(LoadCursorA(NULL, IDC_ARROW));

		return 0;

	case WM_LBUTTONDOWN:
		if (f_in_edit)
			SetFocus(hwnd_edit);

		return 0;

	case WM_SIZE:

		// Retrieves height / width of banner window. This is
		// used in WM_PAINT processing below.

		x_banner = LOWORD(lParam);
		y_banner = HIWORD(lParam);
		return 0;

	case WM_PAINT:
		hdc = BeginPaint(hwnd, &ps);

		// Draw truck and title bitmaps
		drawTitle(hdc, p_data->p_bitmaps + 7, p_data->p_bitmaps + 8);

		// Draw edit control border
		drawEditRect(hwnd_edit, hdc, (p_data->p_bitmaps + 9)->hdc_mem,
		             &edit_rect, f_in_edit);

		// Draw blue hatch marks under title bar rectangle
		P_SW_BITMAP p_bitmap_hatch = p_data->p_bitmaps +
		                             p_data->num_bitmaps - 1;

		paintHatchLines(hdc, p_bitmap_hatch, 0, 72, x_banner, 88,
		                RGB(0, 86, 214));

		ReleaseDC(hwnd, hdc);
		return 0;

	case WM_DRAWITEM: {
		hdc_btn = ((LPDRAWITEMSTRUCT)lParam)->hDC;
		rect_btn = ((LPDRAWITEMSTRUCT)lParam)->rcItem;
		h_font = p_data->h_font_title;

		// wParam is the ID of the control. BTN_ID_CLEAR is the lowest ID
		// of a group of controls whose states are stored in an array
		// called button_states. This statement calculates the index into
		// the array that holds the state for the given ID.
		WORD x = (WORD)wParam - BTN_ID_CLEAR;

		if (wParam == BTN_ID_ARROW) {
			hdc_mem = (p_data->p_bitmaps + 11)->hdc_mem;
			drawArrowButton(hdc_btn, hdc_mem, &(button_states[x]), arrow_enabled);
		}
		else {
			hdc_mem = (p_data->p_bitmaps + 9)->hdc_mem;
			drawButtonRect(hdc_btn, &rect_btn);

			// this wParam is an ID and fits in a WORD
			drawBtnTogText(hdc_btn, h_font, btn_text[x], &rect_btn,
			               &(button_states[x]), (WORD)wParam);
			drawButtonBitmap(hdc_btn, hdc_mem, &(button_states[x]), (WORD)wParam);
		}
		return 0;
	}

	}
	return DefWindowProcA(hwnd, message, wParam, lParam);
}

////////////////////////////////////////////////////////////////////////////////
// drawTitle                                                                  //
//                                                                            //
// The title consists of a small bitmap logo of a Volvo truck and the title   //
// of the program drawn in the Broad Pro font. This function performs both of //
// the BitBlt operations to draw these items.                                 //
////////////////////////////////////////////////////////////////////////////////

void drawTitle(HDC hdc, P_SW_BITMAP p_bitmap_truck, P_SW_BITMAP p_bitmap_title)
{
	BITMAP bm = { 0 };
	GetObjectA(p_bitmap_truck->h_bitmap, sizeof(BITMAP), &bm);
	BitBlt(hdc, 8, 4, bm.bmWidth, bm.bmHeight,
	       p_bitmap_truck->hdc_mem, 0, 0, SRCCOPY);
	GetObjectA(p_bitmap_title->h_bitmap, sizeof(BITMAP), &bm);
	BitBlt(hdc, 64, 0, bm.bmWidth, bm.bmHeight,
	       p_bitmap_title->hdc_mem, 0, 0, SRCCOPY);
}

////////////////////////////////////////////////////////////////////////////////
// checkInput                                                                 //
//                                                                            //
// Every time input changes in the edit control, this function checks the     //
// input to determine whether it comprises a valid VSS number. A valid VSS    //
// number has one of the following formats:                                   //
//                                                                            //
//     Six-digit suffix:  VSS-##-######   - or -                              //
//     Five-digit suffix: VSS-##-#####                                        //
//                                                                            //
// If the input matches one of these formats (including the dash marks), the  //
// arrow button next to the edit control is enabled and the input may be      //
// submitted. Otherwise the button is disabled.                               //
//                                                                            //
// Possible future improvement: allow lowercase vss prefix characters, and    //
// automatically enter dash marks when typing in a number. This was not       //
// prioritized because most of the time, users are pasting the VSS number     //
// into the edit control, and it already includes the dashes.                 //
////////////////////////////////////////////////////////////////////////////////

int checkInput(const char* input, size_t length)
{
	// Input must be in the following format:

	//     As a VSS number: VSS-##-######
	//          - or -    : VSS-##-#####

	int i;

	if (input == NULL)
			return -1;
	if (length < 12 || length > 13)
			return -2;

	// Edit control forces caps with ES_UPPERCASE
	if (*input != 'V')
			return -3;
	if (*(input + 1) != 'S')
			return -4;
	if (*(input + 2) != 'S')
			return -5;
	if (*(input + 3) != '-')
			return -6;

	if (!isdigit(*(input + 4)))
			return -7;
	if (!isdigit(*(input + 5)))
			return -8;

	if (*(input + 6) != '-')
			return -9;

	// String buffer is between 12 and 13 characters in length (see 2nd
	// if statement above). This means the array will be indexed from
	// 0 - 11 or 0 - 12: 0 - 11 for five digits in the tertiary part, and
	// 0 - 12 for six digits in the tertiary part (the maximum possible).
	for (i = 7; i < length; i++) {
		if (!isdigit(*(input + i)))
			return -(i + 3);
	}

	return 0;
}

////////////////////////////////////////////////////////////////////////////////
// drawArrowButton                                                            //
//                                                                            //
// Draws the arrow button, which is different depending on whether it is      //
// enabled, and whether the cursor is hovering over and and it's being        //
// clicked.                                                                   //
//                                                                            //
// If the button is disabled, it's drawn with a subtle gray background. If    //
// it's enabled, the gray background is removed and the arrow is slightly     //
// darker gray. If the cursor is hovering over the button, the arrow is       //
// drawn blue. While being clicked, the blue arrow is slightly offset to      //
// provide the user with visual feedback.                                     //
//                                                                            //
// The different button backgrounds are different sections of a bitmap which  //
// are BitBlt'ed to the button client area.                                   //
////////////////////////////////////////////////////////////////////////////////

void drawArrowButton(HDC hdc_btn, HDC hdc_mem,
                     const struct button_state* p_state, BOOL f_enabled)
{
	int y_pos_bm = 0;

	int x_offset = 0;
	int y_offset = 0;

	if (!f_enabled) {
		BitBlt(hdc_btn, 0, 0, 40, 40, hdc_mem, 0, 0, SRCCOPY);
		return;
	}

	if (p_state->clicking) {
		y_pos_bm = 80;
		x_offset = 1;
		y_offset = 2;
	}
	else if (p_state->hovering) {
		y_pos_bm = 80;
	}
	else
		y_pos_bm = 40;

	BitBlt(hdc_btn, x_offset, y_offset, 40, 40, hdc_mem, 0, y_pos_bm, SRCCOPY);
}

////////////////////////////////////////////////////////////////////////////////
// drawEditRect                                                               //
//                                                                            //
// Draws the bounding box of the edit control. The bounding box, or           //
// rectangle, encloses the edit control as well as the magnifying glass       //
// icon, which is used to signal to the user that the control is meant for    //
// input that will be searched.                                               //
//                                                                            //
// The rectangle is not the area of the edit control. The edit control        //
// area is slightly smaller and offset to the right to make room for the      //
// icon. The coordinates for the four points of this rectangle are stored in  //
// a static RECT variable called edit_rect. During WM_MOUSEMOVE processing in //
// the window procedure for the banner window, the application checks if the  //
// pointer is located within this rectangle. If it is, the static BOOL        //
// variable f_in_edit is set to TRUE, and the rectangle is colored blue.      //
// Otherwise, it's drawn in gray.                                             //
//                                                                            //
// The rectangle is also drawn blue if the edit control has the input focus.  //
// In the WM_LBUTTONDOWN processing in the banner window procedure, the edit  //
// control gains input focus if the user clicks inside the rectangle.         //
////////////////////////////////////////////////////////////////////////////////

void drawEditRect(HWND hwnd, HDC hdc, HDC hdc_mem, LPRECT p_rect,
                  BOOL f_in_edit)
{
	HBRUSH h_brush;

	if (f_in_edit || (hwnd == GetFocus()))
		h_brush = CreateSolidBrush(CTA_BLUE);
	else
		h_brush = CreateSolidBrush(GRAY);

	FrameRect(hdc, p_rect, h_brush);
	DeleteObject(h_brush);

	BitBlt(hdc, 460, 24, 24, 24, hdc_mem, f_in_edit ? 24 : 0, 0, SRCCOPY);
}

////////////////////////////////////////////////////////////////////////////////
// getVssFromEdit                                                             //
//                                                                            //
// Retrieves the VSS number from the edit control and stores it into a        //
// buffer, a pointer to which is passed as a parameter to this function. The  //
// buffer must be 14 characters in length to hold all valid VSS numbers.      //
////////////////////////////////////////////////////////////////////////////////

int getVssFromEdit(HWND hwnd_edit, char* buf_vss, unsigned buf_size)
{
	if (!buf_vss)
		return -1;
	if (buf_size < 14)
		return -2;
	if (buf_size > 127)
		return -3;

	ZeroMemory(buf_vss, buf_size);

	// Setting this member to size of array is necessary for EM_GETLINE
	*buf_vss = (char)buf_size;

	// This call will not return a number greater than 14, so it's safe to
	// cast to an int
	return (int)SendMessageA(hwnd_edit, EM_GETLINE, 0, (LPARAM)buf_vss);
}

////////////////////////////////////////////////////////////////////////////////
// vssEditProc                                                                //
//                                                                            //
// This subclass procedure processes messages sent to the Edit control used   //
// to enter VSS numbers. If the user presses the enter key while the edit     //
// control has the input focus, this window procedure emulates a mouse click  //
// on the arrow button. It also alerts the banner window procedure if the     //
// edit control loses input focus to ensure the bounding box is drawn         //
// properly.                                                                  //
//                                                                            //
// All other messages are passed to the default Edit window procedure.        //
////////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK vssEditProc
(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_CHAR:
		switch (wParam) {
		case VK_RETURN:
			SendMessageA(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(BTN_ID_ARROW, 0),
							(LPARAM)hwnd);
			return 0;
		}
		break;

	case WM_KILLFOCUS:
		SendMessageA(GetParent(hwnd), WM_SETFOCUSEDIT, 0, 0);
		break; // break here to get default processing (kill caret)
	}
	return CallWindowProcA(oldEditProc, hwnd, message, wParam, lParam);
}

////////////////////////////////////////////////////////////////////////////////
// genVssURL                                                                  //
//                                                                            //
// This application can retrieve a truck spec over the internet from a        //
// database. The URL for a spec is generated from a VSS number. This function //
// extracts the portions of the VSS number required to generate the URL. The  //
// middle two digits, and trailing five/six digits are these two pieces.      //
////////////////////////////////////////////////////////////////////////////////

int genVssURL(char* dest, const char* src, int dest_size)
{
	// src format: VSS-12-123456
	//   - or -  : VSS-12-12345

	// Input has already been verified by checkInput()

	char p1[] = "https://edb.volvo.net/cgi-bin/wis2/vehspec5.cgi?func=1&svariants=&pkl=04&f1=VSS&f2=";
	char p2[] = "&f3=";
	char p3[] = "&funcflag=0&varfam=1&limitfunc=&onlykola=";

	char f2[3] = { 0 };
	char f3[7] = { 0 };

	int i = 0;

	// Make sure dest buffer has enough space
	// 2 = f2, 6 = f3, 1 = terminating null
	if ((lstrlenA(p1) + 2 + lstrlenA(p2) + 6 + lstrlenA(p3) + 1) > dest_size)
		return -1;

	// Retrieve middle two digits from VSS number and store them into f2
	f2[0] = *(src + 4);
	f2[1] = *(src + 5);
	// f2[2] = '\0';

	for (i = 7; i < lstrlenA(src); i++)
		f3[i - 7] = *(src + i);

	if (!lstrcpyA(dest, p1))
		return -2;
	if (!lstrcatA(dest, f2))
		return -3;
	if (!lstrcatA(dest, p2))
		return -4;
	if (!lstrcatA(dest, f3))
		return -5;
	if (!lstrcatA(dest, p3))
		return -6;

	return lstrlenA(dest);
}