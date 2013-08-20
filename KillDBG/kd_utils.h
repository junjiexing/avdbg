#pragma once

#include <functional>

namespace kd_utils
{
	struct scope_exit
	{
		scope_exit(std::function<void (void)> f) : f_(f) {}
		~scope_exit(void) { f_(); }
		void Reset(std::function<void (void)> f){f_ = f;}
	private:
		std::function<void (void)> f_;
	};
}
