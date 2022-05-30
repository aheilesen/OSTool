////////////////////////////////////////////////////////////////////////////////
// parse_switch.c                                                             //
//                                                                            //
// This TU contains functions used to parse the SP_SWITCH_DATA.csv and        //
// CA_SWITCH_DATA.csv resource files. These files contain data for every      //
// switch configuration currently offered on our trucks. SP_SWITCH_DATA       //
// contains data for the standard product offering, and CA_SWITCH_DATA        //
// contains data for the customer adaptation offering. These files contain    //
// the location, part number, variant string, and quantity for every switch   //
// configuration. These switch configurations are referred to as              //
// "switch links" in this program. The values for these switch links are      //
// stored in struct sw_link objects (see ost_data.h).                         //
//                                                                            //
// There are small "helper" functions that retrieve each of these values,     //
// called by processCSVLine(). parseCSV() calls processCSVLine() for every    //
// line with relevant data in the csv resource files.                         //
//                                                                            //
// Once the truck spec has been retrieved from EDB (or from a file), the      //
// program parses the spec and retrieves the variants for each option on the  //
// spec. Then, the program calls parseCSV() for each of the csv files to      //
// find all of the matching switch links.                                     //
//                                                                            //
// The switch links are stored in a linked list data structure. This TU also  //
// contains functions that insert a new switch link in the list, remove a     //
// link, and free memory associated with a link when it's removed.            //
////////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>	// for atoi
#include <stdio.h>
#include <string.h>	// for strncmp
#include "ost_data.h"

#include "parse_switch.h"
#include "resource.h"

////////////////////////////////////////////////////////////////////////////////
// skipToSwitches                                                             //
//                                                                            //
// Skip the first five lines of the CSV files. The first five lines just      //
// have metadata & column header info.                                        //
////////////////////////////////////////////////////////////////////////////////

int skipToSwitches(char** c)
{
	int i;

	for (i = 0; i < 5; i++) 			// skip 5 lines
		while (*((*c)++) != '\n') {}

	return 0;
}

////////////////////////////////////////////////////////////////////////////////
// getSwitchLoc                                                               //
//                                                                            //
// The location is the first piece of data on each line of the CSV files that //
// contain switch data. Each line with switch data has a location that is one //
// or two digits in size and immediately delimited with a semicolon. atoi()   //
// is therefore a simple and safe solution.                                   //
////////////////////////////////////////////////////////////////////////////////

int getSwitchLoc(const char* line)
{
	return atoi(line);
}

////////////////////////////////////////////////////////////////////////////////
// getPartNum                                                                 //
//                                                                            //
// The part number for each switch link is the second item on each line in    //
// the csv files. It's delimited with a semicolon, so use atoi() to retrieve  //
// it after skipping the first field (location).                              // 
////////////////////////////////////////////////////////////////////////////////

int getPartNum(const char* line)
{
	int i;
	for (i = 0; i < 2; i++)				// Second column in CSV
		while (*line++ != ';') {}

	return atoi(line);
}

////////////////////////////////////////////////////////////////////////////////
// getSWQty                                                                   //
//                                                                            //
// Same process as getPartNum. The switch link quantity is the sixth item on  //
// each line in the csv files. It's delimited with a semicolon, so use atoi() //
// to retrieve it after skipping the first field (location).                  //
////////////////////////////////////////////////////////////////////////////////

int getSWQty(const char* line)
{
	int i;
	for (i = 0; i < 6; i++)				// Sixth column in CSV
		while (*line++ != ';') {}

	return atoi(line);
}

////////////////////////////////////////////////////////////////////////////////
// getVarString                                                               //
//                                                                            //
// The variant string is a comma-delimited list of character strings. Each    //
// one of these strings is a variant; every one of the variants in this list  //
// must be on the spec for this switch link to be a match. The entire string  //
// is delimited with a semicolon. It's the fifth item on each line of the csv //
// files.                                                                     //
//                                                                            //
// This function copies the variant string into a buffer pointed to by the    //
// 'buf' parameter. The buffer has a size of VAR_STR_LENGTH bytes, and this   //
// function only writes a maximum of VAR_STR_LENGTH bytes to the buffer.      //
// VAR_STR_LENGTH is defined in parse_switch.h and was determined to be large //
// enough to hold all variant string lists in each csv file. If the csv files //
// were modified and some string lists held more than VAR_STR_LENGTH          //
// characters, some of the variants would not be found in the spec because    //
// they would be truncated, and the program would not match those switch      //
// links properly.                                                            //
//                                                                            //
// This function will return an error if the variant string list is greater   //
// in length than VAR_STR_LENGTH and won't fit into the buffer.               //
////////////////////////////////////////////////////////////////////////////////

int getVarString(char* buf, const char* line)
{
	int i;
	const char* tmp;

	for (i = 0; i < 5; i++)				// Fifth column in CSV
		while (*(line++) != ';') {}

	tmp = line;
	while (*tmp++ != ';') {}

	strncpy_s(buf, VAR_STR_LENGTH, line, tmp - line - 1);

	// strncpy_s sets the first member of buf to '\0' if it can't copy the
	// source to dest because it won't fit
	if (buf[0] == '\0')
		return -1;
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
// processCSVLine                                                             //
//                                                                            //
// This is the 'parent' function for getSwitchLoc(), getPartNum(),            //
// getSWQty(), and getVarString(). This function is called once for every     //
// line in each csv file by parseCSV(). If an error occurs in any of these    //
// 'helper' functions, this function returns -1 to indicate error, and        //
// parseCSV() returns with an error indication to its caller.                 //
////////////////////////////////////////////////////////////////////////////////

int processCSVLine(int* loc, int* pn, int* qty, char* buf, const char* line)
{
	*loc = getSwitchLoc(line);
	*pn = getPartNum(line);
	*qty = getSWQty(line);

	if (*loc == 0 || *pn == 0 || *qty == 0)
		return -1;

	return getVarString(buf, line);
}

////////////////////////////////////////////////////////////////////////////////
// checkVarString                                                             //
//                                                                            //
// This function checks each variant string for each switch link to see if it //
// is in the spec. If every variant in the string is in the spec it returns   //
// 0. Otherwise, it returns -1.                                               //
//                                                                            //
// A variant string list contains one or more variant strings. Each one of    //
// these strings is extracted into a buffer, terminated with a null           //
// character, and compared against every string in the variant list until a   //
// match is found or the entire list has been traversed.                      //
//                                                                            //
// Possible future improvement: this part of the program has a lot of room    //
// for improvement. For every variant string in every variant list (one list  //
// per switch link, around 750 total switch links), the entire list of        //
// variants is checked with a linear search using strncmp. There are usually  //
// around 900 variants in a spec. This results in many thousands of calls to  //
// strncmp.                                                                   //
//                                                                            //
// Possible improvements include using a different data structure (a hash     //
// table?) or implementing a more intelligent search. The variants are stored //
// in order of ascending variant family. With some more pre-generated data,   //
// like embedding the families in the csv files, I could make the searching   //
// much more efficient.                                                       //
//                                                                            //
// This is not a super high-priority improvement because the program is       //
// already quite fast. On modern processors, this program feels               //
// instantaneous when analyzing a spec from a file. Retrieving a spec from    //
// the internet takes much, much longer than analyzing it (it introduces an   //
// actually perceptible delay).                                               //
//                                                                            //
// Still, it's an inefficient design, and if this program were ever expanded  //
// upon to handle more functionality than drawing a dash, it would probaly be //
// important to improve this.                                                 //
////////////////////////////////////////////////////////////////////////////////

int checkVarString(const Variant* var_list, int num_var, const char* sw_vars)
{
	int i   = 0;
	int j   = 0;
	int end = 0;          // == 1 if '\0' (end of variant string) is encountered

	char var_tmp[SYMBOL_LENGTH + 1] = { 0 };

	while (!end) {
		i = 0;
		while ((var_tmp[i] = *sw_vars) != ',' && *sw_vars != '\0') {
			if (i == SYMBOL_LENGTH)
				return -1;
			i++;
			sw_vars++;
		}

		var_tmp[i] = '\0';
		end = (*sw_vars == '\0' ? 1 : 0);
		sw_vars++;

		// Loop through every variant in the spec. Check if var_tmp is
		// one of those variants.
		for (j = 0; j < num_var; j++)
			if (!strncmp((var_list + j)->symbol, var_tmp, i))
				break;

		if (j == num_var)
			return -1;
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
// parseCSV                                                                   //
//                                                                            //
// This function is called twice each time a spec is analyzed: once for       //
// SP_SWITCH_DATA.csv, and once for CA_SWITCH_DATA.csv. The first time it's   //
// called, it allocates memory on the heap for the switch list. This is a     //
// linked list of switch links.                                               //
//                                                                            //
// Depending on what was passed to the res_ID parameter, the function loads   //
// one of the two csv resource files. It skips to the sections of those csv   //
// files with switch data, and performs processing on each line of the file   //
// until the eof marker is encountered. A tilde (~) is used to denote eof.    //
//                                                                            //
// processCSVLine() is called for each line, and then checkVarString() is     //
// called to see if every variant string for that switch link is contained in //
// the spec. If they are, a new switch link is created and inserted into the  //
// linked list. Otherwise, the next line is assessed.                         //
//                                                                            //
// If an error occurs, a negative value is returned. The caller will halt     //
// processing and the dash will not be drawn. A message box indicating an     //
// error occurred while parsing the csv file will be displayed to the user.   //
////////////////////////////////////////////////////////////////////////////////

int parseCSV(LL** pSwitchList, const Variant* var_list, int num_var, WORD res_ID)
{
	HRSRC hrsrc;
	HGLOBAL hglobal;

	char* sw_tmp;
	char line[LINE_LENGTH];

	if (*pSwitchList == NULL) {
		if ((*pSwitchList = malloc(sizeof(LL))) == NULL)
			return -1;

		LL_Init(*pSwitchList, freeSWLink);
	}

	// Load CSV resource
	hrsrc = FindResourceA(NULL, MAKEINTRESOURCEA(res_ID), "CSV");
	if (hrsrc == NULL)
		return -2;
	if ((hglobal = LoadResource(NULL, hrsrc)) == NULL)
		return -3;
	if ((sw_tmp = LockResource(hglobal)) == NULL)
		return -4;

	if (skipToSwitches(&sw_tmp))
		return -5;

	// Process each line of the csv file
	while (!getLine(line, LINE_LENGTH, &sw_tmp)) {
		int loc, pn, qty;
		int insw = 0;
		char buf[VAR_STR_LENGTH];

		if (processCSVLine(&loc, &pn, &qty, buf, line) == -1)
			return -6;

		// There are some switch links in the SP_SWITCH_DATA.csv file that don't
		// pertain to dash switches - they're used for secondary gauge clusters,
		// the headlight switch assembly, ignition assembly, etc. The locations
		// for these links are skipped because they are not of interest.
		// 31 == main gauge cluster
		// 32 == light selector switch module
		// 39 == ignition switch
		if ((loc > 30 && loc < 35) || loc > 38)
			continue;

		// Likewise, switch links for covers or plugs are not of interest. These
		// are used when no switch is called out for a particular location. The
		// program assumes a plug is used when no switch link matches a
		// particular location. Plugs/covers are not shown in the list of
		// switches when the dash is drawn.
		if (pn == PLUG || pn == COVER)
			continue;

		if (checkVarString(var_list, num_var, buf))
			continue;
		
		if ((insw = (insertNewSW(*pSwitchList, loc, pn, buf, qty))) != 0)
			return insw;
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
// getLine                                                                    //
//                                                                            //
// This is a helper function used by parseCSV that extracts a line from one   //
// of the csv files into a buffer. It returns -1 to indicate eof.             //
////////////////////////////////////////////////////////////////////////////////

int getLine(char* dest, int dest_size, char** src)
{
	int i = 0;

	if (**src == '~')					// EOF
		return -1;

	// advance resource pointer to next line
	while ((*(dest + i++) = *((*src)++)) != '\n') {
		if (i >= dest_size - 1)
			return -2;
	}

	*(dest + i) = '\0';
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
// insertNewSW                                                                //
//                                                                            //
// When parseCSV() determines that one of the switch links matches the spec   //
// being analyzed, that link is inserted into the linked list of switch       //
// links.                                                                     //
//                                                                            //
// This function does two things of note.                                     //
// 1) If the quantity of a switch link is -1, removeSW() is called.           //
// 2) Switches are inserted in ascending order of location.                   //
//                                                                            //
// Some switch links in the CA_SWITCH_DATA file have a quantity of -1.        //
// This is used to remove a switch from a particular location. For every      //
// spec, the SP_SWITCH_DATA file is scanned first. This file contains data    //
// for the standard product offering. If a spec has a certain combination     //
// of variants that aren't standard, or it has a custom variant that moves    //
// a switch, the CA_SWITCH_DATA file uses a qty of -1 to remove a switch      //
// added by the SP_SWITCH_DATA.csv file. For example, if a switch was placed  //
// in location 5 after parsing the SP_SWITCH_DATA file, but the truck has     //
// a custom variant that moves it to location 6, the CA_SWITCH_DATA file will //
// place that same switch in location 5 with a quantity of -1, and place it   //
// again in location 6 with a quantity of 1. removeSW() would remove it from  //
// location 5, and only the link for location 6 would remain in the list.     //
//                                                                            //
// This function allocates memory for each switch link on the heap. It's      //
// freed when the list is destroyed, which happens every time freeMemory()    //
// is called in ostool.c. See description of that function for every time     //
// it's called.                                                               //
////////////////////////////////////////////////////////////////////////////////

int insertNewSW(LL* sw_list, int loc, int pn, const char* buf, int qty)
{
	struct sw_link* new_link = NULL;
	size_t vars_size = 0;

	LL_elem* elem_tmp = NULL;
	LL_elem* elem_prev_tmp = NULL;

	// If the quantity field is -1, the current switch is not
	// inserted, and a previous switch with the same part number
	// and location is deleted.

	int rst = 0;
	if (qty == -1) {
		if ((rst = removeSW(sw_list, loc, pn)) != 0)
			return rst;
		return 0;
	}

	if ((new_link = malloc(sizeof(struct sw_link))) == NULL)
		return -10;

	// At this point, new_link points to memory allocated
	// on the heap. It's member pointer vars does not.

	vars_size = strlen(buf) + 1;
	if ((new_link->vars = malloc(vars_size)) == NULL) {
		free(new_link);
		return -11;
	}

	// At this point, new link and its member pointer vars
	// both point to memory allocated on the heap

	new_link->loc = loc;
	new_link->pn = pn;
	strcpy_s(new_link->vars, vars_size, buf);
	new_link->qty = qty;

	// Insert the switches in ascending order (of location)

	elem_tmp = elem_prev_tmp = sw_list->head;
	while (elem_tmp) {
		if (((struct sw_link*)(elem_tmp->data))->loc > new_link->loc)
			break;

		elem_prev_tmp = elem_tmp;
		elem_tmp = elem_tmp->next;
	}

	if (LL_Ins_Next(sw_list, elem_prev_tmp, new_link) != 0) {
		freeSWLink(new_link);
		return -12;
	}

	// At this point, the newly inserted list element's
	// data field points to the memory in the heap
	// that is referenced by new_link. Therefore, this
	// function can now be safely exited (destroying new_link)

	return 0;
}

////////////////////////////////////////////////////////////////////////////////
// removeSW                                                                   //
//                                                                            //
// Removes a switch link from the linked list when -1 is encountered as the   //
// quantity of the switch link in the CA_SWITCH_DATA file. See description    //
// of insertNewSW() for why this is necessary.                                //
////////////////////////////////////////////////////////////////////////////////

int removeSW(LL* sw_list, int loc, int pn)
{
	LL_elem* sw_link;
	LL_elem* sw_link_prev = NULL;
	void* data;

	if (!sw_list)
		return -7;

	sw_link = sw_list->head;

	while (sw_link) {
		if (((SW_link*)sw_link->data)->pn == pn &&
			((SW_link*)sw_link->data)->loc == loc)
		{
			if (LL_Rem_Next(sw_list, sw_link_prev, &data)) {
				return -8;
			}
			else {
				freeSWLink(data);
				return 0;
			}
		}
		sw_link_prev = sw_link;
		sw_link = sw_link->next;
	}

	// Switch to remove wasn't in list (that shouldn't happen),
	// or the list was empty and head was NULL
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
// freeSWLink                                                                 //
//                                                                            //
// Each switch link structure has four data members: three automatic          //
// variables and one pointer to memory on the heap. This function properly    //
// frees the memory for each switch link, calling free() for the pointer      //
// member, and then free() on the switch link itself.                         //
//                                                                            //
// This function is called on every switch link in the linked list when it is //
// destroyed.                                                                 //
////////////////////////////////////////////////////////////////////////////////

void freeSWLink(void* link)
{
	if (((struct sw_link*)link)->vars) {
		free(((struct sw_link*)link)->vars);
		((struct sw_link*)link)->vars = NULL;
	}

	free((struct sw_link*)link);
}