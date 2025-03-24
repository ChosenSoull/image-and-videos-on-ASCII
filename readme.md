# Image-and-videos-on-ASCII

This program is a command-line tool designed to convert images and video files into ASCII art. It allows users to create ASCII art from their favorite images and videos, customizing the output size and saving the results to a text file.

Key Features:

    Image to ASCII Art Conversion:
        Supports various image formats (powered by the stb_image library).
        Image scaling to fit specified dimensions.
        Output of ASCII art to the terminal with colored characters or to a text file.
        Transparency support for images.
    Video to ASCII Art Conversion:
        Supports popular video formats (MP4, AVI, MKV) (powered by libavcodec and libavformat libraries).
        Video decoding and frame-by-frame conversion to ASCII art.
        Frame scaling to fit specified dimensions.
        Output of ASCII art to the terminal with colored characters or to a text file.
    Output Size Customization:
        Allows setting maximum width and height for the ASCII art via command-line arguments.
    File Output:
        Option to save the ASCII art to a text file (output.txt).
    Colored Output:
        Output colored ASCII art to the console.

How to build:
```bash
git clone https://github.com/ChosenSoull/image-and-videos-on-ASCII
cd image-and-videos-on-ASCII
gcc -o ascii ascii.c -lavformat -lavcodec -lswscale -lavutil -lm
```
Usage:
```bash
./ascii -i <file> [-w <width>] [-h <height>] [--output]

    -i <file>: Specifies the input file (image or video).
    -w <width>: Maximum width of the ASCII art (default 600).
    -h <height>: Maximum height of the ASCII art (default 140).
    --output: (Optional) Outputs the ASCII art to a text file (output.txt).
```
Examples:
```bash
./ascii -i image.jpg -w 100 -h 50
./ascii -i video.mp4 --output
./ascii -i image.png -w 200 -h 100 --output
```
Dependencies:

    stb_image
    libavcodec
    libavformat
    libswscale

Known Limitations:

Image scaling uses a basic nearest-neighbor algorithm.
Video processing can be resource-intensive.

Author: ChosenSoul