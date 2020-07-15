#include "MyClient.h"

#include <iostream>


int main()
{
	if (Network::Initialize())
	{
		MyClient client;
		if (client.Connect(IPEndPoint("::1", 6112)))
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