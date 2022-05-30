#ifndef OST_SHARED_H_
#define OST_SHARED_H_

#include <Windows.h>
#include "ost_data.h"

// Child control IDs for misc. controls
#define LISTBOX_ID_SW   4000
#define EDIT_ID         4001
#define STAT_ID_A       4002
#define STAT_ID_B       4003

// Child control IDs for buttons
#define BTN_ID_CLEAR    5000
#define BTN_ID_FILE     5001
#define BTN_ID_HELP     5002
#define BTN_ID_ARROW    5003

#define TOG_ID_LEGEND   6003

// Button types
#define NORMAL_BUTTON   0
#define TOGGLE_BUTTON   1

// Custom Messages

// WM_DRAWHIGHLIGHT is a custom message used to facilitate communication
// between the cab view window and list view window. When an item is
// selected in the list box, the list view window procedure sends a
// WM_DRAWHIGHLIGHT message to the parent window procedure with the index
// of the selected item as the wParam. The parent window procedure uses
// this information to determine the position of the switch that was
// selected. It then sends a WM_DRAWHIGHLIGHT message to the cab view
// window procedure with the position as the lParam.
#define WM_DRAWHIGHLIGHT (WM_USER)

// WM_CLEARHIGHLIGHT is a custom message used to remove a selection
// from the list box when the main window client area is clicked. The
// parent window procedure sends a WM_CLEARHIGHLIGHT message to the
// list view window procedure which clears the list box.
#define WM_CLEARHIGHLIGHT (WM_USER+1)

#define WM_SETFOCUSEDIT (WM_USER+2)

struct button_state {
        BOOL clicking;
        BOOL hovering;
};
int getOldButtonProc(HWND hwnd_parent);

HWND createVSSButton(HWND hwnd_parent, char* p_button_text,
                     int x_pos, int y_pos, int x_size, int y_size,
                     LONG_PTR id, struct button_state* b_state);
LRESULT CALLBACK vssButtonProc(HWND hwnd, UINT message, WPARAM wParam,
                               LPARAM lParam);
void drawButtonRect(HDC hdc, LPRECT p_rect);
void drawBtnTogText(HDC hdc, HFONT h_font, char* p_text, LPRECT p_btn_rect,
                    const struct button_state* p_state, WORD button_ID);
void drawButtonBitmap(HDC hdc_btn, HDC hdc_mem,
                      const struct button_state* p_state, WORD button_ID);
void drawToggleBitmap(HDC hdc_btn, HDC hdc_mem, BOOL toggle_on, LPRECT p_rect);
int paintHatchLines(HDC hdc, P_SW_BITMAP p_bitmap_hatch,
                    int x_start, int y_start,
                    int x_end, int y_end, COLORREF hatch_color);
void printWindowTitle(HDC hdc, HFONT h_font_title, char* p_text,
                      P_SW_BITMAP p_bitmap_hatch);
BOOL panelConflict(int loc, P_STATE_DATA p_data);

#endif