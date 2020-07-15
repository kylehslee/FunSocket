#pragma once
#include "IPVersion.h"
#include "WS2tcpip.h"

#include <iostream>
#include <string>
#include <vector>

namespace PNet
{
	class IPEndPoint
	{
	private:
		IPVersion ipversion = IPVersion::Unknown;
		std::string hostname = "";
		std::string ip_string = "";
		std::vector<uint8_t> ip_bytes;
		unsigned short port = 0;

	public:
		IPEndPoint() {};
		IPEndPoint(const char* ip, unsigned short port);
		IPEndPoint(sockaddr* addr);

		IPVersion getIpversion();
		std::string getHostname();
		std::string getIpString();
		std::vector<uint8_t> getIpBytes();
		unsigned short getPort();
		sockaddr_in getSockAddrIPv4();
		sockaddr_in6 getSockAddrIPv6();

		void Print();
	};
}