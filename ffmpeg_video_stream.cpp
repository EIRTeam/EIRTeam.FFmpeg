/**************************************************************************/
/*  ffmpeg_video_stream.cpp                                               */
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

#include "ffmpeg_video_stream.h"

#ifdef GDEXTENSION
#include "gdextension_build/gdex_print.h"
#endif

#include "tracy_import.h"

void FFmpegVideoStreamPlayback::seek_into_sync() {
	decoder->seek(playback_position);
	Vector<Ref<DecodedFrame>> decoded_frames;
	for (Ref<DecodedFrame> df : available_frames) {
		decoded_frames.push_back(df);
	}
	decoder->return_frames(decoded_frames);
	available_frames.clear();
	available_audio_frames.clear();
}

double FFmpegVideoStreamPlayback::get_current_frame_time() {
	if (last_frame.is_valid()) {
		return last_frame->get_time();
	}
	return 0.0f;
}

bool FFmpegVideoStreamPlayback::check_next_frame_valid(Ref<DecodedFrame> p_decoded_frame) {
	// in the case of looping, we may start a seek back to the beginning but still receive some lingering frames from the end of the last loop. these should be allowed to continue playing.
	if (looping && Math::abs((p_decoded_frame->get_time() - decoder->get_duration()) - playback_position) < LENIENCE_BEFORE_SEEK)
		return true;

	return p_decoded_frame->get_time() <= playback_position && Math::abs(p_decoded_frame->get_time() - playback_position) < LENIENCE_BEFORE_SEEK;
}

bool FFmpegVideoStreamPlayback::check_next_audio_frame_valid(Ref<DecodedAudioFrame> p_decoded_frame) {
	// in the case of looping, we may start a seek back to the beginning but still receive some lingering frames from the end of the last loop. these should be allowed to continue playing.
	if (looping && Math::abs((p_decoded_frame->get_time() - decoder->get_duration()) - playback_position) < LENIENCE_BEFORE_SEEK)
		return true;

	return p_decoded_frame->get_time() <= playback_position && Math::abs(p_decoded_frame->get_time() - playback_position) < LENIENCE_BEFORE_SEEK;
}

const char *const upd_str = "update_internal";

void FFmpegVideoStreamPlayback::update_internal(double p_delta) {
	ZoneScopedN("update_internal");

	if (paused || !playing) {
		return;
	}

	playback_position += p_delta * 1000.0f;

	if (decoder->get_decoder_state() == VideoDecoder::DecoderState::END_OF_STREAM && available_frames.size() == 0) {
		// if at the end of the stream but our playback enters a valid time region again, a seek operation is required to get the decoder back on track.
		if (playback_position < decoder->get_last_decoded_frame_time()) {
			seek_into_sync();
		} else {
			playing = false;
		}
	}

	Ref<DecodedFrame> peek_frame = available_frames.size() > 0 ? available_frames[0] : nullptr;
	bool out_of_sync = false;

	if (peek_frame.is_valid()) {
		out_of_sync = Math::abs(playback_position - peek_frame->get_time()) > LENIENCE_BEFORE_SEEK;

		if (looping) {
			out_of_sync &= Math::abs(playback_position - decoder->get_duration() - peek_frame->get_time()) > LENIENCE_BEFORE_SEEK &&
					Math::abs(playback_position + decoder->get_duration() - peek_frame->get_time()) > LENIENCE_BEFORE_SEEK;
		}
	}

	if (out_of_sync) {
		print_line(vformat("Video too far out of sync (%.2f), seeking to %.2f", peek_frame->get_time(), playback_position));
		seek_into_sync();
	}

	double frame_time = get_current_frame_time();

	bool got_new_frame = false;

	while (available_frames.size() > 0 && check_next_frame_valid(available_frames[0])) {
		ZoneNamedN(__frame_receive, "frame_receive", true);

		if (last_frame.is_valid()) {
			decoder->return_frame(last_frame);
		}
		last_frame = available_frames[0];
		last_frame_image = last_frame->get_image();
#ifdef FFMPEG_MT_GPU_UPLOAD
		last_frame_texture = last_frame->get_texture();
#endif
		available_frames.pop_front();
		got_new_frame = true;
	}
#ifndef FFMPEG_MT_GPU_UPLOAD
	if (got_new_frame) {
		if (texture.is_valid()) {
			if (texture->get_size() != last_frame_image->get_size() || texture->get_format() != last_frame_image->get_format()) {
				ZoneNamedN(__img_upate_slow, "Image update slow", true);
				texture->set_image(last_frame_image); // should never happen, but life has many doors ed-boy...
			} else {
				ZoneNamedN(__img_upate_fast, "Image update fast", true);
				texture->update(last_frame_image);
			}
		}
	}
#endif

	if (available_frames.size() == 0) {
		for (Ref<DecodedFrame> frame : decoder->get_decoded_frames()) {
			available_frames.push_back(frame);
		}
	}

	Ref<DecodedAudioFrame> peek_audio_frame;
	if (available_audio_frames.size() > 0) {
		peek_audio_frame = available_audio_frames[0];
	}

	bool audio_out_of_sync = false;

	if (peek_audio_frame.is_valid()) {
		audio_out_of_sync = Math::abs(playback_position - peek_audio_frame->get_time()) > LENIENCE_BEFORE_SEEK;

		if (looping) {
			out_of_sync &= Math::abs(playback_position - decoder->get_duration() - peek_audio_frame->get_time()) > LENIENCE_BEFORE_SEEK &&
					Math::abs(playback_position + decoder->get_duration() - peek_audio_frame->get_time()) > LENIENCE_BEFORE_SEEK;
		}
	}

	if (audio_out_of_sync) {
		// TODO: seek audio stream individually if it desyncs
	}

	while (available_audio_frames.size() > 0 && check_next_audio_frame_valid(available_audio_frames[0])) {
		ZoneNamedN(__audio_mix, "Audio mix", true);
		Ref<DecodedAudioFrame> audio_frame = available_audio_frames[0];
		int sample_count = audio_frame->get_sample_data().size() / decoder->get_audio_channel_count();
#ifdef GDEXTENSION
		mix_audio(sample_count, audio_frame->get_sample_data(), 0);
#else
		mix_callback(mix_udata, audio_frame->get_sample_data().ptr(), sample_count);
#endif
		available_audio_frames.pop_front();
	}
	if (available_audio_frames.size() == 0) {
		for (Ref<DecodedAudioFrame> frame : decoder->get_decoded_audio_frames()) {
			available_audio_frames.push_back(frame);
		}
	}

	buffering = decoder->is_running() && available_frames.size() == 0;

	if (frame_time != get_current_frame_time()) {
		frames_processed++;
	}
}

void FFmpegVideoStreamPlayback::load(Ref<FileAccess> p_file_access) {
	decoder = Ref<VideoDecoder>(memnew(VideoDecoder(p_file_access)));

	decoder->start_decoding();
	Vector2i size = decoder->get_size();
	if (decoder->get_decoder_state() != VideoDecoder::FAULTED) {
#ifdef GDEXTENSION
		texture = ImageTexture::create_from_image(Image::create(size.x, size.y, false, Image::FORMAT_RGBA8));
#else
		texture = ImageTexture::create_from_image(Image::create_empty(size.x, size.y, false, Image::FORMAT_RGBA8));
#endif
	}
}

bool FFmpegVideoStreamPlayback::is_paused_internal() const {
	return paused;
}

bool FFmpegVideoStreamPlayback::is_playing_internal() const {
	return playing;
}

void FFmpegVideoStreamPlayback::set_paused_internal(bool p_paused) {
	paused = p_paused;
}

void FFmpegVideoStreamPlayback::play_internal() {
	if (decoder->get_decoder_state() == VideoDecoder::FAULTED) {
		playing = false;
		return;
	}
	clear();
	playback_position = 0;
	decoder->seek(0, true);
	playing = true;
}

void FFmpegVideoStreamPlayback::stop_internal() {
	if (playing) {
		clear();
		playback_position = 0.0f;
		decoder->seek(playback_position, true);
	}
	playing = false;
}

void FFmpegVideoStreamPlayback::seek_internal(double p_time) {
	decoder->seek(p_time * 1000.0f);
	available_frames.clear();
	available_audio_frames.clear();
	playback_position = p_time * 1000.0f;
}

double FFmpegVideoStreamPlayback::get_length_internal() const {
	return decoder->get_duration() / 1000.0f;
}

Ref<Texture2D> FFmpegVideoStreamPlayback::get_texture_internal() const {
#ifdef FFMPEG_MT_GPU_UPLOAD
	return last_frame_texture;
#else
	return texture;
#endif
}

double FFmpegVideoStreamPlayback::get_playback_position_internal() const {
	return playback_position / 1000.0;
}

int FFmpegVideoStreamPlayback::get_mix_rate_internal() const {
	return decoder->get_audio_mix_rate();
}

int FFmpegVideoStreamPlayback::get_channels_internal() const {
	return decoder->get_audio_channel_count();
}

FFmpegVideoStreamPlayback::FFmpegVideoStreamPlayback() {
}

void FFmpegVideoStreamPlayback::clear() {
	last_frame.unref();
	last_frame_texture.unref();
	available_frames.clear();
	available_audio_frames.clear();
	frames_processed = 0;
	playing = false;
}
