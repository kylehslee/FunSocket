#include "Socket.h"

#include <assert.h>
#include <iostream>

namespace PNet
{
	Socket::Socket(IPVersion ipversion, SOCKET handle)
		: ipversion( ipversion ), handle( handle )
	{
		assert(this->ipversion == IPVersion::IPv4 || this->ipversion == IPVersion::IPv6);
	}

	PResult Socket::Create()
	{
		assert(this->ipversion == IPVersion::IPv4 || this->ipversion == IPVersion::IPv6);
 
		if (this->handle != INVALID_SOCKET)
		{
			return PResult::P_GenericError;
		}

		this->handle = socket((this->ipversion == IPVersion::IPv4) ? AF_INET : AF_INET6, SOCK_STREAM, IPPROTO_TCP);

		if (this->handle == INVALID_SOCKET)
		{
			int err = WSAGetLastError();
			return PResult::P_GenericError;
		}

		if (this->SetBlocking(false) != PResult::P_Success)
		{
			return PResult::P_GenericError;
		}

		if (this->SetSocketOption(SocketOption::TCP_NoDelay, TRUE) != PResult::P_Success)
		{
			return PResult::P_GenericError;
		}

		return PResult::P_Success;
	}

	PResult Socket::Close()
	{
		if (this->handle == INVALID_SOCKET)
		{
			return PResult::P_GenericError;
		}

		int result = closesocket(this->handle);

		if (result != 0)
		{
			int error = WSAGetLastError();
			return PResult::P_GenericError;
		}

		this->handle = INVALID_SOCKET;
		return PResult::P_Success;
	}

	PResult Socket::Bind(IPEndPoint endpoint)
	{
		assert(this->ipversion == endpoint.getIpversion());

		if (this->ipversion == IPVersion::IPv4)
		{
			sockaddr_in addr = endpoint.getSockAddrIPv4();
			int result = bind(this->handle, (sockaddr*)(&addr), sizeof(sockaddr_in));

			if (result != 0)
			{
				int error = WSAGetLastError();
				return PResult::P_GenericError;
			}
		}
		else
		{
			sockaddr_in6 addr = endpoint.getSockAddrIPv6();
			int result = bind(this->handle, (sockaddr*)(&addr), sizeof(sockaddr_in6));

			if (result != 0)
			{
				int error = WSAGetLastError();
				return PResult::P_GenericError;
			}
		}

		
		return PResult::P_Success;
	}

	PResult Socket::Listen(IPEndPoint endpoint, int backlog)
	{
		if (this->ipversion == IPVersion::IPv6)
		{
			if (this->SetSocketOption(SocketOption::IPV6_Only, FALSE) != PResult::P_Success)
			{
				return PResult::P_GenericError;
			}
		}

		if (this->Bind(endpoint) != PResult::P_Success)
		{
			return PResult::P_GenericError;
		}

		int result = listen(this->handle, backlog);

		if (result != 0)
		{
			int error = WSAGetLastError();
			return PResult::P_GenericError;
		}

		return PResult::P_Success;
	}

	PResult Socket::Accept(Socket& OutSocket, IPEndPoint* endpoint)
	{
		assert(this->ipversion == IPVersion::IPv4 || this->ipversion == IPVersion::IPv6);

		if (this->ipversion == IPVersion::IPv4)
		{
			sockaddr_in addr = {};
			int len = sizeof(sockaddr_in);

			SocketHandle acceptedConnectionSocket = accept(this->handle, (sockaddr*)(&addr), &len);
			if (acceptedConnectionSocket == INVALID_SOCKET)
			{
				int error = WSAGetLastError();
				return PResult::P_GenericError;
			}
			
			if (endpoint != nullptr)
			{
				*endpoint = IPEndPoint((sockaddr*)&addr);
			}

			OutSocket = Socket(IPVersion::IPv4, acceptedConnectionSocket);
		}
		else 
		{
			sockaddr_in6 addr = {};
			int len = sizeof(sockaddr_in6);

			SocketHandle acceptedConnectionSocket = accept(this->handle, (sockaddr*)(&addr), &len);
			if (acceptedConnectionSocket == INVALID_SOCKET)
			{
				int error = WSAGetLastError();
				return PResult::P_GenericError;
			}

			if (endpoint != nullptr)
			{
				*endpoint = IPEndPoint((sockaddr*)&addr);
			}

			OutSocket = Socket(IPVersion::IPv6, acceptedConnectionSocket);
		}
		

		return PResult::P_Success;
	}

	PResult Socket::Connect(IPEndPoint endpoint)
	{
		int result = 0;

		if (this->ipversion == IPVersion::IPv4)
		{
			sockaddr_in addr = endpoint.getSockAddrIPv4();
			result = connect(this->handle, (sockaddr*)(&addr), sizeof(sockaddr_in));
		}
		else
		{
			sockaddr_in6 addr = endpoint.getSockAddrIPv6();
			result = connect(this->handle, (sockaddr*)(&addr), sizeof(sockaddr_in6));
		}
		

		if (result != 0)
		{
			int error = WSAGetLastError();
			return PResult::P_GenericError;
		}

		return PResult::P_Success;
	}

	PResult Socket::Send(const void* data, int NumOfBytes, int& bytesSent)
	{
		bytesSent = send(this->handle, (const char*)data, NumOfBytes, NULL);

		if (bytesSent == SOCKET_ERROR)
		{
			int error = WSAGetLastError();
			return PResult::P_GenericError;
		}

		return PResult::P_Success;
	}

	PResult Socket::Recv(void* destination, int NumOfBytes, int& bytesReceived)
	{
		bytesReceived = recv(this->handle, (char*)destination, NumOfBytes, NULL);

		if (bytesReceived == 0)
		{
			return PResult::P_GenericError;
		}

		if (bytesReceived == SOCKET_ERROR)
		{
			int error = WSAGetLastError();
			return PResult::P_GenericError;
		}

		return PResult::P_Success;
	}

	PResult Socket::Send(Packet& packet)
	{	
		uint16_t encodedPacketSize = htons(packet.buffer.size());
		PResult result = SendAll(&encodedPacketSize, sizeof(uint16_t));
		if (result != PResult::P_Success)
		{
			return PResult::P_GenericError;
		}

		result = SendAll(packet.buffer.data(), packet.buffer.size());
		if (result != PResult::P_Success)
		{
			return PResult::P_GenericError;
		}

		return PResult::P_Success;
	}

	PResult Socket::Recv(Packet& packet)
	{
		packet.Clear();

		uint16_t encodedSize = 0;
		PResult result = RecvAll(&encodedSize, sizeof(uint16_t));

		if (result != PResult::P_Success)
		{
			return PResult::P_GenericError;
		}

		uint16_t bufferSize = ntohs(encodedSize);

		if (bufferSize > PNet::g_MaxPacketSize)
			return PResult::P_GenericError;

		packet.buffer.resize(bufferSize);

		result = RecvAll(&packet.buffer[0], bufferSize);
		if (result != PResult::P_Success)
		{
			return PResult::P_GenericError;
		}

		return PResult::P_Success;
	}

	PResult Socket::SendAll(const void* data, int NumOfBytes)
	{
		int totalBytesSent = 0;

		while (totalBytesSent < NumOfBytes)
		{
			int remainingBytes = NumOfBytes - totalBytesSent;
			int bytesSent = 0;
			char* bufferOffset = (char*)data + totalBytesSent;
			PResult result = Send(bufferOffset, remainingBytes, bytesSent);

			while (result != PResult::P_Success)
			{
				return PResult::P_GenericError;
			}

			totalBytesSent += bytesSent;
		}
		return PResult::P_Success;
	}

	PResult Socket::RecvAll(void* destination, int NumOfBytes)
	{
		int totalBytesReceived = 0;

		while (totalBytesReceived < NumOfBytes)
		{
			int remainingBytes = NumOfBytes - totalBytesReceived;
			int bytesReceived = 0;

			char* bufferOffset = (char*)destination + totalBytesReceived;

			PResult result = Recv(bufferOffset, remainingBytes, bytesReceived);

			while (result != PResult::P_Success)
			{
				return PResult::P_GenericError;
			}

			totalBytesReceived += bytesReceived;
			
		}
		return PResult::P_Success;
	}

	PResult Socket::SetBlocking(bool isBlocking)
	{
		unsigned long nonBlocking = 1;
		unsigned long blocking = 0;

		int result = ioctlsocket(this->handle, FIONBIO, isBlocking ? &blocking : &nonBlocking);
		if (result == SOCKET_ERROR)
		{
			int error = WSAGetLastError();
			return PResult::P_GenericError;
		}
		return PResult::P_Success;
	}

	SOCKET Socket::GetHandle()
	{
		return this->handle;
	}

	IPVersion Socket::GetIPVersion()
	{
		return this->ipversion;
	}

	PResult Socket::SetSocketOption(SocketOption option, BOOL value)
	{
		int result = 0;
		switch (option)
		{
		case SocketOption::TCP_NoDelay:
			result = setsockopt(this->handle, IPPROTO_TCP, TCP_NODELAY, (const char*)&value, sizeof(value));
			break;

		case SocketOption::IPV6_Only:
			result = setsockopt(this->handle, IPPROTO_IPV6, IPV6_V6ONLY, (const char*)&value, sizeof(value));
			break;

		default:
			return PResult::P_GenericError;
		}

		if (result != 0)
		{
			int error = WSAGetLastError();
			return PResult::P_GenericError;
		}
		return PResult::P_Success;
	}

}
