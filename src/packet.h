
#ifndef ZEQ_PACKET_H
#define ZEQ_PACKET_H

#include <cstring>
#include "type.h"

#define ZEQ_TCP_HEADER_SIZE 8 //made up numbers, look up later
#define ZEQ_UDP_HEADER_SIZE 12

class Packet
{
public:
	virtual ~Packet();
	const byte* GetData() const { return mData; }
	const size_t GetLen() const { return mLen; }
protected:
	byte*	mData;
	size_t	mLen;
};

class TCPPacket : public Packet
{
public:
	TCPPacket(const byte* data, size_t len, bool inbound = false);
};

class UDPPacket : public Packet
{
public:
	UDPPacket(const byte* data, size_t len, bool inbound = false);
	void SetAckReq();
};

#endif
