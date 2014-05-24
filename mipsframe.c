/*
 * CS5161 Assignment 5
 * 
 * Ming Zhou 4465225
 * 
 * Activation Record Module - MIPS architecture
 */

#include <stdio.h>
#include <stdlib.h>
#include "table.h"
#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"
#include "frame.h"
#include "mipsframe.h"

const int F_wordSize = 4;

static Temp_temp fp = NULL;
//Get the frame pointer
Temp_temp F_FP(){
	if(fp==NULL){
		fp = Temp_newtemp();
		//printf("FP pointer initialized: %s\n", Temp_look(Temp_name(), fp));
	}
	return fp;
}

static Temp_temp ra = NULL;
//Get the return value register
Temp_temp F_RA(){
	if(ra==NULL){
		ra = Temp_newtemp();
		//printf("RA pointer initialized: %s\n", Temp_look(Temp_name(), ra));
	}
	return ra;
}


static Temp_temp rv = NULL;
//Get the return value register
Temp_temp F_RV(){
	if(rv==NULL){
		rv = F_VN(0);
		//printf("RV pointer initialized: %s\n", Temp_look(Temp_name(), rv));
	}
	return rv;
}

static Temp_temp sp = NULL;
//Get the stack register
Temp_temp F_SP(){
	if(sp==NULL){
		sp = Temp_newtemp();
		//printf("SP pointer initialized: %s\n", Temp_look(Temp_name(), sp));
	}
	return sp;
}

static Temp_temp zero = NULL;
//Get the zero constant register
Temp_temp F_ZERO(){
	if(zero==NULL){
		zero = Temp_newtemp();
		//printf("ZERO pointer initialized: %s\n", Temp_look(Temp_name(), zero));
	}
	return zero;
}

static Temp_temp sn[8] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
static Temp_temp tn[10] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
static Temp_temp an[4] = {NULL, NULL, NULL, NULL};
static Temp_temp vn[2] = {NULL, NULL};

Temp_temp F_SN(int i){
	if(0<=i && i<=7) {
		if(sn[i]==NULL){
			sn[i] = Temp_newtemp();
		}
		return sn[i];
	} else {
		printf("Internal error: using a pointer that is not available for MIPS.");
		exit(2);
	}
}

Temp_temp F_TN(int i){
	if(0<=i && i<=9) {
		if(tn[i]==NULL){
			tn[i] = Temp_newtemp();
		}
		return tn[i];
	} else {
		printf("Internal error: using a pointer that is not available for MIPS.");
		exit(2);
	}
}

Temp_temp F_AN(int i){
	if(0<=i && i<=3) {
		if(an[i]==NULL){
			an[i] = Temp_newtemp();
		}
		return an[i];
	} else {
		printf("Internal error: using a pointer that is not available for MIPS.");
		exit(2);
	}
}

Temp_temp F_VN(int i){
	if(0<=i && i<=1) {
		if(vn[i]==NULL){
			vn[i] = Temp_newtemp();
		}
		return vn[i];
	} else {
		printf("Internal error: using a pointer that is not available for MIPS.");
		exit(2);
	}
}

static Temp_map _F_tempMap = NULL;

Temp_map F_tempMap(){
	if(_F_tempMap==NULL){
		_F_tempMap = Temp_empty();
		Temp_enter(_F_tempMap, F_RV(), String("$v0"));	//$v0 and $v1 ($2 and $3) are for returned values
		Temp_enter(_F_tempMap, F_RA(), String("$ra"));	//$31
		Temp_enter(_F_tempMap, F_FP(), String("$fp"));	//$30
		Temp_enter(_F_tempMap, F_SP(), String("$sp"));	//$29
		Temp_enter(_F_tempMap, F_ZERO(), String("$zero"));	//$0, constant 0
		int i;
		for(i=0;i<8;i++){
			char* s = checked_malloc(8*sizeof(char)); 
			sprintf(s, "$s%d", i);
			Temp_enter(_F_tempMap, F_SN(i), s);
		}
		for(i=0;i<10;i++){
			char* s = checked_malloc(8*sizeof(char)); 
			sprintf(s, "$t%d", i);
			Temp_enter(_F_tempMap, F_TN(i), s);
		}
		for(i=0;i<4;i++){
			char* s = checked_malloc(8*sizeof(char)); 
			sprintf(s, "$a%d", i);
			Temp_enter(_F_tempMap, F_AN(i), s);
		}
	}
	return _F_tempMap;
}

static Temp_tempList regLists[4] = {NULL, NULL, NULL, NULL};

Temp_tempList F_getRegList(RL_type type){
	int tp = (int)type;
	if(regLists[tp] == NULL){
		switch(type){
		case specialregs:
			regLists[tp] = Temp_TempList(
							F_RV(), Temp_TempList(
							F_RA(), Temp_TempList(
							F_FP(), Temp_TempList(
							F_SP(), NULL
							))));
			break;
		case argregs:
			regLists[tp] = Temp_TempList(
							F_AN(0), Temp_TempList(
							F_AN(1), Temp_TempList(
							F_AN(2), Temp_TempList(
							F_AN(3), NULL
							))));
			break;	
		case calleesaves:
			regLists[tp] = Temp_TempList(
							F_SN(0), Temp_TempList(
							F_SN(1), Temp_TempList(
							F_SN(2), Temp_TempList(
							F_SN(3), Temp_TempList(
							F_SN(4), Temp_TempList(
							F_SN(5), Temp_TempList(
							F_SN(6), Temp_TempList(
							F_SN(7), NULL
							))))))));
			break;	
		case callersaves:
			regLists[tp] = Temp_TempList(
							F_TN(0), Temp_TempList(
							F_TN(1), Temp_TempList(
							F_TN(2), Temp_TempList(
							F_TN(3), Temp_TempList(
							F_TN(4), Temp_TempList(
							F_TN(5), Temp_TempList(
							F_TN(6), Temp_TempList(
							F_TN(7), Temp_TempList(
							F_TN(8), Temp_TempList(
							F_TN(9), NULL
							))))))))));
			break;		
		}
	}

	return regLists[tp];
}

void F_printFrag(F_frag frag){
	if(frag->kind == F_stringFrag){
		printf("String: label = %s; content = \"%s\"\n", S_name(frag->u.stringg.label), frag->u.stringg.str);
	} else {
		printf("Proc: locals = %d\n", frag->u.proc.frame->locals);
		printf("      offset = %d\n", frag->u.proc.frame->offset);
		printStmList(stdout, T_StmList(frag->u.proc.body, NULL));
	}
}

F_frag F_StringFrag(Temp_label label, string str){
	F_frag frag = (F_frag)checked_malloc(sizeof(struct F_frag_));
	frag->kind = F_stringFrag;
	frag->u.stringg.label = label;
	frag->u.stringg.str = str;
	return frag;
}

F_frag F_ProcFrag(T_stm body, F_frame frame){
	F_frag frag = (F_frag)checked_malloc(sizeof(struct F_frag_));
	frag->kind = F_procFrag;
	frag->u.proc.body = body;
	frag->u.proc.frame = frame;
	return frag;
}

F_fragList F_FragList(F_frag head, F_fragList tail){
	F_fragList fragList = (F_fragList)checked_malloc(sizeof(struct F_fragList_));
	fragList->head = head;
	fragList->tail = tail;
	return fragList;
}

static F_fragList *fragList = NULL;
static F_fragList fragList_head = NULL;

static F_frag* extendFragList(){
	if(fragList==NULL){
		fragList = (F_fragList*)checked_malloc(sizeof(F_fragList*));
	}

	*fragList = (F_fragList)checked_malloc(sizeof(struct F_fragList_));

	if(fragList_head==NULL){
		//Remember the head of frag list
		fragList_head = *fragList;
	}
	F_frag *currentFrag = &((*fragList)->head);
	fragList = &((*fragList)->tail);
	*fragList=NULL; 
    
	return currentFrag;
}

void F_String(Temp_label label, string str){
	F_frag *currentFrag = extendFragList();
	*currentFrag = F_StringFrag(label, str);
}

void F_Proc(T_stm body, F_frame frame){
	F_frag *currentFrag = extendFragList();
	*currentFrag = F_ProcFrag(body, frame);
//printf("New proc added to frag list\n");fflush(stdout);
}

F_fragList F_getFragList(){
	return fragList_head;
}

//Generate an IRT expression to access target address
T_exp F_Exp(F_access access, T_exp framePtr){
	if(access->kind==inFrame){
		return T_Mem(T_Binop(T_plus, framePtr, T_Const(access->u.offset)));
	} else {//access->kind==inReg
		return T_Temp(access->u.reg);
	}
}

static F_access InFrame(int offset){
	F_access acc = (F_access)checked_malloc(sizeof(struct F_access_));
	acc->kind = inFrame;
	acc->u.offset = offset;
	return acc;
}

static F_access InReg(Temp_temp reg){
	F_access acc = (F_access)checked_malloc(sizeof(struct F_access_));
	acc->kind = inReg;
	acc->u.reg = reg;
	return acc;
}

static int getNextLoc(F_frame frame){
	return frame->offset;
}

static Temp_temp getNextReg(F_frame frame){
	frame->locals++;
	return Temp_newtemp();
}

static F_access static_acc = NULL;
F_access F_staticLink(){
	if(static_acc==NULL){
		static_acc = InFrame(0);
	}
	return static_acc;
}

F_frame F_newFrame(Temp_label name, U_boolList formals){
	//1) Allocate mem for the new frame
	F_frame frame = (F_frame)checked_malloc(sizeof(struct F_frame_));
	frame->offset = 0;
	frame->locals = 0;
	frame->begin_label = name;

/* Problematic on 64-bit machine
	//2) Calculate the number of formals
	int i = 0;
	U_boolList bl;
	for(bl=formals; bl!=NULL; bl=bl->tail){
		i++;
	}

	if(i==0){
		frame->formals = NULL;
	} else {
		//3) Allocate mem for the formals in the frame
		int acc_size = sizeof(struct F_access_);
		F_accessList accList = (F_accessList)checked_malloc((i+1) * acc_size);
		F_accessList accList_head = accList;
		//Construct a list in the allocated mem
		for(;formals!=NULL;formals=formals->tail){
			accList->head = (F_access)accList;
			F_access *acc = &accList->head;
			if(formals->head==TRUE){
			//Escaping: go to memory (stack)
				*acc = InFrame(frame->offset);
				frame->offset -= F_wordSize;
			} else {//FALSE
				//Not escaping: go to register
				*acc = InReg(getNextReg(frame));
			}
			accList->tail = accList + acc_size/8;
			accList = accList->tail;
		}
		accList->tail = NULL;
		frame->formals = accList_head->tail;
	}
	return frame;
*/

	F_accessList accList = NULL;
	F_accessList accList_head = NULL;

	for(;formals!=NULL;formals=formals->tail){
		if(accList_head==NULL){
			accList = (F_accessList)checked_malloc(sizeof(struct F_accessList_));
			accList_head = accList;
		} else {
			accList->tail = (F_accessList)checked_malloc(sizeof(struct F_accessList_));
			accList = accList->tail;
		}

		if(formals->head==TRUE){
		//Escaping: go to memory (stack)
			accList->head = InFrame(frame->offset);
			frame->offset -= F_wordSize;
		} else {//FALSE
			//Not escaping: go to register
			accList->head = InReg(getNextReg(frame));
		}
	}

	if(accList!=NULL){
		accList->tail == NULL;
	}

	frame->formals = accList_head;
	return frame;
}

Temp_label F_name(F_frame frame){
	return frame->begin_label;
}

F_accessList F_formals(F_frame frame){
	return frame->formals;
}

F_access F_allocLocal(F_frame frame, bool escape){
	F_access acc;
	if(escape==TRUE){
	//Escaping: go to memory (stack)
		acc = InFrame(frame->offset);
		frame->offset -= F_wordSize;
	} else {//FALSE
		//Not escaping: go to register
		acc = InReg(getNextReg(frame));
	}
	return acc;
}

T_exp F_externalCall(string s, T_expList args){
	return T_Call(T_Name(Temp_namedlabel(s)), args);
}

static void createEscapeList(U_boolList *formal_escs, int i){
	int node_size = (sizeof(struct U_boolList_))/8;
	U_boolList escs = (U_boolList)checked_malloc(i*sizeof(struct U_boolList_));
	U_boolList escs_head = escs;
	//escs->head = TRUE;
	for(;i>0;i--){
		escs->head = TRUE; // = fList->head->escape;
		if(i>1){
			escs->tail = escs + node_size;
			escs = escs->tail; //!!!
		} else {
			escs->tail = NULL;
		}			
	}

	//

	*formal_escs = escs_head;
}


/* Only for local test *	//(add '/' after '*' to decomment)
#include <stdlib.h>
int main(int argc, char **argv) {
	//F_frame frame = F_newFrame(Temp_namedlabel("testfun"), U_BoolList(FALSE, U_BoolList(TRUE, U_BoolList(TRUE, U_BoolList(FALSE, NULL)))));
	U_boolList blist;
	createEscapeList(&blist, 3);
	F_frame frame = F_newFrame(Temp_namedlabel("testfun"), blist);
	free(blist);
	Temp_label label = F_name(frame);
	F_accessList accList = F_formals(frame);
	
	printf("%s\n", Temp_labelstring(label));
	for(;accList!=NULL;accList=accList->tail){
		F_access acc = accList->head;
		if(acc!=NULL){//inFrame, inReg
			printf("kind=%d\n", acc->kind);
			if(acc->kind==inFrame){
				printf("mem add=%d\n", acc->u.offset);
			}//Temp_temp reg;
		}
	}

	printf("\nframe->offset=%d\n", frame->offset);
	printf("frame->locals=%d\n\n", frame->locals);
	F_allocLocal(frame, TRUE);
	F_allocLocal(frame, FALSE);
	printf("frame->offset=%d\n", frame->offset);
	printf("frame->locals=%d\n", frame->locals);
	
	return 0;
}
//*/
