#ifndef CAB_VIEW_H_
#define CAB_VIEW_H_

#include <Windows.h>
#include "ost_data.h"

LRESULT CALLBACK cabViewProc(HWND hwnd, UINT message, WPARAM wParam,
                             LPARAM lParam);

void drawDash(HDC hdc, HFONT h_font, int x_pos, int y_pos,
	      const P_STATE_DATA p_data);

void drawLegend(HDC hdc, int zone_4);
void drawHighlight(HDC hdc, POINT pt);

#endif