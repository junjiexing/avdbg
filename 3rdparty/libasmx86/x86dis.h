/* 
*	HT Editor
*	x86dis.h
*
*	Copyright (C) 1999-2002 Stefan Weyergraf
*	Copyright (C) 2005-2007 Sebastian Biallas (sb@biallas.net)
*
*	This program is free software; you can redistribute it and/or modify
*	it under the terms of the GNU General Public License version 2 as
*	published by the Free Software Foundation.
*
*	This program is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with this program; if not, write to the Free Software
*	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef __X86DIS_H__
#define __X86DIS_H__

#include "x86opc.h"
#include <string>

/* generic disassembler styles */
//#define DIS_STYLE_HIGHLIGHT		0x80000000		/* create highlighting information in strf() */
#define DIS_STYLE_HEX_CSTYLE		0x40000000		/* IF SET: mov eax, 0x12345678 		ELSE: mov eax, 12345678 */
#define DIS_STYLE_HEX_ASMSTYLE		0x20000000		/* IF SET: mov eax, 12345678h 		ELSE: mov eax, 12345678 */
#define DIS_STYLE_HEX_UPPERCASE		0x10000000		/* IF SET: mov eax, 5678ABCD	 	ELSE: mov eax, 5678abcd */
#define DIS_STYLE_HEX_NOZEROPAD		0x08000000		/* IF SET: mov eax, 8002344	 	ELSE: mov eax, 008002344 */
#define DIS_STYLE_SIGNED		0x04000000		/* IF SET: mov eax, -1	 		ELSE: mov eax, 0ffffffffh */

#define DIS_STYLE_TABSIZE			12

enum AsmSyntaxHighlightEnum
{
	e_cs_default=0,
	e_cs_comment,
	e_cs_number,
	e_cs_symbol,
	e_cs_string
};

/*****************************************************************************
*	The strf() format                                                       *
*****************************************************************************
String	Action
--------------------------------------------------
%x		substitute expression with symbol "x"
?xy...y	if symbol "x" is undefined leave out the whole expression,
otherwise subsitute expression with string between the two "y"s

Symbol	Desc
--------------------------------------------------
p 		prefix
n 		name
1 		first operand
2 		second operand
3 		third operand
4 		forth operand
*/

#define DISASM_STRF_VAR			'%'
#define DISASM_STRF_COND		'?'

#define DISASM_STRF_PREFIX		'p'
#define DISASM_STRF_NAME		'n'
#define DISASM_STRF_FIRST		'1'
#define DISASM_STRF_SECOND		'2'
#define DISASM_STRF_THIRD		'3'
#define DISASM_STRF_FORTH		'4'
#define DISASM_STRF_FIFTH		'5'

//#define DISASM_STRF_DEFAULT_FORMAT	"?p#%p #%n\t%1?2#, %2?3/, %3/?4-, %4-#"
#define DISASM_STRF_DEFAULT_FORMAT	"?p#%p #%n\t%1?2#, %2#?3#, %3#?4#, %4#?5#, %5#"
#define DISASM_STRF_SMALL_FORMAT	"?p#%p #%n?1# %1#?2#,%2#?3#,%3#?4#,%4#?5#,%5#"

//#define MAGIC32(magic) (unsigned long)(((unsigned char)magic[0]<<24) | ((unsigned char)magic[1]<<16) | ((unsigned char)magic[2]<<8) | (unsigned char)magic[3])

#define ASM_SYNTAX_DEFAULT "\\@d"
#define ASM_SYNTAX_COMMENT "\\@#"
#define ASM_SYNTAX_NUMBER "\\@n"
#define ASM_SYNTAX_SYMBOL "\\@c"
#define ASM_SYNTAX_STRING "\\@s"


#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#define X86DIS_OPCODE_CLASS_STD		0		/* no prefix */
#define X86DIS_OPCODE_CLASS_EXT		1		/* 0F */
#define X86DIS_OPCODE_CLASS_EXT_66	2		/* 66 0F */
#define X86DIS_OPCODE_CLASS_EXT_F2	3		/* F2 0F */
#define X86DIS_OPCODE_CLASS_EXT_F3	4		/* F3 0F */
#define X86DIS_OPCODE_CLASS_EXTEXT	5		/* 0F 0F */

/* x86-specific styles */
#define X86DIS_STYLE_EXPLICIT_MEMSIZE	0x00000001	/* IF SET: mov word ptr [0000], ax 	ELSE: mov [0000], ax */
#define X86DIS_STYLE_OPTIMIZE_ADDR	0x00000002	/* IF SET: mov [eax*3], ax 		ELSE: mov [eax+eax*2+00000000], ax */

struct x86dis_vex
{
	uint8 mmmm;
	uint8 vvvv;
	uint8 l;
	uint8 w;
	uint8 pp;
};

struct x86dis_insn
{
	bool invalid;
	sint8 opsizeprefix;
	sint8 lockprefix;
	sint8 repprefix;
	sint8 segprefix;
	uint8 rexprefix;
	x86dis_vex vexprefix;
	int size;
	int opcode;
	int opcodeclass;
	X86OpSize eopsize;
	X86AddrSize eaddrsize;
	bool ambiguous;
	const char *name;
	x86_insn_op op[5];
};

struct x86dis_str 
{
	char prefix[64];
	char opcode[32];
	int operand_len[5];
	char operand[5][512];
};

/*
*	CLASS x86dis
*/

class x86dis
{
public:
	X86OpSize opsize;
	X86AddrSize addrsize;

	x86opc_insn (*x86_insns)[256];

protected:
	x86dis_insn insn;
	char insnstr[256];
	byte *codep, *ocodep;
	CPU_ADDR addr;
	byte c;
	int modrm;
	int sib;
	int drex;
	int maxlen;
	int special_imm;
	uint32 disp;
	bool have_disp;
	bool fixdisp;

	/* new */
	virtual void checkInfo(x86opc_insn *xinsn);
	void decode_insn(x86opc_insn *insn);
	void decode_vex_insn(x86opc_vex_insn *xinsn);
	virtual void decode_modrm(x86_insn_op *op, char size, bool allow_reg, bool allow_mem, bool mmx, bool xmm, bool ymm);
	void decode_op(x86_insn_op *op, x86opc_insn_op *xop);
	void decode_sib(x86_insn_op *op, int mod);
	int	esizeop(uint c);
	int	esizeop_ex(uint c);
	byte getbyte();
	uint16 getword();
	uint32 getdword();
	uint64 getqword();
	int	getmodrm();
	int	getsib();
	int	getdrex();
	uint32 getdisp();
	int	getspecialimm();
	void invalidate();
	bool isfloat(char c);
	bool isaddr(char c);
	virtual void prefixes();
	void str_format(char **str, const char **format, char *p, char *n, char *op[3], int oplen[3], char stopchar, int print);
	virtual void str_op(char *opstr, int *opstrlen, const x86dis_insn *insn, const x86_insn_op *op, bool explicit_params);
	uint mkmod(uint modrm);
	uint mkreg(uint modrm);
	uint mkindex(uint modrm);
	uint mkrm(uint modrm);
	virtual uint64 getoffset();
	virtual void filloffset(CPU_ADDR &addr, uint64 offset);

	//Disassembler
	int options;
	//bool highlight;
	char* (*addr_sym_func)(CPU_ADDR addr, int *symstrlen, void *context);
	void* addr_sym_func_context;

	//const char *get_cs(AsmSyntaxHighlightEnum style);
	void hexd(char **s, int size, int options, uint32 imm);
	void hexq(char **s, int size, int options, uint64 imm);
	//void enable_highlighting();
	//void disable_highlighting();

public:
	x86dis(X86OpSize opsize, X86AddrSize addrsize);

	/* overwritten */
	virtual	dis_insn* decode(byte *code, int maxlen, CPU_ADDR addr);
	virtual	dis_insn* duplicateInsn(dis_insn *disasm_insn);
	virtual	void getOpcodeMetrics(int &min_length, int &max_length, int &min_look_ahead, int &avg_look_ahead, int &addr_align);
	virtual	const char*	getName();
	virtual	byte getSize(dis_insn *disasm_insn);
	virtual const char* str(dis_insn *disasm_insn, int options);
	virtual const char* strf(dis_insn *disasm_insn, int options, const char *format);
	virtual bool str_insn( const x86dis_insn* insn,int opt, x86dis_str& result);
	virtual bool validInsn(dis_insn *disasm_insn);

	//Disassembler
	virtual	dis_insn *createInvalidInsn();
	virtual	bool selectNext(dis_insn *disasm_insn);
	void set_addr_sym_func(char* (*pfn)(CPU_ADDR addr, int *symstrlen, void *context),void* pContext);
};

class x86_64dis: public x86dis
{
	static x86opc_insn (*x86_64_insns)[256];
public:	
	x86_64dis();
	virtual	void checkInfo(x86opc_insn *xinsn);
	virtual	void decode_modrm(x86_insn_op *op, char size, bool allow_reg, bool allow_mem, bool mmx, bool xmm, bool ymm);
	virtual	void prefixes();
	virtual	uint64 getoffset();
	virtual	void filloffset(CPU_ADDR &addr, uint64 offset);

	void prepInsns();
};

#endif /* __X86DIS_H__ */
