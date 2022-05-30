////////////////////////////////////////////////////////////////////////////////
// list_view.c                                                                //
//                                                                            //
// This TU contains data and functions used by the list view child window,    //
// one of three child windows to the main parent window. The list view is     //
// drawn at the left of the application client area, under the banner         //
// window. When a spec is processed, it displays the switches in a list box   //
// control. Otherise it's empty.                                              //
//                                                                            //
// The items in the list are selectable with a mouse click. Once one of the   //
// switches is selected, the other switches can be selected with the arrow    //
// keys. If one of the switches is selected, its background in the list is    //
// colored orange, and a highlight rectangle is drawn over the switch in the  //
// cab view window.                                                           //
//                                                                            //
// Clicking in the gray area between the child windows deselects the switch,  //
// as does pressing the escape key.                                           //
//                                                                            //
// In the event of a conflict, in which two switches occupy the same          //
// location, the text for the affected switches is colored red. Switches      //
// that can't be placed in zone 4 because of a mismatched panel are not       //
// shown in the list view.                                                    //
////////////////////////////////////////////////////////////////////////////////

#include <Windows.h>

#include "list_view.h"
#include "andrewll.h"
#include "ost_shared.h"
#include "resource.h"

// Pointer to a text file resource that contains switch descriptions
static const char* g_desc_res;

LRESULT CALLBACK listViewProc
(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HWND hwnd_sw_list_box;
	static P_STATE_DATA p_data;

	// h_rsrc and h_global don't need to be static because they are
	// freed automatically when the program closes
	HRSRC h_rsrc;
	HGLOBAL h_global;

	PAINTSTRUCT ps;
	HDC hdc;
	int sw_index;

	switch (message) {
	case WM_CREATE:

		// Load switch description resource
		h_rsrc = FindResourceA(NULL, MAKEINTRESOURCEA(IDR_CSV1), "CSV");
		if (h_rsrc == NULL)
			return -1;

		if ((h_global = LoadResource(NULL, h_rsrc)) == NULL)
			return -1;

		if ((g_desc_res = LockResource(h_global)) == NULL)
			return -1;

		SetWindowLongPtrA(hwnd, GWLP_USERDATA,
								(LONG_PTR)((LPCREATESTRUCT)lParam)->lpCreateParams);

		p_data = (P_STATE_DATA)GetWindowLongPtrA(hwnd, GWLP_USERDATA);

		// Create listbox control for switch list
		hwnd_sw_list_box = createListBox(hwnd);

		SendMessageA(hwnd_sw_list_box, WM_SETFONT,
		             (WPARAM)((P_STATE_DATA)
		             (((LPCREATESTRUCT)lParam)->lpCreateParams))->h_font_text,
		             FALSE);

		SendMessageA(hwnd_sw_list_box, LB_SETITEMHEIGHT, 0, 32);

		return 0;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case BTN_ID_CLEAR:
			SendMessageA(hwnd_sw_list_box, LB_RESETCONTENT, 0, 0);
			break;

		case LISTBOX_ID_SW:
			switch (HIWORD(wParam)) {
			case LBN_SELCHANGE:
				// An int can safely hold the index of any item in the list box.
				// It must be able to hold -1 which is why sw_index is declared as
				// an int.
				sw_index = (int)SendMessageA(hwnd_sw_list_box, LB_GETCURSEL, 0, 0);
				SendMessageA(GetParent(hwnd), WM_DRAWHIGHLIGHT, sw_index, 0);
				break;
			}
		}
		return 0;

	case WM_PAINT:
		hdc = BeginPaint(hwnd, &ps);

		printWindowTitle(hdc, p_data->h_font_title, "Switch List",
							  p_data->p_bitmaps + p_data->num_bitmaps - 1);

		printListBoxHeader(hdc, p_data->h_font_text);

		// Only populate the list box if it's currently empty
		// Potential improvement - find a more elegant way of doing this.
		if (!SendMessageA(hwnd_sw_list_box, LB_GETCOUNT, 0, 0)) {
			populateListBox(hwnd_sw_list_box, p_data->p_sw_list);
			setLbItemFlags(hwnd_sw_list_box, p_data);
		}

		ReleaseDC(hwnd, hdc);
		return 0;

	case WM_CLEARHIGHLIGHT:
		// Even though WPARAM is unsigned, the documentation says to use -1 here
		SendMessageA(hwnd_sw_list_box, LB_SETCURSEL, (WPARAM)-1, 0);
		return 0;

	case WM_DRAWITEM: {
		PDRAWITEMSTRUCT pdis = (PDRAWITEMSTRUCT)lParam;
		HBRUSH hbrush_lb;
		HBRUSH hbrush_old;
		HFONT h_font_old;
		RECT rect_lb;
		char lb_buf[60] = { 0 };
		int len = 0;

		switch (pdis->itemAction) {
		case ODA_SELECT:
			if (pdis->itemState == ODS_SELECTED) {
				hbrush_lb = CreateSolidBrush(ORANGE);
				hbrush_old = SelectObject(pdis->hDC, hbrush_lb);
				FillRect(pdis->hDC, &(pdis->rcItem), hbrush_lb);
				SetBkColor(pdis->hDC, ORANGE);
				DeleteObject(SelectObject(pdis->hDC, hbrush_old));
			}
			else
				FillRect(pdis->hDC, &(pdis->rcItem), GetStockObject(WHITE_BRUSH));

			// fall through

		case ODA_DRAWENTIRE: {
			h_font_old = SelectObject(pdis->hDC, p_data->h_font_text);
			DRAWTEXTPARAMS dpt = { 0 };

			// The longest switch description is 43 characters in length.
			// Therefore, len should range from 0 - 46. This variable is
			// used as an argument for DrawTextA, which takes an int for its
			// fourth parameter.
			len = (int)SendMessageA(hwnd_sw_list_box, LB_GETTEXT, pdis->itemID,
			                        (LPARAM)lb_buf);

			CopyRect(&rect_lb, &(pdis->rcItem));
			rect_lb.left += 16;

			if (SendMessage(hwnd_sw_list_box, LB_GETITEMDATA, pdis->itemID, 0))
				SetTextColor(pdis->hDC, RGB(255, 0, 0));
			else
				SetTextColor(pdis->hDC, 0);

			dpt.cbSize = sizeof(DRAWTEXTPARAMS);
			dpt.iTabLength = 6;

			DrawTextExA(pdis->hDC, lb_buf, len, &rect_lb,
			            DT_SINGLELINE | DT_VCENTER | DT_EXPANDTABS | DT_TABSTOP,
			            &dpt);
			SelectObject(pdis->hDC, h_font_old);
		}
		}
		return 0;
	}
	}
	return DefWindowProcA(hwnd, message, wParam, lParam);
}

////////////////////////////////////////////////////////////////////////////////
// createListBox                                                              //
//                                                                            //
// This function just removes boilerplate code so WM_CREATE processing for    //
// the list box window procedure is a little cleaner. It creates the list     //
// box child control with all of the necessary list box styles and creation   //
// parameters.                                                                //
//                                                                            //
// The list box control has the LBS_OWNERDRAWFIXED style. This is necessary   //
// to control the colors of the list box items; in particular, coloring the   //
// text red in the event of a switch conflict.                                //
////////////////////////////////////////////////////////////////////////////////

HWND createListBox(HWND hwnd_parent)
{
	DWORD list_style = WS_CHILD | WS_VISIBLE | LBS_OWNERDRAWFIXED |
	                   LBS_HASSTRINGS | LBS_NOTIFY | WS_VSCROLL;

	// The list box parent window is 777 pixels in height. Since
	// the list box is placed 136 pixels below the origin, it should
	// extend all the way to the bottom of its parent with a height
	// of 641 pixels, but it does not. I'm not sure why this is. It
	// might have something to do with the list box only able to be
	// sized to a height that would display elements in their
	// entirety.
	return
	CreateWindowA("listbox", NULL, list_style, 0, 136, 384, 641, hwnd_parent,
	              (HMENU)LISTBOX_ID_SW,
	              (HINSTANCE)GetWindowLongPtr(hwnd_parent, GWLP_HINSTANCE),
	              NULL);
}

////////////////////////////////////////////////////////////////////////////////
// printListBoxHeader                                                         //
//                                                                            //
// Prints the header for the list box, including the underline. Doing this    //
// in a way that accounts for changes in the header font or length is         //
// surprisingly involved. GetTextExtentPoint32 does not expand tabs (there    //
// is no method of communicating positions of tab stops to the function), so  //
// there is no trivial way of calculating the length of the gap between       //
// words when a tab separates them. This length is required for drawing the   //
// underlines.                                                                //
//                                                                            //
// To determine the span of this gap (which is based on the tab stop being    //
// located at 6 average character widths, the value chosen to provide the     //
// gap between columns), the DT_CALCRECT argument was used in the DrawTextEx  //
// function. This expands the 'right' member in the size structure            //
// argument to fit the enclosed string. Once the spans of the "Loc" and       //
// "Switch Type" strings are calculated with GetTextExtentPoint32, the legnth //
// of the gap can be calculated by subtracting these spans from the total     //
// horizontal size.                                                           //
//                                                                            //
// Once the text is printed with DrawTextEx, the underlines are drawn uner    //
// "Loc" and "Switch Type", separated by the calculated gap distance.         //
////////////////////////////////////////////////////////////////////////////////

void printListBoxHeader(HDC hdc, HFONT h_font)
{
	int x_start = 16;
	int y_start = 104;

	int loc_len = 0;
	int swt_len = 0;
	int tab_len = 0;

	int ul_pos = 0;

	SelectObject(hdc, h_font);
	DRAWTEXTPARAMS dpt = { 0 };
	dpt.cbSize = sizeof(DRAWTEXTPARAMS);
	dpt.iTabLength = 6;
	RECT rect = { 0 };
	SIZE size = { 0 };

	char header[] = "Loc\tSwitch Type";

	// Get length of "Loc"
	GetTextExtentPoint32A(hdc, header, 3, &size);
	loc_len = size.cx;

	// Get length of "Switch Type"
	GetTextExtentPoint32A(hdc, header + 4, 11, &size);
	swt_len = size.cx;

	// Get length of gap
	SetRect(&rect, x_start, y_start, x_start + size.cx, y_start + size.cy);
	DrawTextExA(hdc, header, -1, &rect,
	            DT_SINGLELINE | DT_EXPANDTABS | DT_TABSTOP | DT_CALCRECT, &dpt);
	tab_len = rect.right - rect.left - swt_len - loc_len;

	// Save vertical position for drawing underline
	GetTextExtentPoint32A(hdc, header, 15, &size);
	ul_pos = y_start + size.cy + 2;
	
	// Draw the header string
	DrawTextExA(hdc, header, -1, &rect,
	            DT_SINGLELINE | DT_EXPANDTABS | DT_TABSTOP, &dpt);

	// Print underlines
	MoveToEx(hdc, x_start, ul_pos, NULL);
	LineTo(hdc, x_start + loc_len, ul_pos);

	MoveToEx(hdc, x_start + loc_len + tab_len, ul_pos, NULL);
	LineTo(hdc, x_start + loc_len + tab_len + swt_len, ul_pos);
}

////////////////////////////////////////////////////////////////////////////////
// populateListBox                                                            //
//                                                                            //
// Populates the list box with switches in the switch list after a spec is    //
// processed. The switch link list is traveresed, and for every item in the   //
// list, a string is inserted into the list box. Each of these strings        //
// consists of a location and a description, separated by a tab.              //
//                                                                            //
// The location for each entry is taken from the item in the list. The        //
// description comes from a resource file, which is just a text file that     //
// lists switch part numbers and associated descriptions in a table. The      //
// part number is taken from the item in the list, and cross-referenced with  //
// this resource to retrieve the description.                                 //
//                                                                            //
// Before the list box is populated, this function sends a message to prevent //
// the list box from updating itself every time a string is inserted. This    //
// is re-enabled once the switch is populated and then the list is updated.   //
////////////////////////////////////////////////////////////////////////////////

int populateListBox(HWND hwnd, LL* p_sw_list)
{
	LRESULT list_res;
	LL_elem* p_tmp = NULL;
	char list_buf[60] = { 0 };
	char desc_buf[50] = { 0 };

	if (!p_sw_list)
		return -1;

	p_tmp = p_sw_list->head;
	
	// Pause updating so the list isn't redrawn for every insertion
	SendMessageA(hwnd, WM_SETREDRAW, FALSE, 0);

	// Loop through switch list, adding each item to the list box
	while (p_tmp) {

		int loc = ((SW_link*)(p_tmp->data))->loc;
		int pn  = ((SW_link*)(p_tmp->data))->pn;

		// Get switch description from resource file
		getSWDesc(desc_buf, 50, pn);

		// Concatenate the location and description, separated by a tab
		wsprintfA(list_buf, "%d\t%s", loc, desc_buf);

		// Insert item into the list
		list_res = SendMessageA(hwnd, LB_ADDSTRING, 0, (LPARAM)list_buf);
		if (list_res == LB_ERR || list_res == LB_ERRSPACE)
			return -2;

		p_tmp = p_tmp->next;
	}

	// Redraw the list
	SendMessageA(hwnd, WM_SETREDRAW, TRUE, 0);
	UINT rdr_fl = RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN;
	RedrawWindow(hwnd, NULL, NULL, rdr_fl);

	return 0;
}

////////////////////////////////////////////////////////////////////////////////
// getSWDesc                                                                  //
//                                                                            //
// The descriptions for the switches, which are displayed in the list box,    //
// are located in a text file which is compiled as a resource for this        //
// application. This text file contains a table with all of the descriptions  //
// linked to the part numbers for each switch. This function retrieves the    //
// description for a given part number from this resource and stores it in    //
// the buffer pointed to by desc.                                             //
////////////////////////////////////////////////////////////////////////////////

void getSWDesc(char* desc, int desc_size, int pn)
{
	const char* desc_tmp = g_desc_res;

	char buf[BUF_LENGTH] = { 0 };
	int i = 0;

	if (!desc)
		return;

	*desc = '\0';

	if (desc_size < 0)
		return;

	// At this point desc_tmp points to the first byte of data
	// in the sw_desc.txt resource

	while (*desc_tmp != '~') {			// '~' == EOF marker
		if ((buf[i++] = *desc_tmp++) != '\n')
			continue;

		buf[i - 1] = '\0';

		if (atoi(buf) == pn) {
			i = 0;
			while (buf[i++] != ';') {}

			strncpy_s(desc, desc_size, buf + i, strlen(buf + i));
			return;
		}
		i = 0;
	}
}

////////////////////////////////////////////////////////////////////////////////
// setLbItemFlags                                                             //
//                                                                            //
// The built-in list box control supports appending 'user data' to each item  //
// in the list box. This program utilizes this feature to enable drawing      //
// text red for switches that conflict with other switches or the zone 4      //
// panel.                                                                     //
//                                                                            //
// This function checks each switch to see if the subsequent switch occupies  //
// the same location; if so, it sets this user data equal to 1 for both. It   //
// also checks if each switch conflicts with the zone 4 panel chosen, and     //
// assigns the value 1 to the user data section of each affected switch.      //
//                                                                            //
// When the list box is drawn, the custom processing draws the text red if    //
// this user data is 1, and black otherwise. Enabling this functionality      //
// required the LBS_OWNERDRAWFIXED window style for the list box control.     //
////////////////////////////////////////////////////////////////////////////////

void setLbItemFlags(HWND hwnd_list, P_STATE_DATA p_data)
{
	int i = 0;

	// LB_GETCOUNT can return LB_ERR which is defined as -1, so count must
	// be signed. An int is more than large enough to hold the number of
	// items in the list box.
	int count = (int)SendMessageA(hwnd_list, LB_GETCOUNT, 0, 0);

	char buf_cur[60]  = { 0 };
	char buf_next[60] = { 0 };
	int loc = 0;

	for (i = 0; i < count - 1; i++) {
		SendMessageA(hwnd_list, LB_GETTEXT, i,             (LPARAM)buf_cur);
		SendMessageA(hwnd_list, LB_GETTEXT, (WPARAM)i + 1, (LPARAM)buf_next);
		loc = atoi(buf_cur);

		// Check for switches in the same location
		if (buf_cur[0] == buf_next[0] && buf_cur[1] == buf_next[1]) {
			SendMessageA(hwnd_list, LB_SETITEMDATA, i, 1);
			SendMessageA(hwnd_list, LB_SETITEMDATA, (WPARAM)i + 1, 1);
		}
		// Check for switches that conflict with the zone 4 panel chosen
		else if (panelConflict(loc, p_data))
			SendMessageA(hwnd_list, LB_SETITEMDATA, i, 1);
	}
}