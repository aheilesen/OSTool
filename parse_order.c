#include "parse_order.h"

int GetLineBuffer(const char *src, char *dest, int dest_size)
{
   while ((*dest++ = *src++) != '\n') {
		if (--dest_size <= 0) {
			return -1;
		}
   }
	*(dest - 1) = '\0';
   return 0;
}

static void GetProperty(char **line, char *dest, int skip, int length)
{
	int count = 0;
	int i;

   for (i = 0; i < skip; i++) {
      (*line)++;
   }
   
   for (i = 0; i < length; i++) {
      if (**line == '&') {   // skip HTML entities
			count = 0;
			while (*(*line)++ != ';') {
				if (++count > 10)
					break;
			}
      }
		// Rarely, the variant description may be terminated with
		// an endline before occupying the full 60 character limit
      if ((*dest++ = *(*line)++) == '\n')
         return;
   }
}

static int skipToVariantsOrderBuffer(char **cur_pos)
{
	int i;
	int count;
	char line[LINE_LENGTH] = { 0 };

	for (i = 0; i < 14; i++) {
		count = 0;
		while (**cur_pos != '\n') {
			// '~' is used as the EOF marker. EOF should
			// not be encountered at this point.
			if (**cur_pos == '~') {
				return -1;
			}
			// In a valid spec, no line before the variants
			// begin should be 210 characters or more in length.
			if (++count > 250) {
				return -2;
			}
			(*cur_pos)++;
		}
		(*cur_pos)++;
	}

	// Check for presence of "PRODUCT CLASS" which should exist
	// on the first line of the variant list. Can't use strstr()
	// on the buffer because lines aren't null-terminated.
	if (GetLineBuffer(*cur_pos, line, LINE_LENGTH))
		return -3;
	if (strstr(line, "PRODUCT CLASS") == NULL)
		return -4;

	return 0;
}

static int countLinesOrderBuffer(char *buf)
{
	int i = 0;
	int count;

	while (i < MAX_VARIANTS) {
		count = 0;
		while (*buf != '\n') {
			// '~' is used as the EOF marker. EOF should
			// not be encountered at this point.
			if (*buf == '~') {
				return -1;
			}
			// In a valid spec, a line containing variant data should not be
			// 500 characters or more in length. In practice they will be much
			// shorter; this is just a safeguard.
			if (++count > 500) {
				return -2;
			}
			buf++;
		}
		i++;
		buf++;

		// A '<' in the first position denotes the end of the variant list
		if (*buf == '<') {
			return i;
		}
	}
	return -3;
}

static void processOrderLineBuffer(char **buf_pos, struct variant *var)
{
	GetIDVAR6(buf_pos, var->idvar6);
	GetFamDesc(buf_pos, var->fam_desc);
	GetSymbol(buf_pos, var->symbol);
	GetVarDesc(buf_pos, var->var_desc);
}

struct variant *parseOrderBuffer(char *buf, int *num_var)
{
	struct variant *var_list;
	char *starting_pos = buf;
	int i;

	if (!buf) return NULL;

	if (skipToVariantsOrderBuffer(&buf)) {
		free(starting_pos);
		return NULL;
	}

	// At this point, this function's local copy of buf points to the
	// first line with variant data in the buffer

	if ((*num_var = countLinesOrderBuffer(buf)) < 0) {
		free(starting_pos);
		return NULL;
	}

	// At this point, the local copy of buf still points to the
	// first line with variant data in the buffer. countLinesOrderBuffer()
	// received its own copy of buf.

	if ((var_list = malloc(sizeof(struct variant) * (*num_var))) == NULL) {
		free(starting_pos);
		return NULL;
	}

	// !
	// At this point, var_list points to memory on the heap
	// !

	for (i = 0; i < *num_var; i++) {
		processOrderLineBuffer(&buf, var_list + i);
	}

	// At this point, var_list still points to memory on the heap. It will
	// be up to the application to ensure it is freed at some point.

	free(starting_pos);
	return var_list;
}