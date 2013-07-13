#pragma once

#include <functional>

namespace kd_utils
{
	struct ScopeExit
	{
		ScopeExit(std::function<void (void)> f) : f_(f) {}
		~ScopeExit(void) { f_(); }
		void Reset(std::function<void (void)> f){f_ = f;}
	private:
		std::function<void (void)> f_;
	};
}
