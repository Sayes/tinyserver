#include "euputhread.h"

CEupuThread::CEupuThread()
: m_pid(0)
, m_bOperate(false)
, m_bIsExit(true)
{
    setMaskSIGUSR1();
    sigemptyset(&m_waitSig);
    sigaddset(&m_waitSig, SIGUSR1);
}

CEupuThread::~CEupuThread()
{
    m_bOperate = false;
    m_bIsExit = false;
}

void* CEupuThread::ThreadFunc(void* arg)
{
    CEupuThread* pthread = (CEupuThread*)arg;
    pthread->run();
    return NULL;
}

void CEupuThread::setMaskSIGUSR1()
{
    sigset_t sig;
    sigemptyset(&sig);
    sigaddset(&sig, SIGUSR1);
    pthread_sigmask(SIG_BLOCK, &sig, NULL);
}

bool CEupuThread::start()
{
    int nret = pthread_create(&m_pid, NULL, ThreadFunc, this);
    
    if (0 == nret)
    {
        nret = pthread_detach(m_pid);
        if (0 == nret)
        {
            m_bOperate = true;
            m_bIsExit = false;
            return true;
        }
    }
    else
    {
        return false;
    }
    return true;
}

void CEupuThread::pause()
{
    int sig;
    sigwait(&m_waitSig, &sig);
}

//continue the thread, the real object needn't implement this method
void CEupuThread::continues()
{
    //send SIGUSR1 signal to current thread to continue
    pthread_kill(m_pid, SIGUSR1);
}

bool CEupuThread::stop()
{
    m_bOperate = false;
	return true;
}

bool CEupuThread::isStarted()
{
    return m_bOperate;
}
