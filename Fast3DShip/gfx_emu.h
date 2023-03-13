#pragma once

#include <stdint.h>
#include <string>

#include "resource/type/Texture.h"

// #define MAX_LIGHTS 2
#define MAX_LIGHTS 32

struct EmuLight {
    unsigned char pad0, b, g, r;
    unsigned char pad1, b2, g2, r2;
    char pad2, z, y, x;
};

struct RawTexMetadata {
    uint16_t width, height;
    float h_byte_scale = 1, v_pixel_scale = 1;
    std::string name;
    Ship::TextureType type;
};

extern uintptr_t segmentPointers[16];

void gfx_matrix_mul(float res[4][4], const float a[4][4], const float b[4][4]);
void gfx_sp_matrix(bool projection, bool load, bool push, const int32_t* addr);
void gfx_sp_pop_matrix(uint32_t count);
void gfx_sp_vertex(size_t n_vertices, size_t dest_index, const Vtx* vertices);
void gfx_sp_modify_vertex(uint16_t vtx_idx, uint8_t where, uint32_t val);
void gfx_sp_tri1(uint8_t vtx1_idx, uint8_t vtx2_idx, uint8_t vtx3_idx, bool is_rect);
void gfx_sp_geometry_mode(uint32_t clear, uint32_t set);
void gfx_calc_and_set_viewport(const Vp_t* viewport);

void gfx_sp_mv_current_lights(int idx, const EmuLight* emu_light);
void gfx_sp_mv_lookat(const EmuLight* emu_light);
void gfx_sp_set_num_lights(uint8_t num);
void gfx_sp_set_fog(int16_t mul, int16_t off);
void gfx_set_segment(int idx, uint32_t data);

void gfx_sp_texture(uint16_t sc, uint16_t tc, uint8_t level, uint8_t tile, uint8_t on);
void gfx_dp_set_scissor(uint32_t mode, uint32_t ulx, uint32_t uly, uint32_t lrx, uint32_t lry);
void gfx_dp_set_texture_image(uint32_t format, uint32_t size, uint32_t width, const char* texPath, uint32_t texFlags,
                              RawTexMetadata rawTexMetdata, const void* addr);
void gfx_dp_set_tile(uint8_t fmt, uint32_t siz, uint32_t line, uint32_t tmem, uint8_t tile, uint32_t palette,
                     uint32_t cmt, uint32_t maskt, uint32_t shiftt, uint32_t cms, uint32_t masks, uint32_t shifts);
void gfx_dp_set_tile_size(uint8_t tile, uint16_t uls, uint16_t ult, uint16_t lrs, uint16_t lrt);
void gfx_dp_load_tlut(uint8_t tile, uint32_t high_index);
void gfx_dp_load_block(uint8_t tile, uint32_t uls, uint32_t ult, uint32_t lrs, uint32_t dxt);
void gfx_dp_load_tile(uint8_t tile, uint32_t uls, uint32_t ult, uint32_t lrs, uint32_t lrt);
void gfx_dp_set_combine_mode(uint32_t rgb, uint32_t alpha, uint32_t rgb_cyc2, uint32_t alpha_cyc2);
void gfx_dp_set_grayscale_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void gfx_dp_set_env_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void gfx_dp_set_prim_color(uint8_t m, uint8_t l, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void gfx_dp_set_fog_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void gfx_dp_set_fill_color(uint32_t packed_color);
void gfx_dp_texture_rectangle(int32_t ulx, int32_t uly, int32_t lrx, int32_t lry, uint8_t tile, int16_t uls,
                              int16_t ult, int16_t dsdx, int16_t dtdy, bool flip);
void gfx_dp_fill_rectangle(int32_t ulx, int32_t uly, int32_t lrx, int32_t lry);
void gfx_dp_set_z_image(void* z_buf_address);
void gfx_dp_set_color_image(uint32_t format, uint32_t size, uint32_t width, void* address);
void gfx_sp_set_other_mode(uint32_t shift, uint32_t num_bits, uint64_t mode);
void gfx_dp_set_other_mode(uint32_t h, uint32_t l);

void gfx_load_ucode(void);
void gfx_dp_full_sync(void);

static inline uint32_t color_comb(uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
    return (a & 0xf) | ((b & 0xf) << 4) | ((c & 0x1f) << 8) | ((d & 7) << 13);
}

static inline uint32_t alpha_comb(uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
    return (a & 7) | ((b & 7) << 3) | ((c & 7) << 6) | ((d & 7) << 9);
}

// These values are defined as a F3D/F3DEX values, on EX2 those are emulated backwards
#define GC_SHADING_SMOOTH 0x00000200
#define GC_CULL_FRONT 0x00001000
#define GC_CULL_BACK 0x00002000
#define GC_CULL_BOTH 0x00003000
