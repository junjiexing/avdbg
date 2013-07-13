#include "stdafx.h"
#include "x86Analysis.h"
#include "DebugKernel.h"
#include "MainFrm.h"
#include "DebugUtils.h"

DebugKernel::DebugKernel(void)
	:continue_status_(DBG_CONTINUE),main_frm_ptr_(NULL)
{
}


DebugKernel::~DebugKernel(void)
{
}

bool DebugKernel::load_exe( const std::string& exe_path,const std::string& command_str,const std::string& current_path )
{
	std::thread debug_thread([this,exe_path,command_str,current_path]()
	{
		DebugSetProcessKillOnExit(TRUE);

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
			output_string(std::string("创建调试进程失败！！"),OUT_ERROR);
			return;
		}

		process_id_ = pi.dwProcessId;
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);

		debug_thread_proc();
	});

	debug_thread.detach();

	return true;
}

bool DebugKernel::attach_process( DWORD process_id )
{
	std::thread debug_thread([this,process_id]()
	{
		DebugSetProcessKillOnExit(FALSE);
		if (!DebugActiveProcess(process_id))
		{
			output_string(std::string("附加指定进程失败！！"),OUT_ERROR);
			return;
		}
		process_id_ = process_id;
		
		debug_thread_proc();
	});

	debug_thread.detach();

	return true;
}

void DebugKernel::debug_thread_proc()
{
	DEBUG_EVENT debug_event;
	debugee_exit_ = false;
	while (!debugee_exit_)
	{
		BOOL ret = WaitForDebugEvent(&debug_event,0);
		if (!ret)
		{
			//OnIdle();
			continue;
		}

		BOOL	bContinue = TRUE;
		switch(debug_event.dwDebugEventCode)
		{
		case CREATE_PROCESS_DEBUG_EVENT:	//创建被调试进程
			on_create_process_event(debug_event.u.CreateProcessInfo);
			break;
		case EXIT_PROCESS_DEBUG_EVENT:
			on_exit_process_event(debug_event.u.ExitProcess);
			continue;
			break;
		case CREATE_THREAD_DEBUG_EVENT:
			on_create_thread_event(debug_event.u.CreateThread);
			break;
		case EXIT_THREAD_DEBUG_EVENT:
			on_exit_thread_event(debug_event.u.ExitThread);
			//continue;
			break;
		case LOAD_DLL_DEBUG_EVENT:		//加载DLL
			on_load_dll_event(debug_event.u.LoadDll);
			break;
		case UNLOAD_DLL_DEBUG_EVENT:
			on_unload_dll_event(debug_event.u.UnloadDll);
			break;
		case OUTPUT_DEBUG_STRING_EVENT:
			on_output_debug_string_event(debug_event.u.DebugString);
			break;
		case RIP_EVENT:
			on_rip_event(debug_event.u.RipInfo);
			break;
		case EXCEPTION_DEBUG_EVENT:
			on_exception_event(debug_event.u.Exception);
			break;
		}
		BOOL bRet = ContinueDebugEvent(debug_event.dwProcessId, debug_event.dwThreadId, continue_status_);
		ASSERT(bRet);

	}

}

void DebugKernel::on_create_process_event( const CREATE_PROCESS_DEBUG_INFO& create_process_info )
{
	std::string program_path;
	debug_utils::get_file_name_frome_handle(create_process_info.hFile,program_path);
	boost::format fmter("加载程序：“%s”");
	fmter % program_path;
	output_string( fmter.str());

	CloseHandle(create_process_info.hFile);
	process_handle_ = create_process_info.hProcess;
	CloseHandle(create_process_info.hThread);
}

void DebugKernel::on_exit_process_event( const EXIT_PROCESS_DEBUG_INFO& exit_process )
{
	boost::format fmter("被调试进程退出,退出代码为: %u");
	fmter % exit_process.dwExitCode;
	output_string(fmter.str());
	debugee_exit_ = true;
}

void DebugKernel::on_create_thread_event( const CREATE_THREAD_DEBUG_INFO& create_thread )
{
	boost::format fmter("创建线程，起始地址为：0x%08X");
	fmter % create_thread.lpStartAddress;
	output_string(fmter.str());
}

void DebugKernel::on_exit_thread_event( const EXIT_THREAD_DEBUG_INFO& exit_thread )
{
	boost::format fmter("线程退出，退出代码为：%u");
	fmter % exit_thread.dwExitCode;
	output_string(fmter.str());
}

void DebugKernel::on_load_dll_event( const LOAD_DLL_DEBUG_INFO& load_dll )
{
	std::string dll_path;
	debug_utils::get_file_name_frome_handle(load_dll.hFile,dll_path);

	boost::format fmter("加载模块:\"%s\"");
	fmter % dll_path;
	output_string(fmter.str());

	CloseHandle(load_dll.hFile);
}

void DebugKernel::on_unload_dll_event( const UNLOAD_DLL_DEBUG_INFO& unload_dll )
{
	boost::format fmter("卸载模块:\"0x%08X\"");
	fmter % unload_dll.lpBaseOfDll;
	output_string(fmter.str());
}

void DebugKernel::on_output_debug_string_event( const OUTPUT_DEBUG_STRING_INFO& debug_string )
{
	int str_len = debug_string.nDebugStringLength;		//字符串的长度（字节）
	BYTE read_buffer[str_len];

	char output_str[str_len+50];

	SIZE_T bytesRead;

	if (!read_debugee_memory(debug_string.lpDebugStringData,read_buffer,str_len))
	{
		output_string(std::string("获取调试字符串失败"),OUT_WARNING);
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
	output_string(std::string(output_str));
}

void DebugKernel::on_rip_event( const RIP_INFO& rip_info )
{

}

void DebugKernel::on_exception_event( const EXCEPTION_DEBUG_INFO& debug_exception )
{
	if (debug_exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT)
	{
		const void* addr = debug_exception.ExceptionRecord.ExceptionAddress;
		boost::format fmter("断点异常，地址：0x%08X,异常代码：0x%08X\n");
		fmter % addr % debug_exception.ExceptionRecord.ExceptionCode;
		output_string(fmter.str());

		memory_region_info_t info;
		bool ret = this->get_memory_info_by_addr(addr,info);
		if (!ret)
		{
			output_string(std::string("异常地址无法访问。"),OUT_ERROR);
			return;
		}

// 		std::thread([this,info,addr]()
// 		{
			std::vector<byte> buffer(info.size);
			SIZE_T	nRead = 0;
			read_debugee_memory(info.start_addr,buffer.data(),info.size);
			//ASSERT(nRead == info.dwSize);

			std::shared_ptr<x86Analysis> analy(new x86Analysis(buffer.data(),info.size,(unsigned long)(info.start_addr),shared_from_this())) ;
			analy->add_entry((uint32)addr);
			std::vector<std::string> asmcode;
			analy->process(asmcode);
			main_frm_ptr_->m_wndAsmView.SetAnalysiser(analy);


// 		}).detach();
	}
}

bool DebugKernel::read_debugee_memory( const void* address,void* buffer,size_t size )
{
	if ((DWORD)address>=0x80000000 || (DWORD)address<0xff)
	{
		return false;
	}

	SIZE_T num_read = 0;
	if (ReadProcessMemory(process_handle_,address,buffer,size,&num_read)
		&& num_read == size)
	{
		return true;
	}


	std::string message;
	debug_utils::get_error_msg(GetLastError(),message);
	
	boost::format fmt("读取被调试进程内存失败，地址：0x%08X,大小：%u错误信息：%s");
	fmt % address % size % message;

	output_string(fmt.str(),OUT_ERROR);
	return false;
}

void DebugKernel::refresh_memory_map( void )
{
	memory_info_vector_.clear();

	//获取进程中所有模块和模块信息
	HANDLE hsnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,get_process_id());
	debug_utils::ScopeExit  close_hsnap([&hsnap](){CloseHandle(hsnap);hsnap = NULL;});
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

	std::vector<module_info_t>	module_vector;
	do 
	{
		module_info_t module_info = {0};
		module_info.module_base_addr = module_entry.modBaseAddr;
		module_info.module_base_size = module_entry.modBaseSize;
		strcpy(module_info.module_name,module_entry.szModule);

		//读取dos头
		IMAGE_DOS_HEADER dos_header;
		if (!read_debugee_memory(module_entry.modBaseAddr,&dos_header,sizeof(IMAGE_DOS_HEADER))
			|| dos_header.e_magic != IMAGE_DOS_SIGNATURE)
		{
			return;
		}

		//读取PE文件头
		IMAGE_NT_HEADERS nt_header;
		if (!read_debugee_memory(module_entry.modBaseAddr+dos_header.e_lfanew,&nt_header,sizeof(IMAGE_NT_HEADERS)) 
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
		IMAGE_SECTION_HEADER* sectionHeaders = new IMAGE_SECTION_HEADER[nt_header.FileHeader.NumberOfSections];
		if (!read_debugee_memory(module_entry.modBaseAddr+dos_header.e_lfanew+sizeof(IMAGE_NT_HEADERS),sectionHeaders,sizeof(IMAGE_SECTION_HEADER)*nt_header.FileHeader.NumberOfSections))
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
		delete[] sectionHeaders;
		module_vector.push_back(module_info);
	} while (Module32Next(hsnap,&module_entry));


	PBYTE	Address = NULL;
	MEMORY_BASIC_INFORMATION	info = {0};
	while (VirtualQueryEx(process_handle_,Address,&info,sizeof(info)) == sizeof(info))
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
		for (std::vector<module_info_t>::iterator it=module_vector.begin();
			it!=module_vector.end();++it)
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

void DebugKernel::output_string( const std::string& str,OutputType type /*= OUT_INFO*/ )
{
	// 		if (!output_message_)
	// 		{
	// 			return;
	// 		}
	// 		output_message_(str,type);
	if (!main_frm_ptr_)
	{
		return;
	}
	main_frm_ptr_->m_wndOutputWnd.AddLine(str);
}
