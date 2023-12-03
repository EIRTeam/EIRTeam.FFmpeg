/**************************************************************************/
/*  video_stream_ffmpeg_loader.h                                          */
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

#ifndef VIDEO_STREAM_FFMPEG_LOADER_H
#define VIDEO_STREAM_FFMPEG_LOADER_H

#ifdef GDEXTENSION

// Headers for building as GDExtension plug-in.
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/resource_format_loader.hpp>

using namespace godot;

#else

#include "core/io/resource_loader.h"

#endif

#include "gdextension_build/func_redirect.h"

class VideoStreamFFMpegLoader : public ResourceFormatLoader {
	GDCLASS(VideoStreamFFMpegLoader, ResourceFormatLoader);
	PackedStringArray recognized_extension_cache;

private:
	void _update_recognized_extension_cache() const;
	String get_resource_type_internal(const String &p_path) const;
	PackedStringArray get_recognized_extensions_internal() const;
	Ref<Resource> load_internal(const String &p_path, const String &p_original_path = "", Error *r_error = nullptr, bool p_use_sub_threads = false, float *r_progress = nullptr, CacheMode p_cache_mode = CACHE_MODE_REUSE) const;
	bool handles_type_internal(const String &p_type) const;

protected:
	static void _bind_methods() {} // Required for gdextension, do not remove;
public:
	STREAM_FUNC_REDIRECT_1_CONST(String, get_resource_type, const String &, p_path);
#ifdef GDEXTENSION
	virtual PackedStringArray _get_recognized_extensions() const override;
	virtual bool _handles_type(const StringName &p_type) const override;
	virtual Variant _load(const String &p_path, const String &p_original_path, bool p_use_sub_threads, int32_t p_cache_mode) const override;
#else
	virtual void get_recognized_extensions(List<String> *p_extensions) const override;
	virtual Ref<Resource> load(const String &p_path, const String &p_original_path = "", Error *r_error = nullptr, bool p_use_sub_threads = false, float *r_progress = nullptr, CacheMode p_cache_mode = CACHE_MODE_REUSE) override;
	STREAM_FUNC_REDIRECT_1_CONST(bool, handles_type, const String &, p_type);
#endif
};

#endif // VIDEO_STREAM_FFMPEG_LOADER_H
