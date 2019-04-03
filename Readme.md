# VR-Together project

## CWISignalsIntegration repository

This repository contains the following tools:
 - pcl2dash: capture (device|stub or file), encode (CWI encoder), package (fMP4) and stream (MPEG-DASH).
 - A dynamic-libraryfication of pcl2dash for CERTH.

## How to setup Pilot#1: easy as 1-2-3!

 1. Capture: plug your hardware or download some samples (e.g. https://baltig.viaccess-orca.com:8443/VRT/nativeclient-group/cwipc_codec/uploads/d1f5ef3f2c205b0c296dca58bada7c7e/loot-compressed.zip)
 2. HTTP server. It is highly recommended as low latency DASH may fail with your filesystem. Please install the low latency HTTP node.js server (https://github.com/gpac/node-gpac-dash) and launch it: ```node gpac-dash.js -segment-marker eods -chunk-media-segments```.
 3. Launch pcl2dash (cf below for details) e.g. ```./pcl2dash.exe -t 1 -n -1 -s 10000 -p cwi.json folder/to/201904_loot-compressed```.

## How to use it (standalone)

Examples:

Only 10 frames:
```./pcl2dash.exe -t 1 -n 10```

Infinite frames:
```./pcl2dash.exe -t 1 -n -1```

Segment duration of 10 seconds:
```./pcl2dash.exe -t 1 -n -1 -s 10000```

Fake delay in seconds (to avoir clock-instability near realtime):
```./pcl2dash.exe -t 1 -n -1 -s 10000 -d 1```

Custom encoding parameters:
```./pcl2dash.exe -t 1 -n -1 -s 10000 -p cwi.json```

with cwi.json
```
{
  "num_threads": 1,
  "do_inter_frame": false,
  "gop_size": 1,
  "exp_factor": 0.0,
  "octree_bits": 7,
  "color_bits": 8,
  "jpeg_quality": 85,
  "macroblock_size": 16
}
```

Stream a custom folder:
```./pcl2dash.exe -t 1 -n -1 -s 10000 -p cwi.json folder/to/201904_loot-compressed```

## Stubbing

The capture (Signals usr-module) is stubbed twice. If the multiFrame.dll is found, then it is used either with a capture device when found or with the watermelon sample. If multiFrame.dll is not found then the capture will use the path pattern from the command-line.

## Current limitations and legacy

The program is not multi-threaded due to API issues in the CWI capture/codec.

The program holds on a specific cwi branch of Signals that contains non-conformant DASH modifications for compatibility with the i2cat GUB software.
