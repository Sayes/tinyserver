// Copyright shenyizhong@gmail.com, 2014

#include "network/euputhread.h"
#include <string.h>
#include "logger/eupulogger4system.h"

CEupuThread::CEupuThread() : m_bOperate(false), m_bIsExit(true) {
    setMaskSIGUSR1();
#ifdef OS_LINUX
    m_pid = 0;
    sigemptyset(&m_waitSig);
    sigaddset(&m_waitSig, SIGUSR1);
#elif OS_WINDOWS
    memset(&m_pid, 0, sizeof(pthread_t));
    sem_init(&m_waitSig, 0, 1);
#endif
}

CEupuThread::~CEupuThread() {
#ifdef OS_WINDOWS
    sem_destroy(&m_waitSig);
#endif
    m_bOperate = false;
    m_bIsExit = false;
}

void *CEupuThread::ThreadFunc(void *arg) {
    CEupuThread *pthread = reinterpret_cast<CEupuThread *>(arg);
    pthread->run();
    return NULL;
}

void CEupuThread::setMaskSIGUSR1() {
#ifdef OS_LINUX
    sigset_t sig;
    sigemptyset(&sig);
    sigaddset(&sig, SIGUSR1);
    pthread_sigmask(SIG_BLOCK, &sig, NULL);
#elif OS_WINDOWS
// TODO(syz) , suspend, implement setMaskSIGUSR1
#endif
}

bool CEupuThread::start() {
    int nret = pthread_create(&m_pid, NULL, ThreadFunc, this);

    if (0 == nret) {
        nret = pthread_detach(m_pid);
        if (0 == nret) {
            m_bOperate = true;
            m_bIsExit = false;
        }
        return true;
    } else {
        return false;
    }
}

bool CEupuThread::stop() {
    m_bOperate = false;
    return true;
}

void CEupuThread::pause() {
#ifdef OS_LINUX
    int sig;
    sigwait(&m_waitSig, &sig);
#elif OS_WINDOWS
    // TODO(syz) , suspend, check here
    LOG(_INFO_, "CEupuThread::pause() before");
    sem_wait(&m_waitSig);
    LOG(_INFO_, "CEupuThread::pause() after");
#endif
}

// continue the thread, the real object needn't implement this method
void CEupuThread::continues() {
// send SIGUSR1 signal to current thread to continue
#ifdef OS_LINUX
    pthread_kill(m_pid, SIGUSR1);
#elif OS_WINDOWS
    // TODO(syz) , suspend, check here
    sem_post(&m_waitSig);
#endif
}

bool CEupuThread::isStarted() { return m_bOperate; }
