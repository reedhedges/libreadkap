libreadkap
==========

Small GPL C++17 library to flexibly and efficiently read raster nautical chart data from BSB/KAP files.

libreadkap is intended to be relatively easy to integrate as a file decoder module with
other libraries and frameworks. A functional interface is provided allowing you to customize 
how the KAP file data is read, or you can provide standard C++ istream or filename to read from.   
After reading the KAP header, a callback function is called in which you can create or prepare
the output (a `KAPData` struct reference is passed which provides image size and metadata read from KAP file
header). then each row of KAP raster data is decoded and pixel data is written to an array 
of bytes via a pointer you provide from another callback function (with no intermediary buffer).  
These callback functions are std::function objects that can be bound to existing functions in 
other libraries or frameworks, or be lambda functions that call into other libraries or frameworks 
as desired.

A few basic utility functions for converting from geographic latitude/longitude to/from 
WGS84 flat coordinate system are provided.

An example of reading files from a BSB zip file (using libzip) and creating PNG images (using 
the FreeImage library) is provided.

Multiple palette options (e.g. NGT, DSK, GRY, etc.) are not supported.

The maximum image size is 65535 pixels x 65535 pixels.

Building
--------

You will need GNU Make and a C++ compiler supporting C++17.

Run `make all` to build the library.

The freeimage and libzip libraries are required to build the examples. To install on Ubuntu, run:
````
sudo apt install libfreeimage-dev libzip-dev
````
Run `make examples` to build the examples.

Catch2 is required to build the unit and approval tests.
Run `make tests` to build and run the tests.

Bear is required to generate `compile_commands.json`, which is used to help configure IDEs, linters, etc.
Install with `sudo apt install bear` and run `make compile_commands.json` to (re)-generate.

To Do
-----

* Add unit and approval tests.
* Support some pixel format conversions (e.g. to RGB formats frequently used in 
  GUI frameworks.  Qt recommends xRGB (0xffRRGGBB) but also supports many others.)
* Replace guts with code from libbsb instead of imgkap?
* Utilities to convert from lat/lon to point on chart image and vice-versa.
* Option to downsample/scale image while reading?
  
License
-------

libreadkap was created by Reed Hedges based on imgkap by MdJ, H.N, 
and Pavel Kalian (see <https://github.com/nohal/imgkap> or imgkap.c 
for original imgkap code).

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
