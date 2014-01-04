
#ifndef ZEQ_SOCKET_H
#define ZEQ_SOCKET_H

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <errno.h>
#endif

#include <queue>
#include "packet.h"
#include "exception.h"

#define ZEQ_RECVBUF_SIZE 8192
#define ZEQ_SOCKET_CLOSED 0

class Socket 
{
public:
	Socket(bool blocking, const char* host, const char* port, int sock_type);
	virtual ~Socket();
	virtual void Receive() = 0;
#ifdef WIN32
	static void InitializeSocketLib();
	static void CloseSocketLib();
#endif
protected:
	SOCKET mSocket;
	char mRecvBuf[ZEQ_RECVBUF_SIZE];
	std::queue<Packet*> mPacketQueue;
};

class TCPSocket : public Socket
{
public:
	TCPSocket(const char* host, const char* port, bool blocking = false);
	virtual void Receive() override;
	virtual void Send(const byte* raw_data, size_t len);
	virtual void Send(Packet* packet);
protected:
	uint16 mRecvBuf_pos;
	uint32 mRecvBuf_end;
};

class UDPSocket : public Socket
{
public:
	UDPSocket(const char* host, const char* port, bool blocking = false);
	void Receive() override;
	void Send(const byte* raw_data, size_t len, bool ack_req = false);
	void Send(Packet* packet, bool ack_req = false);
};

class HTTPSocket : public TCPSocket
{
public:
	HTTPSocket(const char* host, bool blocking = false);
	void Receive() override;
};

#endif
