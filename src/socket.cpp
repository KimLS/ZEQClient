
#include "socket.h"

Socket::Socket(bool blocking, const char* host, const char* port, int sock_type)
{
	addrinfo hints;
	memset(&hints,0,sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = sock_type;
	hints.ai_protocol = (sock_type == SOCK_STREAM) ? IPPROTO_TCP : IPPROTO_UDP;
	hints.ai_flags = AI_PASSIVE;

	addrinfo* res;
	if (getaddrinfo(host,port,&hints,&res) != 0)
	{
		throw ZEQException("Could not validate remote address");
	}

	for (addrinfo* use = res; use != nullptr; use = use->ai_next)
	{
		if ((mSocket = socket(use->ai_family,use->ai_socktype,use->ai_protocol)) == SOCKET_ERROR)
		{
			continue;
		}
#ifdef WIN32
		unsigned long nonblock = 1;
#endif
		if (
			connect(mSocket,use->ai_addr,use->ai_addrlen) == SOCKET_ERROR || (!blocking &&
#ifdef WIN32
			ioctlsocket(mSocket,FIONBIO,&nonblock) == SOCKET_ERROR)
#else
			fcntl(mSocket,F_SETFL,O_NONBLOCK) == SOCKET_ERROR)
#endif
			)
		{
#ifdef WIN32
			closesocket(mSocket);
#else
			close(mSocket);
#endif
			continue;
		}
	}

	freeaddrinfo(res);

	if (mSocket == NULL || mSocket == SOCKET_ERROR)
	{
		throw ZEQException("Could not establish socket connection");
	}
}

Socket::~Socket()
{
	if (mSocket != ZEQ_SOCKET_CLOSED)
	{
#ifdef WIN32
		closesocket(mSocket);
#else
		close(mSocket);
#endif
	}
}

#ifdef WIN32
void Socket::InitializeSocketLib()
{
	WSADATA data;
	if (WSAStartup(MAKEWORD(2,0),&data) != 0)
	{
		throw ZEQException("Could not initialize WinSock");
	}
}

void Socket::CloseSocketLib()
{
	WSACleanup();
}
#endif //WIN32


TCPSocket::TCPSocket(const char* host, const char* port, bool blocking) :
Socket(blocking,host,port,SOCK_STREAM)
{
	mRecvBuf_pos = mRecvBuf_end = 0;
}

void TCPSocket::Receive()
{
	if (mRecvBuf_pos >= mRecvBuf_end)
	{
		//buffer is empty, get fresh data
	}
}

void TCPSocket::Send(const byte* data, size_t len)
{
	//set header
	//byte* out_packet = (byte*)alloca(len + ?);
	for (;;)
	{
		int sent = send(mSocket,(const char*)data,len,0);
		if (sent > 0)
		{
			return;
		}
		else if (sent == 0)
		{
			mSocket = ZEQ_SOCKET_CLOSED;
			throw ZEQException("Socket remote connection closed");
		}
		else {
#ifdef WIN32
			int err = WSAGetLastError();
			if (err != WSAEWOULDBLOCK)
			{
#else
			if (errno != EWOULDBLOCK)
			{
#endif
				throw ZEQException("Socket send operation failed");
			}
		}
	}
}

void TCPSocket::Send(Packet* packet)
{

}


UDPSocket::UDPSocket(const char* host, const char* port, bool blocking) :
Socket(blocking,host,port,SOCK_DGRAM)
{

}

void UDPSocket::Receive()
{
	for (;;)
	{
		int received = recv(mSocket,mRecvBuf,ZEQ_RECVBUF_SIZE,0);
		if (received > 0)
		{
			mPacketQueue.push(new UDPPacket((byte*)mRecvBuf,received,true));
		}
		else if (received == SOCKET_ERROR)
		{
#ifdef WIN32
			int err = WSAGetLastError();
			if (err != WSAEWOULDBLOCK)
			{
#else
			if (errno != EWOULDBLOCK)
			{
#endif
				throw ZEQException("Socket receive operation failed");
			}
			return;
		}
	}
}

void UDPSocket::Send(const byte* data, size_t len, bool ack_req)
{
	//set ack_req field if needed
	if (ack_req)
	{
		//alloca(); ?
	}
	for (;;)
	{
		int sent = send(mSocket,(const char*)data,len,0);
		if (sent > 0)
			return;
#ifdef WIN32
		int err = WSAGetLastError();
		if (err != WSAEWOULDBLOCK)
		{
#else
		if (errno != EWOULDBLOCK)
		{
#endif
			throw ZEQException("Socket send operation failed");
		}
	}
}

void UDPSocket::Send(Packet* packet, bool ack_req)
{

}


HTTPSocket::HTTPSocket(const char* host, bool blocking) :
TCPSocket(host,"80",blocking)
{

}

void HTTPSocket::Receive()
{

}
