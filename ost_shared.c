////////////////////////////////////////////////////////////////////////////////
// ost_shared.c                                                               //
//                                                                            //
// This TU contains functions used by multiple child windows of the main      //
// window (the banner, list view, and cab view windows). It contains other    //
// 'helper' functions as well, that aren't shared between multiple child      //
// windows.                                                                   //
//                                                                            //
// Most of these functions involve drawing and other processing required for  //
// buttons. There are two different kinds of buttons in this program:         //
//                                                                            //
// 1: Regular push buttons                                                    //
// 2: Toggle buttons                                                          //
//                                                                            //
// There are four push buttons (one without text used to submit a VSS number  //
// for query, and three others with text used to clear the screen, open a     //
// file, and open the help dialog). There is one toggle button, used to show  //
// and hide the legend, which displays location numbers for each switch in    //
// the dash currently drawn on the screen.                                    //
//                                                                            //
// The window procedure for the default button class was subclassed so I      //
// could track the mouse pointer. The buttons are updated to show their text  //
// in blue when the mouse is hovering over them. They are also offset         //
// slightly while being clicked to provide visual feedback.                   //
////////////////////////////////////////////////////////////////////////////////

#include "ost_shared.h"

static WNDPROC oldButtonProc;

////////////////////////////////////////////////////////////////////////////////
// getOldButtonProc                                                           //
//                                                                            //
// This function creates a temporary window of the built-in "button" class.   //
// The purpose of this is to retrieve the address of its window procedure,    //
// which is the address of the window procedure shared amongst all windows    //
// of the "button" class. That address is stored in oldButtonProc (declared   //
// above), and is used in the subclassed window procedure vssButtonProc().    //
//                                                                            //
// This function is called in the WM_CREATE processing of the main window     //
// procedure. If it fails, the program is aborted.                            //
////////////////////////////////////////////////////////////////////////////////

int getOldButtonProc(HWND hwnd_parent)
{
	// Create a temporary button window so I can store the address of the
	// default button window procedure
	HWND hwnd_button =
	CreateWindowA("button", NULL, WS_CHILD, 0, 0, 0, 0, hwnd_parent, NULL,
	              (HINSTANCE)GetWindowLongPtrA(hwnd_parent, GWLP_HINSTANCE),
	              NULL);

	if (!hwnd_button)
		return -1;

	// Sublass default button procedure for mouse tracking
	oldButtonProc = (WNDPROC)GetWindowLongPtr(hwnd_button, GWLP_WNDPROC);

	// Destroy the temporary button
	DestroyWindow(hwnd_button);

	if (!(oldButtonProc))
		return -1;

	return 0;
}

////////////////////////////////////////////////////////////////////////////////
// createVSSButton                                                            //
//                                                                            //
// The buttons in this program are based on the default "button" class        //
// provided in windows, with the BS_OWNERDRAW style. That means this program  //
// is responsible for drawing the buttons instead of windows. There is data   //
// tied to each button that needs to be set by the program:  custom data      //
// that stores the button state, and the address of the subclass window       //
// procedure, which is used for the mouse tracking processing.                //
//                                                                            //
// The button state data is a structure of type struct button_state, which is //
// defined in ost_shared.h. This structure contains two boolean members that  //
// are true when the mouse is clicking on and hovering over a button,         //
// respectively, and false otherwise. The sublass window procedure assigns    //
// these members according to the state of the mouse.                         //
//                                                                            //
// This function encapsulates the CreateWindow call and two SetWindowLongPtr  //
// calls, so the WM_CREATE processing for the banner and cab view window      //
// procedures is less cluttered.                                              //
////////////////////////////////////////////////////////////////////////////////

HWND createVSSButton(HWND hwnd_parent, char* p_button_text,
                     int x_pos, int y_pos, int x_size, int y_size,
                     LONG_PTR id, struct button_state* b_state)
{
	HWND hwnd_button = CreateWindowA("button", p_button_text,
		WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
		x_pos, y_pos, x_size, y_size, hwnd_parent, (HMENU)id,
		(HINSTANCE)GetWindowLongPtrA(hwnd_parent, GWLP_HINSTANCE), NULL);

	SetWindowLongPtrA(hwnd_button, GWLP_USERDATA, (LONG_PTR)b_state);
	SetWindowLongPtrA(hwnd_button, GWLP_WNDPROC,  (LONG_PTR)vssButtonProc);

	return hwnd_button;
}

////////////////////////////////////////////////////////////////////////////////
// vssButtonProc                                                              //
//                                                                            //
// This is the sublass window procedure used for the buttons in this program. //
// The window class from which it is sublassed is the built-in "button"       //
// class.                                                                     //
//                                                                            //
// When the mouse is first detected in the client area of the button, the     //
// hovering flag in the button_state structure is set to true, and            //
// TrackMouseEvent() is called to detect when it leaves (at which point the   //
// hovering flag is set back to false).                                       //
//                                                                            //
// Left button clicks are detected so the clicking flag in the button_state   //
// structure can be updated.                                                  //
//                                                                            //
// All other processing is passed to the default window procedure for the     //
// button class.                                                              //
////////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK vssButtonProc(HWND hwnd, UINT message, WPARAM wParam,
			       LPARAM lParam)
{
	struct button_state* p_state =
	(struct button_state*)GetWindowLongPtrA(hwnd, GWLP_USERDATA);

	switch (message) {
	case WM_MOUSEMOVE:
		if (p_state->hovering == FALSE)
		{
			TRACKMOUSEEVENT tme = { 0 };
			tme.cbSize = sizeof(TRACKMOUSEEVENT);
			tme.dwFlags = TME_LEAVE;
			tme.hwndTrack = hwnd;

			TrackMouseEvent(&tme);
			InvalidateRect(hwnd, NULL, FALSE);
			p_state->hovering = TRUE;
		}
		break;

	case WM_MOUSELEAVE:
		p_state->hovering = FALSE;
		p_state->clicking = FALSE;
		InvalidateRect(hwnd, NULL, FALSE);
		break;

	case WM_LBUTTONDOWN:
		p_state->clicking = TRUE;
		break;

	case WM_LBUTTONUP:
		p_state->clicking = FALSE;
		break;
	}
	// Pass all other processing to default button window procedure
	return CallWindowProcA(oldButtonProc, hwnd, message, wParam, lParam);
}

////////////////////////////////////////////////////////////////////////////////
// drawButtonRect                                                             //
//                                                                            //
// This simple function draws a white rectangle as the background for each    //
// button. Drawing this rectangle is necessary with an owner-draw button.     //
// This function is called is multiple locations, making it easy to change    //
// the appearance of every button by modifying this function.                 //
////////////////////////////////////////////////////////////////////////////////

void drawButtonRect(HDC hdc, LPRECT p_rect)
{
	FillRect(hdc, p_rect, GetStockObject(WHITE_BRUSH));
}

////////////////////////////////////////////////////////////////////////////////
// drawBtnTogText                                                             //
//                                                                            //
// Draws the text for each button. This function is slightly complicated      //
// because the button texts have different horizontal starting positions      //
// depending on their type and icon size, and also because the button state   //
// affects the color of the text. The vertical starting position is           //
// calculated so the text is centered in the button.                          //
//                                                                            //
// With an owner-draw control such as a button, it is important to leave the  //
// device context in the same condition in which it was provided to you.      //
// That's why this function has the variables called 'old_bk_color'           //
// 'old_text_color' - they store the defaults so the DC can be restored.      //
//                                                                            //
// Possible future improvement: multiple button text functions to simplify    //
// the code and reduce the number of parameters needed to call a text         //
// drawing function.                                                          //
////////////////////////////////////////////////////////////////////////////////

void drawBtnTogText(HDC hdc, HFONT h_font, char* p_text, LPRECT p_btn_rect,
                    const struct button_state* p_state, WORD button_ID)
{
	HFONT h_font_old = SelectObject(hdc, h_font);
	COLORREF old_bk_color = SetBkColor(hdc, WHITE);
	COLORREF old_text_color = { 0 };
	SIZE size_text = { 0 };

	int x_pos = 0;
	int y_pos = 0;

	if (!p_text || !p_btn_rect || !p_state)
		return;

	switch (button_ID) {
	case BTN_ID_CLEAR:
	case BTN_ID_FILE:
		x_pos = 42;
		break;
	case BTN_ID_HELP:      // Help button bitmap looks slightly wider than the
		x_pos = 46;         // other two buttons, so draw text further right
		break;
	case TOG_ID_LEGEND:    // no icon bitmap on toggle button - and the toggle
		x_pos = 8;          // bitmap is to the right of the text
		break;
	default:
		x_pos = 8;
		break;
	}

	// Center text vertically in the button
	GetTextExtentPoint32A(hdc, p_text, lstrlenA(p_text), &size_text);
	y_pos = ((p_btn_rect->bottom - p_btn_rect->top) - size_text.cy) / 2;

	// Set the text color and text position based on button state
	if (p_state->clicking) {
		old_text_color = SetTextColor(hdc, CTA_BLUE);
		x_pos += 1;
		y_pos += 2;
	}
	else if (p_state->hovering)
		old_text_color = SetTextColor(hdc, CTA_BLUE);
	else
		old_text_color = SetTextColor(hdc, GRAY);

	TextOutA(hdc, x_pos, y_pos, p_text, lstrlenA(p_text));

	// Restore previous values in device context
	if (old_text_color != CLR_INVALID)
		SetTextColor(hdc, old_text_color);
	if (old_bk_color != CLR_INVALID)
		SetBkColor(hdc, old_bk_color);
	if (h_font_old)
		SelectObject(hdc, h_font_old);
}

////////////////////////////////////////////////////////////////////////////////
// drawButtonBitmap                                                           //
//                                                                            //
// Draws the icons for the regular push buttons (not the toggle button).      //
// Like the button text, the icons change color depending on the button       //
// state; the icons are normally gray, but turn blue while the mouse is       //
// hovering over a button. The icons, along with the text, are also offset    //
// slightly while the button is being clicked - this provides the user with   //
// visual feedback so he/she knows the click is registered.                   //
//                                                                            //
// The clear and open file buttons have a slight negative horizontal offset.  //
// This is because their icons appear to be narrower than the help button     //
// icon. This was done to make them appear to be drawn in the same relative   //
// position.                                                                  //
////////////////////////////////////////////////////////////////////////////////

void drawButtonBitmap(HDC hdc_btn, HDC hdc_mem,
                      const struct button_state* p_state, WORD button_ID)
{
	int x_pos_bm = 0;
	int y_pos_bm = 0;

	int x_offset = 0;
	int y_offset = 0;

	if (p_state->clicking) {
		x_pos_bm = 24;
		x_offset = 1;
		y_offset = 2;
	}
	else if (p_state->hovering) {
		x_pos_bm = 24;
	}
	else
		x_pos_bm = 0;

	switch (button_ID) {
	case BTN_ID_CLEAR: y_pos_bm = 24; x_offset -= 2; break;
	case BTN_ID_FILE:  y_pos_bm = 48; x_offset -= 2; break;
	case BTN_ID_HELP:  y_pos_bm = 72; break;
	default:           y_pos_bm = 0;  break;
	}

	BitBlt(hdc_btn, 12 + x_offset, 12 + y_offset, 24, 24,
	       hdc_mem, x_pos_bm, y_pos_bm, SRCCOPY);
}

////////////////////////////////////////////////////////////////////////////////
// drawToggleBitmap                                                           //
//                                                                            //
// The toggle button's icon signals whether the legend is enabled or          //
// disabled. It doesn't depend on button state, so this function is simpler   //
// than drawButtonBitmap(). The state for the legend is stored in a static    //
// variable which is passed as the toggle_on argument. If toggle_on is true,  //
// the icon drawn will be an enabbled toggle switch; otherwise, the disabled  //
// toggle switch is drawn.                                                    //
////////////////////////////////////////////////////////////////////////////////

void drawToggleBitmap(HDC hdc_btn, HDC hdc_mem, BOOL toggle_on, LPRECT p_rect)
{
	int y_pos;

	if (toggle_on)
		y_pos = 96;
	else
		y_pos = 0;

	BitBlt(hdc_btn, p_rect->right - 53, 0, 37, 32, hdc_mem, 0, y_pos,
	       SRCCOPY);
}

////////////////////////////////////////////////////////////////////////////////
// paintHatchLines                                                            //
//                                                                            //
// Hatch lines are used as a stylistic effect in a few places. The areas in   //
// which the lines are drawn vary in size and shape. This function allows     //
// painting hatch lines in retangles of arbitrary length and height. It also  //
// sets the color of the lines to the hatch_color parameter.                  //
//                                                                            //
// x_start, y_start, x_end, and y_end define points of a rectangle, within    //
// which hatch lines will be painted. The hatch lines are painted using a     //
// BitBlt with a 10px by 10px monochrome bitmap. When a bitmap in a           //
// monochrome device context is BitBlt'ed into a color device context, the    //
// black pixels in the source are converted to the text color of the          //
// destination, and the white pixels in the source are converted to the       //
// background color of the destination, which is the color painted            //
// 'underneath' text. This function sets the text color of the device context //
// passed as hdc to hatch_color before performing the BitBlt.                 //
////////////////////////////////////////////////////////////////////////////////

int paintHatchLines(HDC hdc, P_SW_BITMAP p_bitmap_hatch, int x_start, int y_start,
		    int x_end, int y_end, COLORREF hatch_color)
{
	BITMAP bm = { 0 };

	// Create temporary "shadow" memory DC
	HDC hdc_mem_shadow;
	if ((hdc_mem_shadow = CreateCompatibleDC(hdc)) == NULL)
		return -1;

	// Create appropriately-sized bitmap for shadow DC
	HBITMAP h_bitmap_shadow =
	CreateCompatibleBitmap(hdc, x_end - x_start, y_end - y_start);

	if (!h_bitmap_shadow) {
		DeleteDC(hdc_mem_shadow);
		return -2;
	}

	SelectObject(hdc_mem_shadow, h_bitmap_shadow);

	GetObjectA(p_bitmap_hatch->h_bitmap, sizeof(BITMAP), &bm);

	// Set destination DC text color, which will be the hatch mark color
	SetTextColor(hdc_mem_shadow, hatch_color);
	
	// Paint the 10x10 hatch bitmap over the shadow bitmap
	for (int y = 0; y < y_end - y_start; y += bm.bmHeight)
	for (int x = 0; x < x_end - x_start; x += bm.bmWidth)
		BitBlt(hdc_mem_shadow, x, y, bm.bmWidth, bm.bmHeight,
		       p_bitmap_hatch->hdc_mem, 0, 0, SRCCOPY);

	// BitBlt shadow bitmap to destination
	BitBlt(hdc, x_start, y_start, x_end - x_start, y_end - y_start,
	       hdc_mem_shadow, 0, 0, SRCCOPY);

	DeleteDC(hdc_mem_shadow);
	DeleteObject(h_bitmap_shadow);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
// printWindowTitle                                                           //
//                                                                            //
// The list view and cab view child windows have a title, with text printed   //
// in Novum font, as well as a hatch mark underline. This function cleans up  //
// the code in those child window procedures that would otherwise be          //
// duplicated. The hatch marks are drawn so they are 1.7 times the length of  //
// the printed text.                                                          //
////////////////////////////////////////////////////////////////////////////////

void printWindowTitle(HDC hdc, HFONT h_font_title, char* p_text,
                      P_SW_BITMAP p_bitmap_hatch)
{
	SIZE size = { 0 };

	SelectObject(hdc, h_font_title);
	GetTextExtentPoint32A(hdc, p_text, lstrlenA(p_text), &size);

	// Print list view title
	TextOutA(hdc, 16, 32, p_text, lstrlenA(p_text));

	// Paint list view title underline
	paintHatchLines(hdc, p_bitmap_hatch, 16, 66, (int)(size.cx * 1.7),
	                74, RGB(0, 86, 214));
}

////////////////////////////////////////////////////////////////////////////////
// panelConflict                                                              //
//                                                                            //
// Determines whether a switch location conflicts with the zone 4 switch      //
// panel on a given spec. The "zone 4" panel is an area of the dash that may  //
// or may not contain switches depending on the option on the particular      //
// spec. Zone 4 contains the area occupied by switch locations 21-30.         //
//                                                                            //
// For example, the customer may have selected the storage cubby option, in   //
// which case any switch placed in 21-30 would cause a conflict. Or, the      //
// 6-switch panel may have been selected, in which case switches 21, 22, 26,  //
// and 27 would cause a conflict. If the 10-switch panel is selected, none    //
// of the switches cause a conflict.                                          //
//                                                                            //
// In case of a conflict, the function returns FALSE. Otherwise it returns    //
// TRUE. This function serves as a helpfer function and is used in            //
// cab_view.c and list_view.c.                                                //
////////////////////////////////////////////////////////////////////////////////

BOOL panelConflict(int loc, P_STATE_DATA p_data) {
	// Only look at switches in zone 4
	if (loc < 21 || loc > 30) {
		return FALSE;
	}
	else {
		// If the 10-switch panel was chosen, there is no conflict
		if (p_data->src_bitmap_pos[0] == 3) {
			return FALSE;
		}
		// If the 2-switch panel was chosen, only switches in 23 or 28 will
		// cause a conflict
		if ((loc == 23 || loc == 28) && p_data->src_bitmap_pos[0] == 1) {
			return FALSE;
		}
		// If the 6-switch panel was chosen, only switches in 21, 22, 26, or
		// 27 will cause a conflict
		if ((loc != 21 && loc != 22 && loc != 26 && loc != 27) &&
			p_data->src_bitmap_pos[0] == 2) {
			return FALSE;
		}
	}
	return TRUE;
}