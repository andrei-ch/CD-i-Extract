# CD-i Extract
Extract filesystem and MPEG streams from Philips CD-i disk images

## Background
Philips CD-i disc filesystem is not supported by modern operating systems. It was last supported natively in Windows 3.1.

Third-party Windows 95/98/ME filesystem driver [exists](http://www.icdia.co.uk/articles/filesystem.html) but it did not work for me.

There's also a Windows program called ISOBuster which was able to partially decode the filesystem but came short of decoding any useful media. 

## What this is
This tool allows extracting most of the media content stored on CD-i disc, provided your drive can read it on a physical level (many can't).

It supports extracting the complete filesystem, as well as real-time MPEG streams (a.k.a. "Full Motion Extension"). These MPEG streams cannot be accessed using only the filesystem queries because their playback requires information from sector headers.

Additionally, this tool's source code includes a library for parsing CD-ROM XA and CD-i sectors, and navigating CD-i filesystem.

## Build
This project requires boost library to build: `brew install boost`. It was last built using Xcode 11.6 and tested on macOS 10.15.16.

## Use
Before you can use this tool, you need to dump the contents of CD-i track from the disc. Note that data must be extracted as raw, preserving all sector content, not just audio parts.

For example:

1) Install cdda2wav:

`brew install cdda2wav`

2) Unmount disk prior to ripping (use your actual device):

`sudo umount /dev/disk4`

3) Extract CD-i track:

`sudo cdda2wav output-format=raw cdrom-endianess=big -t 0 image.raw`

4) Finally, run the tool to decode the image:

`cdi_extract print image.raw`

At this point you should see a list of directories and files stored on CD-i.

`cdi_extract extract image.raw`

`cdi_extract extract-mpegs image.raw`

## To address in future

1. Extract DYUV still images and ADPCM audio
