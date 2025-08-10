/**************************************************************************/
/*  video_decoder.h                                                       */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             EIRTeam.FFmpeg                             */
/*                         https://ph.eirteam.moe                         */
/**************************************************************************/
/* Copyright (c) 2023-present Álex Román (EIRTeam) & contributors.        */
/*                                                                        */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#ifndef VIDEO_DECODER_H
#define VIDEO_DECODER_H
// Osu inspired ffmpeg decoding code

#include "gdextension_build/sync_compat.h"

#ifdef GDEXTENSION

// Headers for building as GDExtension plug-in.
#include "gdextension_build/command_queue_mt.h"
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/mutex.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/core/mutex_lock.hpp>
#include <godot_cpp/godot.hpp>
#include <godot_cpp/templates/list.hpp>

using namespace godot;

#else

#include "core/io/file_access.h"
#include "core/templates/command_queue_mt.h"
#include "scene/resources/image_texture.h"

#endif

#include "ffmpeg_codec.h"
#include "ffmpeg_frame.h"
extern "C" {
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
}

#include <thread>

String ffmpeg_get_error_message(int p_error_code);

enum FFmpegFrameFormat {
	RGBA8,
	YUV420P,
	YUVA420P,
};

class DecodedFrame : public RefCounted {
	double time;
	Ref<ImageTexture> texture;
	Ref<Image> image;
	Ref<Image> yuv_images[4];
	FFmpegFrameFormat format;

public:
	Ref<ImageTexture> get_texture() const;
	void set_texture(const Ref<ImageTexture> &p_texture);
	Ref<Image> get_image() const { return image; };

	double get_time() const;
	void set_time(double p_time);

	void set_yuv_image_plane(int p_plane_idx, Ref<Image> p_image);
	Ref<Image> get_yuv_image_plane(int p_plane_idx) const;

	DecodedFrame(double p_time, Ref<ImageTexture> p_texture);
	DecodedFrame(double p_time, Ref<Image> p_image);

	FFmpegFrameFormat get_format() const { return format; }
	void set_format(const FFmpegFrameFormat &p_format) { format = p_format; }
};

class DecodedAudioFrame : public RefCounted {
	double time;

public:
	PackedFloat32Array sample_data;
	double get_time() const;
	PackedFloat32Array get_sample_data() const;
	DecodedAudioFrame(double p_time) { time = p_time; };
};

class VideoDecoder : public RefCounted {
public:
	enum HardwareVideoDecoder {
		NONE = 0,
		NVDEC = 1,
		INTEL_QUICK_SYNC = 2,
		DXVA2 = 4,
		VDPAU = 8,
		VAAPI = 16,
		ANDROID_MEDIACODEC = 32,
		APPLE_VIDEOTOOLBOX = 64,
		ANY = INT_MAX,
	};
	enum DecoderState {
		READY,
		RUNNING,
		FAULTED,
		END_OF_STREAM,
		STOPPED
	};

private:
	FFmpegFrameFormat frame_format;
	Vector<Ref<DecodedAudioFrame>> decoded_audio_frames;

	Ref<CoreBind::Mutex> audio_buffer_mutex;

	SwsContext *sws_context = nullptr;
	SwrContext *swr_context = nullptr;
	DecoderState decoder_state = DecoderState::READY;
	mutable CommandQueueMT decoder_commands;
	AVStream *video_stream = nullptr;
	AVStream *audio_stream = nullptr;
	AVIOContext *io_context = nullptr;
	AVFormatContext *format_context = nullptr;
	AVCodecContext *video_codec_context = nullptr;
	AVCodecContext *audio_codec_context = nullptr;
	bool input_opened = false;
	bool has_audio = false;
	bool hw_decoding_allowed = false;
	double video_time_base_in_seconds;
	double audio_time_base_in_seconds;
	double duration;
	double skip_output_until_time = -1.0;
	SafeFlag skip_current_outputs;
	SafeNumeric<float> last_decoded_frame_time;
	Ref<FileAccess> video_file;
	BitField<HardwareVideoDecoder> target_hw_video_decoders = HardwareVideoDecoder::ANY;
	Ref<CoreBind::Mutex> available_textures_mutex;
	List<Ref<ImageTexture>> available_textures;
	Ref<CoreBind::Mutex> hw_transfer_frames_mutex;
	List<Ref<FFmpegFrame>> hw_transfer_frames;
	Ref<CoreBind::Mutex> scaler_frames_mutex;
	List<Ref<FFmpegFrame>> scaler_frames;
	Ref<CoreBind::Mutex> decoded_frames_mutex;
	Vector<Ref<DecodedFrame>> decoded_frames;
	std::thread *thread = nullptr;
	SafeFlag thread_abort;
	AVCodec const *forced_video_codec = nullptr;

	bool looping = false;

	static int _read_packet_callback(void *p_opaque, uint8_t *p_buf, int p_buf_size);
	static int64_t _stream_seek_callback(void *p_opaque, int64_t p_offset, int p_whence);
	void prepare_decoding();
	Error recreate_codec_context();
	static HardwareVideoDecoder from_av_hw_device_type(AVHWDeviceType p_device_type);

	void _seek_command(double p_target_timestamp);
	static void _thread_func(void *userdata);
	void _decode_next_frame(AVPacket *p_packet, AVFrame *p_receive_frame);
	int _send_packet(AVCodecContext *p_codec_context, AVFrame *p_receive_frame, AVPacket *p_packet);
	void _try_disable_hw_decoding(int p_error_code);
	void _read_decoded_frames(AVFrame *p_received_frame);
	void _read_decoded_audio_frames(AVFrame *p_received_frame);

	void _hw_transfer_frame_return(Ref<FFmpegFrame> p_hw_frame);
	void _scaler_frame_return(Ref<FFmpegFrame> p_hw_frame);

	Ref<FFmpegFrame> _ensure_frame_pixel_format(Ref<FFmpegFrame> p_frame, AVPixelFormat p_target_pixel_format);
	Ref<DecodedFrame> _unwrap_yuv_frame(double p_frame_time, Ref<FFmpegFrame> p_frame, FFmpegFrameFormat p_out_format);
	AVFrame *_ensure_frame_audio_format(AVFrame *p_frame, AVSampleFormat p_target_audio_format);
	String _codec_id_to_libvpx(AVCodecID p_codec_id) const;

public:
	struct AvailableDecoderInfo {
		Ref<FFmpegCodec> codec;
		AVHWDeviceType device_type;
	};
	void seek(double p_time, bool p_wait = false);
	void start_decoding();
	Vector<AvailableDecoderInfo> get_available_video_decoders(const AVInputFormat *p_format, AVCodecID p_codec_id, BitField<HardwareVideoDecoder> p_target_decoders);
	void return_frames(Vector<Ref<DecodedFrame>> p_frames);
	void return_frame(Ref<DecodedFrame> p_frame);
	Vector<Ref<DecodedFrame>> get_decoded_frames();
	Vector<Ref<DecodedAudioFrame>> get_decoded_audio_frames();
	DecoderState get_decoder_state() const;
	double get_last_decoded_frame_time() const;
	bool is_running() const;
	double get_duration() const;
	Vector2i get_size() const;
	int get_audio_mix_rate() const;
	int get_audio_channel_count() const;
	FFmpegFrameFormat get_frame_format() const { return frame_format; }

	VideoDecoder(Ref<FileAccess> p_file);
	~VideoDecoder();
};

#endif // VIDEO_DECODER_H
