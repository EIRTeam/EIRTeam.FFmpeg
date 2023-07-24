/**************************************************************************/
/*  register_types.cpp                                                    */
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

#include "register_types.h"

#ifdef GDEXTENSION
#include "gdextension_build/gdex_print.h"
#else
#include "core/string/print_string.h"
#endif

#include "et_video_stream.h"

static void print_codecs() {
	const AVCodecDescriptor *desc = NULL;
	unsigned nb_codecs = 0, i = 0;

	char msg[512] = { 0 };
	print_line("Supported video codecs:");
	while ((desc = avcodec_descriptor_next(desc))) {
		const AVCodec *codec = NULL;
		void *i = NULL;
		bool found = false;
		while ((codec = av_codec_iterate(&i))) {
			if (codec->id == desc->id && av_codec_is_decoder(codec)) {
				if (!found && avcodec_find_decoder(desc->id) || avcodec_find_encoder(desc->id)) {
					snprintf(msg, sizeof(msg) - 1, "\t%s%s%s",
							avcodec_find_decoder(desc->id) ? "decode " : "",
							avcodec_find_encoder(desc->id) ? "encode " : "",
							desc->name);
					found = true;
					print_line(msg);
				}
				if (strcmp(codec->name, desc->name) != 0) {
					snprintf(msg, sizeof(msg) - 1, "\t  codec: %s", codec->name);
					print_line(msg);
				}
			}
		}
	}
}

void initialize_ffmpeg_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	print_codecs();
	GDREGISTER_ABSTRACT_CLASS(ETVideoStreamPlayback);
	GDREGISTER_CLASS(ETVideoStream);
}

void uninitialize_ffmpeg_module(ModuleInitializationLevel p_level) {
}

#ifdef GDEXTENSION

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/core/memory.hpp>

using namespace godot;

extern "C" {

GDExtensionBool GDE_EXPORT ffmpeg_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, const GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
	GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

	init_obj.register_initializer(&initialize_ffmpeg_module);
	init_obj.register_terminator(&uninitialize_ffmpeg_module);
	init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SERVERS);

	return init_obj.init();
}

} // ! extern "C"

#endif // ! GDEXTENSION
