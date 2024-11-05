#include "stream.hpp"
#include "uhda/kernel_api.h"

using namespace uhda;

UhdaStream::~UhdaStream() {
	destroy();
}

static constexpr size_t MAX_DESCRIPTORS = 0x1000 / sizeof(BufferDescriptor);

#define memcpy __builtin_memcpy
#define memset __builtin_memset

UhdaStatus UhdaStream::setup(uint32_t buffer_size) {
	auto status = uhda_kernel_allocate_physical(0x1000, &bdl_phys);
	if (status != UHDA_STATUS_SUCCESS) {
		return status;
	}

	void* bdl_ptr;
	status = uhda_kernel_map(bdl_phys, 0x1000, &bdl_ptr);
	if (status != UHDA_STATUS_SUCCESS) {
		uhda_kernel_deallocate_physical(bdl_phys, 0x1000);
		bdl_phys = 0;
		return status;
	}

	bdl = launder(static_cast<BufferDescriptor*>(bdl_ptr));

	buffer_pages = static_cast<void**>(uhda_kernel_malloc(MAX_DESCRIPTORS * sizeof(void*)));
	if (!buffer_pages) {
		uhda_kernel_unmap(bdl, 0x1000);
		uhda_kernel_deallocate_physical(bdl_phys, 0x1000);
		bdl = nullptr;
		bdl_phys = 0;
		return UHDA_STATUS_NO_MEMORY;
	}

	for (size_t i = 0; i < MAX_DESCRIPTORS; ++i) {
		uintptr_t page = 0;
		status = uhda_kernel_allocate_physical(0x1000, &page);

		buffer_pages[i] = nullptr;
		if (status == UHDA_STATUS_SUCCESS) {
			status = uhda_kernel_map(page, 0x1000, &buffer_pages[i]);
		}

		if (status != UHDA_STATUS_SUCCESS) {
			for (size_t j = 0; j < i; ++j) {
				uhda_kernel_unmap(buffer_pages[j], 0x1000);
				uhda_kernel_deallocate_physical(bdl[j].address, 0x1000);
			}

			if (buffer_pages[i]) {
				uhda_kernel_unmap(buffer_pages[i], 0x1000);
			}
			if (page) {
				uhda_kernel_deallocate_physical(page, 0x1000);
			}

			uhda_kernel_free(buffer_pages, MAX_DESCRIPTORS * sizeof(void*));

			uhda_kernel_unmap(bdl, 0x1000);
			uhda_kernel_deallocate_physical(bdl_phys, 0x1000);
			buffer_pages = nullptr;
			bdl = nullptr;
			bdl_phys = 0;
			return status;
		}

		bdl[i].address = page;
		bdl[i].length = 0x1000;
		bdl[i].ioc = 1;
	}

	ring_buffer_capacity = buffer_size;
	ring_buffer = uhda_kernel_malloc(ring_buffer_capacity);
	if (!ring_buffer) {
		destroy();
		return UHDA_STATUS_NO_MEMORY;
	}

	space.store(regs::stream::BDPL, bdl_phys);
	space.store(regs::stream::BDPU, bdl_phys >> 32);

	space.store(regs::stream::CBL, MAX_DESCRIPTORS * 0x1000);

	auto lvi = space.load(regs::stream::LVI);
	lvi &= ~sdlvi::LVI;
	lvi |= sdlvi::LVI(MAX_DESCRIPTORS - 1);
	space.store(regs::stream::LVI, lvi);

	auto ctl2 = space.load(regs::stream::CTL2);
	ctl2 &= ~sdctl2::STRM;
	ctl2 |= sdctl2::STRM(index + 1);
	space.store(regs::stream::CTL2, ctl2);

	auto ctl0 = space.load(regs::stream::CTL0);
	ctl0 |= sdctl0::IOCE(true);
	space.store(regs::stream::CTL0, ctl0);

	return UHDA_STATUS_SUCCESS;
}

void UhdaStream::destroy() {
	if (buffer_pages) {
		for (size_t i = 0; i < MAX_DESCRIPTORS; ++i) {
			uhda_kernel_unmap(buffer_pages[i], 0x1000);
			uhda_kernel_deallocate_physical(bdl[i].address, 0x1000);
		}

		uhda_kernel_free(buffer_pages, MAX_DESCRIPTORS * sizeof(void*));

		if (ring_buffer) {
			uhda_kernel_free(ring_buffer, ring_buffer_capacity);
		}

		uhda_kernel_unmap(bdl, 0x1000);
		uhda_kernel_deallocate_physical(bdl_phys, 0x1000);

		buffer_pages = nullptr;
		bdl = nullptr;
		bdl_phys = 0;
	}
}

static constexpr size_t BUFFER_SIZE = 0x1000 * MAX_DESCRIPTORS;

void UhdaStream::queue_data(const void* data, uint32_t* size) {
	// todo lock
	auto ring_remaining = ring_buffer_size;

	uint32_t to_copy = ring_buffer_capacity - ring_remaining;
	if (*size < to_copy) {
		to_copy = *size;
	}

	auto remaining_at_end = ring_buffer_capacity - ring_buffer_write_pos;

	if (remaining_at_end >= to_copy) {
		memcpy(
			launder(static_cast<char*>(ring_buffer) + ring_buffer_write_pos),
			data,
			to_copy);
		ring_buffer_write_pos += to_copy;
		if (ring_buffer_write_pos == BUFFER_SIZE) {
			ring_buffer_write_pos = 0;
		}
	}
	else {
		memcpy(
			launder(static_cast<char*>(ring_buffer) + ring_buffer_write_pos),
			data,
			remaining_at_end);

		auto remaining = to_copy - remaining_at_end;
		memcpy(
			ring_buffer,
			launder(static_cast<const char*>(data) + remaining_at_end),
			remaining);
		ring_buffer_write_pos = remaining;
		if (ring_buffer_write_pos == ring_buffer_capacity) {
			ring_buffer_write_pos = 0;
		}
	}

	ring_buffer_size += to_copy;

	*size = to_copy;
}

void UhdaStream::output_irq() {
	auto pos = current_pos;

	auto desc_index = pos / 0x1000;
	auto desc_ptr = buffer_pages[desc_index];

	// todo lock
	auto size = ring_buffer_size;
	if (size < 0x1000) {
		// underflow
		if (buffer_fill_fn) {
			uint32_t can_copy = ring_buffer_capacity - size;

			ring_buffer_size += can_copy;

			auto remaining_at_end = ring_buffer_capacity - ring_buffer_write_pos;

			if (remaining_at_end >= can_copy) {
				buffer_fill_fn(
					buffer_fill_fn_arg,
					launder(static_cast<char*>(ring_buffer) + ring_buffer_write_pos),
					can_copy);
				ring_buffer_write_pos += can_copy;
				if (ring_buffer_write_pos == ring_buffer_capacity) {
					ring_buffer_write_pos = 0;
				}
			}
			else {
				buffer_fill_fn(
					buffer_fill_fn_arg,
					launder(static_cast<char*>(ring_buffer) + ring_buffer_write_pos),
					remaining_at_end);
				can_copy -= remaining_at_end;

				buffer_fill_fn(
					buffer_fill_fn_arg,
					ring_buffer,
					can_copy);
				ring_buffer_write_pos = can_copy;
			}

			auto ptr = launder(
				static_cast<char*>(ring_buffer) +
				ring_buffer_read_pos);
			memcpy(desc_ptr, ptr, 0x1000);
			ring_buffer_read_pos += 0x1000;
			if (ring_buffer_read_pos == ring_buffer_capacity) {
				ring_buffer_read_pos = 0;
			}
			ring_buffer_size -= 0x1000;
		}
		else {
			memset(desc_ptr, 0, 0x1000);
		}
	}
	else {
		auto ptr = launder(
			static_cast<char*>(ring_buffer) +
			ring_buffer_read_pos);
		memcpy(desc_ptr, ptr, 0x1000);
		ring_buffer_read_pos += 0x1000;
		if (ring_buffer_read_pos == ring_buffer_capacity) {
			ring_buffer_read_pos = 0;
		}
		ring_buffer_size -= 0x1000;
	}

	current_pos += 0x1000;
	if (current_pos == BUFFER_SIZE) {
		current_pos = 0;
	}

	auto sts = space.load(regs::stream::STS);
	sts &= ~sdsts::BCIS;
	space.store(regs::stream::STS, sts);
}
