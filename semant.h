/* 
 * TIGER's IR Translation Module
 *
*/

#ifndef _SEMANT_H_
#define _SEMANT_H_
bool anyErrors;

/* 
 * The main interface of this module.
 *
 * Translate the AST to a bunch of fragments organized in a list, each 
 * corresponding to a function or string. The order they are placed in  
 * the list is not necessarily consistent with the natural order they 
 * appear in the source code.
 * 
 * If any error has occured during the tranlsation, the external variable
 * anyErrors will be set TRUE.
 */
F_fragList SEM_transProg(A_exp exp);

/* ENV Module */

typedef struct E_enventry_ *E_enventry;

struct E_enventry_ {
	enum {
		E_varEntry,
		E_funEntry
	} kind;
	union {
		struct { 
			Tr_access access;
			Ty_ty ty; 
		} var;
		struct { 
			Tr_level level;
			Temp_label label;
			Ty_tyList formals; 
			Ty_ty result; 
		} fun;
	} u;
};

E_enventry E_VarEntry(Tr_access access, Ty_ty ty);
E_enventry E_FunEntry(Tr_level level, Temp_label label, Ty_tyList formals, Ty_ty result);

/* Ty_ty environment */
S_table E_base_tenv(void);

/* E_enventry environment */
S_table E_base_venv(void);

/* Type Checking Module */
struct expty {
	Tr_exp exp;
	Ty_ty ty;
};

struct expty expTy(Tr_exp exp, Ty_ty ty);

struct expty transVar(Tr_level level, S_table venv, S_table tenv, A_var v);

struct expty transExp(Tr_level level, S_table venv, S_table tenv, A_exp a);

Tr_exp transDec(Tr_level level, S_table venv, S_table tenv, A_dec d);

void transTy(S_table tenv, S_symbol sym, A_ty a);

#endif
