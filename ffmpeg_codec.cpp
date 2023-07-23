/**************************************************************************/
/*  ffmpeg_codec.cpp                                                      */
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

#include "ffmpeg_codec.h"
#include "libavcodec/codec.h"

Vector<AVHWDeviceType> FFmpegCodec::get_supported_hw_device_types() {
	if (has_cached_hw_device_types) {
		return hw_device_types;
	}
	has_cached_hw_device_types = true;
	{
		const AVCodecHWConfig *hw_config;
		int i = 0;
		do {
			hw_config = avcodec_get_hw_config(codec, i);
			if (hw_config != NULL) {
				hw_device_types.push_back(hw_config->device_type);
			}
			i++;
		} while (hw_config != NULL);
	}
	return hw_device_types;
}

const AVCodec *FFmpegCodec::get_codec_ptr() const {
	return codec;
}

FFmpegCodec::FFmpegCodec(const AVCodec *p_codec) {
	codec = p_codec;
}
