#include "MyServer.h"


int main()
{
	if (Network::Initialize())
	{
		MyServer server;
		if (server.Initialize(IPEndPoint("0.0.0.0", 6112)))
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

