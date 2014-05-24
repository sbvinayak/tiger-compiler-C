/* 
 * CS5161 Assignment 6
 * 
 * TIGER's Generation of Control Flow Graph
 *
 * Ming Zhou (4465225)
 * Kuo-shih Tseng (4436736)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "table.h"
#include "symbol.h"
#include "absyn.h"
#include "temp.h"
#include "tree.h"
#include "frame.h"
#include "assem.h"
#include "codegen.h"
#include "graph.h"

/* Interfaces */
struct _UDinfo {
	Temp_tempList uses;
	Temp_tempList defs;
	bool isMove;
};
typedef struct _UDinfo *UDinfo;

//Create a table mapping between node and UDinfo
static void UD_init();
//Enter a new node with UDinfo into the table
static void UD_enter(G_node n, UDinfo info);
//Retrieve info from the table
static UDinfo UD_lookup(G_node n);
//Create a UDInfo with given information
static UDinfo UDInfo(Temp_tempList uses, Temp_tempList defs, bool isMove);

//Get the Lable-Node table
static TAB_table LNTable();
//Enter a new entry into the label-node table
static void LT_enter(Temp_label l, G_node n);
//Look up a node using lable as key
static G_node LT_lookup(Temp_label l);

/* Implementation */
static G_table UDTable = NULL;

static void UD_init(){
	if (UDTable==NULL) {
		UDTable = G_empty();
	}
}

static void UD_enter(G_node n, UDinfo info) {
	G_enter(UDTable, n, info);
}

static UDinfo UD_lookup(G_node n) {
	return G_look(UDTable, n);
}

static UDinfo UDInfo(Temp_tempList uses, Temp_tempList defs, bool isMove) {
	UDinfo info = (UDinfo)checked_malloc(sizeof(struct _UDinfo));
	info->uses = uses;
	info->defs = defs;
	info->isMove = isMove;
	return info;
}

static TAB_table _lntable = NULL;

static TAB_table LNTable() {
	if (_lntable == NULL){
		_lntable = TAB_empty();
	}
	return _lntable;
}

static G_node LT_lookup(Temp_label l) {
	return (G_node)TAB_look(LNTable(), l);
}

static void LT_enter(Temp_label l, G_node n) {
	TAB_enter(LNTable(), l, n);
}

Temp_tempList FG_def(G_node n) {
	return UD_lookup(n)->defs;
}

Temp_tempList FG_use(G_node n) {
	return UD_lookup(n)->uses;
}

void FG_Showinfo(FILE *out, AS_instr instr, Temp_map map) {
	char* cs;
	char* lb;
    char r[200]; /* result */
	switch(instr->kind) {
	case I_OPER:
		cs = instr->u.OPER.assem;
		lb = strrchr(cs, '\n');
		if(lb!=NULL){
			*lb = '\0';
		}
		AS_format(r, cs, instr->u.OPER.dst, instr->u.OPER.src, instr->u.OPER.jumps, map);
		fprintf(out, "[%20s] ", r);//instr->u.OPER.assem);
		break;
	case I_LABEL:
		cs = instr->u.LABEL.assem;
		lb = strrchr(cs, '\n');
		if(lb!=NULL){
			*lb = '\0';
		}
    	AS_format(r, cs, NULL, NULL, NULL, map);
		fprintf(out, "[%20s] ", r);
		break;
	case I_MOVE:
		cs = instr->u.MOVE.assem;
		lb = strrchr(cs, '\n');
		if(lb!=NULL){
			*lb = '\0';
		}
   		AS_format(r, cs, instr->u.MOVE.dst, instr->u.MOVE.src, NULL, map);
		fprintf(out, "[%20s] ", r);
		break;
	}
}

#define IT_COMMON 0
#define IT_JUMP 1
#define IT_MOVE 2
G_graph FG_AssemFlowGraph(AS_instrList il) {
	UD_init();

	//(I) Iterate over the entire instruction list
	AS_instr instr = NULL;
	G_node prev = NULL;
	G_node curr = NULL;
	G_graph graph = G_Graph();
	G_nodeList jumpList = NULL;
	G_nodeList jumpListHead = NULL;
	for(; il!=NULL; il=il->tail){
		instr = il->head;
		if(instr!=NULL){
			//1) create a node (and put it into the graph), using the 
			//   instruction as the associated info.
			curr = G_Node(graph, instr);

			//2) special handling
			int type = IT_COMMON;
			Temp_tempList defs = NULL;
			Temp_tempList uses = NULL;
			switch(instr->kind) {
			case I_OPER:
				// Check if it's a JUMP instruction
				// We do this check here by looking if As_target is null, 
				// instead of inspecting the assembly op (j, beq, ...)
				if(instr->u.OPER.jumps!=NULL &&
					instr->u.OPER.jumps->labels!=NULL){
					type = IT_JUMP;
					// put this instruction into a separate list
					if(jumpList==NULL){
						jumpList = G_NodeList(curr, NULL);
						jumpListHead = jumpList;
					} else {
						jumpList->tail = G_NodeList(curr, NULL);
						jumpList = jumpList->tail;
					}
				}
				defs = instr->u.OPER.dst;
				uses = instr->u.OPER.src;
				break;
			case I_LABEL:
				//2.2) label should be also saved in the label-node list for (II)
				LT_enter(instr->u.LABEL.label, curr);
				break;
			case I_MOVE:
				//2.3) it's a move instruction
				type = IT_MOVE;
				defs = instr->u.MOVE.dst;
				uses = instr->u.MOVE.src;
				break;
			}
			
			//3) put information into table
			UD_enter(curr, UDInfo(uses, defs, type == IT_MOVE));

			//4) link with the previous node for falling through, if possible.
			//   Note that prev is NULL if the previous instruction is a JUMP.
			if(prev!=NULL){
				G_addEdge(prev, curr);
			}

			//5) set as previous node for next time of iteration
			prev = (type!=IT_JUMP)?curr:NULL;
		}
	}

	//(II) Iterate over the list that has all the JUMP instruction collected.
	Temp_labelList labels;
	for(; jumpListHead!=NULL; jumpListHead=jumpListHead->tail){
		curr = jumpListHead->head;
		labels = ((AS_instr)G_nodeInfo(curr))->u.OPER.jumps->labels;//no need to check its nullity again
		Temp_label label;
		G_node dest;
		// for each target it may jump to, add a corresponding edge in the graph
		for(;labels!=NULL;labels=labels->tail){
			label = labels->head;
			if(label!=NULL){
				// quickly retieve the target node using the label-node table
				dest = LT_lookup(label);
				// establish edge between this node and its jump target
				G_addEdge(curr, dest);
			}
		}
	}

	return graph;
}
#undef IT_COMMON
#undef IT_JUMP
#undef IT_MOVE
