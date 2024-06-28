SHELL:=/usr/bin/env bash

HOST:=$(shell uname -s)
UNAME:=$(shell uname -a)

FFMPEG_LIBS:=libavcodec libavdevice libavfilter libavformat libavutil libswresample libswscale
ANDROID_ARCHS:=arm-v7a arm-v7a-neon arm64-v8a x86 x86_64
MACOS_ARCHS:=arm64 x86_64
LINUX_ARCHS:=x86_32 x86_64

ifneq (,$(filter $(HOST), Darwin))
	PLATFORM?=macos
else ifneq (,$(filter $(HOST), Linux))
	PLATFORM?=linux
else
	PLATFORM?=android
endif

ifeq (,$(filter $(PLATFORM), android macos linux))
$(error Invalid targeting platform: "$(PLATFORM)". Currently only support "android", "macos" and "linux".)
endif

ifeq ($(PLATFORM), android)
	TARGET_ARCH?=arm64-v8a
ifeq (,$(filter $(TARGET_ARCH), $(ANDROID_ARCHS)))
$(error Invalid targeting arch: "$(TARGET_ARCH)". Currently only support $(ANDROID_ARCHS) on Android.)
endif
else ifeq ($(PLATFORM), macos)
	TARGET_ARCH?=arm64 x86_64
ifeq (,$(filter $(TARGET_ARCH), $(MACOS_ARCHS)))
$(error Invalid targeting arch: "$(TARGET_ARCH)". Currently only support $(MACOS_ARCHS) on macOS.)
endif
else ifeq ($(PLATFORM), linux)
	TARGET_ARCH?=x86_64
ifeq (,$(filter $(TARGET_ARCH), $(LINUX_ARCHS)))
$(error Invalid targeting arch: "$(TARGET_ARCH)". Currently only support $(LINUX_ARCHS) on Linux.)
endif
else ifneq (,$(filter $(UNAME), x86_64))
	TARGET_ARCH?=x86_64
endif

ifeq ($(PLATFORM), android)
	ifeq ($(TARGET_ARCH), arm64-v8a)
		TARGET_HOST:=android-arm64
		PREFIX_ARCH:=arm64
	else ifeq ($(TARGET_ARCH), arm-v7a)
		TARGET_HOST:=android-arm
		PREFIX_ARCH:=arm32
	else ifeq ($(TARGET_ARCH), arm-v7a-neon)
		TARGET_HOST:=android-arm-neon
		PREFIX_ARCH:=arm32
	else ifeq ($(TARGET_ARCH), x86)
		TARGET_HOST:=android-x86
		PREFIX_ARCH:=x86_32
	else ifeq ($(TARGET_ARCH), x86_64)
		TARGET_HOST:=android-x86_64
		PREFIX_ARCH:=x86_64
	endif
	export ANDROID_SDK_ROOT:=$(or $(ANDROID_HOME),$(ANDROID_SDK_ROOT))
	export ANDROID_NDK_ROOT:=$(or $(ANDROID_NDK_HOME),$(ANDROID_NDK_ROOT))
	_ := $(foreach arch,$(ANDROID_ARCHS),\
			$(if $(filter $(TARGET_ARCH),$(arch)),,\
		 		$(eval EXTRA_PARAMS += --disable-$(subst _,-,$(arch)))))
else ifeq ($(PLATFORM), macos)
	ifeq ($(findstring arm64,$(TARGET_ARCH))$(findstring x86_64,$(TARGET_ARCH)),arm64x86_64)
		TARGET_HOST=apple-macos-universal
		PREFIX_ARCH:=universal
		TARGET_HOST_OSX_ARM64:=apple-macos-arm64
		TARGET_HOST_OSX_X86_64:=apple-macos-x86_64
		FFMPEG_LIB_PKG:=ffmpeg-kit/prebuilt/$(TARGET_HOST_OSX_ARM64)/ffmpeg/lib/pkgconfig/libavcodec.pc
		FFMPEG_INC_DIR:=ffmpeg-kit/prebuilt/$(TARGET_HOST_OSX_ARM64)/ffmpeg/include
	else
		TARGET_HOST=apple-macos-$(TARGET_ARCH)
		PREFIX_ARCH:=$(TARGET_ARCH)
	endif
	ifneq (,$(findstring universal,$(TARGET_HOST)))
		EXTRA_PARAMS :=
	else ifeq (,$(findstring arm64,$(TARGET_HOST)))
		EXTRA_PARAMS := --disable-arm64
	else ifeq (,$(findstring x86_64,$(TARGET_HOST)))
		EXTRA_PARAMS := --disable-x86-64
	endif
else ifeq ($(PLATFORM), linux)
	TARGET_HOST=linux-$(TARGET_ARCH)
	PREFIX_ARCH:=$(TARGET_ARCH)
endif

FFMPEG_LIB_PKG?=ffmpeg-kit/prebuilt/$(TARGET_HOST)/ffmpeg/lib/pkgconfig
FFMPEG_INC_DIR?=ffmpeg-kit/prebuilt/$(TARGET_HOST)/ffmpeg/include
FFMPEG_PREFIX:=thirdparty/ffmpeg/$(PLATFORM)/$(PREFIX_ARCH)


bootstrap:
ifneq (,$(filter $(HOST), Darwin))
	@echo "Bootstrapping for macOS..."
	brew install autoconf automake libtool pkg-config curl cmake gperf groff texinfo yasm nasm bison autogen git wget meson ninja guile
else ifneq (,$(filter $(HOST), Linux))
	@echo "Bootstrapping for Linux..."
	sudo apt install autoconf automake libtool pkg-config curl cmake gperf groff texinfo yasm nasm bison autogen git wget meson ninja rapidjson-dev
endif


$(FFMPEG_LIB_PKG):
	@echo "TARGET_HOST:" $(TARGET_HOST)
	@echo "TARGET_ARCH:" $(TARGET_ARCH)
	@echo "EXTRA_PARAMS:" $(EXTRA_PARAMS)
ifeq ($(PLATFORM), android)
ifneq ($(wildcard $(ANDROID_SDK_ROOT)/platforms/.*),)
	@echo "Found Android SDK at" $(ANDROID_SDK_ROOT)
else
	$(error "Android SDK not found. Please set ANDROID_SDK_ROOT or ANDROID_HOME.")
endif
ifneq ($(wildcard $(ANDROID_NDK_ROOT)/prebuilt/.*),)
	@echo "Found Android NDK at" $(ANDROID_NDK_ROOT)
else
	$(error "Android NDK not found. Please set ANDROID_NDK_ROOT or ANDROID_NDK_HOME.")
endif
	cd ffmpeg-kit && ./android.sh $(EXTRA_PARAMS) --enable-android-media-codec --enable-android-zlib --no-archive \
		--enable-gpl --enable-x264 --enable-x265 --enable-lame
else ifeq ($(PLATFORM), macos)
	cd ffmpeg-kit && ./macos.sh --target=10.12 $(EXTRA_PARAMS) --enable-gpl --enable-macos-avfoundation --no-framework \
		--enable-gpl --enable-x264 --enable-x265 --enable-lame
else ifeq ($(PLATFORM), linux)
	cd ffmpeg-kit && ./linux.sh --enable-gpl --enable-x264 --enable-linux-x265 --enable-linux-lame
endif


ffmpeg: $(FFMPEG_LIB_PKG)
	rm -rf $(FFMPEG_PREFIX)
	mkdir -p $(FFMPEG_PREFIX)/lib
	mkdir -p $(FFMPEG_PREFIX)/include && cp -r $(FFMPEG_INC_DIR)/* $(FFMPEG_PREFIX)/include
ifeq ($(TARGET_HOST), apple-macos-universal)
	$(foreach lib,$(FFMPEG_LIBS), \
        lipo -create -output $(FFMPEG_PREFIX)/lib/$(lib).dylib \
		ffmpeg-kit/prebuilt/$(TARGET_HOST_OSX_ARM64)/ffmpeg/lib/$(lib).dylib \
		ffmpeg-kit/prebuilt/$(TARGET_HOST_OSX_X86_64)/ffmpeg/lib/$(lib).dylib;)
else ifeq ($(PLATFORM), macos)
	$(foreach lib,$(FFMPEG_LIBS), \
		cp ffmpeg-kit/prebuilt/$(TARGET_HOST)/ffmpeg/lib/$(lib).dylib $(FFMPEG_PREFIX)/lib;)
else ifeq ($(TARGET_HOST), android-arm-neon)
	$(foreach lib,$(FFMPEG_LIBS), \
		cp ffmpeg-kit/prebuilt/$(TARGET_HOST)/ffmpeg/lib/$(lib)_neon.so $(FFMPEG_PREFIX)/lib;)
else
	$(foreach lib,$(FFMPEG_LIBS), \
		cp ffmpeg-kit/prebuilt/$(TARGET_HOST)/ffmpeg/lib/$(lib).so $(FFMPEG_PREFIX)/lib;)
endif


gdextension: ffmpeg
ifeq ($(PLATFORM), android)
	cd gdextension_build && scons platform=$(PLATFORM) arch=$(PREFIX_ARCH) target=template_release ffmpeg_path=../$(FFMPEG_PREFIX)
	cd gdextension_build && scons platform=$(PLATFORM) arch=$(PREFIX_ARCH) target=template_debug ffmpeg_path=../$(FFMPEG_PREFIX)
else
	cd gdextension_build && scons platform=$(PLATFORM) target=template_release ffmpeg_path=../$(FFMPEG_PREFIX)
	cd gdextension_build && scons platform=$(PLATFORM) target=template_debug ffmpeg_path=../$(FFMPEG_PREFIX)
endif


clean:
	rm -rf thirdparty/ffmpeg
