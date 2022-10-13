#!/bin/bash

VST_SDK_DIR=../../VST_SDK

rm -rf build

mkdir -p build
cp -R asid.vst3/Contents build

mkdir -p build/asid.vst3/Contents/MacOS

SOURCES="
	src/vst3/entry.cpp \
	src/vst3/plugin.cpp \
	src/vst3/controllerOSX.cpp \
	src/vst3/controllerOSXTimer.mm \
	$VST_SDK_DIR/vst3sdk/base/source/fobject.cpp \
	$VST_SDK_DIR/vst3sdk/base/source/baseiids.cpp \
	$VST_SDK_DIR/vst3sdk/base/source/fstreamer.cpp \
	$VST_SDK_DIR/vst3sdk/base/source/fstring.cpp \
	$VST_SDK_DIR/vst3sdk/pluginterfaces/base/coreiids.cpp \
	$VST_SDK_DIR/vst3sdk/pluginterfaces/base/funknown.cpp \
	$VST_SDK_DIR/vst3sdk/pluginterfaces/base/ustring.cpp \
	$VST_SDK_DIR/vst3sdk/pluginterfaces/base/conststringtable.cpp \
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
	$VST_SDK_DIR/vst3sdk/base/source/fdebug.cpp \
	$VST_SDK_DIR/vst3sdk/base/source/updatehandler.cpp \
	$VST_SDK_DIR/vst3sdk/base/thread/source/flock.cpp \
	$VST_SDK_DIR/vst3sdk/base/source/fbuffer.cpp \
	$VST_SDK_DIR/vst3sdk/public.sdk/source/main/macmain.cpp \
	../src/asid.c \
	../src/mos_8580_filter.c \
	src/asid_gui.c \
	src/gui-cocoa.mm \
"

CFLAGS="
	-framework Cocoa \
	-std=c++11 \
	\
	-I../src \
	-Isrc \
	-I$VST_SDK_DIR/vst3sdk/ \
	-fPIC -shared \
	-o build/asid.vst3/Contents/MacOS/asid \
	-DRELEASE=1 \
	-O3 \
	-ffast-math
"

# -lobjc -Wno-import 

clang++ $SOURCES $CFLAGS -arch x86_64 -o build/asid.vst3/Contents/MacOS/asid-x86_64
clang++ $SOURCES $CFLAGS -arch arm64 -o build/asid.vst3/Contents/MacOS/asid-arm64

lipo -create -output build/asid.vst3/Contents/MacOS/asid build/asid.vst3/Contents/MacOS/asid-x86_64 build/asid.vst3/Contents/MacOS/asid-arm64
rm build/asid.vst3/Contents/MacOS/asid-x86_64 build/asid.vst3/Contents/MacOS/asid-arm64
