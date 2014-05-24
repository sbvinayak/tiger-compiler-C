/* function prototype from retrace.c */
#ifndef _RETRACE_H_
#define _RETRACE_H_

//void ret_LoadCT(Var_VarList tab);
A_exp ret_Refold(A_pos pos, A_oper oper, A_exp left, A_exp right);

void ret_ScanTabInit();
Var_VarList ret_ScantabReturn();

void retReplace_exp(  A_exp *v, int d);
void retFold_exp(  A_exp *v, int d);
void retScan_exp(  A_exp *v, int d);

void ret_ReplaceLoadCT(Var_VarList tab);
void ret_ScanLoadCT(Var_VarList tab);
#endif
