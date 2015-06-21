#include "xepem.h"
const Uint16 font_16x16[] = {
0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x180,0x180,0x180,0x180,0x180,0x180,0x0,0x0,0x180,0x180,0x0,0x0,0x0,
0x0,0x0,0x0,0x6c0,0x6c0,0x6c0,0x6c0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x120,0x120,0x120,0x7e0,0x240,0x240,0x7e0,0x0,0x480,0x480,0x0,0x0,0x0,
0x0,0x0,0x100,0x380,0x7c0,0xd60,0xd00,0x780,0x1c0,0x160,0xd60,0x7c0,0x380,0x100,0x0,0x0,
0x0,0x0,0x0,0x440,0xa40,0xa00,0xa80,0x480,0x120,0x150,0x50,0x250,0x220,0x0,0x0,0x0,
0x0,0x0,0x0,0x3c0,0x660,0x660,0x3c0,0x380,0x7c0,0xcf0,0xc70,0xc70,0x3d8,0x0,0x0,0x0,
0x0,0x0,0x0,0x180,0x180,0x180,0x180,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x40,0xc0,0x80,0x180,0x180,0x180,0x180,0x180,0x180,0x180,0x80,0xc0,0x40,
0x0,0x0,0x0,0x100,0x180,0x80,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0x80,0x180,0x100,
0x0,0x0,0x0,0x80,0x3e0,0x1c0,0x360,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x180,0x180,0x180,0xff0,0xff0,0x180,0x180,0x180,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x180,0x180,0x80,0x100,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x1e0,0x1e0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x180,0x180,0x0,0x0,0x0,
0x0,0x0,0x0,0x40,0x40,0x80,0x80,0x80,0x100,0x100,0x100,0x200,0x200,0x0,0x0,0x0,
0x0,0x0,0x0,0x380,0x7c0,0xee0,0xc60,0xc60,0xc60,0xc60,0xee0,0x7c0,0x380,0x0,0x0,0x0,
0x0,0x0,0x0,0x180,0x380,0x780,0x580,0x180,0x180,0x180,0x180,0x180,0x180,0x0,0x0,0x0,
0x0,0x0,0x0,0x3c0,0x7e0,0xc60,0xc60,0xc0,0x180,0x300,0x600,0xfe0,0xfe0,0x0,0x0,0x0,
0x0,0x0,0x0,0x3c0,0x7e0,0xc60,0x60,0x1c0,0x60,0xc60,0xe60,0x7c0,0x380,0x0,0x0,0x0,
0x0,0x0,0x0,0xc0,0x1c0,0x1c0,0x2c0,0x6c0,0xcc0,0xfe0,0xfe0,0xc0,0xc0,0x0,0x0,0x0,
0x0,0x0,0x0,0x7e0,0x7e0,0x400,0xf80,0xfc0,0xc60,0x60,0xc60,0x7c0,0x380,0x0,0x0,0x0,
0x0,0x0,0x0,0x3c0,0x7e0,0xe60,0xc00,0xfc0,0xfe0,0xc60,0xc60,0x7e0,0x380,0x0,0x0,0x0,
0x0,0x0,0x0,0xfe0,0xfe0,0x40,0x80,0x180,0x180,0x100,0x300,0x300,0x300,0x0,0x0,0x0,
0x0,0x0,0x0,0x7c0,0xfe0,0xc60,0xc60,0x7c0,0x7c0,0xc60,0xc60,0xfe0,0x3c0,0x0,0x0,0x0,
0x0,0x0,0x0,0x380,0xfc0,0xc60,0xc60,0xfe0,0x7e0,0x60,0xce0,0xfc0,0x780,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x180,0x180,0x0,0x0,0x0,0x0,0x180,0x180,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x180,0x180,0x0,0x0,0x0,0x0,0x180,0x180,0x80,0x100,0x0,
0x0,0x0,0x0,0x0,0x20,0xe0,0x3c0,0xe00,0xf00,0x3c0,0xe0,0x20,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0xfe0,0xfe0,0x0,0xfe0,0xfe0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x800,0xe00,0x780,0x1e0,0x1e0,0x780,0xe00,0x800,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x3c0,0x7e0,0xc30,0x30,0xe0,0x1c0,0x180,0x0,0x180,0x180,0x0,0x0,0x0,
0x0,0x0,0x0,0x3f0,0xc18,0x13b4,0x27f2,0x4672,0x4c72,0x4c62,0x4ce4,0x4fec,0x2770,0x3002,0x180c,0x7f0,
0x0,0x0,0x0,0x1c0,0x1c0,0x3e0,0x360,0x360,0x630,0x7f0,0x7f0,0xc18,0xc18,0x0,0x0,0x0,
0x0,0x0,0x0,0xfe0,0xff0,0xc30,0xc30,0xfe0,0xff0,0xc30,0xc30,0xff0,0xfe0,0x0,0x0,0x0,
0x0,0x0,0x0,0x3c0,0x7e0,0x670,0xc20,0xc00,0xc00,0xc20,0xe70,0x7e0,0x3c0,0x0,0x0,0x0,
0x0,0x0,0x0,0xfc0,0xfe0,0xc70,0xc30,0xc30,0xc30,0xc30,0xc70,0xfe0,0xfc0,0x0,0x0,0x0,
0x0,0x0,0x0,0x7f0,0x7f0,0x600,0x600,0x7f0,0x7f0,0x600,0x600,0x7f0,0x7f0,0x0,0x0,0x0,
0x0,0x0,0x0,0x7f0,0x7f0,0x600,0x600,0x7e0,0x7e0,0x600,0x600,0x600,0x600,0x0,0x0,0x0,
0x0,0x0,0x0,0x3c0,0xfe0,0x1c70,0x1820,0x1800,0x18f0,0x18f0,0x1c30,0xfe0,0x3c0,0x0,0x0,0x0,
0x0,0x0,0x0,0xc30,0xc30,0xc30,0xc30,0xff0,0xff0,0xc30,0xc30,0xc30,0xc30,0x0,0x0,0x0,
0x0,0x0,0x0,0x180,0x180,0x180,0x180,0x180,0x180,0x180,0x180,0x180,0x180,0x0,0x0,0x0,
0x0,0x0,0x0,0x60,0x60,0x60,0x60,0x60,0x60,0xc60,0xc60,0xfe0,0x780,0x0,0x0,0x0,
0x0,0x0,0x0,0xc30,0xc60,0xcc0,0xd80,0xfc0,0xec0,0xc60,0xc60,0xc30,0xc30,0x0,0x0,0x0,
0x0,0x0,0x0,0x600,0x600,0x600,0x600,0x600,0x600,0x600,0x600,0x7f0,0x7f0,0x0,0x0,0x0,
0x0,0x0,0x0,0x1c1c,0x1e3c,0x1e3c,0x1a2c,0x1b6c,0x1b6c,0x19cc,0x19cc,0x19cc,0x188c,0x0,0x0,0x0,
0x0,0x0,0x0,0xc30,0xe30,0xe30,0xf30,0xdb0,0xdb0,0xcf0,0xcf0,0xc70,0xc30,0x0,0x0,0x0,
0x0,0x0,0x0,0x7c0,0xfe0,0xc60,0x1830,0x1830,0x1830,0x1830,0xc60,0xfe0,0x7c0,0x0,0x0,0x0,
0x0,0x0,0x0,0x7e0,0x7f0,0x630,0x630,0x7f0,0x7e0,0x600,0x600,0x600,0x600,0x0,0x0,0x0,
0x0,0x0,0x0,0x7c0,0xfe0,0xc60,0x1830,0x1830,0x1830,0x1930,0x1cf0,0xfe0,0x7a0,0x10,0x0,0x0,
0x0,0x0,0x0,0xfe0,0xff0,0xc30,0xc30,0xfe0,0xfc0,0xce0,0xc60,0xc70,0xc38,0x0,0x0,0x0,
0x0,0x0,0x0,0x3e0,0x7f0,0x630,0x600,0x7e0,0x3f0,0x30,0x630,0x3f0,0x1e0,0x0,0x0,0x0,
0x0,0x0,0x0,0xff0,0xff0,0x180,0x180,0x180,0x180,0x180,0x180,0x180,0x180,0x0,0x0,0x0,
0x0,0x0,0x0,0xc30,0xc30,0xc30,0xc30,0xc30,0xc30,0xc30,0xe70,0x7e0,0x3c0,0x0,0x0,0x0,
0x0,0x0,0x0,0xc18,0x630,0x630,0x630,0x360,0x360,0x3e0,0x1c0,0x1c0,0x1c0,0x0,0x0,0x0,
0x0,0x0,0x0,0x31c6,0x31c6,0x19cc,0x1b6c,0x1b6c,0x1b6c,0x1f7c,0xe38,0xe38,0xe38,0x0,0x0,0x0,
0x0,0x0,0x0,0x630,0x630,0x360,0x1c0,0x1c0,0x1c0,0x1c0,0x360,0x630,0x630,0x0,0x0,0x0,
0x0,0x0,0x0,0xc30,0xc30,0x660,0x3c0,0x3c0,0x180,0x180,0x180,0x180,0x180,0x0,0x0,0x0,
0x0,0x0,0x0,0xff0,0xff0,0x60,0xc0,0x180,0x180,0x300,0x600,0xff0,0xff0,0x0,0x0,0x0,
0x0,0x0,0x0,0x1e0,0x1e0,0x180,0x180,0x180,0x180,0x180,0x180,0x180,0x180,0x180,0x1e0,0x1e0,
0x0,0x0,0x0,0x200,0x200,0x100,0x100,0x100,0x80,0x80,0x80,0x40,0x40,0x0,0x0,0x0,
0x0,0x0,0x0,0x3c0,0x3c0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0x3c0,0x3c0,
0x0,0x0,0x0,0x180,0x180,0x3c0,0x240,0x660,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0xff0,
0x0,0x0,0x0,0x300,0x180,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x3c0,0x7e0,0x60,0x1e0,0x360,0x660,0x7e0,0x3b0,0x0,0x0,0x0,
0x0,0x0,0x0,0x600,0x600,0x6c0,0x7e0,0x770,0x630,0x630,0x670,0x7e0,0x6c0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x1c0,0x3c0,0x660,0x600,0x600,0x660,0x3e0,0x1c0,0x0,0x0,0x0,
0x0,0x0,0x0,0x30,0x30,0x1b0,0x3f0,0x770,0x630,0x630,0x770,0x3f0,0x1b0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x1c0,0x3e0,0x630,0x7f0,0x7f0,0x600,0x3f0,0x1e0,0x0,0x0,0x0,
0x0,0x0,0x0,0xe0,0x180,0x3e0,0x3e0,0x180,0x180,0x180,0x180,0x180,0x180,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x1b0,0x3f0,0x770,0x630,0x630,0x770,0x3f0,0x1b0,0x630,0x7f0,0x3e0,
0x0,0x0,0x0,0x600,0x600,0x6e0,0x7f0,0x730,0x630,0x630,0x630,0x630,0x630,0x0,0x0,0x0,
0x0,0x0,0x0,0x180,0x0,0x180,0x180,0x180,0x180,0x180,0x180,0x180,0x180,0x0,0x0,0x0,
0x0,0x0,0x0,0x180,0x0,0x180,0x180,0x180,0x180,0x180,0x180,0x180,0x180,0x180,0x780,0x700,
0x0,0x0,0x0,0x600,0x600,0x660,0x6c0,0x780,0x780,0x7c0,0x6c0,0x660,0x660,0x0,0x0,0x0,
0x0,0x0,0x0,0x180,0x180,0x180,0x180,0x180,0x180,0x180,0x180,0x180,0x180,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x1b70,0x1ff8,0x1998,0x1998,0x1998,0x1998,0x1998,0x1998,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x6e0,0x7f0,0x730,0x630,0x630,0x630,0x630,0x630,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x1c0,0x3e0,0x770,0x630,0x630,0x770,0x3e0,0x1c0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x6c0,0x7e0,0x770,0x630,0x630,0x770,0x7e0,0x6c0,0x600,0x600,0x600,
0x0,0x0,0x0,0x0,0x0,0x1b0,0x3f0,0x770,0x630,0x630,0x770,0x3f0,0x1b0,0x30,0x30,0x30,
0x0,0x0,0x0,0x0,0x0,0x360,0x3e0,0x380,0x300,0x300,0x300,0x300,0x300,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x3c0,0x7e0,0x600,0x780,0x1e0,0x60,0x7e0,0x3c0,0x0,0x0,0x0,
0x0,0x0,0x0,0x80,0x180,0x3e0,0x3e0,0x180,0x180,0x180,0x180,0x1e0,0xe0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x630,0x630,0x630,0x630,0x630,0x670,0x7f0,0x3b0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x630,0x630,0x630,0x360,0x360,0x1c0,0x1c0,0x1c0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x3118,0x3398,0x1bb0,0x1bb0,0x1ef0,0xee0,0xee0,0xc60,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x660,0x640,0x3c0,0x180,0x180,0x3c0,0x660,0x660,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0xc60,0xc60,0x6c0,0x6c0,0x6c0,0x7c0,0x380,0x380,0x380,0x300,0x700,
0x0,0x0,0x0,0x0,0x0,0x7c0,0x7c0,0xc0,0x180,0x300,0x200,0x7c0,0x7c0,0x0,0x0,0x0,
0x0,0x0,0x0,0xe0,0x1e0,0x180,0x180,0x180,0x180,0x300,0x180,0x180,0x180,0x180,0x1e0,0xe0,
0x0,0x0,0x0,0x100,0x100,0x100,0x100,0x100,0x100,0x100,0x100,0x100,0x100,0x100,0x100,0x100,
0x0,0x0,0x0,0x380,0x3c0,0xc0,0xc0,0xc0,0xc0,0x60,0xc0,0xc0,0xc0,0xc0,0x3c0,0x380,
0x0,0x0,0x0,0x0,0x0,0x0,0x720,0xfe0,0xc0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x7f0,0x630,0x630,0x630,0x630,0x630,0x630,0x630,0x630,0x630,0x7f0,0x0,0x0,0x0,
};
