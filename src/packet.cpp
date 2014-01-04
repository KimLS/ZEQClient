
#include "packet.h"

Packet::~Packet()
{
	delete[] mData;
}


TCPPacket::TCPPacket(const byte* data, size_t len, bool inbound)
{
	if (inbound) {
		//just copy data
		mData = new byte[len];
		memcpy(mData,data,len);
		mLen = len;
	}
	else {
		//outbound, must add header
		mData = new byte[len + ZEQ_TCP_HEADER_SIZE];
		//set header fields
		//...
		//set data
		memcpy(&mData[ZEQ_TCP_HEADER_SIZE],data,len);
		mLen = len + ZEQ_TCP_HEADER_SIZE;
	}
}


UDPPacket::UDPPacket(const byte* data, size_t len, bool inbound)
{
	if (inbound) {
		//just copy data
		mData = new byte[len];
		memcpy(mData,data,len);
		mLen = len;
	}
	else {
		//outbound, must add header
		mData = new byte[len + ZEQ_UDP_HEADER_SIZE];
		//set header fields
		//...
		//set data
		memcpy(&mData[ZEQ_UDP_HEADER_SIZE],data,len);
		mLen = len + ZEQ_UDP_HEADER_SIZE;
	}
}
