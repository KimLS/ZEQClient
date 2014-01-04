
#ifndef ZEQ_BYTE_ORDER_H
#define ZEQ_BYTE_ORDER_H

#ifdef ZEQ_ENDIAN_CHECK

#include "type.h"

/*
EQ's data files are all little-endian. Most player's machines probably are too.
But, just in case, we'll use a small set of hton-like functions to play nice with big-endian machines.
(Can't use the hton functions themselves because they're all "to/from big-endian",
and we want "from little-endian")

The proper way of doing this would probably be preprocessor defs...
but for now, we rely on a runtime hack to determine native endianness instead
*/

static union {
	int dummy;
	char LITTLE_ENDIAN;
} const native = {1};

/*
Brief explanation:
Members of a union variable all begin at the same address and stretch out for their particular byte
length toward higher addresses. Thus for the above union, the first 8 bits are overlapped between the
int and char members, while the remaining 24 bits are the int's alone. For unions, the = {} initializer
syntax only applies to the first declared member type, in this case the int. So, after the assignment
the bits of the int will either look like this:

Little-endian:	00000001 00000000 00000000 00000000

Or, like this:

Big-endian:		00000000 00000000 00000000 00000001

And so, we can now determine the endianness by looking only at the first 8 bits, through the char
interpretation of our union: if it's 1, it's little-endian, and if it's 0, it's big-endian.
*/

//Creates a mask of n bits
#define MASK32(n)		(~((uint32)0xFFFFFFFF << n))
#define MASK16(n)		(~((uint16)0xFFFF << n))
/*
i.e.: MASK32(3) ->
1) 11111111 11111111 11111111 11111111 (begin)
2) 11111111 11111111 11111111 11111000 (<< n step)
3) 00000000 00000000 00000000 00000111 (~ step)
*/

static uint32 endian_uint32(uint32 v)
{
	if (!native.LITTLE_ENDIAN) {
		//this is probably an awful implementation!
		uint32 out = 0;
		out |= v & MASK32(8); //copy right-most byte ->	00000000 00000000 00000000 xxxxxxxx
		out <<= 8; //shift it to the next byte ->		00000000 00000000 xxxxxxxx 00000000
		v >>= 8; //shift source so the second right-most byte is now the right-most byte
		//repeat until we're done (unrolled loop)
		out |= v & MASK32(8);
		out <<= 8;
		v >>= 8;
		out |= v & MASK32(8);
		out <<= 8;
		v >>= 8;
		out |= v & MASK32(8);
		return out;
	}
	return v;
}

static int32 endian_int32(int32 v)
{
	if (!native.LITTLE_ENDIAN) {
		//nothing needs to be done to preserve the sign bit:
		//it is always in the most significant byte and will move along with it just fine
		int32 out = 0;
		out |= v & MASK32(8);
		out <<= 8;
		v >>= 8;
		out |= v & MASK32(8);
		out <<= 8;
		v >>= 8;
		out |= v & MASK32(8);
		out <<= 8;
		v >>= 8;
		out |= v & MASK32(8);
		return out;
	}
	return v;
}

static uint16 endian_uint16(uint16 v)
{
	if (!native.LITTLE_ENDIAN) {
		uint16 out = 0;
		out |= v & MASK16(8);
		out <<= 8;
		v >>= 8;
		out |= v & MASK16(8);
		return out;
	}
	return v;
}

static int16 endian_int16(int16 v)
{
	if (!native.LITTLE_ENDIAN) {
		int16 out = 0;
		out |= v & MASK16(8);
		out <<= 8;
		v >>= 8;
		out |= v & MASK16(8);
		return out;
	}
	return v;
}


inline static bool isLittleEndian()
{
	return (native.LITTLE_ENDIAN != 0);
}

//as for floats: better hope your machine uses IEEE format...

#endif //ZEQ_ENDIAN_CHECK

#endif
