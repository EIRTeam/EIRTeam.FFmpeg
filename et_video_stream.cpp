/**************************************************************************/
/*  et_video_stream.cpp                                                   */
/**************************************************************************/
/*                         This file is part of:                          */
/*                           EIRTeam.Steamworks                           */
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

#include "et_video_stream.h"

#ifdef GDEXTENSION
#include "gdextension_build/gdex_print.h"
#endif

void ETVideoStreamPlayback::seek_into_sync() {
	decoder->seek(playback_position);
	Vector<Ref<DecodedFrame>> decoded_frames;
	for (Ref<DecodedFrame> df : available_frames) {
		decoded_frames.push_back(df);
	}
	decoder->return_frames(decoded_frames);
	available_frames.clear();
}

double ETVideoStreamPlayback::get_current_frame_time() {
	if (last_frame.is_valid()) {
		return last_frame->get_time();
	}
	return 0.0f;
}

bool ETVideoStreamPlayback::check_next_frame_valid(Ref<DecodedFrame> p_decoded_frame) {
	// in the case of looping, we may start a seek back to the beginning but still receive some lingering frames from the end of the last loop. these should be allowed to continue playing.
	if (looping && Math::abs((p_decoded_frame->get_time() - decoder->get_duration()) - playback_position) < LENIENCE_BEFORE_SEEK)
		return true;

	return p_decoded_frame->get_time() <= playback_position && Math::abs(p_decoded_frame->get_time() - playback_position) < LENIENCE_BEFORE_SEEK;
}

void ETVideoStreamPlayback::update_internal(double p_delta) {
	if (paused || !playing) {
		return;
	}
	playback_position += p_delta * 1000.0f;

	if (decoder->get_decoder_state() == VideoDecoder::DecoderState::END_OF_STREAM && available_frames.size() == 0) {
		// if at the end of the stream but our playback enters a valid time region again, a seek operation is required to get the decoder back on track.
		if (playback_position < decoder->get_last_decoded_frame_time()) {
			seek_into_sync();
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
				texture->set_image(last_frame_image); // should never happen, but life has many doors ed-boy...
			} else {
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

	PackedFloat32Array audio_frames = decoder->get_decoded_audio_frames();

#ifdef GDEXTENSION
	if (audio_frames.size() > 0) {
		mix_audio(audio_frames.size() / decoder->get_audio_channel_count(), audio_frames, 0);
	}
#else
	if (mix_callback && audio_frames.size() > 0) {
		mix_callback(mix_udata, audio_frames.ptr(), audio_frames.size() / decoder->get_audio_channel_count());
	}
#endif

	buffering = decoder->is_running() && available_frames.size() == 0;

	if (frame_time != get_current_frame_time()) {
		frames_processed++;
	}
}

void ETVideoStreamPlayback::load(Ref<FileAccess> p_file_access) {
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

bool ETVideoStreamPlayback::is_paused_internal() const {
	return paused;
}

bool ETVideoStreamPlayback::is_playing_internal() const {
	return playing;
}

void ETVideoStreamPlayback::set_paused_internal(bool p_paused) {
	paused = p_paused;
}

void ETVideoStreamPlayback::play_internal() {
	if (!playing) {
		playback_position = 0;
	} else {
		stop_internal();
	}
	playing = true;
}

void ETVideoStreamPlayback::stop_internal() {
	if (playing) {
		clear();
		seek_internal(0);
	}
	playing = false;
}

void ETVideoStreamPlayback::seek_internal(double p_time) {
	decoder->seek(p_time);
}

double ETVideoStreamPlayback::get_length_internal() const {
	return decoder->get_duration();
}

Ref<Texture2D> ETVideoStreamPlayback::get_texture_internal() const {
#ifdef FFMPEG_MT_GPU_UPLOAD
	return last_frame_texture;
#else
	return texture;
#endif
}

double ETVideoStreamPlayback::get_playback_position_internal() const {
	return playback_position;
}

int ETVideoStreamPlayback::get_mix_rate_internal() const {
	return decoder->get_audio_mix_rate();
}

int ETVideoStreamPlayback::get_channels_internal() const {
	return decoder->get_audio_channel_count();
}

ETVideoStreamPlayback::ETVideoStreamPlayback() {
}

void ETVideoStreamPlayback::clear() {
	last_frame.unref();
	last_frame_texture.unref();
	available_frames.clear();
	frames_processed = 0;
	playing = false;
}
