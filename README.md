libreadkap
==========

Small GPL C++20 library to flexibly and efficiently read raster nautical chart data from BSB/KAP files.

libreadkap is intended to be relatively easy to integrate as a file decoder module with
other libraries and frameworks. A functional interface is provided allowing you to customize 
how the KAP file data is read, or you can provide a `std::istream` stream or filename to read from.   

After reading the KAP header, a callback function is called in which you can create or prepare
the output (a `KAPData` struct reference is passed which provides image size and metadata read from KAP file
header). 

Next, each row of KAP raster data is decoded and pixel data is written directly into an array 
of bytes via a pointer you provide from another callback function (with no intermediary buffer),
or directly into a byte buffer you provide.

The callback functions are `std::function` objects which can be bound to existing functions in 
other libraries or frameworks, or be lambda functions that call into other libraries or frameworks 
as desired.

A few basic utility functions for converting from geographic latitude/longitude to/from 
WGS84 flat coordinate system are provided.

An example of reading files from a BSB zip file (using libzip) and creating PNG images (using 
the FreeImage library) is provided.

Multiple palette options (e.g. NGT, DSK, GRY, etc.) are not supported.

The maximum image size is 65535 pixels x 65535 pixels.

Feature ideas to maybe add someday include: 
  * Utilities to convert from lat/lon to point on chart image and vice-versa.
  * Option to downsample/scale image while reading.
  * Function to parse BSB index file header and provide useful info (or provide path to parse any BSB or KAP file and get just metadata)
  * If not too big, could all or most of the library just be moved into header, maybe much of it could be inlined by compiler into user applications?

Building
--------

You will need GNU Make and a C++ compiler supporting C++17.

Run `make all` to build the library.

The freeimage and libzip libraries are required to build the examples. To install on Ubuntu, run `sudo apt install libfreeimage-dev libzip-dev`

Run `make examples` to build the examples.

Catch2 is required to build the unit and approval tests.
Run `make tests` to build and run the tests.

Bear is required to generate `compile_commands.json`, which is used to help configure IDEs, linters, etc.
Install with `sudo apt install bear` and run `make compile_commands.json` to (re)-generate.

License
-------

libreadkap was created by Reed Hedges based on parts of imgkap by MdJ, H.N, 
and Pavel Kalian (see <https://github.com/nohal/imgkap> or `imgkap.c.orig`
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


See Also
--------

* BSB/KAP file format description: <https://opencpn.org/wiki/dokuwiki/doku.php?id=opencpn:supplementary_software:chart_conversion_manual:bsb_kap_file_format>
* BSB/KAP file format description: <http://libbsb.sourceforge.net/bsb_file_format.html>
* libbsb library: <https://sourceforge.net/projects/libbsb/>
* GDAL support for BSB/KAP files: <https://gdal.org/drivers/raster/bsb.html>
* Example using GDAL to convert BSB/KAP files: <https://gist.github.com/colemanm/4587067>
* OpenCPN documentation with file conversion examples: <https://opencpn.org/wiki/dokuwiki/doku.php?id=opencpn:supplementary_software:chart_conversion_manual>
* Free RNC charts from US NOAA: <https://charts.noaa.gov/InteractiveCatalog/nrnc.shtml#mapTabs-1> (Choose "Paper Charts RNC & PDF", select chart, and click "RNC" button under "Available Products")


