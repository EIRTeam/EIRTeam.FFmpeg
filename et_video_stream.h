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

#include "core/object/ref_counted.h"
#include "core/templates/local_vector.h"
#include "ffmpeg_frame.h"
#include "scene/resources/video_stream.h"
#include "video_decoder.h"

class ETVideoStreamPlayback : public VideoStreamPlayback {
	GDCLASS(ETVideoStreamPlayback, VideoStreamPlayback);
	const int LENIENCE_BEFORE_SEEK = 2500;
	double playback_position = 0.0f;

	Ref<VideoDecoder> decoder;
	List<Ref<DecodedFrame>> available_frames;
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
	bool paused = false;
	bool playing = false;

protected:
	void clear();

public:
	void update(double p_delta) override;
	void load(Ref<FileAccess> p_file_access);
	virtual bool is_paused() const override;
	virtual bool is_playing() const override;
	virtual void set_paused(bool p_paused) override;
	virtual void play() override;
	virtual void stop() override;
	virtual void seek(double p_time) override;
	virtual double get_length() const override;
	virtual Ref<Texture2D> get_texture() const override;
	virtual double get_playback_position() const override;
	ETVideoStreamPlayback();
};

class ETVideoStream : public VideoStream {
	GDCLASS(ETVideoStream, VideoStream);

public:
	virtual Ref<VideoStreamPlayback> instantiate_playback() override {
		Ref<ETVideoStreamPlayback> pb;
		pb.instantiate();
		Ref<FileAccess> fa = FileAccess::open(file, FileAccess::READ);
		pb->load(fa);
		return pb;
	}
};

#endif // ET_VIDEO_STREAM_H
