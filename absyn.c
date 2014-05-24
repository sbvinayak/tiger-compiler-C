/*
 * absyn.c - Abstract Syntax Functions. Most functions create an instance of an
 *           abstract syntax rule.
 */

#include "util.h"
#include "symbol.h" /* symbol table data structures */
#include "absyn.h"  /* abstract syntax data structures */
#include "stdio.h"
#include "constants.h"



A_var A_SimpleVar(A_pos pos, S_symbol sym)
{A_var p = checked_malloc(sizeof(*p));
 p->kind=A_simpleVar;
 p->pos=pos;
 p->u.simple=sym;
 return p;
}

A_var A_FieldVar(A_pos pos, A_var var, S_symbol sym)
{A_var p = checked_malloc(sizeof(*p));
 p->kind=A_fieldVar;
 p->pos=pos;
 p->u.field.var=var;
 p->u.field.sym=sym;
 return p;
}

A_var A_SubscriptVar(A_pos pos, A_var var, A_exp exp)
{A_var p = checked_malloc(sizeof(*p));
 p->kind=A_subscriptVar;
 p->pos=pos;
 p->u.subscript.var=var;
 p->u.subscript.exp=exp;
 return p;
}


A_exp A_VarExp(A_pos pos, A_var var)
{A_exp p = checked_malloc(sizeof(*p));
 p->kind=A_varExp;
 p->pos=pos;
 p->u.var=var;
 return p;
}

A_exp A_NilExp(A_pos pos)
{A_exp p = checked_malloc(sizeof(*p));
 p->kind=A_nilExp;
 p->pos=pos;
 return p;
}

A_exp A_IntExp(A_pos pos, int i)
{ A_exp p = checked_malloc(sizeof(*p));
 p->kind=A_intExp;
 p->pos=pos;
 p->u.intt=i;
 return p;
}

A_exp A_StringExp(A_pos pos, string s)
{A_exp p = checked_malloc(sizeof(*p));
 p->kind=A_stringExp;
 p->pos=pos;
 p->u.stringg=s;
 return p;
}

A_exp A_CallExp(A_pos pos, S_symbol func, A_expList args)
{A_exp p = checked_malloc(sizeof(*p));
 p->kind=A_callExp;
 p->pos=pos;
 p->u.call.func=func;
 p->u.call.args=args;
 return p;
}
/*
A_exp A_OpExp(A_pos pos, A_oper oper, A_exp left, A_exp right)
{A_exp p = checked_malloc(sizeof(*p));
 p->kind=A_opExp;
 p->pos=pos;
 p->u.op.oper=oper;
 p->u.op.left=left;
 p->u.op.right=right;
 return p;
}
*/


//Vin-edit
/*
A_exp A_OpExp(A_pos pos, A_oper oper, A_exp left, A_exp right)
{
	//printf("VINPRINT: Left operation : %d , Right_operation: %d, Left kind : %d , Current Operation : %d , Left value:%d Right value:%d \n ,",left->u.op.oper,right->u.op.oper,left->kind,oper,left->u.intt,right->u.intt);
	if (left->kind == A_intExp  && right->kind == A_intExp)
	{
		//Checking operation and performing const folding
		switch(oper)
		{
		case A_plusOp: return A_IntExp(pos, left->u.intt + right->u.intt);break;
		case A_minusOp: return A_IntExp(pos, left->u.intt - right->u.intt);break;
		case A_timesOp:  return A_IntExp(pos, left->u.intt * right->u.intt);break;
		case A_divideOp:  return A_IntExp(pos, left->u.intt / right->u.intt);break;
		default: printf("invalid operand\n"); break;
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
		default: printf("invalid operand\n"); break;
		}
		
		left->u.op.right = leftexp;
		return left;
	}

	else
	{
		
	A_exp p = checked_malloc(sizeof(*p));
	p->kind = A_opExp;
	p->pos = pos;
	p->u.op.oper = oper;
	p->u.op.left = left;
	p->u.op.right = right;
	//printf("VINPRINT: Entered last else Operation : %d , Right Exp operation: %d\n",p->u.op.oper, (p->u.op.right)->u.op.oper);
	return p;
	
	}
}
*/

A_exp A_OpExp(A_pos pos, A_oper oper, A_exp left, A_exp right)
{
	
	A_exp p = checked_malloc(sizeof(*p));
	p->kind = A_opExp;
	p->pos = pos;
	p->u.op.oper = oper;
	p->u.op.left = left;
	p->u.op.right = right;
	return p;

}


A_exp A_RecordExp(A_pos pos, S_symbol typ, A_efieldList fields)
{A_exp p = checked_malloc(sizeof(*p));
 p->kind=A_recordExp;
 p->pos=pos;
 p->u.record.typ=typ;
 p->u.record.fields=fields;
 return p;
}

A_exp A_SeqExp(A_pos pos, A_expList seq)
{A_exp p = checked_malloc(sizeof(*p));
 p->kind=A_seqExp;
 p->pos=pos;
 p->u.seq=seq;
 return p;
}

A_exp A_AssignExp(A_pos pos, A_var var, A_exp exp)
{A_exp p = checked_malloc(sizeof(*p));
 p->kind=A_assignExp;
 p->pos=pos;
 p->u.assign.var=var;
 p->u.assign.exp=exp;
 //Vin-edit removed const allocation from here
 return p;
}

A_exp A_IfExp(A_pos pos, A_exp test, A_exp then, A_exp elsee)
{A_exp p = checked_malloc(sizeof(*p));
 p->kind=A_ifExp;
 p->pos=pos;
 p->u.iff.test=test;
 p->u.iff.then=then;
 p->u.iff.elsee=elsee;
 return p;
}

A_exp A_WhileExp(A_pos pos, A_exp test, A_exp body)
{A_exp p = checked_malloc(sizeof(*p));
 p->kind=A_whileExp;
 p->pos=pos;
 p->u.whilee.test=test;
 p->u.whilee.body=body;
 return p;
}

A_exp A_ForExp(A_pos pos, S_symbol var, A_exp lo, A_exp hi, A_exp body)
{A_exp p = checked_malloc(sizeof(*p));
 p->kind=A_forExp;
 p->pos=pos;
 p->u.forr.var=var;
 p->u.forr.lo=lo;
 p->u.forr.hi=hi;
 p->u.forr.body=body;
 p->u.forr.escape=TRUE;
 return p;
}

A_exp A_BreakExp(A_pos pos)
{A_exp p = checked_malloc(sizeof(*p));
 p->kind=A_breakExp;
 p->pos=pos;
 return p;
}

A_exp A_LetExp(A_pos pos, A_decList decs, A_exp body)
{A_exp p = checked_malloc(sizeof(*p));
 p->kind=A_letExp;
 p->pos=pos;
 p->u.let.decs=decs;
 p->u.let.body=body;
 return p;
}

A_exp A_ArrayExp(A_pos pos, S_symbol typ, A_exp size, A_exp init)
{A_exp p = checked_malloc(sizeof(*p));
 p->kind=A_arrayExp;
 p->pos=pos;
 p->u.array.typ=typ;
 p->u.array.size=size;
 p->u.array.init=init;
 return p;
}

A_dec A_FunctionDec(A_pos pos, A_fundecList function)
{A_dec p = checked_malloc(sizeof(*p));
 p->kind=A_functionDec;
 p->pos=pos;
 p->u.function=function;
 return p;
}

A_dec A_VarDec(A_pos pos, S_symbol var, S_symbol typ, A_exp init)
{A_dec p = checked_malloc(sizeof(*p));
 p->kind=A_varDec;
 p->pos=pos;
 p->u.var.var=var;
 p->u.var.typ=typ;
 p->u.var.init=init;
 p->u.var.escape=TRUE;
// removed Constant table entry form here
	 
 return p;
}

A_dec A_TypeDec(A_pos pos, A_nametyList type)
{A_dec p = checked_malloc(sizeof(*p));
 p->kind=A_typeDec;
 p->pos=pos;
 p->u.type=type;
 return p;
}

A_ty A_NameTy(A_pos pos, S_symbol name)
{A_ty p = checked_malloc(sizeof(*p));
 p->kind=A_nameTy;
 p->pos=pos;
 p->u.name=name;
 return p;
}

A_ty A_RecordTy(A_pos pos, A_fieldList record)
{A_ty p = checked_malloc(sizeof(*p));
 p->kind=A_recordTy;
 p->pos=pos;
 p->u.record=record;
 return p;
}

A_ty A_ArrayTy(A_pos pos, S_symbol array)
{A_ty p = checked_malloc(sizeof(*p));
 p->kind=A_arrayTy;
 p->pos=pos;
 p->u.array=array;
 return p;
}

A_field A_Field(A_pos pos, S_symbol name, S_symbol typ)
{A_field p = checked_malloc(sizeof(*p));
 p->pos=pos;
 p->name=name;
 p->typ=typ;
 p->escape=TRUE;
 return p;
}

A_fieldList A_FieldList(A_field head, A_fieldList tail)
{A_fieldList p = checked_malloc(sizeof(*p));
 p->head=head;
 p->tail=tail;
 return p;
}

A_expList A_ExpList(A_exp head, A_expList tail)
{A_expList p = checked_malloc(sizeof(*p));
 p->head=head;
 p->tail=tail;
 return p;
}

A_fundec A_Fundec(A_pos pos, S_symbol name, A_fieldList params, S_symbol result,
		  A_exp body)
{A_fundec p = checked_malloc(sizeof(*p));
 p->pos=pos;
 p->name=name;
 p->params=params;
 p->result=result;
 p->body=body;
 return p;
}

A_fundecList A_FundecList(A_fundec head, A_fundecList tail)
{A_fundecList p = checked_malloc(sizeof(*p));
 p->head=head;
 p->tail=tail;
 return p;
}

A_decList A_DecList(A_dec head, A_decList tail)
{A_decList p = checked_malloc(sizeof(*p));
 p->head=head;
 p->tail=tail;
 return p;
}

A_namety A_Namety(S_symbol name, A_ty ty)
{A_namety p = checked_malloc(sizeof(*p));
 p->name=name;
 p->ty=ty;
 return p;
}

A_nametyList A_NametyList(A_namety head, A_nametyList tail)
{A_nametyList p = checked_malloc(sizeof(*p));
 p->head=head;
 p->tail=tail;
 return p;
}

A_efield A_Efield(S_symbol name, A_exp exp)
{A_efield p = checked_malloc(sizeof(*p));
 p->name=name;
 p->exp=exp;
 return p;
}

A_efieldList A_EfieldList(A_efield head, A_efieldList tail)
{A_efieldList p = checked_malloc(sizeof(*p));
 p->head=head;
 p->tail=tail;
 return p;
}

void print_S_symbol(S_symbol s) {
  if(s == NULL) 
     printf("UnDefined");
  else 
     printf("%s", S_name(s));
}


void print_absyn_exp(A_exp exp, int indent) {
  if(exp == NULL) return; 
  switch(exp->kind) {
  case A_varExp:
    print_VarExp(exp, indent); 
    break;
  case A_nilExp:
    print_NilExp(exp, indent); 
    break;
  case A_intExp:
    print_IntExp(exp, indent); 
    break;
  case A_stringExp:
    print_StringExp(exp, indent); 
    break;
  case A_callExp:
    print_CallExp(exp, indent); 
    break;
  case A_opExp:
    print_OpExp(exp, indent); 
    break;
  case A_recordExp:
    print_RecordExp(exp, indent); 
    break;
  case A_seqExp:
    print_SeqExp(exp, indent); 
    break;
  case A_assignExp:
    print_AssignExp(exp, indent); 
    break;
  case A_ifExp:
    print_IfExp(exp, indent); 
    break;
  case A_whileExp:
     print_WhileExp(exp, indent); 
    break;
  case A_forExp:
    print_ForExp(exp, indent); 
    break;
  case A_breakExp:
    print_BreakExp(exp, indent); 
    break;
  case A_letExp:
    print_LetExp(exp, indent); 
    break;
  case A_arrayExp:
    print_ArrayExp(exp, indent); 
    break;
  default:
    printf("Illegal Expression");
  }
}

void print_absyn_var(A_var var, int indent) {
  if(var == 0) return; 
  switch(var->kind) {
  case A_simpleVar:
    print_SimpleVar(var, indent);
    break;
  case A_fieldVar:
    print_FieldVar(var, indent);
    break;
  case A_subscriptVar:
    print_SubscriptVar(var, indent);
    break;
  default:
    printf("Illegal Var"); 
  }
}

void print_absyn_dec(A_dec dec, int indent) {
  if(dec == 0) return; 
  switch (dec->kind) {
  case A_functionDec:
    print_FunctionDec(dec, indent); 
    break; 
  case A_varDec:
    print_VarDec(dec, indent);
    break; 
  case A_typeDec:
    print_TypeDec(dec, indent);
    break; 
  default:
    printf("Illegal Dec");
  }
}

void print_absyn_ty(A_ty ty, int indent) {
  if(ty == 0) return; 
  switch(ty->kind) {
  case A_nameTy:
    print_NameTy(ty, indent);
    break; 
  case A_recordTy:
    print_RecordTy(ty, indent);
    break; 
  case A_arrayTy:
    print_ArrayTy(ty, indent);
    break;
  default:
    printf("Illegal Ty");
  }
}

void print_absyn_oper(A_oper oper) {
  switch(oper) {
  case A_plusOp:
    printf("+");
    break;
  case A_minusOp:
    printf("-");
    break;
  case A_timesOp:
    printf("*");
    break;
  case A_divideOp:
    printf("*");
    break;
  case A_eqOp:
    printf("=");
    break; 
  case A_neqOp:
    printf("-");
    break;
  case A_ltOp:
    printf("<");
    break; 
  case A_leOp:
    printf("<=");
    break;
  case A_gtOp:
    printf(">");
    break;
  case A_geOp:
    printf(">=");
    break;
  default:
    printf("Illegal Operator"); 
  }
}


void print_SimpleVar(A_var var, int indent){
  print_indent(indent);
  
  printf("SimpleVar: "); 
  print_S_symbol(var->u.simple);
  print_end(); 
}

void print_FieldVar(A_var var, int indent) {
  print_indent(indent);
  printf("FieldVar: ");

  print_indent(indent);
  printf(" sym = ");
  print_S_symbol(var->u.field.sym);

  print_indent(indent);
  printf(" var = ");
  print_absyn_var(var->u.field.var, indent + 1);

  print_end(); 
}

void print_SubscriptVar(A_var var, int indent) {
  print_indent(indent);
  printf("SubscriptVar: ");

  print_indent(indent);
  printf(" var = ");
  print_absyn_var(var->u.subscript.var, indent + 1);

  print_indent(indent);
  printf(" exp = ");
  print_absyn_exp(var->u.subscript.exp, indent + 1);

  print_end();  
}

void print_VarExp(A_exp exp, int indent) {
  print_indent(indent);
  printf("VarExp: ");
  print_absyn_var(exp->u.var, indent + 1);

  print_end(); 
}

void print_NilExp(A_exp exp, int indent) {
  print_indent(indent);
  printf("NilExp");

  print_end(); 
}

void print_IntExp(A_exp exp, int indent) {
  print_indent(indent);
  printf("IntExp: %d", exp->u.intt);
  print_end(); 
}

void print_StringExp(A_exp exp, int indent) {
  print_indent(indent);
  printf("StringExp: %s", exp->u.stringg);

  print_end(); 
}

void print_CallExp(A_exp exp, int indent) {
  print_indent(indent);
  printf("CallExp: ");

  print_indent(indent);
  printf(" func = ");
  print_S_symbol(exp->u.call.func);

  print_indent(indent);
  printf(" args = ");
  print_ExpList(exp->u.call.args, indent + 1);

  print_end(); 
}

void print_OpExp(A_exp exp, int indent) {
  print_indent(indent);

  printf("OpExp ("); 
  print_absyn_oper(exp->u.op.oper);
  printf(")"); 

  print_indent(indent);
  printf(" left"); 
  print_absyn_exp(exp->u.op.left, indent + 1);

  print_indent(indent);
  printf(" right"); 
  print_absyn_exp(exp->u.op.right, indent + 1);

  print_end(); 
}

void print_RecordExp(A_exp exp, int indent) {
  print_indent(indent);
  printf("RecordExp: ");

  print_indent(indent);
  printf(" typ =  ");
  print_S_symbol(exp->u.record.typ);
  
  print_indent(indent);
  printf(" fields = ");
  print_EfieldList(exp->u.record.fields, indent + 1);
  
  print_end(); 
}

void print_SeqExp(A_exp exp, int indent) {
  print_indent(indent);
  printf("SeqExp: ");
  print_ExpList(exp->u.seq, indent + 1); 

  print_end(); 
}

void print_AssignExp(A_exp exp, int indent) {
  print_indent(indent);

  printf("Assign: ");

  print_indent(indent);
  printf(" var = ");
  print_absyn_var(exp->u.assign.var, indent + 1);

  print_indent(indent);
  printf(" exp = ");
  print_absyn_exp(exp->u.assign.exp, indent + 1);  

  print_end(); 
}

void print_IfExp(A_exp exp, int indent) {
  print_indent(indent);

  printf("If: ");

  print_indent(indent);
  printf(" test = ");
  print_absyn_exp(exp->u.iff.test, indent + 1);

  print_indent(indent);
  printf(" then = ");
  print_absyn_exp(exp->u.iff.then, indent + 1);

  print_indent(indent);
  printf(" else = ");
  print_absyn_exp(exp->u.iff.elsee, indent + 1); 

  print_end();
}

void print_WhileExp(A_exp exp, int indent) {
  print_indent(indent);
  printf("While:");

  print_indent(indent);
  printf(" test = "); 
  print_absyn_exp(exp->u.whilee.test, indent + 1);

  print_indent(indent);
  printf(" body = "); 
  print_absyn_exp(exp->u.whilee.body, indent + 1);

  print_end();
}

void print_ForExp(A_exp exp, int indent) {
  print_indent(indent);
  printf("For: ", exp->u.forr.escape);

  print_indent(indent);
  printf(" escape = %d", exp->u.forr.escape);

  print_indent(indent);
  printf(" var = ");
  print_S_symbol(exp->u.forr.var);
  
  printf(" low = "); 
  print_indent(indent);
  print_absyn_exp(exp->u.forr.lo, indent + 1);
  
  printf(" high = "); 
  print_indent(indent);
  print_absyn_exp(exp->u.forr.hi, indent + 1);
  
  printf(" body "); 
  print_indent(indent);
  print_absyn_exp(exp->u.forr.body, indent + 1);
  
  print_end();
}

void print_BreakExp(A_exp exp, int indent) {
  print_indent(indent);

  printf("Break: ");
  
  print_end();
}

void print_LetExp(A_exp exp, int indent) {
  print_indent(indent);
  printf("Let: ");

  print_indent(indent);
  printf(" decs = ");
  print_DecList(exp->u.let.decs, indent + 1);

  print_indent(indent);
  printf(" body = ");
  print_absyn_exp(exp->u.let.body, indent + 1); 

  print_end();
}

void print_ArrayExp(A_exp exp, int indent) {
  print_indent(indent);
  printf("Array: ");

  print_indent(indent);
  printf(" type = ");
  print_S_symbol(exp->u.array.typ);
  
  print_indent(indent);
  printf(" size = "); 
  print_absyn_exp(exp->u.array.size, indent + 1);
  
  print_indent(indent);
  printf(" init = ");
  print_absyn_exp(exp->u.array.init, indent + 1); 

  print_end();
}

void print_FunctionDec(A_dec dec, int indent) {
  print_indent(indent);

  printf("FunctionDec: ");
  print_FundecList(dec->u.function, indent + 1);

  print_end();
}

void print_VarDec(A_dec dec, int indent) {
  print_indent(indent);
  printf("VarDec: ");
  
  print_indent(indent);
  printf(" name = ");
  print_S_symbol(dec->u.var.var);

  print_indent(indent);
  printf(" escape = %d ", dec->u.var.escape);

  print_indent(indent);
  printf(" type = ");
  print_S_symbol(dec->u.var.typ);

  print_indent(indent);
  printf(" init = ");
  print_absyn_exp(dec->u.var.init, indent + 1);
  
  print_end();
}

void print_TypeDec(A_dec dec, int indent) {
  print_indent(indent);
  printf("TypeDec: ");

  print_NametyList(dec->u.type, indent + 1);
  print_end();
}

void print_NameTy(A_ty ty, int indent) {
  print_indent(indent);

  printf("NameTy: ");
  print_S_symbol(ty->u.name);

  print_end();
}

void print_RecordTy(A_ty ty, int indent) {
  print_indent(indent);

  printf("RecordTy: ");
  print_FieldList(ty->u.record, indent + 1); 

  print_end();
}

void print_ArrayTy(A_ty ty, int indent) {
  print_indent(indent);

  printf("ArrayTy: ");
  print_S_symbol(ty->u.array);
  
  print_end();
}


void print_Field(A_field field, int indent) {
  print_indent(indent);

  printf("Field: ");

  print_indent(indent);
  printf(" escape = %d ", field->escape);

  print_indent(indent);
  printf(" name = ");
  print_S_symbol(field->name);

  print_indent(indent);
  printf(" type = ");
  print_S_symbol(field->typ);
  
  print_end();
}
void print_Fundec(A_fundec fundec, int indent) {
  print_indent(indent);

  printf("Fundec: ");

  print_indent(indent);
  printf(" result = "); 
  print_S_symbol(fundec->result);

  print_indent(indent);
  printf(" name = "); 
  print_S_symbol(fundec->name);

  print_indent(indent);
  printf(" params = "); 
  print_FieldList(fundec->params, indent + 1);

  print_indent(indent);
  printf(" body = "); 
  print_absyn_exp(fundec->body, indent + 1); 

  print_end();
}

void print_Namety(A_namety namety, int indent) {
  print_indent(indent);
  printf("Namety: ");

  print_indent(indent);
  printf(" name = ");
  print_S_symbol(namety->name);

  print_indent(indent);
  printf(" type = ");
  print_absyn_ty(namety->ty, indent + 1);
  
  print_end();
}

void print_Efield(A_efield efield, int indent) {
  print_indent(indent);
  printf("Efield: ");

  print_indent(indent);
  printf(" name = ");
  print_S_symbol(efield->name);

  print_indent(indent);
  printf(" exp = ");
  print_absyn_exp(efield->exp, indent + 1);
  
  print_end();
}


void print_EfieldList(A_efieldList efieldList, int indent) {
  if(efieldList == NULL) return; 
  print_Efield(efieldList->head, indent);
  print_EfieldList(efieldList->tail, indent);
}

void print_FieldList(A_fieldList fieldList, int indent) {
  if(fieldList == NULL) return; 
  print_Field(fieldList->head, indent);
  print_FieldList(fieldList->tail, indent);
}

void print_ExpList(A_expList expList, int indent) {
  if(expList == NULL) return; 
  print_absyn_exp(expList->head, indent);
  print_ExpList(expList->tail, indent);   
}

void print_NametyList(A_nametyList nametyList, int indent) {
  if(nametyList == NULL) return; 
  print_Namety(nametyList->head, indent);
  print_NametyList(nametyList->tail, indent); 
}

void print_FundecList(A_fundecList fundecList, int indent) {
  if(fundecList == NULL) return; 
  print_Fundec(fundecList->head, indent);
  print_FundecList(fundecList->tail, indent);
}


void print_DecList(A_decList decList, int indent) {
  if(decList == NULL) return; 
  print_absyn_dec(decList->head, indent);
  print_DecList(decList->tail, indent); 
}


void print_indent(int indent) {
  int i;
  printf("\n");
  for (i = 0; i < indent; i++)
    printf("    ");
}

void print_end() {
}
