#include "stdafx.h"
#include "x86Analysis.h"
#include "DebugUtils.h"
#include <algorithm>
#include <string.h>

x86Analysis::x86Analysis( byte* code_to_analy, unsigned code_size, unsigned long code_start_va, std::shared_ptr<DebugKernel>& debug_kernel_ptr)
	:disasmbler_(X86_OPSIZE32,X86_ADDRSIZE32),
	code_to_analy_(code_to_analy),code_size_(code_size),
	code_start_va_(code_start_va),cache_insn(10),debug_kernel_wp_(debug_kernel_ptr)
{

}

x86Analysis::~x86Analysis( void )
{

}

void x86Analysis::get_block( uint32 entry_addr )
{
	if (!is_addr_valid(entry_addr))
	{
		return;
	}

	CPU_ADDR	current_addr;
	current_addr.addr32.seg = 0;
	current_addr.addr32.offset = entry_addr;
	uint32 end_addr = 0;

	debug_utils::ScopeExit add_block_on_exit([entry_addr,&end_addr,this]()
	{
		assert(entry_addr != 0);
		assert(end_addr != 0);
		code_block block = {entry_addr,end_addr};
		add_block(block);
	});

	for (unsigned i=entry_addr-code_start_va_;i<code_size_;)
	{
		//assert(curAddr.addr32.offset != 0x00401029);
		x86dis_insn* insn = (x86dis_insn*)disasmbler_.decode(code_to_analy_+i,code_size_-i,current_addr);

		//const char* pcsIns = m_Decoder.str(insn,DIS_STYLE_HEX_ASMSTYLE | DIS_STYLE_HEX_UPPERCASE | DIS_STYLE_HEX_NOZEROPAD);
		//printf("%08X\t%s\n",curAddr.addr32.offset, pcsIns);
		i += insn->size;
		current_addr.addr32.offset += insn->size;

		if (cache_insn.size()>=10)
		{
			cache_insn.pop_back();
		}
		cache_insn.push_front(*insn);

		end_addr = code_offset_to_va(code_to_analy_ + i);

		switch (is_branch(insn))
		{
		case BR_RET:
			return;
		case BR_JMP:
			{
				if (is_jump_table(insn))
				{
					// 获取跳转表地址
					BYTE* index_tab_addr = NULL;
					BYTE* address_tab_addr = (BYTE*)insn->op[0].mem.disp;
					int index_size = 0;
					int index_tab_size = 0;
					for each (x86dis_insn insn in cache_insn)
					{
						if (insn.name == NULL)
						{
							continue;
						}

						if (strstr(insn.name,"mov")
							&& insn.op[1].type == X86_OPTYPE_MEM
							&& insn.op[0].type == X86_OPTYPE_REG
							&& index_tab_addr == NULL)
						{
							index_tab_addr = (BYTE*)insn.op[1].mem.disp;
							index_size = insn.op[1].size;
							continue;
						}
						if (strstr(insn.name,"cmp")
							&& insn.op[1].type == X86_OPTYPE_IMM
							&& index_tab_size == 0)
						{
							index_tab_size = insn.op[1].imm+1;
							continue;
						}

// 						if (strstr(insn.name,""))
// 						{
// 							continue;
// 						}
					}

					if (index_tab_addr == NULL
						|| address_tab_addr == NULL
						|| index_tab_size == 0
						|| index_size == 0)
					{
						// 构成跳转表的东西不够
						return;
					}

					BYTE index_tab_cpy[index_tab_size*index_size];
					//BOOL ret = ReadProcessMemory(hProcess,pIndexTabAddr,pIndexTabCpy,nIndexTabSize*nIndexSize,&nRead);
					std::shared_ptr<DebugKernel> debug_kernel_ptr = debug_kernel_wp_.lock();
					debug_kernel_ptr->read_debugee_memory(index_tab_addr,index_tab_cpy,index_tab_size*index_size);

					int index = 0;

					for (int i=0;i<index_tab_size*index_size;i+=index_size)
					{
						switch (index_size)
						{
						case 1:
							{
								index = index_tab_cpy[i];
							}
							break;
						case 2:
							{
								index = (WORD)index_tab_cpy[i];
							}
							break;
						case 4:
							{
								index = (DWORD)index_tab_cpy[i];
							}
							break;
						}
						uint32 address = 0;
						//ReadProcessMemory(hProcess,pAddressTabAddr+nIndex*4,&address,4,NULL);
						debug_kernel_ptr->read_debugee_memory(address_tab_addr+index*4,&address,4);
						AtlTrace("Index:%d,Address:0x%08x\n",index,address);
						add_entry(address);
					}
					return;
				}
				else
				{
					CPU_ADDR addr = branch_addr(insn);
					add_entry(addr.addr32.offset);
					return;
				}
			}
		case BR_JCC:
			{
				CPU_ADDR addr = branch_addr(insn);
				//AddEntry(uEnd);
				get_block(end_addr);
				add_entry(addr.addr32.offset);
				return;
			}
		case BR_CALL:
			{
				CPU_ADDR addr = branch_addr(insn);
				add_entry(end_addr);
				add_entry(addr.addr32.offset);
			}
			break;
		}
	}

}


uint32 x86Analysis::get_block_size( uint32 entry_addr )
{
	if (!is_addr_valid(entry_addr))
	{
		assert(false);
		return 0;
	}

	CPU_ADDR	current_addr;
	current_addr.addr32.offset = entry_addr;

	for (unsigned i=entry_addr-code_start_va_;i<code_size_;)
	{
		x86dis_insn* insn = (x86dis_insn*)disasmbler_.decode(code_to_analy_+i,code_size_-i,current_addr);

		i += insn->size;
		current_addr.addr32.offset += insn->size;
		BRANCHTYPE type = is_branch(insn);
		if (type == BR_JMP || type == BR_RET)
		{
			break;
		}
	}

	return current_addr.addr32.offset - entry_addr;
}


x86Analysis::BRANCHTYPE x86Analysis::is_branch(x86dis_insn* opcode)
{
	const char *opcode_str = opcode->name;
	if (opcode_str[0] == '~')
	{
		opcode_str++;
	}
	if (opcode_str[0] == '|')
	{
		opcode_str++;
	}

	if (opcode_str[0]=='j')
	{
		if (opcode_str[1]=='m')
			return BR_JMP;
		else
			return BR_JCC;
	}
	else if ((opcode_str[0]=='l') && (opcode_str[1]=='o')  && (opcode_str[2]=='o'))
	{
		// loop opcode will be threated like a jXX
		return BR_JCC;
	}
	else if ((opcode_str[0]=='c') && (opcode_str[1]=='a'))
	{
		return BR_CALL;
	}
	else if ((opcode_str[0]=='r') && (opcode_str[1]=='e'))
	{
		return BR_RET;
	}
	else return BR_NONE;
}

CPU_ADDR x86Analysis::branch_addr(x86dis_insn *opcode)
{
	CPU_ADDR addr = {0};
	//assert(o->op[1].type == X86_OPTYPE_EMPTY);
	if (opcode->op[1].type != X86_OPTYPE_EMPTY)
	{
		return addr;
	}
	switch (opcode->op[0].type)
	{
	case X86_OPTYPE_IMM:
		{		
			addr.addr32.offset = opcode->op[0].imm;
			break;
		}
		// 	case X86_OPTYPE_FARPTR:
		// 		break;
	case X86_OPTYPE_MEM:
		{
			if (opcode->op[0].mem.hasdisp)
			{
				//addr.addr32.offset = opcode->op[0].mem.disp;
				std::shared_ptr<DebugKernel> debug_kernel_ptr = debug_kernel_wp_.lock();
				debug_kernel_ptr->read_debugee_memory((LPVOID)opcode->op[0].mem.disp,&addr.addr32.offset,4);
			}
			break;
		}
// 	case X86_OPTYPE_REG:
// 		{
// 			assert(false);
// 		}
// 		break;
	default: break;
	}
	return addr;
}

bool x86Analysis::process( std::vector<std::string>& asmcode )
{
	// TODO:添加更多函数开始的特征码
	for (byte* i = code_to_analy_; i < code_to_analy_+code_size_;)
	{
		uint32 length = code_to_analy_+code_size_-i;
		// 检查push ebp;mov ebp,sp
		if (
			length > 3 &&		// 剩的代码不超过3字节的话就不检查了
			*i == 0x55 &&			// 0x55为push ebp的机器码 
			( (*(i+1) == 0x8B && *(i+2) == 0xEC) || (*(i+1) == 0x89 && *(i+2) == 0xE5) ) // 两种mov ebp,esp的编码方式
			)
		{
			uint32 va = code_offset_to_va(i);
			add_entry(va);
			i += get_block_size(va);
			continue;
		}


		// 检查push ebp;mov eax,[esp+xx]
		if (
			length > 4 &&
			*i == 0x55 &&
			(*(i+1) == 0x8B && *(i+2) == 0x44 && *(i+3) == 0x24 )
			)
		{
			uint32 va = code_offset_to_va(i);
			add_entry(va);
			i += get_block_size(va);
			continue;
		}

		// 直接就是mov eax,[esp+xx]
		if (
			length > 3 &&
			(*i == 0x8B && *(i+1) == 0x44 && *(i+2) == 0x24 )
			)
		{
			uint32 va = code_offset_to_va(i);
			add_entry(va);
			i += get_block_size(va);
			continue;
		}

		// enter xx,0指令
		if (length > 4 && *i == 0xC8 && *(i+3) == 0x00 )
		{
			uint32 va = code_offset_to_va(i);
			add_entry(va);
			i += get_block_size(va);
			continue;
		}

		// mov exb,esp
		if (length > 2 && *i == 0x8B && *(i+1) == 0xDC)
		{
			uint32 va = code_offset_to_va(i);
			add_entry(va);
			i += get_block_size(va);
			continue;
		}
		++i;
	}

	for each (uint32 entry in entry_addr_list)
	{
		get_block(entry);
	}

	std::sort(blocks_vect_.begin(),blocks_vect_.end(),
		[](code_block& a,code_block& b)->bool
	{
		assert(a.start != b.start);
		assert(a.end != b.end);
		return a.start < b.start;
	});

	uint32 start_va = code_start_va_;
	char	szBuffer[100]; // FIXME：可能缓冲区溢出
	for each (const code_block& block in blocks_vect_)
	{
		if (start_va != block.start)
		{
			for (uint32 i = start_va; i < block.start; ++i)
			{
				sprintf(szBuffer,"%08X  DB %02X",i,*va_to_code_offset(i));
				asmcode.push_back(std::string(szBuffer));
			}
		}

		disasm_block(block,asmcode);
		start_va = block.end;
	}

	FILE* f = fopen("asm.txt","a+");
	for each (std::string s in asmcode)
	{
		fprintf(f,"%s\n",s.c_str());
	}
	fclose(f);

	return true;
}

void x86Analysis::add_block( const code_block& block )
{
	for (auto it = blocks_vect_.begin();
		it != blocks_vect_.end();++it)
	{
		code_block old = *it;
		if (old.start == block.start)
		{
			if (old.end == block.end)
			{
				return;
			}

			if (old.end > block.end)
			{
				code_block tmp1,tmp2;
				blocks_vect_.erase(it);
				tmp1 = block;
				tmp2.start = block.end;
				tmp2.end = old.end;
				add_block(tmp1);
				add_block(tmp2);
				return;
			}

			code_block tmp1,tmp2;
			blocks_vect_.erase(it);
			tmp1 = old;
			tmp2.start = old.end;
			tmp2.end = block.end;
			add_block(tmp1);
			add_block(tmp2);
			return;
		}

		if (block.start > old.start && block.start < old.end)
		{
			if (block.end == old.end)
			{
				code_block tmp1,tmp2;
				tmp1.start = old.start;
				tmp1.end = block.start;
				tmp2 = block;
				add_block(tmp1);
				add_block(tmp2);
				return;
			}

			if (block.end < old.end)
			{
				code_block tmp1,tmp2,tmp3;
				tmp1.start = old.start;
				tmp1.end = block.start;
				tmp2 = block;
				tmp3.start = block.end;
				tmp3.end = old.end;
				add_block(tmp1);
				add_block(tmp2);
				add_block(tmp3);
				return;
			}

			code_block tmp1,tmp2,tmp3;
			tmp1.start = old.start;
			tmp1.end = block.start;
			tmp2.start = block.start;
			tmp2.end = old.end;
			tmp3.start = old.end;
			tmp3.end = block.end;
			add_block(tmp1);
			add_block(tmp2);
			add_block(tmp3);
			return;
		}

		if (old.start > block.start && old.start < block.end)
		{
			if (old.end == block.end)
			{
				code_block tmp1,tmp2;
				tmp1.start = block.start;
				tmp1.end = old.start;
				tmp2 = old;
				add_block(tmp1);
				add_block(tmp2);
				return;
			}

			if (old.end < block.end)
			{
				code_block tmp1,tmp2,tmp3;
				tmp1.start = block.start;
				tmp1.end = old.start;
				tmp2 = old;
				tmp3.start = old.end;
				tmp3.end = block.end;
				add_block(tmp1);
				add_block(tmp2);
				add_block(tmp3);
				return;
			}

			code_block tmp1,tmp2,tmp3;
			tmp1.start = block.start;
			tmp1.end = old.start;
			tmp2.start = old.start;
			tmp2.end = block.end;
			tmp3.start = block.end;
			tmp3.end = old.end;
			add_block(tmp1);
			add_block(tmp2);
			add_block(tmp3);
			return; 
		}
	}

	blocks_vect_.push_back(block);
}

void x86Analysis::disasm_block( const code_block& block,std::vector<std::string>& asmcode )
{
	if (!is_addr_valid(block.start))
	{
		assert(false);
		return;
	}

	CPU_ADDR	current_addr;
	current_addr.addr32.offset = block.start;
	char szBuffer[100]; // FIXME：可能缓冲区溢出
	byte* pCodeStart = va_to_code_offset(block.start);

	for (unsigned i = 0;
		i < block.end - block.start;)
	{
		x86dis_insn* insn = (x86dis_insn*)disasmbler_.decode(pCodeStart+i,code_size_-i,current_addr);

		const char* pcsIns = disasmbler_.str(insn,DIS_STYLE_HEX_ASMSTYLE | DIS_STYLE_HEX_UPPERCASE | DIS_STYLE_HEX_NOZEROPAD);
		//printf("%08X\t%s\n",curAddr.addr32.offset, pcsIns);
		sprintf(szBuffer, "%08X  %s",current_addr.addr32.offset, pcsIns);
		asmcode.push_back(std::string(szBuffer));
		i += insn->size;
		current_addr.addr32.offset += insn->size;
	}
}

bool x86Analysis::is_jump_table( x86dis_insn* insn )
{
	const char *opcode_str = insn->name;
	if (opcode_str[0] == '~')
	{
		opcode_str++;
	}
	if (opcode_str[0] == '|')
	{
		opcode_str++;
	}

	if (opcode_str[0]!='j' || opcode_str[1]!='m')
	{
		return false;
	}

	const x86_insn_op& op0 = insn->op[0];
	if (op0.type == X86_OPTYPE_MEM 
		&& op0.mem.hasdisp 
		&& op0.mem.index != X86_REG_NO 
		&& op0.mem.scale == 4)
	{
		return true;
	}
	return false;

}

