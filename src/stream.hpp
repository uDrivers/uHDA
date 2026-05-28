#pragma once
#include "reg.hpp"
#include "uhda/types.h"
#include "spec.hpp"

struct UhdaStream {
	~UhdaStream();

	UhdaStatus setup(const UhdaStreamParams* params);
	void destroy();

	void play(bool play);

	[[nodiscard]] uint32_t get_pos() const;

	void output_irq();

	uhda::MemSpace space {0};
	uintptr_t bdl_phys {};
	uhda::BufferDescriptor* bdl {};

	UhdaScatterChunk* bdl_chunks {};
	uint32_t bdl_chunk_count {};
	uint32_t bdl_chunk_size {};
	UhdaPeriodFn period_callback {};
	void* period_callback_arg {};

	volatile uint32_t* dma_pos {};

	uint8_t index {};
	bool output {};
};
