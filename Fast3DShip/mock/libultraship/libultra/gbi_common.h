// GBI but all version specific defines are dropped and replaced with enum classes
#pragma once

#ifndef _LANGUAGE_C
#define _LANGUAGE_C
#endif
#include "gbi.h"

#undef G_MTX_MODELVIEW	
#undef G_MTX_PROJECTION	
#undef G_MTX_MUL	
#undef G_MTX_LOAD		
#undef G_MTX_NOPUSH		
#undef G_MTX_PUSH		

#undef G_TEXTURE_ENABLE
#undef G_SHADING_SMOOTH
#undef G_CULL_FRONT
#undef G_CULL_BACK
#undef G_CULL_BOTH

#undef G_MV_VIEWPORT
#undef G_MV_LOOKATY
#undef G_MV_LOOKATX
#undef G_MV_L0
#undef G_MV_L1
#undef G_MV_L2
#undef G_MV_L3
#undef G_MV_L4
#undef G_MV_L5
#undef G_MV_L6
#undef G_MV_L7
#undef G_MV_TXTATT
#undef G_MV_MATRIX_1
#undef G_MV_MATRIX_2
#undef G_MV_MATRIX_3
#undef G_MV_MATRIX_4

#undef G_MW_FORCEMTX

#undef G_MWO_aLIGHT_2
#undef G_MWO_bLIGHT_2
#undef G_MWO_aLIGHT_3
#undef G_MWO_bLIGHT_3
#undef G_MWO_aLIGHT_4
#undef G_MWO_bLIGHT_4
#undef G_MWO_aLIGHT_5
#undef G_MWO_bLIGHT_5
#undef G_MWO_aLIGHT_6
#undef G_MWO_bLIGHT_6
#undef G_MWO_aLIGHT_7
#undef G_MWO_bLIGHT_7
#undef G_MWO_aLIGHT_8
#undef G_MWO_bLIGHT_8
