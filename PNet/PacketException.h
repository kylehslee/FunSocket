#pragma once
#include <string>

namespace PNet
{
	class PacketException
	{
	private:
		std::string exception;

	public:
		PacketException(std::string exception)
			: exception(exception)
		{

		}

		const char* what()
		{
			return this->exception.c_str();
		}

		std::string ToString()
		{
			return this->exception;
		}

	};

}