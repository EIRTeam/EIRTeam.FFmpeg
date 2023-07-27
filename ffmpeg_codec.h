/**************************************************************************/
/*  ffmpeg_codec.h                                                        */
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

#ifndef FFMPEG_CODEC_H
#define FFMPEG_CODEC_H
extern "C" {
#include "libavcodec/avcodec.h"
}

#ifdef GDEXTENSION

// Headers for building as GDExtension plug-in.
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/godot.hpp>
#include <godot_cpp/templates/vector.hpp>

using namespace godot;

#else

#include "core/object/ref_counted.h"
#include "core/templates/vector.h"

#endif

class FFmpegCodec : public RefCounted {
	const AVCodec *codec = nullptr;
	bool has_cached_hw_device_types = false;
	Vector<AVHWDeviceType> hw_device_types;

public:
	Vector<AVHWDeviceType> get_supported_hw_device_types();
	const AVCodec *get_codec_ptr() const;
	FFmpegCodec(const AVCodec *p_codec);
};

#endif // FFMPEG_CODEC_H
