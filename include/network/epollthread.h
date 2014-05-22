#ifndef __EUPUEPOLLTHREAD_H__
#define __EUPUEPOLLTHREAD_H__

#ifdef OS_LINUX

#include <map>
#include <globaldef.h>
#include <euputhread.h>
#include <netcommon.h>

using std::map;

class CEpollThread : public CEupuThread {
public:
	CEpollThread();

	virtual ~CEpollThread();

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
	bool addClientToEpoll(SOCKET_SET* psockset);
	bool doAccept(int fd);
	void doRecvMessage(SOCKET_KEY* pkey);
	int doSendMessage(SOCKET_KEY* pkey);
	bool doListen();

	void doEpollEvent();
	void doSystemEvent();
    void closeClient(int fd, time_t conntime);
    void createClientCloseMsg(SOCKET_SET* psockset);
    bool createConnectServerMsg(SOCKET_SET* psockset);
    void doKeepaliveTimeout();
    void doSendKeepaliveToServer();
    void deleteSendMsgFromSendMap(int fd);

    time_t getIndex();

private:
	int m_epollfd;
	int m_listenfd;
	SOCKET_KEY* m_listenkey;
	int m_keepalivetimeout;		//connection timeout time
	int m_keepaliveinterval;	//send keepalive message to server time interval
	time_t m_checkkeepalivetime;
	time_t m_lastkeepalivetime;

	string m_serverip;
	unsigned int m_serverport;

	int m_readbufsize;
	int m_sendbufsize;
	char* m_recvbuffer;
	int m_recvbuflen;

	struct epoll_event* m_events;

	list<int> m_delsendfdlist;
	map<int, SOCKET_SET*> m_socketmap;

	int m_maxepollsize;

	list<NET_DATA*> m_recvlist;
	time_t m_index;
	
};

#endif//OS_LINUX

#endif//__EUPUEPOLLTHREAD_H__
