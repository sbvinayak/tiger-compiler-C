/*
 * main.c 
 * FOR FINAL PHASE SUBMISSION WHICH INCLUDES OPTIMIZAIONS RUN WITH "r" IN THE COMMAND LINE FOR INLUDING OPTIMIZATION IN THE CODE
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "util.h"
#include "symbol.h"
#include "absyn.h"
#include "errormsg.h"
#include "temp.h" /* needed by translate.h */
#include "tree.h" /* needed by frame.h */
#include "assem.h"
#include "frame.h" /* needed by translate.h and printfrags prototype */
#include "types.h"
#include "translate.h"
#include "semant.h" /* function prototype for transProg */
#include "canon.h"
#include "prabsyn.h"
#include "printtree.h"
//#include "escape.h" //NOT AVAILABLE
//#include "parse.h" //NOT AVAILABLE; Instead copy "A_exp parse(string fname)" from parsetest.c
#include "codegen.h"
#include "graph.h"
#include "flowgraph.h"
#include "constants.h"
#include "retrace.h"


extern bool anyErrors;

static void printHelp();

extern int yyparse(void);
extern A_exp absyn_root;

/* parse source file fname; 
   return abstract syntax data structure */
static A_exp parse(string fname) {
	EM_reset(fname);
	if (yyparse() == 0) {/* parsing worked */
		return absyn_root;
	} else {
		return NULL;
	}
}

static void printHelp() 
	{
	printf("%s", 
			"TIGER compiler\n\
			<usage> tiger file.tig [a|b|c|d|e|o]*|h\n [r|n]*\
			<options> a: print IR tree\n\
			b: print linearized IR tree\n\
			c: print canonical statement list\n\
			d: print assembly code\n\
			e: print CFG (default)\n\
			h: output this help message\n\
			r: perform optimization(default) eg: tiger file.tig a r \n\
			n: no optimization eg: tiger file.tig a n \n\
			<default> tiger file.tig e n\n\
			(with result printed into file.tig.s)\n\
			\n");
}

static char* outputs = "e";
static char* optimize = "r";
static void doProc(FILE *out, F_frame frame, T_stm body)
{
	static seq = 0;

	AS_proc proc;
	//	struct RA_result allocation;
	T_stmList stmList;
	AS_instrList iList;

	if(strlen(outputs)>0){
		fprintf(out, "\nProcedure %d (%s):\n", ++seq, Temp_labelstring(F_name(frame)));
	}

	if(strchr(outputs, 'a')){
		fprintf(out, "\n(<A> original IR tree)\n");
		printStmList(out, T_StmList(body, NULL));
	}

	stmList = C_linearize(body);
	if(strchr(outputs, 'b')){
		fprintf(out, "\n(<B> linearized tree)\n");
		printStmList(out, stmList);
	}

	struct C_block block = C_basicBlocks(stmList);
	stmList = C_traceSchedule(block);
	if(strchr(outputs, 'c')){
		fprintf(out, "\n(<C> blocked and traced tree)\n");
		printStmList(out, stmList);
	}

	iList = F_codegen(frame, stmList);
	Temp_map map = Temp_layerMap(F_tempMap(), Temp_name());
	if(strchr(outputs, 'd')){	
		fprintf(out, "\n(<D> assembly code before register allocation)\n");
		fprintf(out, "BEGIN %s\n", Temp_labelstring(F_name(frame)));
		AS_printInstrList (out, iList, map);
		fprintf(out, "END %s\n", Temp_labelstring(F_name(frame)));
	}

	proc = F_procEntryExit3(frame, iList);
	G_graph graph = FG_AssemFlowGraph(proc->body);
	if(strchr(outputs, 'e')){
		fprintf(out, "\n(<E> CFG)\n");
		G_show(out, G_nodes(graph), map, (void (*)(FILE *out, void *, Temp_map))FG_Showinfo);//(void (*)(void *))FG_Showinfo
	}
}

int countlines(char *filename)
{
  // count the number of lines in the file called filename                                    
  FILE *fp = fopen(filename,"r");
  int ch=0;
  int lines=0;

  if (fp == NULL) return 0;

  lines++;
  while ((ch = fgetc(fp)) != EOF)
    {
      if (ch == '\n')
    lines++;
    }
  fclose(fp);
  return lines;
}

int main(int argc, string *argv)
{
	Var_VarList ctable;
	A_exp absyn_root = NULL,final_root=NULL;
	
	F_fragList frags = NULL;
	
	char outfile[300];
	FILE *out = stdout;
	FILE *out1 = fopen("nonPropAsynTree.s", "w");
	FILE *out2 = fopen("PropAsynTree.s", "w");
	FILE *out3 = fopen("InitialAsynTree.s", "w");
	
	if (argc>=2) {
		absyn_root = parse(argv[1]);
		if (!absyn_root) {
			return 1;
		} 
		if(argv[2]){
			outputs = argv[2];
		}
		if(argv[3]){
			optimize = argv[3];
		}
		
		final_root=absyn_root;
		
		if(strchr(optimize, 'r')){
			A_exp new_root=absyn_root;
			
			pr_exp(out3, absyn_root, 0); /* print absyn data structure */
			fclose(out3);
			
			printf("!!!!!!!!!!!nonPropAsynTree %d   ",countlines("nonPropAsynTree.s"));
			printf("!!!!!!!!!!!PropAsynTree %d  \n",countlines("PropAsynTree.s"));
			printf("Printing out here\n");
			int pass_count=0;
						
			
			do
			{
			out1 = fopen("nonPropAsynTree.s", "w");
			printf("\n--------------------PASS %d --------------------\n",pass_count);
			pr_exp(out1, new_root, 0); //print the previous iteration values into file
			fclose(out1);
			
			ret_ScanTabInit(); //Initialize the Scan constant table 
			ret_ScanLoadCT(ret_ScantabReturn()); //load the constant table for the first scan
			retScan_exp(&new_root,0); // perform checks to find constant expression 
			ret_ReplaceLoadCT(ret_ScantabReturn()); //load the new constant list into the replace by propogation table
			Const_tablePrintExt(ret_ScantabReturn());
			retReplace_exp(&new_root,0); //replace the constants by propagation
			retFold_exp(&new_root,0); //fold the constants after replacements
						
			/*Create files for comparison*/
			
			out2 = fopen("PropAsynTree.s", "w");
			pr_exp(out2, new_root, 0);  //print the current iteration values into the file
			fclose(out2);
			
			pass_count++; //increment the pass count
			printf("nonPropAsynTree %d",countlines("nonPropAsynTree.s"));
			}while((countlines("nonPropAsynTree.s")!=countlines("PropAsynTree.s")));
			
			fprintf(out, "\n");
			final_root=new_root;

		}
		if(strchr(outputs, 'h')){
			printHelp();
			return 0;
		}


		//		Esc_findEscape(absyn_root); /* set varDec's escape field */	//NOT AVAILABLE
		
		frags = SEM_transProg(final_root);
		if (anyErrors) {
			// don't continue
			return 1; 
		}

		sprintf(outfile, "%s.s", argv[1]);
			out = fopen(outfile, "w");
		
		for (;frags;frags=frags->tail) {
			if (frags->head != NULL) {
				if (frags->head->kind == F_procFrag) {
					doProc(out, frags->head->u.proc.frame, frags->head->u.proc.body);
				} /* else if (frags->head->kind == F_stringFrag) {
				     fprintf(out, "%s\n", frags->head->u.stringg.str);
				     }*/
			}
		}
		
			fclose(out);
		
	}
	printHelp();	
	return 1;
}
