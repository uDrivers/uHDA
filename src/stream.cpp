#include "stream.hpp"
#include "lock_guard.hpp"
#include "uhda/kernel_api.h"

using namespace uhda;

UhdaStream::~UhdaStream() {
	destroy();
}

static constexpr size_t MAX_DESCRIPTORS = 0x1000 / sizeof(BufferDescriptor);
static constexpr size_t BUFFER_SIZE = 0x1000 * MAX_DESCRIPTORS;
static constexpr uint32_t ALLOWED_SOFTWARE_AHEAD = 0x1000 * 4;

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
	LockGuard guard {lock};

	if (buffer_pages) {
		for (size_t i = 0; i < MAX_DESCRIPTORS; ++i) {
			uhda_kernel_unmap(buffer_pages[i], 0x1000);
			uhda_kernel_deallocate_physical(bdl[i].address, 0x1000);
		}

		uhda_kernel_free(buffer_pages, MAX_DESCRIPTORS * sizeof(void*));

		if (ring_buffer) {
			uhda_kernel_free(ring_buffer, ring_buffer_capacity);
			ring_buffer = nullptr;
		}

		uhda_kernel_unmap(bdl, 0x1000);
		uhda_kernel_deallocate_physical(bdl_phys, 0x1000);

		buffer_pages = nullptr;
		bdl = nullptr;
		bdl_phys = 0;
	}

	space.store(regs::stream::CTL0, sdctl0::RST(true));
	// todo maybe a timeout here,
	// it's unlikely that the controller is broken at this point though.
	while (!(space.load(regs::stream::CTL0) & sdctl0::RST));
	space.store(regs::stream::CTL0, 0);
	while (space.load(regs::stream::CTL0) & sdctl0::RST);

	current_fill_pos = 0;
	ring_buffer_read_pos = 0;
	ring_buffer_write_pos = 0;
}

void UhdaStream::play(bool play) {
	LockGuard guard {lock};

	auto ctl0 = space.load(regs::stream::CTL0);
	if (play) {
		if (ctl0 & sdctl0::RUN) {
			return;
		}

		auto pos = get_pos() % BUFFER_SIZE;
		auto software_ahead = get_software_ahead(pos);

		if (software_ahead < ALLOWED_SOFTWARE_AHEAD) {
			auto allowed_copy = ALLOWED_SOFTWARE_AHEAD - software_ahead;
			allowed_copy = (allowed_copy + 0xFFF) & ~0xFFF;

			if (ring_buffer_size >= 0x1000) {
				auto to_copy = allowed_copy;

				if (ring_buffer_size < allowed_copy) {
					to_copy = ring_buffer_size & ~0xFFF;
				}

				for (uint32_t i = 0; i < to_copy; i += 0x1000) {
					auto desc_index = current_fill_pos / 0x1000;
					auto desc_ptr = buffer_pages[desc_index];

					auto ptr = launder(
						static_cast<char*>(ring_buffer) +
						ring_buffer_read_pos);
					memcpy(desc_ptr, ptr, 0x1000);
					ring_buffer_read_pos += 0x1000;
					if (ring_buffer_read_pos == ring_buffer_capacity) {
						ring_buffer_read_pos = 0;
					}
					ring_buffer_size -= 0x1000;

					current_fill_pos += 0x1000;
					if (current_fill_pos == BUFFER_SIZE) {
						current_fill_pos = 0;
					}
				}

				allowed_copy = to_copy;
			}

			if (allowed_copy) {
				for (uint32_t i = 0; i < allowed_copy; i += 0x1000) {
					auto desc_index = current_fill_pos / 0x1000;
					auto desc_ptr = buffer_pages[desc_index];

					memset(desc_ptr, 0, 0x1000);

					current_fill_pos += 0x1000;
					if (current_fill_pos == BUFFER_SIZE) {
						current_fill_pos = 0;
					}
				}
			}
		}

		ctl0 |= sdctl0::RUN(true);
		space.store(regs::stream::CTL0, ctl0);
	}
	else {
		if (ctl0 & sdctl0::RUN) {
			ctl0 &= ~sdctl0::RUN;
			space.store(regs::stream::CTL0, ctl0);
		}
	}
}

void UhdaStream::queue_data(const void* data, uint32_t* size) {
	LockGuard guard {lock};

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
		if (ring_buffer_write_pos == ring_buffer_capacity) {
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

uint32_t UhdaStream::get_pos() const {
	return *dma_pos;
}

uint32_t UhdaStream::get_software_ahead(uint32_t pos) const {
	uint32_t software_ahead;
	if (current_fill_pos >= pos) {
		software_ahead = current_fill_pos - pos;
	}
	else {
		software_ahead = BUFFER_SIZE - pos + current_fill_pos;
	}
	return software_ahead;
}

void UhdaStream::ring_buffer_read(void* dest, size_t size) {
	auto src_ptr = launder(static_cast<char*>(ring_buffer) + ring_buffer_read_pos);
	auto dest_ptr = launder(static_cast<char*>(dest));

	auto remaining_at_end = ring_buffer_capacity - ring_buffer_read_pos;
	if (size <= remaining_at_end) {
		memcpy(dest_ptr, src_ptr, size);
		ring_buffer_read_pos += size;
		if (ring_buffer_read_pos == ring_buffer_capacity) {
			ring_buffer_read_pos = 0;
		}
		ring_buffer_size -= size;
	}
	else {
		memcpy(dest_ptr, src_ptr, remaining_at_end);
		memcpy(dest_ptr + remaining_at_end, ring_buffer, size - remaining_at_end);

		ring_buffer_read_pos = size - remaining_at_end;
		if (ring_buffer_read_pos == ring_buffer_capacity) {
			ring_buffer_read_pos = 0;
		}
		ring_buffer_size -= size;
	}
}

void UhdaStream::output_irq() {
	auto pos = get_pos() % BUFFER_SIZE;

	auto software_ahead = get_software_ahead(pos);

	if (software_ahead > ALLOWED_SOFTWARE_AHEAD) {
		prev_irq_pos = pos;

		space.store(regs::stream::STS, sdsts::BCIS(true));

		return;
	}

	uint32_t bytes_after_last_irq;
	if (pos >= prev_irq_pos) {
		bytes_after_last_irq = pos - prev_irq_pos;
	}
	else {
		bytes_after_last_irq = BUFFER_SIZE - prev_irq_pos + pos;
	}

	LockGuard guard {lock};

	if (buffer_trip_threshold && ring_buffer_size < buffer_trip_threshold) {
		buffer_trip_fn(buffer_trip_fn_arg, ring_buffer_size);
	}

	for (uint32_t remaining = bytes_after_last_irq; remaining;) {
		auto desc_index = current_fill_pos / 0x1000;
		auto desc_offset = current_fill_pos % 0x1000;
		auto desc_ptr = static_cast<char*>(buffer_pages[desc_index]) + desc_offset;

		uint32_t to_copy_desc = 0x1000 - desc_offset;

		if (remaining < to_copy_desc) {
			to_copy_desc = remaining;
		}

		auto orig_to_copy = to_copy_desc;

		auto ring_buffer_available = ring_buffer_size;
		if (ring_buffer_available >= to_copy_desc) {
			ring_buffer_read(desc_ptr, to_copy_desc);
		}
		else {
			if (ring_buffer_available) {
				ring_buffer_read(desc_ptr, ring_buffer_available);
				desc_ptr += ring_buffer_available;
				to_copy_desc -= ring_buffer_available;
				ring_buffer_available = 0;
			}

			if (buffer_fill_fn) {
				uint32_t can_copy = ring_buffer_capacity - ring_buffer_size;

				auto remaining_at_end = ring_buffer_capacity - ring_buffer_write_pos;

				if (remaining_at_end >= can_copy) {
					auto copied = buffer_fill_fn(
						buffer_fill_fn_arg,
						launder(static_cast<char*>(ring_buffer) + ring_buffer_write_pos),
						can_copy);
					ring_buffer_write_pos += copied;
					if (ring_buffer_write_pos == ring_buffer_capacity) {
						ring_buffer_write_pos = 0;
					}
					ring_buffer_size += copied;
				}
				else {
					auto copied = buffer_fill_fn(
						buffer_fill_fn_arg,
						launder(static_cast<char*>(ring_buffer) + ring_buffer_write_pos),
						remaining_at_end);
					can_copy -= copied;
					ring_buffer_size += copied;

					if (copied == remaining_at_end) {
						ring_buffer_write_pos = 0;
						copied = buffer_fill_fn(
							buffer_fill_fn_arg,
							ring_buffer,
							can_copy);
						ring_buffer_write_pos += copied;
						if (ring_buffer_write_pos == ring_buffer_capacity) {
							ring_buffer_write_pos = 0;
						}
						ring_buffer_size += copied;
					}
					else {
						ring_buffer_write_pos += copied;
					}
				}
			}

			if (ring_buffer_available >= to_copy_desc) {
				ring_buffer_read(desc_ptr, to_copy_desc);
			}
			else {
				if (ring_buffer_available) {
					ring_buffer_read(desc_ptr, ring_buffer_available);
					desc_ptr += ring_buffer_available;
					to_copy_desc -= ring_buffer_available;
				}

				memset(desc_ptr, 0, to_copy_desc);
			}
		}

		remaining -= orig_to_copy;

		current_fill_pos += orig_to_copy;
		if (current_fill_pos == BUFFER_SIZE) {
			current_fill_pos = 0;
		}
	}

	prev_irq_pos = pos;

	space.store(regs::stream::STS, sdsts::BCIS(true));
}
