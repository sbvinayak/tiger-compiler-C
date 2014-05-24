/*
 * Activation Record Module - MIPS architecture
 */

#ifndef _MIPSFRAME_H_
#define _MIPSFRAME_H_

struct F_frame_ {
	F_accessList formals;
	int locals;
	int offset;
	Temp_label begin_label;
};

struct F_access_ {
	enum {inFrame, inReg} kind;
	union {
		int offset;
		Temp_temp reg;
	} u;
};

#endif
