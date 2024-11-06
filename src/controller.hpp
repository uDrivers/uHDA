#pragma once
#include "uhda/uhda.h"
#include "reg.hpp"
#include "spec.hpp"
#include "stream.hpp"
#include "vector.hpp"
#include "codec.hpp"

struct UhdaController {
	constexpr explicit UhdaController(void* pci_device) : pci_device {pci_device} {}

	~UhdaController();

	UhdaStatus init();
	UhdaStatus destroy();

	UhdaStatus suspend();
	UhdaStatus resume();

	uint8_t submit_verb(uint8_t cid, uint8_t nid, uint16_t cmd, uint8_t data);
	uint8_t submit_verb_long(uint8_t cid, uint8_t nid, uint8_t cmd, uint16_t data);
	UhdaStatus wait_for_verb(uint8_t index, uhda::ResponseDescriptor& res);

	UhdaStatus pci_setup();
	UhdaStatus map_bar();

	void* pci_device;
	void* irq {};
	uhda::MemSpace space {0};
	uint32_t bar {};
	uint16_t corb_size {};
	uint16_t rirb_size {};
	uintptr_t corb_phys {};
	uintptr_t rirb_phys {};
	uintptr_t dpl_phys {};
	uhda::VerbDescriptor* corb {};
	uhda::ResponseDescriptor* rirb {};
	uint32_t* dma_pos {};
	UhdaStream in_streams[16] {};
	UhdaStream out_streams[16] {};
	UhdaStream* in_stream_ptrs[16] {};
	UhdaStream* out_stream_ptrs[16] {};
	uhda::vector<UhdaCodec*> codecs;
	uint8_t in_stream_count {};
	uint8_t out_stream_count {};

	void* lock {};
};
