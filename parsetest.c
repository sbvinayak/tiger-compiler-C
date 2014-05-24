#include <stdio.h>
#include "util.h"
#include "errormsg.h"
#include "symbol.h"
#include "absyn.h"

extern int yyparse(void);
extern A_exp absyn_root;

/* parse source file fname; 
   return abstract syntax data structure */
A_exp parse(string fname)
{EM_reset(fname);
 if (yyparse() == 0) /* parsing worked */
   return absyn_root;
 else return NULL;
}



int main(int argc, char **argv) {
 A_exp exp; 
 if (argc!=2) {fprintf(stderr,"usage: a.out filename\n"); exit(1);}
 exp = parse(argv[1]);  
#ifdef DISPLAY  
 print_absyn_exp(exp, 0);
 printf("\n");
#endif

 SEM_transProg(exp); 

 return 0;
}
