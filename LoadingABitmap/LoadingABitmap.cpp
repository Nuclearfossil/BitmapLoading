// LoadingABitmap.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <windows.h>

#include <allegro5\allegro.h>
#include <allegro5\allegro_image.h>
#include <allegro5\allegro_primitives.h>

// -----------------------------------------------------
//  copied from https://msdn.microsoft.com/en-us/library/windows/desktop/dd183374(v=vs.85).aspx
//  for reference
// -----------------------------------------------------
// typedef struct tagBITMAPFILEHEADER {
//     WORD  bfType;
//     DWORD bfSize;
//     WORD  bfReserved1;
//     WORD  bfReserved2;
//     DWORD bfOffBits;
// } BITMAPFILEHEADER;

// ===================================================
// forward declarations
// ===================================================
bool GetImagePixelsRaw(const BITMAPINFOHEADER* infoHeader, BYTE** data, FILE* imageFileHandle, int offsetToData);

// ===================================================
// code time
// ===================================================

bool LoadFromFile(BITMAPFILEHEADER* header, BITMAPINFOHEADER* infoHeader, BYTE** data, const char* filename)
{
    if (header == nullptr) return false;
    if (infoHeader == nullptr) return false;
    if (filename == nullptr) return false;

    FILE* imageFileHandle;
    errno_t err = fopen_s(&imageFileHandle, filename, "rb");
    if (err != 0 || imageFileHandle == nullptr) return false;

    size_t result = fread(header, sizeof(BITMAPFILEHEADER), 1, imageFileHandle);
    if (result != 1 || header->bfType != 0x4D42) return false;
    
    result = fread(infoHeader, sizeof(BITMAPINFOHEADER), 1, imageFileHandle);
    if (result != 1) return false;

    GetImagePixelsRaw(infoHeader, data, imageFileHandle, header->bfOffBits);

    fclose(imageFileHandle);

    return true;
}

bool GetImagePixelsRaw(const BITMAPINFOHEADER* infoHeader, BYTE** data, FILE* imageFileHandle, int offsetToData)
{
    if (infoHeader == nullptr) return false;
    if (imageFileHandle == nullptr) return false;

    int dataSize = infoHeader->biSizeImage;
    if (dataSize == 0)
    {
        // figure it out manually
        dataSize = infoHeader->biWidth * infoHeader->biHeight * (infoHeader->biBitCount / 8);
    }

    int bitcount = infoHeader->biBitCount;

    *data = new BYTE[dataSize];
    fseek(imageFileHandle, offsetToData, SEEK_SET);

    fread(*data, sizeof(BYTE), dataSize, imageFileHandle);
    return true;
}

BYTE* LoadBitmap(const char* filename, int* width, int* height, int* bytesPerPixel)
{
    BITMAPFILEHEADER imageHeader;
    BITMAPINFOHEADER imageInfoHeader;

    BYTE* rawImageData = nullptr;
    if (!LoadFromFile(&imageHeader, &imageInfoHeader, &rawImageData, filename))
    {
        return nullptr;
    }

    *bytesPerPixel = imageInfoHeader.biBitCount / 8;
    // Apparently, not all widths are calculated equally.
    if (imageInfoHeader.biSizeImage > 0)
    {
        *width = imageInfoHeader.biSizeImage / imageInfoHeader.biHeight / *bytesPerPixel;
    }
    else
    {
        *width = imageInfoHeader.biWidth;
    }

    *height = imageInfoHeader.biHeight;

    return rawImageData;
}

void DrawFrameSlow(int width, int height, BYTE* imageData)
{
    unsigned char r;
    unsigned char g;
    unsigned char b;

    // it doesn't get any slower than this for displaying an image. But you get a full view of the image
    // without a lot of opengl cruft
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            b = (unsigned char)imageData[0];
            imageData++;
            g = (unsigned char)imageData[0];
            imageData++;
            r = (unsigned char)imageData[0];
            imageData++;
            al_put_pixel(x, y, al_map_rgb(r, g, b));
        }
    }
}

int main()
{
    int imageWidth = 0;
    int imageHeight = 0;
    int bytesPerPixel = 0;
    BYTE* rawImageData = LoadBitmap("..\\Images\\TestImage_sm.bmp", &imageWidth, &imageHeight, &bytesPerPixel);

    al_init();
    al_init_image_addon();
    al_init_primitives_addon();

    ALLEGRO_DISPLAY* display = al_create_display(800, 600);
    ALLEGRO_EVENT_QUEUE* eventQueue = al_create_event_queue();

    al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
    al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_RGB_888);

    ALLEGRO_BITMAP* texture = al_create_bitmap(imageWidth, imageHeight);
    ALLEGRO_LOCKED_REGION* region = al_lock_bitmap(texture, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_WRITEONLY);
    memcpy(region->data, rawImageData, imageWidth*imageHeight * bytesPerPixel);
    al_unlock_bitmap(texture);

    al_register_event_source(eventQueue, al_get_display_event_source(display));

    al_clear_to_color(al_map_rgb(255, 0, 255));

    while (true)
    {
        ALLEGRO_EVENT event;
        ALLEGRO_TIMEOUT timeout;
        al_init_timeout(&timeout, 0.05);

        bool getEvent = al_wait_for_event_until(eventQueue, &event, &timeout);

        if (getEvent && event.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
        {
            break;
        }

        // DrawFrameSlow(imageWidth, imageHeight, rawImageData);
        al_draw_bitmap(texture, 0.0f, 0.0f, 0);

        al_flip_display();
    }

    al_destroy_bitmap(texture);

    al_destroy_display(display);

    delete rawImageData;

    return 0;
}

