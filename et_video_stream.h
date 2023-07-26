/**************************************************************************/
/*  et_video_stream.h                                                     */
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

#ifndef ET_VIDEO_STREAM_H
#define ET_VIDEO_STREAM_H

#ifdef GDEXTENSION

// Headers for building as GDExtension plug-in.
#include <godot_cpp/classes/video_stream.hpp>
#include <godot_cpp/classes/video_stream_playback.hpp>
#include <godot_cpp/godot.hpp>
#include <godot_cpp/templates/list.hpp>
#include <godot_cpp/templates/vector.hpp>

using namespace godot;

#else

#include "core/object/ref_counted.h"
#include "scene/resources/video_stream.h"

#endif

#include "video_decoder.h"

// We have to use this function redirection system for GDExtension because the naming conventions
// for the functions we are supposed to override are different there

#ifdef GDEXTENSION
#define STREAM_FUNCNAME(funcname) _##funcname
#else
#define STREAM_FUNCNAME(funcname) funcname
#endif

#define STREAM_FUNC_REDIRECT_0_(retval, funcname, const) \
	virtual retval STREAM_FUNCNAME(funcname)() const override { return funcname##_internal(); };

#define STREAM_FUNC_REDIRECT_1_(retval, funcname, const, arg0type, arg0name) \
	virtual retval STREAM_FUNCNAME(funcname)(arg0type arg0name) const override { return funcname##_internal(arg0name); };

#define STREAM_FUNC_REDIRECT_0(retval, funcname) STREAM_FUNC_REDIRECT_0_(retval, funcname, );
#define STREAM_FUNC_REDIRECT_0_CONST(retval, funcname) STREAM_FUNC_REDIRECT_0_(retval, funcname, const);

#define STREAM_FUNC_REDIRECT_1(retval, funcname, arg0type, arg0name) STREAM_FUNC_REDIRECT_1_(retval, funcname, , arg0type, arg0name);
#define STREAM_FUNC_REDIRECT_1_CONST(retval, funcname, arg0type, arg0name) STREAM_FUNC_REDIRECT_1_(retval, funcname, const, arg0type, arg0name);

class ETVideoStreamPlayback : public VideoStreamPlayback {
	GDCLASS(ETVideoStreamPlayback, VideoStreamPlayback);
	const int LENIENCE_BEFORE_SEEK = 2500;
	double playback_position = 0.0f;

	Ref<VideoDecoder> decoder;
	List<Ref<DecodedFrame>> available_frames;
	List<Ref<DecodedAudioFrame>> available_audio_frames;
	Ref<DecodedFrame> last_frame;
#ifndef FFMPEG_MT_GPU_UPLOAD
	Ref<ImageTexture> last_frame_texture;
#endif
	Ref<Image> last_frame_image;
	Ref<ImageTexture> texture;
	bool looping = false;
	bool buffering = false;
	int frames_processed = 0;
	void seek_into_sync();
	double get_current_frame_time();
	bool check_next_frame_valid(Ref<DecodedFrame> p_decoded_frame);
	bool check_next_audio_frame_valid(Ref<DecodedAudioFrame> p_decoded_frame);
	bool paused = false;
	bool playing = false;

private:
	bool is_paused_internal() const;
	void update_internal(double p_delta);
	bool is_playing_internal() const;
	void set_paused_internal(bool p_paused);
	void play_internal();
	void stop_internal();
	void seek_internal(double p_time);
	double get_length_internal() const;
	Ref<Texture2D> get_texture_internal() const;
	double get_playback_position_internal() const;
	int get_mix_rate_internal() const;
	int get_channels_internal() const;

protected:
	void clear();
	static void _bind_methods(){}; // Required by GDExtension, do not remove

public:
	void load(Ref<FileAccess> p_file_access);

	STREAM_FUNC_REDIRECT_0_CONST(bool, is_paused);
	STREAM_FUNC_REDIRECT_1(void, update, double, p_delta);
	STREAM_FUNC_REDIRECT_0_CONST(bool, is_playing);
	STREAM_FUNC_REDIRECT_1(void, set_paused, bool, p_paused);
	STREAM_FUNC_REDIRECT_0(void, play);
	STREAM_FUNC_REDIRECT_0(void, stop);
	STREAM_FUNC_REDIRECT_1(void, seek, double, p_time);
	STREAM_FUNC_REDIRECT_0_CONST(double, get_length);
	STREAM_FUNC_REDIRECT_0_CONST(Ref<Texture2D>, get_texture);
	STREAM_FUNC_REDIRECT_0_CONST(double, get_playback_position);
	STREAM_FUNC_REDIRECT_0_CONST(int, get_mix_rate);
	STREAM_FUNC_REDIRECT_0_CONST(int, get_channels);
	ETVideoStreamPlayback();
};

class ETVideoStream : public VideoStream {
	GDCLASS(ETVideoStream, VideoStream);

protected:
	static void _bind_methods(){}; // Required by GDExtension, do not remove
	Ref<VideoStreamPlayback> instantiate_playback_internal() {
		Ref<FileAccess> fa = FileAccess::open(get_file(), FileAccess::READ);
		if (!fa.is_valid()) {
			return Ref<VideoStreamPlayback>();
		}
		Ref<ETVideoStreamPlayback> pb;
		pb.instantiate();
		pb->load(fa);
		return pb;
	}

public:
	STREAM_FUNC_REDIRECT_0(Ref<VideoStreamPlayback>, instantiate_playback);
};

#endif // ET_VIDEO_STREAM_H
