#pragma once
#include "SocketHandle.h"
#include "PResult.h"
#include "IPVersion.h"
#include "SocketOption.h"
#include "IPEndPoint.h"
#include "Constants.h"
#include "Packet.h"

#include <iostream>

namespace PNet
{
	class Socket
	{
	public:

		Socket(IPVersion ipversion = IPVersion::IPv4,
			   SOCKET handle = INVALID_SOCKET);

		PResult Create();
		PResult Close();

		PResult Bind(IPEndPoint endpoint);
		PResult Listen(IPEndPoint endpoint, int backlog = 5);
		PResult Accept(Socket& OutSocket, IPEndPoint* endpoint = nullptr);
		PResult Connect(IPEndPoint endpoint);

		PResult Send(const void* data, int NumOfBytes, int& bytesSent);
		PResult Recv(void* destination, int NumOfBytes, int& bytesReceived);

		PResult Send(Packet& packet);
		PResult Recv(Packet& packet);

		PResult SendAll(const void* data, int NumOfBytes);
		PResult RecvAll(void* destination, int NumOfBytes);

		PResult SetBlocking(bool isBlocking);

		SOCKET GetHandle();
		IPVersion GetIPVersion();

	private:

		PResult SetSocketOption(SocketOption option, BOOL value);
		IPVersion ipversion = IPVersion::IPv4;
		SOCKET handle = INVALID_SOCKET;

	};
}