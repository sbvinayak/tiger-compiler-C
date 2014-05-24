/*
 * CS5161 Assignment 5
 * 
 * TIGER's IR Translation Module
 *
 * Ming Zhou (4465225)
 * Kuo-shih Tseng (4436736)
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "table.h"
#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"
#include "frame.h"
#include "absyn.h"
#include "translate.h"
#include "mipsframe.h"
#include "printtree.h"

#define debug(...) U_debug ( "[translate] ", __VA_ARGS__ )

/* Generic Stack Structure START */

/* Push a new node into the stack */
void GS_push(stack_node* plist, void* key){
	int size = sizeof(struct stack_node_);
	stack_node list = *plist;
	if(list==NULL){
		list = checked_malloc(size);
		list->key = key;
		list->next = NULL;
	} else {
		stack_node head = checked_malloc(size);
		head->key = key;
		head->next = list;
		list = head;
	}	
	*plist=list;
}

/* Pop a node from the stack */
void GS_pop(stack_node *plist){
	stack_node list = *plist;
	if(list!=NULL){
		stack_node head = list;
		list = head->next;
		checked_free(head);
	}
	*plist=list;
}

void* GS_peek(stack_node *plist){
	return (*plist)->key;
}

/* Empty the stack */
void GS_empty(stack_node *plist){
	while(*plist!=NULL){
		GS_pop(plist);
	}
}

/* Check if a node matching the given key is existent.
 * The 3rd arg is a function comparing the key of node and the given one (2nd arg).
 * It should return TRUE if the two are deemed to be equal.
 */
bool GS_check(stack_node list, void* key, bool (*compare)(void*, void*)){
	stack_node cursor;
	for(cursor = list; cursor!=NULL; cursor = cursor->next){
		if(compare(cursor->key, key)){
			return TRUE;
		}
	}
	return FALSE;
}
/* Generic Stack Structure END */

static stack_node loop_label_list = NULL;

static void LL_push(Temp_label label){
	GS_push(&loop_label_list, label);
}
static void LL_pop(){
	GS_pop(&loop_label_list);
}
static Temp_label LL_peek(){
	return GS_peek(&loop_label_list);
}

Tr_expList Tr_ExpList(Tr_exp head, Tr_expList tail){
	Tr_expList expList = (Tr_expList)checked_malloc(sizeof(struct Tr_expList_));
	expList->head = head;
	expList->tail = tail;
	return expList;
}

Tr_accessList Tr_AccessList(Tr_access head, Tr_accessList tail){
	Tr_accessList accList = (Tr_accessList)checked_malloc(sizeof(struct Tr_accessList_));
	accList->head = head;
	accList->tail = tail;
	return accList;
}

struct Tr_access_ {
	Tr_level level;
	F_access access;
};

struct Tr_level_ {
	Tr_level parent;
	F_frame frame;
	int depth; //DEBUG
};

static Tr_level outermostLevel = NULL;
Tr_level Tr_outermost(){
	if(outermostLevel==NULL){
		outermostLevel = (Tr_level)checked_malloc(sizeof(struct Tr_level_));
		outermostLevel->parent = NULL;
		outermostLevel->frame = F_newFrame(Temp_namedlabel("main"), NULL);
		outermostLevel->depth = 0;
	}
	return outermostLevel;
}

Tr_level Tr_newLevel(Tr_level parent, Temp_label name, U_boolList formals){
	Tr_level level = (Tr_level)checked_malloc(sizeof(struct Tr_level_));
	level->parent = parent;
	level->frame = F_newFrame(name, U_BoolList(TRUE, formals));//Add a new formal used as static link
	level->depth = parent->depth + 1;
	return level;
}

Tr_level Tr_getParent(Tr_level level){
	return (level==outermostLevel)?outermostLevel:level->parent;
}

void Tr_printLevel(Tr_level level){
	printf("Level: depth = %d; name = %s\n", level->depth, S_name(F_name(level->frame)));
}

Tr_accessList Tr_formals(Tr_level level){
	F_accessList faccList = F_formals(level->frame);
	Tr_accessList accList = NULL;
	Tr_accessList accList_head = NULL;

	for(;faccList!=NULL;faccList=faccList->tail){
		if(accList_head==NULL){
			accList = (Tr_accessList)checked_malloc(sizeof(struct Tr_accessList_));
			accList_head = accList;
		} else {
			accList->tail = (Tr_accessList)checked_malloc(sizeof(struct Tr_accessList_));
			accList = accList->tail;
		}
		accList->head = (Tr_access)checked_malloc(sizeof(struct Tr_access_));
		accList->head->level = level;
		accList->head->access = faccList->head;
	}
	if(accList!=NULL){
		accList->tail = NULL;
	}

/* Problematic on 64-bit machine
	//1) Calculate the number of formals
	int i = 0;
	F_accessList faccList = F_formals(level->frame);
	F_accessList cursor;
	for(cursor=faccList; cursor!=NULL; cursor=cursor->tail){
		i++;
	}

	//2) Allocate mem for the accesse list
	int acc_size = sizeof(struct Tr_access_);
	int acc_list_size = sizeof(struct Tr_accessList_);
	int total_list_size = (i+1) * acc_list_size;
	Tr_accessList accList = (Tr_accessList)checked_malloc(total_list_size);
	Tr_accessList accList_head = accList;

	//Construct a list in the allocated mem
	for(;faccList!=NULL;faccList=faccList->tail){
		if(faccList->head!=NULL){
			accList->head = (Tr_access)checked_malloc(acc_size);
			accList->head->level = level;
			accList->head->access = faccList->head;

			// Make the tail point to the head of next node	
			//  |---- node  1 ----||---- node  2 ----|
			//	|--head--|--tail--||--head--|--tail--|
			//                 |____+
			accList->tail = accList + acc_list_size/8;

			// Move to next node
			accList = accList->tail;
		}
	}
	accList->tail = NULL;
*/
	return accList_head;
}

Tr_access Tr_allocLocal(Tr_level level, bool escape){
	F_access acc = F_allocLocal(level->frame, escape);	
	Tr_access tracc = (Tr_access)checked_malloc(sizeof(struct Tr_access_));	
	tracc->access = acc;
	tracc->level = level;

	return tracc;
}

static patchList PatchList(Temp_label *head, patchList tail){
	patchList pl = (patchList)checked_malloc(sizeof(struct patchList_));	
	pl->head = head;
	pl->tail = tail;
	return pl;
}

static void doPatch(patchList list, Temp_label label){
	Temp_label *l;
	for(; list!=NULL; list=list->tail){
		l = list->head;
		if(l!=NULL){
			*l = label;
		}
	}
}

//Constructors
static Tr_exp Tr_Ex(T_exp ex){
	Tr_exp e = (Tr_exp)checked_malloc(sizeof(struct Tr_exp_));	
	e->kind = Tr_ex;
	e->u.ex = ex;
	return e;
}

static Tr_exp Tr_Nx(T_stm nx){
	Tr_exp e = (Tr_exp)checked_malloc(sizeof(struct Tr_exp_));	
	e->kind = Tr_nx;
	e->u.nx = nx;
	return e;
}

static Tr_exp Tr_Cx(patchList trues, patchList falses, T_stm stm){
	Tr_exp e = (Tr_exp)checked_malloc(sizeof(struct Tr_exp_));	
	e->kind = Tr_cx;
	e->u.cx.trues = trues;
	e->u.cx.falses = falses;
	e->u.cx.stm = stm;
	return e;
}

//Extractors
static T_exp unEx(Tr_exp e){
	Temp_temp r;
	Temp_label t,f;
	T_exp ex;

	switch(e->kind){
	case Tr_ex:
		ex = e->u.ex;
		break;
	case Tr_cx:
		r = Temp_newtemp();
		t = Temp_newlabel();
		f = Temp_newlabel();
		doPatch(e->u.cx.trues, t);
		doPatch(e->u.cx.falses, f);
		ex = T_Eseq(T_Move(T_Temp(r), T_Const(1)),
				T_Eseq(e->u.cx.stm,
					T_Eseq(T_Label(f),
						T_Eseq(T_Move(T_Temp(r), T_Const(0)),
							T_Eseq(T_Label(t),
								T_Temp(r)
							)
						)
					)
				)
			);
		break;
	case Tr_nx:
		ex = T_Eseq(e->u.nx, T_Const(0));
		break;
	}

	return ex;
}

static T_stm unNx(Tr_exp e){
	Temp_label t,f;
	T_stm st;

	switch(e->kind){
	case Tr_ex:
		st = T_Exp(e->u.ex);
		break;
	case Tr_cx:
		t = Temp_newlabel();
		f = Temp_newlabel();
		doPatch(e->u.cx.trues, t);
		doPatch(e->u.cx.falses, f);
		st = T_Seq(e->u.cx.stm, T_Seq(T_Label(t), T_Label(f)));
		break;
	case Tr_nx:
		st = e->u.nx;
		break;
	}
	return st;
}

static struct Cx unCx(Tr_exp e){
	struct Cx cx;

	switch(e->kind){
	case Tr_ex:
		cx.stm = T_Cjump(T_ne, e->u.ex, T_Const(0), NULL, NULL);
		cx.trues = PatchList(&cx.stm->u.CJUMP.true, NULL);
		cx.falses = PatchList(&cx.stm->u.CJUMP.false, NULL);
		break;
	case Tr_cx:
		cx = e->u.cx;
		break;
	case Tr_nx://Should not reach here
		cx.trues = NULL;
		cx.falses = NULL;
		cx.stm = e->u.nx;
		break;
	}
	return cx;
}

/*
struct F_access_ {
	enum {inFrame, inReg} kind;
	union {
		int offset;
		Temp_temp reg;
	} u;
};

T_exp T_Binop(T_binOp, T_exp, T_exp);
T_exp T_Mem(T_exp);
*/

void Tr_printResult(){
    F_fragList frags = Tr_getResult();
    int cnt = 1;	
	for(; frags!=NULL; frags=frags->tail, cnt++){
		if(frags->head!=NULL){
			printf("Frag %d:\n", cnt);fflush(stdout);
			F_printFrag(frags->head);
		}
	}
}

void Tr_printTree(Tr_exp exp){
	printStmList(stdout, T_StmList(unNx(exp), NULL));
}

/* IR generation for each expression */

Tr_exp Tr_assignExp(Tr_exp lvalue, Tr_exp right){
debug("Tr_assignExp");

	if(right!=NULL){
		return Tr_Nx(T_Move(unEx(lvalue), unEx(right)));
	} else {
		return lvalue;
	}
}

Tr_exp Tr_forExp(Tr_exp var, Tr_exp low, Tr_exp high, Tr_exp body){
debug("Tr_forExp");

	T_stm st;
	T_exp v = unEx(var);
	Temp_label t = Temp_newlabel();
	Temp_label f = LL_peek();//Get done label from the list, which should have been prepared before this function is called.
	Temp_label start = Temp_newlabel();

	T_stm cond = T_Cjump(T_le, v, unEx(high), t, f);

	/*
		T_Move   var <- low
		T_Label  start
		T_CJump	 var <= high, t, f
		T_Label(t)
		body
		T_Move   var + 1
		T_Jump	 start
		T_Label(f)
	*/
	st = T_Seq(T_Move(v, unEx(low)),
			T_Seq(T_Label(start),
				T_Seq(cond,
					T_Seq(T_Label(t),
						T_Seq(unNx(body),
							T_Seq(T_Move(v, T_Binop(T_plus, v, T_Const(1))),
								T_Seq(
									T_Jump(
										T_Name(start), 
										Temp_LabelList(start, NULL)
									),
									T_Label(f)
								)
							)
						)
					)
				)
			)
		);

	//Pop the label as it's no longer used
	LL_pop();

	return Tr_Nx(st);
}

void Tr_genLoopDoneLabel(){
	//Put a new label into the label list
	Temp_label f = Temp_newlabel();
	LL_push(f);
debug("new loop done label generated");
}

Tr_exp Tr_breakExp(){
	//Get done label from the list, which should have been prepared before this function is called.
	Temp_label f = LL_peek();
	T_stm st = 
		T_Jump(
			T_Name(f), 
			Temp_LabelList(f, NULL)
		);

	return Tr_Nx(st);
}

Tr_exp Tr_whileExp(Tr_exp cond, Tr_exp body){
debug("Tr_whileExp");

	T_stm st;
	struct Cx cx_cond = unCx(cond);

	Temp_label t = Temp_newlabel();
	Temp_label f = LL_peek();//Get done label from the list, which should have been prepared before this function is called.
	Temp_label start = Temp_newlabel();

	doPatch(cx_cond.trues, t);
	doPatch(cx_cond.falses, f);

	/*
		T_Label  start
		cx_cond.stm (true->t, false->f)
		T_Label(t)
		body
		T_Jump	 start
		T_Label(f)
	*/
	st = T_Seq(T_Label(start),
			T_Seq(cx_cond.stm,
				T_Seq(T_Label(t),
					T_Seq(unNx(body),
						T_Seq(
							T_Jump(
								T_Name(start), 
								Temp_LabelList(start, NULL)
							),
							T_Label(f)
						)
					)
				)
			)
		);
	
	//Pop the label as it's no longer used
	LL_pop();

	return Tr_Nx(st);
}

Tr_exp Tr_ifExp(Tr_exp cond, Tr_exp thenb, Tr_exp elseb){
	T_exp ex;
	T_stm st;
	Tr_exp ret;
	struct Cx cx_cond = unCx(cond);

	Temp_label t = Temp_newlabel();
	Temp_label f = Temp_newlabel();
	Temp_label join = Temp_newlabel();

	doPatch(cx_cond.trues, t);
	doPatch(cx_cond.falses, f);
	
	if(elseb==NULL){
	//case 1: if-then
debug("Tr_ifExp: if-then");
		st = T_Seq(cx_cond.stm,
				T_Seq(T_Label(t),
					T_Seq(unNx(thenb),
						T_Label(f)
					)
				)
			);
		ret = Tr_Nx(st);
	} else {
	//case 2: if-then-else
		if(thenb->kind==Tr_nx && elseb->kind==Tr_nx){
debug("Tr_ifExp: if-then-else (2 nx)");
			//special case: two statements
			st = T_Seq(cx_cond.stm,
					T_Seq(T_Label(t),
						T_Seq(unNx(thenb),
							T_Seq(
								T_Jump(
									T_Name(join), 
									Temp_LabelList(join, NULL)
								),
								T_Seq(T_Label(f),
									T_Seq(unNx(elseb),
										T_Label(join)
									)
								)
							)
						)
					)
				);
			ret = Tr_Nx(st);
//TODO: special case - two cx.
		} else {
debug("Tr_ifExp: if-then-else");
			Temp_temp r = Temp_newtemp();
			ex = T_Eseq(cx_cond.stm,
					T_Eseq(T_Label(t),
						T_Eseq(T_Move(T_Temp(r), unEx(thenb)),
							T_Eseq(
								T_Jump(
									T_Name(join), 
									Temp_LabelList(join, NULL)
								),
								T_Eseq(T_Label(f),
									T_Eseq(T_Move(T_Temp(r), unEx(elseb)),
										T_Eseq(T_Label(join),
											T_Temp(r)
										)
									)
								)
							)
						)
					)
				);
			ret = Tr_Ex(ex);
		}

	}
	
	return ret;
}

Tr_exp Tr_simpleVar(Tr_access tr_acc, Tr_level level){
//printf("Tr_simpleVar\n");
	F_access access = tr_acc->access;
	T_exp exp;
	if(level!=tr_acc->level){
int cnt = 0;
//printf("  (Tr_simpleVar: Follow static link %d)\n", ++cnt);
		//Follow the static link
		exp = F_Exp(F_staticLink(), T_Temp(F_FP()));
		level = level->parent;
		//TODO check: level cannot be null
		while(level!=tr_acc->level){
//printf("  (Tr_simpleVar: Follow static link %d)\n", ++cnt);
			exp = F_Exp(F_staticLink(), exp);			
			level = level->parent;
			//TODO check: level cannot be null
		}
		exp = F_Exp(access, exp);
	} else {
//printf("  (Tr_simpleVar: Don't follow link)\n");
		//Just use the current frame pointer
		exp = F_Exp(access, T_Temp(F_FP()));
	}
	return Tr_Ex(exp);
}

Tr_exp Tr_fieldVar(Tr_exp base, int field_offset){
	return Tr_Ex(T_Mem(T_Binop(T_plus, unEx(base), T_Const(field_offset * F_wordSize))));
}

Tr_exp Tr_arrayVar(Tr_exp base, Tr_exp offset_exp){
	return Tr_Ex(T_Mem(T_Binop(T_plus, unEx(base), T_Binop(T_mul, unEx(offset_exp), T_Const(F_wordSize)))));
}

Tr_exp Tr_intExp(int int_val){
	return Tr_Ex(T_Const(int_val));
}

Tr_exp Tr_stringExp(string str_val){
	Temp_label tl = Temp_newlabel();
	F_String(tl, str_val);
	return Tr_Ex(T_Name(tl));
}

/* deprecated
void Tr_functionDec(Tr_level level, Temp_label label, Tr_exp body){
	T_stm stm;

	stm = 
		T_Seq(T_Label(label),
			T_Seq(T_Move(T_Temp(F_RV()), unEx(body)),
				T_Jump(F_RA(), NULL)
			)
		);

	//Put the frag into the list
	F_Proc(stm, level->frame);
}
*/

void Tr_procEntryExit(Tr_level level, Tr_exp body, Tr_accessList formals, Temp_label label){
	T_stm stm;

	stm = 
		T_Seq(
			T_Label(label),
			T_Move(T_Temp(F_RV()), unEx(body))
		);

	//Put the frag into the list
	F_Proc(stm, level->frame);
}

Tr_exp Tr_callExp(Tr_level caller_lvl, Tr_level callee_lvl, Temp_label fun_label, Tr_exp* argv, int args){

int z = 0;
int cnt = 0;
debug("caller = %d; callee = %d", caller_lvl->depth, callee_lvl->depth);
	//1) Get the static link for the called function
	T_exp slk;
	if(caller_lvl!=callee_lvl){
debug("  (Tr_callExp: Follow static link %d)", ++cnt);
		//Follow the static link
		slk = F_Exp(F_staticLink(), T_Temp(F_FP()));
		caller_lvl = caller_lvl->parent;
		//TODO check: caller_lvl cannot be null
		while(caller_lvl!=callee_lvl){
debug("  (Tr_callExp: Follow static link %d)", ++cnt);
			slk = F_Exp(F_staticLink(), slk);			
			caller_lvl = caller_lvl->parent;
			//TODO check: caller_lvl cannot be null
		}
	} else {
debug("  (Tr_callExp: Don't follow link)");
		//Just use the current frame pointer
		slk = F_Exp(F_staticLink(), T_Temp(F_FP()));
	}
debug("Static link done");

	//2) Create the argument list, including the static link
	//2.1) Allocate memory
	T_expList listp_head = NULL;
	if(args>0){
/* Problematic on 64-bit machine
		int node_size = sizeof(struct T_expList_)/8;
		T_expList listp = (T_expList)checked_malloc((args+1) * sizeof(struct T_expList_));
		listp_head = listp;

		//2.2) Initialize
		listp->head = slk;
		listp->tail = listp + node_size;
		listp = listp->tail;
		int i = 0;
		while(i < args){
			//Let head point to the next argument
			listp->head = unEx(*argv);

			//Let tail point to next head
			listp->tail = listp + node_size;

			//Move the base pointer over one node
			if(i<args-1){
				listp = listp->tail;
				argv++;
			}
			i++;
		}
		listp->tail = NULL;
*/
		T_expList listp = (T_expList)checked_malloc(sizeof(struct T_expList_));
		listp_head = listp;
		//2.2) Initialize
		listp->head = slk;
		listp->tail = NULL;
		int i = 0;		
		while(i < args){
			//Create new list on the tail
			listp->tail = (T_expList)checked_malloc(sizeof(struct T_expList_));
			listp = listp->tail;

			//Let head point to the next argument
			listp->head = unEx(*argv);

			//Let tail point to next head
			listp->tail = NULL;

			//Get next argument
			argv++;

			i++;
		}
	}
debug("argument list prepared");

	//3) Generate IR
	T_exp exp = T_Call(T_Name(fun_label), listp_head);

/*
int i = 0;
while(listp_head!=NULL){
printf("arg %d\n", i++);listp_head=listp_head->tail;
}
*/

debug("call done");
	return Tr_Ex(exp);
}

Tr_exp Tr_arithExp(A_oper oper, Tr_exp left, Tr_exp right){
	T_exp exp;

	switch(oper){
	case A_plusOp:
		exp = T_Binop(T_plus, unEx(left), unEx(right));
		break;
	case A_minusOp:
		exp = T_Binop(T_minus, unEx(left), unEx(right));
		break;
	case A_timesOp:
		exp = T_Binop(T_mul, unEx(left), unEx(right));
		break;
	case A_divideOp:
		exp = T_Binop(T_div, unEx(left), unEx(right));
		break;
	default:
		//Should not reach here
		printf("Illegal arithmetic operator");
		exit(2);
	}

	return Tr_Ex(exp);
}

Tr_exp Tr_logicExp(A_oper oper, Tr_exp left, Tr_exp right, bool isStrCompare){
	T_stm stm;
	patchList tl;
	patchList fl;

	if(isStrCompare){
		T_exp call = F_externalCall("stringEqual", T_ExpList(unEx(left), T_ExpList(unEx(right), NULL)));
		switch(oper){
		case A_eqOp:
			stm = T_Cjump(T_eq, call, T_Const(1), NULL, NULL);
			break;
		case A_neqOp:
			stm = T_Cjump(T_eq, call, T_Const(0), NULL, NULL);
			break;
		default:
			//Should not reach here
			printf("Illegal logic operator on string comparison");
			exit(2);
		}
	} else {
		switch(oper){
		case A_eqOp:
			stm = T_Cjump(T_eq, unEx(left), unEx(right), NULL, NULL);
			break;
		case A_neqOp:
			stm = T_Cjump(T_ne, unEx(left), unEx(right), NULL, NULL);
			break;
		case A_ltOp:
			stm = T_Cjump(T_lt, unEx(left), unEx(right), NULL, NULL);
			break;
		case A_gtOp:
			stm = T_Cjump(T_gt, unEx(left), unEx(right), NULL, NULL);
			break;
		case A_leOp:
			stm = T_Cjump(T_le, unEx(left), unEx(right), NULL, NULL);
			break;
		case A_geOp:
			stm = T_Cjump(T_ge, unEx(left), unEx(right), NULL, NULL);
			break;
		default:
			//Should not reach here
			printf("Illegal logic operator");
			exit(2);
		}
	}

	tl = PatchList(&stm->u.CJUMP.true, NULL);
	fl = PatchList(&stm->u.CJUMP.false, NULL);
	return Tr_Cx(tl, fl, stm);
}

Tr_exp Tr_arrayExp(Tr_exp size, Tr_exp init){
debug("Tr_arrayExp");

/* Dynamic array creation
		T_Move   var <- 0
		T_Label  start
		T_CJump	 var < size, t, f
		T_Label(t)
		T_Move   offset <- base + var * wordSize
		T_Move   Mem(offset) <- init
		T_Move   var <- var + 1
		T_Jump	 start
		T_Label(f)
*/
/*
	T_exp call = F_externalCall("malloc", T_ExpList(unEx(size), NULL));
	Temp_temp _var = Temp_newtemp();
	Temp_temp _offset = Temp_newtemp();
	Temp_temp _base = Temp_newtemp();
	T_exp var = T_Temp(_var);
	T_exp offset = T_Temp(_offset);
	T_exp base = T_Temp(_base);
	Temp_label t = Temp_newlabel();
	Temp_label f = Temp_newlabel();
	Temp_label start = Temp_newlabel();

	T_exp ex =
		T_Eseq(T_Move(base, call),						//Allocate the memory; base := starting address of the allocated mem
		T_Eseq(T_Move(var, T_Const(0)),					//var := 0
		T_Eseq(T_Label(start),							//start:
		T_Eseq(T_Cjump(T_lt, var, unEx(size), t, f),	//if var < size then jump t else jump f
		T_Eseq(T_Label(t),								//t:
		T_Eseq(T_Move(offset, T_Binop(T_plus, base, T_Binop(T_mul, var, T_Const(F_wordSize)))),
														//offset := base + var * wordSize
		T_Eseq(T_Move(T_Mem(offset), unEx(init)),		//MEM[offset] := init
		T_Eseq(T_Move(var, T_Binop(T_plus, var, T_Const(1))),
														//var := var + 1
		T_Eseq(T_Jump(T_Name(start), Temp_LabelList(start, NULL)),
														//jump t
			T_Eseq(T_Label(f),							//f:
				base
			)
		)))))))));
*/

	//Use system function initArray, which returns pointer to the allocated memory
	T_exp ex = F_externalCall("initArray", T_ExpList(unEx(size), T_ExpList(unEx(init), NULL)));

	return Tr_Ex(ex);
}

Tr_exp Tr_recordExp_new(int cnt){
debug("Tr_recordExp_new fields=%d", cnt);
	T_exp call = F_externalCall("malloc", T_ExpList(T_Const(cnt*F_wordSize), NULL));

	Temp_temp r = Temp_newtemp();
	T_stm move = T_Seq(T_Move(T_Temp(r), call), NULL);
	T_exp ex = T_Eseq(move, T_Temp(r));
	return Tr_Ex(ex);
}

//Refer to FIGURE 7.4 (P168)
//struct {T_stm left, right;} SEQ;  T_stm_
//struct {T_stm stm; T_exp exp;} ESEQ;  T_exp_
void Tr_recordExp_app(Tr_exp te, Tr_exp init, bool last){
debug("Tr_recordExp_app %s", last==TRUE?"(LAST)":"");
	//1) Retrieve T_stm, which should be an ESEQ
	T_exp ex = unEx(te);

	//2) Prepare for iteration
	//The first sub-node for field initialization 
	T_stm* right = &(ex->u.ESEQ.stm->u.SEQ.right);
	//The register saving the base addr of record var
	T_exp temp = ex->u.ESEQ.exp;

	//3) Iterate over the SEQs, until the rightmost node is found, which should be NULL
	int i = 0;
	while(*right!=NULL){
		right = &((*right)->u.SEQ.right);
		i++;
	}

	//4) Create a new sub-node to initialize the next field
	T_stm move = T_Move(T_Mem(T_Binop(T_plus, temp, T_Const(i*F_wordSize))), unEx(init));

	//5) Substitue the rightmode node
	if(!last){
		*right = T_Seq(move, NULL);
	} else {
		*right = move;
	}	

}

//For test
void Tr_printTrExp(Tr_exp e){
	debug("dump Tr_Exp START");
	switch(e->kind){
	case Tr_ex:
		debug("  ex");
		debug("  addr = %d", (int)(e->u.ex));
		break;
	case Tr_cx:
		debug("  cx");
		debug("  addr = %d", (int)(e->u.cx.stm));
		break;
	case Tr_nx:
		debug("  nx");
		debug("  addr = %d", (int)(e->u.nx));
		break;
	}
	debug("dump Tr_Exp END");
}

Tr_exp Tr_seqExp(Tr_exp* array, int size){
    T_exp _pex = (T_exp)checked_malloc(sizeof(struct T_exp_));
	T_exp *pex = &_pex;//Always point to the rightmost node in the tree during the spanning
	T_exp ex_head;

	int i = 0;
	int last = size - 1;
	while(i<size){
Tr_printTrExp(array[i]);
		if(i!=last){
			//Not the last one: substitute the rightmost node by T_Eseq (stm-exp pair),
			//where stm is the Tr_exp
			*pex = T_Eseq(unNx(array[i]), NULL);
		} else {
			//Last one: substitute the rightmost node by Tr_exp itself
			*pex = unEx(array[i]);
		}

		//In case of first node, remember it
		if(i==0){
			ex_head = *pex;
		}

		//Not the last one: let pex point to the rightmost node again
		if(i!=last){
			pex = &((*pex)->u.ESEQ.exp);
		} 
		
		i++;
	}
//debug("Tr_seqExp done");
	return Tr_Ex(ex_head);
}

static Tr_exp Tr_NOOP = NULL;
Tr_exp Tr_voidExp(){
	if(Tr_NOOP==NULL){
		Tr_NOOP = Tr_Ex(T_Const(0));
	}
	return Tr_NOOP;
}

static Tr_exp Tr_NIL = NULL;
Tr_exp Tr_nilExp(){
	if(Tr_NIL==NULL){
		Tr_NIL = Tr_Ex(T_Mem(T_Const(0)));
	}
	return Tr_NIL;
}

F_fragList Tr_getResult(){
	return F_getFragList();
}

/* Only for local test *	//(add '/' after '*' to decomment)
int main(int argc, char **argv) {
	Tr_level level = Tr_newLevel(
		Tr_outermost(), 
		Temp_namedlabel("testfun"), 
		U_BoolList(FALSE, U_BoolList(TRUE, U_BoolList(TRUE, NULL))));

	Tr_accessList accList = Tr_formals(level);

	for(;accList!=NULL;accList=accList->tail){
		Tr_access acc = accList->head;
		if(acc!=NULL){//inFrame, inReg
			F_access facc = acc->access;
			printf("kind=%d\n", facc->kind);
			if(facc->kind==inFrame){
				printf("mem add=%d\n", facc->u.offset);
			}//Temp_temp reg;
		}
		
	}

	Tr_allocLocal(level, TRUE);
	Tr_allocLocal(level, FALSE);

	return 0;
}
//*/

