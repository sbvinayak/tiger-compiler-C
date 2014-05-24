/*
 * util.c - commonly used utility functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "util.h"

//Added for test by Ming Zhou
//#define U_DEBUG

void U_debug(char* module, char *message, ...){
#ifdef U_DEBUG
	va_list ap;
	va_start(ap, message);
	fprintf(stdout, "%s", module);
	vfprintf(stdout, message, ap);
	fprintf(stdout, "\n");
	fflush(stdout);
	va_end(ap);
#endif
}

void *checked_malloc(int len)
{void *p = malloc(len);
 if (!p) {
    fprintf(stderr,"\nRan out of memory!\n");
    exit(1);
 }
 return p;
}

void checked_free(void* pointer){
 free(pointer);
}

string String(char *s)
{string p = checked_malloc(strlen(s)+1);
 strcpy(p,s);
 return p;
}

U_boolList U_BoolList(bool head, U_boolList tail)
{ U_boolList list = checked_malloc(sizeof(*list));
  list->head = head;
  list->tail = tail;
  return list;
}
