#include "MyServer.h"


int main()
{
	if (Network::Initialize())
	{
		MyServer server;
		if (server.Initialize(IPEndPoint("192.168.0.137", 6112)))
		{
			while (true)
			{
				server.Frame();
			}
		}
	}

	Network::Shutdown();
	system("pause");

	return 0;
}

