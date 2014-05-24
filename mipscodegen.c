/* 
 * CS5161 Assignment 6
 * 
 * TIGER's Generation of Assembly Code
 *
 * Ming Zhou (4465225)
 * Kuo-shih Tseng (4436736)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "util.h"
#include "symbol.h"
#include "absyn.h"
#include "temp.h"
#include "tree.h"
#include "assem.h"
#include "frame.h"
#include "mipsframe.h"
#include "errormsg.h"
#include "codegen.h"

//Interface
static AS_instrList F_procEntryExit2(AS_instrList body);
static Temp_temp munchExp(T_exp e);
static Temp_temp munchStm(T_stm s);
static void emit(AS_instr inst);
static Temp_tempList L(Temp_temp h, Temp_tempList t);
static AS_instrList iList = NULL, last = NULL;

static Temp_tempList munchArgs(T_expList args, int *num);
static Temp_tempList getCalldefs();
static Temp_temp emitBinop(char *op, T_exp exp0, T_exp exp1);
static Temp_temp emitShiftop(char *op, T_exp exp0, T_exp exp1);
static void emitCondJump(char *op, T_exp left, T_exp right, Temp_label tLabel, Temp_label fLabel);
static void emitComment(char *message);

static char* build(char *message, ...);
static void error(char *message);

extern const int F_wordSize;
static F_frame thisFrame;
static Temp_temp *callerSaveRegArray = NULL;
static int callerSaveRegsNum = -1;
static void callerRestoreRegs();
static void callerSaveRegs();
static inline void initCallerSaveRegs();

//Debug & Test
#define debug(...) U_debug ( "[codegen] ", __VA_ARGS__ )
static char z_str_exp_kind[][8] = {
   "BINOP", "MEM", "TEMP", "ESEQ", "NAME", "CONST", "CALL"};

//Implementation
/*
 * ATTENTION!
 * This function has no size check for the message passed in, 
 * It CANNOT output a message longer than 78 bytes, formatted.
 */
static char* build(char *message, ...) {
    va_list ap;
    va_start(ap, message);
    
	char* head = checked_malloc(80 * sizeof(char));
    char* s = head;
    int n = vsprintf(s, message, ap);
	s += n;
	sprintf(s, "\n\0");
	va_end(ap);

	return head;
}

static void error(char *message) {
	printf("[Assemble Codegen Error] %s\n", message);
	exit(1);
}

static Temp_tempList munchArgs(T_expList args, int *num){
	int n = 0;
	if(args==NULL){
		return NULL;
	}

	T_exp arg = NULL;
	Temp_tempList list = NULL;
	Temp_tempList listHead = NULL;
	for(; args!=NULL; args=args->tail){
		arg = args->head;
		if(arg != NULL){
			if(list == NULL){
				//First time
				list = Temp_TempList(munchExp(arg), NULL);
				listHead = list;
			} else {
				//Second and more
				list->tail = Temp_TempList(munchExp(arg), NULL);
				list = list->tail;
			}
			n++;
		}
	}
//printf("n=%d\n", n);
	*num = n;
	return listHead;
}

static Temp_tempList calldefs = NULL;
//The list of regs that callee may trash. The caller must preserve them in stack.
static Temp_tempList getCalldefs(){
	if(calldefs==NULL){
		Temp_tempList list = F_getRegList(specialregs);
		calldefs = list;
		while(list->tail != NULL){
			list = list->tail;
		}
		list->tail = F_getRegList(callersaves);
	}
	return calldefs;
}

static Temp_temp emitBinop(char *op, T_exp exp0, T_exp exp1){
	//op $d,$s,$t
	Temp_temp r = Temp_newtemp();
	emit(AS_Oper(
		build("%s `d0, `s0, `s1", op), 
		L(r, NULL),					//d0
		L(munchExp(exp0), 			//s0, s1
			L(munchExp(exp1), NULL)),
		NULL
	));
	return r;
}

static Temp_temp emitShiftop(char *op, T_exp exp0, T_exp exp1){
	//op $d,$s,$t
	if(exp0->kind == T_CONST){
		//op $t,$s,shamt
		Temp_temp r = Temp_newtemp();
		emit(AS_Oper(
			build("%s `d0, `s0, %d", op, exp0->u.CONST), 
			L(r, NULL),					//d0
			L(munchExp(exp1), NULL),	//s0
			NULL
		));
		return r;				
	} else if (exp1->kind == T_CONST){
		//op $t,$s,shamt
		Temp_temp r = Temp_newtemp();
		emit(AS_Oper(
			build("%s `d0, `s0, %d", op, exp1->u.CONST), 
			L(r, NULL),					//d0
			L(munchExp(exp0), NULL),	//s0
			NULL
		));
		return r;				
	}

	error("One of the operands for logical shift must be constant");	

	return NULL;
}

static void emitCondJump(char* op, T_exp left, T_exp right, Temp_label tLabel, Temp_label fLabel) {
	emit(AS_Oper(
		build("%s `s0, `s1, `j0", op), 
		NULL,
		L(munchExp(left), 							//s0
			L(munchExp(right), NULL)),				//s1
		AS_Targets(Temp_LabelList(					//j0, 
			tLabel, Temp_LabelList(fLabel, NULL)))	//j1
	));
}

static inline void initCallerSaveRegs(){
	{
		if(callerSaveRegsNum == -1){//not yet initialized
			callerSaveRegsNum = 0;//make sure of the initialization
			Temp_tempList list;
			list = F_getRegList(callersaves);//$t
			while(list!=NULL && list->head!=NULL){
				callerSaveRegsNum++;
				list = list->tail;
			}
/*
			list = F_getRegList(argregs);//$a
			while(list!=NULL && list->head!=NULL){
				callerSaveRegsNum++;
				list = list->tail;
			}
			callerSaveRegsNum++;//$v0
*/
			//Put these regs into an array for reuse in future
			callerSaveRegArray = checked_malloc(callerSaveRegsNum * sizeof(Temp_temp));
			int cnt = 0;
			list = F_getRegList(callersaves);//$t
			while(list!=NULL && list->head!=NULL){
				callerSaveRegArray[cnt] = list->head;
				cnt++;
				list = list->tail;
			}
/*
			list = F_getRegList(argregs);//$a
			while(list!=NULL && list->head!=NULL){
				callerSaveRegArray[cnt] = list->head;
				cnt++;
				list = list->tail;
			}
			callerSaveRegArray[cnt] = F_RV();//$v0
*/
		}
	}
}

static void emitComment(char *message){
	emit(AS_Oper(
		String(message), 
		NULL,NULL,NULL
	));
}

static void callerSaveRegs(){
	initCallerSaveRegs();
	int cnt = 0;
	if(callerSaveRegsNum>0){
		emitComment("#save registers that callee may use\n");
	}
	for(;cnt<callerSaveRegsNum;cnt++){
		emit(AS_Oper(
			build("sw `s0, %d(`s1)", thisFrame->offset + (-1*cnt*F_wordSize)), 
			NULL,										
			L(callerSaveRegArray[cnt], L(F_FP(), NULL)),	//s0, s1
			NULL
		));
	}
}

static void callerRestoreRegs(){
	initCallerSaveRegs();
	int cnt = callerSaveRegsNum-1;
	if(callerSaveRegsNum>0){
		emitComment("#restore registers saved before\n");
	}
	for(;cnt>=0;cnt--){
		emit(AS_Oper(
			build("ld `d0, %d(`s0)", thisFrame->offset + (-1*cnt*F_wordSize)), 
			L(callerSaveRegArray[cnt], NULL),	//d0										
			L(F_FP(), NULL),				//s0
			NULL
		));
	}	
}

static Temp_temp munchExp(T_exp e) {
	T_exp exp0 = NULL;
	T_exp exp1 = NULL;
	T_exp exp2 = NULL;

	//1) load from memory
	if(e->kind == T_MEM){		
		exp0 = e->u.MEM;

		//1.1) relative access 
		//MEM(BINOP(PLUS, e1, CONST(i)))
		if(exp0->kind == T_BINOP) {
			exp1 = exp0->u.BINOP.left;
			exp2 = exp0->u.BINOP.right;
			if(exp0->u.BINOP.op == T_plus){
				// special case: if the operator is '+' and either operand is a const
				if(exp1->kind == T_CONST){
					//ld $t,C($s)
					Temp_temp r = Temp_newtemp();
					emit(AS_Oper(
						build("ld `d0, %d(`s0)", exp1->u.CONST), 
						L(r, NULL),	//d0
						L(munchExp(exp2), NULL),	//s0
						NULL
					));
					return r;
				} else if (exp2->kind == T_CONST){
					//ld $t,C($s)
					Temp_temp r = Temp_newtemp();
					emit(AS_Oper(
						build("ld `d0, %d(`s0)", exp2->u.CONST), 
						L(r, NULL),	//d0
						L(munchExp(exp1), NULL),	//s0
						NULL
					));
					return r;
				}
			}
		} 
		//1.2) absolute access 
		//MEM(CONST(i))		
		else if (exp0->kind == T_CONST) {
			//ld $t,C($zero)
			Temp_temp r = Temp_newtemp();
			emit(AS_Oper(
				build("ld `d0, %d(`s0)", exp0->u.CONST), 
				L(r, NULL),	//d0
				L(F_ZERO(), NULL),			//s0
				NULL
			));
			return r;
		}

		//1.3) a more general case for relative access: calculate the absolute address and put into a reg
		//MEM(T_Exp)		
		//ld $t,0($s)
		Temp_temp r = Temp_newtemp();
		emit(AS_Oper(
			build("ld `d0, 0(`s0)"), 
			L(r, NULL),	//d0
			L(munchExp(exp0), NULL),	//s0
			NULL
		));
		return r;
	}

	//2) BINOP
	else if(e->kind == T_BINOP){		
		exp0 = e->u.BINOP.left;
		exp1 = e->u.BINOP.right;

		switch(e->u.BINOP.op){
		case T_plus: 
			// special cases: immediate value present
			if(exp0->kind == T_CONST){
				//2.1.1) addi $t,$s,C
				Temp_temp r = Temp_newtemp();
				emit(AS_Oper(
					build("addi `d0, `s0, %d", exp0->u.CONST), 
					L(r, NULL),					//d0
					L(munchExp(exp1), NULL),	//s0
					NULL
				));
				return r;				
			} else if (exp1->kind == T_CONST){
				//2.1.2) addi $t,$s,C
				Temp_temp r = Temp_newtemp();
				emit(AS_Oper(
					build("addi `d0, `s0, %d", exp1->u.CONST), 
					L(r, NULL),					//d0
					L(munchExp(exp0), NULL),	//s0
					NULL
				));
				return r;				
			} 
			return emitBinop("add", exp0, exp1);
		case T_minus: 
			return emitBinop("sub", exp0, exp1);
		case T_mul: 
			//NOTE: pseudo op "mul" will only return the lower 32 bits to dest
			return emitBinop("mul", exp0, exp1);
		case T_div:
			return emitBinop("div", exp0, exp1);
		case T_and: 
			// special cases: immediate value present
			if(exp0->kind == T_CONST){
				//2.5.1) andi $t,$s,C
				Temp_temp r = Temp_newtemp();
				emit(AS_Oper(
					build("andi `d0, `s0, %d", exp0->u.CONST), 
					L(r, NULL),					//d0
					L(munchExp(exp1), NULL),	//s0
					NULL
				));
				return r;				
			} else if (exp1->kind == T_CONST){
				//2.5.2) andi $t,$s,C
				Temp_temp r = Temp_newtemp();
				emit(AS_Oper(
					build("andi `d0, `s0, %d", exp1->u.CONST), 
					L(r, NULL),					//d0
					L(munchExp(exp0), NULL),	//s0
					NULL
				));
				return r;				
			} 
			return emitBinop("and", exp0, exp1);
		case T_or: 
			// special cases: immediate value present
			if(exp0->kind == T_CONST){
				//2.6.1) ori $t,$s,C
				Temp_temp r = Temp_newtemp();
				emit(AS_Oper(
					build("ori `d0, `s0, %d", exp0->u.CONST), 
					L(r, NULL),					//d0
					L(munchExp(exp1), NULL),	//s0
					NULL
				));
				return r;				
			} else if (exp1->kind == T_CONST){
				//2.6.2) ori $t,$s,C
				Temp_temp r = Temp_newtemp();
				emit(AS_Oper(
					build("ori `d0, `s0, %d", exp1->u.CONST), 
					L(r, NULL),					//d0
					L(munchExp(exp0), NULL),	//s0
					NULL
				));
				return r;				
			} 
			return emitBinop("or", exp0, exp1);
		case T_lshift: 
			return emitShiftop("sll", exp0, exp1);
		case T_rshift: 
			return emitShiftop("srl", exp0, exp1);
		case T_arshift: 
			return emitShiftop("sra", exp0, exp1);
		case T_xor:
			return emitBinop("xor", exp0, exp1);
		default:		
			error("unexpected (illegal) binary operator");
		}
	}

	//3) TEMP
	else if(e->kind == T_TEMP){	
		return e->u.TEMP;
	}

	//4) NAME
	else if(e->kind == T_NAME){	
		Temp_temp r = Temp_newtemp();
		emit(AS_Oper(
			build("la `d0, %s", Temp_labelstring(e->u.NAME)), 
			L(r, NULL),		//d0
			NULL,			//s0
			NULL
		));
		return r;	
	}

	//5) CONST
	else if(e->kind == T_CONST){	
		Temp_temp r = Temp_newtemp();
		emit(AS_Oper(//"addi `d0, `s0, %d"
			build("li `d0, %d", e->u.CONST), 
			L(r, NULL),					//d0
			NULL,
			NULL
		));
		return r;	
	}

	//6) CALL
	else if(e->kind == T_CALL){
		int total = 0;
		Temp_tempList list = munchArgs(e->u.CALL.args, &total);
		exp0 = e->u.CALL.fun;
		//The jump target must be a label
		if(exp0->kind == T_NAME){

			//1) save regs
			callerSaveRegs();

			//2) pass arguments
			if(total>0){
				emitComment("#place arguments into registers and stack (from the 5th one, if any)\n");	
			}
			//2.1) the first four go to arg regs
			int cnt;	
			Temp_tempList argList;
			for(cnt=0, argList = list; cnt<4 && cnt<total; cnt++, argList=argList->tail){
				emit(AS_Move(
					build("move `d0, `s0"), 
					L(F_AN(cnt), NULL),		//d0							
					L(argList->head, NULL)		//s0
				));		
			}
			//2.2) the rest go to stack, in reversed order (the first arg goes to the bottom of frame)
			cnt = 0;
			total -= 4;
			for(; argList!=NULL; argList=argList->tail, cnt++){
				emit(AS_Oper(
					build("sw `s0, %d(`s1)", 
						thisFrame->offset + (-1*(total-cnt+callerSaveRegsNum-1)*F_wordSize)), 
					NULL,										
					L(argList->head, L(F_FP(), NULL)),	//s0, s1
					NULL
				));	
			}

			//3) go
			emitComment("#registers saved; call function now\n");
			emit(AS_Oper(
				build("jal %s", Temp_labelstring(exp0->u.NAME)),
				getCalldefs(),	 //destination: all the regs that the caller may trash, and are saved by caller
				list,			 //source: the arguments
				NULL
			));
		
			//4) restore regs
			callerRestoreRegs();

			return F_RV();	
		} 
		error("unexpected or illegal target for procedure call");
	}
	
	//Errors
	else {
		error("unexpected or illegal expression type");
	}
	
	//Shouldn't reach here
	return NULL;
}

static Temp_temp munchStm(T_stm s) {
	T_exp exp0 = NULL;
	T_exp exp1 = NULL;
	T_exp exp2 = NULL;
	T_exp exp3 = NULL;

	//1) store
	if(s->kind == T_MOVE){		
		exp0 = s->u.MOVE.dst;
		exp3 = s->u.MOVE.src;

		//1.1) to memory
		//MOVE(MEM(... ...), e3)
		//     |--- e0 ---|
		if(exp0->kind == T_MEM) {
			exp0 = exp0->u.MEM;

			//1.1.1) relative access
			if(exp0->kind == T_BINOP) {
				//MOVE(MEM(BINOP(... ...)), e3)
				//         |---- e0 ----|
				exp1 = exp0->u.BINOP.left;
				exp2 = exp0->u.BINOP.right;
				if(exp0->u.BINOP.op == T_plus){
					if(exp1->kind == T_CONST){
						//MOVE(MEM(BINOP(PLUS, e1, CONST(i))), e3)
						//        			  |e1| |- e2 -|
						//sw $t,C($s)	Memory[$s + C] = $t
						emit(AS_Oper(
							build("sw `s0, %d(`s1)", exp1->u.CONST), 
							NULL,										
							L(munchExp(exp3), L(munchExp(exp2), NULL)),	//s0, s1
							NULL
						));
						return;
					} else if (exp2->kind == T_CONST){
						//MOVE(MEM(BINOP(PLUS, CONST(i), e1)), e3)
						//        			   |- e1 -| |e2| 
						//sw $t,C($s)	Memory[$s + C] = $t
						emit(AS_Oper(
							build("sw `s0, %d(`s1)", exp2->u.CONST), 
							NULL,										
							L(munchExp(exp3), L(munchExp(exp1), NULL)),	//s0, s1
							NULL
						));
						return;
					}
				}
			} 
			//1.1.2) absolute access
			else if (exp0->kind == T_CONST) {
				//MOVE(MEM(CONST(i)), e3)
				//     |--- e0 ---|
				//sw $t,C($s)	Memory[$s + C] = $t
				emit(AS_Oper(
					build("sw `s0, %d(`s1)", exp0->u.CONST), 
					NULL,									
					L(munchExp(exp3), L(F_ZERO(), NULL)),	//s0, s1($zero)
					NULL
				));
				return;
			}

			//1.1.3) a more generalized case for absolute access
			//MOVE(MEM(... ...), e3)
			//        |-- e0 -|
			//sw $t,0($s)	Memory[$s + 0] = $t
			emit(AS_Oper(
				build("sw `s0, 0(`s1)"), 
				NULL,							
				L(munchExp(exp3), 				//s0
					L(munchExp(exp0), NULL)),	//s1
				NULL
			));
			return;
		} 
		//1.2) to register
		//MOVE(TEMP(... ...), e3)
		//     |--- e0  ---|
		else if(exp0->kind == T_TEMP) {

			//1.2.1) special case: move between temps; we use psuedo instruction "move" for this scenario			
			if(exp3->kind == T_TEMP){
				//move $rt,$rs	
				emit(AS_Move(
					build("move `d0, `s0"), 
					L(exp0->u.TEMP, NULL),	//d0							
					L(exp3->u.TEMP, NULL)	//s0
				));
				return;
			}

			//1.2.2) general case
			//addi $rt,$rs,0
			emit(AS_Move(
				build("move `d0, `s0"), 
				L(exp0->u.TEMP, NULL),		//d0							
				L(munchExp(exp3), NULL)		//s0
			));
			return;
		}

		error("illegal destination for MOVE");
	}

	//2) unconditional jump
	else if(s->kind == T_JUMP){		
		exp0 = s->u.JUMP.exp;
		Temp_labelList jumps = s->u.JUMP.jumps;

		//2.1) to a label
		if(exp0->kind == T_NAME){
			emit(AS_Oper(
				build("j `j0"), 
				NULL,						
				NULL,
				AS_Targets(jumps)	//j0	Temp_LabelList(exp0->u.NAME, NULL)
			));
			return;
		}

		error("illegal target for JUMP: can only be a LABEL");
		return;
	}

	//3) conditional jump
	else if(s->kind == T_CJUMP){		
		exp0 = s->u.CJUMP.left;
		exp1 = s->u.CJUMP.right;
		Temp_label tLabel = s->u.CJUMP.true;
		Temp_label fLabel = s->u.CJUMP.false;
		switch(s->u.CJUMP.op){
		case T_eq:
			emitCondJump("beq", exp0, exp1, tLabel, fLabel);
			return;
		case T_ne:
			emitCondJump("bne", exp0, exp1, tLabel, fLabel);
			return;
		case T_lt:
			emitCondJump("blt", exp0, exp1, tLabel, fLabel);
			return;
		case T_gt:
			emitCondJump("bgt", exp0, exp1, tLabel, fLabel);
			return;
		case T_le:
			emitCondJump("ble", exp0, exp1, tLabel, fLabel);
			return;
		case T_ge:
			emitCondJump("bge", exp0, exp1, tLabel, fLabel);
			return;
		default:
			error("unexpected relational operator; only eq, ne, lt, gt, le, ge are supported so far");
		}
		return;
	}

	//4) label
	else if(s->kind == T_LABEL){
		emit(AS_Label(build("%s: ", Temp_labelstring(s->u.LABEL)), s->u.LABEL));
		return;
	}

	//5) expression
	else if(s->kind == T_EXP){
		munchExp(s->u.EXP);//The result is discarded
		return;
	}

	//Errors
	error("unexpected or illegal statement type");	
}

static void emit(AS_instr inst) {
//static int num = 1;
//debug("append to instr list: %d", num++);
	if(last==NULL){
		//First time. Remember the head of list (iList)
		iList = AS_InstrList(inst, NULL);
		last = iList;
	} else {
		//Append instr to the list
		last->tail = AS_InstrList(inst, NULL);
		last = last->tail;
	}
}

static Temp_tempList L(Temp_temp h, Temp_tempList t) {
	return Temp_TempList(h, t);
}

AS_instrList F_codegen(F_frame f, T_stmList stmList) {
	AS_instrList list;
	T_stmList sl;
	thisFrame = f;

	for (sl=stmList; sl!=NULL; sl=sl->tail) {
		munchStm(sl->head);
	}

	list = iList;

	//Clean up
	iList = NULL;
	last = NULL;
	
	return F_procEntryExit2(list);
}

static Temp_tempList returnSink = NULL;
static AS_instrList F_procEntryExit2(AS_instrList body) {
	if(returnSink==NULL){
		returnSink = Temp_TempList(F_ZERO(), 
						Temp_TempList(F_RA(), 
						Temp_TempList(F_RV(), 
						Temp_TempList(F_SP(), 
					NULL))));
	}

	return AS_splice(body, AS_InstrList(AS_Oper("", NULL, returnSink, NULL), NULL)); 
}

AS_proc F_procEntryExit3(F_frame frame, AS_instrList body){
	char buf[80];
	sprintf(buf, "PROCEDURE %s\n", S_name(F_name(frame)));
	return AS_Proc(String(buf), body, "END\n");
}
