/**************************************************************************/
/*  ffmpeg_video_stream.h                                                 */
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

#ifndef FFMPEG_STREAM_INFO_H
#define FFMPEG_STREAM_INFO_H

#ifndef GDEXTENSION
#ifdef ENABLE_STREAM_INFO

#include "core/object/ref_counted.h"

class FileAccess;

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
}

class FFmpegStreamInfo : public RefCounted {
	GDCLASS(FFmpegStreamInfo, RefCounted);

public:
	enum VideoCodec {
		VIDEO_CODEC_H264,
		VIDEO_CODEC_H265,
		VIDEO_CODEC_VP9,
		VIDEO_CODEC_MAX,
		VIDEO_CODEC_UNKNOWN = -1
	};

	enum AudioCodec {
		AUDIO_CODEC_VORBIS,
		AUDIO_CODEC_AAC,
		AUDIO_CODEC_MAX,
		AUDIO_CODEC_UNKNOWN = -1
	};

private:
	Ref<FileAccess> current_file;

	AVIOContext *io_context = nullptr;
	AVFormatContext *format_context = nullptr;
	AVCodecContext *video_codec_context = nullptr;
	AVCodecContext *audio_codec_context = nullptr;

	static int64_t _stream_seek_callback(void *p_opaque, int64_t p_offset, int p_whence);
	static int _read_packet_callback(void *p_opaque, uint8_t *p_buf, int p_buf_size);

	struct VideoStream {
		VideoCodec video_codec;
	};

	struct AudioStream {
		AudioCodec audio_codec;
	};

	LocalVector<VideoStream> video_streams;
	LocalVector<AudioStream> audio_streams;

protected:
	static void _bind_methods();

public:
	int get_video_stream_count() const;
	VideoCodec get_video_stream_codec(int p_idx) const;
	AudioCodec get_audio_stream_codec(int p_idx) const;
	int get_audio_stream_count() const;
	int load(String p_path);
	static String video_codec_to_string(VideoCodec p_codec);
	static String audio_codec_to_string(AudioCodec p_codec);
};

VARIANT_ENUM_CAST(FFmpegStreamInfo::VideoCodec);
VARIANT_ENUM_CAST(FFmpegStreamInfo::AudioCodec);

#endif // FFMPEG_STREAM_INFO_H

#endif

#endif // FFMPEG_STREAM_INFO_H
