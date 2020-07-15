#include "MyClient.h"

#include <iostream>


int main()
{
	if (Network::Initialize())
	{
		MyClient client;
		if (client.Connect(IPEndPoint("192.168.0.147", 6112)))
		{
			while (client.IsConnected())
			{
				client.Frame();
			}
		}
	}
	
	Network::Shutdown();
	system("pause");
	return 0;
}