#include "MyClient.h"


int main()
{
	if (Network::Initialize())
	{
		MyClient client;
		if (client.Connect(IPEndPoint("192.168.0.137", 6112)))
		{
			while (client.IsConnected())
			{
				client.Frame();
			}
		}
	}
	Network::Shutdown();
	system("pause");
}