#image-tools
The `imgt` command is similar to the ImageMagick `identify` command. It provides basic image format, width, height, etc information. The main goal is to expose the image analysis features in the pagespeed library as a command line interface including the complex analysis for determining if the specified image is a photo or a computer generated image. Also uses libgif to analzye animated gifs and properly determine transparency based on whether or not the transparent colour is actually used.

##Install
  * Only builds on x64 Linux. 
  * Requires cmake 2.6 or above and uses ExternalProject module.
  * Automatically downloads the pagespeed binary and statically links to it. 
  * Automatically downloads libgif and libpng and builds local copy to link with.
  * There is no acutal install step yet.
```
mkdir build
cd build
cmake ..
make
```
##Usage
```
Usage:  imgt options [ inputfile ... ]
  -h  --help             Display this usage information.
  -p  --photo            Check if the image is a photo.
  -t  --transparency     Check if the image usage transparency.
  -a  --animated         Check if the image is animated.
  -A  --All              Check all available options.
  -v  --verbose          Print verbose messages. 
```
##Example
```
imgt -A animated.gif 
format=GIF
width=365
height=360
photo=0
transparent=1
animated=1
frames=8
```