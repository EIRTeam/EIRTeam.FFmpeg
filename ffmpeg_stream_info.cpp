#ifndef GDEXTENSION
#include "core/error/error_macros.h"
#include "core/object/class_db.h"
extern "C" {
#include "libavcodec/codec_id.h"
#include "libavformat/avformat.h"
#include "libavformat/avio.h"
#include "libavutil/avutil.h"
};
#ifdef ENABLE_STREAM_INFO
#include "core/io/file_access.h"
#include "ffmpeg_stream_info.h"
#include "video_decoder.h"

int64_t FFmpegStreamInfo::_stream_seek_callback(void *p_opaque, int64_t p_offset, int p_whence) {
	FFmpegStreamInfo *decoder = (FFmpegStreamInfo *)p_opaque;
	switch (p_whence) {
		case SEEK_CUR: {
			decoder->current_file->seek(decoder->current_file->get_position() + p_offset);
		} break;
		case SEEK_SET: {
			decoder->current_file->seek(p_offset);
		} break;
		case SEEK_END: {
			decoder->current_file->seek_end(p_offset);
		} break;
		case AVSEEK_SIZE: {
			return decoder->current_file->get_length();
		} break;
		default: {
			return -1;
		} break;
	}
	return decoder->current_file->get_position();
}

int FFmpegStreamInfo::_read_packet_callback(void *p_opaque, uint8_t *p_buf, int p_buf_size) {
	FFmpegStreamInfo *decoder = (FFmpegStreamInfo *)p_opaque;
	uint64_t read_bytes = decoder->current_file->get_buffer(p_buf, p_buf_size);
	return read_bytes != 0 ? read_bytes : AVERROR_EOF;
}

void FFmpegStreamInfo::_bind_methods() {
	ClassDB::bind_method(D_METHOD("load", "path"), &FFmpegStreamInfo::load);
	ClassDB::bind_method(D_METHOD("get_video_stream_count"), &FFmpegStreamInfo::get_video_stream_count);
	ClassDB::bind_method(D_METHOD("get_audio_stream_count"), &FFmpegStreamInfo::get_audio_stream_count);
	ClassDB::bind_method(D_METHOD("get_video_stream_codec"), &FFmpegStreamInfo::get_video_stream_codec);
	ClassDB::bind_method(D_METHOD("get_audio_stream_codec"), &FFmpegStreamInfo::get_audio_stream_codec);
	ClassDB::bind_static_method("FFmpegStreamInfo", D_METHOD("video_codec_to_string"), &FFmpegStreamInfo::video_codec_to_string);
	ClassDB::bind_static_method("FFmpegStreamInfo", D_METHOD("audio_codec_to_string"), &FFmpegStreamInfo::audio_codec_to_string);

	BIND_ENUM_CONSTANT(VIDEO_CODEC_H264);
	BIND_ENUM_CONSTANT(VIDEO_CODEC_H265);
	BIND_ENUM_CONSTANT(VIDEO_CODEC_VP9);
	BIND_ENUM_CONSTANT(VIDEO_CODEC_UNKNOWN);

	BIND_ENUM_CONSTANT(AUDIO_CODEC_VORBIS);
	BIND_ENUM_CONSTANT(AUDIO_CODEC_AAC);
	BIND_ENUM_CONSTANT(AUDIO_CODEC_UNKNOWN);
}

int FFmpegStreamInfo::get_video_stream_count() const {
	return video_streams.size();
}

FFmpegStreamInfo::VideoCodec FFmpegStreamInfo::get_video_stream_codec(int p_idx) const {
	ERR_FAIL_UNSIGNED_INDEX_V(p_idx, video_streams.size(), VIDEO_CODEC_UNKNOWN);
	return video_streams[p_idx].video_codec;
}

FFmpegStreamInfo::AudioCodec FFmpegStreamInfo::get_audio_stream_codec(int p_idx) const {
	ERR_FAIL_UNSIGNED_INDEX_V(p_idx, audio_streams.size(), AUDIO_CODEC_UNKNOWN);
	return audio_streams[p_idx].audio_codec;
}

int FFmpegStreamInfo::get_audio_stream_count() const {
	return audio_streams.size();
}

FFmpegStreamInfo::VideoCodec _ffmpeg_to_video_codec(AVCodecID p_codec) {
	FFmpegStreamInfo::VideoCodec video_codec = FFmpegStreamInfo::VIDEO_CODEC_UNKNOWN;
	switch (p_codec) {
		case AV_CODEC_ID_H264: {
			video_codec = FFmpegStreamInfo::VIDEO_CODEC_H264;
		} break;
		case AV_CODEC_ID_H265: {
			video_codec = FFmpegStreamInfo::VIDEO_CODEC_H265;
		} break;
		case AV_CODEC_ID_VP9: {
			video_codec = FFmpegStreamInfo::VIDEO_CODEC_H265;
		} break;
		default: {
		} break;
	}
	return video_codec;
}

FFmpegStreamInfo::AudioCodec _ffmpeg_to_audio_codec(AVCodecID p_codec) {
	FFmpegStreamInfo::AudioCodec audio_codec = FFmpegStreamInfo::AUDIO_CODEC_UNKNOWN;
	switch (p_codec) {
		case AV_CODEC_ID_AAC: {
			audio_codec = FFmpegStreamInfo::AUDIO_CODEC_AAC;
		} break;
		case AV_CODEC_ID_VORBIS: {
			audio_codec = FFmpegStreamInfo::AUDIO_CODEC_VORBIS;
		} break;
		default: {
		} break;
	}
	return audio_codec;
}

int FFmpegStreamInfo::load(String p_path) {
	Error err;
	current_file = FileAccess::open(p_path, FileAccess::READ, &err);

	if (err != OK) {
		return err;
	}

	avio_seek(io_context, 0, SEEK_SET);
	if (!io_context) {
		const int context_buffer_size = 4096;
		unsigned char *context_buffer = (unsigned char *)av_malloc(context_buffer_size);
		io_context = avio_alloc_context(context_buffer, context_buffer_size, 0, this, &FFmpegStreamInfo::_read_packet_callback, nullptr, &FFmpegStreamInfo::_stream_seek_callback);
	}

	format_context = avformat_alloc_context();
	format_context->pb = io_context;
	format_context->flags |= AVFMT_FLAG_GENPTS;

	int open_input_res = avformat_open_input(&format_context, "dummy", nullptr, nullptr);
	bool input_opened = open_input_res >= 0;
	if (!input_opened) {
		ERR_PRINT(vformat("Error opening file or stream: %s", ffmpeg_get_error_message(open_input_res)));
		avformat_close_input(&format_context);
		av_free(io_context->buffer);
		avio_context_free(&io_context);
		avformat_free_context(format_context);
		return FAILED;
	}

	int find_stream_info_result = avformat_find_stream_info(format_context, nullptr);
	if (find_stream_info_result < 0) {
		ERR_PRINT(vformat("Error finding stream info: %s", ffmpeg_get_error_message(find_stream_info_result)));
		avformat_close_input(&format_context);
		av_free(io_context->buffer);
		avio_context_free(&io_context);
		avformat_free_context(format_context);
		return FAILED;
	}

	for (int i = 0; i < format_context->nb_streams; i++) {
		AVStream *stream = format_context->streams[i];
		if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			video_streams.push_back({
					.video_codec = _ffmpeg_to_video_codec(stream->codecpar->codec_id),
			});
		} else if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
			audio_streams.push_back({
					.audio_codec = _ffmpeg_to_audio_codec(stream->codecpar->codec_id),
			});
		}
	}

	avformat_close_input(&format_context);
	av_free(io_context->buffer);
	avio_context_free(&io_context);
	avformat_free_context(format_context);

	return OK;
}

String FFmpegStreamInfo::video_codec_to_string(VideoCodec p_codec) {
	String codec = "Invalid";
	switch (p_codec) {
		case VIDEO_CODEC_H264: {
			codec = "H264";
		} break;
		case VIDEO_CODEC_H265: {
			codec = "H265";
		} break;
		case VIDEO_CODEC_VP9: {
			codec = "VP9";
		} break;
		case VIDEO_CODEC_UNKNOWN: {
			codec = "Unknown";
		} break;
		default:
			break;
	}
	return codec;
}

String FFmpegStreamInfo::audio_codec_to_string(AudioCodec p_codec) {
	String codec = "Invalid";

	switch (p_codec) {
		case AUDIO_CODEC_VORBIS: {
			codec = "Vorbis";
		} break;
		case AUDIO_CODEC_AAC: {
			codec = "AAC";
		} break;
		case AUDIO_CODEC_UNKNOWN: {
			codec = "Unknown";
		} break;
		default:
			break;
	}
	return codec;
}

#endif // ENABLE_STREAM_INFO
#endif