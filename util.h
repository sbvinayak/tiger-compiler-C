#ifndef _UTIL_H_
#define _UTIL_H_
#include <assert.h>

typedef char *string;
typedef char bool;

#define TRUE 1
#define FALSE 0

void *checked_malloc(int);
string String(char *);

typedef struct U_boolList_ *U_boolList;
struct U_boolList_ {bool head; U_boolList tail;};
U_boolList U_BoolList(bool head, U_boolList tail);

//Added by Ming Zhou. For debugging only
void U_debug(char* module, char *message, ...);
void checked_free(void* pointer);
#endif
