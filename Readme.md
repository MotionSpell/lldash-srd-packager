# VR-Together project

## CWISignalsIntegration repository

### Tools

This repository contains the tools used to stream Point Clouds for Pilot#1. The repository contains the following applications:
 - ```bin2dash```: dynamic-libraryfication of the encoding/streaming part of ```pcl2dash```.
 - ```bin2dash_app```: a sample app looping on compressed data using the bin2dash DLL.
 - ```pcl2dash```: capture ((device or watermelon stub) or uncompressed file), encode (CWI encoder), package (fMP4) and stream (MPEG-DASH). Used for Pilot#1.
 
These tools are available both for Windows 10 64 bits and Ubuntu 18.04 LTS.

Building this repository requires to contact Motion Spell for private code access to the Signals dependency.

## How to setup Pilot#1 easy as 1-2-3!

### Test content

Compressed (cwipc) and uncompressed (PLY) point cloud data can be found at https://baltig.viaccess-orca.com:8443/VRT/nativeclient-group/cwipc_test/releases

### Pilot #1 live from capture or raw PLY

 1. Capture: plug your hardware or download some PLY samples.
 2. HTTP server. It is highly recommended as low latency DASH may fail with your filesystem. Please install SFU (https://baltig.viaccess-orca.com:8443/VRT/deliverymcu-group/DeliveryMCU/releases) or the low latency HTTP node.js server (https://github.com/gpac/node-gpac-dash + ```node gpac-dash.js -segment-marker eods -no-marker-write -chunk-media-segments```).
 3. Launch ```pcl2dash``` (cf below for details) e.g. ```./pcl2dash.exe -t 1 -n -1 -s 10000 -p cwi.json folder/to/loot-ply-uncompressed```. If you give no folder or URL, the capture will start from the camera.
 
/!\ Don't use ```pcl2dash``` with compressed data!

/!\ ```pcl2dash``` sleeps 100ms between two PLY readings to avoid I/O saturation. ```bin2dash_app``` exposes customs parameters for regulation. 

### Pilot #1 live from encoded CWIPC data

 1. HTTP server. See previous section. Note that you can install the SFU locally if needed.
 2. Launch ```bin2dash_app``` e.g. ```./bin2dash_app.exe -s 100 folder/to/cwipc_loot-compressed```. If you give no folder or URL, the capture will start from the camera.

### Developers: replay a MPEG-DASH session

 1. Generate a capture session (e.g. 1234 frames): ```./pcl2dash.exe -t 1 -n 1234 -s 10000 -p cwi.json folder/to/ply_uncompressed```. If you want to stop the capture, press ctrl-c once and wait for the tool to exit.
 2. HTTP server for replay: ```node gpac-dash.js```. Omit the low latency paremeters as they will stuck your downloads.

### Developers: convert a MPEG-DASH session into a file and replay

The SUB supports playback from both HTTP for MPEG-DASH and from MP4 files.

To generate a MP4 file from a DASH session, concatenate the initialization segment with your media segment e.g. ```cat `ls *init.mp4` `ls -v *.m4s` > session.mp4```.

## How to use bin2dash_app.exe (standalone)

```
Usage: bin2dash_app [options, see below] [file_pattern]
    -d, --durationInMs                      0: segmentTimeline, otherwise SegmentNumber [default=10000]
    -t, --sleepAfterFrameInMs               Sleep time in ms after each frame, used for regulation [default=0]
    -u, --publishURL                        Publish URL. If empty files are written and the node-gpac-http server should be used, otherwise use the Evanescent SFU. [default=""]
```

```./bin2dash_app.exe -t 30 -u http://vrt-pcl2dash.viaccess-orca.com/ folder/to/cwipc_loot-compressed```

### API

See https://baltig.viaccess-orca.com:8443/VRT/nativeclient-group/EncodingEncapsulation/blob/dev/src/apps/bin2dash/bin2dash.hpp.

## How to use pcl2dash.exe (standalone)

This is useful when you want to generate data ready to be streamed (e.g. to be copied on the HTTP server). When one closes a ```pcl2dash``` session cleanly (i.e. no more frames or ctrl-c, not by killing the process or windows), the live session is automatically transformed into an on-demande session ready to replay.

### Capture source

If you give ```pcl2dash``` a URL, ```pcl2dash``` will open this URL. Otherwise it will try to get the data from the capture device (```MultiFrame.dll``` from CWI) which contains the "watermelon" fallback.

### Examples

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
```./pcl2dash.exe -t 1 -n -1 -s 10000 -p cwi.json folder/to/ply_uncompressed```

Stream to a server:
```./pcl2dash.exe -t 1 -n -1 -s 10000 -p cwi.json -u http://vrt-pcl2dash.viaccess-orca.com:9000/ folder/to/ply_uncompressed```

## Stubbing

The ```pcl2dash``` capture (Signals user-module) is stubbed twice. If the multiFrame.dll is found, then it is used either with a capture device when found or with the watermelon sample. If multiFrame.dll is not found then the capture will use the path pattern from the command-line.

```bin2dash``` allows to easily stub the capture and encoding.

## Current limitations, legacy, and perspective

Multi-threaded works on Signals' side but needs to be tested with the CWI libraries.

The program holds on a specific cwi branch of Signals that contains non-conformant DASH modifications for compatibility with the i2cat GUB software. Not needed anymore.
