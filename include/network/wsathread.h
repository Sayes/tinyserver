#ifndef _WSATHREAD_H_
#define _WSATHREAD_H_

#include <map>
#include "globaldef.h"
#include "euputhread.h"
#include "netcommon.h"

class CWSAThread : public CEupuThread {
public:
	CWSAThread();
	virtual ~CWSAThread();

	virtual bool stop();
	virtual void run();
	virtual void reset();

	bool isStop()
	{
		return false;
	}

	bool startup();

private:
	bool parsePacketToRecvQueue(SOCKET_SET* psockset, char* buf, int buflen);
	bool addSocketToMap(SOCKET_SET* psockset);
	bool addClientToWSA(SOCKET_SET* psockset);
	bool doAccept(int fd);
	void doRecvMessage(SOCKET_KEY* pkey);
	int doSendMessage(SOCKET_KEY* pkey);
	bool doListen();

	void doWSAEvent();
	void doSystemEvent();
	void closeClient(int fd, time_t conn_time);
	void createClientCloseMsg(SOCKET_SET* psockset);
	bool createConnectServerMsg(SOCKET_SET* psockset);
	void doKeepaliveTimeout();
	void doSendKeepaliveToServer();
	void deleteSendMsgFromSendMap(int fd);

	time_t getIndex();

private:
	int m_listenfd;
	SOCKET_KEY* m_listenkey;
	int m_keepalivetimeout;
	int m_keepaliveinterval;
	time_t m_checkkeepalivetime;
	time_t m_lastkeepalivetime;

	string m_serverip;
	unsigned int m_serverport;

	int m_readbufsize;
	int m_sendbufsize;
	char* m_recvbuffer;
	int m_recvbuflen;

	list<int> m_delsendfdlist;
	map<int, SOCKET_SET*> m_socketmap;

	list<NET_DATA*> m_recvlist;
	time_t m_index;

};

#endif//_WSATHREAD_H_