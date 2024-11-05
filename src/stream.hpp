#pragma once
#include "reg.hpp"
#include "uhda/types.h"
#include "spec.hpp"

struct UhdaStream {
	~UhdaStream();

	UhdaStatus setup(uint32_t buffer_size);
	void destroy();

	void queue_data(const void* data, uint32_t* size);

	void output_irq();

	uhda::MemSpace space {0};
	UhdaBufferFillFn buffer_fill_fn {};
	void* buffer_fill_fn_arg {};
	uintptr_t bdl_phys {};
	uhda::BufferDescriptor* bdl {};

	void** buffer_pages {};
	void* ring_buffer {};
	uint32_t ring_buffer_capacity {};
	volatile uint32_t ring_buffer_size {};
	uint32_t current_pos {};
	uint32_t ring_buffer_write_pos {};
	uint32_t ring_buffer_read_pos {};

	uint8_t index {};
	bool output {};
};
