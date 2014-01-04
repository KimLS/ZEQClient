
#ifndef ZEQ_TYPE_H
#define ZEQ_TYPE_H

typedef unsigned char byte;
typedef signed char int8;
typedef unsigned char uint8;
typedef signed short int16;
typedef unsigned short uint16;
typedef signed int int32;
typedef unsigned int uint32;

#ifdef WIN32
#define sprintf _sprintf
#define snprintf _snprintf
#define strlwr _strlwr
#endif

#endif
