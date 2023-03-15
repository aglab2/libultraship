#include "screenshot.h"

#include <stdint.h>

#include <string>

#include "png.h"

#include <Windows.h>

static void write_png_file(const std::string& file_name, int width, int height, const uint8_t* buffer, bool yflip) {
#pragma warning(disable : 4996)
    FILE* fp = fopen(file_name.c_str(), "wb");
#pragma warning(default : 4996)
    if (!fp)
        return;

    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL) {
        fclose(fp);
        return;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        fclose(fp);
        return;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return;
    }

    png_init_io(png_ptr, fp);

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return;
    }

    png_byte bit_depth = 8;
    png_byte color_type = PNG_COLOR_TYPE_RGB;
    png_set_IHDR(png_ptr, info_ptr, width, height, bit_depth, color_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
                 PNG_FILTER_TYPE_BASE);

    png_write_info(png_ptr, info_ptr);

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return;
    }

    int pixel_size = 3;
    int p = 0;
    png_bytep* row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
    if (yflip) {
        for (int y = height - 1; y >= 0; y--) {
            row_pointers[y] = (png_byte*)malloc(width * pixel_size);
            for (int x = 0; x < width; x++) {
                row_pointers[y][x * pixel_size + 0] = buffer[p++];
                row_pointers[y][x * pixel_size + 1] = buffer[p++];
                row_pointers[y][x * pixel_size + 2] = buffer[p++];
                p++;
            }
        }
    } else {
        for (int y = 0; y < height; y++) {
            row_pointers[y] = (png_byte*)malloc(width * pixel_size);
            for (int x = 0; x < width; x++) {
                row_pointers[y][x * pixel_size + 0] = buffer[p++];
                row_pointers[y][x * pixel_size + 1] = buffer[p++];
                row_pointers[y][x * pixel_size + 2] = buffer[p++];
                p++;
            }
        }
    }
    png_write_image(png_ptr, row_pointers);

    for (int y = 0; y < height; y++) {
        free(row_pointers[y]);
    }
    free(row_pointers);

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return;
    }
    png_write_end(png_ptr, NULL);
    fclose(fp);
}

void save_screenshot(std::string folder, std::string romName, int width, int height,
                    const unsigned char* data, bool yflip) {
    const wchar_t* fileExt = L"png";

    if (folder.size() > 1 && folder[folder.size() - 1] == '\\') {
        folder.resize(folder.size() - 1);
    }

    CreateDirectory(folder.c_str(), NULL);
    for (size_t i = 0, n = romName.size(); i < n; i++) {
        if (romName[i] == ' ') {
            romName[i] = '_';
        } else if (romName[i] == ':') {
            romName[i] = ';';
        } else if (romName[i] == '/') {
            romName[i] = '-';
        }
    }

    auto tim = time(NULL);
    struct tm tm = *localtime(&tim);
    char timeLine[128];
    sprintf(timeLine, "%d-%02d-%02d %02d-%02d-%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    write_png_file(folder + "\\Fast3DShip_" + timeLine + ".png", width, height, data, yflip);
}