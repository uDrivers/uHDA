#pragma once

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef UHDA_PCI_DEVICE
#define UHDA_PCI_DEVICE(vendor_id, device_id) \
	{(vendor_id), (device_id)}
#endif

/*
 * A table containing all special PCI devices not matched by the class match that uHDA supports.
 */
#define UHDA_MATCHING_DEVICES \
	/* Intel Tiger Lake Smart Sound Technology */ \
	UHDA_PCI_DEVICE(0x8086, 0xA0C8)

/*
 * PCI class/subclass pair that uHDA most likely supports.
 */
#define UHDA_MATCHING_CLASS 4
#define UHDA_MATCHING_SUBCLASS 3

/*
 * Checks whether a PCI vendor/device pair is supported by uHDA.
 */
bool uhda_device_matches(uint16_t vendor, uint16_t device);

/*
 * Checks whether a PCI class/subclass pair is supported by uHDA.
 */
bool uhda_class_matches(uint16_t clazz, uint16_t subclass);

/*
 * Initializes HDA for the PCI device.
 */
UhdaStatus uhda_init(void* pci_device, UhdaController** res);

/*
 * Destroys a previously initialized HDA controller instance.
 *
 * Note: the controller instance is destroyed even if the returned status is not success
 * so it's not safe to use it anymore.
 * However, it may still be possible to completely re-initialize the controller using
 * `uhda_init` depending on what caused the destruction to fail.
 */
UhdaStatus uhda_destroy(UhdaController* controller);

/*
 * Prepares the HDA controller for system suspend (e.g. ACPI S3).
 *
 * Note: it is not safe to use the controller until `uhda_resume` is called
 * even if the returned status is not success.
 */
UhdaStatus uhda_suspend(UhdaController* controller);

/*
 * Resumes the HDA controller after system suspend.
 *
 * Note: uHDA expects the kernel to restore the PCI BARs before calling this function.
 * Note: it is safe to call this function multiple times in case it fails.
 */
UhdaStatus uhda_resume(UhdaController* controller);

/*
 * Gets a list of HDA codecs attached to the controller.
 */
void uhda_get_codecs(UhdaController* controller, const UhdaCodec* const** codecs, size_t* codec_count);

/*
 * Gets a list of HDA output streams.
 */
void uhda_get_output_streams(UhdaController* controller, UhdaStream*** streams, size_t* stream_count);

/*
 * Gets a list of output groups that a codec has.
 */
void uhda_codec_get_output_groups(
	const UhdaCodec* codec,
	const UhdaOutputGroup* const** output_groups,
	size_t* output_group_count);

/*
 * Gets a list of outputs from an output group.
 */
void uhda_output_group_get_outputs(
	const UhdaOutputGroup* output_group,
	const UhdaOutput* const** outputs,
	size_t* output_count);

/*
 * Gets info about a HDA output.
 */
UhdaOutputInfo uhda_output_get_info(const UhdaOutput* output);

/*
 * Checks whether the provided paths are usable at the same time,
 * either for playing the same stream on all of them or different ones.
 */
bool uhda_paths_usable_simultaneously(const UhdaPath** paths, size_t count, bool same_stream);

/*
 * Finds a path to the output that is usable at the same time as the other provided paths if provided.
 *
 * `same_stream` means that all the streams are going to be playing the same stream.
 */
UhdaStatus uhda_find_path(
	const UhdaOutput* dest,
	const UhdaPath** other_paths,
	size_t other_path_count,
	bool same_stream,
	UhdaPath** res);

/*
 * Sets up a path for playback.
 *
 * `params` contains hints for the stream parameters,
 * it's also updated to reflect the actual parameters.
 */
UhdaStatus uhda_path_setup(UhdaPath* path, UhdaStreamParams* params, UhdaStream* stream);

/*
 * Shuts down an already set up path.
 */
UhdaStatus uhda_path_shutdown(UhdaPath* path);

/*
 * Sets the volume on the path to `volume` (0-100)
 */
UhdaStatus uhda_path_set_volume(UhdaPath* path, int volume);

/*
 * Mutes/unmutes the path.
 */
UhdaStatus uhda_path_mute(UhdaPath* path, bool mute);

/*
 * Sets up a stream for playback.
 *
 * `ring_buffer_size` is the size of an internal ring buffer that is used to queue output.
 * `buffer_fill_fn` is an optional callback that is called when the ring buffer has space,
 * it is called from an interrupt context.
 */
UhdaStatus uhda_stream_setup(
	UhdaStream* stream,
	const UhdaStreamParams* params,
	uint32_t ring_buffer_size,
	UhdaBufferFillFn buffer_fill_fn,
	void* arg);

/*
 * Shuts down an already set up stream.
 *
 * Note: the stream must be stopped prior to shutting it down.
 */
UhdaStatus uhda_stream_shutdown(UhdaStream* stream);

/*
 * Begins/stops playback on the stream.
 */
UhdaStatus uhda_stream_play(UhdaStream* stream, bool play);

/*
 * Queues data to the stream and returns the actual amount of data written in `size`.
 *
 * Note: this function is asynchronous, it doesn't block if the ring buffer space is exhausted.
 */
UhdaStatus uhda_stream_queue_data(UhdaStream* stream, const void* data, uint32_t* size);

#ifdef __cplusplus
}
#endif
