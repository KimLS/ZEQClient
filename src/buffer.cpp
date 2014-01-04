
#include "buffer.h"

ArrayBuffer::ArrayBuffer()
{
	data = new byte[ZEQ_ARRAYBUFFER_INIT_SIZE];
	capacity = ZEQ_ARRAYBUFFER_INIT_SIZE;
	len = 0;
}

ArrayBuffer::~ArrayBuffer()
{
	if (data)
	{
		delete[] data;
		data = nullptr;
	}
}

void ArrayBuffer::Add(byte* str, size_t in_len)
{
	size_t newlen = len + in_len;
	if (newlen > capacity)
	{
		do
		{
			capacity <<= 1;
		}
		while (newlen > capacity);
		byte* copy = new byte[capacity];
		memcpy(copy,data,len);
		memcpy(&copy[len],str,in_len);
		delete[] data;
		data = copy;
	}
	else
	{
		memcpy(&data[len],str,in_len);
	}
	len = newlen;
}

const byte* ArrayBuffer::GetData()
{
	return data;
}

byte* ArrayBuffer::CopyData()
{
	byte* copy = new byte[len];
	memcpy(copy,data,len);
	return copy;
}

byte* ArrayBuffer::TakeData(bool reset_buffer)
{
	byte* give = data;
	if (reset_buffer)
	{
		data = new byte[ZEQ_ARRAYBUFFER_INIT_SIZE];
		capacity = ZEQ_ARRAYBUFFER_INIT_SIZE;
	}
	else
	{
		data = nullptr;
	}
	len = 0;
	return give;
}

size_t ArrayBuffer::GetLen()
{
	return len;
}


StackBuffer::StackBuffer()
{
	stack = (_BufferString*)malloc(sizeof(_BufferString) * ZEQ_STACKBUFFER_INIT_SIZE);
	stack_capacity = ZEQ_STACKBUFFER_INIT_SIZE;
	stack_top = ZEQ_STACKBUFFER_EMPTY;
	internal_string.data = nullptr;
	internal_string.len = 0;
}

StackBuffer::~StackBuffer()
{
	//free any stack strings
	if (stack_top != ZEQ_STACKBUFFER_EMPTY)
	{
		for (int i = 0; i <= stack_top; ++i)
		{
			delete[] stack[i].data;
		}
	}
	free(stack);
	if (internal_string.data)
		delete[] internal_string.data;
}

void StackBuffer::Add(byte* str, size_t len)
{
	DoAdd(str,len,true);
}

void StackBuffer::AddWithoutCopying(byte* str, size_t len)
{
	DoAdd(str,len,false);
}

const byte* StackBuffer::GetData()
{
	if (stack_top != ZEQ_STACKBUFFER_EMPTY)
		BuildString();
	return internal_string.data;
}

byte* StackBuffer::CopyData()
{
	if (stack_top != ZEQ_STACKBUFFER_EMPTY)
		BuildString();
	size_t len = internal_string.len;
	byte* copy = new byte[len];
	memcpy(copy,internal_string.data,len);
	return copy;
}

byte* StackBuffer::TakeData(bool reset_buffer)
{
	if (stack_top != ZEQ_STACKBUFFER_EMPTY)
		BuildString();
	byte* give = internal_string.data;
	internal_string.data = nullptr;
	internal_string.len = 0;
	return give;
}

size_t StackBuffer::GetLen()
{
	if (stack_top != ZEQ_STACKBUFFER_EMPTY)
		BuildString();
	return internal_string.len;
}

void StackBuffer::DoAdd(byte* str, size_t len, bool make_copy)
{
	//check if we should concat
	if (stack_top != ZEQ_STACKBUFFER_EMPTY && stack[stack_top].len < len)
	{
		size_t total_len = len;
		_BufferString* entry;
		for (int i = stack_top; i >= -1; --i)
		{
			entry = &stack[i];
			if (i == -1 || entry->len >= total_len)
			{
				//terminus; concat all entries above this one
				byte* newstring = new byte[total_len];
				size_t pos = 0;
				int new_top = i + 1;
				for (int j = new_top; j <= stack_top; ++j)
				{
					entry = &stack[j];
					memcpy(&newstring[pos],entry->data,entry->len);
					pos += entry->len;
					delete[] entry->data;
				}
				//concat new addition
				memcpy(&newstring[pos],str,len);
				if (!make_copy) {
					//the buffer owns the input string, and no longer needs it
					delete[] str;
				}
				//set new top entry
				stack_top = new_top;
				entry = &stack[new_top];
				entry->data = newstring;
				entry->len = total_len;
				return;
			}
			total_len += entry->len;
		}
	}
	else
	{
		//just insert
		if (++stack_top >= stack_capacity)
		{
			//grow stack
			stack_capacity <<= 1;
			stack = (_BufferString*)realloc(stack,sizeof(_BufferString) * stack_capacity);
		}
		_BufferString* entry = &stack[stack_top];
		if (make_copy)
		{
			byte* copy = new byte[len];
			memcpy(copy,str,len);
			entry->data = copy;
		}
		else
		{
			entry->data = str;
		}
		entry->len = len;
	}
}

void StackBuffer::BuildString()
{
	size_t total_len = 0;
	for (int i = 0; i <= stack_top; ++i)
	{
		total_len += stack[i].len;
	}

	byte* newstring;
	size_t pos;
	if (internal_string.data)
	{
		//add everything onto the existing string
		size_t ilen = internal_string.len;
		total_len += ilen;
		newstring = new byte[total_len];
		memcpy(newstring,internal_string.data,ilen);
		pos = ilen;
		delete[] internal_string.data;
	}
	else
	{
		newstring = new byte[total_len];
		pos = 0;
	}

	for (int i = 0; i <= stack_top; ++i)
	{
		_BufferString* entry = &stack[i];
		memcpy(&newstring[pos],entry->data,entry->len);
		pos += entry->len;
	}

	internal_string.data = newstring;
	internal_string.len = total_len;
}
