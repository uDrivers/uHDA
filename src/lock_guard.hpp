#pragma once
#include "uhda/kernel_api.h"

namespace uhda {
	struct LockGuard {
		inline explicit LockGuard(void* lock) : lock {lock} {
			irq_state = uhda_kernel_lock_spinlock(lock);
		}

		inline ~LockGuard() {
			uhda_kernel_unlock_spinlock(lock, irq_state);
		}

		LockGuard(LockGuard&&) = delete;

	private:
		void* lock;
		UhdaIrqState irq_state;
	};
}
