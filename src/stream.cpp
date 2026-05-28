#include "stream.hpp"
#include "fmt_utils.hpp"
#include "scope_guard.hpp"
#include "uhda/kernel_api.h"

using namespace uhda;

UhdaStream::~UhdaStream() {
	destroy();
}

UhdaStatus UhdaStream::setup(const UhdaStreamParams* params) {
	PcmFormat fmt = pcm_format_from_params(params->sample_rate, params->channels, params->fmt);
	space.store(regs::stream::FMT, fmt.value);

	UHDA_TRY(uhda_kernel_allocate_physical(0x1000, &bdl_phys));

	ScopeGuard destroy_guard {[&] {
		destroy();
	}};

	void* bdl_ptr;
	UHDA_TRY(uhda_kernel_map(bdl_phys, 0x1000, &bdl_ptr));
	bdl = launder(static_cast<BufferDescriptor*>(bdl_ptr));

	bdl_chunks = static_cast<UhdaScatterChunk*>(uhda_kernel_malloc(params->period_count * sizeof(UhdaScatterChunk)));
	if (!bdl_chunks) {
		return UHDA_STATUS_NO_MEMORY;
	}

	UHDA_TRY(uhda_kernel_allocate_scatter(params->period_count, params->period_size, bdl_chunks));

	bdl_chunk_count = params->period_count;
	bdl_chunk_size = params->period_size;

	for (size_t i = 0; i < bdl_chunk_count; ++i) {
		if (bdl_chunks[i].phys % 128 != 0) {
			return UHDA_STATUS_MISALIGNED_MEMORY;
		}

		bdl[i].address = bdl_chunks[i].phys;
		bdl[i].length = bdl_chunk_size;
		bdl[i].ioc = i % params->period_callback_distance == 0;
	}

	period_callback = params->period_callback;
	period_callback_arg = params->period_callback_arg;

	destroy_guard.done();

	space.store(regs::stream::BDPL, bdl_phys);
	space.store(regs::stream::BDPU, bdl_phys >> 32);

	space.store(regs::stream::CBL, bdl_chunk_count * bdl_chunk_size);

	auto lvi = space.load(regs::stream::LVI);
	lvi &= ~sdlvi::LVI;
	lvi |= sdlvi::LVI(bdl_chunk_count - 1);
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
	if (bdl_chunks) {
		if (bdl_chunk_count != 0) {
			uhda_kernel_deallocate_scatter(bdl_chunks, bdl_chunk_count, bdl_chunk_size);
			bdl_chunk_count = 0;
		}

		uhda_kernel_free(bdl_chunks, bdl_chunk_count * sizeof(UhdaScatterChunk));
		bdl_chunks = nullptr;
	}

	if (bdl) {
		uhda_kernel_unmap(bdl, 0x1000);
		bdl = nullptr;
	}

	if (bdl_phys) {
		uhda_kernel_deallocate_physical(bdl_phys, 0x1000);
		bdl_phys = 0;
	}

	space.store(regs::stream::CTL0, sdctl0::RST(true));
	// todo maybe a timeout here,
	// it's unlikely that the controller is broken at this point though.
	while (!(space.load(regs::stream::CTL0) & sdctl0::RST));
	space.store(regs::stream::CTL0, 0);
	while (space.load(regs::stream::CTL0) & sdctl0::RST);
}

void UhdaStream::play(bool play) {
	auto ctl0 = space.load(regs::stream::CTL0);
	if (play) {
		if (ctl0 & sdctl0::RUN) {
			return;
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

uint32_t UhdaStream::get_pos() const {
	return *dma_pos;
}

void UhdaStream::output_irq() {
	period_callback(this, period_callback_arg);
	space.store(regs::stream::STS, sdsts::BCIS(true));
}
