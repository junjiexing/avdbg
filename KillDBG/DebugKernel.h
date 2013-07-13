#pragma once

class CMainFrame;

class DebugKernel : public std::enable_shared_from_this<DebugKernel>
{
public:
	DebugKernel(void);
	~DebugKernel(void);

	bool load_exe(const std::string& exe_path,const std::string& command_str,const std::string& current_path);
	bool attach_process(DWORD process_id);
	bool read_debugee_memory(const void* address,void* buffer,size_t size);

	enum OutputType
	{
		OUT_INFO,
		OUT_WARNING,
		OUT_ERROR
	};

	CMainFrame* main_frm_ptr_;
	void output_string(const std::string& str,OutputType type = OUT_INFO);

	void set_output_func(const std::function<void(const std::string&,OutputType)>& fn)
	{output_message_ = fn;}

	DWORD get_process_id(){return process_id_;}

	SIZE_T virtual_query_ex(const void* address,MEMORY_BASIC_INFORMATION& mem_info)
	{
		return VirtualQueryEx(process_handle_,address,&mem_info,sizeof(mem_info));
	}

private:
	void debug_thread_proc();
	void on_create_process_event(const CREATE_PROCESS_DEBUG_INFO& create_process_info);
	void on_exit_process_event(const EXIT_PROCESS_DEBUG_INFO& exit_process);
	void on_create_thread_event(const CREATE_THREAD_DEBUG_INFO& create_thread);
	void on_exit_thread_event(const EXIT_THREAD_DEBUG_INFO& exit_thread);
	void on_load_dll_event(const LOAD_DLL_DEBUG_INFO& load_dll);
	void on_unload_dll_event(const UNLOAD_DLL_DEBUG_INFO& unload_dll);
	void on_output_debug_string_event(const OUTPUT_DEBUG_STRING_INFO& debug_string);
	void on_rip_event(const RIP_INFO& rip_info);
	void on_exception_event(const EXCEPTION_DEBUG_INFO& debug_exception);


	DWORD continue_status_;
	HANDLE process_handle_;
	bool debugee_exit_;
	DWORD process_id_;
	std::function<void(const std::string&,OutputType)> output_message_;

////////////////////////////////////////////////////////////////////////////////////////////////
	//memory mapœ‡πÿ
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
};

