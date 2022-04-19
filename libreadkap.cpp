

#include "libreadkap.h"


//#define VERS   "1.16"

#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
//#include <time.h>
#include <cfloat>
#include <cassert>

//#include <FreeImage.h> // todo remove, but there are still some symbols and types used from here

using namespace libreadkap;


#if 0
/* color type and mask */
typedef union
{
    RGBQUAD  q;
    uint32_t p;
} Color32;
#define RGBMASK 0x00FFFFFF
#endif

/* --- Copy this part in header for use imgkap functions in programs and define LIBIMGKAP*/

#define METTERS     0
#define FATHOMS     1
#define FEET        2

#define NORMAL      0
#define OLDKAP      1

#define COLOR_NONE  1
#define COLOR_IMG   2
#define COLOR_MAP   3
#define COLOR_KAP   4

#define FIF_KAP     1024
#define FIF_NO1     1025
#define FIF_TXT     1026
#define FIF_KML     1027


/* --- End copy */


static const double WGSinvf                           = 298.257223563;       /* WGS84 1/f */
static const double WGSexentrik                       = 0.081819;             /* e = 1/WGSinvf; e = sqrt(2*e -e*e) ;*/

/* struct structlistoption
{
    char const *name;
    int val;
} ;

static struct structlistoption imagetype[] =
{
    {"KAP",FIF_KAP},
    {"NO1",FIF_NO1},
    {"KML",FIF_KML},
    {"TXT",FIF_TXT},
    {NULL, FIF_UNKNOWN},
} ;

static struct structlistoption listoptcolor[] =
{
    {"NONE",COLOR_NONE},
    {"KAP",COLOR_KAP},
    {"IMG",COLOR_IMG},
    {"MAP",COLOR_MAP},
    {NULL,COLOR_NONE}
} ;

int findoptlist(struct structlistoption *liste,char *name)
{
    while (liste->name != NULL)
    {
        if (!strcasecmp(liste->name,name)) return liste->val;
        liste++;
    }
    return liste->val;
}

int findfiletype(char *file)
{
    char *s ;

    s = file + strlen(file)-1;
    while ((s > file) && (*s != '.')) s--;
    s++;
    return findoptlist(imagetype,s);
} */


double heading_between(double lat0, double lon0, double lat1, double lon1)
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


double longitude_to_x_wgs84(double l)
{
    return l*M_PI/180;
}

double latitude_to_y_wgs84(double l)
{
    double e = WGSexentrik;
    //double s = sinl(l*M_PI/180.0);
    double s = sin(l*M_PI/180.0);

    return log(tan(M_PI/4 + l * M_PI/360)*pow((1. - e * s)/(1. + e * s), e/2.));
}




#if 0
/*------------------ Single Memory algorithm used by histogram algorithms ------------------*/

#define MYBSIZE 1572880

typedef struct smymemory
{
    struct smymemory *next;
    uint32_t size;
} mymemory;

static mymemory *mymemoryfirst = 0;
static mymemory *mymemorycur = 0;

void * myalloc(int size)

{
    void *s = NULL;
    mymemory *mem = mymemorycur;

    if (mem && ((mem->size + size) > MYBSIZE)) mem = 0;

    if (!mem)
    {
        mem = (mymemory *)calloc(MYBSIZE,1);
        if (mem == NULL) return 0;
        mem->size = sizeof(mymemory);

        if (mymemorycur) mymemorycur->next = mem;
        mymemorycur = mem;
        if (!mymemoryfirst) mymemoryfirst = mem;
    }

    s = ((int8_t *)mem + mem->size);
    mem->size += size;
    return s;
}

void myfree(void)

{
    struct smymemory *mem, *next;

    mem = mymemoryfirst;
    while (mem)
    {
        next = mem->next;
        free(mem);
        mem = next;
    }
    mymemoryfirst = 0;
    mymemorycur = 0;
}


/*------------------ Histogram algorithm ------------------*/

typedef struct
{
    Color32 color;
    uint32_t count;
    int16_t num;
} helem;

typedef struct shistogram
{
    Color32 color;
    uint32_t count;
    int16_t num;
    int16_t used;
    struct shistogram *child ;
} histogram;


#define HistIndex2(p,l) ((((p.q.rgbRed >> l) & 0x03) << 4) | (((p.q.rgbGreen >> l) & 0x03) << 2) |    ((p.q.rgbBlue >> l) & 0x03) )
#define HistSize(l) (l?sizeof(histogram):sizeof(helem))
#define HistInc(h,l) (histogram *)(((char *)h)+HistSize(l))
#define HistIndex(h,p,l) (histogram *)((char *)h+HistSize(l)*HistIndex2(p,l))

static histogram *HistAddColor (histogram *h, Color32 pixel )
{
    char level;

    for (level=6;level>=0;level -=2)
    {
        h = HistIndex(h,pixel,level) ;

        if (h->color.p == pixel.p) break;
        if (!h->count && !h->num)
        {
            h->color.p = pixel.p;
            break;
        }
        if (!h->child)
        {
            h->child = (histogram *)myalloc(HistSize(level)*64);
            if (h->child == NULL) return 0;
        }
        h = h->child;
    }

    h->count++;
    return h;
}

static int HistGetColorNum (histogram *h, Color32 pixel)
{
    char level;

    for (level=6;level>=0;level -=2)
    {
        /* 0 < index < 64 */
        h = HistIndex(h,pixel,level) ;
        if (h->color.p == pixel.p) break;
        if (!level)
            break; // erreur
        if (!h->child) break;
        h = h->child;
    }
    if (h->num < 0) return -1-h->num;
    return h->num-1;
}

#define HistColorsCount(h) HistColorsCountLevel(h,6)

static int32_t HistColorsCountLevel (histogram *h,int level)
{
    int i;
    uint32_t count = 0;

    for (i=0;i<64;i++)
    {
        if (h->count) count++;
        if (level)
        {
            if(h->child) count += HistColorsCountLevel(h->child,level-2);
        }
        h = HistInc(h,level);
    }
    return count;
}


/*--------------- reduce begin -------------*/

typedef struct
{
    histogram   *h;

    int32_t     nbin;
    int32_t     nbout;

    int32_t     colorsin;
    int32_t     colorsout;

    int         nextcote;
    int         maxcote;
    int         limcote[8];

    uint64_t    count;
    uint64_t    red;
    uint64_t    green;
    uint64_t    blue;

} reduce;

static inline int HistDist(Color32 a, Color32 b)
{
   int c,r;

   c = a.q.rgbRed - b.q.rgbRed;
   r = c*c;

   c = a.q.rgbGreen - b.q.rgbGreen;
   r += c*c;

   c = a.q.rgbBlue - b.q.rgbBlue;
   r += c*c;

   return sqrt(r);
}

static int HistReduceDist(reduce *r, histogram *h, histogram *e, int cote, int level)
{
    int i;
    int used = 1;
    int curcote;
    int limcote = r->limcote[level];

    for (i=0;i<64;i++)
    {
        if (h->count && !h->num)
        {

            curcote = HistDist((Color32)e->color,(Color32)h->color);

            if (curcote <= cote)
            {
                    uint64_t c;

                    c = h->count;

                    r->count += c;
                    r->red += c * (uint64_t)((Color32)h->color).q.rgbRed ;
                    r->green +=  c * (uint64_t)((Color32)h->color).q.rgbGreen;
                    r->blue +=  c * (uint64_t)((Color32)h->color).q.rgbBlue;

                    h->num = r->nbout;
                    h->count = 0;
                    r->nbin++;
            }
            else
            {
                    if (r->nextcote > curcote)
                        r->nextcote = curcote;
                    used = 0;
            }
        }
        if (level && h->child && !h->used)
        {
            limcote += cote ;

            curcote = HistDist((Color32)e->color,(Color32)h->color);

            if (curcote <= limcote)
                h->used = HistReduceDist(r,h->child,e,cote,level-2);
            if (!h->used)
            {
                if ((curcote > limcote) && (r->nextcote > limcote))
                    r->nextcote = curcote ;
                used = 0;
            }
            limcote -= cote ;
        }
        h = HistInc(h,level);
    }
    return used;
}

static void HistReduceLevel(reduce *r, histogram *h, int level)
{
    int i;

    for (i=0;i<64;i++)
    {
        if (level && h->child && !h->used)
        {
            HistReduceLevel(r, h->child,level-2);
            if (!r->colorsout) break;
        }

        if (h->count && !h->num)
        {
            int32_t cote = 0;
            int32_t nbcolors;
            int32_t curv;

            r->count = r->red = r->green = r->blue = 0;
            r->nbin = 0;
            r->nextcote = 0;
            r->nbout++;

            cote = (int32_t)(pow((double)((1<<24)/(double)r->colorsout),1.0/3.0)/2); //-1;
            r->maxcote = sqrt(3*cote*cote);

            curv = 0;
            nbcolors = (r->colorsin +r->colorsout -1)/r->colorsout;

            while (r->nbin < nbcolors)
            {
                curv += nbcolors - r->nbin;
                cote = (int32_t)(pow(curv,1.0/3.0)/2); // - 1;
                cote = sqrt(3*cote*cote);

                if (r->nextcote > cote)
                    cote = r->nextcote;

                r->nextcote = r->maxcote+1;

                if (cote >= r->maxcote)
                        break;

                h->used = HistReduceDist(r,r->h,h,cote,6);

                if (!r->count)
                {
                    fprintf(stderr,"Erreur quantize\n");
                    return;
                }
            }

            r->colorsin -= r->nbin;
            r->colorsout--;
            {
                histogram *e;
                Color32 pixel ;
                uint64_t c,cc;

                c = r->count; cc = c >> 1 ;
                pixel.q.rgbRed = (uint8_t)((r->red + cc) / c);
                pixel.q.rgbGreen = (uint8_t)((r->green + cc) / c);
                pixel.q.rgbBlue = (uint8_t)((r->blue + cc) / c);
                pixel.q.rgbReserved = 0;

                e = HistAddColor(r->h,pixel);
                e->count = r->count;
                e->num = -r->nbout;

            }

            if (!r->colorsout) break;
        }
        h = HistInc(h,level);
    }

}

static int HistReduce(histogram *h, int colorsin, int colorsout)
{
    reduce r;

    r.h = h;

    r.nbout = 0;

    if (!colorsout || !colorsin) return 0;

    if (colorsout > 0x7FFF) colorsout = 0x7FFF;
    if (colorsout > colorsin) colorsout = colorsin;
    r.colorsin = colorsin;
    r.colorsout = colorsout;

    r.limcote[2] = sqrt(3*3*3) ;
    r.limcote[4] = sqrt(3*15*15) ;
    r.limcote[6] = sqrt(3*63*63) ;

    HistReduceLevel(&r,h,6);

    return r.nbout;
}

/*--------------- reduce end -------------*/


static int _HistGetList(histogram *h,helem **e,int nbcolors,char level)
{
    int i;
    int nb;

    nb = 0;
    for (i=0;i<64;i++)
    {
        if (h->count && (h->num < 0))
        {
            e[-1-h->num] = (helem *)h;
            nb++;
        }

        if (level && h->child) nb += _HistGetList(h->child,e,nbcolors-nb,level-2);
        if (nb > nbcolors)
                return 0;
        h = HistInc(h,level);
    }
    return nb;
}



static int HistGetPalette(uint8_t *colorskap,uint8_t *colors,Color32 *palette,histogram *h,int nbcolors, int nb, int optcolors, Color32 *imgpal,int maxpal)
{
    int i,j;
    helem *t,*e[128];
    uint8_t numpal[128];

    /* get colors used */
    if ((i= _HistGetList(h,e,nbcolors,6)) != nbcolors)
    {
        fprintf(stderr, "Can't process the palette, reduce it before using imgkap.\n");
        return 0;
    }

    /* load all color in final palette */
    memset(numpal,0,sizeof(numpal));
    if (!imgpal)
    {
        for (i=0;i<nbcolors;i++)
        {
            if (!(palette[i].q.rgbReserved & 1)) palette[i].p = e[i]->color.p;
            palette[i].q.rgbReserved |= 1;
            colors[i] = i;
            numpal[i] = i;
        }
        palette->q.rgbReserved |= 8;
        maxpal = nbcolors;
    }
    else
    {
        for (i=maxpal-1;i>=0;i--)
        {
            j = HistGetColorNum(h,imgpal[i]);
            if (j>=0)
            {
                if (!(palette[i].q.rgbReserved & 1))
                    palette[i].p = imgpal[i].p;
                palette[i].q.rgbReserved |= 1;
                numpal[j] = i;
                colors[i] = j;
            }
        }
        palette->q.rgbReserved |= 8;
    }

    /* sort palette desc count */
    for (i=0;i<nbcolors;i++)
    {
        for (j=i+1;j<nbcolors;j++)
            if (e[j]->count > e[i]->count)
            {
                t =  e[i];
                e[i] = e[j];
                e[j] = t;
            }
    }
    /* if palkap 0 put first in last */
    if (nb)
    {
        nb=1;
        t =  e[0];
        e[0] = e[nbcolors-1];
        e[nbcolors-1] = t;
    }

    /* get kap palette colors */
    colorskap[0] = 0;
    for (i=0;i<nbcolors;i++)
        colorskap[i+nb] = numpal[-1-e[i]->num];

    /* get num colors in kap palette */
    for (i=0;i<maxpal;i++)
    {
        for (j=0;j<nbcolors;j++)
            if (colors[i] == (-1 - e[j]->num))
                break;
        colors[i] = j+nb;
    }

    /* taitement img && map sur colorskap */
    if ((optcolors == COLOR_IMG) || (optcolors == COLOR_MAP))
    {
        for(i=0;i<maxpal;i++)
        {

            palette[256+i].q.rgbRed = palette[i].q.rgbRed ;
            palette[256+i].q.rgbGreen = palette[i].q.rgbGreen ;
            palette[256+i].q.rgbBlue = palette[i].q.rgbBlue ;
            palette[512+i].q.rgbRed = (palette[i].q.rgbRed)/2 ;
            palette[512+i].q.rgbGreen = (palette[i].q.rgbGreen)/2 ;
            palette[512+i].q.rgbBlue = (palette[i].q.rgbBlue)/2 ;
            palette[768+i].q.rgbRed = (palette[i].q.rgbRed)/4 ;
            palette[768+i].q.rgbGreen = (palette[i].q.rgbGreen)/4 ;
            palette[768+i].q.rgbBlue = (palette[i].q.rgbBlue)/4 ;
            palette[768+i].q.rgbReserved = palette[512+i].q.rgbReserved = palette[256+i].q.rgbReserved = palette[i].q.rgbReserved ;
        }

        if ((optcolors == COLOR_MAP) && (nbcolors < 64))
        {
            Color32 *p = palette+768;

            for(i=0;i<maxpal;i++)
            {
                if ((p->q.rgbRed <= 4) && (p->q.rgbGreen <= 4) && (p->q.rgbBlue <= 4))
                    p->q.rgbRed = p->q.rgbGreen = p->q.rgbBlue = 55;

                if ((p->q.rgbRed >= 60) && (p->q.rgbGreen >= 60) && (p->q.rgbBlue >= 60))
                    p->q.rgbRed = p->q.rgbGreen = p->q.rgbBlue = 0;
                p++;
            }


        }
    }
/*
    for (i=0;i<nbcolors;i++)
    {
        printf("eorder %d rgb %d %d %d\n",i,e[i]->color.q.rgbRed,e[i]->color.q.rgbGreen,e[i]->color.q.rgbBlue);
    }
    for (i=0;i<maxpal;i++)
    {
        printf("palette %d rgb %d %d %d\n",i,palette[i].q.rgbRed,palette[i].q.rgbGreen,palette[i].q.rgbBlue);
    }
    for (i=0;i<nbcolors+nb;i++)
    {
        j = colorskap[i];
        printf("palkap %d rgb %d %d %d\n",i,palette[j].q.rgbRed,palette[j].q.rgbGreen,palette[j].q.rgbBlue);
    }
    for (i=0;i<maxpal;i++)
    {
        printf("indexcol %i colors : %d\n",i,colors[i]);
    }
*/
    nbcolors += nb;
    return nbcolors;
}

#define HistFree(h) myfree()

/*------------------ End of Histogram ------------------*/
#endif

#if 0
typedef struct shsv
{
    double hue;
    double sat;
    double val;
} HSV;
#endif

/* read in kap file */

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

/* function read and write kap index */
#if 0
static int bsb_write_index(FILE *fp, uint16_t height, uint32_t *index)
{
    uint8_t l;

        /* Write index table */
        while (height--)
        {
            /* Indices must be written as big-endian */
            l = (*index >> 24) & 0xff;
            fputc(l, fp);
            l = (*index >> 16) & 0xff;
            fputc(l, fp);
            l = (*index >> 8) & 0xff;
            fputc(l, fp);
            l = *index & 0xff;
            fputc(l, fp);
            index++;
        }
        return 1;
} 
#endif


static uint32_t *bsb_read_index(fread_function freadfn, fseek_function fseekfn, uint16_t height)
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


/* bsb uncompress number */

static uint16_t bsb_uncompress_nb(fread_function readfn, uint8_t *pixel, uint8_t decin, uint8_t maxin)
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

/* read line bsb */

static int bsb_uncompress_row(fread_function &readfn, uint8_t *line, const KAPData &kapdata)
//(int typein, FILE *in, uint8_t *buf_out, uint16_t bits_in,uint16_t bits_out, uint16_t width)
{
    uint16_t    count;
    uint8_t     pixel;
    uint8_t     decin, maxin;
    uint16_t    xout = 0;

    decin = (uint8_t)(7-kapdata.bits_in);
    maxin = (uint8_t)((1<<decin) - 1);

    /* read the line number */
    count = bsb_uncompress_nb(readfn, &pixel, 0, 0x7F); //(typein, in,&pixel,0,0x7F);

    /* no test count = line number */
    uint16_t w = kapdata.width;

    // todo support only 8 bit depth
    switch (kapdata.bits_out)
    {
        case 1:
            while (w)
            {
                count = bsb_uncompress_nb(readfn, &pixel, decin, maxin); //typein,in,&pixel, decin,maxin);
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
                count = bsb_uncompress_nb(readfn,&pixel, decin,maxin);
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
                count = bsb_uncompress_nb(readfn,&pixel, decin,maxin);
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


static uint32_t GetHistogram(FIBITMAP *bitmap,uint32_t bits,uint16_t width,uint16_t height,Color32 *pal,histogram *hist)
{
    uint32_t    i,j;
    Color32     cur;
    uint8_t     *line,k;
    histogram   *h = hist;

    switch (bits)
    {
        case 1:
            HistAddColor (hist, pal[0]);
            h = HistAddColor (hist, pal[1]);
            h->count++;
            break;

        case 4:
            for (i=0;i<height;i++)
            {
                line = FreeImage_GetScanLine(bitmap, i);
                cur.p = (width+1)>>1;
                for (j=0;j<cur.p;j++)
                {
                    k = (*line++)>>4;
                    if (h->color.p == pal[k].p)
                    {
                        h->count++;
                        continue;
                    }
                    h = HistAddColor (hist, pal[k]);
                }
                line = FreeImage_GetScanLine(bitmap, i);
                cur.p = width >> 1;
                for (j=0;j<cur.p;j++)
                {
                    k = (*line++)&0x0F;
                    if (h->color.p == pal[k].p)
                    {
                        h->count++;
                        continue;
                    }
                    h = HistAddColor (hist, pal[k]);
                }
            }
            break;

        case 8:
            for (i=0;i<height;i++)
            {
                line = FreeImage_GetScanLine(bitmap, i);
                for (j=0;j<width;j++)
                {
                    k = *line++ ;
                    if (h->color.p == pal[k].p)
                    {
                        h->count++;
                        continue;
                    }
                    h = HistAddColor (hist, pal[k]);
                }
            }
            break;

        case 24:
            for (i=0;i<height;i++)
            {
                line = FreeImage_GetScanLine(bitmap, i);
                for (j=0;j<width;j++)
                {
                    cur.p = *(uint32_t *)(line) & RGBMASK;
                    line += 3;
                    if (h->color.p == cur.p)
                    {
                        h->count++;
                        continue;
                    }
                    h = HistAddColor (hist, cur);
                }
            }
            break;
    }

    return HistColorsCount(hist);
} */

//static const char *colortype[] = {"RGB","DAY","DSK","NGT","NGR","GRY","PRC","PRG"};



static bool readkapheader(fread_function freadfn, KAPData& kapdata)
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

        /*
        if ((out != NULL) && (typeout != FIF_UNKNOWN))
        {
            if (typeout != FIF_TXT)
            {
                if (!strncmp(line,"RGB/",4)) continue;
                if (!strncmp(line,"DAY/",4)) continue;
                if (!strncmp(line,"DSK/",4)) continue;
                if (!strncmp(line,"NGT/",4)) continue;
                if (!strncmp(line,"NGR/",4)) continue;
                if (!strncmp(line,"GRY/",4)) continue;
                if (!strncmp(line,"PRC/",4)) continue;
                if (!strncmp(line,"PRG/",4)) continue;
            }

            if ((*line == '!') && strstr(line,"M'dJ")) continue;
            if ((*line == '!') && strstr(line,"imgkap")) continue;

            if (!strncmp(line,"VER/",4) && ((optcolor == COLOR_IMG) || (optcolor == COLOR_MAP)))
            {
                fprintf(out,"VER/3.0\r\n");
                continue;
            }

            if (!strncmp(line,"OST/",4)) continue;
            if (!strncmp(line,"IFM/",4)) continue;

            if ((s = strstr(line, "ED=")) && (date != NULL))
            {
                *s = 0;
                while (*s && (*s != ',')) s++;
                fprintf(out,"%sED=%s%s\r\n",line,date,s);
                continue;

            }
        */

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


Status readkap(fread_function freadfn, fseek_function fseekfn, start_function startfn, get_line_function getlinefn)
{

    //RGBQUAD *bitmappal;
    //header = NULL;

/*  RGBQUAD palette[256*8];
    memset(palette,0,sizeof(palette));
    char *optionpal = opts.palette;
    // Note, palette type "ALL" generates multiple images in the TIFF, todo remove support?
    if (optionpal && !strcasecmp(optionpal,"ALL") && (typeout != (int)FIF_TIFF) && (typeout != (int)FIF_GIF))
    {
        typeout = FIF_TIFF;

        fprintf(stderr,"ERROR - Palette ALL accepted with only TIF or GIF %s\n",fileout);
        return 2;
    } 
*/

    KAPData kapdata;
   // read header:
    bool result = readkapheader(freadfn, kapdata);

    if(!result) return KAPHeaderError;

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


    uint32_t *index = bsb_read_index(freadfn, fseekfn, kapdata.height);
    if (index == NULL)
    {
        return KAPIndexTableError;
    } 


    // call user function with header data. width and height are bitmap size
    auto user_info = startfn(kapdata);

    // todo truncate?
    assert(kapdata.height <= user_info.maxheight);
    assert(kapdata.width <= user_info.maxwidth);
    if(kapdata.height > user_info.maxheight) return UserLimitsError;
    if(kapdata.width > user_info.maxwidth) return UserLimitsError;

    // todo convert?
    assert(kapdata.bits_out == user_info.desired_bitdepth);
    if(kapdata.bits_out != user_info.desired_bitdepth) return UserLimitsError;

    /* uncompress and write each row */
    for (uint16_t r = 0; r < kapdata.height; r++)
    {
        // todo check against user_info.maxheight and user_info.maxwidth.
        fseekfn(index[r]);
        uint8_t *line = getlinefn((size_t)(kapdata.height - r - 1), kapdata);
        bsb_uncompress_row(freadfn, line, kapdata);
    }

    free(index); 
    
/*  TODO select palette:
    int cpt = 0;
    if (optionpal)
    {
        cpt = -2;
        if (!strcasecmp(optionpal,"ALL")) cpt = -1;
        if (!strcasecmp(optionpal,"RGB")) cpt = 0;
        if (!strcasecmp(optionpal,"DAY")) cpt = 1;
        if (!strcasecmp(optionpal,"DSK")) cpt = 2;
        if (!strcasecmp(optionpal,"NGT")) cpt = 3;
        if (!strcasecmp(optionpal,"NGR")) cpt = 4;
        if (!strcasecmp(optionpal,"GRY")) cpt = 5;
        if (!strcasecmp(optionpal,"PRC")) cpt = 6;
        if (!strcasecmp(optionpal,"PRG")) cpt = 7;
        if (cpt == -2)
        {
            fprintf(stderr,"ERROR - Palette %s not exist in %s\n",optionpal,filein);
            FreeImage_Unload(bitmap);
            return 2;
        }
    }
    if (cpt >= 0)
    {
        if (cpt > 0)
        {
            RGBQUAD *pal = palette + 256*cpt;
            if (!pal->rgbReserved)
            {
                fprintf(stderr,"ERROR - Palette %s not exist in %s\n",optionpal,filein);
                FreeImage_Unload(bitmap);
                return 2;
            }
            for (result=0;result<256;result++) pal[result].rgbReserved = 0;
            memcpy(bitmappal,pal,sizeof(RGBQUAD)*256);
        }

        result = FreeImage_Save((FREE_IMAGE_FORMAT)typeout,bitmap,fileout,0);
    }
    else
    {
        FIMULTIBITMAP *multi;
        RGBQUAD *pal;

        multi = FreeImage_OpenMultiBitmap((FREE_IMAGE_FORMAT)typeout,fileout,TRUE,FALSE,TRUE,0);
        if (multi == NULL)
        {
            fprintf(stderr,"ERROR - Alloc multi bitmap\n");
            FreeImage_Unload(bitmap);
            return 2;
        }
        for (cpt=0;cpt<8;cpt++)
        {
            pal = palette + 256*cpt;
            if (pal->rgbReserved)
            {
                for (result=0;result<256;result++) pal[result].rgbReserved = 0;
                memcpy(bitmappal,pal,sizeof(RGBQUAD)*256);
                FreeImage_AppendPage(multi,bitmap);
            }
        }
        FreeImage_CloseMultiBitmap(multi,0);
        result = 1;
    } */


    return NoError;
}
