/**************************************************************************/
/*  ffmpeg_frame.cpp                                                      */
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

#include "ffmpeg_frame.h"

#ifdef GDEXTENSION
#include <godot_cpp/variant/utility_functions.hpp>
#endif

void FFmpegFrame::_bind_methods() {
	ADD_SIGNAL(MethodInfo("return_frame", PropertyInfo(Variant::OBJECT, "frame", PROPERTY_HINT_RESOURCE_TYPE, "FFmpegFrame")));
}

AVFrame *FFmpegFrame::get_frame() const {
	return frame;
}

void FFmpegFrame::do_return() {
	emit_signal("return_frame", this);
}

FFmpegFrame::FFmpegFrame() {
	frame = av_frame_alloc();
}

FFmpegFrame::~FFmpegFrame() {
	av_frame_free(&frame);
}
