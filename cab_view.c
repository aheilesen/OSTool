////////////////////////////////////////////////////////////////////////////////
// cab_view.c                                                                 //
//                                                                            //
// This TU contains data and functions used to paint a visual representation  //
// of the dash once a spec is analyzed. Using some pre-determined coordinates //
// and sizes for bitmap resources, as well as offsets calculated by           //
// getSrcBitmapPos() in ostool.c, various BitBlt operations fill a shadow     //
// bitmap which is then copied to the cab view window. The bitmaps were       //
// created by taking screenshots of a CAD model of the dash with different    //
// switches selected at a time.                                               //
//                                                                            //
// Each bitmap is associated with a section of the dash. The sections coverd  //
// by these bitmaps are as small as possible to limit the number of unique    //
// views that had to be captured in these bitmaps. Unfortunately, because of  //
// perspective, each switch is shaped slightly differently and/or has a       //
// different surrounding, so I couldn't just use a mask to BitBlt one switch  //
// image all over the dash. Choosing the smallest 'groups' in the dash to     //
// take images of kept the total view count as low as possible. For example,  //
// two groups of two switches would require 8 unique views, whereas a panel   //
// of four switches would require 16 unique views.                            //
//                                                                            //
// Each section of the dash contains its own bitmap (and in one case, there   //
// are views for multiple sections embedded in the same bitmap). Within each  //
// section are multiple views (no switches, 1 switch in each location, etc    //
// etc... up to every location filled). To simplify the drawing, the same     //
// model is used for every switch. There are some switches that are shaped    //
// differently from others, but accounting for every switch shape being       //
// placed in every possible location was not worth the effort. For the        //
// purposes of idenitfying switch placement and preventing conflicts, this    //
// approach is more than satisfactory.                                        //
//                                                                            //
// Once the spec is processed, the src_bitmap_pos array in the state data     //
// structure declared in ostool.c is populated. During the painting           //
// operations, these values are multiplied by the vertical sizes in the       //
// destBitmapInfo structure (defined below) to select the bitmap that         //
// corresponds to the switch options chosen.                                  //
//                                                                            //
// Clicking on the 'Show Legend' toggle, or pressing F2, will draw location   //
// numbers and lines pointing to each location in the dash. Repeating the     //
// action will hide the legend. Additionally, clicking on a switch in the     //
// list view will cause a blue rectangle to be drawn around the selected      //
// switch in the dash. The functions that perform these operations are also   //
// in this TU.                                                                //
////////////////////////////////////////////////////////////////////////////////

#include "cab_view.h"
#include "ost_shared.h"
#include "andrewll.h"

// This array of structures stores the starting point and size of each
// source bitmap used in the BitBlt operations onto the shadow bitmap.

static struct {
	POINT pt;
	SIZE size;
} destBitmapInfo[] = {
	587, 469, 160, 128,       // Zone 4,    0   bitmap P
	361, 574, 96,  48,        // Pos 5-10,  1   bitmap C
	465, 476, 96,  48,        // Pos 11-15, 2   bitmap D
	465, 524, 96,  48,        // Pos 16-20, 3   bitmap E
	413, 197, 96,  48,        // Pos 35-37, 4   bitmap M

	102, 581, 48,  48,        // Pos 1,2,   5   bitmap ABFGHIJKL (A)
	182, 581, 48,  48,        // Pos 3,4,   6   bitmap ABFGHIJKL (B)
	604, 486, 48,  48,        // Pos 21,22, 7   bitmap ABFGHIJKL (F)
	670, 493, 48,  48,        // Pos 24,25, 8   bitmap ABFGHIJKL (H)
	595, 533, 48,  48,        // Pos 26,27, 9   bitmap ABFGHIJKL (I)
	669, 542, 48,  48,        // Pos 29,30, 10  bitmap ABFGHIJKL (K)
	417, 150, 48,  48,        // Pos 38,    11  bitmap ABFGHIJKL (L)

	641, 494, 48,  48,        // Pos 23,    12  bitmap ABFGHIJKL (G)
	632, 529, 48,  48,        // Pos 28,    13  bitmap ABFGHIJKL (J)
};

// This array of structures stores the coordinates for each line used to
// draw the legend. The legend displays location numbers with lines drawn
// to areas on the dash. This helps the user identify switch locations.

static POINT line_pts[] = {
	78,  455, 116, 584,  // Loc 1-2
	112, 442, 130, 583,

	164, 436, 194, 581,  // Loc 3-4
	201, 434, 210, 580,

	283, 432, 374, 578,	// Loc 5-10
	315, 418, 388, 577,
	347, 408, 401, 577,
	380, 404, 415, 577,
	412, 403, 429, 577,
	446, 408, 445, 577,

	493, 407, 498, 486,	// Loc 11-15
	521, 401, 512, 487,
	549, 398, 526, 488,
	575, 403, 540, 489,
	599, 411, 554, 491,

	397, 719, 487, 558,	// Loc 16-20
	437, 719, 502, 557,
	479, 716, 516, 559,
	514, 713, 529, 560,
	549, 706, 542, 560,

	655, 405, 632, 498,	// Loc 21-25
	690, 399, 643, 499,
	727, 395, 670, 501,
	758, 403, 694, 506,
	791, 413, 706, 507,

	592, 710, 613, 565,	// Loc 26-30
	623, 717, 626, 567,
	659, 726, 651, 571,
	696, 732, 677, 575,
	735, 735, 689, 576,

	403, 277, 441, 233,	// Loc 35-38
	448, 279, 454, 233,
	495, 269, 470, 233,
	419, 125, 443, 167,
};

// This array of structures stores the starting points for each highlight
// rectangle. A highlight rectangle is a blue rectangle drawn around a
// switch when it is selected in in the list box. This is another way the
// user can see where a switch is placed in the dash.

static POINT square_pts[] = {
	101,581,  // Loc 1-2
	115,580,

	180,578,  // Loc 3-4
	195,577,

	358,573,	 // Loc 5-10
	372,573,
	386,573,
	401,573,
	414,573,
	428,573,

	480,479,	 // Loc 11-15
	493,480,
	507,481,
	521,483,
	534,484,

	475,514,	 // Loc 16-20
	488,516,
	501,517,
	515,518,
	528,519,

	610,490,	 // Loc 21-25
	623,492,
	648,494,
	673,498,
	686,500,

	601,525,	 // Loc 26-30
	614,526,
	638,528,
	664,532,
	677,534,

	425,192,	 // Loc 35-38
	439,191,
	453,191,
	426,159,
};

// Stores the state of the toggle button (mouse hovering over / clicking)
static struct button_state toggle_state;

// Stores the state of the legend (enabled / disabled)
static BOOL f_legend;

// The location of the currently drawn highlight. -1 means it's disabled
static int highlight = -1;

LRESULT CALLBACK cabViewProc(HWND hwnd, UINT message, WPARAM wParam,
                             LPARAM lParam)
{
	static P_STATE_DATA p_data;

	switch (message) {
	case WM_CREATE:
		SetWindowLongPtrA(hwnd, GWLP_USERDATA,
		                  (LONG_PTR)((LPCREATESTRUCT)lParam)->lpCreateParams);

		p_data = (P_STATE_DATA)GetWindowLongPtrA(hwnd, GWLP_USERDATA);

		createVSSButton(hwnd, NULL, 768, 32, 176, 32, TOG_ID_LEGEND,
		                &toggle_state);

		return 0;

	case WM_COMMAND:
		if (LOWORD(wParam) == TOG_ID_LEGEND) {
			f_legend = !f_legend;
			InvalidateRect(hwnd, NULL, FALSE);
		}
		return 0;

	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);

		drawDash(hdc, p_data->h_font_text, 0, 104, p_data);

		printWindowTitle(hdc, p_data->h_font_title, "Cab View",
		                 p_data->p_bitmaps + p_data->num_bitmaps - 1);

		// If highlight is not -1, it represents the switch location.
		// It's one less than the location (it's used as the index of the
		// square_pts array)
		if (highlight >= 0 && !panelConflict(highlight + 1, p_data))
			drawHighlight(hdc, square_pts[highlight]);

		ReleaseDC(hwnd, hdc);
		return 0;
		}

	case WM_DRAWITEM: {
		HDC hdc_btn;
		RECT rect_btn;
		HFONT h_font;
		HDC hdc_mem;

		// Draw toggle button

		hdc_btn = ((LPDRAWITEMSTRUCT)lParam)->hDC;
		rect_btn = ((LPDRAWITEMSTRUCT)lParam)->rcItem;
		drawButtonRect(hdc_btn, &rect_btn);

		h_font = p_data->h_font_text;
		drawBtnTogText(hdc_btn, h_font, "Show Legend", &rect_btn,
		               &toggle_state, TOGGLE_BUTTON);

		hdc_mem = (p_data->p_bitmaps + 10)->hdc_mem;
		drawToggleBitmap(hdc_btn, hdc_mem, f_legend, &rect_btn);

		return 0;
		}

	case WM_DRAWHIGHLIGHT:
		// Possible values for lParam: -1 through 37, excluding 30, 31, 32,
		// and 33. -1 is a special value used to clear the highlight.
		highlight = (int)lParam;
		InvalidateRect(hwnd, NULL, FALSE);
		return 0;
	}
	return DefWindowProcA(hwnd, message, wParam, lParam);
}

////////////////////////////////////////////////////////////////////////////////
// drawDash                                                                   //
//                                                                            //
// Performs all of the BitBlt operations to draw the dash. This requires      //
// multiple steps:                                                            //
//                                                                            //
// 1: Create the temporary memory DC, or 'shadow bitmap', to perform all      //
//    intermediate operations on. Create and select a bitmap into this DC.    //
//    This bitmap is the same size as the final drawn dash.                   //
//                                                                            //
// 2: Perform BitBlt operations for all switches except 23 and 28.            //
//                                                                            //
// 3: Perform BitBlt operations for 23 and 28. These locations require        //
//    special treatment (masking).                                            //
//                                                                            //
// 4: Draw the legend (if the toggle button is enabled).                      //
//                                                                            //
// 5: BitBlt the intermediate shadow bitmap to the destination DC.            //
//                                                                            //
// Potential future improvements: the drawLegend() call happens in this       //
// function. It is required to perform these options when the legend is       //
// removed, but drawing the legend over an already-painted dash should not    //
// require the dash be painted again.                                         //
//                                                                            //
// Additionally, this function is quite long. I could break up the operations //
// in this function, and call each of those sub-functions from drawDash().    //
// This would make the code easier to debug in case of an error.              //
////////////////////////////////////////////////////////////////////////////////

void drawDash
(HDC hdc, HFONT h_font, int x_pos, int y_pos, const P_STATE_DATA p_data)
{
	int i;

	HDC hdc_mem_src;
	BITMAP bm = { 0 };

	HDC hdc_mem_temp;
	HBITMAP h_bitmap;

	const P_SW_BITMAP p_sw_bitmap = p_data->p_bitmaps;
	const int* p_src_bitmap_pos = p_data->src_bitmap_pos;
	int zone_4 = p_data->src_bitmap_pos[0];

	// Prepare temporary memory DC with shadow bitmap
	GetObjectA(p_sw_bitmap[6].h_bitmap, sizeof(BITMAP), &bm);
	h_bitmap = CreateCompatibleBitmap(hdc, bm.bmWidth, bm.bmHeight);
	hdc_mem_temp = CreateCompatibleDC(hdc);
	SelectObject(hdc_mem_temp, h_bitmap);

	// BitBlt blank dash to the temporary memory DC
	BitBlt(hdc_mem_temp, 0, 0, bm.bmWidth, bm.bmHeight,
	       p_sw_bitmap[6].hdc_mem, 0, 0, SRCCOPY);

	// BitBlt for bitmap sections  0 - 11 (ref destBitmapInfo[] struct above).
	// This performs the BitBlt operations for every switch location except
	// 23 and 28.
	for (i = 0; i < 12; i++) {

		int x = 0;

		// Skip locations 21, 22, 26, and 27 if the 10-switch panel
		// wasn't selected
		if ((i == 7 || i == 9) && p_src_bitmap_pos[0] != 3)
			continue;

		// Skip locations 24, 25, 29, and 30 if the zone 4 panel selected
		// is either the storage cubby or the 2-switch panel
		if ((i == 8 || i == 10) &&
			(p_src_bitmap_pos[0] == 0 || p_src_bitmap_pos[0] == 1)) {
			continue;
		}

		// Determine x-pos in source bitmap
		switch (i) {
		case 5:  x = 0;    hdc_mem_src = p_sw_bitmap[5].hdc_mem;  break;
		case 6:  x = 48;   hdc_mem_src = p_sw_bitmap[5].hdc_mem;  break;
		case 7:  x = 96;   hdc_mem_src = p_sw_bitmap[5].hdc_mem;  break;
		case 8:  x = 144;  hdc_mem_src = p_sw_bitmap[5].hdc_mem;  break;
		case 9:  x = 192;  hdc_mem_src = p_sw_bitmap[5].hdc_mem;  break;
		case 10: x = 240;  hdc_mem_src = p_sw_bitmap[5].hdc_mem;  break;
		case 11: x = 336;  hdc_mem_src = p_sw_bitmap[5].hdc_mem;  break;
		default: x = 0;    hdc_mem_src = p_sw_bitmap[i].hdc_mem;  break;
		}

		BitBlt(hdc_mem_temp,
		       destBitmapInfo[i].pt.x, destBitmapInfo[i].pt.y,
		       destBitmapInfo[i].size.cx, destBitmapInfo[i].size.cy,
		       hdc_mem_src, x,
		       p_src_bitmap_pos[i] * (i == 0 ? 128 : 48), SRCCOPY);
	}

	// BitBlt for bitmap sections 12 & 13 (ref destBitmapInfo[] struct above).
	// Special processing was required for switch locations 23 and 28. It was
	// not possible to BitBlt a rectangluar bitmap over these locations without
	// affecting the adjacent locations, so I had to use a mask to protect
	// the surrounding area.
	if (p_src_bitmap_pos[0] != 0) {
		for (i = 12; i < 14; i++) {

			// 0x220326 is the raster operation: Dest & ~Src. Src is a bitmap
			// with an area around the switch colored white, and it's surrounding
			// in black. This operation draws a black area around the switch in
			// the temporary memory DC.
			BitBlt(hdc_mem_temp,
			       destBitmapInfo[i].pt.x, destBitmapInfo[i].pt.y,
			       destBitmapInfo[i].size.cx, destBitmapInfo[i].size.cy,
			       p_sw_bitmap[5].hdc_mem, 288, -(12 - i) * 96, 0x220326);

			// SRCPAINT is the raster operation Dest | Src. This time, the Src
			// is an image of the switch (either occupied or not) surrounded by
			// black. The black parts get ignored and the switch is drawn onto
			// the dash leaving the surroundings untouched.
			BitBlt(hdc_mem_temp,
			       destBitmapInfo[i].pt.x, destBitmapInfo[i].pt.y,
			       destBitmapInfo[i].size.cx, destBitmapInfo[i].size.cy,
			       p_sw_bitmap[5].hdc_mem, p_src_bitmap_pos[i] ? 288 : 336,
			       -(12 - i) * 96 + 48, SRCPAINT);
		}
	}
	
	// Draw legend if it's enabled (based on the toggle button setting)
	if (f_legend) {
		SelectObject(hdc_mem_temp, h_font);
		drawLegend(hdc_mem_temp, zone_4);
	}

	// Final BitBlt. Copy the shadow bitmap to the destination DC
	BitBlt(hdc, x_pos, y_pos, bm.bmWidth, bm.bmHeight - 120,
	       hdc_mem_temp, 0, 104, SRCCOPY);

	DeleteDC(hdc_mem_temp);
	DeleteObject(h_bitmap);
}

////////////////////////////////////////////////////////////////////////////////
// drawLegend                                                                 //
//                                                                            //
// Draws the legend onto the dash. The dash does not need to be populated to  //
// draw the legend. The legend will update based on the zone 4 panel          //
// selected; if it's enabled, and a spec with a different zone 4 panel is     //
// processed, the legend will be updated to only show switch locations which  //
// exist with the selected zone 4 panel.                                      //
//                                                                            //
// The legend consists of numbers that represent switch locations, along with //
// lines that connect those numbers to their respective locations on the      //
// dash. The legend can be enabled/disabled by clicking on the 'Show Legend'  //
// button in the cab view window, or hitting the F2 key. Performing this      //
// action toggles the value of the static BOOL variable f_legend and redraws  //
// the dash.                                                                  //
//                                                                            //
// Potential future improvements: Draw the legend lines with anti-aliasing.   //
// Additionally, the numbers are always positioned directly above or below    //
// the endpoint of the line; it would be more aesthetically pleasing if the   //
// numbers were offset horizontally depending on the slope of the line.       //
////////////////////////////////////////////////////////////////////////////////

void drawLegend(HDC hdc, int zone_4)
{
	int i;
	int gap = 3;

	HPEN h_pen = SelectObject(hdc, CreatePen(PS_SOLID, 2, DARK_GRAY));

	for (i = 0; i < sizeof(line_pts) / sizeof(line_pts[0]); i += 2) {
		int len = 0;
		int x_pos = 0;
		int y_pos = 0;
		int loc = ((i >> 1) < 30) ? ((i >> 1) + 1) : ((i >> 1) + 5);
		char sz_position[3] = { 0 };
		SIZE size = { 0 };

		// Skip certain lines depending on zone 4 panel in spec

		// Cubby
		if (zone_4 == 0) {
			if (loc > 20 && loc <= 30)
				continue;
		}
		// 2-switch panel
		else if (zone_4 == 1) {
			if (loc > 20 && loc <= 30 && loc != 23 && loc != 28)
				continue;
		}
		// 6-switch panel
		else if (zone_4 == 2) {
			if (loc == 21 || loc == 22 || loc == 26 || loc == 27)
				continue;
		}

		// Draw line
		MoveToEx(hdc, line_pts[i].x,     line_pts[i].y,    NULL);
		LineTo(  hdc, line_pts[i + 1].x, line_pts[i + 1].y);

		// Get extents of drawn text so I can center it over the end points
		wsprintfA(sz_position, "%d", loc);
		len = lstrlenA(sz_position);
		GetTextExtentPoint32A(hdc, sz_position, len, &size);

		// Place number above or below line, depending on how it's drawn
		if (line_pts[i].y < line_pts[i + 1].y)
			y_pos = line_pts[i].y - size.cy - gap;
		else
			y_pos = line_pts[i].y + gap;

		// Get x position of centered text and draw it
		x_pos = line_pts[i].x - (size.cx / 2);
		TextOutA(hdc, x_pos, y_pos, sz_position, len);
	}

	DeleteObject(SelectObject(hdc, h_pen));
}

////////////////////////////////////////////////////////////////////////////////
// drawHighlight                                                              //
//                                                                            //
// When the user clicks on a switch in the list box, the list view window     //
// procedure sents a custom WM_DRAWHIGHLIGHT message to the parent window     //
// procedure with the index of the selected item. The parent window procedure //
// takes that index and determines the location in the dash that switch       //
// occupies, and sends another WM_DRAWHIGHLIGHT message to the cab view       //
// window procedure with this information. The static variable 'highlight'    //
// stores this location, and the cab view window procedure calls this         //
// function to draw the highlight.                                            //
//                                                                            //
// The 'highlight' is a blue rectangle drawn over location that corresponds   //
// to the selected switch. This serves as a visual aid to help the user       //
// identify the location of each switch. The highlight is automatically       //
// updated for every new selection. Clicking on the gray area between the     //
// child windows clears the selection and erases the highlight, as does       //
// pressing the escape key. This sets the value of the static variable        //
// 'highlight' to -1, which is a signal that the highlight is not to be       //
// shown.
////////////////////////////////////////////////////////////////////////////////

void drawHighlight(HDC hdc, POINT pt)
{
	HPEN h_pen_old = SelectObject(hdc, CreatePen(PS_SOLID, 3, CTA_BLUE));
	HBRUSH h_brush_old = SelectObject(hdc, GetStockObject(NULL_BRUSH));
	Rectangle(hdc, pt.x, pt.y, pt.x + 32, pt.y + 48);

	DeleteObject(SelectObject(hdc, h_pen_old));
	SelectObject(hdc, h_brush_old);
}