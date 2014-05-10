#include <sys/epoll.h>
#include "epollthread.h"
#include "sysqueue.h"
#include "common.h"
#include "globalmgr.h"
#include "eupulogger4system.h"

CEpollThread::CEpollThread()
: CEupuThread()
, m_epollfd(-1)
, m_listenfd(-1)
, m_listenkey(NULL)
, m_maxepollsize(5000)
, m_keepalivetimeout(120)
, m_keepaliveinterval(60)
, m_serverport(0)
, m_events(NULL)
, m_recvbuffer(NULL)
, m_recvbuflen(0)
, m_Index(0)
{
	m_checkkeepalivetime = time(NULL);
	m_lastkeepalivetime = time(NULL);
}

CEpollThread::~CEpollThread()
{
	if (m_epollfd > 0)
	{
		close(m_epollfd);
	}

	if (m_listenfd > 0)
	{
		close(m_listenfd);
	}

	if (m_listenkey != NULL)
	{
		delete m_listenkey;
		m_listenkey = NULL;
	}

	map<int, SOCKET_SET*>::iterator it = m_socketmap.begin();
	for (; it != m_socketmap.end(); ++it)
	{
		delete it->second;
		it->second = NULL;
	}
	m_socketmap.clear();

	list<NET_DATA*>::iterator recviter = m_recvlist.begin();
	for (; recviter != m_recvlist.end(); ++recviter)
	{
		if ((*recviter) != NULL)
		{
			delete (*recviter);
			(*recviter) = NULL;
		}
	}
	m_recvlist.clear();

	if (m_events)
	{
		delete []m_events;
		m_events = NULL;
	}

	if (m_recvbuffer)
	{
		delete []m_recvbuffer;
		m_recvbuffer = NULL;
	}
}

// the thread work function, it will wait for the continue signal
// otherwise it will be pause
void CEpollThread::Run()
{
	pause();

	m_bIsExist = false;

	doEpollEvent();
	m_bIsExist = true;
}

void CEpollThread::Reset()
{
}

bool CEpollThread::StartUp()
{
	return false;
}

time_t CEpollThread::getIndex()
{
    m_Index++;
    if (m_Index == 0xFFFFFFFF)
        m_Index = 1;
    return m_Index;
}

void CEpollThread::doEpollEvent()
{
	int nevent = 0;
	
	SOCKET_KEY* pkey = NULL;

	while (m_bOperate)
	{
		///////begin handle epoll events///////
		nevent = epoll_wait(m_epollfd, m_events, m_maxepollsize, 1);
		for (int i = 0; i < nevent; ++i)
		{
			pkey = (SOCKET_KEY*)m_events[i].data.ptr;
			if (!pkey)
			{
				continue;
			}

			if (m_events[i].events & EPOLLIN)
			{
				if (pkey->fd == m_listenfd)
				{
					if (!doAccept(pkey->fd))
					{
						//LOG
					}
					continue;
				}

                doRecvMessage(pkey);
            }
            else if (m_events[i].events && EPOLLOUT)
            {
                if (doSendMessage(pkey) < 0)
                {
                    closeClient(pkey->fd, pkey->connect_time);
                }
                else
                {
                    struct epoll_event wev;
                    wev.events = EPOLLIN | EPOLLET;
                    wev.data.ptr = pkey;
                    if (epoll_ctl(m_epollfd, EPOLL_CTL_MOD, pkey->fd, &wev) < 0)
                    {
                        closeClient(pkey->fd, pkey->connect_time);
                    }
                }
            }
            else
            {
                closeClient(pkey->fd, pkey->connect_time);
            }
		}//end for

        //// end handle epoll events /////

        if (m_recvlist.size() > 0)
        {
            CSysQueue<NET_DATA>* precvlist = CGlobalMgr::getInstance()->getRecvQueue();
        }
	}
}


bool CEpollThread::Stop()
{
    return false;
}

void CEpollThread::doKeepaliveTimeout()
{

}

void CEpollThread::doSendkeepaliveToServer()
{
    time_t curtime = time(NULL);
    if (curtime - m_lastkeepalivetime < m_keepaliveinterval)
    {
        return;
    }

    m_lastkeepalivetime = time(NULL);

}

bool CEpollThread::doListen()
{
    bool bret = false;

    sockaddr_in serv_addr;
    m_listenfd = socket(AF_INET, SOCK_STREAM, 0);

    do {
        if (m_listenfd < 0)
        {
            LOG(_ERROR_, "CEpollThread::doListen() create listen socket error");
            break;
        }

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(m_serverport);
        if (m_serverip.empty())
        {
            serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        }
        else
        {
            serv_addr.sin_addr.s_addr = fgAtoN(m_serverip.c_str());
        }

        int tmp = 1;
        if (setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, &tmp, sizeof(tmp)) < 0)
        {
            LOG(_ERROR_, "CEpollThread::doListen() setsockopt(m_listenfd...) error");
            break;
        }

        if (!setNonBlock(m_listenfd))
        {
            LOG(_ERROR_, "CEpollThread::doLIsten() setNonBlock(m_listenfd) error");
            break;
        }

        if (bind(m_listenfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
        {
            LOG(_ERROR_, "CEpollThread::doListen() bind(m_listenfd...) error");
            break;
        }

        if (listen(m_listenfd, 4096) < 0) //max pending queue
        {
            LOG(_ERROR_, "CEpollThread::doListen() listen(m_listenfd...) error");
            break;
        }

        m_listenkey = new SOCKET_KEY;
        if (!m_listenkey)
        {
            LOG(_ERROR_, "CEpollThread::doListen() new SOCKET_KEY error");
            exit(-1);
        }

        m_listenkey->fd = m_listenfd;
        m_listenkey->connect_time = getIndex();

        struct epoll_event ev;
        ev.data.ptr = m_listenkey;
        ev.events = EPOLLIN | EPOLLET;

        if (epoll_ctl(m_epollfd, EPOLL_CTL_ADD, m_listenfd, &ev) < 0)
        {
            LOG(_ERROR_, "CEpollThread::doListen() epoll_ctl(m_epollfd...) error");
            break;
        }

        LOG(_INFO_, "CEpollThread::doListen() successed, listen and add to epoll poll, m_listenfd=%d, m_epollfd=%d ", m_listenfd, m_epollfd);
        bret = true;

    } while (false);

    return bret;
}

bool CEpollThread::doAccept(int fd)
{
    struct sockaddr_in addr = {0};
    socklen_t addrlen = sizeof(addr);
    int connfd = -1;

    while (true)
    {
        addrlen = sizeof(addr);
        memset(&addr, 0, addrlen);
        connfd = accept(fd, (sockaddr*)&addr, &addrlen);

        if (connfd < 0)
        {
            if (errno != EAGAIN)
            {
                LOG(_ERROR_, "CEpollThread::doAccept(fd) accept(fd) error, fd=%d, errno=%s", fd, strerror(errno));
            }
            return true;
        }

        string peerip = fgNtoA(ntohl(addr.sin_addr.s_addr));
        unsigned short port = ntohs(addr.sin_port);

        if (!setNonBlock(connfd))
        {
            LOG(_ERROR_, "CEpollThread::doAccept(fd) setNonBlock(fd) error, fd=%d, peerip=%s, port=%d", fd, peerip.c_str(), port);
            close(connfd);
            continue;
        }

        if (setsockopt(connfd, SOL_SOCKET, SO_SNDBUF, &m_sendbufsize, sizeof(m_sendbufsize)) < 0)
        {
            LOG(_ERROR_, "CEpollThread::doAccept(fd) setsockopt(connfd...) error, connfd=%d, peerip=%s, port=%d, error=%s", connfd, peerip.c_str(), port, strerror(errno));
            close(connfd);
            continue;
        }

        if (setsockopt(connfd, SOL_SOCKET, SO_RCVBUF, &m_readbufsize, sizeof(m_readbufsize)) < 0)
        {
            LOG(_ERROR_, "CEpollThread::doAccept(fd) setsockopt(connfd...) error, connfd=%d, peerip=%s, port=%d, error=%s", connfd, peerip.c_str(), port, strerror(errno));
            close(connfd);
            continue;
        }

        int opt = 1;
        if (setsockopt(connfd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) < 0)
        {
            LOG(_ERROR_, "CEpollThread::doAccept(fd) setsockopt(connfd...) error, connfd=%d, peerip=%s, port=%d, error=%s", connfd, peerip.c_str(), port, strerror(errno));
            close(connfd);
            continue;
        }

        SOCKET_SET* psocket = initSocketset(connfd, getIndex(), peerip, port, CLIENT_TYPE);

        if (!psocket)
        {
            LOG(_ERROR_, "CEpollThread::doAccept(fd) initSocketset(connfd...) error, connfd=%d, peerip=%s, port=%d, error=%s", connfd, peerip.c_str(), port, strerror(errno));
            close(connfd);
            continue;
        }

        if (!createConnectServerMsg(psocket))
        {
            LOG(_ERROR_, "CEpollThread::doAccept(fd) createConnectServerMsg(psocket) error, connfd=%d, conntime=%u, peerip=%s, port=%d, type=%d",
                psocket->key->fd, psocket->key->connect_time, GETNULLSTR(psocket->peer_ip), psocket->peer_port, psocket->type);
            delete psocket;
            close(connfd);
            continue;
        }

        if (!addClientToEpoll(psocket))
        {
            LOG(_ERROR_, "CEpollThread::doAccept(fd) addClientToEpoll(psocket) error, connfd=%d, conntime=%u, peerip=%s, port=%d, type=%d",
                psocket->key->fd, psocket->key->connect_time, GETNULLSTR(psocket->peer_ip), psocket->peer_port, psocket->type);
            delete psocket;
            close(connfd);
            continue;
        }
    }

    return true;
}

bool CEpollThread::addClientToEpoll(SOCKET_SET* psocket)
{
    if (psocket == NULL || psocket->key == NULL)
    {
        LOG(_ERROR_, "CEpollThread::addClientToEpoll() error"); 
        return false;
    }

    struct epoll_event ev = {0};
    ev.events = EPOLLIN | EPOLLET;
    ev.data.ptr = psocket->key;

    if (epoll_ctl(m_epollfd, EPOLL_CTL_ADD, psocket->key->fd, &ev) < 0)
    {
        LOG(_ERROR_, "CEpollThread::addClientToEpoll() epoll_ctl() error, fd=%d, error=%s", psocket->key->fd, strerror(errno));
        return false;
    }

    if (!addSocketToMap(psocket))
    {
        LOG(_ERROR_, "CEpollThread::addClientToEpoll() addSocketToMap() error, fd=%d, peerip=%s, port=%d", psocket->key->fd, GETNULLSTR(psocket->peer_ip), psocket->peer_port);
        return false;
    }

    return true;
}

void CEpollThread::doRecvMessage(SOCKET_KEY* key)
{
    if (key == NULL)
        return;

    int buflen = 0;
    int nret = 0;

    map<int, SOCKET_SET*>::iterator iter = m_socketmap.find(key->fd);

    if (iter == m_socketmap.end() || iter->second == NULL || iter->second->key == NULL)
    {
        LOG(_ERROR_, "CEpollThread::doRecvMessage() fd not found in m_socketmap, fd=%d, time=%u", key->fd, key->connect_time);
        closeClient(key->fd, key->connect_time);
        return;
    }

    if (iter->second->key != key)
    {
        LOG(_ERROR_, "CEpollThread::doRecvMessage() the found socket doesn't match, fd=%d, cur=%p, old=%p", key->fd, key, iter->second->key);
        closeClient(key->fd, key->connect_time);
        return;
    }

    SOCKET_SET* psocket = iter->second;

    while (1)
    {
        buflen = m_recvbuflen;
        memset(m_recvbuffer, 0, m_recvbuflen);

        buflen -= psocket->part_len;
        nret = recv_msg(key->fd, m_recvbuffer + psocket->part_len, buflen);

        if (nret < 0)
        {
            LOG(_ERROR_, "CEpollThread::doRecvMessage() recv_msg() error, fd=%d, time=%u, peerip=%s, port=%d", key->fd, key->connect_time, GETNULLSTR(iter->second->peer_ip), iter->second->peer_port);
            return;
        }

        psocket->refresh_time = time(NULL);
        bool bparse = true;
        if (buflen > 0)
        {
            memcpy(m_recvbuffer, psocket->part_buf, psocket->part_len);
            buflen += psocket->part_len;
            psocket->part_len = 0;
            bparse = parsePacketToRecvQueue(psocket, m_recvbuffer, buflen);
            if (!bparse)
            {
                LOG(_ERROR_, "CEpollThread::doRecvMessage() parsePacketToRecvQueue() error, fd=%d, time=%u, peerip=%s, port=%d",
                        key->fd, key->connect_time, GETNULLSTR(iter->second->peer_ip), iter->second->peer_port);
            }
        }

        if (nret == 0)
        {
            LOG(_ERROR_, "CEpollThread::doRecvMessage() error, the peer close the connection, fd=%d, time=%u, peerip=%s, port=%d",
                    key->fd, key->connect_time, GETNULLSTR(iter->second->peer_ip), iter->second->peer_port);
        }

        if (nret == 0 || !bparse)
        {
            closeClient(key->fd, key->connect_time);
            return;
        }

        if (nret == 1)
        {
            break;  //EAGAIN
        }
    }//end while
}

int CEpollThread::doSendMessage(SOCKET_KEY* key)
{
    if (!key)
        return 0;

    map<int, SOCKET_SET*>::iterator itermap = m_socketmap.find(key->fd);

    if (itermap == m_socketmap.end() || itermap->second == NULL || itermap->second->key == NULL)
    {
        LOG(_ERROR_, "CEpollThread::doSendMessage() error, not found socket, fd=%d", key->fd);
        deleteSendMsgFromSendMap(key->fd);
        return 0;
    }

    int num = 0;
    int buflen = 0;
    map<int, list<NET_DATA*> *> * psendmap = CGlobalMgr::getInstance()->getBakSendMap();
    map<int, list<NET_DATA*> *>::iterator itersend = psendmap->find(key->fd);
    if (itersend == psendmap->end())
    {
        LOG(_WARN_, "CEpollThread::doSendMessage() not found the socket %d at send msg map", key->fd);
        return 0;
    }

    if (itersend->second == NULL)
    {
        LOG(_DEBUG_, "CEpollThread::doSendMessage(), the send message list is empty");
        psendmap->erase(itersend);
        return 0;
    }

    while (itersend->second->size() > 0)
    {
        NET_DATA* pdata = itersend->second->front();
        if (!pdata)
        {
            LOG(_WARN_, "CEpollThread::doSendMessage(), send message data is empty, fd=%d", key->fd);
            itersend->second->pop_front();
            continue;
        }

        if ( !((itermap->second->key->fd == pdata->fd) && (itermap->second->key->connect_time == pdata->connect_time)) )
        {
            LOG(_ERROR_, "CEpollThread::doSendMessage(), send message data connect time is not match, cur=%u, old=%u", pdata->connect_time, itermap->second->key->connect_time);
            LOGHEX(_ERROR_, "the send message:", pdata->pdata, pdata->data_len);
            delete pdata;
            itersend->second->pop_front();
            continue;
        }

        buflen = pdata->data_len;
        int nsend = send_msg(pdata->fd, pdata->pdata, buflen);
        if (nsend < 0)
        {
            LOG(_ERROR_, "CEpollThread::doSendMessage() send_msg() error, fd=%d, time=%u, peerip=%s, port=%d, err=%s",
                    pdata->fd, pdata->connect_time, GETNULLSTR(itermap->second->peer_ip), itermap->second->peer_port, strerror(errno));
            LOGHEX(_DEBUG_, "send message failed:", pdata->pdata, pdata->data_len);
            deleteSendMsgFromSendMap(key->fd);
            return -1;
        }
        else if (nsend == 0)
        {
            num = pdata->data_len - buflen;
            if (num > 0)
            {
                LOG(_INFO_, "CEpollThread::doSendMessage() send part message success, fd=%d, time=%u, peerip=%s, port=%d",
                        pdata->fd, pdata->connect_time, GETNULLSTR(itermap->second->peer_ip), itermap->second->peer_port);
                LOGHEX(_DEBUG_, "send part message:", pdata->pdata, buflen);
                pdata->data_len = num;
                memmove(pdata->pdata, pdata->pdata + buflen, num);
            }
            else
            {
                LOG(_INFO_, "CEpollThread::doSendMessage() send message success, fd=%d, time=%u, peerip=%s, port=%d",
                        pdata->fd, pdata->connect_time, GETNULLSTR(itermap->second->peer_ip), itermap->second->peer_port);
                LOGHEX(_DEBUG_, "send message:", pdata->pdata, buflen);

                delete pdata;
                itersend->second->pop_front();
            }
            break;
        }
        else
        {
            LOG(_INFO_, "CEpollThread::doSendMessage() send message success, fd=%d, time=%u, peerip=%s, port=%d",
                    pdata->fd, pdata->connect_time, GETNULLSTR(itermap->second->peer_ip), itermap->second->peer_port);
            LOGHEX(_DEBUG_, "send message:", pdata->pdata, buflen);
            delete pdata;
            itersend->second->pop_front();
            continue;
        }
    }//end while

    if (itersend->second->size() <= 0)
    {
        delete itersend->second;
        itersend->second = NULL;
        psendmap->erase(itersend);
    }
    return 1; 
}

bool CEpollThread::parsePacketToRecvQueue(SOCKET_SET* psocket, char* buf, int buflen)
{
    return true;
}

void CEpollThread::closeClient(int fd, time_t conntime)
{

}

void CEpollThread::createClientCloseMsg(SOCKET_SET* key)
{
}

bool CEpollThread::addSocketToMap(SOCKET_SET* psocket)
{
    return false;
}

void CEpollThread::deleteSendMsgFromSendMap(int fd)
{
}

void CEpollThread::doSystemEvent()
{
}

bool CEpollThread::createConnectServerMsg(SOCKET_SET* psocket)
{
    return false;
}

