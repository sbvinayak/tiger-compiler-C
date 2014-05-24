/*
 * CS5161 Assignment 5
 * 
 * TIGER's IR Translation Module
 *
 * Ming Zhou (4465225)
 * Kuo-shih Tseng (4436736)
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "util.h"
#include "symbol.h" 
#include "errormsg.h"
#include "types.h"
#include "absyn.h"
#include "temp.h"
#include "tree.h"
#include "frame.h"
#include "translate.h"
#include "semant.h"

/* TEST ONLY Start */
static char z_str_ty[][12] = {
   "ty_record", "ty_nil", "ty_int", "ty_string", 
   "ty_array", "ty_name", "ty_void"};
void z_Ty_print(Ty_ty t, int ind);
#define debug(...) U_debug ( "[semant] ", __VA_ARGS__ )
/* TEST ONLY End */

static char str_operator[][4] = {
  "+", "-", "*", "/", 
  "=", "<>", "<", "<=", ">", ">="};
static Ty_ty transTyHeader(A_ty a);
static Ty_tyList makeFormalTyList(S_table tenv, A_fieldList params);
static bool isReferToOutside(bool* marked, int marked_len, S_symbol* syms, S_symbol refered);
static Ty_ty actual_ty(Ty_ty ty);
static bool looseTyCompare(Ty_ty a, Ty_ty b);
static void createEscapeList(U_boolList *formal_escs, A_fieldList fList);

/* Error Message for Type Checking START */
typedef enum {WARNING, ERROR, SEVERE, FATAL} sem_error_level;
/* "SEVERE" and "FATAL" means termination of type checking; 
 * in all the other aspects it is same to "ERROR".
 * Therefore it's not necessary to make distinction between the two for user.*/
static char str_sem_error_level[][15] = {"[warning] ", "[error] ", "[error] ", "[error] "};
static int strlen_sem_error_level[] = {10, 8, 8, 8}; 
static int SEM_alert_level = -1;

static void SEM_msg(sem_error_level level, int pos, char *message, ...);
static struct expty SEM_check_exit(sem_error_level level);

static struct expty *dummy_expty_p;
/* Error Message for Type Checking END */

/* Loop variants START */
static void VL_push(S_symbol sym);
static void VL_pop();
static bool VL_check(S_symbol sym);
static bool pointerCompare(void *a, void *b);

static stack_node variant_list = NULL;//variant_node 

static bool pointerCompare(void *a, void *b){
	return a==b?TRUE:FALSE;
}

static void VL_push(S_symbol sym){
	GS_push(&variant_list, sym);
}
static void VL_pop(){
	GS_pop(&variant_list);
}
static bool VL_check(S_symbol sym){
	return GS_check(variant_list, sym, pointerCompare);
}
static bool VL_isEmpty(){
	return variant_list == NULL;
}
/* Loop variants END */

/* Symbol list (for generic use) START */
static void SL_push(S_symbol sym);
static void SL_pop();
static bool SL_check(S_symbol sym);
static void SL_empty();

static stack_node symbol_list = NULL;

static void SL_push(S_symbol sym){
	GS_push(&symbol_list, sym);
}
static void SL_pop(){
	GS_pop(&symbol_list);
}
static bool SL_check(S_symbol sym){
	return GS_check(symbol_list, sym, pointerCompare);
}
static void SL_empty(){
	GS_empty(&symbol_list);
}
/* Symbol list (for generic use) END */

/* Initialization */
void init(){
	dummy_expty_p = (struct expty*) checked_malloc(sizeof(struct expty));
	dummy_expty_p->exp = Tr_voidExp();
	dummy_expty_p->ty = Ty_Void();
}

F_fragList SEM_transProg(A_exp exp) {
	init();

	S_table venv = E_base_venv();
	S_table tenv = E_base_tenv();
	//The main function actually should have no static link. But it's not a big deal
	//to allocate space for static link, so long as we don't use it.
	Tr_level mainLevel = Tr_newLevel(Tr_outermost(), Temp_namedlabel("_main_"), NULL);
	struct expty result = transExp(mainLevel, venv, tenv, exp);
	
	//Print main function
	//Tr_printTree(result.exp);

	//Print fragments
	//Tr_printResult();

	//Add main function to the fragment list
	Tr_procEntryExit(mainLevel, result.exp, Tr_formals(mainLevel), Temp_newlabel());

	anyErrors = (SEM_alert_level>=(int)ERROR)?TRUE:FALSE;
	
	return Tr_getResult();
}

/*
 * ATTENTION!
 * This function has no size check for the message passed in, 
 * It CANNOT output a message longer than 128 bytes, formatted.
 */
static void SEM_msg(sem_error_level level, int pos, char *message, ...) {
    va_list ap;
    va_start(ap, message);
    char s0[128];
    char* s = s0;

    strcpy(s, str_sem_error_level[level]);
    s = s + strlen_sem_error_level[level];
    vsprintf(s, message, ap);
    va_end(ap);
    EM_error(pos, s0);

    if(SEM_alert_level<(int)level){
        SEM_alert_level=(int)level;
    }

    if(level==FATAL){
        exit(1);
    }
}

/*
 * Check if an error of the given level has ever been thrown. If so,
 * terminates the program. Return the dummy expty otherwise.
 *
 */
static struct expty SEM_check_exit(sem_error_level level){
    if(SEM_alert_level>=(int)level){
		exit(1);
	}
	return *dummy_expty_p;
}

/* Type Checking Module */
#define EXACT_ARGS 0
#define LESS_ARGS 1
#define MORE_ARGS 2
struct expty transExp(Tr_level level, S_table venv, S_table tenv, A_exp a){
	//For common use	
	int i;
	struct expty expty;
	A_exp exp;
	
	//A_letExp
	A_decList d;

	//A_callExp
	Ty_tyList formals;
	A_expList args;
	Ty_ty ty;
	E_enventry entry;
	int arg_number;
	Tr_exp* arg_exps;

	//A_opExp
	struct expty left_expty; 
	struct expty right_expty; 
	Ty_ty left_ty;
	Ty_ty right_ty;
	A_oper oper;
    bool isStrCompare;

	//A_recordExp
	Ty_fieldList tyfList;
	Ty_field tyField;
	A_efieldList eFields;
	A_efield a_efield;
	int cnt;

	//A_ifExp, A_whileExp
	struct expty true_exp;
	struct expty false_exp;
	struct expty test_exp;
	bool if_then_else;

	//A_forExp, A_whileExp
	S_symbol var_sym;
	struct expty for_bottom_exp;
	struct expty for_upper_exp;
	Tr_access tr_acc;
	struct expty body_exp;
	struct expty var_exp;

	//A_seqExp
	A_expList expList;
	Tr_exp* te_array;

	//A_arrayExp
	Ty_ty ele_ty;
	struct expty ele_expty;

	//For IR translation
	Tr_exp te;
 
//For test	
Ty_tyList tyList;

	if(a==NULL){
		return *dummy_expty_p;
	}
	
	/*
	 * A_varExp, A_nilExp, A_intExp, A_stringExp, A_callExp,
	 * A_opExp, A_recordExp, A_seqExp, A_assignExp, A_ifExp,
	 * A_whileExp, A_forExp, A_breakExp, A_letExp, A_arrayExp
	 */
	switch(a->kind) {
	case A_varExp:
		return transVar(level, venv, tenv, a->u.var);
	case A_nilExp:
		te = Tr_nilExp();
		return expTy(te, Ty_Nil());
	case A_intExp:
		te = Tr_intExp(a->u.intt);
		return expTy(te, Ty_Int());
	case A_stringExp:
		te = Tr_stringExp(a->u.stringg);
		return expTy(te, Ty_String());
	case A_callExp://struct {S_symbol func; A_expList args;} call;
debug("A_callExp");
		arg_number = EXACT_ARGS;
		//1) Retrieve the function dec from env
		entry = S_look(venv, a->u.call.func); 
debug("  function name: %s", S_name(a->u.call.func));

		//2) Check if the function is defined
		if (entry != NULL && entry->kind == E_funEntry) {
			//3) Preprare an array to hold the arguments
			i = 0;
			for(formals = entry->u.fun.formals; formals!=NULL; formals=formals->tail){
				if(formals->head!=NULL){
					i++;
				}
			}	
			arg_exps = (i>0)?(Tr_exp*)checked_malloc(i * sizeof(Tr_exp)):NULL;

			//4) Check the arguments
			i = 0;
			for(formals = entry->u.fun.formals, args = a->u.call.args; 
				formals!=NULL && args!=NULL; 
				formals=formals->tail, args=args->tail, i++){
				ty = formals->head;
				exp = args->head;
				if(exp!=NULL && ty!=NULL){
					expty = transExp(level, venv, tenv, exp);
					if(actual_ty(expty.ty) != actual_ty(ty)){
						SEM_msg(SEVERE, exp->pos, 
							"Type of argument %d passed to function '%s' is incompatible with the declared type", 
							i+1, S_name(a->u.call.func));
					}
					//5) Put argument into the arg list
					arg_exps[i] = expty.exp;
				} else {
					if (exp==NULL && ty!=NULL){
						arg_number = LESS_ARGS;	
						break;
					} else if (exp!=NULL && ty==NULL) {
						arg_number = MORE_ARGS;
						break;
					}		
				}
			}
			if(formals != NULL && args == NULL){
				if(formals->head!=NULL){
					arg_number = LESS_ARGS;	
				}
			} else if (formals == NULL && args != NULL){
				if(args->head!=NULL){
					arg_number = MORE_ARGS;
				}
			} 
			if(arg_number == MORE_ARGS){
				SEM_msg(SEVERE, a->pos, 
					"More than necessary arguments are passed to function '%s'", S_name(a->u.call.func));
			} else if (arg_number == LESS_ARGS){
				SEM_msg(SEVERE, a->pos, 
					"Less than necessary arguments are passed to function '%s'", S_name(a->u.call.func));
			} 
			//Terminate in case of any error
			SEM_check_exit(SEVERE);

			//6) Generate IR tree
			te = Tr_callExp(level, Tr_getParent(entry->u.fun.level), entry->u.fun.label, arg_exps, i);
			checked_free(arg_exps);

			return expTy(te, entry->u.fun.result);
		} else {
			SEM_msg(FATAL, a->pos, "Undefined function: %s", S_name(a->u.call.func));
			return SEM_check_exit(FATAL);//Never reach here
		}
	case A_opExp://struct {A_oper oper; A_exp left; A_exp right;} op;
debug("A_opExp");
		left_expty = transExp(level, venv, tenv, a->u.op.left);
		right_expty = transExp(level, venv, tenv, a->u.op.right);
		left_ty = actual_ty(left_expty.ty);
		right_ty = actual_ty(right_expty.ty);
		oper = a->u.op.oper;
		switch(oper){//A_plusOp, A_minusOp, A_timesOp, A_divideOp, A_eqOp, A_neqOp, A_ltOp, A_leOp, A_gtOp, A_geOp
		//1) For arithmetic expressions where op is +, -, *, /: 
		//must be integer arguments, and return integer result.
		case A_plusOp:
		case A_minusOp:
		case A_timesOp:
		case A_divideOp:
			if(left_ty->kind != Ty_int){
				SEM_msg(SEVERE, a->pos, "Illegal operation: the left side of '%s' operation is not an integer.", str_operator[oper]);
			}
			if(right_ty->kind != Ty_int){
				SEM_msg(SEVERE, a->pos, "Illegal operation: the right side of '%s' operation is not an integer.", str_operator[oper]);
			}
			//Terminate in case of any error
			SEM_check_exit(SEVERE);

			te = Tr_arithExp(oper, left_expty.exp, right_expty.exp);
			return expTy(te, Ty_Int());
		//2) For comparison expressions where op is <, <=, >, >=:
		//must be either integer or string; the two sides must be of the same type; 
		case A_ltOp:
		case A_leOp:
		case A_gtOp:
		case A_geOp:
			if(left_ty->kind != Ty_int && left_ty->kind != Ty_string) {
				SEM_msg(SEVERE, a->pos, "Illegal operation: the left side of '%s' operation is not an integer or string", str_operator[oper]);
			}
			if(right_ty->kind != Ty_int && right_ty->kind != Ty_string) {
				SEM_msg(SEVERE, a->pos, "Illegal operation: the right side of '%s' operation is not an integer or string", str_operator[oper]);
			}
			//Terminate in case of any error
			SEM_check_exit(SEVERE);

			if(left_ty->kind != right_ty->kind){
				SEM_msg(FATAL, a->pos, "The operands of '%s' operation must be of the same type (either integer or string)", str_operator[oper]);
			} else {
				te = Tr_logicExp(oper, left_expty.exp, right_expty.exp, FALSE);//FIXME: need built-in function to support lexicographic comparison
				return expTy(te, Ty_Int());			
			}
			break;
		//3) For comparison expressions where op is ==, <>:
		//must be integer, string, record or array; the two sides must be of same type; 
		case A_eqOp:
		case A_neqOp:
			if(left_ty->kind != Ty_int && left_ty->kind != Ty_string
				&& left_ty->kind != Ty_array && left_ty->kind != Ty_record && left_ty->kind != Ty_nil) {
				SEM_msg(SEVERE, a->pos, 
				"Illegal operation: the left side of '%s' operation is not one of the following types: integer, string, record, array", 
				str_operator[oper]);
			}
			if(right_ty->kind != Ty_int && right_ty->kind != Ty_string
				&& right_ty->kind != Ty_array && right_ty->kind != Ty_record && right_ty->kind != Ty_nil) {
				SEM_msg(SEVERE, a->pos, 
				"Illegal operation: the right side of '%s' operation is not one of the following types: integer, string, record, array", 
				str_operator[oper]);
			}
			if(left_ty->kind == Ty_nil && right_ty->kind == Ty_nil) {
				SEM_msg(SEVERE, a->pos, 
				"Illegal operation: the both sides of '%s' operation cannot be NIL simultaneously", 
				str_operator[oper]);
			}
			//Terminate in case of any error
			SEM_check_exit(SEVERE);

			if(left_ty->kind != right_ty->kind){
				//Allows one of the operands is NIL
				if(!(left_ty->kind == Ty_nil || right_ty->kind == Ty_nil)){
					SEM_msg(FATAL, a->pos, 
					"The operands of '%s' operation must be of the same type (one of the following types: integer, string, record, array)", 
					str_operator[oper]);
				}
			}

			isStrCompare = (left_ty->kind == Ty_string && right_ty->kind == Ty_string)?TRUE:FALSE;

			te = Tr_logicExp(oper, left_expty.exp, right_expty.exp, isStrCompare);
			return expTy(te, Ty_Int());
		}
		break;
	case A_recordExp://struct {S_symbol typ; A_efieldList fields;} record;
debug("A_recordExp");
		arg_number == EXACT_ARGS;
		//Recursively check each field. 
		// * All the fields must be initialized;
		// * Fields must be initialized in the declared order;
		// * Return expty of record type.
		ty = S_look(tenv, a->u.record.typ);

		//1) Check if the record expression's leading ID is a declared record type
		if(ty!=NULL && ty->kind == Ty_record){
			//1) Calculate the number of fields
			for(tyfList=ty->u.record, cnt=0; tyfList!=NULL; tyfList=tyfList->tail){
				if(tyfList->head!=NULL){
					cnt++;
				}
			}
//debug("record fields in total: %d", cnt);
			te = Tr_recordExp_new(cnt);

			//2) Recursively check each field
			for(tyfList=ty->u.record, eFields=a->u.record.fields, i=1; 
				tyfList!=NULL && eFields!=NULL; 
				tyfList=tyfList->tail, eFields=eFields->tail, i++){
				tyField = tyfList->head;
				a_efield = eFields->head;
//debug("record field");
				if(tyField!=NULL && a_efield!=NULL){
//debug("record field init: %d", i);
					//2.1) Check field name's consistency
					if(tyField->name != a_efield->name){
						SEM_msg(SEVERE, a_efield->exp->pos, 
							"The field %d initialized for record '%s' is inconsistent with the declared field's name '%s'", 
							i, S_name(a->u.record.typ), S_name(tyField->name));
					} else {
						//2.2) Check the compatibility between the declared field type and that of the initializer
						expty = transExp(level, venv, tenv, a_efield->exp);
						if(!looseTyCompare(expty.ty, tyField->ty)){
							SEM_msg(SEVERE, exp->pos, 
								"Type of field %d initialized for record '%s' is incompatible with the declared type", 
								i, S_name(a->u.record.typ));
						}	
						//2.3) Append the field initialization to the IR
//debug("             type: %s", z_str_ty[expty.ty->kind]);
						Tr_recordExp_app(te, expty.exp, i==cnt?TRUE:FALSE);//Initialize next field
					}
				} else {
					//In case one list is run out, but the other not, an error occurs.
					if (a_efield==NULL && tyField!=NULL){
						arg_number = LESS_ARGS;	
						break;
					} else if (a_efield!=NULL && tyField==NULL) {
						arg_number = MORE_ARGS;
						break;
					}		
				}
			}

			if(arg_number == MORE_ARGS){
				SEM_msg(SEVERE, a->pos, 
					"More than necessary fields are initialized for record '%s'", S_name(a->u.record.typ));	
			} else if (arg_number == LESS_ARGS || (tyfList != NULL && tyfList->head != NULL)){
				SEM_msg(SEVERE, a->pos, 
					"Less than necessary fields are initialized for record '%s'", S_name(a->u.record.typ));
			} 

			//Terminate in case of any error
			SEM_check_exit(SEVERE);

			//3) Return the result
			return expTy(te, ty);
		} else {
			SEM_msg(SEVERE, a->pos, "Undefined type '%s'", S_name(a->u.record.typ));
		}

		//Terminate in case of any error
		return SEM_check_exit(SEVERE);
	case A_assignExp://{A_var var; A_exp exp;} assign;
debug("A_assignExp");
		//1) Check if the var to assign to is a loop variant
		var_sym = a->u.assign.var->u.simple;
		if(VL_check(var_sym)){
			SEM_msg(SEVERE, a->u.assign.var->pos, "Loop variant '%s' cannot be assigned to", S_name(var_sym));
		}

		//1) Translate the assignee. If the variable is not defined, 
		//an error would have occured in transVar() function.
		var_exp = transVar(level, venv, tenv, a->u.assign.var);

		//2) Translate the expression to assign
		expty = transExp(level, venv, tenv, a->u.assign.exp);

		//3) Check if the assignee has a type incompatible with the expression at the right side
		ty = actual_ty(var_exp.ty);
		if(ty != NULL && !(looseTyCompare(ty, expty.ty))){
			SEM_msg(ERROR, a->u.assign.var->pos, 
				"Variable is assigned with a value whose type is incompatible with the declared one");
			//NOTE: In the message above, it'd be better to print the name of variable, but this is hard 
			//(and very inefficient) to do with the current implementation of AST

			//3.1) In this case, assign nothing to it. This is just a hack to make the type-checking continue.
			return expTy(Tr_assignExp(var_exp.exp, NULL), Ty_Void());
		}

		//Terminate in case of any error
		SEM_check_exit(SEVERE);

		//4) Return the translation of the whole assignment
		return expTy(Tr_assignExp(var_exp.exp, expty.exp), Ty_Void());
	case A_seqExp://A_expList seq;
debug("A_seqExp");
		//1) Calculate the number of exps in the list
		i = 0;
		for(expList = a->u.seq; expList!=NULL; expList = expList->tail){
			if(expList->head!=NULL){
				i++;
			}
		}

		//2) If the exp list has no exp, create one to return
		if(i==0){
			expty = expTy(Tr_voidExp(), Ty_Void());
		} else {
			//3) Create an array to hold IR for each sub expression
			te_array = (Tr_exp*)checked_malloc(i * sizeof(Tr_exp));

			//4) Type check each exp in order
			for(expList = a->u.seq, i=0; expList!=NULL; expList = expList->tail){
				exp = expList->head;
				if(exp!=NULL){
					//4.1) Translate each expression
					expty = transExp(level, venv, tenv, exp);

					//4.2) Put the IR into the array
					te_array[i] = expty.exp;
//debug(" A_seqExp subexp allocated %d", (int)te_array[i]);
//Tr_printTrExp(te_array[i]);
					i++;
				}
			}
debug(" A_seqExp subexps prepared (%d in total)", i);
			//5) Translate the entire sequence; 
			//   Use the type of last expression as the type of sequence
			expty = expTy(Tr_seqExp(te_array, i), expty.ty);
			checked_free(te_array);
		}

		return expty;
	case A_ifExp://struct {A_exp test, then, elsee;} iff
debug("A_ifExp");
		//1) Translate the test expression
		test_exp = transExp(level, venv, tenv, a->u.iff.test);
		if(actual_ty(test_exp.ty)->kind != Ty_int){
			SEM_msg(FATAL, a->u.iff.test->pos, "The condition test of IF expression must be of integer type");		
		}

		//2) Translate the true-branch expression
		true_exp = transExp(level, venv, tenv, a->u.iff.then);

		//3) Translate the false-branch expression		
		if(a->u.iff.elsee != NULL){
			if_then_else = TRUE;
			false_exp = transExp(level, venv, tenv, a->u.iff.elsee);
		} else {
			if_then_else = FALSE;
		}

		//4) Test the type consistency
		if(!if_then_else){
			//4.1) if it's IF-THEN expression, no value shall be returned from true-branch.
			if(true_exp.ty->kind != Ty_void){
				SEM_msg(WARNING, a->u.iff.then->pos, "Value returned from IF-THEN expression is ignored");	
			}
			expty = expTy(Tr_ifExp(test_exp.exp, true_exp.exp, NULL), Ty_Void());
		} else {
			//4.2) if it's IF-THEN-ELSE expression, the two branches shall return values of same type
			if(!looseTyCompare(true_exp.ty, false_exp.ty)){
				SEM_msg(FATAL, a->u.iff.then->pos, "Value returned from THEN branch must share the same type of that from ELSE branch");	
			}
			expty = expTy(Tr_ifExp(test_exp.exp, true_exp.exp, false_exp.exp), 
							actual_ty(true_exp.ty)->kind!=Ty_nil?true_exp.ty:false_exp.ty);//either that is representative of the actual type
		}

		return expty;
	case A_whileExp://struct {A_exp test, body;} whilee
debug("A_whileExp");
		//1) Add a dummy loop variant to variant list, so that VL_isEmpty() will return FALSE
		//By this hack we can check if an exp is in a while/for loop by calling VL_isEmpty(),
		//useful for determining the legality of A_breakExp.
		VL_push(NULL);
		
		//Generate a new done label to be used later by Tr_whileExp(), 
		//and possibly Tr_breakExp() within the loop body
		Tr_genLoopDoneLabel();

		//2) Translate the test expression
		test_exp = transExp(level, venv, tenv, a->u.whilee.test);
		if(actual_ty(test_exp.ty)->kind != Ty_int){
			SEM_msg(FATAL, a->u.whilee.test->pos, "The condition test of WHILE expression must be of integer type");		
		}

		//3) Translate the loop body
		body_exp = transExp(level, venv, tenv, a->u.whilee.body);

		if(actual_ty(body_exp.ty)->kind != Ty_void){
			SEM_msg(WARNING, a->pos, "Value returned from WHILE expression is ignored");	
		}

		//4) Pop the dummy loop variant	
		VL_pop();

		//5) Return valueless expty
		return expTy(Tr_whileExp(test_exp.exp, body_exp.exp), Ty_Void());
	case A_forExp://struct {S_symbol var; A_exp lo,hi,body; bool escape;} forr
debug("A_forExp");
		/* This is one of two possible places where a variable is declared, the other being let-expression. */
		//1) Enter in a special new scope in which the implicit declared loop variant is effective
		S_beginScope(venv);

		//2) Check if the variant's name has been used as loop variant in outer loops
		var_sym = a->u.forr.var;
		if(VL_check(var_sym)){
			SEM_msg(SEVERE, a->pos, "The name '%s' has been used by outer loop's variant", S_name(var_sym));
		} else {
		//2.1) Make note of this variant
			VL_push(var_sym);
		}

		//Generate a new done label to be used later by Tr_forExp(), 
		//and possibly Tr_breakExp() within the loop body
		Tr_genLoopDoneLabel();

		//3) Translate the bottom and upper
		for_bottom_exp = transExp(level, venv, tenv, a->u.forr.lo);
		for_upper_exp = transExp(level, venv, tenv, a->u.forr.hi);
		//3.1) Check the legality of bottom and upper
		if(actual_ty(for_bottom_exp.ty)->kind != Ty_int){
			SEM_msg(SEVERE, a->u.forr.lo->pos, "The lower bound of FOR expression must be of integer type");		
		}
		if(actual_ty(for_upper_exp.ty)->kind != Ty_int){
			SEM_msg(SEVERE, a->u.forr.hi->pos, "The upper bound of FOR expression must be of integer type");		
		}

		//4) Enter the variable into the env
		tr_acc = Tr_allocLocal(level, TRUE);//??? always escaping
		S_enter(venv, var_sym, E_VarEntry(tr_acc, Ty_Int()));
		var_exp = transVar(level, venv, tenv, A_SimpleVar(a->pos, var_sym));

		//5) Translate the loop body
		body_exp = transExp(level, venv, tenv, a->u.forr.body);
		if(actual_ty(body_exp.ty)->kind != Ty_void){
			SEM_msg(WARNING, a->pos, "Value returned from WHILE expression is ignored");	
		}

		// Terminate in case of any error
		SEM_check_exit(SEVERE);

		//6) Pop the loop variant, exit the special scope		
		VL_pop();
		S_endScope(venv);

		//7) Return valueless expty
		return expTy(Tr_forExp(var_exp.exp, for_bottom_exp.exp, for_upper_exp.exp, body_exp.exp), Ty_Void());
	case A_breakExp:
debug("A_breakExp");
		//1) Check if it's in a for/while loop
		if(VL_isEmpty()){
			SEM_msg(WARNING, a->pos, "Illegal break is ignored (can only break out of a WHILE or FOR loop)");
		}
		return expTy(Tr_breakExp(), Ty_Void());
	case A_letExp: 
debug("A_letExp");
		//1) Begin new scopes
		S_beginScope(venv);
		S_beginScope(tenv);

		//2) Prepare for the generation of resulting IR
		//2.1) Calculate the number of declarations between let and in
		i = 0;
		for(d=a->u.let.decs; d; d=d->tail){
			if(d->head!=NULL){
				i++;
			}
		}

		//2.2) Initialize an array to hold all the expressions, including
		//	-  the initialization for each variable declaration
		//  -  the seqExp between in and end
		//Therefore need i + 1 slots in total
		te_array = (Tr_exp*)checked_malloc((i+1) * sizeof(Tr_exp));
		i = 0;

		//3) Translate declarations in given order
		for(d=a->u.let.decs; d; d=d->tail){
			if(d->head!=NULL){
				te_array[i++] = transDec(level, venv, tenv, d->head);
			}
		}

		//4) Translate the body between in and end
		expty = transExp(level, venv, tenv, a->u.let.body);

		//4.1) Put the result into the array's last slot
		te_array[i] = expty.exp;
debug("A_letExp: weaving all together");
		//4.2) Generate a new IR tree weaving all together
		expty.exp = Tr_seqExp(te_array, i+1);
		checked_free(te_array);

		//5) Exit scopes
		S_endScope(tenv);
		S_endScope(venv);

		return expty;
	case A_arrayExp://{S_symbol typ; A_exp size, init;} array;
debug("A_arrayExp");
		//1) Check if the leading ID is an array type
		ty = S_look(tenv, a->u.array.typ);
		if(ty != NULL){
			ty = actual_ty(ty);
			if(ty->kind != Ty_array){
				SEM_msg(SEVERE, a->pos, "Type '%s' is not array", S_name(a->u.array.typ));	
			}
		} else {
			SEM_msg(FATAL, a->pos, "Array type '%s' doesn't exist", S_name(a->u.array.typ));	
		}

		//2) Check if the size is integer
		expty = transExp(level, venv, tenv, a->u.array.size);
		if(expty.ty->kind != Ty_int){
			SEM_msg(SEVERE, a->u.array.size->pos, "Size of an array must be of integer type");	
		}

		//3) Check if the initializer shares the same type as that of array's element type
		ele_ty = actual_ty(ty->u.array);
		ele_expty = transExp(level, venv, tenv, a->u.array.init);
		if(ele_ty != actual_ty(ele_expty.ty)){
			SEM_msg(SEVERE, a->u.array.init->pos, "Initializer of an array's element must follow the type as declared");	
		}

		//Terminate in case of any error
		SEM_check_exit(SEVERE);

		return expTy(Tr_arrayExp(expty.exp, ele_expty.exp), ty);
	default:
		//Should never reach here
		printf("Illegal type (%d) of AST node.\n", a->kind);
		exit(2);
	}

	//Should never reach here	
	exit(2);
}

struct expty transVar(Tr_level level, S_table venv, S_table tenv, A_var v){
	E_enventry entry;
	
	/* Type checking */
	Ty_fieldList fList;
	Ty_field field;
	struct expty exp;
	struct expty exp2;

	/* IR generation */
	Tr_exp te;
	int offset;

	switch(v->kind) {//A_simpleVar, A_fieldVar, A_subscriptVar
	case A_simpleVar:
debug("transVar: A_simpleVar = %s", S_name(v->u.simple));
		entry = S_look(venv, v->u.simple); 
		if (entry != NULL && entry->kind == E_varEntry) {
			te = Tr_simpleVar(entry->u.var.access, level);
			return expTy(te, actual_ty(entry->u.var.ty));
		} else {
			SEM_msg(FATAL, v->pos, "Undefined variable: %s", S_name(v->u.simple));
			break;
		}
	case A_fieldVar:
debug("transVar: A_fieldVar");
		//1) Translate the part ahead of '.', which should be a record
		exp = transVar(level, venv, tenv, v->u.field.var);
		if(exp.ty->kind != Ty_record){
			SEM_msg(FATAL, v->pos, "Field access to a non-record variable");
		}
		//2) Find the field in the record that matches the part following '.'
		offset = 0;
		for(fList = exp.ty->u.record; fList!=NULL; fList = fList->tail, offset++){
			field = fList->head;
			if(field!=NULL && field->name == v->u.field.sym){
debug("transVar: A_fieldVar = %s", S_name(field->name));
				te = Tr_fieldVar(exp.exp, offset);
				return expTy(te, actual_ty(field->ty));
			}
		}
		SEM_msg(FATAL, v->pos, "Undefined field '%s' in record variable", S_name(v->u.field.sym));
		break;
	case A_subscriptVar:
debug("transVar: A_subscriptVar");
		//1) Translate the part ahead of '[', which should be an array
		exp = transVar(level, venv, tenv, v->u.subscript.var);
		if(exp.ty->kind != Ty_array){
			SEM_msg(FATAL, v->pos, "Indexed access to a non-array variable");
			break;
		}

		//2) Translate the part between [ and ], which should be an int
		exp2 = transExp(level, venv, tenv, v->u.subscript.exp);
		if(exp2.ty->kind != Ty_int){
			SEM_msg(FATAL, v->pos, "Indexed access to array variable must be of integer type");
			break;
		}
		te = Tr_arrayVar(exp.exp, exp2.exp);

		//3) Get the type of array's element
		return expTy(te, actual_ty(exp.ty->u.array));
	}

	return SEM_check_exit(FATAL);//Never reach here
}

Tr_exp transDec(Tr_level level, S_table venv, S_table tenv, A_dec d){
	Tr_exp tr_exp;//To be returned
	A_nametyList types;
	A_namety type;
	A_fundecList functions;
	A_fundec head;
	Ty_ty resultTy;
	Ty_tyList formalTys;
	E_enventry entry;
	A_fieldList fList;
	Ty_tyList tList;
	struct expty e;
	Ty_ty varDecTy;
	Ty_ty ty;

	/* used for frame manipulation */
	Tr_access tr_acc;
	U_boolList formal_escs;
	Tr_level funLevel;
	Tr_accessList tr_formals;

	/* used in detecting cyclic type definition */
	A_ty* nameTys;
	int names = 0;
	bool changed;
	int ind;
	A_ty aty;
	bool* marked;
	A_ty refered;
	S_symbol* tySymbols;
	S_symbol var_sym;
	int total_marks;
	int marked_len;
	
	switch(d->kind) {//A_varDec, A_functionDec, A_typeDec
	case A_varDec: //struct {S_symbol var; S_symbol typ; A_exp init; bool escape;} var;
debug("A_varDec");

		var_sym = d->u.var.var;
		//1) Check if the variable has been used as loop variant
		if(VL_check(var_sym)){
			SEM_msg(FATAL, d->pos, "The name of loop variant '%s' cannot be reused", S_name(var_sym));
		}

		//2) Translate the initializer
		e = transExp(level, venv, tenv, d->u.var.init);
		ty = actual_ty(e.ty);

		//3) Determine the var's type
		if(d->u.var.typ != NULL){
			varDecTy = S_look(tenv, d->u.var.typ);
			if(varDecTy == NULL || !looseTyCompare(ty, varDecTy)){
				SEM_msg(ERROR, d->pos, "The initializer of variable '%s' is incompatible with the declared type", S_name(var_sym));
			}
		} else {
			if(ty->kind == Ty_nil){
				SEM_msg(FATAL, d->pos, "Cannot inference the type of variable '%s' as NIL is used as initializer", S_name(var_sym));
			}
			varDecTy = e.ty;
		}
		
		//4) Enter the var into env
		tr_acc = Tr_allocLocal(level, TRUE);//??? always escaping?
		S_enter(venv, var_sym, E_VarEntry(tr_acc, varDecTy));

		//5) Return the IR doing initialization and assignment 
		tr_exp = Tr_assignExp(transVar(level, venv, tenv, A_SimpleVar(d->pos, d->u.var.var)).exp, e.exp);

		break;
	case A_functionDec: 
debug("A_functionDec");
		//1) Put header (formals and result) of function declarations into the 
		//environment for recursive reference during next step.
		//NOTE: Also check if there are functions of same name. In a mutually 
		//recursive declaration block no two functions may have the same name.
		//(namely no re-declaration)
		SL_empty();
		for(functions = d->u.function; functions!=NULL; functions=functions->tail){
			head = functions->head;
			if(head != NULL){
debug("  A_funDec put header: %s", S_name(head->name));
				//1.1) Check if it's been used in this block
				if(SL_check(head->name)){
					SEM_msg(FATAL, d->pos, 
					"The function name '%s' has been used adjacently", 
					S_name(head->name));
				} else {
					SL_push(head->name);
				}

				//1.2) Determine the type of returned value
				if(head->result!=NULL){
					resultTy = S_look(tenv, head->result);
				} else {
					resultTy = Ty_Void();
				}

				//1.3) Solve the formal parameter list
				formalTys = makeFormalTyList(tenv, head->params);

				//1.4) Enter a new function into env
				//1.4.1) Create level
				createEscapeList(&formal_escs, head->params);
				funLevel = Tr_newLevel(level, Temp_namedlabel(S_name(head->name)), formal_escs);
				checked_free(formal_escs);
				//1.4.2) Update env
				S_enter(venv, head->name, E_FunEntry(funLevel, Temp_newlabel(), formalTys, resultTy));
			}
		}

		//2) Translate function declarations.
		for(functions = d->u.function; functions!=NULL; functions=functions->tail){
			head = functions->head;
			if(head != NULL){
debug("  A_funDec translate body: %s", S_name(head->name));
				//2.1) retrieve the formals of this E_FunEntry
				entry = S_look(venv, head->name);
				formalTys = entry->u.fun.formals;

				//2.2) start new scope
				S_beginScope(venv);

				//2.3) add all parameters to venv as declared variables
				//Use tail here since the first argument is the static link.
				tr_formals = Tr_formals(entry->u.fun.level)->tail;
				for(fList = head->params, tList = formalTys; fList!=NULL; 
					fList = fList->tail, tList = tList->tail, tr_formals = tr_formals->tail){
					S_enter(venv, fList->head->name, E_VarEntry(tr_formals->head, tList->head));				
				}
				//2.4) translate the body, which is an exp
				e = transExp(entry->u.fun.level, venv, tenv, head->body);

				//2.5) check the type compatibility between body and the declared returned value
				resultTy = entry->u.fun.result;
				if(head->result != NULL){
				//2.5.1) it's a function
					ty = actual_ty(e.ty);
					if(actual_ty(resultTy) != ty){
						if(ty->kind == Ty_void){
							SEM_msg(ERROR, head->pos, 
							"The function '%s' has no value returned", 
							S_name(head->name));
						} else {
							SEM_msg(ERROR, head->pos, 
							"The returned value of function '%s' is incompatible with the declared one", 
							S_name(head->name));
						}
					}
				} else {
				//2.5.2) it's a procedure
					if(actual_ty(e.ty)->kind != Ty_void){
						SEM_msg(WARNING, head->pos, 
						"The returned value of procedure '%s' will be ignored", 
						S_name(head->name));
					}
				}
				
				//2.6) Translate to fragment
				Tr_procEntryExit(entry->u.fun.level, e.exp, Tr_formals(entry->u.fun.level), entry->u.fun.label);
		
				//2.7) exit the scope
				S_endScope(venv);
			}
		}

		tr_exp = Tr_voidExp();//Return NO_OP
		break;
	case A_typeDec: 
debug("A_typeDec");
		//1) Put header of type declarations into the environment for 
		//recursive reference during next step.
		//1.1) Also check if there are types of same name. In a mutually 
		//recursive declaration block no two types may have the same name.
		//(namely no re-declaration)
		SL_empty();
		for(types = d->u.type; types!=NULL; types=types->tail){
			type = types->head;
			if(type != NULL){
				//1) Check if it's been used in this block
				if(SL_check(type->name)){
					SEM_msg(FATAL, d->pos, 
					"The type name '%s' has been used adjacently", 
					S_name(type->name));
				} else {
					SL_push(type->name);
				}

				S_enter(tenv, type->name, transTyHeader(type->ty));

				//Prepare for 3) - see below
				if(type->ty->kind == A_nameTy){
					names++;
				}
			}
		}

		//Prepare for 3) - see below
		nameTys = checked_malloc(sizeof(A_ty)*names);
		tySymbols = checked_malloc(sizeof(S_symbol)*names);
		marked = checked_malloc(sizeof(bool)*names);
		for(ind = 0; ind < names; ind++){
			marked[ind] = FALSE;
		}
		marked_len = names;
		names = 0;

		//2) Translate type declarations: instantiate the body of Ty_ty
		//created by transTyHeader().
		for(types = d->u.type; types!=NULL; types=types->tail){
			type = types->head;
			if(type != NULL){
				transTy(tenv, type->name, type->ty);

				//Prepare for 3) - see below
				if(type->ty->kind == A_nameTy){
					tySymbols[names] = type->name;
					nameTys[names++] = type->ty;
				}
			}
		}
		
		//3) Inspect cyclic references
		total_marks = names;
		while(total_marks>0){
			changed = FALSE;
			for(ind = 0; ind < names; ind++){
				if(marked[ind]==TRUE){
					continue;
				}
				aty = nameTys[ind];
				if(isReferToOutside(marked, marked_len, tySymbols, aty->u.name)){
					marked[ind] = TRUE;
					changed = TRUE;
					total_marks--;
				}
			}
			if(changed==FALSE && total_marks>0){
				for(ind = 0; ind < names; ind++){
					if(marked[ind]==FALSE){
						aty = nameTys[ind];
						SEM_msg(SEVERE, aty->pos, "Cyclic type definition detected: %s", S_name(tySymbols[ind]));
					}
				}
				SEM_check_exit(SEVERE);
				break;
			}
		}
		checked_free(nameTys);
		checked_free(tySymbols);
		checked_free(marked);

		//4) Print out the types [WARNING: Infinite loop]
		/*
		for(types = d->u.type; types!=NULL; types=types->tail){
			if(types->head != NULL){
				z_Ty_print(S_look(tenv, types->head->name), 0);
			}
		}
		*/
		tr_exp = Tr_voidExp();//Return NO_OP
		break;
	}

	return tr_exp;
}

/*
static void _createEscapeList(U_boolList *formal_escs, A_fieldList fList){
	A_fieldList fl;
	int i = 0;
	for(fl=fList;fl!=NULL;fl=fl->tail){
		i++;
	}
int j=0;
	int node_size = (sizeof(struct U_boolList_))/8;
	U_boolList escs = (U_boolList)checked_malloc((i+1)*sizeof(struct U_boolList_));
	U_boolList escs_head = escs;
	for(;fList!=NULL;fList=fList->tail){
		escs->head = TRUE;//FIXME (should be: = fList->head->escape;)
		if(fList->head!=NULL){
			escs->tail = escs + node_size;
			escs = escs->tail; 
		} else {
			escs->tail = NULL;
		}		
j++;	
	}

	*formal_escs = escs_head;
debug("escapes: %d in total", j);

j=0;
U_boolList l = *formal_escs;
for(;l;l=l->tail){
j++;
}
debug("escapes recheck: %d in total", j);
}
*/

static void createEscapeList(U_boolList *formal_escs, A_fieldList fList){
	if(fList==NULL){
		*formal_escs = NULL;
		return;
	}

	U_boolList escs = NULL;
	U_boolList escs_head = NULL;
	for(;fList!=NULL;fList=fList->tail){
		if(escs_head==NULL){
			escs = U_BoolList(TRUE, NULL);
			escs_head = escs;
		} else {
			escs->tail = U_BoolList(TRUE, NULL);
			escs = escs->tail;	
		}
	}

	if(escs!=NULL){
		escs->tail = NULL;
	}

	*formal_escs = escs_head;
}

static Ty_ty actual_ty(Ty_ty ty){
	if(ty->kind == Ty_name){//Ty_record, Ty_nil, Ty_int, Ty_string, Ty_array, Ty_name, Ty_void
		return actual_ty(ty->u.name.ty);
	}
	return ty;
}

static bool looseTyCompare(Ty_ty a, Ty_ty b){
	Ty_ty _a = actual_ty(a);
	Ty_ty _b = actual_ty(b);
	if(_a == _b){
		return TRUE;
	}
	if(_a->kind == Ty_nil || _b->kind == Ty_nil){
		return TRUE;
	}
	return FALSE;
}

static bool isReferToOutside(bool* marked, int marked_len, S_symbol* syms, S_symbol refered){
	int i;	
	for(i=0;i<marked_len;i++){
		if(syms[i]==refered){
			//Found the referred type in the dec sequence
			return marked[i];
		}
	}

	//Didn't find the referred type in the dec sequence. Must be referring to outside.
	return TRUE;
}

static Ty_tyList makeFormalTyList(S_table tenv, A_fieldList params){
	Ty_tyList tyList = Ty_TyList(NULL, NULL);
	Ty_tyList tyList_head = tyList;
	bool legal = TRUE;

	S_symbol _sym;
	Ty_ty ty;

	//Always complete the checking for the formals regardless of errors
	SL_empty();
	for(; params!=NULL; params=params->tail){
		A_field head = params->head;
		if(head==NULL){
			continue;
		}
		//1) Check if the variable has been used as loop variant
		if(VL_check(head->name)){
			SEM_msg(SEVERE, head->pos, "The name of loop variant '%s' cannot be reused by function's argument", S_name(head->name));
		}
		//2) Check if the type for this argument has been declared
		_sym = head->typ;
		ty = S_look(tenv, _sym);
		if(ty==NULL){
			SEM_msg(SEVERE, head->pos, "Undefined type: %s", S_name(_sym));
			legal = legal && FALSE;
		}
		//3) Check if the formal's name has been used in the list
		if(!SL_check(head->name)){
			SL_push(head->name);
		} else {
			SEM_msg(SEVERE, head->pos, "The name of formal argument '%s' has been used for this function", S_name(head->name));
		}
		if(legal){
			tyList->tail = Ty_TyList(NULL, NULL);
			tyList->head = ty;
			tyList = tyList->tail;
		}
	}

	tyList->tail = NULL;

	//Terminate if any error occured before in the loop
	SEM_check_exit(SEVERE);

	return tyList_head->head!=NULL?tyList_head:NULL;
}

/* 
 * Translate A_ty to Ty_ty. Only generates an empty object of the corresponding
 * type for now. The contents will be populated later.
 * 
 * This function doesn't update the environment.
 */
static Ty_ty transTyHeader(A_ty a){
	S_symbol sym;
	Ty_ty ty;
	switch(a->kind){//A_nameTy, A_recordTy, A_arrayTy
	case A_nameTy:
		return Ty_Name(a->u.name, NULL);		
	case A_recordTy:
		return Ty_Record(NULL);
	case A_arrayTy:
		return Ty_Array(NULL);	
	}
}

/* 
 * Populate Ty_ty. Basically, search in the given environment: if the referred 
 * type is found, create a new type based on the found one; if not, report 
 * error.
 *
 * This function doesn't update the environment per se. It only changes what is
 * in Ty_ty, but will not create new. 
 */
void transTy(S_table tenv, S_symbol sym, A_ty a){
	A_fieldList flist;
	Ty_fieldList tyList = Ty_FieldList(NULL, NULL);
	Ty_fieldList tyList_head = tyList;

	//Get entry of this type from env. Should have been entered by transTyHeader() 
	Ty_ty this_ty = S_look(tenv, sym);
	Ty_ty deref_ty;

	S_symbol _sym;
	Ty_ty ty;

//Ty_fieldList tyfList;
//int i;

	switch(a->kind){//A_nameTy, A_recordTy, A_arrayTy
	case A_nameTy:
		_sym = a->u.name;
		ty = S_look(tenv, _sym);
		if(ty==NULL){
			SEM_msg(FATAL, a->pos, "Undefined type: %s", S_name(_sym));
		} else {
			this_ty->u.name.ty = ty;
		}
		break;	
	case A_recordTy:
		for(flist = a->u.record; flist!=NULL; flist=flist->tail){
			A_field head=flist->head;
			if(head==NULL){
				continue;
			}
			_sym = head->typ;
			ty = S_look(tenv, _sym);
			if(ty==NULL){
				SEM_msg(SEVERE, a->pos, "Undefined type: %s", S_name(_sym));
			} else {
				tyList->tail = Ty_FieldList(NULL, NULL);
				tyList->head = Ty_Field(head->name, ty);
				tyList = tyList->tail;
			}
		}
		//Terminate in case of error
		SEM_check_exit(SEVERE);

		tyList->tail = NULL;
		this_ty->u.record = tyList_head;
		break;
	case A_arrayTy:
		_sym = a->u.array;
		ty = S_look(tenv, _sym);
		if(ty==NULL){
			SEM_msg(FATAL, a->pos, "Undefined type: %s", S_name(_sym));
		} else {
			this_ty->u.array = ty;
		}
		break;
	}
}

struct expty expTy(Tr_exp exp, Ty_ty ty){
	struct expty e;
	e.exp = exp;
	e.ty = ty;
	return e;
}

/* ENV Module */
E_enventry E_VarEntry(Tr_access access, Ty_ty ty){
	E_enventry entry = checked_malloc(sizeof(*entry));
	entry->kind = E_varEntry;
	entry->u.var.ty = ty;
	entry->u.var.access = access;
	return entry;
}

E_enventry E_FunEntry(Tr_level level, Temp_label label, Ty_tyList formals, Ty_ty result){
	E_enventry entry = checked_malloc(sizeof(*entry));
	entry->kind = E_funEntry;
	entry->u.fun.formals = formals;
	entry->u.fun.result = result;
	entry->u.fun.level = level;
	entry->u.fun.label = label;
	return entry;
}

/* Ty_ty environment */
S_table E_base_tenv(void){
	S_table table = S_empty();
	S_enter(table, S_Symbol("int"), Ty_Int());
	S_enter(table, S_Symbol("string"), Ty_String());
	return table;
}

/* E_enventry environment */
S_table E_base_venv(void){
	S_table table = S_empty();
	Tr_level level;

	//Built-in functions are put in the same level where the main sits
	level = Tr_newLevel(Tr_outermost(), Temp_namedlabel("print"), U_BoolList(TRUE, NULL));
	S_enter(table, S_Symbol("print"), E_FunEntry(level, Temp_newlabel(), Ty_TyList(Ty_String(), NULL), Ty_Void()));

	level = Tr_newLevel(Tr_outermost(), Temp_namedlabel("flush"), NULL);
	S_enter(table, S_Symbol("flush"), E_FunEntry(level, Temp_newlabel(), NULL, Ty_Void()));

	level = Tr_newLevel(Tr_outermost(), Temp_namedlabel("getchar"), NULL);
	S_enter(table, S_Symbol("getchar"), E_FunEntry(level, Temp_newlabel(), NULL, Ty_String()));

	level = Tr_newLevel(Tr_outermost(), Temp_namedlabel("ord"), U_BoolList(TRUE, NULL));
	S_enter(table, S_Symbol("ord"), E_FunEntry(level, Temp_newlabel(), Ty_TyList(Ty_String(), NULL), Ty_Int()));

	level = Tr_newLevel(Tr_outermost(), Temp_namedlabel("chr"), U_BoolList(TRUE, NULL));
	S_enter(table, S_Symbol("chr"), E_FunEntry(level, Temp_newlabel(), Ty_TyList(Ty_Int(), NULL), Ty_String()));

	level = Tr_newLevel(Tr_outermost(), Temp_namedlabel("size"), U_BoolList(TRUE, NULL));
	S_enter(table, S_Symbol("size"), E_FunEntry(level, Temp_newlabel(), Ty_TyList(Ty_String(), NULL), Ty_Int()));

	level = Tr_newLevel(Tr_outermost(), Temp_namedlabel("substring"), U_BoolList(TRUE, U_BoolList(TRUE, U_BoolList(TRUE, NULL))));
	S_enter(table, S_Symbol("substring"), E_FunEntry(level, Temp_newlabel(), Ty_TyList(Ty_String(), Ty_TyList(Ty_Int(), Ty_TyList(Ty_Int(), NULL))), Ty_String()));

	level = Tr_newLevel(Tr_outermost(), Temp_namedlabel("concat"), U_BoolList(TRUE, U_BoolList(TRUE, NULL)));
	S_enter(table, S_Symbol("concat"), E_FunEntry(level, Temp_newlabel(), Ty_TyList(Ty_String(), Ty_TyList(Ty_String(), NULL)), Ty_String()));

	level = Tr_newLevel(Tr_outermost(), Temp_namedlabel("not"), U_BoolList(TRUE, NULL));
	S_enter(table, S_Symbol("not"), E_FunEntry(level, Temp_newlabel(), Ty_TyList(Ty_Int(), NULL), Ty_Int()));

	level = Tr_newLevel(Tr_outermost(), Temp_namedlabel("exit"), U_BoolList(TRUE, NULL));
	S_enter(table, S_Symbol("exit"), E_FunEntry(level, Temp_newlabel(), Ty_TyList(Ty_Int(), NULL), Ty_Void()));

	return table;
}

/* TEST ONLY Start */
void z_Ty_print(Ty_ty t, int ind) {
	Ty_fieldList rlist;
	Ty_ty ty;
	int i = ind;
	while(i>0){
		i--;
		printf(" ");
	}
	if (t == NULL) {
		printf("null\n");
	} else { 
		printf("%s", z_str_ty[t->kind]);
        switch(t->kind){
		case Ty_name:
			printf(", %s\n", S_name(t->u.name.sym));
			break;
		case Ty_record:
			printf(",\n");
			for(rlist = t->u.record; rlist!=NULL; rlist=rlist->tail){
				if(rlist->head != NULL){
					z_Ty_print(rlist->head->ty, ind+2);		
				}
			}
			break;
		default:
			printf("\n");
			//printf("\nOTHER TYPES\n");
			//z_Ty_print(t->u.array, ind+2);
		}
	}
}
/* TEST ONLY End */
