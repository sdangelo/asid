#!/bin/sh

BASEDIR=$(dirname "$0")
rm -fr /Library/Audio/Plug-Ins/VST3/asid.vst3 || exit 1
cp -R $BASEDIR/asid.vst3 /Library/Audio/Plug-Ins/VST3/ || exit 1
chmod +x /Library/Audio/Plug-Ins/VST3/asid.vst3/Contents/MacOS/asid || exit 1
xattr -rd com.apple.quarantine /Library/Audio/Plug-Ins/VST3/asid.vst3 || exit 1
