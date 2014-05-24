		/*
		 * retfold.c
		 This module performs the constant folding
		 */
		
		#include <stdio.h>
		#include "util.h"
		#include "symbol.h" /* symbol table data structures */
		#include "absyn.h"  /* abstract syntax data structures */
		#include "constants.h"
		#include "retrace.h" /* function prototype */
		
		/* local function prototypes */
		static void ret_var(  A_var *v, int d);
		static void ret_dec(  A_dec *v, int d);
		static void ret_ty(  A_ty *v, int d);
		static void ret_field(  A_field *v, int d);
		static void ret_fieldList(  A_fieldList *v, int d);
		static void ret_expList(  A_expList *v, int d);
		static void ret_fundec(  A_fundec *v, int d);
		static void ret_fundecList(  A_fundecList *v, int d);
		static void ret_decList(  A_decList *v, int d);
		static void ret_namety(  A_namety *v, int d);
		static void ret_nametyList(  A_nametyList *v, int d);
		static void ret_efield(  A_efield *v, int d);
		static void ret_efieldList(  A_efieldList *v, int d);
		static int folded=0;
	
		
		A_exp ret_Refold(A_pos pos, A_oper oper, A_exp left, A_exp right)
		{
			
			A_exp p = checked_malloc(sizeof(*p));
				p->kind = A_opExp;
				p->pos = pos;
				p->u.op.oper = oper;
				p->u.op.left = left;
				p->u.op.right = right;
			//printf("VINPRINT: Left operation : %d , Right_operation: %d, Left kind : %d , Current Operation : %d , Left value:%d Right value:%d ,Right kind : %d Right value: %d\n",left->u.op.oper,right->u.op.oper,left->kind,oper,left->u.intt,right->u.intt,right->kind,right->u.intt);
			if (left->kind == A_intExp  && right->kind == A_intExp)
			{
				int l,r;
								
				l=left->u.intt;
				r=right->u.intt;
				//printf("VINPRINT: Entered int int oper %d pos %d\n",oper,pos);
				
				//printf("VINPRINT:!!@$@#$ Entered int int oper %d pos %d\n",oper,pos);
				//Checking operation and performing const folding
				
				switch(oper)
				{
				case A_plusOp: folded=1;return A_IntExp(pos, (left->u.intt) + (right->u.intt));break;
				case A_minusOp:  folded=1;return A_IntExp(pos, left->u.intt - right->u.intt);break;
				case A_timesOp:   folded=1;return A_IntExp(pos, left->u.intt * right->u.intt);break;
				case A_divideOp:   folded=1;return A_IntExp(pos, left->u.intt / right->u.intt);break;
				default: return p; break;
				
				}
				
				
				
				}
				
			else if ( (left->kind == A_opExp  && right->kind == A_intExp) && ((left->u.op.oper)!= A_timesOp) &&((left->u.op.oper)!= A_divideOp)&&((left->u.op.right)->kind==A_intExp))
			{
				A_exp leftexp,op_chl,op_chr;
				leftexp= left->u.op.right;
				//printf("VINPRINT: Entered here\n");
				//Checking operation and performing const folding
				switch(oper)
				{
				case A_plusOp: leftexp->u.intt = leftexp->u.intt + right->u.intt;break;
				case A_minusOp: leftexp->u.intt = leftexp->u.intt - right->u.intt;break;
				case A_timesOp:  leftexp->u.intt = leftexp->u.intt * right->u.intt;break;
				case A_divideOp:  leftexp->u.intt = leftexp->u.intt / right->u.intt;break;
				default: return p; break;
				}
				folded=1;
				left->u.op.right = leftexp;
				return left;
			}
		
			else
			{
				
			//printf("VINPRINT: Entered last else Operation : %d , Right Exp operation: %d\n",p->u.op.oper, (p->u.op.right)->u.op.oper);
			return p;
			folded=0;
			}
		}
		
		/* Print A_var types. Indent d spaces. */
		static void ret_var(  A_var *v, int d) {
		 //indent(  d);
		 switch ((*v)->kind) {
		 case A_simpleVar:
		   //fprintf(  "simpleVar(%s)", S_name(v->u.simple)); 
		   break;
		 case A_fieldVar:
		   //fprintf(  "%s\n", "fieldVar(");
		   ret_var(  &(*v)->u.field.var, d+1); //fprintf(  "%s\n", ","); 
		   //indent(  d+1); //fprintf(  "%s)", S_name((*v)->u.field.sym));
		   break;
		 case A_subscriptVar:
		   //fprintf(  "%s\n", "subscriptVar(");
		   ret_var(  &(*v)->u.subscript.var, d+1); //fprintf(  "%s\n", ","); 
		   retFold_exp(  &(*v)->u.subscript.exp, d+1); //fprintf(  "%s", ")");
		   break;
		 default:
		   assert(0); 
		 } 
		}
		
		static char str_oper[][12] = {
		   "PLUS", "MINUS", "TIMES", "DIVIDE", 
		   "EQUAL", "NOTEQUAL", "LESSTHAN", "LESSEQ", "GREAT", "GREATEQ"};
		 
		static void ret_oper(  A_oper d) {
		  //fprintf(  "%s", str_oper[d]);
		}
		
		/* Print A_var types. Indent d spaces. */
		void retFold_exp(  A_exp *v, int d) {
			//printf("Entered here retexp %d\n", (*v)->kind);
		 //indent(  d);
		 switch ((*v)->kind) {
		 case A_varExp:
		   //fprintf(  "varExp(\n");
		   //printf(  "varExp(\n");
		   ret_var(  &(*v)->u.var, d+1);
		   //f//printf(  "%s", ")");
		   break;
		 case A_nilExp:
		   //fprintf(  "nilExp()");
		   //printf(  "nilExp()");
		   break;
		 case A_intExp:
		   //fprintf(  "intExp(%d)", (*v)->u.intt);
		   //printf(  "intExp(%d)", (*v)->u.intt);
		   break;
		 case A_stringExp:
		   //fprintf(  "stringExp(%s)", (*v)->u.stringg);
		   //printf(  "stringExp(%s)", (*v)->u.stringg);
		   break;
		 case A_callExp:
		   //fprintf(  "callExp(%s,\n", S_name((*v)->u.call.func));
		   //printf(  "callExp(%s,\n", S_name((*v)->u.call.func));
		   ret_expList(  &(*v)->u.call.args, d+1); //fprintf(  ")");
		   break;
		 case A_opExp:
		   //fprintf(  "opExp(\n");
		   //printf(  "opExp(\n");
		   //indent(  d+1); 
		   (*v)=ret_Refold((*v)->pos, (*v)->u.op.oper, (*v)->u.op.left, (*v)->u.op.right);
		   //printf("Folded: %d",folded);
		   //pr_exp(stdout,(*v),0);
		   if(folded==0){
				ret_oper(  (*v)->u.op.oper); //fprintf(  ",\n"); 
				retFold_exp(  &(*v)->u.op.left, d+1); //fprintf(  ",\n"); 
				retFold_exp(  &(*v)->u.op.right, d+1); ////fprintf(  ")");
			}
			else
			{
				folded=0;
			}
		   break;
		 case A_recordExp:
		   ////fprintf(  "recordExp(%s,\n", S_name((*v)->u.record.typ)); 
		   ret_efieldList(  &(*v)->u.record.fields, d+1); ////fprintf(  ")");
		   break;
		 case A_seqExp:
		   ////fprintf(  "seqExp(\n");
		   ret_expList(  &(*v)->u.seq, d+1); ////fprintf(  ")");
		   break;
		 case A_assignExp:
		   ////fprintf(  "assignExp(\n");
		   //printf(  "assignExp(\n");
		   ret_var(  &(*v)->u.assign.var, d+1); ////fprintf(  ",\n");
		   //printf("fine here11!!!");
		   retFold_exp(  &(*v)->u.assign.exp, d+1); ////fprintf(  ")");
		   break;
		 case A_ifExp:
		   ////fprintf(  "iffExp(\n");
		   retFold_exp(  &(*v)->u.iff.test, d+1); ////fprintf(  ",\n");
		   retFold_exp(  &(*v)->u.iff.then, d+1);
		   if ((*v)->u.iff.elsee) { /* else is optional */
		      ////fprintf(  ",\n");
		      retFold_exp(  &(*v)->u.iff.elsee, d+1);
		   }
		   //fprintf(  ")");
		   break;
		 case A_whileExp:
		   //fprintf(  "whileExp(\n");
		   retFold_exp(  &(*v)->u.whilee.test, d+1); //fprintf(  ",\n");
		   retFold_exp(  &(*v)->u.whilee.body, d+1); //fprintf(  ")\n");
		   break;
		 case A_forExp:
		   //fprintf(  "forExp(%s,\n", S_name((*v)->u.forr.var)); 
		   retFold_exp(  &(*v)->u.forr.lo, d+1); //fprintf(  ",\n");
		   retFold_exp(  &(*v)->u.forr.hi, d+1); //fprintf(  "%s\n", ",");
		   retFold_exp(  &(*v)->u.forr.body, d+1); //fprintf(  ",\n");
		   //indent(  d+1); //fprintf(  "%s", (*v)->u.forr.escape ? "TRUE)" : "FALSE)");
		   break;
		 case A_breakExp:
		   //fprintf(  "breakExp()");
		   break;
		 case A_letExp:
		   //fprintf(  "letExp(\n");
		   ret_decList(  &(*v)->u.let.decs, d+1); //fprintf(  ",\n");
		   retFold_exp(  &(*v)->u.let.body, d+1); //fprintf(  ")");
		   break;
		 case A_arrayExp:
		   //fprintf(  "arrayExp(%s,\n", S_name((*v)->u.array.typ));
		   retFold_exp(  &(*v)->u.array.size, d+1); //fprintf(  ",\n");
		   retFold_exp(  &(*v)->u.array.init, d+1); //fprintf(  ")");
		   break;
		 default:
		   assert(0); 
		 } 
		}
		
		static void ret_dec(  A_dec *v, int d) {
		 //indent(  d);
		 switch ((*v)->kind) {
		 case A_functionDec:
		   //fprintf(  "functionDec(\n"); 
		   ret_fundecList(  &(*v)->u.function, d+1); //fprintf(  ")");
		   break;
		 case A_varDec:
		   //fprintf(  "varDec(%s,\n", S_name((*v)->u.var.var));
		   retFold_exp(  &(*v)->u.var.init, d+1); //fprintf(  ",\n");
		   //indent(  d+1); //fprintf(  "%s", (*v)->u.var.escape ? "TRUE)" : "FALSE)");
		   break;
		 case A_typeDec:
		   //fprintf(  "typeDec(\n"); 
		   ret_nametyList(  &(*v)->u.type, d+1); //fprintf(  ")");
		   break;
		 default:
		   assert(0); 
		 } 
		}
		
		static void ret_ty(  A_ty *v, int d) {
		 //indent(  d);
		 switch ((*v)->kind) {
		 case A_nameTy:
		   //fprintf(  "nameTy(%s)", S_name((*v)->u.name));
		   break;
		 case A_recordTy:
		   //fprintf(  "recordTy(\n");
		   ret_fieldList(  &(*v)->u.record, d+1); //fprintf(  ")");
		   break;
		 case A_arrayTy:
		   //fprintf(  "arrayTy(%s)", S_name((*v)->u.array));
		   break;
		 default:
		   assert(0); 
		 } 
		}
		
		static void ret_field(  A_field *v, int d) {
		 //indent(  d);
		 //fprintf(  "field(%s,\n", S_name((*v)->name));
		 //indent(  d+1); //fprintf(  "%s,\n", S_name((*v)->typ));
		 //indent(  d+1); //fprintf(  "%s", (*v)->escape ? "TRUE)" : "FALSE)");
		}
		
		static void ret_fieldList(  A_fieldList *v, int d) {
		 //indent(  d);
		 if (*v) {
		   //fprintf(  "fieldList(\n");
		   ret_field(  &(*v)->head, d+1); //fprintf(  ",\n");
		   ret_fieldList(  &(*v)->tail, d+1); //fprintf(  ")");
		 }
		 else 
		 { //fprintf(  "fieldList()");
		 }
		}
		
		static void ret_expList(  A_expList *v, int d) {
		 //indent(  d);
		 if (*v) {
		   //fprintf(  "expList(\n"); 
		   retFold_exp(  &(*v)->head, d+1); //fprintf(  ",\n");
		   ret_expList(  &(*v)->tail, d+1);
		   //fprintf(  ")");
		 }
		 else {
			 //fprintf(  "expList()"); 
		 }
		
		}
		
		static void ret_fundec(  A_fundec *v, int d) {
		 //indent(  d);
		 //fprintf(  "fundec(%s,\n", S_name((*v)->name));
		 ret_fieldList(  &(*v)->params, d+1); //fprintf(  ",\n");
		 if ((*v)->result) {
		   //indent(  d+1); //fprintf(  "%s,\n", S_name((*v)->result));
		 }
		 retFold_exp(  &(*v)->body, d+1); //fprintf(  ")");
		}
		
		static void ret_fundecList(  A_fundecList *v, int d) {
		 //indent(  d);
		 if (*v) {
		   //fprintf(  "fundecList(\n"); 
		   ret_fundec(  &(*v)->head, d+1); //fprintf(  ",\n");
		   ret_fundecList(  &(*v)->tail, d+1); //fprintf(  ")");
		 }
		 else {
		 //fprintf(  "fundecList()");
		}
		}
		
		static void ret_decList(  A_decList *v, int d) {
		 //indent(  d);
		 if (*v) {
		   //fprintf(  "decList(\n"); 
		   
		   ret_dec(  &(*v)->head, d+1); //fprintf(  ",\n");
		   
		   ret_decList(  &(*v)->tail, d+1);
		   //fprintf(  ")");
		 }
		 else {
			 //fprintf(  "decList()"); 
		 }
		
		}
		
		static void ret_namety(  A_namety *v, int d) {
		 //indent(  d);
		 //fprintf(  "namety(%s,\n", S_name((*v)->name)); 
		 ret_ty(  &(*v)->ty, d+1); //fprintf(  ")");
		}
		
		static void ret_nametyList(  A_nametyList *v, int d) {
		 //indent(  d);
		 if (*v) {
		   //fprintf(  "nametyList(\n"); 
		   ret_namety(  &(*v)->head, d+1); //fprintf(  ",\n");
		   ret_nametyList(  &(*v)->tail, d+1); //fprintf(  ")");
		 }
		 else {
			 //fprintf(  "nametyList()");
		 }
		 
		}
		
		static void ret_efield(  A_efield *v, int d) {
		 //indent(  d);
		 if (*v) {
		   //fprintf(  "efield(%s,\n", S_name((*v)->name));
		   retFold_exp(  &(*v)->exp, d+1); //fprintf(  ")");
		 }
		 else {
			 //fprintf(  "efield()");
		 }
		}
		
		static void ret_efieldList(  A_efieldList *v, int d) {
		 //indent(  d);
		 if (*v) {
		   //fprintf(  "efieldList(\n"); 
		   ret_efield(  &(*v)->head, d+1); //fprintf(  ",\n");
		   ret_efieldList(  &(*v)->tail, d+1); //fprintf(  ")");
		 }
		 else {
			 //fprintf(  "efieldList()");
		 }
		}
		
		
		
		
		
	
