# vmp
Visual music player

Visuam Music Player is a music player, visualizer and organizer inspired by the 2000's era iTunes and the tracker music players of the early 90's. VMP was made for SwayOS.

![alt text](screenshot.png)

## Features ##

- retro-minimalistic UI
- beautiful and smooth UX experience
- frequency and scope analyzer visualizers
- library auto-organization ( if enabled )
- activity window and human-readable database for transparent operation
- super lightwight, no GTK/QT

## Installation ##

Run these commands:

```
git clone https://github.com/milgra/vmp.git
cd vmp
meson build --buildtype=release
ninja -C build
sudo ninja -C build install
```

### From packages

[![Packaging status](https://repology.org/badge/tiny-repos/vmp.svg)](https://repology.org/project/vmp/versions)

## Command line options

-h 	   show help
-v 	   verbose
-l [path]  use custom library path, ~/Music is used by default
-o 	   toggle library organization, music files will be moved under [Library]/artist/album/track name.extension
-r [path]  set custom resource path

## Remote control

Send characters to /tmp/vmp, for example

```
echo "1" > /tmp/vmp
```

"1" toggle pause
"2" play next
"3" play previous song