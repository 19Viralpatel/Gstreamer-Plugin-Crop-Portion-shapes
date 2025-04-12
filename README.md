# GStreamer Plugin: Crop Portion Shapes

A custom GStreamer plugin that enables cropping video frames using different geometric shapes such as rectangles, ellipses, and circles. This plugin provides an easy way to highlight or isolate specific portions of a video stream based on shape-based cropping.

## Features

-  Rectangle-based cropping  
-  Square-based cropping  
-  Circle-based cropping  
-  Triangle-based cropping
-  Easy integration with GStreamer pipelines  
-  Simple and modular design for extensibility

## Table of Contents

- [Installation](#installation)
- [Usage](#usage)
- [Pipeline Examples](#pipeline-examples)
- [Plugin Parameters](#plugin-parameters)
- [Directory Structure](#directory-structure)
- [Contributing](#contributing)
- [License](#license)

---

## Installation

1. **Clone the Repository**
   ```bash
   git clone https://github.com/19Viralpatel/Gstreamer-Plugin-Crop-Portion-shapes.git
   cd Gstreamer-Plugin-Crop-Portion-shapes

## Usage

After installing, the plugin can be used in a GStreamer pipeline. You can list all properties using:

gst-inspect-1.0 shape_crop

gst-launch-1.0 -v videotestsrc ! video/x-raw, format=NV12, width=1920, height=1080, 
framerate=30/1 ! cropportion crop-coordinate=, shape-size=100, shape=square ! 
xvimagesink hue=100 saturation=-100 brightness=100 

gst-launch-1.0 -v -m videotestsrc ! video/x-raw, format=NV12, width=1280, 
height=1024, framerate=30/1 ! cropportion shape-size=150, shape=circle, fill-color=red ! 
autovideosink 


## Plugin Parameters

shape: Specifies the cropping shape. Supported values are rect, square, circle, ellipse, and triangle. Example: shape=triangle.

x: X-coordinate of the shape's center or origin, depending on the shape. Example: x=100.

y: Y-coordinate of the shape's center or origin, depending on the shape. Example: y=100.

width: Applicable for rect and ellipse. Defines the width of the cropping area. Example: width=300.

height: Applicable for rect, ellipse, and triangle. Defines the height of the cropping area. Example: height=200.

size: Applicable for square. Sets the length of the square’s sides. Example: size=150.

radius: Applicable for circle. Defines the radius of the circular crop. Example: radius=100.

base: Applicable for triangle. Specifies the base length of the triangle. Example: base=120.

## Directory Structure

Gstreamer-Plugin-Crop-Portion-shapes/
??? Makefile             # Build instructions
??? plugin/              # Plugin source code
?   ??? gstshapecrop.c   # Core plugin logic
??? test/                # Test scripts and sample pipelines
??? README.md            # Project documentation

## License
This project is licensed under the MIT License - see the LICENSE file for details.
GStreamer itself is licensed under the Lesser General Public License version 2.1 or (at your option) any later version: https://www.gnu.org/licenses/lgpl-2.1.html

## Contributing
Please read CONTRIBUTING.md for details on our code of conduct, and the process for submitting pull requests to us.

## Changelog
See the CHANGELOG.md file for details about the changes between versions.

## Acknowledgements
Hat tip to anyone whose code was used
Inspiration
etc
If you use this plugin in your project, consider adding your name/company to the acknowledgments.
