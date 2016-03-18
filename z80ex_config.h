/* Xep128: Minimalistic Enterprise-128 emulator with focus on "exotic" hardware
   Copyright (C)2015,2016 LGB (Gábor Lénárt) <lgblgblgb@gmail.com>
   http://xep128.lgb.hu/

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

#ifndef __XEP128_Z80EX_CONFIG_H_INCLUDED
#define __XEP128_Z80EX_CONFIG_H_INCLUDED

/* Modified Z80ex features requested */
#define Z80EX_Z180_SUPPORT
#define Z80EX_ED_TRAPPING_SUPPORT
#define Z80EX_CALLBACK_PROTOTYPE extern

/* Types, from original Z80Ex for now ... */
#define Z80EX_TYPES_DEFINED
#if defined(__SYMBIAN32__)
typedef unsigned char Z80EX_BYTE;
typedef signed char Z80EX_SIGNED_BYTE;
typedef unsigned short Z80EX_WORD;
typedef unsigned int Z80EX_DWORD;
#elif defined(__GNUC__)
#include <stdint.h>
typedef uint8_t Z80EX_BYTE;
typedef int8_t Z80EX_SIGNED_BYTE;
typedef uint16_t Z80EX_WORD;
typedef uint32_t Z80EX_DWORD;
#elif defined(_MSC_VER)
typedef unsigned __int8 Z80EX_BYTE;
typedef signed __int8 Z80EX_SIGNED_BYTE;
typedef unsigned __int16 Z80EX_WORD;
typedef unsigned __int32 Z80EX_DWORD;
#else
typedef unsigned char Z80EX_BYTE;
typedef signed char Z80EX_SIGNED_BYTE;
typedef unsigned short Z80EX_WORD;
typedef unsigned int Z80EX_DWORD;
#endif

/* Endian related stuffs for Z80ex */
#include "SDL_endian.h"
#ifndef SDL_BYTEORDER
#	error "SDL_BYTEORDER is not defined!"
#endif
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
#	ifdef Z80EX_WORDS_BIG_ENDIAN
#		undef Z80EX_WORDS_BIG_ENDIAN
#	endif
#else
#	ifndef Z80EX_WORDS_BIG_ENDIAN
#		define Z80EX_WORDS_BIG_ENDIAN
#	endif
#endif

#endif
