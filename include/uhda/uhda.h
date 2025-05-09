#pragma once

#include "types.h"

/*
 * Concepts:
 * ------------------------------------------------------------------------
 * Controller
 *  A HDA controller sending data to the attached codecs.
 *
 * Codec
 *  The actual audio processing unit, usually there is only one,
 *  but it is possible to have multiple connected to the same Controller.
 *
 * Stream
 *  A stream of audio data either going from the controller to the codecs or
 *  from the codecs to the controller (in case of input).
 *
 * Output group
 *  A group of logically-related outputs sorted by their sequence,
 *  used for multichannel output (likely surround) or just
 *  grouping related outputs together (e.g. jacks on the back of a motherboard).
 *  It may also contain different outputs that go to the same physical output,
 *  just with different connections.
 *
 * Output
 *  A physical output, for an example a speaker or a headphone jack.
 *
 * Path
 *  A path from the codec to an input/output.
 *  In case of an output the audio is picked up from a stream by the other end of the path,
 *  it then traverses through the path finally coming out of the output.
 *  The case of input is the inverse of that, the audio comes from the input
 *  and is picked up by the stream on the other end of the path.
 */

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
 * Gets presence info of an output if available.
 */
UhdaStatus uhda_output_get_presence(const UhdaOutput* output, bool* presence);

/*
 * Gets info about an output.
 */
UhdaOutputInfo uhda_output_get_info(const UhdaOutput* output);

/*
 * A table mapping colors to strings.
 */
extern const char* UHDA_OUTPUT_COLOR_STRINGS[UHDA_COLOR_OTHER + 1];

/*
 * A table mapping locations to strings.
 */
extern const char* UHDA_LOCATION_STRINGS[UHDA_LOCATION_UNKNOWN + 1];

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
 * `buffer_fill_fn` is an optional callback that is called when the ring buffer has space
 * and more data is needed.
 * `buffer_trip_threshold` is the trip threshold in bytes for the optional buffer trip function,
 * called once per data period when the buffer size is below the specified threshold.
 *
 * Note: all callback functions are run in an interrupt context.
 */
UhdaStatus uhda_stream_setup(
	UhdaStream* stream,
	const UhdaStreamParams* params,
	uint32_t ring_buffer_size,
	UhdaBufferFillFn buffer_fill_fn,
	void* buffer_fill_arg,
	uint32_t buffer_trip_threshold,
	UhdaBufferTripFn buffer_trip_fn,
	void* buffer_trip_arg);

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

/*
 * Clears all currently queued data from the stream.
 */
UhdaStatus uhda_stream_clear_queue(UhdaStream* stream);

/*
 * Gets the status of a stream.
 */
UhdaStreamStatus uhda_stream_get_status(const UhdaStream* stream);

/*
 * Gets the amount of remaining queued data within a stream.
 */
UhdaStatus uhda_stream_get_remaining(const UhdaStream* stream, uint32_t* remaining);

/*
 * Gets the size of the stream's ring buffer chosen in uhda_stream_setup.
 */
uint32_t uhda_stream_get_buffer_size(const UhdaStream* stream);

#ifdef __cplusplus
}
#endif
