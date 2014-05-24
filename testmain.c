#include <stdlib.h>
#include <stdio.h>
#include "util.h"

static void createEscapeList(int num);

int main(int argc, string *argv)
{
	createEscapeList(3);
}

static void createEscapeList(int num){
int j=0;
	U_boolList escs = NULL;
	U_boolList escs_head = NULL;
	for(;num>0;num--){
		if(escs_head==NULL){
			escs = U_BoolList(TRUE, NULL);
			escs_head = escs;
		} else {
			escs->tail = U_BoolList(TRUE, NULL);
			escs = escs->tail;	
		}
printf("addr escs=%d\n", (int)escs);
		
j++;
	}
	if(escs!=NULL){
		escs->tail = NULL;
	}
printf("escapes: %d in total\n", j);

j=0;
U_boolList l = escs_head;
for(;l;l=l->tail){
printf("addr l=%d\n", (int)l);
printf("addr tail=%d\n", (int)l->tail);
j++;
}
printf("escapes recheck: %d in total\n", j);
}
