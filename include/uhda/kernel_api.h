#pragma once

#include "types.h"

/*
 * Reads `size` bytes from the PCI configuration space at offset `offset`.
 */
UhdaStatus uhda_kernel_pci_read(void* pci_device, uint8_t offset, uint8_t size, uint32_t* res);

/*
 * Writes `size` bytes of `value` to the PCI configuration space at offset `offset`
 */
UhdaStatus uhda_kernel_pci_write(void* pci_device, uint8_t offset, uint8_t size, uint32_t value);

/*
 * Allocates a PCI irq for the device and sets `fn` as the callback to be called when the irq occurs.
 *
 * HDA only needs one irq, so this is guaranteed to be called exactly once for each device.
 * `opaque_irq` can be used to store a handle to the interrupt.
 */
UhdaStatus uhda_kernel_pci_allocate_irq(void* pci_device, UhdaIrqHandlerFn fn, void* arg, void** opaque_irq);

/*
 * Deallocates a previously allocated PCI irq.
 *
 * Guaranteed to be called only after the irq has been disabled.
 */
void uhda_kernel_pci_deallocate_irq(void* pci_device, void* opaque_irq);

/*
 * Enables or disables a previously allocated PCI irq for the device.
 */
void uhda_kernel_pci_enable_irq(void* pci_device, void* opaque_irq, bool enable);

/*
 * Maps a PCI BAR to virtual memory.
 */
UhdaStatus uhda_kernel_pci_map_bar(void* pci_device, uint32_t bar, void** virt);

/*
 * Unmaps a previously mapped PCI BAR.
 */
void uhda_kernel_pci_unmap_bar(void* pci_device, uint32_t bar, void* virt);

/*
 * Allocates `size` bytes of memory.
 */
void* uhda_kernel_malloc(size_t size);

/*
 * Frees previously allocated memory.
 */
void uhda_kernel_free(void* ptr, size_t size);

/*
 * Busy-waits for the specified number of microseconds.
 */
void uhda_kernel_delay(uint32_t microseconds);

/*
 * Logs a message.
 */
void uhda_kernel_log(const char* str);

/*
 * Allocates `size` bytes of aligned contiguous physical memory.
 *
 * Note: align is always 0x1000.
 */
UhdaStatus uhda_kernel_allocate_physical(size_t size, uintptr_t* res);

/*
 * Deallocates previously allocated contiguous physical memory.
 */
void uhda_kernel_deallocate_physical(uintptr_t phys, size_t size);

/*
 * Maps `size` bytes starting at physical address `phys` in virtual memory as uncacheable.
 */
UhdaStatus uhda_kernel_map(uintptr_t phys, size_t size, void** virt);

/*
 * Unmaps previously mapped memory.
 */
void uhda_kernel_unmap(void* virt, size_t size);
