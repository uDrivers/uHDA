#pragma once

#include "utils.hpp"

namespace uhda {
	template<typename F>
	struct ScopeGuard {
		constexpr ScopeGuard(F fn) : fn {move(fn)} {}

		constexpr ~ScopeGuard() {
			if (!done_) {
				fn();
			}
		}

		constexpr void done() {
			done_ = true;
		}

	private:
		F fn;
		bool done_ {};
	};
}
