#pragma once

#include <functional>
#include <map>
#include <string>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <climits>
#include <cassert>

namespace libreadkap {

    /// Metadata from KAP file
    struct KAPData {
        uint16_t width; ///< number of image rows. from RA field. 
        uint16_t height; ///< number of image columns. from RA field. 
        double rx; ///< Pixel size (From DX field)
        double ry; ///< Pixel size (From DY field)
        int bits_in;  ///< Pixel depth (bits) of KAP data. From IFM field. TODO is this whole pixel or pixel color component. (E.g. does 8 mean 8 bits per pixel, or 24 bit 888=RGB ?)
        int bits_out; // todo move out of this into function?
        // todo georeference points, scale, units, projection, datum etc.
        // todo std::string title; ///< from first field in BSB/NA
        // todo std::map<std::string, std::string> headers_text; ///< all headers. TODO option to readkap() to not save these
    };

    struct Options {

    };

    enum Status {
        NoError,
        UnknownError,
        FileOpenError,
        KAPIndexTableError,
        FileReadError,
        KAPHeaderError,
        UserLimitsError
    };

    struct UserBufferInfo {
        size_t maxheight;
        size_t maxwidth;
        uint8_t desired_bitdepth;
    };

    /// Called  by readkap() after reading KAP header.  In this function you can create and set up your destination bitmap data.
    using start_function = std::function<UserBufferInfo(const KAPData& data)>;

    /// Called by readkap() to obtain a pointer to a certain row in the buffer. 
    /// TODO Is it useful to be able to provide separate rows rather than provide to readkap() one pointer to a monolithic bitmap buffer? (E.g. do some image data structures use separate/aligned scanline rows?)
    using get_line_function = std::function<uint8_t*(size_t row_index, const KAPData& kapdata)>; 

    /// Read @a nmemb items of data, each @a size bytes long from some file, storing them at @a buf
    /// Return number of items (@a nmemb) successfully read.  If return value is less than @a nmemb, an error occurred.
    using fread_function = std::function<size_t(void *buf, size_t size, size_t nmemb)>;

    /// Change internal read pointer position in a file (either offset relative to start, or backwards from end, according to sign of @a pos)
    /// If pos is negative, seek backwards relative to end of file, otherwise seek forwards relative to start of file.
    /// Return new current position. 
    using fseek_function = std::function<long(long pos)>;

    /** Example fread_function and fseek_function functions which are lambda closures that capture FILE* pointers and wrap C stdio functions: 
            FILE *fp;
            auto  cstd_fread = [=fp](void *buf, size_t size, size_t nmemb) -> size_t {
                return fread(buf, size, nmemb, fp);
            };
            auto cstd_fseek = [=fp](long pos) -> long {
                if(pos < 0)
                    fseek(fp, pos, SEEK_END);
                else
                    fseek(fp, pos, SEEK_SET);
                return ftell(fp);
            }
    */

    /// Use this to provide an alternate fread()-like and fseek()-like functions to read KAP file data
    Status readkap(fread_function freadfn, fseek_function fseekfn, start_function startfn, get_line_function linefn);


    /// Read KAP file from std::istream
    Status readkap(std::istream& stream, start_function startfn, get_line_function linefn)
    {
        auto readfn = [&stream](void *buf, size_t size, size_t nmemb) -> size_t {
            try {
                // std::istream is a basic_istream with char_type = char
                char *cptr = reinterpret_cast<char*>(buf);
                size_t n = size * nmemb;
                assert(n <= LONG_MAX); // istream::read() takes streamsize AKA 'long' argument
                stream.read(cptr, (std::streamsize)n);
            } catch(std::exception&) {
                return 0;
            }
            return size*nmemb;
        };
        auto seekfn = [&stream](long pos) -> long {
            if(pos < 0)
                stream.seekg(pos, stream.end);
            else
                stream.seekg(pos);
            return stream.tellg();
        };
        return readkap(readfn, seekfn, startfn, linefn);
    }

    /// Read KAP file from std::istream
    Status readkap(std::istream& stream, start_function startfn, uint8_t* buffer)
    {
        auto linefn = [buffer](size_t row, const KAPData& kapdata) -> uint8_t* {
            return buffer + (row * kapdata.width);
        };
        return readkap(stream, startfn, linefn);
    }

    /// Read KAP file with given name on filesystem
    Status readkap(const std::filesystem::path& filename, start_function startfn, get_line_function linefn)
    {
        std::ifstream stream(filename.c_str());
        if(!stream.is_open())
            return FileOpenError;
        return readkap(stream, startfn, linefn);
    }

    /// Read KAP file with given name on filesystem
    Status readkap(const std::filesystem::path& filename, start_function startfn, uint8_t* buffer)
    {
        auto linefn = [buffer](size_t row, const KAPData& kapdata) -> uint8_t* {
            return buffer + (row * kapdata.width);
        };
        return readkap(filename, startfn, linefn);
    }

  
    /// Return heading from one geographic point to another, in degrees
    double heading_between(double lat0, double lon0, double lat1, double lon1);

    /// Convert longitude to position in flat space
    double longitude_to_x_wgs84(double l);

    /// Convert latitude to position in WGS84 
    double latitude_to_y_wgs84(double l);

    // TODO add versions of the above that find points on chart, and vice-versa.
}
