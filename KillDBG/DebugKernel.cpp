#include "stdafx.h"
#include "x86Analysis.h"
#include "DebugKernel.h"
#include "MainFrm.h"
#include "DebugUtils.h"
#include <string.h>

debug_kernel::debug_kernel(void)
	:debug_status_(STOP)
{
	continue_event_ = CreateEvent(NULL,FALSE,TRUE,NULL);
}


debug_kernel::~debug_kernel(void)
{
	for each (load_dll_info_t info in load_dll_info_)
	{
		CloseHandle(info.file_handle);
	}
}

bool debug_kernel::load_exe(std::string& exe_path, std::string& command_str,std::string& current_path)
{
	std::thread debug_thread([this,exe_path,command_str,current_path]()
	{
		STARTUPINFO si = {0};
		si.cb = sizeof(si);
		PROCESS_INFORMATION pi = {0};
		GetStartupInfo(&si);
		DebugSetProcessKillOnExit(TRUE);

		char command_copy[command_str.size()+1];
		strcpy(command_copy,command_str.c_str());
		if (!CreateProcess(exe_path.c_str(), command_copy, 
			NULL, NULL, FALSE, DEBUG_ONLY_THIS_PROCESS, NULL, current_path.c_str(), &si, &pi))
		{
			main_frame->m_wndOutput.output_string(std::string("创建调试进程失败！！"),COutputWnd::OUT_ERROR);
			return;
		}

		pid_ = pi.dwProcessId;
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);

		debug_thread_proc();
	});

	debug_thread.detach();

	return true;
}

bool debug_kernel::attach_process(DWORD pid)
{
	pid_ = pid;
	std::thread debug_thread([this]()
	{
		DebugSetProcessKillOnExit(FALSE);
		if (!DebugActiveProcess(pid_))
		{
			main_frame->m_wndOutput.output_string(std::string("附加指定进程失败！！"),COutputWnd::OUT_ERROR);
			return;
		}		
		debug_thread_proc();
	});

	debug_thread.detach();

	return true;
}

void debug_kernel::debug_thread_proc()
{
	debugee_exit_ = false;
	while (!debugee_exit_)
	{
		while (!WaitForDebugEvent(&debug_event_,0))
		{
			on_idle();
		}

		debug_status_ = BREAK;

		HANDLE thd_handle = OpenThread(THREAD_QUERY_INFORMATION | THREAD_GET_CONTEXT | THREAD_SET_CONTEXT,FALSE,debug_event_.dwThreadId);
		context_.ContextFlags = CONTEXT_ALL;
		GetThreadContext(thd_handle,&context_);
		CloseHandle(thd_handle);

		main_frame->m_wndCallStack.Invalidate(FALSE);

		main_frame->m_wndRegister.PostMessage(WM_USER_SETCONTEXT,(WPARAM)&context_);
		main_frame->m_wndStackView.SetAddrToView(context_.Esp);

		switch(debug_event_.dwDebugEventCode)
		{
		case CREATE_PROCESS_DEBUG_EVENT:	//创建被调试进程
			on_create_process_event(debug_event_.u.CreateProcessInfo);
			break;
		case EXIT_PROCESS_DEBUG_EVENT:
			on_exit_process_event(debug_event_.u.ExitProcess);
			//continue;
			break;
		case CREATE_THREAD_DEBUG_EVENT:
			on_create_thread_event(debug_event_.u.CreateThread);
			break;
		case EXIT_THREAD_DEBUG_EVENT:
			on_exit_thread_event(debug_event_.u.ExitThread);
			//continue;
			break;
		case LOAD_DLL_DEBUG_EVENT:		//加载DLL
			on_load_dll_event(debug_event_.u.LoadDll);
			break;
		case UNLOAD_DLL_DEBUG_EVENT:
			on_unload_dll_event(debug_event_.u.UnloadDll);
			break;
		case OUTPUT_DEBUG_STRING_EVENT:
			on_output_debug_string_event(debug_event_.u.DebugString);
			break;
		case RIP_EVENT:
			on_rip_event(debug_event_.u.RipInfo);
			break;
		case EXCEPTION_DEBUG_EVENT:
			on_exception_event(debug_event_.u.Exception);
			break;
		}
	}

	debug_status_ = STOP;
}

void debug_kernel::on_create_process_event( const CREATE_PROCESS_DEBUG_INFO& create_process_info )
{
	std::string program_path;
	debug_utils::get_file_name_frome_handle(create_process_info.hFile,program_path);
	boost::format fmter("加载程序：“%s”");
	fmter % program_path;
	main_frame->m_wndOutput.output_string( fmter.str());

	handle_ = create_process_info.hProcess;

	IMAGE_DOS_HEADER dos_header;
	DWORD base = (DWORD)create_process_info.lpBaseOfImage;
	read_memory(base,&dos_header,sizeof(IMAGE_DOS_HEADER));
	IMAGE_NT_HEADERS nt_header;
	read_memory(base + dos_header.e_lfanew,&nt_header,sizeof(IMAGE_NT_HEADERS));
	add_breakpoint( base + nt_header.OptionalHeader.AddressOfEntryPoint,true);
	main_frame->m_wndMemView.SetAddrToView(base + nt_header.OptionalHeader.AddressOfEntryPoint);
	CloseHandle(create_process_info.hThread);

// 	char buffer[512];
// 	sprintf( buffer,"srv*%s*http://msdl.microsoft.com/download/symbols", "E:\\123");
// 
// 	if( !SetEnvironmentVariable( _T( "_NT_SYMBOL_PATH" ), buffer ) )
// 	{
// 		return FALSE;
// 	}

	DWORD Options = SymGetOptions(); 

#ifdef DEBUG
	Options |= SYMOPT_DEBUG; 
#endif // _DEBUG

	SymSetOptions( Options ); 

	SymInitialize(handle_,sym_search_path_.c_str(),FALSE);

	load_dll_info_t info = {create_process_info.hFile,base,program_path};
	load_symbol(info);

	load_dll_info_.push_back(info);

	continue_debug();
}

void debug_kernel::on_exit_process_event( const EXIT_PROCESS_DEBUG_INFO& exit_process )
{
	boost::format fmter("被调试进程退出,退出代码为: %u");
	fmter % exit_process.dwExitCode;
	main_frame->m_wndOutput.output_string(fmter.str());
	debugee_exit_ = true;

	SymCleanup(handle_);
	main_frame->m_wndAsmView.Invalidate(FALSE);
	continue_debug();
}

void debug_kernel::on_create_thread_event( const CREATE_THREAD_DEBUG_INFO& create_thread )
{
	boost::format fmter("创建线程，起始地址为：0x%08X");
	fmter % create_thread.lpStartAddress;
	main_frame->m_wndOutput.output_string(fmter.str());

	continue_debug();
}

void debug_kernel::on_exit_thread_event( const EXIT_THREAD_DEBUG_INFO& exit_thread )
{
	boost::format fmter("线程退出，退出代码为：%u");
	fmter % exit_thread.dwExitCode;
	main_frame->m_wndOutput.output_string(fmter.str());

	continue_debug();
}

void debug_kernel::on_load_dll_event( const LOAD_DLL_DEBUG_INFO& load_dll )
{
	std::string dll_path;
	debug_utils::get_file_name_frome_handle(load_dll.hFile,dll_path);

	boost::format fmter("加载模块:\"%s\"");
	fmter % dll_path;
	main_frame->m_wndOutput.output_string(fmter.str());

	load_dll_info_t info = {load_dll.hFile,(DWORD)load_dll.lpBaseOfDll,dll_path};
	load_symbol(info);

	load_dll_info_.push_back(info);

	continue_debug();
}

void debug_kernel::on_unload_dll_event( const UNLOAD_DLL_DEBUG_INFO& unload_dll )
{
	boost::format fmter("卸载模块:\"0x%08X\"");
	fmter % unload_dll.lpBaseOfDll;
	main_frame->m_wndOutput.output_string(fmter.str());

	SymUnloadModule64(handle_,(DWORD)unload_dll.lpBaseOfDll);
	continue_debug();
}

void debug_kernel::on_output_debug_string_event( const OUTPUT_DEBUG_STRING_INFO& debug_string )
{
	int str_len = debug_string.nDebugStringLength;		//字符串的长度（字节）
	BYTE read_buffer[str_len];

	char output_str[str_len+50];

	SIZE_T bytesRead;

	if (!read_memory((DWORD)debug_string.lpDebugStringData,read_buffer,str_len))
	{
		main_frame->m_wndOutput.output_string(std::string("获取调试字符串失败"),COutputWnd::OUT_WARNING);
		return;
	}

	//据说这里永远是多字节字符，
	//这个if里面的东西永远也执行不到
	//但是为了防止操蛋的微软改了，还是这么来吧
	if (debug_string.fUnicode)
	{
		char ansi_str[str_len+1];
		WideCharToMultiByte(CP_ACP,NULL,(WCHAR*)read_buffer,-1,ansi_str,str_len,NULL,FALSE);

		sprintf(output_str,"调试字符串：“%s”\n",ansi_str);
	}
	else
	{
		sprintf(output_str,"调试字符串：“%s”\n",read_buffer);
	}
	main_frame->m_wndOutput.output_string(std::string(output_str));

	continue_debug();
}

void debug_kernel::on_rip_event( const RIP_INFO& rip_info )
{
	continue_debug();
}

void debug_kernel::on_exception_event( const EXCEPTION_DEBUG_INFO& debug_exception )
{
	DWORD addr = (DWORD)debug_exception.ExceptionRecord.ExceptionAddress;
	main_frame->m_wndAsmView.SetEIP(addr);

	char out_str[100];
	const char* fmt = "异常：%s,地址：0x%08X,第 %d 次处理机会。";

	int chance = debug_exception.dwFirstChance?1:2;

	switch (debug_exception.ExceptionRecord.ExceptionCode)
	{
	case EXCEPTION_ACCESS_VIOLATION:
		{
			sprintf(out_str,fmt,"EXCEPTION_ACCESS_VIOLATION",addr,chance);
			main_frame->m_wndOutput.output_string(std::string(out_str));
		}
		break;
	case EXCEPTION_DATATYPE_MISALIGNMENT:
		{
			sprintf(out_str,fmt,"EXCEPTION_DATATYPE_MISALIGNMENT",addr,chance);
			main_frame->m_wndOutput.output_string(std::string(out_str));
		}
		break;
	case EXCEPTION_BREAKPOINT:
		{
			sprintf(out_str,fmt,"EXCEPTION_BREAKPOINT",addr,chance);
			main_frame->m_wndOutput.output_string(std::string(out_str));

			breakpoint_t* bp = find_breakpoint_by_address(addr);
			if (bp)	// 调试器设置的断点
			{
				if (bp->is_once)
				{
					delete_breakpoint(bp->address);
				}
				else
				{
					bp->hits++;
					main_frame->m_wndBpList.Refresh();
				}

				HANDLE thd_handle = OpenThread(THREAD_QUERY_INFORMATION | THREAD_GET_CONTEXT | THREAD_SET_CONTEXT,FALSE,debug_event_.dwThreadId);
				context_.Eip -= 1;
				SetThreadContext(thd_handle,&context_);
				CloseHandle(thd_handle);			
			}
			else
			{
				// 系统断点或程序自己写的断点
				main_frame->m_wndAsmView.SetEIP(addr+1);
			}

			main_frame->m_wndAsmView.Invalidate(FALSE);
			
		}

		break;

	case EXCEPTION_SINGLE_STEP:
		{
			void* addr = debug_exception.ExceptionRecord.ExceptionAddress;
			main_frame->m_wndAsmView.SetEIP((DWORD)addr);
			main_frame->m_wndAsmView.Invalidate(FALSE);

		}
		break;
	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
		{
			sprintf(out_str,fmt,"EXCEPTION_ARRAY_BOUNDS_EXCEEDED",addr,chance);
			main_frame->m_wndOutput.output_string(std::string(out_str));
		}
		break;
	case EXCEPTION_FLT_DENORMAL_OPERAND:
		{
			sprintf(out_str,fmt,"EXCEPTION_FLT_DENORMAL_OPERAND",addr,chance);
			main_frame->m_wndOutput.output_string(std::string(out_str));
		}
		break;
	case EXCEPTION_FLT_DIVIDE_BY_ZERO:
		{
			sprintf(out_str,fmt,"EXCEPTION_FLT_DIVIDE_BY_ZERO",addr,chance);
			main_frame->m_wndOutput.output_string(std::string(out_str));
		}
		break;
	case EXCEPTION_FLT_INEXACT_RESULT:
		{
			sprintf(out_str,fmt,"EXCEPTION_FLT_INEXACT_RESULT",addr,chance);
			main_frame->m_wndOutput.output_string(std::string(out_str));
		}
		break;
	case EXCEPTION_FLT_INVALID_OPERATION:
		{
			sprintf(out_str,fmt,"EXCEPTION_FLT_INVALID_OPERATION",addr,chance);
			main_frame->m_wndOutput.output_string(std::string(out_str));
		}
		break;
	case EXCEPTION_FLT_OVERFLOW:
		{
			sprintf(out_str,fmt,"EXCEPTION_FLT_OVERFLOW",addr,chance);
			main_frame->m_wndOutput.output_string(std::string(out_str));
		}
		break;
	case EXCEPTION_FLT_STACK_CHECK:
		{
			sprintf(out_str,fmt,"EXCEPTION_FLT_STACK_CHECK",addr,chance);
			main_frame->m_wndOutput.output_string(std::string(out_str));
		}
		break;
	case EXCEPTION_FLT_UNDERFLOW:
		{
			sprintf(out_str,fmt,"EXCEPTION_FLT_UNDERFLOW",addr,chance);
			main_frame->m_wndOutput.output_string(std::string(out_str));
		}
		break;
	case EXCEPTION_INT_DIVIDE_BY_ZERO:
		{
			sprintf(out_str,fmt,"EXCEPTION_INT_DIVIDE_BY_ZERO",addr,chance);
			main_frame->m_wndOutput.output_string(std::string(out_str));
		}
		break;
	case EXCEPTION_INT_OVERFLOW:
		{
			sprintf(out_str,fmt,"EXCEPTION_INT_OVERFLOW",addr,chance);
			main_frame->m_wndOutput.output_string(std::string(out_str));
		}
		break;
	case EXCEPTION_PRIV_INSTRUCTION:
		{
			sprintf(out_str,fmt,"EXCEPTION_PRIV_INSTRUCTION",addr,chance);
			main_frame->m_wndOutput.output_string(std::string(out_str));
		}
		break;
	case EXCEPTION_IN_PAGE_ERROR:
		{
			sprintf(out_str,fmt,"EXCEPTION_IN_PAGE_ERROR",addr,chance);
			main_frame->m_wndOutput.output_string(std::string(out_str));
		}
		break;
	case EXCEPTION_ILLEGAL_INSTRUCTION:
		{
			sprintf(out_str,fmt,"EXCEPTION_ILLEGAL_INSTRUCTION",addr,chance);
			main_frame->m_wndOutput.output_string(std::string(out_str));
		}
		break;
	case EXCEPTION_NONCONTINUABLE_EXCEPTION:
		{
			sprintf(out_str,fmt,"EXCEPTION_NONCONTINUABLE_EXCEPTION",addr,chance);
			main_frame->m_wndOutput.output_string(std::string(out_str));
		}
		break;
	case EXCEPTION_STACK_OVERFLOW:
		{
			sprintf(out_str,fmt,"EXCEPTION_STACK_OVERFLOW",addr,chance);
			main_frame->m_wndOutput.output_string(std::string(out_str));
		}
		break;
	case EXCEPTION_INVALID_DISPOSITION:
		{
			sprintf(out_str,fmt,"EXCEPTION_INVALID_DISPOSITION",addr,chance);
			main_frame->m_wndOutput.output_string(std::string(out_str));
		}
		break;
	case EXCEPTION_GUARD_PAGE:
		{
			sprintf(out_str,fmt,"EXCEPTION_GUARD_PAGE",addr,chance);
			main_frame->m_wndOutput.output_string(std::string(out_str));
		}
		break;
	case EXCEPTION_INVALID_HANDLE:
		{
			sprintf(out_str,fmt,"EXCEPTION_INVALID_HANDLE",addr,chance);
			main_frame->m_wndOutput.output_string(std::string(out_str));
		}
		break;
// 	case EXCEPTION_POSSIBLE_DEADLOCK:
// 		break;
	case CONTROL_C_EXIT:
		{
			sprintf(out_str,fmt,"CONTROL_C_EXIT",addr,chance);
			main_frame->m_wndOutput.output_string(std::string(out_str));
		}
		break;
	default:
		{
			main_frame->m_wndOutput.output_string(std::string("未知异常"));
		}
		break;
	}

}

void debug_kernel::refresh_memory_map( void )
{
	memory_info_vector_.clear();

	//获取进程中所有模块和模块信息
	HANDLE hsnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,pid_);
	debug_utils::scope_exit  close_hsnap([&hsnap](){CloseHandle(hsnap);hsnap = NULL;});
	if (hsnap == INVALID_HANDLE_VALUE)
	{
		return;
	}

	MODULEENTRY32	module_entry = {0};
	module_entry.dwSize = sizeof(MODULEENTRY32);
	if (!Module32First(hsnap,&module_entry))
	{
		return;
	}

	module_vector_.clear();
	do 
	{
		module_info_t module_info = {0};
		module_info.module_base_addr = module_entry.modBaseAddr;
		module_info.module_base_size = module_entry.modBaseSize;
		strcpy(module_info.module_name,module_entry.szModule);

		//读取dos头
		IMAGE_DOS_HEADER dos_header;
		if (!read_memory((DWORD)module_entry.modBaseAddr,&dos_header,sizeof(IMAGE_DOS_HEADER))
			|| dos_header.e_magic != IMAGE_DOS_SIGNATURE)
		{
			return;
		}

		//读取PE文件头
		IMAGE_NT_HEADERS nt_header;
		if (!read_memory((DWORD)module_entry.modBaseAddr+dos_header.e_lfanew,&nt_header,sizeof(IMAGE_NT_HEADERS)) 
			|| nt_header.Signature != IMAGE_NT_SIGNATURE)
		{
			return;
		}
		//判断区段数量是否正确
		if (nt_header.FileHeader.NumberOfSections<0
			|| nt_header.FileHeader.NumberOfSections>96)
		{
			return;
		}
		module_info.section_nums = nt_header.FileHeader.NumberOfSections;

		//模块入口点
		module_info.entry_vector.push_back((BYTE*)(nt_header.OptionalHeader.ImageBase + nt_header.OptionalHeader.AddressOfEntryPoint));

		//从模块开始到区段表结束之前的大小
		module_info.pe_header_addr = dos_header.e_lfanew 
			+ sizeof(IMAGE_NT_HEADERS) 
			+ sizeof(IMAGE_SECTION_HEADER)*module_info.section_nums;

		//读取所有的区段头
		IMAGE_SECTION_HEADER sectionHeaders[nt_header.FileHeader.NumberOfSections];
		if (!read_memory((DWORD)module_entry.modBaseAddr+dos_header.e_lfanew+sizeof(IMAGE_NT_HEADERS),sectionHeaders,sizeof(IMAGE_SECTION_HEADER)*nt_header.FileHeader.NumberOfSections))
		{
			return;
		}

		for (int i=0;i<module_info.section_nums;++i)
		{
			strncpy(module_info.section_info[i].name,(const char*)sectionHeaders[i].Name,IMAGE_SIZEOF_SHORT_NAME);
			module_info.section_info[i].start_addr = module_entry.modBaseAddr+sectionHeaders[i].VirtualAddress;
			//ModuleInfo.stSections[i].nSize = sectionHeaders[i].SizeOfRawData;
			module_info.section_info[i].size = sectionHeaders[i].Misc.VirtualSize;
		}

		module_vector_.push_back(module_info);
	} while (Module32Next(hsnap,&module_entry));


	PBYTE	Address = NULL;
	MEMORY_BASIC_INFORMATION	info = {0};
	while (VirtualQueryEx(handle_,Address,&info,sizeof(info)) == sizeof(info))
	{
		//AtlTrace("%08X\n",Address);
		memory_region_info_t	MemInfo = {0};
		MemInfo.start_addr = Address;	//起始地址
		MemInfo.protect = info.Protect;	//访问属性
		MemInfo.alloc_protect = info.AllocationProtect;		//初始访问属性
		MemInfo.type = info.Type;		//类型
		MemInfo.size = info.RegionSize - (Address - (BYTE*)info.BaseAddress);

		byte*	pRgnStart = MemInfo.start_addr;
		byte*	pRgnEnd = pRgnStart + MemInfo.size;

		//查找该段内存属于哪个模块
		for (std::vector<module_info_t>::iterator it=module_vector_.begin();
			it!=module_vector_.end();++it)
		{
			module_info_t& info = *it;
			//查找该段内存属于哪个模块
			if (pRgnStart<info.module_base_addr || pRgnStart>=(info.module_base_addr+info.module_base_size))
			{
				continue;
			}

			MemInfo.owner_name = info.module_name;

			AtlTrace("RgnDtart:%08X\n",pRgnStart);

			if (pRgnStart == info.module_base_addr)
			{
				MemInfo.section_name = "PE头";
			}

			//查找该段内存属于哪个区段
			for (int i=0;i<info.section_nums;++i)
			{
				section_info_t& SecInfo = info.section_info[i];
				byte*	pSecStart = SecInfo.start_addr;
				byte*	pSecEnd = pSecStart + SecInfo.size;
				AtlTrace("SecStart:%08X,SecEnd:%08X\n",pSecStart,pSecEnd);

				if (pRgnStart == pSecStart)
				{
					MemInfo.section_name = SecInfo.name;
				}
				else if (pSecStart>pRgnStart && pSecStart<pRgnEnd)
				{
					//MemInfo.strSectionName = "";
					MemInfo.size = pSecStart - pRgnStart;
					break;
				}
			}

			for each (BYTE* it in info.entry_vector)
			{
				if (it>=MemInfo.start_addr && it<(MemInfo.start_addr + MemInfo.size))
				{
					MemInfo.entry_vector.push_back(it);
				}
			}

			break;
		}
		memory_info_vector_.push_back(MemInfo);
		Address += MemInfo.size;
		AtlTrace("%X\n",MemInfo.size);
	}

}

bool debug_kernel::step_in()
{
	if (debug_status_ != BREAK)
	{
		return false;
	}

	HANDLE thd_handle = OpenThread(THREAD_QUERY_INFORMATION | THREAD_GET_CONTEXT | THREAD_SET_CONTEXT,FALSE,debug_event_.dwThreadId);
	if (!thd_handle)
	{
		return false;
	}
	
	CONTEXT	context;
	context.ContextFlags = CONTEXT_CONTROL;
	if (!GetThreadContext(thd_handle,&context))
	{
		CloseHandle(thd_handle);
		return false;
	}
	
	context.EFlags  |= (1 << 8);
	
	if (!SetThreadContext(thd_handle,&context))
	{
		CloseHandle(thd_handle);
		return false;
	}
	CloseHandle(thd_handle);
	
	continue_debug();
	return true;
}

bool debug_kernel::add_breakpoint( DWORD address,bool is_once /*= false*/ )
{
	breakpoint_t bp = {0};
	bp.address = address;
	bp.user_enable = true;
	bp.valid = true;
	if (!read_memory(address,&bp.org_data,1))
	{
		return false;
	}

	byte bp_code = 0xCC;
	if (!write_memory(bp.address,&bp_code,1))
	{
		return false;
	}

	bp.is_once = is_once;
	bp_vec_.push_back(bp);

	main_frame->m_wndBpList.Refresh();

	return true;
}

bool debug_kernel::read_memory( DWORD address,void* buffer,size_t size,SIZE_T* num_read /*= NULL */ )
{
	if ((DWORD)address>=0x80000000 || (DWORD)address<0xff)
	{
		return false;
	}

	BOOL ret = ReadProcessMemory(handle_,(LPVOID)address,buffer,size,num_read);
	if (!ret)
	{
		DWORD old_protect = 0;
		if (!modify_memory_prop(address,size,PAGE_EXECUTE_READ,&old_protect))
		{
			return false;
		}
		ret = ReadProcessMemory(handle_,(LPVOID)address,buffer,size,num_read);
		modify_memory_prop(address,size,old_protect);
	}

	if (!ret)
	{
		return false;
	}

	for each (breakpoint_t bp in bp_vec_)
	{
		if (bp.address >= address && bp.address <= address+size)
		{
			((byte*)buffer)[bp.address-address] = bp.org_data;
		}
	}


	// 	std::string message;
	// 	debug_utils::get_error_msg(GetLastError(),message);
	// 	
	// 	boost::format fmt("读取被调试进程内存失败，地址：0x%08X,大小：%u错误信息：%s");
	// 	fmt % address % size % message;
	// 
	// 	output_string(fmt.str(),OUT_ERROR);
	return true;
}

bool debug_kernel::write_memory( DWORD address,void* data,SIZE_T size, SIZE_T* num_written /*= NULL*/ )
{
	return WriteProcessMemory(handle_,(LPVOID)address,data,size,num_written);
}

bool debug_kernel::modify_memory_prop( DWORD address,SIZE_T size,DWORD new_protect,DWORD* old_protect /*= NULL*/ )
{
	return VirtualProtectEx(handle_,(LPVOID)address,size,new_protect,old_protect);
}

bool debug_kernel::virtual_query_ex(DWORD address,MEMORY_BASIC_INFORMATION& info)
{
	return VirtualQueryEx(handle_,(LPCVOID)address,&info,sizeof(MEMORY_BASIC_INFORMATION)) == sizeof(MEMORY_BASIC_INFORMATION);
}

debug_kernel::breakpoint_t* debug_kernel::find_breakpoint_by_address( DWORD address )
{
	for (int i=0;i<bp_vec_.size();++i)
	{
		if (bp_vec_[i].address == address)
		{
			return &bp_vec_[i];
		}
	}

	return NULL;
}

bool debug_kernel::enable_breakpoint( breakpoint_t* bp )
{
	if (!bp)
	{
		return false;
	}

	if (!valid_breakpoint(bp))
	{
		return false;
	}

	bp->user_enable = true;
	main_frame->m_wndBpList.Refresh();
	return true;
}

bool debug_kernel::enable_breakpoint( DWORD address )
{
	return enable_breakpoint(find_breakpoint_by_address(address));
}

bool debug_kernel::disable_breakpoint( breakpoint_t* bp )
{
	if (!bp)
	{
		return false;
	}

	if (!invalid_breakpoint(bp))
	{
		return false;
	}

	bp->user_enable = false;
	main_frame->m_wndBpList.Refresh();
	return true;
}

bool debug_kernel::disable_breakpoint( DWORD address )
{
	return disable_breakpoint(find_breakpoint_by_address(address));
}

bool debug_kernel::valid_breakpoint( breakpoint_t* bp )
{
	if (!bp)
	{
		return false;
	}

	byte bp_code = 0xCC;
	if (write_memory(bp->address,&bp_code,1))
	{
		bp->valid = true;
		return true;
	}
	return false;
}

bool debug_kernel::valid_breakpoint( DWORD address )
{
	return valid_breakpoint(find_breakpoint_by_address(address));
}

bool debug_kernel::invalid_breakpoint( breakpoint_t* bp )
{
	if (!bp)
	{
		return false;
	}

	if (write_memory(bp->address,&bp->org_data,1))
	{
		bp->valid = false;
		return true;
	}
	return false;
}

bool debug_kernel::invalid_breakpoint( DWORD address )
{
	return invalid_breakpoint(find_breakpoint_by_address(address));
}

bool debug_kernel::delete_breakpoint( DWORD address )
{
	for (auto it = bp_vec_.begin();it!=bp_vec_.end();++it)
	{
		if (it->address == address)
		{
			disable_breakpoint(address);
			bp_vec_.erase(it);
			main_frame->m_wndBpList.Refresh();
			return true;
		}
	}

	return false;
}

bool debug_kernel::step_over()
{
	x86dis decoder(X86_OPSIZE32,X86_ADDRSIZE32);
	byte buffer[15];
	SIZE_T num_read = 0;
	if (!read_memory(context_.Eip,buffer,15,&num_read))
	{
		return false;
	}

	CPU_ADDR cur_addr = {0};
	cur_addr.addr32.offset = context_.Eip;
	x86dis_insn* insn = (x86dis_insn*)decoder.decode(buffer,num_read,cur_addr);

	const char *opcode_str = insn->name;
	if (opcode_str[0] == '~')
	{
		opcode_str++;
	}
	if (opcode_str[0] == '|')
	{
		opcode_str++;
	}

	if ((opcode_str[0]=='c') && (opcode_str[1]=='a'))
	{
		if (!add_breakpoint(context_.Eip + insn->size,true))
		{
			return false;
		}
		
		return continue_debug();
	}
	return step_in();
}

bool debug_kernel::symbol_from_addr( DWORD addr,std::string& symbol,bool allow_in_func )
{
	char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
	PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;

	pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
	pSymbol->MaxNameLen = MAX_SYM_NAME;
	bool ret = SymFromAddr(handle_,addr,NULL,pSymbol) == TRUE;// && ;
	if (!ret)
	{
		return false;
	}

	symbol = pSymbol->Name;

	if (pSymbol->Address == addr)
	{
		return true;
	}

	if (allow_in_func)
	{
		char offset[10];
		sprintf(offset,"+%X",addr-(DWORD)pSymbol->Address);
		symbol += offset;
		return true;
	}

	return false;
}

bool debug_kernel::symbol_from_addr( DWORD addr,PSYMBOL_INFO symbol_info)
{
	return SymFromAddr(handle_,addr,NULL,symbol_info) == TRUE;
}

void debug_kernel::set_sym_search_path(const char* paths, bool reload)
{
	sym_search_path_ = paths;

	if (reload)
	{
		SymCleanup(handle_);
		SymInitialize(handle_,sym_search_path_.c_str(),FALSE);
		
		for each (load_dll_info_t info in load_dll_info_)
		{
			load_symbol(info);
		}
	}

	SymSetSearchPath(handle_,paths);
}

bool debug_kernel::load_symbol( const load_dll_info_t& info )
{
	SymLoadModuleEx(handle_,info.file_handle,info.dll_path.c_str(),NULL,info.base_of_dll,0,NULL,NULL);
	//SymLoadModule64(handle_,NULL,info.dll_path.c_str(),NULL,NULL,NULL);

	IMAGEHLP_MODULE64 module64 = {sizeof(module64)};
	SymGetModuleInfo64(handle_,info.base_of_dll,&module64);
	if (module64.SymType == SymNone)
	{
		main_frame->m_wndOutput.output_string(str(boost::format("为模块 %s 加载调试信息失败") % info.dll_path));
		return false;
	}

	std::string sym_type;
	switch (module64.SymType)
	{
	case SymCoff:
		sym_type = "COFF 类型";
		break;
	case SymCv:
		sym_type = "CodeView 类型";
		break;
	case SymPdb:
		sym_type = "PDB 类型";
		break;
	case SymSym:
		sym_type = ".sym文件 类型";
		break;
	case SymDia:
		sym_type = "DIA 类型";
		break;
	case SymExport:
		sym_type = "由Dll导出表生成";
		break;
	default:
		sym_type = "其他类型";
		break;
	}
	main_frame->m_wndOutput.output_string(str(boost::format("为模块 %s 加载调试信息成功,调试信息类型为 %s。") % info.dll_path % sym_type));

	return true;
}

bool debug_kernel::stack_walk( STACKFRAME64& stack_frame, CONTEXT& context )
{
	HANDLE thread = OpenThread(THREAD_ALL_ACCESS,FALSE,debug_event_.dwThreadId);
	if (!thread)
	{
		return false;
	}
	bool ret = StackWalk64(IMAGE_FILE_MACHINE_I386,handle_,thread,&stack_frame,&context,NULL,SymFunctionTableAccess64,SymGetModuleBase64,NULL) == TRUE;

	CloseHandle(thread);

	return ret;
}

void debug_kernel::update_breakpoint_starus( bool disable_current_bp )
{
	for (int i=0;i<bp_vec_.size();++i)
	{
		breakpoint_t& bp = bp_vec_[i];
		if (disable_current_bp && bp.address == context_.Eip)
		{
			invalid_breakpoint(&bp);
			continue;
		}

		if (bp.user_enable == true && bp.valid == false)
		{

			valid_breakpoint(&bp);
		}
	}
}

bool debug_kernel::stop_debug()
{
	return TerminateProcess(handle_,0) == TRUE;
}

bool debug_kernel::get_memory_info_by_addr( const void* addr,memory_region_info_t& info )
{
	refresh_memory_map();
	for each (memory_region_info_t tmp in memory_info_vector_)
	{
		if ((unsigned long)(tmp.start_addr) <= (unsigned long)addr && (unsigned long)addr < (unsigned long)(tmp.start_addr + tmp.size))
		{
			info = tmp;
			return true;
		}
	}

	return false;
}

bool debug_kernel::continue_debug( DWORD continue_status /*= DBG_CONTINUE*/,bool disable_current_bp /*= false*/ )
{
	update_breakpoint_starus(disable_current_bp);

	return ContinueDebugEvent(debug_event_.dwProcessId,debug_event_.dwThreadId,continue_status) == TRUE;
}

void debug_kernel::on_idle()
{
	while (ui_event_.size() != 0)
	{
		ui_event_t& event = ui_event_.front();

		switch (event.type)
		{
		case ID_STEP_IN:
			{
				step_in();
			}
			break;
		case ID_STEP_OVER:
			{
				step_over();
			}
			break;
		case ID_RUN:
			{
				continue_debug(DBG_CONTINUE,true);
			}
			break;
		case ID_STEPIN_UNHANDLE_EXCEPT:
			{
				step_in();
			}
			break;
		case ID_STEPOVER_UNHANDLE_EXCEPT:
			{
				step_over();
			}
			break;
		case ID_RUN_UNHANDLE_EXCEPT:
			{
				continue_debug(DBG_EXCEPTION_NOT_HANDLED,true);
			}
			break;
		case ID_SET_BREAKPOINT:
			{
				add_breakpoint(event.param1);
			}
			break;
		case ID_RUN_TO_CURSOR:
			{
				breakpoint_t* bp = find_breakpoint_by_address(event.param1);
				if (!bp && !add_breakpoint(event.param1,true))
				{
					main_frame->m_wndOutput.output_string(str(boost::format("在地址 %08X 处设置断点失败！") % event.param1),COutputWnd::OUT_ERROR);
					break;
				}

				continue_debug(DBG_CONTINUE);
			}
			break;
		case ID_RUN_OUT:
			{
				CONTEXT context = get_current_context();
				STACKFRAME64 frame = {0};
				frame.AddrPC.Mode = AddrModeFlat;
				frame.AddrPC.Offset = context.Eip;
				frame.AddrStack.Mode = AddrModeFlat;
				frame.AddrStack.Offset = context.Esp;
				frame.AddrFrame.Mode = AddrModeFlat;
				frame.AddrFrame.Offset = context.Ebp;

				if (!stack_walk(frame,context))
				{
					main_frame->m_wndOutput.output_string(std::string("查找函数返回地址失败"),COutputWnd::OUT_ERROR);
					break;
				}

				DWORD ret_addr = (DWORD)frame.AddrReturn.Offset;
				breakpoint_t* bp = find_breakpoint_by_address(ret_addr);
				if (!bp && !add_breakpoint(ret_addr,true))
				{
					main_frame->m_wndOutput.output_string(str(boost::format("在函数返回地址 %08X 处设置断点失败！") % ret_addr),COutputWnd::OUT_ERROR);
					break;
				}

				continue_debug(DBG_CONTINUE);
			}
			break;
		case ID_STOP_DEBUG:
			{
				if (!stop_debug())
				{
					main_frame->m_wndOutput.output_string(std::string("停止调试失败"));
				}
				continue_debug();
			}
			break;
		case ID_BREAK_PROCESS:
			{
				if (get_debug_status() != debug_kernel::RUN)
				{
					main_frame->m_wndOutput.output_string(std::string("进程已经是中断或停止状态，不需要执行此功能。"),COutputWnd::OUT_ERROR);
					break;
				}

				if (!break_process())
				{
					main_frame->m_wndOutput.output_string(std::string("中断被调试进程失败。"),COutputWnd::OUT_ERROR);
				}
			}
			break;
// 		case :
// 			break;
		}

		ui_event_.pop_front();
	}
}
