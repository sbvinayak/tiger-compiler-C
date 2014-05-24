/* 
 * Header for the codegen phase
 *
  */
#ifndef _CODEGEN_H_
#define _CODEGEN_H_

AS_instrList F_codegen(F_frame f, T_stmList stmList);

AS_proc F_procEntryExit3(F_frame frame, AS_instrList body);

#endif
