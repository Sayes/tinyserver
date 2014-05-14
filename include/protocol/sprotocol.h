#ifndef _SPROTOCOL_H_
#define _SPROTOCOL_H_

#include "globaldef.h"
#include "eupustream.h"
#include "protocol.h"

#define RS_SERVER_CONNECTED             11
#define RS_SERVER_DISCONNECTED          12

struct MP_Server_Connected : public CEupuStream {
    MP_Server_Connected()
    {
        msgHead.uMainID = RS_SERVER_CONNECTED;
        msgHead.uMessageSize = NET_HEAD_SIZE + sizeof(int);
        m_nServer = -1;
    }

    NetMessageHead msgHead;
    int m_nServer;

    void Debug()
    {
    }
    bool Out(BYTE* pDest, UINT& nlen)
    {
        INT32 ntmp = 0;
        INT32 nret = 0;
        UINT nbuffer = nlen;

        if (msgHead.Out(pDest, nbuffer))
        {
            ntmp = nbuffer;
            nret = OutputValue(pDest + ntmp, nbuffer - ntmp, m_nServer);

            if (nret < 0)
            {
                return false;
            }

            ntmp += nret;
            nlen = ntmp;

            return true;
        }
        return false;
    }

    bool In(BYTE* pSrc, UINT& nlen)
    {
        INT32 ntmp = 0;
        INT32 nret = 0;
        UINT nbuflen = nlen;

        if (msgHead.In(pSrc, nbuflen))
        {
            ntmp = nbuflen;
            nret = InputValue(pSrc + ntmp, nbuflen - ntmp, m_nServer);
            if (nret < 0)
            {
                return false;
            }
            ntmp += nret;
            nlen = ntmp;

            return true;
        }
        return false;
    }
};

struct MP_Server_DisConnected : public CEupuStream {
    MP_Server_DisConnected()
    {
        msgHead.uMainID = RS_SERVER_DISCONNECTED;
        msgHead.uMessageSize = NET_HEAD_SIZE + sizeof(int);
        m_nServer = -1;
    }

    NetMessageHead msgHead;
    int m_nServer;

    void Debug()
    {
    }

    bool Out(BYTE* pDest, UINT& nlen)
    {
        INT32 ntmp = 0;
        INT32 nret = 0;
        UINT nbuflen = nlen;

        if (msgHead.Out(pDest, nbuflen))
        {
            ntmp = nbuflen;
            nret = OutputValue(pDest + ntmp, nbuflen - ntmp, m_nServer);

            if (nret < 0)
            {
                return false;
            }

            ntmp += nret;
            nlen = ntmp;

            return true;
        }
        return false;
    }

    bool In(BYTE* pSrc, UINT& nlen)
    {
        INT32 ntmp = 0;
        INT32 nret = 0;
        UINT nbuflen = nlen;

        if (msgHead.In(pSrc, nbuflen))
        {
            ntmp = nbuflen;
            nret = InputValue(pSrc + ntmp, nbuflen - ntmp, m_nServer);
            if (nret < 0)
            {
                return false;
            }

            ntmp += nret;

            nlen = ntmp;

            return true;
        }
        return false;
    }
};

#endif//_SPROTOCOL_H_
