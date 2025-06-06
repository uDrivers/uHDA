#pragma once
#include "utils.hpp"
#include <stdint.h>

namespace uhda {
	template<typename T, typename R>
	struct Register {
		constexpr explicit Register(uintptr_t offset) : offset {offset} {}

		using bits_type = T;
		using type = R;
		uintptr_t offset;
	};

	template<typename T>
	struct BitValue {
		using type = T;

		constexpr operator type() const volatile { // NOLINT(*-explicit-constructor)
			return value;
		}

		constexpr operator type() const { // NOLINT(*-explicit-constructor)
			return value;
		}

		constexpr void operator|=(type bits) volatile {
			value |= bits;
		}
		constexpr void operator&=(type bits) volatile {
			value &= bits;
		}

		constexpr BitValue& operator|=(type bits) {
			value |= bits;
			return *this;
		}
		constexpr BitValue& operator&=(type bits) {
			value &= bits;
			return *this;
		}

		T value;
	};

	template<typename B, typename T>
	struct BitField {
		friend constexpr T operator&(BitValue<B> value, BitField field) {
			return static_cast<T>((value.value & field.mask) >> field.offset);
		}

		constexpr BitField(int offset, int bits) : offset {offset} {
			mask = ((B {1} << bits) - 1) << offset;
		}

		constexpr B operator()(T value) const {
			return static_cast<B>(value) << offset;
		}

		constexpr B operator~() const {
			return ~mask;
		}

		B mask;
		int offset;
	};

	struct MemSpace {
		constexpr explicit MemSpace(uintptr_t base) : base {base} {}

		template<typename R>
		void store(R reg, typename R::bits_type value) {
			*launder(reinterpret_cast<volatile typename R::bits_type*>(base + reg.offset)) = value;
		}

		template<typename T>
		void store(uintptr_t offset, T value) {
			*launder(reinterpret_cast<volatile T*>(base + offset)) = value;
		}

		template<typename R>
		typename R::type load(R reg) const {
			return static_cast<typename R::type>(*launder(reinterpret_cast<const volatile typename R::bits_type*>(base + reg.offset)));
		}

		template<typename T>
		T load(uintptr_t offset) {
			return *launder(reinterpret_cast<const volatile T*>(base + offset));
		}

		[[nodiscard]] constexpr MemSpace subspace(uintptr_t offset) const {
			return MemSpace {base + offset};
		}

		uintptr_t base;
	};

	template<typename T>
	using BasicRegister = Register<T, T>;
	template<typename T>
	using BitRegister = Register<T, BitValue<T>>;
}
