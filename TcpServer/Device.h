#ifndef DeviceH
#define DeviceH

#include <string>
#include <SysUtils.hpp>
#include <Classes.hpp>
#include <vcl.h>
#include <ScktComp.hpp>
#include <ExtCtrls.hpp>
#include <map>
#include "Mutex.h"

class Device {
public:
	typedef void (*pfcnReceiveCmd)(std::string cmd, void* opaque);
	typedef void (*pfcnPoll)(void* opaque);
private:
	int port;
	TServerSocket *Server;
	TTimer *timerIdle;

	std::string cmd;
	void ProcessCmd(std::string cmd);
	int SendCmd(std::string cmd, bool hideLog);
	int ConnClose(void);

	pfcnReceiveCmd onReceiveCb;
	void* onReceiveOpaque;

	pfcnPoll onPoll;
	void* onPollOpaque;

	void __fastcall ServerClientRead(TObject *Sender, TCustomWinSocket *Socket);
	void __fastcall ServerClientConnect(TObject *Sender, TCustomWinSocket *Socket);
	void __fastcall ServerClientDisconnect(TObject *Sender,
          TCustomWinSocket *Socket);
	void __fastcall ServerClientError(TObject *Sender, TCustomWinSocket *Socket,
		  TErrorEvent ErrorEvent, int &ErrorCode);

	void __fastcall OnTimerIdle(TObject *Sender);

	struct User
	{
		AnsiString cmd;
		unsigned int idleTime;
		User(void):
			idleTime(0)
		{}
	};
	std::map<TCustomWinSocket *, struct User> users;	


public:
	Device(void);
	void SetReceiveCmdCb(pfcnReceiveCmd onReceiveCb, void* onReceiveOpaque) {
		this->onReceiveCb = onReceiveCb;
		this->onReceiveOpaque = onReceiveOpaque;
	}
	void SetPollCb(pfcnPoll onPollCb, void* onPollOpaque) {
		this->onPoll = onPollCb;
		this->onPollOpaque = onPollOpaque;
	}
	int Configure(int port);
	int Start(void);
	void Stop(void);
	void Send(AnsiString msg);
};

#endif
