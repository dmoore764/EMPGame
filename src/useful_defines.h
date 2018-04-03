#define KILOBYTES(Number) (1024L*Number)
#define MEGABYTES(Number) (1024L*KILOBYTES(Number))
#define GIGABYTES(Number) (1024L*MEGABYTES(Number))
#define MAKE_COLOR(r,g,b,a) (((a) << 24) | ((b) << 16) | ((g) << 8) | r)
#define WITH_B(color, b) ((color & ~0x00ff0000) | (b << 16))
#define WITH_G(color, g) ((color & ~0x0000ff00) | (g << 8))
#define WITH_R(color, r) ((color & ~0x000000ff) | (r << 0))
#define WITH_A(color, a) ((color & ~0xff000000) | (a << 24))
#define COL_WHITE 0xffffffff
#define COL_BLACK 0xff000000
#define DEGREES_TO_RADIANS 0.0174533f
#define RADIANS_TO_DEGREES 57.2958f
#define PI 3.1415f

#define ArrayCount(array) (sizeof(array)/sizeof(array[0]))

#define ADD_FLOAT_TO_STREAM(Stream, Float) (*((float *)Stream) = Float)
#define MOVE_BUFFER_ONE_FLOAT(Stream) (Stream = (void *)((char *)Stream + sizeof(float)))

#define ADD_UINT32_TO_STREAM(Stream, Uint) (*((uint32_t *)Stream) = Uint)
#define MOVE_BUFFER_ONE_UINT32(Stream) (Stream = (void *)((char *)Stream + sizeof(uint32_t)))

#define ADD_UNSIGNED_BYTE_TO_STREAM(Stream, Byte) (*((uint8_t *)Stream) = Byte)
#define MOVE_BUFFER_ONE_BYTE(Stream) (Stream = (void *)((char *)Stream + sizeof(uint8_t)))

#define global_variable static
#define internal_function static
#define local_persistent static
