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

    int result = fread(header, sizeof(BITMAPFILEHEADER), 1, imageFileHandle);
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

    int dataSize = infoHeader->biWidth*infoHeader->biHeight * 3; // Assuming 245 bpp

    int bitcount = infoHeader->biBitCount;

    *data = new BYTE[dataSize];
    fseek(imageFileHandle, offsetToData, SEEK_SET);

    int result = fread(*data, sizeof(BYTE), dataSize, imageFileHandle);
    return true;
}

BYTE* LoadBitmap(const char* filename, int* width, int* height)
{
    BITMAPFILEHEADER imageHeader;
    BITMAPINFOHEADER imageInfoHeader;

    BYTE* rawImageData = nullptr;
    if (!LoadFromFile(&imageHeader, &imageInfoHeader, &rawImageData, filename))
    {
        return nullptr;
    }

    *width = imageInfoHeader.biWidth;
    *height = imageInfoHeader.biHeight;

    return rawImageData;
}

void DrawFrame(int width, int height, BYTE* imageData)
{
    unsigned char r;
    unsigned char g;
    unsigned char b;
    for (size_t y = 0; y < height; y++)
    {
        for (size_t x = 0; x < width; x++)
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
    BYTE* rawImageData = LoadBitmap("..\\Images\\TestImage.bmp", &imageWidth, &imageHeight);

    al_init();
    al_init_image_addon();
    al_init_primitives_addon();

    ALLEGRO_DISPLAY* display = al_create_display(800, 600);
    ALLEGRO_EVENT_QUEUE* eventQueue = al_create_event_queue();

    al_register_event_source(eventQueue, al_get_display_event_source(display));

    al_clear_to_color(al_map_rgb(0, 0, 0));

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

        DrawFrame(imageWidth, imageHeight, rawImageData);
        al_flip_display();
    }

    al_destroy_display(display);

    return 0;
}

