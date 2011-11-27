#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <conio.h>
#include <stdlib.h>
#include <stdio.h>

FILE *fout;
COLORREF trans = RGB(255, 0, 255);

typedef struct SPRITE_TYPE {
    int width;
    int height;
    COLORREF *dat;
} SPRITE;

typedef struct RLESPRITE_TYPE {
    int w, h;
    int size;
    DWORD *dat;
} RLESPRITE;

void create_sprite(SPRITE *sprite, int width, int height)
{
    sprite->width = width;
    sprite->height = height;
    sprite->dat = (COLORREF *)malloc(width * height * sizeof(COLORREF));
}

void delete_sprite(SPRITE *sprite)
{
    sprite->width = 0;
    sprite->height = 0;
    free(sprite->dat);
}

COLORREF getpixel(SPRITE *sprite, int x, int y)
{
    return sprite->dat[(y * sprite->width) + x];
}

int row_getcount_transparent(SPRITE *sprite, int y)
{
    int run = 0;
    int len = 0;
    int x = 0;
    int width = sprite->width;
    
    x++;
    len++; // For transparent run count
    while (x < width) {
        while ((x < width) && (getpixel(sprite, x, y) == trans)) {
            x++;
        }
        if (x == width) {
            if (run > 0) {
                len--;
            }
            break;
        }
        len++; // For opaque run count
        run = 1;
        x++;
        while ((x < width) && (getpixel(sprite, x, y) != trans)) {
            run++;
            x++;
        }
        len += run;  // For actual data following Run Count
        if (x < width) {
            len++;    // Because row hasn't ended yet, there HAS
        }
        // to be another transparent run count
    }
    len++;  // For 0x00 Row End data
    
    return len;
}

int row_getcount_opaque(SPRITE *sprite, int y)
{
    int run = 0;
    int len = 0;
    int x = 0;
    int width = sprite->width;
    
    x++;
    len++; // For 0x00 identifying no starting transparency
    len++; // For Run Count of opaque pixels
    run = 1;
    while (x < width) {
        while ((x < width) && (getpixel(sprite, x, y) != trans)) {
            run++;
            x++;
        }
        len += run; // For actual data following Run Count
        if (x == width) {
            break;
        }
        len++; // For Transparent run count
        x++;
        while ((x < width) && (getpixel(sprite, x, y) == trans)) {
            x++;
        }
        run = 0;
        if (x < width) {
            len++; // Because the row hasn't ended yet, there
            // HAS to be another opaque run count
        } else {
            len--;
            break;
        }
        
    }
    len++; // For 0x00 Row End data
    
    return len;
}

void encode_row_transparent(SPRITE *sprite, RLESPRITE *rlesprite, int y, int *ind)
{
    int run = 0;
    int tmp = 0;
    int x = 0;
    int width = sprite->width;
    BOOL onlyt = TRUE;
    COLORREF col;
    
    x++;
    run++;
    while (x < width) {
        while ((x < width) && (getpixel(sprite, x, y) == trans)) {
            x++;
            run++;
        }
        if (x < width) {
            rlesprite->dat[*ind] = run;
            (*ind)++;
        }
        if (x == width) {
            if (onlyt) {
                rlesprite->dat[*ind] = run;
                (*ind)++;
            }
            break;
        }
        onlyt = FALSE;
        run = 1;
        x++;
        while ((x < width) && (getpixel(sprite, x, y) != trans)) {
            run++;
            x++;
        }
        rlesprite->dat[*ind] = run;
        (*ind)++;
        for (tmp = x - run; tmp < x; tmp++) {
            col = getpixel(sprite, tmp, y);
            rlesprite->dat[*ind] = col;
            (*ind)++;
        }
        run = 0;
    }
    rlesprite->dat[*ind] = 0;
    (*ind)++;
}

void encode_row_opaque(SPRITE *sprite, RLESPRITE *rlesprite, int y, int *ind)
{
    int run = 0;
    int tmp = 0;
    int x = 0;
    int width = sprite->width;
    COLORREF col;
    
    x++;
    rlesprite->dat[*ind] = 0;
    (*ind)++;
    run = 1;
    while (x < width) {
        while ((x < width) && (getpixel(sprite, x, y) != trans)) {
            run++;
            x++;
        }
        rlesprite->dat[*ind] = run;
        (*ind)++;
        for (tmp = x - run; tmp < x; tmp++) {
            col = getpixel(sprite, tmp, y);
            rlesprite->dat[*ind] = col;
            (*ind)++;
        }
        if (x == width) {
            break;
        }
        run = 1;
        x++;
        while ((x < width) && (getpixel(sprite, x, y) == trans)) {
            run++;
            x++;
        }
        if (x < width) {
            rlesprite->dat[*ind] = run;
            (*ind)++;
        }
        run = 0;
    }
    rlesprite->dat[*ind] = 0;
    (*ind)++;
}

void sprite_to_rle(SPRITE *sprite, RLESPRITE *rlesprite)
{
    int run = 0;
    int len = 0;
    int ind = 0;
    int tmp = 0;
    int y;
    int width = sprite->width;
    int height = sprite->height;
    
    // Compute the length of the .dat field in the RLESPRITE
    for (y = 0; y < height; y++) {
        if (getpixel(sprite, 0, y) == trans) {
            len += row_getcount_transparent(sprite, y);
        } else {
            len += row_getcount_opaque(sprite, y);
        }
    }
    
    // Initialize the RLESPRITE
    rlesprite->w = width;
    rlesprite->h = height;
    rlesprite->size = len;
    rlesprite->dat = (DWORD *)malloc(len * sizeof(DWORD));
    
    run = 0;
    len = 0;
    ind = 0;
    
    // Encode the SPRITE data into the RLESPRITE
    for (y = 0; y < height; y++) {
        if (getpixel(sprite, 0, y) == trans) {
            encode_row_transparent(sprite, rlesprite, y, &ind);
        } else {
            encode_row_opaque(sprite, rlesprite, y, &ind);
        }
    }
}

void write_rle_tofile(RLESPRITE *rs)
{
    fwrite(rs, 3 * sizeof(int), 1, fout);
    fwrite(rs->dat, rs->size * sizeof(DWORD), 1, fout);
}

void main(int argc, char *argv[])
{
    HBITMAP hbm;
    BITMAP  bm;
    HDC     hdc;
    int     i_width = 0,
            i_height = 0;
    int     num_x = 0,
            num_y = 0;
    int     ix = 0,
            iy = 0;
    SPRITE   sprite;
    RLESPRITE rlesprite;
    int     x = 0,
            y = 0;
    int     count = 0;
    
    if (argc < 4) {
        printf("\nMissing Parameters!\n");
        exit(1);
    }
    
    i_width = atoi(argv[3]);
    i_height = atoi(argv[4]);
    printf("width : %d\nheight : %d", i_width, i_height);
    
    hbm = (HBITMAP)LoadImage(NULL, argv[1], IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
    if (hbm == NULL) {
        exit(1);
    }
    
    GetObject(hbm, sizeof(bm), &bm);      // get size of bitmap
    
    num_x = bm.bmWidth / i_width;
    num_y = bm.bmHeight / i_height;
    
    hdc = CreateCompatibleDC(NULL);
    if (!hdc) {
        printf("\nCreateCompatibleDC() Failed!\n");
        exit(1);
    }
    SelectObject(hdc, hbm);
    
    fout = fopen(argv[2], "wb");
    count = (num_x * num_y);
    fwrite(&count, sizeof(int), 1, fout);
    
    for (iy = 0; iy < num_y; iy++) {
        for (ix = 0; ix < num_x; ix++) {
            create_sprite(&sprite, i_width, i_height);
            for (y = 0; y < i_height; y++)
                for (x = 0; x < i_width; x++) {
                    sprite.dat[(y * i_width) + x] = GetPixel(hdc, (ix * i_width) + x, (iy * i_height) + y);
                }
            sprite_to_rle(&sprite, &rlesprite);
            write_rle_tofile(&rlesprite);
            free(rlesprite.dat);
            delete_sprite(&sprite);
        }
    }
    
    fclose(fout);
    
    DeleteObject(hbm);
    DeleteDC(hdc);
}