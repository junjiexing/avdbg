#include "stdafx.h"
#include "x86Analysis.h"
#include "DebugKernel.h"
#include "MainFrm.h"
#include "DebugUtils.h"

debug_kernel::debug_kernel(void)
	:continue_status_(DBG_CONTINUE),debug_status_(STOP)
{
	continue_event_ = CreateEvent(NULL,FALSE,TRUE,NULL);
}


debug_kernel::~debug_kernel(void)
{
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
			main_frame->m_wndOutputWnd.output_string(std::string("创建调试进程失败！！"),COutputWindow::OUT_ERROR);
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
			main_frame->m_wndOutputWnd.output_string(std::string("附加指定进程失败！！"),COutputWindow::OUT_ERROR);
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
		BOOL ret = WaitForDebugEvent(&debug_event_,INFINITE);

		HANDLE thd_handle = OpenThread(THREAD_QUERY_INFORMATION | THREAD_GET_CONTEXT | THREAD_SET_CONTEXT,FALSE,debug_event_.dwThreadId);
		context_.ContextFlags = CONTEXT_ALL;
		GetThreadContext(thd_handle,&context_);
		CloseHandle(thd_handle);

		bool continue_debug = true;
		switch(debug_event_.dwDebugEventCode)
		{
		case CREATE_PROCESS_DEBUG_EVENT:	//创建被调试进程
			continue_debug = on_create_process_event(debug_event_.u.CreateProcessInfo);
			break;
		case EXIT_PROCESS_DEBUG_EVENT:
			continue_debug = on_exit_process_event(debug_event_.u.ExitProcess);
			continue;
			break;
		case CREATE_THREAD_DEBUG_EVENT:
			continue_debug = on_create_thread_event(debug_event_.u.CreateThread);
			break;
		case EXIT_THREAD_DEBUG_EVENT:
			continue_debug = on_exit_thread_event(debug_event_.u.ExitThread);
			//continue;
			break;
		case LOAD_DLL_DEBUG_EVENT:		//加载DLL
			continue_debug = on_load_dll_event(debug_event_.u.LoadDll);
			break;
		case UNLOAD_DLL_DEBUG_EVENT:
			continue_debug = on_unload_dll_event(debug_event_.u.UnloadDll);
			break;
		case OUTPUT_DEBUG_STRING_EVENT:
			continue_debug = on_output_debug_string_event(debug_event_.u.DebugString);
			break;
		case RIP_EVENT:
			continue_debug = on_rip_event(debug_event_.u.RipInfo);
			break;
		case EXCEPTION_DEBUG_EVENT:
			continue_debug = on_exception_event(debug_event_.u.Exception);
			break;
		}


		update_breakpoint_starus();

		if (!continue_debug)
		{
			debug_status_ = BREAK;

			ResetEvent(continue_event_);
			WaitForSingleObject(continue_event_,INFINITE);
		}
		ContinueDebugEvent(debug_event_.dwProcessId,debug_event_.dwThreadId,continue_status_);
		
		debug_status_ = RUN;
	}

}

bool debug_kernel::on_create_process_event( const CREATE_PROCESS_DEBUG_INFO& create_process_info )
{
	std::string program_path;
	debug_utils::get_file_name_frome_handle(create_process_info.hFile,program_path);
	boost::format fmter("加载程序：“%s”");
	fmter % program_path;
	main_frame->m_wndOutputWnd.output_string( fmter.str());

	CloseHandle(create_process_info.hFile);

	handle_ = create_process_info.hProcess;

	IMAGE_DOS_HEADER dos_header;
	DWORD base = (DWORD)create_process_info.lpBaseOfImage;
	read_memory(base,&dos_header,sizeof(IMAGE_DOS_HEADER));
	IMAGE_NT_HEADERS nt_header;
	read_memory(base + dos_header.e_lfanew,&nt_header,sizeof(IMAGE_NT_HEADERS));
	add_breakpoint( base + nt_header.OptionalHeader.AddressOfEntryPoint,true);
	CloseHandle(create_process_info.hThread);

	continue_status_= DBG_CONTINUE;
	return true;
}

bool debug_kernel::on_exit_process_event( const EXIT_PROCESS_DEBUG_INFO& exit_process )
{
	boost::format fmter("被调试进程退出,退出代码为: %u");
	fmter % exit_process.dwExitCode;
	main_frame->m_wndOutputWnd.output_string(fmter.str());
	debugee_exit_ = true;

	continue_status_= DBG_CONTINUE;
	return true;
}

bool debug_kernel::on_create_thread_event( const CREATE_THREAD_DEBUG_INFO& create_thread )
{
	boost::format fmter("创建线程，起始地址为：0x%08X");
	fmter % create_thread.lpStartAddress;
	main_frame->m_wndOutputWnd.output_string(fmter.str());

	continue_status_= DBG_CONTINUE;
	return true;
}

bool debug_kernel::on_exit_thread_event( const EXIT_THREAD_DEBUG_INFO& exit_thread )
{
	boost::format fmter("线程退出，退出代码为：%u");
	fmter % exit_thread.dwExitCode;
	main_frame->m_wndOutputWnd.output_string(fmter.str());

	continue_status_= DBG_CONTINUE;
	return true;
}

bool debug_kernel::on_load_dll_event( const LOAD_DLL_DEBUG_INFO& load_dll )
{
	std::string dll_path;
	debug_utils::get_file_name_frome_handle(load_dll.hFile,dll_path);

	boost::format fmter("加载模块:\"%s\"");
	fmter % dll_path;
	main_frame->m_wndOutputWnd.output_string(fmter.str());

	CloseHandle(load_dll.hFile);

	continue_status_= DBG_CONTINUE;
	return true;
}

bool debug_kernel::on_unload_dll_event( const UNLOAD_DLL_DEBUG_INFO& unload_dll )
{
	boost::format fmter("卸载模块:\"0x%08X\"");
	fmter % unload_dll.lpBaseOfDll;
	main_frame->m_wndOutputWnd.output_string(fmter.str());

	continue_status_= DBG_CONTINUE;
	return true;
}

bool debug_kernel::on_output_debug_string_event( const OUTPUT_DEBUG_STRING_INFO& debug_string )
{
	int str_len = debug_string.nDebugStringLength;		//字符串的长度（字节）
	BYTE read_buffer[str_len];

	char output_str[str_len+50];

	SIZE_T bytesRead;

	if (!read_memory((DWORD)debug_string.lpDebugStringData,read_buffer,str_len))
	{
		main_frame->m_wndOutputWnd.output_string(std::string("获取调试字符串失败"),COutputWindow::OUT_WARNING);
		return false;
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
	main_frame->m_wndOutputWnd.output_string(std::string(output_str));

	continue_status_= DBG_CONTINUE;
	return true;
}

bool debug_kernel::on_rip_event( const RIP_INFO& rip_info )
{
	continue_status_= DBG_CONTINUE;
	return true;
}

bool debug_kernel::on_exception_event( const EXCEPTION_DEBUG_INFO& debug_exception )
{

	switch (debug_exception.ExceptionRecord.ExceptionCode)
	{
	case EXCEPTION_ACCESS_VIOLATION:
		{
			main_frame->m_wndOutputWnd.output_string(std::string("EXCEPTION_ACCESS_VIOLATION"));
		}
		break;
	case EXCEPTION_DATATYPE_MISALIGNMENT:
		{
			main_frame->m_wndOutputWnd.output_string(std::string("EXCEPTION_DATATYPE_MISALIGNMENT"));
		}
		break;
	case EXCEPTION_BREAKPOINT:
		{
			void* addr = debug_exception.ExceptionRecord.ExceptionAddress;
			boost::format fmter("断点异常，地址：0x%08X,异常代码：0x%08X\n");
			fmter % addr % debug_exception.ExceptionRecord.ExceptionCode;
			main_frame->m_wndOutputWnd.output_string(fmter.str());

			breakpoint_t* bp = find_breakpoint_by_address((DWORD)addr);
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
				main_frame->m_wndAsmView.SetEIP((DWORD)addr);
				
			}
			else
			{
				// 系统断点或程序自己写的断点
				main_frame->m_wndAsmView.SetEIP((DWORD)addr+1);
			}

			main_frame->m_wndAsmView.Invalidate(FALSE);
			
			continue_status_= DBG_CONTINUE;
			// 		memory_region_info_t info;
			// 		bool ret = this->get_memory_info_by_addr(addr,info);
			// 		if (!ret)
			// 		{
			// 			output_string(std::string("异常地址无法访问。"),OUT_ERROR);
			// 			return;
			// 		}
			// 
			// // 		std::thread([this,info,addr]()
			// // 		{
			// 			std::vector<byte> buffer(info.size);
			// 			SIZE_T	nRead = 0;
			// 			read_debugee_memory(info.start_addr,buffer.data(),info.size);
			// 			//ASSERT(nRead == info.dwSize);
			// 
			// 			std::shared_ptr<x86Analysis> analy(new x86Analysis(buffer.data(),info.size,(unsigned long)(info.start_addr),shared_from_this())) ;
			// 			analy->add_entry((uint32)addr);
			// 			std::vector<std::string> asmcode;
			// 			analy->process(asmcode);
			// 			main_frm_ptr_->m_wndAsmView.SetAnalysiser(analy);
			// 		}).detach();
		}

		break;

	case EXCEPTION_SINGLE_STEP:
		{
			void* addr = debug_exception.ExceptionRecord.ExceptionAddress;
			main_frame->m_wndAsmView.SetEIP((DWORD)addr);
			main_frame->m_wndAsmView.Invalidate(FALSE);

			continue_status_= DBG_CONTINUE;
		}
		break;
	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
		{
			main_frame->m_wndOutputWnd.output_string(std::string("EXCEPTION_ARRAY_BOUNDS_EXCEEDED"));
		}
		break;
	case EXCEPTION_FLT_DENORMAL_OPERAND:
		{
			main_frame->m_wndOutputWnd.output_string(std::string("EXCEPTION_FLT_DENORMAL_OPERAND"));
		}
		break;
	case EXCEPTION_FLT_DIVIDE_BY_ZERO:
		{
			main_frame->m_wndOutputWnd.output_string(std::string("EXCEPTION_FLT_DIVIDE_BY_ZERO"));
		}
		break;
	case EXCEPTION_FLT_INEXACT_RESULT:
		{
			main_frame->m_wndOutputWnd.output_string(std::string("EXCEPTION_FLT_INEXACT_RESULT"));
		}
		break;
	case EXCEPTION_FLT_INVALID_OPERATION:
		{
			main_frame->m_wndOutputWnd.output_string(std::string("EXCEPTION_FLT_INVALID_OPERATION"));
		}
		break;
	case EXCEPTION_FLT_OVERFLOW:
		{
			main_frame->m_wndOutputWnd.output_string(std::string("EXCEPTION_FLT_OVERFLOW"));
		}
		break;
	case EXCEPTION_FLT_STACK_CHECK:
		{
			main_frame->m_wndOutputWnd.output_string(std::string("EXCEPTION_FLT_STACK_CHECK"));
		}
		break;
	case EXCEPTION_FLT_UNDERFLOW:
		{
			main_frame->m_wndOutputWnd.output_string(std::string("EXCEPTION_FLT_UNDERFLOW"));
		}
		break;
	case EXCEPTION_INT_DIVIDE_BY_ZERO:
		{
			main_frame->m_wndOutputWnd.output_string(std::string("EXCEPTION_INT_DIVIDE_BY_ZERO"));
		}
		break;
	case EXCEPTION_INT_OVERFLOW:
		{
			main_frame->m_wndOutputWnd.output_string(std::string("EXCEPTION_INT_OVERFLOW"));
		}
		break;
	case EXCEPTION_PRIV_INSTRUCTION:
		{
			main_frame->m_wndOutputWnd.output_string(std::string("EXCEPTION_PRIV_INSTRUCTION"));
		}
		break;
	case EXCEPTION_IN_PAGE_ERROR:
		{
			main_frame->m_wndOutputWnd.output_string(std::string("EXCEPTION_IN_PAGE_ERROR"));
		}
		break;
	case EXCEPTION_ILLEGAL_INSTRUCTION:
		{
			main_frame->m_wndOutputWnd.output_string(std::string("EXCEPTION_ILLEGAL_INSTRUCTION"));
		}
		break;
	case EXCEPTION_NONCONTINUABLE_EXCEPTION:
		{
			main_frame->m_wndOutputWnd.output_string(std::string("EXCEPTION_NONCONTINUABLE_EXCEPTION"));
		}
		break;
	case EXCEPTION_STACK_OVERFLOW:
		{
			main_frame->m_wndOutputWnd.output_string(std::string("EXCEPTION_STACK_OVERFLOW"));
		}
		break;
	case EXCEPTION_INVALID_DISPOSITION:
		{
			main_frame->m_wndOutputWnd.output_string(std::string("EXCEPTION_INVALID_DISPOSITION"));
		}
		break;
	case EXCEPTION_GUARD_PAGE:
		{
			main_frame->m_wndOutputWnd.output_string(std::string("EXCEPTION_GUARD_PAGE"));
		}
		break;
	case EXCEPTION_INVALID_HANDLE:
		{
			main_frame->m_wndOutputWnd.output_string(std::string("EXCEPTION_INVALID_HANDLE"));
		}
		break;
// 	case EXCEPTION_POSSIBLE_DEADLOCK:
// 		break;
	case CONTROL_C_EXIT:
		{
			main_frame->m_wndOutputWnd.output_string(std::string("CONTROL_C_EXIT"));
		}
		break;
	default:
		{
			main_frame->m_wndOutputWnd.output_string(std::string("未知异常"));
		}
		break;
	}
	return false;
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
	continue_debug(DBG_CONTINUE);
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
		return continue_debug(DBG_CONTINUE);
	}
	return step_in();
}
