#include <cstdio>
#include <cstdint>

#include "libreadkap.h"
#include "FreeImage.h"

// read from stdin, write to stdout
int main()
{
    struct {
        FIBITMAP *image;
        libreadkap::KAPData kapdata;
    } data;

    FILE *fp = stdin;

    auto fread_fn = [=](void* buf, size_t size, size_t nmemb) -> size_t {
        return fread(buf, size, nmemb, fp);
    };

    auto fseek_fn = [=](long pos) -> long {
        if(pos < 0)  fseek(fp, pos, SEEK_END);
        else  fseek(fp, pos, SEEK_SET);
        return ftell(fp);
    };

    auto setup_fn = [&data](const libreadkap::KAPData& kapdata) {
        data.image = FreeImage_Allocate(kapdata.width, kapdata.height, kapdata.bits_out);
        printf("Allocated image %dx%d, %d bpp, %u bytes\n", kapdata.width, kapdata.height, kapdata.bits_out, FreeImage_GetMemorySize(data.image));
        assert(data.image);
        data.kapdata = kapdata;
    };

    auto line_fn = [&data](size_t row_index, const libreadkap::KAPData& kapdata) -> uint8_t* {
        //printf("line %lu   \r", row_index);
        return FreeImage_GetScanLine(data.image, row_index);
    };

    FreeImage_Initialise(0);

    libreadkap::readkap(fread_fn, fseek_fn, setup_fn, line_fn);
    
    assert(FreeImage_HasPixels(data.image));
    printf("\nSaving example_output.ppm\n");
    for(int i = 0; i < 20; ++i)
        printf("%hhx ", data.image[i]);
    printf("...\n");
    bool r = FreeImage_Save(FIF_PPM, data.image, "example_output.ppm", PNM_SAVE_ASCII);
    assert(r);
    FreeImage_Unload(data.image);
    FreeImage_DeInitialise();

    return 0;
}