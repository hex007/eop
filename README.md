# eop
Eye of Pi Image Viewer for RaspberryPi using Dispmanx


## Build instructions

Dependency : libpng, libjpeg

```
cd eop
make
sudo make install
```

## Usage
```
Usage: eop <file> [x y w h]
	file     png or jpeg file to display
	x        horizontal offset
	y        vertical offset
	w        width to use [0 for height constrained]
	h        height to use [0 for width constrained]
	           note: both height and width can be zero for fullscreen

```
