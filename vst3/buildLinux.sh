#!/bin/bash

VST_SDK_DIR=../../VST_SDK

rm -rf build

mkdir -p build
cp -R asid.vst3 build

mkdir -p build/asid.vst3/Contents/x86_64-linux

g++ \
	src/vst3/entry.cpp \
	src/vst3/plugin.cpp \
	src/vst3/controllerLinux.cpp \
	$VST_SDK_DIR/vst3sdk/base/source/fobject.cpp \
	$VST_SDK_DIR/vst3sdk/base/source/baseiids.cpp \
	$VST_SDK_DIR/vst3sdk/base/source/fstreamer.cpp \
	$VST_SDK_DIR/vst3sdk/base/source/fstring.cpp \
	$VST_SDK_DIR/vst3sdk/pluginterfaces/base/coreiids.cpp \
	$VST_SDK_DIR/vst3sdk/pluginterfaces/base/funknown.cpp \
	$VST_SDK_DIR/vst3sdk/pluginterfaces/base/ustring.cpp \
	$VST_SDK_DIR/vst3sdk/pluginterfaces/base/conststringtable.cpp \
	$VST_SDK_DIR/vst3sdk/public.sdk/source/main/linuxmain.cpp \
	$VST_SDK_DIR/vst3sdk/public.sdk/source/main/pluginfactory.cpp \
	$VST_SDK_DIR/vst3sdk/public.sdk/source/main/moduleinit.cpp \
	$VST_SDK_DIR/vst3sdk/public.sdk/source/common/commoniids.cpp \
	$VST_SDK_DIR/vst3sdk/public.sdk/source/common/pluginview.cpp \
	$VST_SDK_DIR/vst3sdk/public.sdk/source/vst/vstcomponentbase.cpp \
	$VST_SDK_DIR/vst3sdk/public.sdk/source/vst/vstcomponent.cpp \
	$VST_SDK_DIR/vst3sdk/public.sdk/source/vst/vstaudioeffect.cpp \
	$VST_SDK_DIR/vst3sdk/public.sdk/source/vst/vstinitiids.cpp \
	$VST_SDK_DIR/vst3sdk/public.sdk/source/vst/vstbus.cpp \
	$VST_SDK_DIR/vst3sdk/public.sdk/source/vst/vsteditcontroller.cpp \
	$VST_SDK_DIR/vst3sdk/public.sdk/source/vst/vstparameters.cpp \
	\
	../src/asid.c \
	../src/mos_8580_filter.c \
	src/asid_gui.c \
	src/gui-x.c \
	\
	-DRELEASE=1 \
	-I../src \
	-Isrc \
	-I$VST_SDK_DIR/vst3sdk/ \
	-fPIC -shared \
	-static-libgcc \
	-static-libstdc++ \
	-o build/asid.vst3/Contents/x86_64-linux/asid.so \
	-O3
