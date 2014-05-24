/*
 * absyn.c - Abstract Syntax Functions. Most functions create an instance of an
 *           abstract syntax rule.
 */

#include "util.h"
#include "symbol.h" /* symbol table data structures */
#include "absyn.h"  /* abstract syntax data structures */
#include "stdio.h"
#include "constants.h"

//static Var_VarList vlist = NULL;

/*************************************
 Entering into constant table
*************************************/ 
Var_VarList Const_tableEnter(Var_var newvar ,Var_VarList varlist){
	printf("Entering these into the constant table:%s = %d\n",S_name(newvar->v),newvar->c);
	Var_VarList vl=checked_malloc(sizeof(*vl));
	 vl->head = newvar;
	 vl->tail = varlist;
	 return vl;
	}
int Const_tableCheck(Var_var newvar,Var_VarList *vlist){
	Var_VarList *next;
	Var_VarList *prev;
	next=vlist;
	prev=NULL;
	
	while((*next)!=NULL){
		if(!(strcmp(S_name(newvar->v),S_name((*next)->head->v))))
		{	printf("%s Already in table!\n",S_name(newvar->v));
			printf("Removing this entry in the table\n");
			if((*next)->tail==NULL){printf("Last table elem");}
			
			//printf("reached here 2\n");
			if(prev==NULL && (*next)->tail==NULL)
			{
				printf("The only element");
				(*next)=NULL;
				Const_tablePrintExt((*vlist));
				return 0;
			}
			else
			{
				if(prev==NULL)
				{
				//printf("reached here 4\n");
				(*vlist)=(*next)->tail;
				Const_tablePrintExt((*vlist));
				return 0;
					}
				else
				{
				printf("reached here 3\n");
				(*prev)->tail=(*next)->tail;
				Const_tablePrintExt((*vlist));
				return 0;
				}
			}
			
			printf("reached here 1\n");
			

		}
		else
		{	prev=next;
			next=&((*next)->tail);
			}
		}
	return 1;
	}


void Const_tablePrintExt(Var_VarList tab)
{
	Var_VarList next;
	next=tab;
	printf("Constant list: \n");
	while(next!=NULL){
		printf("%s ",S_name(next->head->v));
		next=next->tail;
		}
	printf("\n");
	}


	


