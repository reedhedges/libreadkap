

#include "libreadkap.h"


#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <cfloat>
#include <cassert>

using namespace libreadkap;


static const double WGSinvf                           = 298.257223563;       /* WGS84 1/f */
static const double WGSexentrik                       = 0.081819;             /* e = 1/WGSinvf; e = sqrt(2*e -e*e) ;*/


double libreadkap::heading_between(double lat0, double lon0, double lat1, double lon1)
{
    double x,v,w;

    lat0 = lat0 * M_PI / 180.;
    lat1 = lat1 * M_PI / 180.;
    lon0 = lon0 * M_PI / 180.;
    lon1 = lon1 * M_PI / 180.;

    v = sin((lon0 - lon1)/2.0);
    w = cos(lat0) * cos(lat1) * v * v;
    x = sin((lat0 - lat1)/2.0);
    long double d = (180. * 60. / M_PI) * 2.0 * asinl(sqrtl(x*x + w));
    assert(d <= DBL_MAX);
    return (double)d;
}


double libreadkap::longitude_to_x_wgs84(double l)
{
    return l*M_PI/180;
}

double libreadkap::latitude_to_y_wgs84(double l)
{
    double e = WGSexentrik;
    //double s = sinl(l*M_PI/180.0);
    double s = sin(l*M_PI/180.0);

    return log(tan(M_PI/4 + l * M_PI/360)*pow((1. - e * s)/(1. + e * s), e/2.));
}



//* si size < 0 lit jusqu'a \n (elimine \r) et convertie NO1 int r = (c - 9) & 0xFF; */

static inline int fgetkapc(fread_function freadfn)
{
    int c = 0;
    freadfn(&c, 1, 1);
    //if (typein == FIF_NO1) return (c - 9) & 0xff;
    return c;
}

static inline int fgetkaps(char *s, int size, fread_function freadfn)
{
    int i,c;

    i = 0;

    // if size is negative, read until size characters are read, \n, EOF, or a '\0' character is encountered?
    // if size is positive, read until size characters are read, \n, EOF.
    // \r characters are skipped and not saved.
    // todo replace positive/negative size weirdness with a simple flag?
    // todo instead of reading character by character, use freadfn to read into a buffer of 'size' characters
    while (((c = fgetkapc(freadfn)) != EOF) && size)
    {
        //if (typein == FIF_NO1) c = (c - 9) & 0xff ;
        if (size > 0)
        {
            s[i++] = (char)c ;
            size--;
            continue;
        }
        if (c == '\r') continue;
        if (c == '\n')
        {
            s[i] = 0;
            size++;
            break;
        }
        s[i++] = (char)c;
        size++;
        if (!c) break;
    }
    return i;
}


static uint32_t *read_index(fread_function freadfn, fseek_function fseekfn, uint16_t height)
{
    long pos = fseekfn(-4);
    assert(pos >= 0);
    uint32_t end = (uint32_t)pos;
    uint8_t bytes[4];
    freadfn(&bytes, 4, 1);
    uint32_t l = 
        ((uint32_t)bytes[0]<<24) + 
        ((uint32_t)bytes[1]<<16)+
        ((uint32_t)bytes[2]<<8) +
        ((uint32_t)bytes[3]);

    assert(l <= end);

    if (((end - l)/4) != height) return NULL;

    uint32_t *index = (uint32_t *)malloc(height*sizeof(uint32_t));
    assert(index);

    /* Read index table */
    fseekfn(l);
    for (uint16_t i=0; i < height; i++)
    {
        freadfn(&bytes, 4, 1);
        index[i] =  ((uint32_t)bytes[0]<<24)+
                    ((uint32_t)bytes[1]<<16)+
                    ((uint32_t)bytes[2]<<8)+
                    ((uint32_t)bytes[3]);
    }

    return index;
}



static uint16_t read_pixel(fread_function readfn, uint8_t *pixel, uint8_t decin, uint8_t maxin)
{
    uint8_t c;
    uint16_t count;

    readfn(&c, 1, 1);

    count = (c & 0x7f);
    assert((count >> decin) <= UCHAR_MAX);
    *pixel = (uint8_t)(count >> decin);
    //count &= maxin;
    count = (uint16_t)(count & maxin);
    while (c & 0x80)
    {
        readfn(&c, 1, 1);
        int newcount = (count << 7) + (c & 0x7f);
        assert(newcount <= USHRT_MAX);
        count = (uint16_t)newcount;
    }
    return count+1;
}



static int decode_row(fread_function &readfn, uint8_t *line, const KAPData &kapdata)
//(int typein, FILE *in, uint8_t *buf_out, uint16_t bits_in,uint16_t bits_out, uint16_t width)
{
    uint16_t    count;
    uint8_t     pixel;
    uint8_t     decin, maxin;
    uint16_t    xout = 0;

    decin = (uint8_t)(7-kapdata.bits_in);
    maxin = (uint8_t)((1<<decin) - 1);

    /* read the line number */
    count = read_pixel(readfn, &pixel, 0, 0x7F); //(typein, in,&pixel,0,0x7F);

    /* no test count = line number */
    uint16_t w = kapdata.width;

    // todo support only 8 bit depth
    // todo read whole line rather than one at a time
    switch (kapdata.bits_out)
    {
        case 1:
            while (w)
            {
                count = read_pixel(readfn, &pixel, decin, maxin); //typein,in,&pixel, decin,maxin);
                if (count > w) count = w;
                w -= count;
                while (count)
                {
                    assert((xout>>3) < kapdata.width);
                    //line[xout>>3] |= pixel<<(7-(xout&0x7));
                    line[xout>>3] = (uint8_t) ( line[xout>>3] | (pixel << (7-(xout & 0x7))) );
                    xout++;
                    count--;
                }
            }
            break;
        case 4:
             while (w)
             {
                count = read_pixel(readfn,&pixel, decin,maxin);
                if (count > w) count = w;
                w -= count;
                while (count)
                {
                    assert((xout>>1) < kapdata.width);
                    //line[xout>>1] |= pixel<<(4-((xout&1)<<2));
                    line[xout>>1] = (uint8_t) ( line[xout>>1] | (pixel<<(4-((xout&1)<<2))) );
                    xout++;
                    count--;
                }
            }
            break;
        case 8:
            while ( w )
            {
                count = read_pixel(readfn,&pixel, decin,maxin);
                if (count > w) count = w;
                w -= count;
                while (count)
                {
                    *line++ = pixel;
                    count--;
                }
            }
            break;
        default:
            assert(kapdata.bits_out == 1 || kapdata.bits_out == 4 || kapdata.bits_out == 8);
    }
    /* read last byte (0) */
    readfn(&pixel, 1, 1);

    return 0;
}

/* static void read_line(uint8_t *in, uint16_t bits, int width, uint8_t *colors, histogram *hist, uint8_t *out)
{
    int i;
    uint8_t c = 0;

    switch (bits)
    {
        case 1:
            for (i=0;i<width;i++)
            {
                out[i] = colors[(in[i>>3] >> (7-(i & 0x7))) & 1];
            }
            return;
        case 4:
            for (i=0;i<width;i++)
            {
                c = in[i >> 1];
                out[i++] = colors[(c>>4) & 0xF];
                out[i] = colors[c & 0xF];
            }
            return;
        case 8:
            for (i=0;i<width;i++)
            {
                out[i] = colors[in[i]];
            }
            return;
        case 24:
        {
            Color32 cur, last;
            last.p = 0xFFFFFFFF;

            for (i=0;i<width;i++)
            {
                cur.p = ( *(uint32_t *)in & RGBMASK);
                if (last.p != cur.p)
                {
                    c = colors[HistGetColorNum(hist, cur)];
                    last = cur;
                }
                out[i] = c;
                in += 3;
            }
        }
    }
}

*/



static bool read_header(fread_function freadfn, KAPData& kapdata)
//int typeout, FILE *out, char *date,char *title,int optcolor,int *widthout, int *heightout,  double *rx, double *ry, int *depth, RGBQUAD *palette)
{
    char    *s;
    char    line[1024];

    /* lit l entete kap  y compris RGB et l'écrit dans un fichier sauf RGB*/
    /* *widthout = *heightout = 0;
    if (depth != NULL) *depth = 0;
    if (palette != NULL) memset(palette,0,sizeof(RGBQUAD)*128);
 */


    while (fgetkaps(line, -1024, freadfn) > 0) // size 1024 is negative as a flag to fgetkaps for special behavior -- todo fix
    {
        // todo do better parsing than strstr and sscanf.  

        // TODO check  for more scanf errors

        if (line[0] == 0x1a) // ? what is this? start of data?
            break;
        if ((s = strstr(line, "RA=")))
        {
            unsigned x0, y0;

            /* Attempt to read old-style NOS (4 parameter) version of RA=  (x0 and y0 are ignored) */
            /* then fall back to newer 2-argument version */
            // TODO verify that width and height read from file will fit in our variables
            // TODO check if file format specifies max unsigned 16-bit sizes for width and height
            long unsigned width, height;
            if ((sscanf(s,"RA=%u,%u,%lu,%lu", &x0, &y0, &width, &height) != 4) &&
                (sscanf(s,"RA=%lu,%lu", &width, &height) != 2))
            {
                // error from sscanf
                return false;
            }
            assert(width <= USHRT_MAX);
            assert(height <= USHRT_MAX);
            kapdata.width = (uint16_t)width;
            kapdata.height = (uint16_t)height;
        }

        /* if (palette != NULL)
        {
            int index,r,g,b;
            RGBQUAD *p = NULL;

            if (sscanf(line, "RGB/%d,%d,%d,%d", &index, &r, &g, &b) == 4) p = palette;
            if (sscanf(line, "DAY/%d,%d,%d,%d", &index, &r, &g, &b) == 4) p = palette+256;
            if (sscanf(line, "DSK/%d,%d,%d,%d", &index, &r, &g, &b) == 4) p = palette+256*2;
            if (sscanf(line, "NGT/%d,%d,%d,%d", &index, &r, &g, &b) == 4) p = palette+256*3;
            if (sscanf(line, "NGR/%d,%d,%d,%d", &index, &r, &g, &b) == 4) p = palette+256*4;
            if (sscanf(line, "GRY/%d,%d,%d,%d", &index, &r, &g, &b) == 4) p = palette+256*5;
            if (sscanf(line, "PRC/%d,%d,%d,%d", &index, &r, &g, &b) == 4) p = palette+256*6;
            if (sscanf(line, "PRG/%d,%d,%d,%d", &index, &r, &g, &b) == 4) p = palette+256*7;

            if (p != NULL)
            {
                if ((unsigned)index > 127)
                {
                    result = 2;
                    break;
                }
                p[0].rgbReserved |= 8;
                p[index].rgbReserved |= 1;
                p[index].rgbRed = r;
                p[index].rgbGreen = g;
                p[index].rgbBlue = b;
            }
        } */

        sscanf(line, "IFM/%d", &kapdata.bits_in);

        if ( (s = strstr(line, "DX=")) )
            sscanf(s, "DX=%lf", &kapdata.rx);
        if ( (s = strstr(line, "DY=")) )
            sscanf(s, "DY=%lf", &kapdata.ry);

        // TODO specifically find georeference registration points, border polygon, projection, datum, scale, units, depth sounding datum, etc. a
        // and store in KAPDATA (from REF and KNP lines in header)

        // TODO parse and save all header lines in kapdata.headers_text.


/*         // todo parse name from NA= and number from NU=
        if ((s = strstr(line, "NA=")) )
        {
            *s = 0;
            while (*s && (*s != ',')) s++;
            fprintf(out,"%sNA=%s%s\r\n",line,title,s);
            continue;
        } */

    }
    return true;
}


Status libreadkap::readkap(fread_function freadfn, fseek_function fseekfn, start_function startfn, get_line_function getlinefn)
{

    KAPData kapdata; 

   // read header:
    bool result = read_header(freadfn, kapdata);

    if(!result) return KAPHeaderError;

    assert(kapdata.bits_in != 0);
    assert(kapdata.width != 0);
    assert(kapdata.height != 0);
    if(kapdata.bits_in == 0) return KAPHeaderError;
    if(kapdata.width == 0) return KAPHeaderError;
    if(kapdata.height == 0) return KAPHeaderError;

    // todo let user specify bits_out?  support 16 and 32 bit depth? or always use 8.
    // todo remove support for 1 and 4 bit images?  does 4 bits_out mean 12-bit RGB or 16bit RGBX?
    // does 8 bits_out mean 24-bit RGB or  32 bit RGBx ..... or is output an indexed format?
    kapdata.bits_out = kapdata.bits_in;
    if (kapdata.bits_in > 1)
    {
        if (kapdata.bits_in > 4)  kapdata.bits_out = 8;
        else kapdata.bits_out = 4;
    }

/*     if ((typeout != (int)FIF_TIFF) && (typeout != (int)FIF_ICO) && (typeout != (int)FIF_PNG) && (typeout != (int)FIF_BMP))
        bits_out = 8;     
    bitmap = FreeImage_AllocateEx(width, height, bits_out,palette,FI_COLOR_IS_RGB_COLOR,palette,0,0,0);
    bitmappal = FreeImage_GetPalette(bitmap);
    */

    // TODO we need to figure out palette (colormap) and provide it to user, or when reading data write it to
    // user in something common such as RGB (e.g. 32 bit xRGB -- 0xffRRGGBB).

    uint32_t *index = read_index(freadfn, fseekfn, kapdata.height);
    if (index == NULL)
    {
        return KAPIndexTableError;
    } 


    // call user function with header data. width and height are bitmap size
    //auto limits = startfn(kapdata);
    startfn(kapdata);


/*     // todo truncate?
    assert(limits.maxheight != 0 && kapdata.height <= limits.maxheight);
    assert(limits.maxwidth != 0 && kapdata.width <= limits.maxwidth);
    if(limits.maxheight != 0 && kapdata.height > limits.maxheight) return UserLimitsError;
    if(limits.maxwidth != 0 && kapdata.width > limits.maxwidth) return UserLimitsError;

    // todo convert?
    assert(limits.desired_bitdepth != 0 && kapdata.bits_out == limits.desired_bitdepth);
    if(limits.desired_bitdepth != 0 && kapdata.bits_out != limits.desired_bitdepth) return UserLimitsError; */

    /* uncompress and write each row */
    for (uint16_t r = 0; r < kapdata.height; r++)
    {
        // todo check against limits.maxheight and limits.maxwidth.
        fseekfn(index[r]);
        uint8_t *line = getlinefn((size_t)(kapdata.height - r - 1), kapdata);
        decode_row(freadfn, line, kapdata);
    }

    free(index); 
    

    return NoError;
}
