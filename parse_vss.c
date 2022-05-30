////////////////////////////////////////////////////////////////////////////////
// parse_vss.c                                                                //
//                                                                            //
// This TU contains functions used to parse the spec for a given truck. A     //
// spec is attained in two different ways:                                    //
//                                                                            //
// 1: From a file (when browsing EDB, using the File->Save As function in a   //
//    browser)                                                                //
// 2: The application pulls the spec from EDB.                                //
//                                                                            //
// EDB is the database / website that stores and displays specification and   //
// other data. When a VSS number is generated for a quote, EDB can display    //
// all of the variant data tied to that VSS number. This program can download //
// the data on the EDB webpage and parse it when provided a VSS number.       //
//                                                                            //
// There are two different versions of the functions in this TU: one for a    //
// spec provided in a file (terminated with 'File'), and one for a spec       //
// pulled by the program (terminated with 'Buffer').                          //
//                                                                            //
// When the spec is parsed, the application populates members in an array of  //
// (struct variant)s. The definition for struct variant is located in         //
// ost_data.h. This list is allocated on the heap in either parseVSSFile() or //
// parseVSSBuffer().                                                          //
////////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "parse_vss.h"

////////////////////////////////////////////////////////////////////////////////
// skipToVariantsFile                                                         //
//                                                                            //
// Sets the file position to the first character of the first line with       //
// variant information from a COS spec. Returns a negative number if an error //
// occurs during this process.                                                //
//                                                                            //
// Reasons an error can occur:                                                //
//                                                                            //
//	1) EOF is encountered (or there's another error executing fgets)           //
//	   before the 14th line of input is arrived at. The 14th line              //
//	   (defined as FIRST_VSS_LINE in parsevss.h) is the first line with        //
//	   variant data from a valid VSS spec retrieval from COS.                  //
//	2) fgetpos() doesn't execute properly                                      //
//	3) The 14th line of input doesn't contain "000  AAX PRODUCT CLASS",        //
//	   which is always the first variant listed in a valid COS retrieval.      //
////////////////////////////////////////////////////////////////////////////////
int skipToVariantsFile(FILE* fp, fpos_t* fpos)
{
	int i;
	char line[LINE_LENGTH];

	// Loop through lines until the first line of the variant list
	// is reached. This should be the 14th line for a valid VSS retrieval
	for (i = 0; i < FIRST_VSS_LINE; i++) {
		if (fgetpos(fp, fpos))
			return -1;

		// If EOF is encoutered at this point, input is invalid
		if (fgets(line, LINE_LENGTH, fp) == NULL)
			return -2;
	}

	if (strstr(line, "000  AAX PRODUCT CLASS") == NULL)
		return -3;

	return 0;
}

////////////////////////////////////////////////////////////////////////////////
// skipToVariantsBuffer                                                       //
//                                                                            //
// Same as skipToVariantsFile, but operates on a buffer that holds a          //
// VSS spec retrieved from the internet (from EDB). Instead of moving a file  //
// position pointer, it moves a character pointer.                            //
////////////////////////////////////////////////////////////////////////////////

int skipToVariantsBuffer(char** cur_pos)
{
	int i;
	int count;

	for (i = 0; i < 13; i++) {

		count = 0;
		while (**cur_pos != '\n') {

			// '~' is used as the EOF marker. EOF should
			// not be encountered at this point.
			if (**cur_pos == '~')
				return -1;

			// In a valid spec, no line before the variants
			// begin should be 210 characters or more in length.
			if (++count > 250)
				return -2;

			(*cur_pos)++;
		}
		(*cur_pos)++;
	}

	if (strstr(*cur_pos, "000  AAX PRODUCT CLASS") == NULL)
		return -2;

	return 0;
}

////////////////////////////////////////////////////////////////////////////////
// countLinesFile                                                             //
//                                                                            //
// Retuns the number of lines with variants, or -1 if the input was invalid.  //
// Reasons for invalid inputs:                                                //
//                                                                            //
// 1) EOF was encountered                                                     //
//	2) a line was encountered that won't fit in the maximum line length        //
//	   LINE_LENGTH                                                             //
//	3) the number of variants encountered exceeds the pre-determined           //
//	   maximum MAX_VARIANTS.                                                   //
////////////////////////////////////////////////////////////////////////////////

int countLinesFile(FILE* fp)
{
	int i;
	char line[LINE_LENGTH];

	for (i = 0; i < MAX_VARIANTS; i++) {

		// EOF encountered
		if (fgets(line, LINE_LENGTH, fp) == NULL)
			return -4;

		// Line won't fit in buffer
		if (strchr(line, '\n') == NULL)
			return -5;

		// End of variant list is reached
		if (line[0] == '\n')
			break;

		// End of file marker (as resource)
		if (line[0] == '~')
			break;
	}

	// Exceeded max number of variants
	if (i == MAX_VARIANTS)
		return -6;

	return i;
}

////////////////////////////////////////////////////////////////////////////////
// countLinesBuffer                                                           //
//                                                                            //
// Same as countLinesFile, but operates on a buffer that holds a              //
// VSS spec retrieved from the internet (from EDB).                           //
////////////////////////////////////////////////////////////////////////////////

int countLinesBuffer(char* buf)
{
	int i = 0;
	int count;

	while (i < MAX_VARIANTS) {
		count = 0;

		while (*buf != '\n') {

			// '~' is used as the EOF marker. EOF should
			// not be encountered at this point.
			if (*buf == '~')
				return -1;

			// In a valid spec, containing the variants
			// should be 500 characters or more in length.
			// The longest line I found in my test spec
			// was 247 characters long.
			if (++count > 500)
				return -2;

			buf++;
		}
		i++;

		// An empty line denotes the end of the variant list
		if (*(buf - 1) == '\n')
			return i;
		buf++;
	}

	return -3;
}

////////////////////////////////////////////////////////////////////////////////
// processVSSLineFile                                                         //
//                                                                            //
// Reads a line with variant data and fills the struct variant fields with    //
// family description, IDVAR6, symbol, and variant description information.   //
// These are the four fields of the struct variant structure (see             //
// ost_data.h). Returns a negative number if fgets fails, or 0.               //
////////////////////////////////////////////////////////////////////////////////

int processVssLineFile(FILE* fp, struct variant* var)
{
	int i = 0;
	int pos;
	char* c;
	char line[LINE_LENGTH];

	if ((fgets(line, LINE_LENGTH, fp)) == NULL)
		return -7;

	// Skip to first field to read (family description)
	pos = 9;

	// Read family description
	for (i = 0; i < FAM_DESC_LENGTH; i++)
		*(var->fam_desc + i) = line[pos++];

	*(var->fam_desc + i) = '\0';
	pos++;

	// Check if this line has a link. If it does, skip text to get to
	// the symbol field.
	c = strstr(line + pos, ".pdf\">");
	if (c != NULL)
		pos = (int)(c - line) + 6;

	// Read Symbol
	for (i = 0; i < SYMBOL_LENGTH; i++)
		*(var->symbol + i) = line[pos++];
	*(var->symbol + i) = '\0';

	// If the line had a link, skip the closing </a> tag
	if (c != NULL)
		pos += 4;

	// Skip the space that separates the symbol field and the IDVAR6 field
	pos++;

	// Read IDVAR6
	for (i = 0; i < 6; i++)
		*(var->idvar6 + i) = line[pos++];
	*(var->idvar6 + i) = '\0';

	// Skip space that separates IDVAR6 field and the var description field
	pos++;

	// Read variant description
	for (i = 0; i < 60; i++) {
		// This if statement was added because of a bug in the
		// input. Variant 260-006 had a variant description
		// that was less than 60 characters (meaning it was not
		// padded with spaces until the 60 character field was
		// full).
		if (line[pos] == '\n')
			break;

		*(var->var_desc + i) = line[pos++];
	}
	*(var->var_desc + i) = '\0';

	return 0;
}

////////////////////////////////////////////////////////////////////////////////
// processVssLineBuffer                                                       //
//                                                                            //
// Same as processVssLineFile, but operates on lines from a buffer that       //
// holds a VSS spec retrieved from the internet (from EDB).                   //
////////////////////////////////////////////////////////////////////////////////

void processVssLineBuffer(char** buf_pos, struct variant* var)
{
	char* c;
	int i;

	BOOL has_link = FALSE;

	// Skip to first field to read (family description)
	*buf_pos += 9;

	// Read family description
	for (i = 0; i < FAM_DESC_LENGTH; i++) {
		*(var->fam_desc + i) = **buf_pos;
		(*buf_pos)++;
	}
	*(var->fam_desc + i) = '\0';
	(*buf_pos)++;

	// Check if this line has a link. If it does, skip text to get to
	// the symbol field.
	c = strstr(*buf_pos, ".pdf\">");
	if (c != NULL && c < strchr(*buf_pos, '\n')) {
		has_link = TRUE;
		*buf_pos = c + 6;
	}

	// Read Symbol
	for (i = 0; i < SYMBOL_LENGTH; i++) {
		*(var->symbol + i) = **buf_pos;
		(*buf_pos)++;
	}
	*(var->symbol + i) = '\0';
	(*buf_pos)++;

	// If the line had a link, skip the closing </a> tag
	if (has_link)
		*buf_pos += 4;

	// Read IDVAR6
	for (i = 0; i < 6; i++) {
		*(var->idvar6 + i) = **buf_pos;
		(*buf_pos)++;
	}
	*(var->idvar6 + i) = '\0';
	(*buf_pos)++;

	// Read variant description
	for (i = 0; i < 60; i++) {
		// This if statement was added because of a bug in the
		// input. Variant 260-006 had a variant description
		// that was less than 60 characters (meaning it was not
		// padded with spaces until the 60 character field was
		// full).
		if (**buf_pos == '\n')
			break;

		*(var->var_desc + i) = **buf_pos;
		(*buf_pos)++;
	}
	*(var->var_desc + i) = '\0';
	(*buf_pos)++;
}

////////////////////////////////////////////////////////////////////////////////
// parseVssFile                                                               //
//                                                                            //
// This function allocates the struct variant array on the heap, and          //
// populates it by calling processVssLineFile() for every line with variant   //
// data in a file containing a VSS spec.                                      //
//                                                                            //
// When this function is exited, the struct variant array will hold storage   //
// allocated on the heap. It will be up to the appliation to free this data   //
// later. This is done every time a spec is analyzed (whether it is retrieved //
// from the internet or from a file), if an error occurs while parsing one    //
// of the CSV resource files, or when the application is exited.              //
//                                                                            //
// If an error occurs, the spec analysis is stopped, and the dash is shown    //
// blank.                                                                     //
////////////////////////////////////////////////////////////////////////////////

struct variant* parseVssFile(const char* file_path, int* num_var)
{
	struct variant* var_list;
	FILE* fp;
	fpos_t fpos;

	int i;

	if ((fopen_s(&fp, file_path, "r")) != 0)
		return NULL;

	if (skipToVariantsFile(fp, &fpos)) {
		fclose(fp);
		return NULL;
	}
	fsetpos(fp, &fpos);

	if ((*num_var = countLinesFile(fp)) < 0) {
		fclose(fp);
		return NULL;
	}
	fsetpos(fp, &fpos);

	if ((var_list = malloc(sizeof(struct variant) * (*num_var))) == NULL) {
		fclose(fp);
		return NULL;
	}

	// !
	// At this point, var_list points to memory on the heap
	// !
	
	for (i = 0; i < *num_var; i++)
		if (processVssLineFile(fp, var_list + i) < 0) {
			fclose(fp);
			free(var_list);
			return NULL;
		}

	// At this point, var_list still points to memory on the heap. It will
	// be up to the application to ensure it is freed at some point.

	fclose(fp);
	return var_list;
}

////////////////////////////////////////////////////////////////////////////////
// parseVssBuffer                                                             //
//                                                                            //
// Same as parseVssFile, but operates on a buffer that holds a VSS spec       //
// retrieved from the internet (from EDB).                                    //
////////////////////////////////////////////////////////////////////////////////

struct variant* parseVssBuffer(char* buf_pos, int* num_var)
{
	struct variant* var_list;
	char* starting_pos = buf_pos;
	int i;

	if (!buf_pos) return NULL;

	if (skipToVariantsBuffer(&buf_pos)) {
		free(starting_pos);
		return NULL;
	}

	// At this point, this function's local copy of buf_pos points to the
	// first line with variant data in the buffer

	if ((*num_var = countLinesBuffer(buf_pos)) < 0) {
		free(starting_pos);
		return NULL;
	}

	// At this point, the local copy of buf_pos still points to the
	// first line with variant data in the buffer. countLinesBuffer()
	// received it's own copy of buf_pos.

	if ((var_list = malloc(sizeof(struct variant) * (*num_var))) == NULL) {
		free(starting_pos);
		return NULL;
	}

	// !
	// At this point, var_list points to memory on the heap
	// !

	for (i = 0; i < *num_var; i++)
		processVssLineBuffer(&buf_pos, var_list + i);

	// At this point, var_list still points to memory on the heap. It will
	// be up to the application to ensure it is freed at some point.

	free(starting_pos);
	return var_list;
}