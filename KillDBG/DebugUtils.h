#pragma once

namespace debug_utils
{

	struct scope_exit
	{
		scope_exit(std::function<void (void)> f) : f_(f) {}
		~scope_exit(void) { f_(); }
	private:
		std::function<void (void)> f_;
	};


	bool get_error_msg(DWORD error_code,std::string& message);
	bool get_file_name_frome_handle(HANDLE hfile, std::string& file_name);

	bool is_bound(char ch);

	bool get_word_from_pos(const std::string& src,int pos,int& start,int& end);
}

