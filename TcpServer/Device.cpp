#include "Device.h"
#include "Log.h"
#include "ScopedLock.h"
#include "Utils.h"
#include <assert.h>

#define TRACE LOG("%s/%04d", __FILE__, __LINE__);


Device::Device(void):
	port(12340),
	Server(NULL),
	timerIdle(NULL),
	onReceiveCb(NULL),
	onReceiveOpaque(NULL),
	onPoll(NULL),
	onPollOpaque(NULL)
{
}

int Device::Configure(int port) {
	this->port = port;
	return 0;
}

int Device::Start(void) {
	assert(onReceiveCb);
	if (Server)
		delete Server;
	Server = new TServerSocket(NULL);
	Server->Port = port;
	Server->ServerType = stNonBlocking;
	Server->OnClientConnect = ServerClientConnect;
	Server->OnClientDisconnect = ServerClientDisconnect;
	Server->OnClientRead = ServerClientRead;
	Server->OnClientError = ServerClientError;
	try
	{
		Server->Active = true;
	}
	catch (...)
	{
		return -1;
	}
	if (timerIdle == NULL)
	{
		timerIdle = new TTimer(NULL);
		timerIdle->Interval = 1000;
		timerIdle->OnTimer = OnTimerIdle;
		timerIdle->Enabled = true;
	}	
	return 0;
}

void Device::Stop(void)
{
	if (timerIdle)
	{
		delete timerIdle;
		timerIdle = NULL;
	}
	if (Server)
	{
		delete Server;
		Server = NULL;
	}
}

void Device::ProcessCmd(std::string cmd) {
	//LOG("RX: %s", cmd.c_str());
	onReceiveCb(cmd, onReceiveOpaque);
}

void __fastcall Device::ServerClientRead(TObject *Sender,
      TCustomWinSocket *Socket)
{
	User& user = users[Socket];
	AnsiString txt = Socket->ReceiveText();
	LOG("RX: %s", txt.c_str());
	if (user.cmd.Length() > 100000)
	{
		LOG("No newline found in received text with over 100000 bytes in buffer!\n");
		user.cmd = "";
		return;
	}
	user.cmd = user.cmd + txt;
	user.idleTime = 0;
	AnsiString send;
	if (int n = user.cmd.LastDelimiter("\n"))
	{
		AnsiString send = user.cmd.SubString(1, n);
		if (send != "")
		{
			onReceiveCb(send.c_str(), onReceiveOpaque);
		}
		user.cmd = user.cmd.SubString(n + 1, txt.Length() - n);
	}
}
//---------------------------------------------------------------------------

void Device::Send(AnsiString msg)
{
	for (int i = 0; i < Server->Socket->ActiveConnections; i++)
    {
		TCustomWinSocket *s = Server->Socket->Connections[i];
		s->SendText(msg);
		users[s].idleTime = 0;
    }
}
void __fastcall Device::ServerClientConnect(TObject *Sender,
      TCustomWinSocket *Socket)
{
	users[Socket].cmd = "";
	users[Socket].idleTime = 0;	
}
//---------------------------------------------------------------------------
void __fastcall Device::ServerClientDisconnect(TObject *Sender,
      TCustomWinSocket *Socket)
{
	users.erase(Socket);	
}
//---------------------------------------------------------------------------
void __fastcall Device::ServerClientError(TObject *Sender,
      TCustomWinSocket *Socket, TErrorEvent ErrorEvent, int &ErrorCode)
{
	LOG("Server: err %d", ErrorCode);
	ErrorCode=0;
}

void __fastcall Device::OnTimerIdle(TObject *Sender)
{
	if (Server == NULL)
		return;
	for (std::map<TCustomWinSocket *, struct User>::iterator iter = users.begin(); iter != users.end(); ++iter)
	{
		iter->second.idleTime++;
		if (iter->second.idleTime >= 60)
		{
			iter->second.idleTime = 0;
			iter->first->SendText("\n");
		}
	}
}



