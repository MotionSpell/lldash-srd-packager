# EncodingEncapsulation repository

## Tools

This repository contains the tools used to stream Point Clouds and TVMs. The repository contains the following applications:
 - ```bin2dash```: dynamic-libraryfication of the encoding/streaming part of ```pcl2dash```.
 - ```bin2dash_app```: a sample app looping on compressed data using the bin2dash DLL. This is a demo app: it does not expose all parameters and is not intended to be used on production.
 - ```pcl2dash```: capture point cloud ((device or watermelon stub) or uncompressed file), encode (CWI encoder), package (fMP4) and stream (MPEG-DASH). Used for Pilot#1.
 
These tools are available both for Windows 64 bits and Linux 64 bits.

Building this repository is not possible. Contact Motion Spell.

## Transmission chain (MPEG-DASH)

The transmission chain includes 3 components:
 - A streamer (this repository).
 - A HTTP server (a node.js script for Pilot#1, the lldash-relay SFU for Pilot#2: https://baltig.viaccess-orca.com:8443/VRT/deliverymcu-group/DeliveryMCU/releases).
 - A client (the SUB (Signals Unity Bridge) for Pilot#2: https://baltig.viaccess-orca.com:8443/VRT/nativeclient-group/SUB/releases)




# How to setup your pilot: easy as 1-2-3!

## Pilots constraints reminder

Pilot#1: two users. Local origin which requires to open ports locally.

## Pilot #1 live from capture or raw PLY

 1. Capture: automatic fake content, plug your hardware, or download some PLY samples (see above).
 2. HTTP server. Please install the low latency HTTP node.js server (https://github.com/gpac/node-gpac-dash + ```node gpac-dash.js -segment-marker eods -no-marker-write -chunk-media-segments```).
 3. Launch ```pcl2dash``` (cf below for details) e.g. ```./pcl2dash.exe -t 1 -n -1 -s 10000 -p cwi.json folder/to/loot-ply-uncompressed```. If you give no folder or URL, the capture will start from the camera.
 
/!\ Don't use ```pcl2dash``` with compressed data!

/!\ ```pcl2dash``` sleeps 100ms between two PLY readings to avoid I/O saturation. ```bin2dash_app``` exposes customs parameters for regulation.

## Pilot #2 live from capture or raw PLY

 1. Capture: automatic fake content, plug your hardware, or download some PLY samples (see above).
 2. HTTP server. Please install SFU (https://baltig.viaccess-orca.com:8443/VRT/deliverymcu-group/DeliveryMCU/releases) ; you can install it locally or remotely.
 3. Launch ```pcl2dash``` (cf below for details) e.g. ```./pcl2dash.exe -t 1 -n -1 -s 10000 -p cwi.json folder/to/loot-ply-uncompressed -u http://localhost:9000/```. If you give no folder or URL, the capture will start from the camera.
 
/!\ Don't user node-gpac-dash with multiple users, this will generate delays!

/!\ Don't use ```pcl2dash``` with compressed data!

/!\ ```pcl2dash``` sleeps 100ms between two PLY readings to avoid I/O saturation. ```bin2dash_app``` exposes customs parameters for regulation.

## Pilot #1 and #2 live from encoded CWIPC data

 1. HTTP server depending on your Pilot (node-gpac-http for Pilot#1 or Evanescent for Pilot#2).
 2. Launch ```bin2dash_app``` e.g. ```./bin2dash_app.exe -s 100 folder/to/cwipc_loot-compressed```. If you give no folder (all pilots) or URL (Pilot#2), the capture will start from the camera.





# Developer tools and information

## Test content

Compressed (cwipc) and uncompressed (PLY) point cloud data can be found at https://baltig.viaccess-orca.com:8443/VRT/nativeclient-group/cwipc_test/releases

```pcl2dash``` is able to generate live content from a connected Realsense2 or from some fake content if you do not have a Realsense2 connected. See below for command-line usage.

## Replay a MPEG-DASH session (pcl2dash only)

 1. Generate a capture session (e.g. 1234 frames): ```./pcl2dash.exe -t 1 -n 1234 -s 10000 -p cwi.json folder/to/ply_uncompressed```. If you want to stop the capture, press ctrl-c once and wait for the tool to exit.
 2. HTTP server for replay: ```node gpac-dash.js```. Omit the low latency paremeters as they will stuck your downloads.

## Developers: convert a MPEG-DASH session into a file and replay

The SUB supports playback from both HTTP for MPEG-DASH and from MP4 files.

To generate a MP4 file from a DASH session, concatenate the initialization segment with your media segment e.g. ```cat `ls *init.mp4` `ls -v *.m4s` > session.mp4```.

# How to use bin2dash_app.exe (standalone)

```
Usage: bin2dash_app [options, see below] [file_pattern]
    -d, --durationInMs                      0: segmentTimeline, otherwise SegmentNumber [default=10000]
    -s, --sleepAfterFrameInMs               Sleep time in ms after each frame, used for regulation [default=0]
    -u, --publishURL                        Publish URL ending with a separator. If empty files are written and the node-gpac-http server should be used, otherwise use the Evanescent SFU. [default=""]
```

```./bin2dash_app.exe -s 30 -u http://vrt-pcl2dash.viaccess-orca.com/ folder/to/cwipc_loot-compressed```

## API

See https://baltig.viaccess-orca.com:8443/VRT/nativeclient-group/EncodingEncapsulation/blob/dev/src/apps/bin2dash/bin2dash.hpp.

# How to use pcl2dash (standalone)

This is useful when you want to generate data ready to be streamed (e.g. to be copied on the HTTP server). When one closes a ```pcl2dash``` session cleanly (i.e. no more frames or ctrl-c, not by killing the process or windows), the live session is automatically transformed into an on-demande session ready to replay.

## Capture source

If you give ```pcl2dash``` a URL, ```pcl2dash``` will open this URL. Otherwise it will try to get the data from the capture device (```MultiFrame.dll``` from CWI) which contains the "watermelon" fallback.

## Examples

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
  "do_inter_frame": false,
  "gop_size": 1,
  "exp_factor": 0.0,
  "octree_bits": 7,
  "jpeg_quality": 85,
  "macroblock_size": 16
  "tilenumber": 0,
  "voxelsize": 0.0,
}
```

Stream a custom folder:
```./pcl2dash.exe -t 1 -n -1 -s 10000 -p cwi.json folder/to/ply_uncompressed```

Stream to a server:
```./pcl2dash.exe -t 1 -n -1 -s 10000 -p cwi.json -u http://vrt-pcl2dash.viaccess-orca.com:9000/ folder/to/ply_uncompressed```

# Stubbing

The ```pcl2dash``` capture (Signals user-module) is stubbed twice. If the multiFrame.dll is found, then it is used either with a capture device when found or with the watermelon sample. If multiFrame.dll is not found then the capture will use the path pattern from the command-line.

```bin2dash``` allows to easily stub the capture and encoding.

# Current limitations, legacy, and perspective

Multi-threaded works on Signals' side but needs to be tested with the CWI libraries.

The program holds on a specific cwi branch of Signals that contains non-conformant DASH modifications for compatibility with the i2cat GUB software. Not needed anymore but requires additional work.
