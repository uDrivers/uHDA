#pragma once
#include "uhda/kernel_api.h"
#include "utils.hpp"

namespace uhda {
	template<typename T>
	class vector {
	public:
		vector() = default;

		vector(const vector&) = delete;
		vector& operator=(const vector&) = delete;

		vector(vector&& other) : ptr {other.ptr}, _size {other._size} {
			other.ptr = nullptr;
			other._size = 0;
		}

		vector& operator=(vector&& other) {
			if (ptr) {
				for (size_t i = 0; i < _size; ++i) {
					ptr[i].~T();
				}

				uhda_kernel_free(ptr, _size * sizeof(T));
			}

			ptr = other.ptr;
			_size = other._size;
			other.ptr = nullptr;
			other._size = 0;
			return *this;
		}

		~vector() {
			for (size_t i = 0; i < _size; ++i) {
				ptr[i].~T();
			}
			uhda_kernel_free(ptr, _size * sizeof(T));
		}

		[[nodiscard]] bool push(T value) {
			auto* new_ptr = static_cast<T*>(uhda_kernel_malloc((_size + 1) * sizeof(T)));
			if (!new_ptr) {
				return false;
			}

			for (size_t i = 0; i < _size; ++i) {
				construct<T>(&new_ptr[i], move(ptr[i]));
				ptr[i].~T();
			}

			uhda_kernel_free(ptr, _size * sizeof(T));

			construct<T>(&new_ptr[_size++], move(value));

			ptr = new_ptr;

			return true;
		}

		[[nodiscard]] bool insert(T* pos, T value) {
			auto* new_ptr = static_cast<T*>(uhda_kernel_malloc((_size + 1) * sizeof(T)));
			if (!new_ptr) {
				return false;
			}

			if (!pos) {
				pos = ptr;
			}

			size_t index = pos - ptr;

			for (size_t i = 0; i < index; ++i) {
				construct<T>(&new_ptr[i], move(ptr[i]));
				ptr[i].~T();
			}

			construct<T>(&new_ptr[index], move(value));

			for (size_t i = index + 1; i < _size; ++i) {
				construct<T>(&new_ptr[i], move(ptr[i - 1]));
				ptr[i - 1].~T();
			}

			uhda_kernel_free(ptr, _size * sizeof(T));

			ptr = new_ptr;

			return true;
		}

		[[nodiscard]] bool resize(size_t new_size) {
			if (new_size < _size) {
				for (size_t i = new_size; i < _size; ++i) {
					ptr[i].~T();
				}
			}
			else {
				auto* new_ptr = static_cast<T*>(uhda_kernel_malloc(new_size * sizeof(T)));
				if (!new_ptr) {
					return false;
				}

				for (size_t i = 0; i < _size; ++i) {
					construct<T>(&new_ptr[i], move(ptr[i]));
					ptr[i].~T();
				}

				uhda_kernel_free(ptr, _size * sizeof(T));

				ptr = new_ptr;

				for (size_t i = _size; i < new_size; ++i) {
					construct<T>(&new_ptr[i]);
				}
			}

			_size = new_size;
			return true;
		}

		void pop() {
			ptr[--_size].~T();
		}

		constexpr T* begin() {
			return ptr;
		}

		constexpr const T* begin() const {
			return ptr;
		}

		constexpr T* end() {
			return ptr + _size;
		}

		constexpr const T* end() const {
			return ptr + _size;
		}

		constexpr T* data() {
			return ptr;
		}

		constexpr const T* data() const {
			return ptr;
		}

		constexpr T& operator[](size_t index) {
			return ptr[index];
		}

		constexpr const T& operator[](size_t index) const {
			return ptr[index];
		}

		constexpr T& front() {
			return ptr[0];
		}

		constexpr T& back() {
			return ptr[_size - 1];
		}

		[[nodiscard]] constexpr size_t size() const {
			return _size;
		}

		[[nodiscard]] constexpr bool is_empty() const {
			return !_size;
		}

	private:
		T* ptr {};
		size_t _size {};
	};
}
