#include "controller.hpp"
#include "uhda/kernel_api.h"

namespace {
	UhdaStatus pci_read_cmd(void* pci_device, uint16_t& cmd) {
		uint32_t value;
		if (auto status = uhda_kernel_pci_read(pci_device, 4, 2, &value);
			status != UHDA_STATUS_SUCCESS) {
			return status;
		}
		cmd = value;
		return UHDA_STATUS_SUCCESS;
	}

	UhdaStatus pci_write_cmd(void* pci_device, uint16_t cmd) {
		return uhda_kernel_pci_write(pci_device, 4, 2, cmd);
	}

	UhdaStatus pci_read_bar(void* pci_device, uint32_t bar, uint32_t& value) {
		return uhda_kernel_pci_read(pci_device, 0x10 + bar * 4, 4, &value);
	}

	UhdaStatus pci_write_bar(void* pci_device, uint32_t bar, uint32_t value) {
		return uhda_kernel_pci_write(pci_device, 0x10 + bar * 4, 4, value);
	}

	enum {
		PCI_CMD_MEM_SPACE = 1 << 1,
		PCI_CMD_BUS_MASTER = 1 << 2
	};
}

using namespace uhda;

UhdaController::~UhdaController() {
	uhda_kernel_pci_enable_irq(pci_device, irq, false);

	if (space.base) {
		uhda_kernel_pci_unmap_bar(pci_device, bar, reinterpret_cast<void*>(space.base));
	}

	if (corb) {
		uhda_kernel_unmap(corb, 0x1000);
	}
	if (rirb) {
		uhda_kernel_unmap(rirb, 0x1000);
	}

	if (corb_phys) {
		uhda_kernel_deallocate_physical(corb_phys, 0x1000);
	}
	if (rirb_phys) {
		uhda_kernel_deallocate_physical(rirb_phys, 0x1000);
	}
}

static bool hda_irq(void* arg) {
	auto* controller = static_cast<UhdaController*>(arg);

	auto intsts = controller->space.load(regs::INTSTS);
	if (!intsts) {
		return false;
	}

	auto streams = intsts & intsts::SIS;

	uint32_t stream_count = controller->in_stream_count + controller->out_stream_count;

	for (uint32_t i = 0; i < stream_count; ++i) {
		if (streams & 1U << i) {
			if (i >= controller->in_stream_count) {
				controller->out_streams[i - controller->in_stream_count].output_irq();
			}
		}
	}

	return true;
}

UhdaStatus UhdaController::init() {
	auto status = pci_setup();
	if (status != UHDA_STATUS_SUCCESS) {
		return status;
	}

	status = map_bar();
	if (status != UHDA_STATUS_SUCCESS) {
		return status;
	}

	status = uhda_kernel_pci_allocate_irq(pci_device, hda_irq, this, &irq);
	if (status != UHDA_STATUS_SUCCESS) {
		return status;
	}

	status = uhda_kernel_allocate_physical(0x1000, &corb_phys);
	if (status != UHDA_STATUS_SUCCESS) {
		uhda_kernel_pci_deallocate_irq(pci_device, irq);
		return status;
	}

	status = uhda_kernel_allocate_physical(0x1000, &rirb_phys);
	if (status != UHDA_STATUS_SUCCESS) {
		uhda_kernel_pci_deallocate_irq(pci_device, irq);
		return status;
	}

	void* corb_ptr;
	void* rirb_ptr;

	status = uhda_kernel_map(corb_phys, 0x1000, &corb_ptr);
	if (status != UHDA_STATUS_SUCCESS) {
		uhda_kernel_pci_deallocate_irq(pci_device, irq);
		return status;
	}

	corb = static_cast<VerbDescriptor*>(corb_ptr);

	status = uhda_kernel_map(rirb_phys, 0x1000, &rirb_ptr);
	if (status != UHDA_STATUS_SUCCESS) {
		uhda_kernel_pci_deallocate_irq(pci_device, irq);
		return status;
	}

	rirb = static_cast<ResponseDescriptor*>(rirb_ptr);

	status = resume();
	if (status != UHDA_STATUS_SUCCESS) {
		uhda_kernel_pci_deallocate_irq(pci_device, irq);
		return status;
	}

	return UHDA_STATUS_SUCCESS;
}

UhdaStatus UhdaController::destroy() {
	auto status = suspend();
	uhda_kernel_pci_deallocate_irq(pci_device, irq);
	return status;
}

UhdaStatus UhdaController::suspend() {
	uhda_kernel_pci_enable_irq(pci_device, irq, false);

	auto gctl = space.load(regs::GCTL);
	if (gctl & gctl::CRST) {
		auto corbctl = space.load(regs::CORBCTL);
		corbctl &= ~corbctl::RUN;
		space.store(regs::CORBCTL, corbctl);

		auto rirbctl = space.load(regs::RIRBCTL);
		rirbctl &= ~rirbctl::DMAEN;
		space.store(regs::RIRBCTL, rirbctl);

		auto gcap = space.load(regs::GCAP);

		auto tmp_in_stream_count = gcap & gcap::ISS;
		auto tmp_out_stream_count = gcap & gcap::OSS;

		for (int i = 0; i < tmp_in_stream_count; ++i) {
			auto stream_space = space.subspace(0x80 + i * 0x20);
			auto ctl0 = stream_space.load(regs::stream::CTL0);
			ctl0 &= ~sdctl0::RUN;
			stream_space.store(regs::stream::CTL0, ctl0);
		}

		for (int i = 0; i < tmp_out_stream_count; ++i) {
			auto stream_space = space.subspace(0x80 + tmp_in_stream_count * 0x20 + i * 0x20);
			auto ctl0 = stream_space.load(regs::stream::CTL0);
			ctl0 &= ~sdctl0::RUN;
			stream_space.store(regs::stream::CTL0, ctl0);
		}

		gctl &= ~gctl::CRST;
		space.store(regs::GCTL, gctl);

		for (int i = 0;; ++i) {
			if (i == 5 * 2000) {
				uhda_kernel_pci_enable_irq(pci_device, irq, false);
				return UHDA_STATUS_TIMEOUT;
			}

			if (!(space.load(regs::GCTL) & gctl::CRST)) {
				break;
			}
			uhda_kernel_delay(200);
		}

		uhda_kernel_delay(200);
	}

	return UHDA_STATUS_SUCCESS;
}

UhdaStatus UhdaController::resume() {
	auto status = pci_setup();
	if (status != UHDA_STATUS_SUCCESS) {
		return status;
	}

	status = suspend();
	if (status != UHDA_STATUS_SUCCESS) {
		return status;
	}

	uhda_kernel_pci_enable_irq(pci_device, irq, true);

	auto gctl = space.load(regs::GCTL);
	gctl |= gctl::CRST(true);
	space.store(regs::GCTL, gctl);

	for (int i = 0;; ++i) {
		if (i == 5 * 2000) {
			uhda_kernel_pci_enable_irq(pci_device, irq, false);
			return UHDA_STATUS_TIMEOUT;
		}

		if (space.load(regs::GCTL) & gctl::CRST) {
			break;
		}
		uhda_kernel_delay(200);
	}

	auto gcap = space.load(regs::GCAP);
	if (!(gcap & gcap::OK64)) {
		uhda_kernel_pci_enable_irq(pci_device, irq, false);
		uhda_kernel_log("error: controllers that support only 32-bit addresses are not supported");
		return UHDA_STATUS_UNSUPPORTED;
	}

	auto corb_size_reg = space.load(regs::CORBSIZE);
	auto corb_cap = corb_size_reg & corbsize::SZCAP;

	uint8_t chosen_corb_size = 0;
	if (corb_cap & 0b100) {
		chosen_corb_size = 0b10;
		corb_size = 256;
	}
	else if (corb_cap & 0b10) {
		chosen_corb_size = 0b01;
		corb_size = 16;
	}
	else {
		corb_size = 2;
	}

	if (corb_cap != chosen_corb_size << 1) {
		corb_size_reg &= ~corbsize::SIZE;
		corb_size_reg |= corbsize::SIZE(chosen_corb_size);
		space.store(regs::CORBSIZE, corb_size_reg);
	}

	auto rirb_size_reg = space.load(regs::RIRBSIZE);
	auto rirb_cap = rirb_size_reg & rirbsize::SZCAP;

	uint8_t chosen_rirb_size = 0;
	if (rirb_cap & 0b100) {
		chosen_rirb_size = 0b10;
		rirb_size = 256;
	}
	else if (rirb_cap & 0b10) {
		chosen_rirb_size = 0b01;
		rirb_size = 16;
	}
	else {
		rirb_size = 2;
	}

	if (rirb_cap != chosen_rirb_size << 1) {
		rirb_size_reg &= ~rirbsize::SIZE;
		rirb_size_reg |= rirbsize::SIZE(chosen_rirb_size);
		space.store(regs::RIRBSIZE, rirb_size_reg);
	}

	space.store(regs::CORBLBASE, corb_phys);
	space.store(regs::CORBUBASE, corb_phys >> 32);
	space.store(regs::RIRBLBASE, rirb_phys);
	space.store(regs::RIRBUBASE, rirb_phys >> 32);

	auto corbctl = space.load(regs::CORBCTL);
	corbctl |= corbctl::RUN(true);
	space.store(regs::CORBCTL, corbctl);
	auto rirbctl = space.load(regs::RIRBCTL);
	rirbctl |= rirbctl::DMAEN(true);
	space.store(regs::RIRBCTL, rirbctl);

	auto rintcnt = space.load(regs::RINTCNT);
	space.store(regs::RINTCNT, (rintcnt & ~0xFF) | 255);

	in_stream_count = gcap & gcap::ISS;
	out_stream_count = gcap & gcap::OSS;

	for (uint32_t i = 0; i < in_stream_count; ++i) {
		in_streams[i].space = space.subspace(0x80 + i * 0x20);
		in_streams[i].index = i;
		in_stream_ptrs[i] = &in_streams[i];
	}

	for (uint32_t i = 0; i < out_stream_count; ++i) {
		out_streams[i].space = space.subspace(0x80 + in_stream_count * 0x20 + i * 0x20);
		out_streams[i].index = i;
		out_streams[i].output = true;
		out_stream_ptrs[i] = &out_streams[i];
	}

	// wait for codec initialization
	uhda_kernel_delay(1000);

	auto intctl = space.load(regs::INTCTL);
	intctl |= intctl::GIE(true);
	intctl |= intctl::SIE((1 << (in_stream_count + out_stream_count)) - 1);
	space.store(regs::INTCTL, intctl);

	auto statests = space.load(regs::STATESTS);
	for (uint32_t i = 0; i < 15; ++i) {
		if (statests & 1 << i) {
			auto* ptr = uhda_kernel_malloc(sizeof(UhdaCodec));
			if (!ptr) {
				return UHDA_STATUS_NO_MEMORY;
			}

			auto* codec = construct<UhdaCodec>(ptr, this, static_cast<uint8_t>(i));
			status = codec->init();
			if (status == UHDA_STATUS_TIMEOUT) {
				codec->~UhdaCodec();
				uhda_kernel_free(ptr, sizeof(UhdaCodec));
				continue;
			}
			else if (status != UHDA_STATUS_SUCCESS) {
				codec->~UhdaCodec();
				uhda_kernel_free(ptr, sizeof(UhdaCodec));
				return status;
			}

			if (!codecs.push(codec)) {
				return UHDA_STATUS_NO_MEMORY;
			}
		}
	}

	return UHDA_STATUS_SUCCESS;
}

uint8_t UhdaController::submit_verb(uint8_t cid, uint8_t nid, uint16_t cmd, uint8_t data) {
	uint8_t index = (space.load(regs::CORBWP) & corbwp::WP) + 1;

	VerbDescriptor verb {};
	verb.set_cid(cid);
	verb.set_nid(nid);
	verb.set_payload(cmd << 8 | data);
	corb[index] = verb;

	auto corbwp_reg = space.load(regs::CORBWP);
	corbwp_reg &= ~corbwp::WP;
	corbwp_reg |= corbwp::WP(index);
	space.store(regs::CORBWP, corbwp_reg);

	return index;
}

uint8_t UhdaController::submit_verb_long(uint8_t cid, uint8_t nid, uint8_t cmd, uint16_t data) {
	uint8_t index = (space.load(regs::CORBWP) & corbwp::WP) + 1;

	VerbDescriptor verb {};
	verb.set_cid(cid);
	verb.set_nid(nid);
	verb.set_payload(cmd << 16 | data);
	corb[index] = verb;

	auto corbwp_reg = space.load(regs::CORBWP);
	corbwp_reg &= ~corbwp::WP;
	corbwp_reg |= corbwp::WP(index);
	space.store(regs::CORBWP, corbwp_reg);

	return index;
}

UhdaStatus UhdaController::wait_for_verb(uint8_t index, uhda::ResponseDescriptor& res) {
	for (int i = 0;; ++i) {
		if (i == 5 * 2000) {
			return UHDA_STATUS_TIMEOUT;
		}

		if ((space.load(regs::CORBWP) & corbwp::WP) == index) {
			break;
		}
	}

	for (int i = 0;; ++i) {
		if (i == 5 * 2000) {
			return UHDA_STATUS_TIMEOUT;
		}

		if ((space.load(regs::RIRBWP) & rirbwp::WP) == index) {
			break;
		}
	}

	res = rirb[index];
	return UHDA_STATUS_SUCCESS;
}

UhdaStatus UhdaController::pci_setup() {
	uint16_t pci_cmd;
	auto status = pci_read_cmd(pci_device, pci_cmd);
	if (status != UHDA_STATUS_SUCCESS) {
		return status;
	}

	pci_cmd |= PCI_CMD_MEM_SPACE;
	pci_cmd |= PCI_CMD_BUS_MASTER;

	status = pci_write_cmd(pci_device, pci_cmd);
	if (status != UHDA_STATUS_SUCCESS) {
		return status;
	}

	return UHDA_STATUS_SUCCESS;
}

UhdaStatus UhdaController::map_bar() {
	for (uint32_t i = 0;; ++i) {
		uint32_t value;
		auto status = pci_read_bar(pci_device, i, value);
		if (status != UHDA_STATUS_SUCCESS) {
			return status;
		}

		if (value & 1) {
			if (i == 5) {
				return UHDA_STATUS_UNSUPPORTED;
			}
			continue;
		}

		void* space_ptr;
		status = uhda_kernel_pci_map_bar(pci_device, i, &space_ptr);
		if (status != UHDA_STATUS_SUCCESS) {
			return status;
		}

		bar = i;
		space.base = reinterpret_cast<uintptr_t>(space_ptr);
		return UHDA_STATUS_SUCCESS;
	}
}
