
#ifndef ZEQ_BUFFER_H
#define ZEQ_BUFFER_H

#include <stdlib.h>
#include <string.h>
#include <cstdio>
#include "type.h"

#define ZEQ_ARRAYBUFFER_INIT_SIZE 32
#define ZEQ_STACKBUFFER_INIT_SIZE 4
#define ZEQ_STACKBUFFER_EMPTY -1

class Buffer
{
public:
	virtual void Add(byte* data, size_t len) = 0;
	//Retrieves pointer internal to the buffer; note that this only good until the next insertion and/or the buffer's deletion
	virtual const byte* GetData() = 0;
	//Copies the buffer data; deleting this copy after use is up to the user
	virtual byte* CopyData() = 0;
	//Takes ownership of the buffer's internal pointer and resets the buffer
	virtual byte* TakeData(bool reset_buffer) = 0;
	virtual size_t GetLen() = 0;
};

//Simple power of two dynamic array buffer
class ArrayBuffer : public Buffer
{
public:
	ArrayBuffer();
	~ArrayBuffer();
	void Add(byte* data, size_t len) override;
	//Retrieves pointer internal to the buffer; note that this only good until the next insertion and/or the buffer's deletion
	const byte* GetData() override;
	//Copies the buffer data; deleting this copy after use is up to the user
	byte* CopyData() override;
	//Takes ownership of the buffer's internal pointer and resets the buffer
	byte* TakeData(bool reset_buffer = true) override;
	size_t GetLen() override;
private:
	byte* data;
	uint32 len;
	uint32 capacity;
};

struct _BufferString
{
	byte* data;
	size_t len;
};

//Buffer with an efficient Tower of Hanoi-style implementation;
//the idea is to minimize the total number of memory copy operations needed in the general case
//downside: the data string is not kept in a representable form during additions,
//and needs to be assembled before it can be used
class StackBuffer : public Buffer
{
public:
	StackBuffer();
	~StackBuffer();
	void Add(byte* data, size_t len) override;
	//If this method is used, the buffer will 'own' the input pointer and delete it at its leisure
	void AddWithoutCopying(byte* data, size_t len);
	//Retrieves pointer internal to the buffer; note that this only good until the next insertion and/or the buffer's deletion
	const byte* GetData() override;
	//Copies the buffer data; deleting this copy after use is up to the user
	byte* CopyData() override;
	//Takes ownership of the buffer's internal pointer and resets the buffer
	byte* TakeData(bool reset_buffer = true) override;
	size_t GetLen() override;
private:
	_BufferString internal_string;
	_BufferString* stack;
	int stack_capacity;
	int stack_top;

	void DoAdd(byte* data, size_t len, bool make_copy);
	void BuildString();
};

#endif
