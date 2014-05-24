/*
 * IR Translation Module
 */

#ifndef _TRANSLATE_H_
#define _TRANSLATE_H_

extern const int F_wordSize;

void checked_free(void* pointer);

/* Debugging */
void U_debug(char *module, char *message, ...);

/* Generic Stack Structure START */
typedef struct stack_node_ *stack_node;
struct stack_node_ {
	void *key;
	stack_node next;
};
void GS_push(stack_node *plist, void *key);
void GS_pop(stack_node *plist);
void GS_empty(stack_node *plist);
bool GS_check(stack_node list, void *key, bool (*compare)(void*, void*));
void* GS_peek(stack_node *plist);
/* Generic Stack Structure END */

typedef struct Tr_access_ *Tr_access;

typedef struct Tr_accessList_ *Tr_accessList;
struct Tr_accessList_ {
	Tr_access head;
	Tr_accessList tail;
};
Tr_accessList Tr_AccessList(Tr_access head, Tr_accessList tail);

typedef struct Tr_level_ *Tr_level;

Tr_level Tr_outermost(void);
Tr_level Tr_newLevel(Tr_level parent, Temp_label name, U_boolList formals);
Tr_accessList Tr_formals(Tr_level level);
Tr_access Tr_allocLocal(Tr_level level, bool escape);
Tr_level Tr_getParent(Tr_level parent);
void Tr_printLevel(Tr_level level);

typedef struct patchList_ *patchList;
struct patchList_ {
	Temp_label *head;
	patchList tail;
};
static patchList PatchList(Temp_label *head, patchList tail);

typedef struct Tr_exp_ *Tr_exp;
typedef struct Tr_expList_ *Tr_expList;
struct Tr_expList_ {
	Tr_exp head;
	Tr_expList tail;
};
Tr_expList Tr_ExpList(Tr_exp head, Tr_expList tail);

struct Cx {
	patchList trues; 
	patchList falses; 
	T_stm stm;
};
struct Tr_exp_ {
	enum {Tr_ex, Tr_nx, Tr_cx} kind;
	union {
		T_exp ex; 
		T_stm nx; 
		struct Cx cx;
	} u;
};

//For debugging only
void Tr_printTrExp(Tr_exp te);
void Tr_printResult();
void Tr_printTree(Tr_exp exp);

//Constructors
static Tr_exp Tr_Ex(T_exp ex);
static Tr_exp Tr_Nx(T_stm nx);
static Tr_exp Tr_Cx(patchList trues, patchList falses, T_stm stm);

//Extractors
static T_exp unEx(Tr_exp e);
static T_stm unNx(Tr_exp e);
static struct Cx unCx(Tr_exp e);

//Tranlation for expressions
Tr_exp Tr_nilExp();
Tr_exp Tr_assignExp(Tr_exp lvalue, Tr_exp right);
Tr_exp Tr_forExp(Tr_exp var, Tr_exp low, Tr_exp high, Tr_exp body);
Tr_exp Tr_whileExp(Tr_exp cond, Tr_exp body);
void Tr_genLoopDoneLabel();
Tr_exp Tr_breakExp();
Tr_exp Tr_ifExp(Tr_exp cond, Tr_exp thenb, Tr_exp elseb);
Tr_exp Tr_simpleVar(Tr_access, Tr_level);
Tr_exp Tr_fieldVar(Tr_exp base, int field_offset);
Tr_exp Tr_arrayVar(Tr_exp base, Tr_exp offset_exp);
Tr_exp Tr_intExp(int int_val);
Tr_exp Tr_stringExp(string str_val);
Tr_exp Tr_arithExp(A_oper oper, Tr_exp left, Tr_exp right);
Tr_exp Tr_logicExp(A_oper oper, Tr_exp left, Tr_exp right, bool isStrCompare);
Tr_exp Tr_arrayExp(Tr_exp size, Tr_exp init);
Tr_exp Tr_recordExp_new(int cnt);
void Tr_recordExp_app(Tr_exp te, Tr_exp init, bool last);
Tr_exp Tr_seqExp(Tr_exp*, int);
Tr_exp Tr_voidExp(void);
Tr_exp Tr_callExp(Tr_level caller_lvl, Tr_level callee_lvl, Temp_label fun_label, Tr_exp* argv, int args);
void Tr_procEntryExit(Tr_level level, Tr_exp body, Tr_accessList formals, Temp_label label);

//Fragment list
F_fragList Tr_getResult(void);

#endif
