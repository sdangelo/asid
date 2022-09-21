# A-SID

Turn your C64 into a wah effect.

You can find information and instructions [here](https://www.orastron.com/asid).

## Subfolders

* c64: the C64 program source;
* img2c64: browser-based tool that converts regular images to C64 hi-res bitmaps and colormaps and lets you quickly swap foreground/background color choice for each 8x8 tile;
* measure: BASIC program to control the C64 filter and output gain stage - actual measurements of the MOS 8580 chip in our C64 (C64C, ser. no. HB41416598E, made in Hong Kong) are available [here](TBD);
* octave: various GNU Octave scripts to generate program data, extract IRs, and simulate the MOS 8580 SID analog filter, output gain stange, and output buffer;
* spice: LTspice schematics of the MOS 8580 SID analog filter, output gain stage, and output buffer;
* src: A-SID sound engine with a full virtual analog model of the MOS 8580 analog filter, output gain stage, and output buffer, both implemented in C;
* vst3: VST3-related part of A-SID, using a code and build script template to develop and build VST3 plugins outisde the original SDK.

## Legal

Copyright (C) 2021, 2022 Orastron srl unipersonale.

Authors: Stefano D'Angelo, Paolo Marrone.

All the code in the repo is released under GPLv3. See the LICENSE file.

The file vst3/src/vst3/plugin.cpp contains code from sse2neon (https://github.com/DLTcollab/sse2neon/), which is released under the MIT license. Details in said file.

VST is a registered trademark of Steinberg Media Technologies GmbH.

All trademarks and registered marks are properties of their respective owners. All company, product, and service names used are for identification purposes only. Use of these names, trademarks, and brands does not imply endorsement.
