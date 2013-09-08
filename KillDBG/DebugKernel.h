#pragma once

class CMainFrame;

extern CMainFrame* main_frame;

class debug_kernel
{
public:
	debug_kernel(void);
	~debug_kernel(void);

public:
	enum debug_status
	{
		STOP,
		RUN,
		BREAK
	};

	struct  breakpoint_t
	{
		DWORD address;	//断点地址
		byte org_data;	//断点处的原始数据
		bool valid;	//断点实际上是否起作用
		bool user_enable;	//用户是否启用了该断点

		int hits;	// 命中次数
		bool is_once; // 是否是一次性的
	};

	// 加载一个exe并开始调试
	bool load_exe(std::string& exe_path, std::string& command_str,std::string& current_path);
	// 附加到指定的进程并开始调试
	bool attach_process(DWORD pid);

	// 单步步入
	bool step_in();
	// 单步步过
	bool step_over();

	// 继续调试，运行被调试进程
	bool continue_debug(void)
	{
		breakpoint_t* bp = find_breakpoint_by_address(context_.Eip);
		if (bp)
		{
			invalid_breakpoint(bp);
		}


		return SetEvent(continue_event_) == TRUE;
	}

	void set_continue_status(DWORD continue_status)
	{
		continue_status_ = continue_status;
	}
	DWORD get_continue_status(void)
	{
		return continue_status_;
	}

	bool add_breakpoint(DWORD address,bool is_once = false);


	bool read_memory(DWORD address,void* buffer,size_t size,SIZE_T* num_read = NULL );
	bool write_memory(DWORD address,void* data,SIZE_T size, SIZE_T* num_written = NULL);
	bool modify_memory_prop(DWORD address,SIZE_T size,DWORD new_protect,DWORD* old_protect = NULL);
	bool virtual_query_ex(DWORD address,MEMORY_BASIC_INFORMATION& info);
	breakpoint_t* find_breakpoint_by_address(DWORD address);
	// 用户启用断点
	bool enable_breakpoint(breakpoint_t* bp);
	bool enable_breakpoint(DWORD address);
	// 用户禁用断点
	bool disable_breakpoint(breakpoint_t* bp);
	bool disable_breakpoint(DWORD address);
	// 使断点有效
	bool valid_breakpoint(breakpoint_t* bp);
	bool valid_breakpoint(DWORD address);
	// 使断点无效（为了进行单步等操作时不受断点的影响，但并不是禁用断点）
	bool invalid_breakpoint(breakpoint_t* bp);
	bool invalid_breakpoint(DWORD address);
	bool delete_breakpoint(DWORD address);
	void update_breakpoint_starus()
	{
		for (int i=0;i<bp_vec_.size();++i)
		{
			breakpoint_t& bp = bp_vec_[i];
			if (bp.user_enable == true && bp.valid == false)
			{
				valid_breakpoint(&bp);
			}
// 			else if(bp.user_enable == true && bp.valid == true)
// 			{
// 				invalid_breakpoint(&bp);
// 			}
		}
	}

	std::vector<breakpoint_t>& get_bp_list()
	{
		return bp_vec_;
	}

	const CONTEXT& get_current_context()
	{
		return context_;
	}


	bool symbol_from_addr(DWORD addr,std::string& symbol,bool allow_in_func = false);
	bool symbol_from_addr( DWORD addr,PSYMBOL_INFO symbol_info);
	bool stack_walk(STACKFRAME64& stack_frame, CONTEXT& context);
	bool stop_debug()
	{
		if (!TerminateProcess(handle_,0))
		{
			return false;
		}
		set_continue_status(DBG_CONTINUE);
		return  continue_debug();
	}

private:
	// 最后一次WaitForDebugEvent获取到的调试事件
	DEBUG_EVENT debug_event_;
	// 最后一次调试事件时的线程context
	CONTEXT context_;
	// 异常是否已经处理
	DWORD	continue_status_;
	// 继续调试所用的事件
	HANDLE continue_event_;
	//HANDLE process_handle_;
	bool debugee_exit_;
	//DWORD process_id_;
	debug_status debug_status_;

	std::vector<breakpoint_t> bp_vec_;

	// 被调试进程的进程句柄
	HANDLE handle_;
	// 被调试进程的进程ID
	DWORD pid_;

	std::string sym_search_path_;

	struct load_dll_info_t 
	{
		HANDLE file_handle;
		DWORD base_of_dll;
		std::string dll_path;
	};

	std::vector<load_dll_info_t> load_dll_info_;

// 	std::string exe_path_;
// 	std::string command_str_;
// 	std::string current_path_;

private:
	void debug_thread_proc();
	bool on_create_process_event(const CREATE_PROCESS_DEBUG_INFO& create_process_info);
	bool on_exit_process_event(const EXIT_PROCESS_DEBUG_INFO& exit_process);
	bool on_create_thread_event(const CREATE_THREAD_DEBUG_INFO& create_thread);
	bool on_exit_thread_event(const EXIT_THREAD_DEBUG_INFO& exit_thread);
	bool on_load_dll_event(const LOAD_DLL_DEBUG_INFO& load_dll);
	bool on_unload_dll_event(const UNLOAD_DLL_DEBUG_INFO& unload_dll);
	bool on_output_debug_string_event(const OUTPUT_DEBUG_STRING_INFO& debug_string);
	bool on_rip_event(const RIP_INFO& rip_info);
	bool on_exception_event(const EXCEPTION_DEBUG_INFO& debug_exception);

	bool load_symbol(const load_dll_info_t& info);


////////////////////////////////////////////////////////////////////////////////////////////////
	//memory map相关
public:
	struct section_info_t
	{
		char	name[IMAGE_SIZEOF_SHORT_NAME+1];
		byte*	start_addr;
		DWORD	size;
	};

	struct module_info_t 
	{
		BYTE  * module_base_addr;        // Base address of module in th32ProcessID's context
		DWORD   module_base_size;        // Size in bytes of module starting at modBaseAddr
		char    module_name[MAX_MODULE_NAME32 + 1];
		DWORD	pe_header_addr;
		int		section_nums;
		section_info_t	section_info[96];

		std::vector<BYTE*> entry_vector;
	};

	struct memory_region_info_t 
	{
		BYTE*	start_addr;
		DWORD	size;
		std::string	owner_name;
		std::string section_name;
		// 		BYTE*	pSectionStart;
// 		DWORD	dwContains;
		DWORD	type;
		DWORD	protect;
		DWORD	alloc_protect;

		std::vector<BYTE*> entry_vector;
		//std::string	strMapAs;
	};

	std::vector<memory_region_info_t>	memory_info_vector_;
	std::vector<module_info_t>	module_vector_;

	void refresh_memory_map(void);
	bool get_memory_info_by_addr(const void* addr,memory_region_info_t& info)
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
	void set_sym_search_path(const char* paths, bool reload);
};

