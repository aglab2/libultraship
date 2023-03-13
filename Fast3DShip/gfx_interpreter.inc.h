#include "gfx_emu.h"
#include "gfx_interpreter.h"
#include "gfx_pc.h"
#include "gfx_ucode.h"
#include "plugin.h"

static void gfx_sp_movemem(uint8_t index, uint8_t offset, const void* data) {
    switch (index) {
        case G_MV_VIEWPORT:
            gfx_calc_and_set_viewport((const Vp_t*)data);
            break;
#if 0
        case G_MV_LOOKATY:
        case G_MV_LOOKATX:
            memcpy(rsp.current_lookat + (index - G_MV_LOOKATY) / 2, data, sizeof(Light_t));
            //rsp.lights_changed = 1;
            break;
#endif
#ifdef F3DEX_GBI_2
        case G_MV_LIGHT: {
            int lightidx = offset / 24 - 2;
            if (lightidx >= 0 && lightidx <= MAX_LIGHTS) { // skip lookat
                // NOTE: reads out of bounds if it is an ambient light
                gfx_sp_mv_current_lights(lightidx, (const EmuLight*)data);
            } else if (lightidx < 0) {
                gfx_sp_mv_lookat((const EmuLight*)data);
            }
            break;
        }
#else
        case G_MV_L0:
        case G_MV_L1:
        case G_MV_L2:
            // NOTE: reads out of bounds if it is an ambient light
            gfx_sp_mv_current_lights((index - G_MV_L0) / 2, (const EmuLight*)data);
            break;
#endif
    }
}

static void gfx_sp_moveword(uint8_t index, uint16_t offset, uint32_t data) {
    switch (index) {
        case G_MW_NUMLIGHT:
#ifdef F3DEX_GBI_2
            gfx_sp_set_num_lights(data / 24 + 1); // add ambient light
#else
            // Ambient light is included
            // The 31th bit is a flag that lights should be recalculated
            gfx_sp_set_num_lights((data - 0x80000000U) / 32);
#endif
            break;
        case G_MW_FOG:
            gfx_sp_set_fog((int16_t)(data >> 16), (int16_t)data);
            break;
        case G_MW_SEGMENT:
            int segNumber = offset / 4;
            gfx_set_segment(segNumber, data);
            break;
    }
}

static inline void* seg_addr(uint32_t w1) {
    int seg = (w1 >> 24) & 0x0f;
    int addr = w1 & 0xffffff;
    return (void*)&Plugin::info().RDRAM[segmentPointers[seg] + addr];
}

#define C0(pos, width) ((cmd->words.w0 >> (pos)) & ((1U << width) - 1))
#define C1(pos, width) ((cmd->words.w1 >> (pos)) & ((1U << width) - 1))

static uint32_t gfx_to_f3d_geometrymode(uint32_t mode) {
#ifdef F3DEX_GBI_2
    bool smooth = !!(mode & G_SHADING_SMOOTH);
    bool front = !!(mode & G_CULL_FRONT);
    bool back = !!(mode & G_CULL_BACK);
    mode &= ~(G_SHADING_SMOOTH | G_CULL_FRONT | G_CULL_BACK);
    if (smooth)
        mode |= GC_SHADING_SMOOTH;

    if (front)
        mode |= GC_CULL_FRONT;

    if (back)
        mode |= GC_CULL_BACK;
#endif
    return mode;
}

static InstructionProcessResult PROCESS_INSTRUCTION_FN(void** pcmd, uint32_t* psegAddr) {
    // puts("dl");
#ifdef OTR
    int dummy = 0;
    char dlName[128];
    const char* fileName;

    Gfx* dListStart = cmd;
    uint64_t ourHash = -1;
#else
    auto& cmd = *(Gfx**)pcmd;
    auto& segAddr = *psegAddr;
#endif

    uint32_t opcode = cmd->words.w0 >> 24;
    // uint32_t opcode = cmd->words.w0 & 0xFF;

    // if (markerOn)
    // printf("OP: %02X\n", opcode);

    switch (opcode) {
        // RSP commands:
#ifdef F3DEX_GBI_2
        case G_LOAD_UCODE:
            gfx_load_ucode();
            break;
#endif
#ifdef OTR
        case G_MARKER: {
            cmd++;

            ourHash = ((uint64_t)cmd->words.w0 << 32) + cmd->words.w1;

#if _DEBUG
            // uint64_t hash = ((uint64_t)cmd->words.w0 << 32) + cmd->words.w1;
            // ResourceMgr_GetNameByCRC(hash, dlName);
            // lusprintf(__FILE__, __LINE__, 6, "G_MARKER: %s\n", dlName);
#endif

            markerOn = true;
        } break;
        case G_INVALTEXCACHE: {
            uintptr_t texAddr = cmd->words.w1;

            if (texAddr == 0) {
                gfx_texture_cache_clear();
            } else {
                gfx_texture_cache_delete((const uint8_t*)texAddr);
            }
        } break;
#endif
        case G_NOOP:
            break;
        case G_MTX: {
            uintptr_t mtxAddr = cmd->words.w1;

#ifdef OTR
            if (mtxAddr == SEG_ADDR(0, 0x12DB20) || mtxAddr == SEG_ADDR(0, 0x12DB40) ||
                mtxAddr == SEG_ADDR(0, 0xFBC20)) {
                mtxAddr = clearMtx;
            }
#endif
#ifdef F3DEX_GBI_2
            uint32_t flags = C0(0, 8) ^ G_MTX_PUSH;
#else
            uint32_t flags = C0(16, 8);
#endif
            gfx_sp_matrix(flags & G_MTX_PROJECTION, flags & G_MTX_LOAD, flags & G_MTX_PUSH,
                          (const int32_t*)seg_addr(cmd->words.w1));

            break;
        }
#ifdef OTR
        case G_MTX_OTR: {
            cmd++;

            uint64_t hash = ((uint64_t)cmd->words.w0 << 32) + cmd->words.w1;

            int32_t* mtx = (int32_t*)GetResourceDataByCrc(hash, false);

#ifdef F3DEX_GBI_2
            if (mtx != NULL) {
                cmd--;
                gfx_sp_matrix(C0(0, 8) ^ G_MTX_PUSH, mtx);
                cmd++;
            }
#else
            gfx_sp_matrix(C0(16, 8), (const int32_t*)seg_addr(cmd->words.w1));
#endif
            break;
        }
#endif
        case (uint8_t)G_POPMTX:
#ifdef F3DEX_GBI_2
            gfx_sp_pop_matrix(cmd->words.w1 / 64);
#else
            gfx_sp_pop_matrix(1);
#endif
            break;
        case G_MOVEMEM:
#ifdef F3DEX_GBI_2
            gfx_sp_movemem(C0(0, 8), C0(8, 8) * 8, seg_addr(cmd->words.w1));
#else
            gfx_sp_movemem(C0(16, 8), 0, seg_addr(cmd->words.w1));
#endif
            break;
        case (uint8_t)G_MOVEWORD:
#ifdef F3DEX_GBI_2
            gfx_sp_moveword(C0(16, 8), C0(0, 16), cmd->words.w1);
#else
            gfx_sp_moveword(C0(0, 8), C0(8, 16), cmd->words.w1);
#endif
            break;
        case (uint8_t)G_TEXTURE:
#ifdef F3DEX_GBI_2
            gfx_sp_texture(C1(16, 16), C1(0, 16), C0(11, 3), C0(8, 3), C0(1, 7));
#else
            gfx_sp_texture(C1(16, 16), C1(0, 16), C0(11, 3), C0(8, 3), C0(0, 8));
#endif
            break;
        case G_VTX:
#ifdef F3DEX_GBI_2
            gfx_sp_vertex(C0(12, 8), C0(1, 7) - C0(12, 8), (const Vtx*)seg_addr(cmd->words.w1));
#elif defined(F3DEX_GBI) || defined(F3DLP_GBI)
            gfx_sp_vertex(C0(10, 6), C0(16, 8) / 2, (const Vtx*)seg_addr(cmd->words.w1));
#else
            gfx_sp_vertex((C0(0, 16)) / sizeof(Vtx), C0(16, 4), (const Vtx*)seg_addr(cmd->words.w1));
#endif
            break;
#ifdef OTR
        case G_VTX_OTR_HASH: {
            // Offset added to the start of the vertices
            uintptr_t offset = cmd->words.w1;
            // This is a two-part display list command, so increment the instruction pointer so we can get the CRC64
            // hash from the second
            cmd++;
            uint64_t hash = ((uint64_t)cmd->words.w0 << 32) + cmd->words.w1;

            // We need to know if the offset is a cached pointer or not. An offset greater than one million is not a
            // real offset, so it must be a real pointer
            if (offset > 0xFFFFF) {
                cmd--;
                gfx_sp_vertex(C0(12, 8), C0(1, 7) - C0(12, 8), (Vtx*)offset);
                cmd++;
            } else {
                Vtx* vtx = (Vtx*)GetResourceDataByCrc(hash, false);

                if (vtx != NULL) {
                    vtx = (Vtx*)((char*)vtx + offset);

                    cmd--;

                    if (ourHash != (uint64_t)-1) {
                        auto res = LoadResource(ourHash, false);
                        if (res != nullptr) {
                            res->RegisterResourceAddressPatch(ourHash, cmd - dListStart, offset);
                        }
                    }

                    cmd->words.w1 = (uintptr_t)vtx;

                    gfx_sp_vertex(C0(12, 8), C0(1, 7) - C0(12, 8), vtx);
                    cmd++;
                }
            }
        } break;
        case G_VTX_OTR_FILEPATH: {
            char* fileName = (char*)cmd->words.w1;
            cmd++;
            int vtxCnt = cmd->words.w0;
            int vtxIdxOff = cmd->words.w1 >> 16;
            int vtxDataOff = cmd->words.w1 & 0xFFFF;
            Vtx* vtx = (Vtx*)GetResourceDataByName((const char*)fileName, false);
            vtx += vtxDataOff;
            cmd--;

            gfx_sp_vertex(vtxCnt, vtxIdxOff, vtx);
        } break;
        case G_DL_OTR_FILEPATH: {
            fileName = (char*)cmd->words.w1;
            Gfx* nDL = (Gfx*)GetResourceDataByName((const char*)fileName, false);

            if (C0(16, 1) == 0) {
                // Push return address
                currentDir.push((char*)fileName);
                gfx_run_dl(nDL);
                currentDir.pop();
            } else {
                cmd = nDL;
                --cmd; // increase after break
            }
        } break;
#endif
#ifdef F3DEX_GBI_2
        case G_MODIFYVTX:
            gfx_sp_modify_vertex(C0(1, 15), C0(16, 8), cmd->words.w1);
            break;
#endif
        case G_DL:
            if (C0(16, 1) == 0) {
                // Push return address
                gfx_run_dl((Gfx*)seg_addr(cmd->words.w1), cmd->words.w1);
            } else {
                segAddr = cmd->words.w1;
                cmd = (Gfx*)seg_addr(segAddr);
                gfx_dec((void**)&cmd, &segAddr);
            }
            break;
#ifdef OTR
        case G_DL_OTR_HASH:
            if (C0(16, 1) == 0) {
                // Push return address

                cmd++;

                uint64_t hash = ((uint64_t)cmd->words.w0 << 32) + cmd->words.w1;

#if _DEBUG
                // char fileName[4096];
                // ResourceMgr_GetNameByCRC(hash, fileName);

                // printf("G_DL_OTR: %s\n", fileName);
#endif

                Gfx* gfx = (Gfx*)GetResourceDataByCrc(hash, false);

                if (gfx != 0) {
                    gfx_run_dl(gfx);
                }
            } else {
                cmd = (Gfx*)seg_addr(cmd->words.w1);
            }
            break;
        case G_PUSHCD:
            gfx_push_current_dir((char*)cmd->words.w1);
            break;
        case G_BRANCH_Z_OTR: {
            // Push return address

            uint8_t vbidx = cmd->words.w0 & 0x00000FFF;
            uint32_t zval = cmd->words.w1;

            cmd++;

            if (rsp.loaded_vertices[vbidx].z <= zval) {

                uint64_t hash = ((uint64_t)cmd->words.w0 << 32) + cmd->words.w1;

#if _DEBUG
                // char fileName[4096];
                // ResourceMgr_GetNameByCRC(hash, fileName);

                // printf("G_BRANCH_Z_OTR: %s\n", fileName);
#endif

                Gfx* gfx = (Gfx*)GetResourceDataByCrc(hash, false);

                if (gfx != 0) {
                    cmd = gfx;
                    --cmd; // increase after break
                }
            }
        } break;
#endif
        case (uint8_t)G_ENDDL:
#ifndef F3DEX_GBI_2
        case 2:
#endif

            // if (markerOn)
            // printf("END DL ON MARKER\n");

#ifdef OTR
            markerOn = false;
#endif
            return InstructionProcessResult::STOP;
#ifdef F3DEX_GBI_2
        case G_GEOMETRYMODE: {
            uint32_t clear = gfx_to_f3d_geometrymode(~C0(0, 24));
            uint32_t set = gfx_to_f3d_geometrymode(cmd->words.w1);
            gfx_sp_geometry_mode(clear, set);
        }
            break;
#else
        case (uint8_t)G_SETGEOMETRYMODE: {
            uint32_t set = gfx_to_f3d_geometrymode(cmd->words.w1);
            gfx_sp_geometry_mode(0, set);
        }
            break;
        case (uint8_t)G_CLEARGEOMETRYMODE: {
            uint32_t clear = gfx_to_f3d_geometrymode(cmd->words.w1);
            gfx_sp_geometry_mode(clear, 0);
        }
            break;
#endif
#ifdef OTR
        case (uint8_t)G_TRI1_OTR: {
            int v00 = cmd->words.w0 & 0x0000FFFF;
            int v01 = cmd->words.w1 >> 16;
            int v02 = cmd->words.w1 & 0x0000FFFF;
            gfx_sp_tri1(v00, v01, v02, false);
        } break;
#endif
        case (uint8_t)G_TRI1:
#ifdef F3DEX_GBI_2
            gfx_sp_tri1(C0(16, 8) / 2, C0(8, 8) / 2, C0(0, 8) / 2, false);
#elif defined(F3DEX_GBI) || defined(F3DLP_GBI)
            gfx_sp_tri1(C1(16, 8) / 2, C1(8, 8) / 2, C1(0, 8) / 2, false);
#else
            gfx_sp_tri1(C1(16, 8) / 10, C1(8, 8) / 10, C1(0, 8) / 10, false);
#endif
            break;
#ifdef F3DEX_GBI_2
        case G_QUAD: {
            [[fallthrough]];
        }
#endif
#if defined(F3DEX_GBI) || defined(F3DLP_GBI)
        case (uint8_t)G_TRI2:
            gfx_sp_tri1(C0(16, 8) / 2, C0(8, 8) / 2, C0(0, 8) / 2, false);
            gfx_sp_tri1(C1(16, 8) / 2, C1(8, 8) / 2, C1(0, 8) / 2, false);
            break;
#endif
        case (uint8_t)G_SETOTHERMODE_L:
#ifdef F3DEX_GBI_2
            gfx_sp_set_other_mode(31 - C0(8, 8) - C0(0, 8), C0(0, 8) + 1, cmd->words.w1);
#else
            gfx_sp_set_other_mode(C0(8, 8), C0(0, 8), cmd->words.w1);
#endif
            break;
        case (uint8_t)G_SETOTHERMODE_H:
#ifdef F3DEX_GBI_2
            gfx_sp_set_other_mode(63 - C0(8, 8) - C0(0, 8), C0(0, 8) + 1, (uint64_t)cmd->words.w1 << 32);
#else
            gfx_sp_set_other_mode(C0(8, 8) + 32, C0(0, 8), (uint64_t)cmd->words.w1 << 32);
#endif
            break;

        // RDP Commands:
        case G_SETTIMG: {
            uintptr_t i = (uintptr_t)seg_addr(cmd->words.w1);

            char* imgData = (char*)i;
            uint32_t texFlags = 0;
            RawTexMetadata rawTexMetdata = {};
#ifdef OTR
            if ((i & 1) != 1) {
                if (gfx_check_image_signature(imgData) == 1) {
                    Ship::Texture* tex = std::static_pointer_cast<Ship::Texture>(LoadResource(imgData, true)).get();

                    i = (uintptr_t) reinterpret_cast<char*>(tex->ImageData);
                    texFlags = tex->Flags;
                    rawTexMetdata.width = tex->Width;
                    rawTexMetdata.height = tex->Height;
                    rawTexMetdata.h_byte_scale = tex->HByteScale;
                    rawTexMetdata.v_pixel_scale = tex->VPixelScale;
                    rawTexMetdata.type = tex->Type;
                    rawTexMetdata.name = std::string(imgData);
                }
            }
#endif

            gfx_dp_set_texture_image(C0(21, 3), C0(19, 2), C0(0, 10), imgData, texFlags, rawTexMetdata, (void*)i);
            break;
        }
#ifdef OTR
        case G_SETTIMG_OTR_HASH: {
            uintptr_t addr = cmd->words.w1;
            cmd++;
            uint64_t hash = ((uint64_t)cmd->words.w0 << 32) + (uint64_t)cmd->words.w1;
            fileName = GetResourceNameByCrc(hash);
            uint32_t texFlags = 0;
            RawTexMetadata rawTexMetdata = {};

            Ship::Texture* texture = std::static_pointer_cast<Ship::Texture>(LoadResource(hash, true)).get();
            texFlags = texture->Flags;
            rawTexMetdata.width = texture->Width;
            rawTexMetdata.height = texture->Height;
            rawTexMetdata.h_byte_scale = texture->HByteScale;
            rawTexMetdata.v_pixel_scale = texture->VPixelScale;
            rawTexMetdata.type = texture->Type;
            rawTexMetdata.name = std::string(fileName);

#if _DEBUG && 0
            tex = reinterpret_cast<char*>(texture->imageData);
            ResourceMgr_GetNameByCRC(hash, fileName);
            printf("G_SETTIMG_OTR_HASH: %s, %08X\n", fileName, hash);
#else
            char* tex = NULL;
#endif

            if (addr != 0) {
                tex = (char*)addr;
            } else {
                tex = reinterpret_cast<char*>(texture->ImageData);
                if (tex != nullptr) {
                    cmd--;
                    uintptr_t oldData = cmd->words.w1;
                    cmd->words.w1 = (uintptr_t)tex;

                    if (ourHash != (uint64_t)-1) {
                        auto res = LoadResource(ourHash, false);
                        if (res != nullptr) {
                            res->RegisterResourceAddressPatch(ourHash, cmd - dListStart, oldData);
                        }
                    }

                    cmd++;
                }
            }

            cmd--;

            uint32_t fmt = C0(21, 3);
            uint32_t size = C0(19, 2);
            uint32_t width = C0(0, 10);

            if (tex != NULL) {
                gfx_dp_set_texture_image(fmt, size, width, fileName, texFlags, rawTexMetdata, tex);
            }

            cmd++;
            break;
        }
        case G_SETTIMG_OTR_FILEPATH: {
            fileName = (char*)cmd->words.w1;

            uint32_t texFlags = 0;
            RawTexMetadata rawTexMetadata = {};

            Ship::Texture* texture = std::static_pointer_cast<Ship::Texture>(LoadResource(fileName, true)).get();
            texFlags = texture->Flags;
            rawTexMetadata.width = texture->Width;
            rawTexMetadata.height = texture->Height;
            rawTexMetadata.h_byte_scale = texture->HByteScale;
            rawTexMetadata.v_pixel_scale = texture->VPixelScale;
            rawTexMetadata.type = texture->Type;
            rawTexMetadata.name = std::string(fileName);

            uint32_t fmt = C0(21, 3);
            uint32_t size = C0(19, 2);
            uint32_t width = C0(0, 10);

            gfx_dp_set_texture_image(fmt, size, width, fileName, texFlags, rawTexMetadata,
                                     reinterpret_cast<char*>(texture->ImageData));
            break;
        }
        case G_SETFB: {
            gfx_flush();
            fbActive = 1;
            active_fb = framebuffers.find(cmd->words.w1);
            gfx_rapi->start_draw_to_framebuffer(active_fb->first, (float)active_fb->second.applied_height /
                                                                      active_fb->second.orig_height);
            gfx_rapi->clear_framebuffer();
            break;
        }
        case G_RESETFB: {
            gfx_flush();
            fbActive = 0;
            gfx_rapi->start_draw_to_framebuffer(game_renders_to_framebuffer ? game_framebuffer : 0,
                                                (float)gfx_current_dimensions.height / SCREEN_HEIGHT);
            break;
        }
        case G_SETTIMG_FB: {
            gfx_flush();
            gfx_rapi->select_texture_fb(cmd->words.w1);
            rdp.textures_changed[0] = false;
            rdp.textures_changed[1] = false;

            // if (texPtr != NULL)
            // gfx_dp_set_texture_image(C0(21, 3), C0(19, 2), C0(0, 10), texPtr);
            break;
        }
        case G_SETGRAYSCALE: {
            rdp.grayscale = cmd->words.w1;
            break;
        }
#endif
        case G_LOADBLOCK:
            gfx_dp_load_block(C1(24, 3), C0(12, 12), C0(0, 12), C1(12, 12), C1(0, 12));
            break;
        case G_LOADTILE:
            gfx_dp_load_tile(C1(24, 3), C0(12, 12), C0(0, 12), C1(12, 12), C1(0, 12));
            break;
        case G_SETTILE:
            gfx_dp_set_tile(C0(21, 3), C0(19, 2), C0(9, 9), C0(0, 9), C1(24, 3), C1(20, 4), C1(18, 2), C1(14, 4),
                            C1(10, 4), C1(8, 2), C1(4, 4), C1(0, 4));
            break;
        case G_SETTILESIZE:
            gfx_dp_set_tile_size(C1(24, 3), C0(12, 12), C0(0, 12), C1(12, 12), C1(0, 12));
            break;
        case G_LOADTLUT:
            gfx_dp_load_tlut(C1(24, 3), C1(14, 10));
            break;
        case G_SETENVCOLOR:
            gfx_dp_set_env_color(C1(24, 8), C1(16, 8), C1(8, 8), C1(0, 8));
            break;
        case G_SETPRIMCOLOR:
            gfx_dp_set_prim_color(C0(8, 8), C0(0, 8), C1(24, 8), C1(16, 8), C1(8, 8), C1(0, 8));
            break;
        case G_SETFOGCOLOR:
            gfx_dp_set_fog_color(C1(24, 8), C1(16, 8), C1(8, 8), C1(0, 8));
            break;
        case G_SETFILLCOLOR:
            gfx_dp_set_fill_color(cmd->words.w1);
            break;
#ifdef OTR
        case G_SETINTENSITY:
            gfx_dp_set_grayscale_color(C1(24, 8), C1(16, 8), C1(8, 8), C1(0, 8));
            break;
#endif
        case G_SETCOMBINE:
            gfx_dp_set_combine_mode(color_comb(C0(20, 4), C1(28, 4), C0(15, 5), C1(15, 3)),
                                    alpha_comb(C0(12, 3), C1(12, 3), C0(9, 3), C1(9, 3)),
                                    color_comb(C0(5, 4), C1(24, 4), C0(0, 5), C1(6, 3)),
                                    alpha_comb(C1(21, 3), C1(3, 3), C1(18, 3), C1(0, 3)));
            break;
        // G_SETPRIMCOLOR, G_CCMUX_PRIMITIVE, G_ACMUX_PRIMITIVE, is used by Goddard
        // G_CCMUX_TEXEL1, LOD_FRACTION is used in Bowser room 1
        case G_TEXRECT:
        case G_TEXRECTFLIP: {
            int32_t lrx, lry, tile, ulx, uly;
            uint32_t uls, ult, dsdx, dtdy;
#ifdef F3DEX_GBI_2E
            lrx = (int32_t)(C0(0, 24) << 8) >> 8;
            lry = (int32_t)(C1(0, 24) << 8) >> 8;
            tile = C1(24, 3);
            ++cmd;
            ulx = (int32_t)(C0(0, 24) << 8) >> 8;
            uly = (int32_t)(C1(0, 24) << 8) >> 8;
            ++cmd;
            uls = C0(16, 16);
            ult = C0(0, 16);
            dsdx = C1(16, 16);
            dtdy = C1(0, 16);
#else
            lrx = C0(12, 12);
            lry = C0(0, 12);
            tile = C1(24, 3);
            ulx = C1(12, 12);
            uly = C1(0, 12);
            gfx_inc((void**)&cmd, &segAddr);
            uls = C1(16, 16);
            ult = C1(0, 16);
            gfx_inc((void**)&cmd, &segAddr);
            dsdx = C1(16, 16);
            dtdy = C1(0, 16);
#endif
            gfx_dp_texture_rectangle(ulx, uly, lrx, lry, tile, uls, ult, dsdx, dtdy, opcode == G_TEXRECTFLIP);
            break;
        }
#ifdef OTR
        case G_TEXRECT_WIDE: {
            int32_t lrx, lry, tile, ulx, uly;
            uint32_t uls, ult, dsdx, dtdy;
            lrx = static_cast<int32_t>((C0(0, 24) << 8)) >> 8;
            lry = static_cast<int32_t>((C1(0, 24) << 8)) >> 8;
            tile = C1(24, 3);
            ++cmd;
            ulx = static_cast<int32_t>((C0(0, 24) << 8)) >> 8;
            uly = static_cast<int32_t>((C1(0, 24) << 8)) >> 8;
            ++cmd;
            uls = C0(16, 16);
            ult = C0(0, 16);
            dsdx = C1(16, 16);
            dtdy = C1(0, 16);
            gfx_dp_texture_rectangle(ulx, uly, lrx, lry, tile, uls, ult, dsdx, dtdy, opcode == G_TEXRECTFLIP);
            break;
        }
#endif
        case G_FILLRECT:
#ifdef F3DEX_GBI_2E
        {
            int32_t lrx, lry, ulx, uly;
            lrx = (int32_t)(C0(0, 24) << 8) >> 8;
            lry = (int32_t)(C1(0, 24) << 8) >> 8;
            ++cmd;
            ulx = (int32_t)(C0(0, 24) << 8) >> 8;
            uly = (int32_t)(C1(0, 24) << 8) >> 8;
            gfx_dp_fill_rectangle(ulx, uly, lrx, lry);
            break;
        }
#else
            gfx_dp_fill_rectangle(C1(12, 12), C1(0, 12), C0(12, 12), C0(0, 12));
            break;
#endif
#ifdef OTR
        case G_FILLWIDERECT: {
            int32_t lrx, lry, ulx, uly;
            lrx = (int32_t)(C0(0, 24) << 8) >> 8;
            lry = (int32_t)(C1(0, 24) << 8) >> 8;
            ++cmd;
            ulx = (int32_t)(C0(0, 24) << 8) >> 8;
            uly = (int32_t)(C1(0, 24) << 8) >> 8;
            gfx_dp_fill_rectangle(ulx, uly, lrx, lry);
            break;
        }
#endif
        case G_SETSCISSOR:
            gfx_dp_set_scissor(C1(24, 2), C0(12, 12), C0(0, 12), C1(12, 12), C1(0, 12));
            break;
        case G_SETZIMG:
            gfx_dp_set_z_image(seg_addr(cmd->words.w1));
            break;
        case G_SETCIMG:
            gfx_dp_set_color_image(C0(21, 3), C0(19, 2), C0(0, 11), seg_addr(cmd->words.w1));
            break;
        case G_RDPSETOTHERMODE:
            gfx_dp_set_other_mode(C0(0, 24), cmd->words.w1);
            break;
#ifdef OTR
            // S2DEX
        case G_BG_COPY:
            if (!markerOn) {
                gfx_s2dex_bg_copy((uObjBg*)cmd->words.w1); // not seg_addr here it seems
            }

            break;
#endif
        case G_RDPFULLSYNC:
            gfx_dp_full_sync();
            break;
    }

    return InstructionProcessResult::CONTINUE;
}

void RUN_DL_FN(void* cmd, uint32_t segAddr) {
    for (;;) {
        InstructionProcessResult res;
        res = PROCESS_INSTRUCTION_FN(&cmd, &segAddr);
        if (res == InstructionProcessResult::STOP)
            return;

        gfx_inc(&cmd, &segAddr);
    }
}