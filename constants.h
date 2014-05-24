/*
 * constants.h - For const folding and propogation
 *

/* Type Definitions */

#ifndef _CONSTANTS_H_
#define _CONSTANTS_H_
/*************************************
 For creating constant array
*************************************/ 
//Vin-edit create a table for constants
typedef struct Var_var_ *Var_var;
struct Var_var_ {S_symbol v; int c;};
typedef struct Var_VarList_ *Var_VarList;
struct Var_VarList_ { Var_var head; Var_VarList tail;};

Var_VarList Const_tableEnter(Var_var newvar ,Var_VarList varlist);
int Const_tableCheck(Var_var newvar,Var_VarList *vlist);

void Const_tablePrintExt(Var_VarList tab);
Var_VarList Const_tableReturn();

//Vin-edit-ends
#endif
