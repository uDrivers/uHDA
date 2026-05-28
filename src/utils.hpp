#pragma once

#include <stddef.h>

namespace uhda {
	template<typename T>
	struct remove_reference {
		using type = T;
	};
	template<typename T>
	struct remove_reference<T&> {
		using type = T;
	};
	template<typename T>
	struct remove_reference<T&&> {
		using type = T;
	};

	template<typename T>
	using remove_reference_t = typename remove_reference<T>::type;

	template<typename T>
	constexpr T&& forward(remove_reference_t<T>& value) {
		return static_cast<T&&>(value);
	}
	template<typename T>
	constexpr T&& forward(remove_reference_t<T>&& value) {
		return static_cast<T&&>(value);
	}

	template<typename T>
	constexpr remove_reference_t<T>&& move(T&& value) {
		return static_cast<remove_reference_t<T>&&>(value);
	}

	template<typename T, typename... Args>
	constexpr T* construct(void* ptr, Args&&... args) {
		struct Container {
			explicit Container(Args&&... args) : value {forward<Args>(args)...} {}

			constexpr void* operator new(size_t, void* ptr) {
				return ptr;
			}

			T value;
		};

		return &(new (ptr) Container {forward<Args>(args)...})->value;
	}

	template<typename T>
	constexpr T* launder(T* ptr) {
		return __builtin_launder(ptr);
	}
}

#define UHDA_TRY(what) do { \
	if (auto try_status_ = what; try_status_ != UHDA_STATUS_SUCCESS) { \
		return try_status_; \
	} \
} while (false)
