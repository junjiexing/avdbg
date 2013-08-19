#pragma once

#include <x86dis.h>
#include <list>
#include <vector>
#include <deque>
#include "DebugKernel.h"

class debug_kernel;

class x86Analysis
{
public:
	typedef struct tag_code_block
	{
		uint32 start;
		uint32 end;
	}code_block;
//private:
	x86dis	disasmbler_;
	std::list<uint32> entry_addr_list;

	byte* code_to_analy_;
	uint32 code_size_;
	uint32 code_start_va_;

	std::deque<x86dis_insn> cache_insn;


	std::vector<code_block> blocks_vect_;
	void get_block(uint32 uEntry);
	uint32 get_block_size(uint32 uEntry);
	byte* va_to_code_offset(uint32 va)
	{
		assert(va >= code_start_va_ && va <= (code_start_va_ + code_size_));
		return code_to_analy_ + (va - code_start_va_);
	}
	uint32 code_offset_to_va(byte* offset)
	{
		assert(offset >= code_to_analy_ && offset <= (code_to_analy_ + code_size_));
		return code_start_va_ + (offset - code_to_analy_);
	}

	enum BRANCHTYPE				//用于判断是否是转移指令
	{
		BR_NONE,				// 没有分支
		BR_JMP,
		BR_RET,
		BR_CALL,
		BR_JCC
	};
	x86Analysis::BRANCHTYPE is_branch(x86dis_insn* opcode);
	bool is_jump_table(x86dis_insn* insn);
	CPU_ADDR branch_addr(x86dis_insn *opcode);
	void add_block(const code_block& block);
	void disasm_block(const code_block& block,std::vector<std::string>& asmcode);

	std::weak_ptr<debug_kernel> debug_kernel_wp_;

public:
	x86Analysis(byte* code_to_analy, unsigned code_size, unsigned long block_start_addr, std::shared_ptr<debug_kernel>& debug_kernel_ptr);
	~x86Analysis(void);

	bool is_addr_valid(uint32 addr)
	{return (addr >= code_start_va_ && addr < (code_start_va_+code_size_));}

	bool add_entry(uint32 entry_addr)
	{
		if (!is_addr_valid(entry_addr))
		{
			return false;
		}

		for (auto it = entry_addr_list.begin();
			it != entry_addr_list.end();++it)
		{
			if (*it == entry_addr)
			{
				return true;
			}
		}

		entry_addr_list.push_back(entry_addr);
		return true;
	}
	//bool IsAddrDis(uint32 uAddr){return false;}
	bool process(std::vector<std::string>& asmcode);
};

