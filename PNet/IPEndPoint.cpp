#include "IPEndPoint.h"
#include "Helpers.h"

#include <assert.h>

namespace PNet
{

	IPEndPoint::IPEndPoint(const char* ip, unsigned short port)
	{
		//IPv4
		this->port = port;
		in_addr addr;
		int result = inet_pton(AF_INET, ip, &addr);

		if (result == 1)
		{
			if (addr.S_un.S_addr != INADDR_NONE)
			{
				this->ip_string = ip;
				this->hostname = ip;

				Helpers::trim(this->ip_string);
				Helpers::trim(this->hostname);

				this->ipversion = IPVersion::IPv4;
				this->ip_bytes.resize(sizeof(ULONG));
				memcpy(&this->ip_bytes[0], &addr.S_un.S_addr, sizeof(ULONG));
				return;
			}	
		}

		addrinfo hints = {};
		hints.ai_family = AF_INET;
		addrinfo* hostinfo = nullptr;
		result = getaddrinfo(ip, NULL, &hints, &hostinfo);
		
		if (result == 0)
		{
			sockaddr_in* host_addr = reinterpret_cast<sockaddr_in*>(hostinfo->ai_addr);

			this->ip_string.resize(16);
			inet_ntop(AF_INET, &host_addr->sin_addr, &this->ip_string[0], 16);

			this->hostname = ip;

			Helpers::trim(this->ip_string);
			Helpers::trim(this->hostname);

			ULONG ip_long = host_addr->sin_addr.S_un.S_addr;
			this->ip_bytes.resize(sizeof(ULONG));
			memcpy(&this->ip_bytes[0], &ip_long, sizeof(ULONG));

			this->ipversion = IPVersion::IPv4;

			freeaddrinfo(hostinfo);

			return;
		}

		//IPv6
		in6_addr addrV6;
		result = inet_pton(AF_INET6, ip, &addrV6);

		if (result == 1)
		{
			this->ip_string = ip;
			this->hostname = ip;

			Helpers::trim(this->ip_string);
			Helpers::trim(this->hostname);

			this->ipversion = IPVersion::IPv6;
			this->ip_bytes.resize(16);
			memcpy(&this->ip_bytes[0], &addrV6.u, 16);
			return;
		}

		addrinfo hintsV6 = {};
		hintsV6.ai_family = AF_INET6;
		addrinfo* hostinfoV6 = nullptr;
		result = getaddrinfo(ip, NULL, &hintsV6, &hostinfoV6);

		if (result == 0)
		{
			sockaddr_in6* host_addr = reinterpret_cast<sockaddr_in6*>(hostinfoV6->ai_addr);

			this->ip_string.resize(46);
			inet_ntop(AF_INET6, &host_addr->sin6_addr, &this->ip_string[0], 46);

			this->hostname = ip;

			Helpers::trim(this->ip_string);
			Helpers::trim(this->hostname); 

			this->ip_bytes.resize(16);
			memcpy(&this->ip_bytes[0], &host_addr->sin6_addr, 16);

			this->ipversion = IPVersion::IPv6;

			freeaddrinfo(hostinfo);

			return;
		}
	}

	IPEndPoint::IPEndPoint(sockaddr* addr)
	{
		assert(addr->sa_family == AF_INET || addr->sa_family == AF_INET6);

		if (addr->sa_family == AF_INET)
		{
			sockaddr_in* addvr4 = reinterpret_cast<sockaddr_in*>(addr);

			this->ipversion = IPVersion::IPv4;
			this->port = ntohs(addvr4->sin_port);

			this->ip_bytes.resize(sizeof(ULONG));
			memcpy(&this->ip_bytes[0], &addvr4->sin_addr, sizeof(ULONG));

			this->ip_string.resize(16);
			inet_ntop(AF_INET, &addvr4->sin_addr, &this->ip_string[0], 16);

			this->hostname = this->ip_string;
		}
		else
		{
			sockaddr_in6* addvr6 = reinterpret_cast<sockaddr_in6*>(addr);

			this->ipversion = IPVersion::IPv6;
			this->port = ntohs(addvr6->sin6_port);

			this->ip_bytes.resize(16);
			memcpy(&this->ip_bytes[0], &addvr6->sin6_addr, 16);

			this->ip_string.resize(46);
			inet_ntop(AF_INET, &addvr6->sin6_addr, &this->ip_string[0], 46);

			this->hostname = this->ip_string;
		}
		Helpers::trim(this->ip_string);
		Helpers::trim(this->hostname);
	}

	IPVersion IPEndPoint::getIpversion()
	{
		return this->ipversion;
	}

	std::string IPEndPoint::getHostname()
	{
		return this->hostname;
	}

	std::string IPEndPoint::getIpString()
	{
		return this->ip_string;
	}

	std::vector<uint8_t> IPEndPoint::getIpBytes()
	{
		return this->ip_bytes;
	}

	unsigned short IPEndPoint::getPort()
	{
		return this->port;
	}

	sockaddr_in IPEndPoint::getSockAddrIPv4()
	{
		assert(this->ipversion == IPVersion::IPv4);
		sockaddr_in addr = {};
		addr.sin_port = htons(this->port);
		addr.sin_family = AF_INET;
		memcpy(&addr.sin_addr, &this->ip_bytes[0], sizeof(ULONG));

		return addr;
	}

	sockaddr_in6 IPEndPoint::getSockAddrIPv6()
	{
		assert(this->ipversion == IPVersion::IPv6);
		sockaddr_in6 addr = {};
		addr.sin6_port = htons(this->port);
		addr.sin6_family = AF_INET6;
		memcpy(&addr.sin6_addr, &this->ip_bytes[0], 16);

		return addr;
	}

	void IPEndPoint::Print()
	{
		switch (this->ipversion)
		{
		case IPVersion::IPv4 :
			std::cout << "This is IP Version 4" << std::endl;
			break;

		case IPVersion::IPv6:
			std::cout << "This is IP Version 6" << std::endl;
			break;

		default:
			std::cout << "This is IP Version Unknown" << std::endl;
			break;
		}
		std::cout << "Host name: " << this->hostname << std::endl;
		std::cout << "IP: " << this->ip_string << " : " << this->port << std::endl;
		std::cout << "IP bytes: " << std::endl;
		for (auto& x : this->ip_bytes)
		{
			std::cout << (int)x << " ";
		}
		std::cout << std::endl;
	}
};
