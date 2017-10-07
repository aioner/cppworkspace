// CIOCPServer.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"

/*
int _tmain(int argc, _TCHAR* argv[])
{
	return 0;
}*/

////////////////////////////////////////
// IOCP.h�ļ�

#ifndef __IOCP_H__
#define __IOCP_H__

#include <winsock2.h>
#include <windows.h>
#include <Mswsock.h>

#define BUFFER_SIZE 1024*4	// I/O����Ļ�������С
#define MAX_THREAD	2		// I/O�����̵߳�����


// ����per-I/O���ݡ������������׽����ϴ���I/O�����ı�Ҫ��Ϣ
struct CIOCPBuffer
{
	WSAOVERLAPPED ol;

	SOCKET sClient;			// AcceptEx���յĿͻ����׽���

	char *buff;				// I/O����ʹ�õĻ�����
	int nLen;				// buff��������ʹ�õģ���С

	ULONG nSequenceNumber;	// ��I/O�����к�

	int nOperation;			// ��������
#define OP_ACCEPT	1
#define OP_WRITE	2
#define OP_READ		3

	CIOCPBuffer *pNext;
};

// ����per-Handle���ݡ���������һ���׽��ֵ���Ϣ
struct CIOCPContext
{
	SOCKET s;						// �׽��־��

	SOCKADDR_IN addrLocal;			// ���ӵı��ص�ַ
	SOCKADDR_IN addrRemote;			// ���ӵ�Զ�̵�ַ

	BOOL bClosing;					// �׽����Ƿ�ر�

	int nOutstandingRecv;			// ���׽������׳����ص�����������
	int nOutstandingSend;


	ULONG nReadSequence;			// ���Ÿ����յ���һ�����к�
	ULONG nCurrentReadSequence;		// ��ǰҪ�������к�
	CIOCPBuffer *pOutOfOrderReads;	// ��¼û�а�˳����ɵĶ�I/O

	CRITICAL_SECTION Lock;			// ��������ṹ

	bool bNotifyCloseOrError;		// [2009.8.22 add Lostyears][���׽��ֹرջ����ʱ�Ƿ���֪ͨ��]

	CIOCPContext *pNext;
};


class CIOCPServer   // �����߳�
{
public:
	CIOCPServer();
	~CIOCPServer();

	// ��ʼ����
	BOOL Start(int nPort = 4567, int nMaxConnections = 2000, 
		int nMaxFreeBuffers = 200, int nMaxFreeContexts = 100, int nInitialReads = 4); 
	// ֹͣ����
	void Shutdown();

	// �ر�һ�����Ӻ͹ر���������
	void CloseAConnection(CIOCPContext *pContext);
	void CloseAllConnections();	

	// ȡ�õ�ǰ����������
	ULONG GetCurrentConnection() { return m_nCurrentConnection; }

	// ��ָ���ͻ������ı�
	BOOL SendText(CIOCPContext *pContext, char *pszText, int nLen); 

protected:

	// ������ͷŻ���������
	CIOCPBuffer *AllocateBuffer(int nLen);
	void ReleaseBuffer(CIOCPBuffer *pBuffer);

	// ������ͷ��׽���������
	CIOCPContext *AllocateContext(SOCKET s);
	void ReleaseContext(CIOCPContext *pContext);

	// �ͷſ��л����������б��Ϳ��������Ķ����б�
	void FreeBuffers();
	void FreeContexts();

	// �������б�������һ������
	BOOL AddAConnection(CIOCPContext *pContext);

	// ������Ƴ�δ���Ľ�������
	BOOL InsertPendingAccept(CIOCPBuffer *pBuffer);
	BOOL RemovePendingAccept(CIOCPBuffer *pBuffer);

	// ȡ����һ��Ҫ��ȡ��
	CIOCPBuffer *GetNextReadBuffer(CIOCPContext *pContext, CIOCPBuffer *pBuffer);


	// Ͷ�ݽ���I/O������I/O������I/O
	BOOL PostAccept(CIOCPBuffer *pBuffer);
	BOOL PostSend(CIOCPContext *pContext, CIOCPBuffer *pBuffer);
	BOOL PostRecv(CIOCPContext *pContext, CIOCPBuffer *pBuffer);

	void HandleIO(DWORD dwKey, CIOCPBuffer *pBuffer, DWORD dwTrans, int nError);


	// [2009.8.22 add Lostyears]
	// ���׼��ֹرջ����ʱ֪ͨ
	void NotifyConnectionClosing(CIOCPContext *pContext, CIOCPBuffer *pBuffer);
	void NotifyConnectionError(CIOCPContext *pContext, CIOCPBuffer *pBuffer, int nError);

	// �¼�֪ͨ����
	// ������һ���µ�����
	virtual void OnConnectionEstablished(CIOCPContext *pContext, CIOCPBuffer *pBuffer);
	// һ�����ӹر�
	virtual void OnConnectionClosing(CIOCPContext *pContext, CIOCPBuffer *pBuffer);
	// ��һ�������Ϸ����˴���
	virtual void OnConnectionError(CIOCPContext *pContext, CIOCPBuffer *pBuffer, int nError);
	// һ�������ϵĶ��������
	virtual void OnReadCompleted(CIOCPContext *pContext, CIOCPBuffer *pBuffer);
	// һ�������ϵ�д�������
	virtual void OnWriteCompleted(CIOCPContext *pContext, CIOCPBuffer *pBuffer);

protected:

	// ��¼���нṹ��Ϣ
	CIOCPBuffer *m_pFreeBufferList;
	CIOCPContext *m_pFreeContextList;
	int m_nFreeBufferCount;
	int m_nFreeContextCount;	
	CRITICAL_SECTION m_FreeBufferListLock;
	CRITICAL_SECTION m_FreeContextListLock;

	// ��¼�׳���Accept����
	CIOCPBuffer *m_pPendingAccepts;   // �׳������б���
	long m_nPendingAcceptCount;
	CRITICAL_SECTION m_PendingAcceptsLock;

	// ��¼�����б�
	CIOCPContext *m_pConnectionList;
	int m_nCurrentConnection;
	CRITICAL_SECTION m_ConnectionListLock;

	// ����Ͷ��Accept����
	HANDLE m_hAcceptEvent;
	HANDLE m_hRepostEvent;
	LONG m_nRepostCount;

	int m_nPort;				// �����������Ķ˿�

	int m_nInitialAccepts;		// ��ʼʱ�׳����첽����Ͷ����
	int m_nInitialReads;
	int m_nMaxAccepts;			// �׳����첽����Ͷ�������ֵ
	int m_nMaxSends;			// �׳����첽����Ͷ�������ֵ(����Ͷ�ݵķ��͵���������ֹ�û����������ݶ������գ����·������׳��������Ͳ���)
	int m_nMaxFreeBuffers;		// �ڴ�������ɵ�����ڴ����(�������������������ͷ��ڴ��)
	int m_nMaxFreeContexts;		// ������[�׽�����Ϣ]�ڴ�������ɵ�����������ڴ����(�������������������ͷ��������ڴ��)
	int m_nMaxConnections;		// ���������

	HANDLE m_hListenThread;			// �����߳�
	HANDLE m_hCompletion;			// ��ɶ˿ھ��
	SOCKET m_sListen;				// �����׽��־��
	LPFN_ACCEPTEX m_lpfnAcceptEx;	// AcceptEx������ַ
	LPFN_GETACCEPTEXSOCKADDRS m_lpfnGetAcceptExSockaddrs; // GetAcceptExSockaddrs������ַ

	BOOL m_bShutDown;		// ����֪ͨ�����߳��˳�
	BOOL m_bServerStarted;	// ��¼�����Ƿ�����

	CRITICAL_SECTION m_CloseOrErrLock;	// [2009.9.1 add Lostyears]

private:	// �̺߳���
	static DWORD WINAPI _ListenThreadProc(LPVOID lpParam);
	static DWORD WINAPI _WorkerThreadProc(LPVOID lpParam);
};


#endif // __IOCP_H__
//////////////////////////////////////////////////
// IOCP.cpp�ļ�

//#include "iocp.h"
#pragma comment(lib, "WS2_32.lib")
#include <stdio.h>
#include <mstcpip.h>

CIOCPServer::CIOCPServer()
{
	// �б�
	m_pFreeBufferList = NULL;
	m_pFreeContextList = NULL;	
	m_pPendingAccepts = NULL;
	m_pConnectionList = NULL;

	m_nFreeBufferCount = 0;
	m_nFreeContextCount = 0;
	m_nPendingAcceptCount = 0;
	m_nCurrentConnection = 0;

	::InitializeCriticalSection(&m_FreeBufferListLock);
	::InitializeCriticalSection(&m_FreeContextListLock);
	::InitializeCriticalSection(&m_PendingAcceptsLock);
	::InitializeCriticalSection(&m_ConnectionListLock);
	::InitializeCriticalSection(&m_CloseOrErrLock);	// [2009.9.1 add Lostyears]

	// Accept����
	m_hAcceptEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hRepostEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	m_nRepostCount = 0;

	m_nPort = 4567;

	m_nInitialAccepts = 10;
	m_nInitialReads = 4;
	m_nMaxAccepts = 100;
	m_nMaxSends = 20;
	m_nMaxFreeBuffers = 200;
	m_nMaxFreeContexts = 100;
	m_nMaxConnections = 2000;

	m_hListenThread = NULL;
	m_hCompletion = NULL;
	m_sListen = INVALID_SOCKET;
	m_lpfnAcceptEx = NULL;
	m_lpfnGetAcceptExSockaddrs = NULL;

	m_bShutDown = FALSE;
	m_bServerStarted = FALSE;

	// ��ʼ��WS2_32.dll
	WSADATA wsaData;
	WORD sockVersion = MAKEWORD(2, 2);
	::WSAStartup(sockVersion, &wsaData);
}

CIOCPServer::~CIOCPServer()
{
	Shutdown();

	if(m_sListen != INVALID_SOCKET)
		::closesocket(m_sListen);
	if(m_hListenThread != NULL)
		::CloseHandle(m_hListenThread);

	::CloseHandle(m_hRepostEvent);
	::CloseHandle(m_hAcceptEvent);

	::DeleteCriticalSection(&m_FreeBufferListLock);
	::DeleteCriticalSection(&m_FreeContextListLock);
	::DeleteCriticalSection(&m_PendingAcceptsLock);
	::DeleteCriticalSection(&m_ConnectionListLock);
	::DeleteCriticalSection(&m_CloseOrErrLock); // [2009.9.1 add Lostyears]

	::WSACleanup();	
}


///////////////////////////////////
// �Զ����������

CIOCPBuffer *CIOCPServer::AllocateBuffer(int nLen)
{
	CIOCPBuffer *pBuffer = NULL;
	if(nLen > BUFFER_SIZE)
		return NULL;

	// Ϊ���������������ڴ�
	::EnterCriticalSection(&m_FreeBufferListLock);
	if(m_pFreeBufferList == NULL)  // �ڴ��Ϊ�գ������µ��ڴ�
	{
		pBuffer = (CIOCPBuffer *)::HeapAlloc(GetProcessHeap(), 
			HEAP_ZERO_MEMORY, sizeof(CIOCPBuffer) + BUFFER_SIZE);
	}
	else	// ���ڴ����ȡһ����ʹ��
	{
		pBuffer = m_pFreeBufferList;
		m_pFreeBufferList = m_pFreeBufferList->pNext;	
		pBuffer->pNext = NULL;
		m_nFreeBufferCount --;
	}
	::LeaveCriticalSection(&m_FreeBufferListLock);

	// ��ʼ���µĻ���������
	if(pBuffer != NULL)
	{
		pBuffer->buff = (char*)(pBuffer + 1);
		pBuffer->nLen = nLen;
		//::ZeroMemory(pBuffer->buff, pBuffer->nLen);
	}
	return pBuffer;
}

void CIOCPServer::ReleaseBuffer(CIOCPBuffer *pBuffer)
{
	::EnterCriticalSection(&m_FreeBufferListLock);

	if(m_nFreeBufferCount < m_nMaxFreeBuffers)	// ��Ҫ�ͷŵ��ڴ����ӵ������б��� [2010.5.15 mod Lostyears]old:m_nFreeBufferCount <= m_nMaxFreeBuffers
	{
		memset(pBuffer, 0, sizeof(CIOCPBuffer) + BUFFER_SIZE);
		pBuffer->pNext = m_pFreeBufferList;
		m_pFreeBufferList = pBuffer;

		m_nFreeBufferCount ++ ;
	}
	else			// �Ѿ��ﵽ���ֵ���������ͷ��ڴ�
	{
		::HeapFree(::GetProcessHeap(), 0, pBuffer);
	}

	::LeaveCriticalSection(&m_FreeBufferListLock);
}


CIOCPContext *CIOCPServer::AllocateContext(SOCKET s)
{
	CIOCPContext *pContext;

	// ����һ��CIOCPContext����
	::EnterCriticalSection(&m_FreeContextListLock);
	if(m_pFreeContextList == NULL)
	{
		pContext = (CIOCPContext *)
			::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CIOCPContext)); 

		::InitializeCriticalSection(&pContext->Lock);
	}
	else	
	{
		// �ڿ����б�������
		pContext = m_pFreeContextList;
		m_pFreeContextList = m_pFreeContextList->pNext;
		pContext->pNext = NULL;

		m_nFreeContextCount --; // [2009.8.9 mod Lostyears][old: m_nFreeBufferCount--] 
	}

	::LeaveCriticalSection(&m_FreeContextListLock);

	// ��ʼ�������Ա
	if(pContext != NULL)
	{
		pContext->s = s;

		// [2009.8.22 add Lostyears]
		pContext->bNotifyCloseOrError = false;
	}

	return pContext;
}

void CIOCPServer::ReleaseContext(CIOCPContext *pContext)
{
	if(pContext->s != INVALID_SOCKET)
		::closesocket(pContext->s);

	// �����ͷţ�����еĻ������׽����ϵ�û�а�˳����ɵĶ�I/O�Ļ�����
	CIOCPBuffer *pNext;
	while(pContext->pOutOfOrderReads != NULL)
	{
		pNext = pContext->pOutOfOrderReads->pNext;
		ReleaseBuffer(pContext->pOutOfOrderReads);
		pContext->pOutOfOrderReads = pNext;
	}

	::EnterCriticalSection(&m_FreeContextListLock);

	if(m_nFreeContextCount < m_nMaxFreeContexts) // ���ӵ������б� [2010.4.10 mod Lostyears][old: m_nFreeContextCount <= m_nMaxFreeContexts]���m_nFreeContextCount==m_nMaxFreeContextsʱ��������һ�ε���m_nFreeContextCount>m_nMaxFreeContexts

	{
		// �Ƚ��ؼ�����α������浽һ����ʱ������
		CRITICAL_SECTION cstmp = pContext->Lock;

		// ��Ҫ�ͷŵ������Ķ����ʼ��Ϊ0
		memset(pContext, 0, sizeof(CIOCPContext));

		// �ٷŻ�ؼ�����α�������Ҫ�ͷŵ������Ķ������ӵ������б��ı�ͷ
		pContext->Lock = cstmp;
		pContext->pNext = m_pFreeContextList;
		m_pFreeContextList = pContext;

		// ���¼���
		m_nFreeContextCount ++;
	}
	else // �Ѿ��ﵽ���ֵ���������ͷ�
	{
		::DeleteCriticalSection(&pContext->Lock);
		::HeapFree(::GetProcessHeap(), 0, pContext);
		pContext = NULL;
	}

	::LeaveCriticalSection(&m_FreeContextListLock);
}

void CIOCPServer::FreeBuffers()
{
	// ����m_pFreeBufferList�����б����ͷŻ��������ڴ�
	::EnterCriticalSection(&m_FreeBufferListLock);

	CIOCPBuffer *pFreeBuffer = m_pFreeBufferList;
	CIOCPBuffer *pNextBuffer;
	while(pFreeBuffer != NULL)
	{
		pNextBuffer = pFreeBuffer->pNext;
		if(!::HeapFree(::GetProcessHeap(), 0, pFreeBuffer))
		{
#ifdef _DEBUG
			::OutputDebugString("  FreeBuffers�ͷ��ڴ������");
#endif // _DEBUG
			break;
		}
		pFreeBuffer = pNextBuffer;
	}
	m_pFreeBufferList = NULL;
	m_nFreeBufferCount = 0;

	::LeaveCriticalSection(&m_FreeBufferListLock);
}

void CIOCPServer::FreeContexts()
{
	// ����m_pFreeContextList�����б����ͷŻ��������ڴ�
	::EnterCriticalSection(&m_FreeContextListLock);

	CIOCPContext *pFreeContext = m_pFreeContextList;
	CIOCPContext *pNextContext;
	while(pFreeContext != NULL)
	{
		pNextContext = pFreeContext->pNext;

		::DeleteCriticalSection(&pFreeContext->Lock);
		if(!::HeapFree(::GetProcessHeap(), 0, pFreeContext))
		{
#ifdef _DEBUG
			::OutputDebugString("  FreeBuffers�ͷ��ڴ������");
#endif // _DEBUG
			break;
		}
		pFreeContext = pNextContext;
	}
	m_pFreeContextList = NULL;
	m_nFreeContextCount = 0;

	::LeaveCriticalSection(&m_FreeContextListLock);
}


BOOL CIOCPServer::AddAConnection(CIOCPContext *pContext)
{
	// ��ͻ������б�����һ��CIOCPContext����

	::EnterCriticalSection(&m_ConnectionListLock);
	if(m_nCurrentConnection <= m_nMaxConnections)
	{
		// ���ӵ���ͷ
		pContext->pNext = m_pConnectionList;
		m_pConnectionList = pContext;
		// ���¼���
		m_nCurrentConnection ++;

		::LeaveCriticalSection(&m_ConnectionListLock);
		return TRUE;
	}
	::LeaveCriticalSection(&m_ConnectionListLock);

	return FALSE;
}

void CIOCPServer::CloseAConnection(CIOCPContext *pContext)
{
	// ���ȴ��б����Ƴ�Ҫ�رյ�����
	::EnterCriticalSection(&m_ConnectionListLock);

	CIOCPContext* pTest = m_pConnectionList;
	if(pTest == pContext)
	{
		m_pConnectionList =  pTest->pNext; // [2009.8.9 mod Lostyears][old: m_pConnectionList =  pContext->pNext]
		m_nCurrentConnection --;
	}
	else
	{
		while(pTest != NULL && pTest->pNext !=  pContext)
			pTest = pTest->pNext;
		if(pTest != NULL)
		{
			pTest->pNext =  pContext->pNext;
			m_nCurrentConnection --;
		}
	}

	::LeaveCriticalSection(&m_ConnectionListLock);

	// Ȼ��رտͻ��׽���
	::EnterCriticalSection(&pContext->Lock);

	if(pContext->s != INVALID_SOCKET)
	{
		::closesocket(pContext->s);	
		pContext->s = INVALID_SOCKET;
	}
	pContext->bClosing = TRUE;

	::LeaveCriticalSection(&pContext->Lock);
}

void CIOCPServer::CloseAllConnections()
{
	// �������������б����ر����еĿͻ��׽���

	::EnterCriticalSection(&m_ConnectionListLock);

	CIOCPContext *pContext = m_pConnectionList;
	while(pContext != NULL)
	{	
		::EnterCriticalSection(&pContext->Lock);

		if(pContext->s != INVALID_SOCKET)
		{
			::closesocket(pContext->s);
			pContext->s = INVALID_SOCKET;
		}

		pContext->bClosing = TRUE;

		::LeaveCriticalSection(&pContext->Lock);	

		pContext = pContext->pNext;
	}

	m_pConnectionList = NULL;
	m_nCurrentConnection = 0;

	::LeaveCriticalSection(&m_ConnectionListLock);
}


BOOL CIOCPServer::InsertPendingAccept(CIOCPBuffer *pBuffer)
{
	// ��һ��I/O������������뵽m_pPendingAccepts����

	::EnterCriticalSection(&m_PendingAcceptsLock);

	if(m_pPendingAccepts == NULL)
		m_pPendingAccepts = pBuffer;
	else
	{
		pBuffer->pNext = m_pPendingAccepts;
		m_pPendingAccepts = pBuffer;
	}
	m_nPendingAcceptCount ++;

	::LeaveCriticalSection(&m_PendingAcceptsLock);

	return TRUE;
}

BOOL CIOCPServer::RemovePendingAccept(CIOCPBuffer *pBuffer)
{
	BOOL bResult = FALSE;

	// ����m_pPendingAccepts���������Ƴ�pBuffer��ָ��Ļ���������
	::EnterCriticalSection(&m_PendingAcceptsLock);

	CIOCPBuffer *pTest = m_pPendingAccepts;
	if(pTest == pBuffer)	// ����Ǳ�ͷԪ��
	{
		m_pPendingAccepts = pTest->pNext; // [2009.8.9 mod Lostyears][old: m_pPendingAccepts = pBuffer->pNext]
		bResult = TRUE;
	}
	else					// ���Ǳ�ͷԪ�صĻ�����Ҫ�����������������
	{
		while(pTest != NULL && pTest->pNext != pBuffer)
			pTest = pTest->pNext;
		if(pTest != NULL)
		{
			pTest->pNext = pBuffer->pNext;
			bResult = TRUE;
		}
	}
	// ���¼���
	if(bResult)
		m_nPendingAcceptCount --;

	::LeaveCriticalSection(&m_PendingAcceptsLock);

	return  bResult;
}


CIOCPBuffer *CIOCPServer::GetNextReadBuffer(CIOCPContext *pContext, CIOCPBuffer *pBuffer)
{
	if(pBuffer != NULL)
	{
		// �����Ҫ������һ�����к���ȣ������黺����
		if(pBuffer->nSequenceNumber == pContext->nCurrentReadSequence)
		{
			return pBuffer;
		}

		// �������ȣ���˵��û�а�˳��������ݣ�����黺�������浽���ӵ�pOutOfOrderReads�б���

		// �б��еĻ������ǰ��������кŴ�С�����˳�����е�

		pBuffer->pNext = NULL;

		CIOCPBuffer *ptr = pContext->pOutOfOrderReads;
		CIOCPBuffer *pPre = NULL;
		while(ptr != NULL)
		{
			if(pBuffer->nSequenceNumber < ptr->nSequenceNumber)
				break;

			pPre = ptr;
			ptr = ptr->pNext;
		}

		if(pPre == NULL) // Ӧ�ò��뵽��ͷ
		{
			pBuffer->pNext = pContext->pOutOfOrderReads;
			pContext->pOutOfOrderReads = pBuffer;
		}
		else			// Ӧ�ò��뵽�����м�
		{
			pBuffer->pNext = pPre->pNext;
			pPre->pNext = pBuffer; // [2009.8.9 mod Lostyears][old: pPre->pNext = pBuffer->pNext]
		}
	}

	// ����ͷԪ�ص����кţ������Ҫ�������к�һ�£��ͽ����ӱ����Ƴ������ظ��û�
	CIOCPBuffer *ptr = pContext->pOutOfOrderReads;
	if(ptr != NULL && (ptr->nSequenceNumber == pContext->nCurrentReadSequence))
	{
		pContext->pOutOfOrderReads = ptr->pNext;
		return ptr;
	}
	return NULL;
}


BOOL CIOCPServer::PostAccept(CIOCPBuffer *pBuffer)	// �ڼ����׽�����Ͷ��Accept����
{
	// ����I/O����
	pBuffer->nOperation = OP_ACCEPT;

	// Ͷ�ݴ��ص�I/O  
	DWORD dwBytes;
	pBuffer->sClient = ::WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	BOOL b = m_lpfnAcceptEx(m_sListen, 
		pBuffer->sClient,
		pBuffer->buff, 
		pBuffer->nLen - ((sizeof(sockaddr_in) + 16) * 2), // [2010.5.16 bak Lostyears]�������Ϊ0, ��ʾ���ȴ��������ݶ�֪ͨ, ��������Ϊ0, ��GetAcceptExSockaddrs�����е���Ӧ����Ҳ����Ӧ��
		sizeof(sockaddr_in) + 16, 
		sizeof(sockaddr_in) + 16, 
		&dwBytes, 
		&pBuffer->ol);
	if(!b && ::WSAGetLastError() != WSA_IO_PENDING)
	{
		return FALSE;
	}
	return TRUE;
}

BOOL CIOCPServer::PostRecv(CIOCPContext *pContext, CIOCPBuffer *pBuffer)
{
	// ����I/O����
	pBuffer->nOperation = OP_READ;	

	::EnterCriticalSection(&pContext->Lock);

	// �������к�
	pBuffer->nSequenceNumber = pContext->nReadSequence;

	// Ͷ�ݴ��ص�I/O
	DWORD dwBytes;
	DWORD dwFlags = 0;
	WSABUF buf;
	buf.buf = pBuffer->buff;
	buf.len = pBuffer->nLen;
	if(::WSARecv(pContext->s, &buf, 1, &dwBytes, &dwFlags, &pBuffer->ol, NULL) != NO_ERROR)
	{
		if(::WSAGetLastError() != WSA_IO_PENDING)
		{
			::LeaveCriticalSection(&pContext->Lock);
			return FALSE;
		}
	}

	// �����׽����ϵ��ص�I/O�����Ͷ����кż���

	pContext->nOutstandingRecv ++;
	pContext->nReadSequence ++;

	::LeaveCriticalSection(&pContext->Lock);

	return TRUE;
}

BOOL CIOCPServer::PostSend(CIOCPContext *pContext, CIOCPBuffer *pBuffer)
{	
	// ����Ͷ�ݵķ��͵���������ֹ�û����������ݶ������գ����·������׳��������Ͳ���
	if(pContext->nOutstandingSend > m_nMaxSends)
		return FALSE;

	// ����I/O���ͣ������׽����ϵ��ص�I/O����
	pBuffer->nOperation = OP_WRITE;

	// Ͷ�ݴ��ص�I/O
	DWORD dwBytes;
	DWORD dwFlags = 0;
	WSABUF buf;
	buf.buf = pBuffer->buff;
	buf.len = pBuffer->nLen;
	if(::WSASend(pContext->s, 
		&buf, 1, &dwBytes, dwFlags, &pBuffer->ol, NULL) != NO_ERROR)
	{
		if(::WSAGetLastError() != WSA_IO_PENDING)
			return FALSE;
	}	

	// �����׽����ϵ��ص�I/O����
	::EnterCriticalSection(&pContext->Lock);
	pContext->nOutstandingSend ++;
	::LeaveCriticalSection(&pContext->Lock);

	return TRUE;
}


BOOL CIOCPServer::Start(int nPort, int nMaxConnections, 
						int nMaxFreeBuffers, int nMaxFreeContexts, int nInitialReads)
{
	// �������Ƿ��Ѿ�����
	if(m_bServerStarted)
		return FALSE;

	// �����û�����
	m_nPort = nPort;
	m_nMaxConnections = nMaxConnections;
	m_nMaxFreeBuffers = nMaxFreeBuffers;
	m_nMaxFreeContexts = nMaxFreeContexts;
	m_nInitialReads = nInitialReads;

	// ��ʼ��״̬����
	m_bShutDown = FALSE;
	m_bServerStarted = TRUE;


	// ���������׽��֣��󶨵����ض˿ڣ��������ģʽ
	m_sListen = ::WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	SOCKADDR_IN si;
	si.sin_family = AF_INET;
	si.sin_port = ::ntohs(m_nPort);
	si.sin_addr.S_un.S_addr = INADDR_ANY;
	if(::bind(m_sListen, (sockaddr*)&si, sizeof(si)) == SOCKET_ERROR)
	{
		m_bServerStarted = FALSE;
		return FALSE;
	}
	::listen(m_sListen, 200);

	// ������ɶ˿ڶ���
	m_hCompletion = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);

	// ������չ����AcceptEx
	GUID GuidAcceptEx = WSAID_ACCEPTEX;
	DWORD dwBytes;
	::WSAIoctl(m_sListen, 
		SIO_GET_EXTENSION_FUNCTION_POINTER, 
		&GuidAcceptEx, 
		sizeof(GuidAcceptEx),
		&m_lpfnAcceptEx, 
		sizeof(m_lpfnAcceptEx), 
		&dwBytes, 
		NULL, 
		NULL);

	// ������չ����GetAcceptExSockaddrs
	GUID GuidGetAcceptExSockaddrs = WSAID_GETACCEPTEXSOCKADDRS;
	::WSAIoctl(m_sListen,
		SIO_GET_EXTENSION_FUNCTION_POINTER,
		&GuidGetAcceptExSockaddrs,
		sizeof(GuidGetAcceptExSockaddrs),
		&m_lpfnGetAcceptExSockaddrs,
		sizeof(m_lpfnGetAcceptExSockaddrs),
		&dwBytes,
		NULL,
		NULL
		);


	// �������׽��ֹ�������ɶ˿ڣ�ע�⣬����Ϊ�����ݵ�CompletionKeyΪ0
	::CreateIoCompletionPort((HANDLE)m_sListen, m_hCompletion, (DWORD)0, 0);

	// ע��FD_ACCEPT�¼���
	// ���Ͷ�ݵ�AcceptEx I/O�������̻߳���յ�FD_ACCEPT�����¼���˵��Ӧ��Ͷ�ݸ����AcceptEx I/O
	WSAEventSelect(m_sListen, m_hAcceptEvent, FD_ACCEPT);

	// ���������߳�
	m_hListenThread = ::CreateThread(NULL, 0, _ListenThreadProc, this, 0, NULL);

	return TRUE;
}

void CIOCPServer::Shutdown()
{
	if(!m_bServerStarted)
		return;

	// ֪ͨ�����̣߳�����ֹͣ����
	m_bShutDown = TRUE;
	::SetEvent(m_hAcceptEvent);
	// �ȴ������߳��˳�
	::WaitForSingleObject(m_hListenThread, INFINITE);
	::CloseHandle(m_hListenThread);
	m_hListenThread = NULL;

	m_bServerStarted = FALSE;
}

DWORD WINAPI CIOCPServer::_ListenThreadProc(LPVOID lpParam)
{
	CIOCPServer *pThis = (CIOCPServer*)lpParam;

	// ���ڼ����׽�����Ͷ�ݼ���Accept I/O
	CIOCPBuffer *pBuffer;
	for(int i=0; i<pThis->m_nInitialAccepts; i++)
	{
		pBuffer = pThis->AllocateBuffer(BUFFER_SIZE);
		if(pBuffer == NULL)
			return -1;
		pThis->InsertPendingAccept(pBuffer);
		pThis->PostAccept(pBuffer);
	}

	// �����¼��������飬�Ա����������WSAWaitForMultipleEvents����
	HANDLE hWaitEvents[2 + MAX_THREAD];
	int nEventCount = 0;
	hWaitEvents[nEventCount ++] = pThis->m_hAcceptEvent;
	hWaitEvents[nEventCount ++] = pThis->m_hRepostEvent;

	// ����ָ�������Ĺ����߳�����ɶ˿��ϴ���I/O
	for(int i=0; i<MAX_THREAD; i++)
	{
		hWaitEvents[nEventCount ++] = ::CreateThread(NULL, 0, _WorkerThreadProc, pThis, 0, NULL);
	}

	// �����������ѭ���������¼����������е��¼�
	while(TRUE)
	{
		int nIndex = ::WSAWaitForMultipleEvents(nEventCount, hWaitEvents, FALSE, 60*1000, FALSE);

		// ���ȼ���Ƿ�Ҫֹͣ����
		if(pThis->m_bShutDown || nIndex == WSA_WAIT_FAILED)
		{
			// �ر���������
			pThis->CloseAllConnections();
			::Sleep(0);		// ��I/O�����߳�һ��ִ�еĻ���
			// �رռ����׽���
			::closesocket(pThis->m_sListen);
			pThis->m_sListen = INVALID_SOCKET;
			::Sleep(0);		// ��I/O�����߳�һ��ִ�еĻ���

			// ֪ͨ����I/O�����߳��˳�
			for(int i=2; i<MAX_THREAD + 2; i++)
			{	
				::PostQueuedCompletionStatus(pThis->m_hCompletion, -1, 0, NULL);
			}

			// �ȴ�I/O�����߳��˳�
			::WaitForMultipleObjects(MAX_THREAD, &hWaitEvents[2], TRUE, 5*1000);

			for(int i=2; i<MAX_THREAD + 2; i++)
			{	
				::CloseHandle(hWaitEvents[i]);
			}

			::CloseHandle(pThis->m_hCompletion);

			pThis->FreeBuffers();
			pThis->FreeContexts();
			::ExitThread(0);
		}	

		// 1����ʱ�������δ���ص�AcceptEx I/O�����ӽ����˶೤ʱ��
		if(nIndex == WSA_WAIT_TIMEOUT)
		{
			pBuffer = pThis->m_pPendingAccepts;
			while(pBuffer != NULL)
			{
				int nSeconds;
				int nLen = sizeof(nSeconds);
				// ȡ�����ӽ�����ʱ��
				::getsockopt(pBuffer->sClient, 
					SOL_SOCKET, SO_CONNECT_TIME, (char *)&nSeconds, &nLen);	
				// �������2���ӿͻ��������ͳ�ʼ���ݣ���������ͻ�go away
				if(nSeconds != -1 && nSeconds > 2*60)
				{   
					closesocket(pBuffer->sClient);
					pBuffer->sClient = INVALID_SOCKET;
				}

				pBuffer = pBuffer->pNext;
			}
		}
		else
		{
			nIndex = nIndex - WAIT_OBJECT_0;
			WSANETWORKEVENTS ne;
			int nLimit=0;
			if(nIndex == 0)			// 2��m_hAcceptEvent�¼��������ţ�˵��Ͷ�ݵ�Accept���󲻹�����Ҫ����
			{
				::WSAEnumNetworkEvents(pThis->m_sListen, hWaitEvents[nIndex], &ne);
				if(ne.lNetworkEvents & FD_ACCEPT)
				{
					nLimit = 50;  // ���ӵĸ�����������Ϊ50��
				}
			}
			else if(nIndex == 1)	// 3��m_hRepostEvent�¼��������ţ�˵������I/O���߳̽��ܵ��µĿͻ�
			{
				nLimit = InterlockedExchange(&pThis->m_nRepostCount, 0);
			}
			else if(nIndex > 1)		// I/O�����߳��˳���˵���д��������رշ�����
			{
				pThis->m_bShutDown = TRUE;
				continue;
			}

			// Ͷ��nLimit��AcceptEx I/O����
			int i = 0;
			while(i++ < nLimit && pThis->m_nPendingAcceptCount < pThis->m_nMaxAccepts)
			{
				pBuffer = pThis->AllocateBuffer(BUFFER_SIZE);
				if(pBuffer != NULL)
				{
					pThis->InsertPendingAccept(pBuffer);
					pThis->PostAccept(pBuffer);
				}
			}
		}
	}
	return 0;
}

DWORD WINAPI CIOCPServer::_WorkerThreadProc(LPVOID lpParam)
{
#ifdef _DEBUG
	::OutputDebugString("	WorkerThread ����... \n");
#endif // _DEBUG

	CIOCPServer *pThis = (CIOCPServer*)lpParam;

	CIOCPBuffer *pBuffer;
	DWORD dwKey;
	DWORD dwTrans;
	LPOVERLAPPED lpol;
	while(TRUE)
	{
		// �ڹ���������ɶ˿ڵ������׽����ϵȴ�I/O���
		BOOL bOK = ::GetQueuedCompletionStatus(pThis->m_hCompletion, 
			&dwTrans, (LPDWORD)&dwKey, (LPOVERLAPPED*)&lpol, WSA_INFINITE);

		if(dwTrans == -1) // �û�֪ͨ�˳�
		{
#ifdef _DEBUG
			::OutputDebugString("	WorkerThread �˳� \n");
#endif // _DEBUG
			::ExitThread(0);
		}


		pBuffer = CONTAINING_RECORD(lpol, CIOCPBuffer, ol); // [2009.8.9 bak Lostyears][lpol��ΪCIOCPBuffer��ol��Ա�������ַȡCIOCPBufferʵ���׵�ַ]
		int nError = NO_ERROR;
		if(!bOK)						// �ڴ��׽������д�����
		{
			SOCKET s;
			if(pBuffer->nOperation == OP_ACCEPT)
			{
				s = pThis->m_sListen;
			}
			else
			{
				if(dwKey == 0)
					break;
				s = ((CIOCPContext*)dwKey)->s;
			}
			DWORD dwFlags = 0;
			if(!::WSAGetOverlappedResult(s, &pBuffer->ol, &dwTrans, FALSE, &dwFlags))
			{
				nError = ::WSAGetLastError();
			}
		}
		pThis->HandleIO(dwKey, pBuffer, dwTrans, nError);
	}

#ifdef _DEBUG
	::OutputDebugString("	WorkerThread �˳� \n");
#endif // _DEBUG
	return 0;
}


void CIOCPServer::HandleIO(DWORD dwKey, CIOCPBuffer *pBuffer, DWORD dwTrans, int nError)
{
	CIOCPContext *pContext = (CIOCPContext *)dwKey;

#ifdef _DEBUG
	::printf("	HandleIO... \n");
#endif // _DEBUG

	// 1�����ȼ����׽����ϵ�δ��I/O����
	if(pContext != NULL)
	{
		::EnterCriticalSection(&pContext->Lock);

		if(pBuffer->nOperation == OP_READ)
			pContext->nOutstandingRecv --;
		else if(pBuffer->nOperation == OP_WRITE)
			pContext->nOutstandingSend --;

		::LeaveCriticalSection(&pContext->Lock);

		// 2������׽����Ƿ��Ѿ������ǹر� [2009.8.9 bak Lostyears][����ر����ͷ�ʣ�µ�δ��IO]
		if(pContext->bClosing) 
		{
#ifdef _DEBUG
			::OutputDebugString("	��鵽�׽����Ѿ������ǹر� \n");
#endif // _DEBUG
			if(pContext->nOutstandingRecv == 0 && pContext->nOutstandingSend == 0)
			{		
				ReleaseContext(pContext);
			}
			// �ͷ��ѹر��׽��ֵ�δ��I/O
			ReleaseBuffer(pBuffer);	
			return;
		}
	}
	else
	{
		RemovePendingAccept(pBuffer); // [2009.8.9 bak Lostyears][sListen������iocp, ����ʱdwKeyΪ0, ���Ե��������ӷ�������ʱ��ִ�е���]
	}

	// 3������׽����Ϸ����Ĵ�������еĻ���֪ͨ�û���Ȼ��ر��׽���
	if(nError != NO_ERROR)
	{
		if(pBuffer->nOperation != OP_ACCEPT)
		{
			NotifyConnectionError(pContext, pBuffer, nError);
			CloseAConnection(pContext);
			if(pContext->nOutstandingRecv == 0 && pContext->nOutstandingSend == 0)
			{		
				ReleaseContext(pContext);
			}
#ifdef _DEBUG
			::OutputDebugString("	��鵽�ͻ��׽����Ϸ������� \n");
#endif // _DEBUG
		}
		else // �ڼ����׽����Ϸ�������Ҳ���Ǽ����׽��ִ����Ŀͻ�������
		{
			// �ͻ��˳������ͷ�I/O������
			if(pBuffer->sClient != INVALID_SOCKET)
			{
				::closesocket(pBuffer->sClient);
				pBuffer->sClient = INVALID_SOCKET;
			}
#ifdef _DEBUG
			::OutputDebugString("	��鵽�����׽����Ϸ������� \n");
#endif // _DEBUG
		}

		ReleaseBuffer(pBuffer);
		return;
	}


	// ��ʼ����
	if(pBuffer->nOperation == OP_ACCEPT)
	{
		if(dwTrans == 0) // [2010.5.16 bak Lostyears]���AcceptEx�����ݽ��ջ�������Ϊ0, һ�����Ͼͻ�ִ�е���
		{
#ifdef _DEBUG
			::OutputDebugString("	�����׽����Ͽͻ��˹ر� \n");
#endif // _DEBUG

			if(pBuffer->sClient != INVALID_SOCKET)
			{
				::closesocket(pBuffer->sClient);
				pBuffer->sClient = INVALID_SOCKET;
			}
		}
		else
		{
			// Ϊ�½��ܵ���������ͻ������Ķ���
			CIOCPContext *pClient = AllocateContext(pBuffer->sClient);
			if(pClient != NULL)
			{
				if(AddAConnection(pClient))
				{	
					// ȡ�ÿͻ���ַ
					int nLocalLen, nRmoteLen;
					LPSOCKADDR pLocalAddr, pRemoteAddr;
					m_lpfnGetAcceptExSockaddrs(
						pBuffer->buff,
						pBuffer->nLen - ((sizeof(sockaddr_in) + 16) * 2), // [2010.5.16 bak Lostyears]��AcceptEx��Ӧ������Ӧ
						sizeof(sockaddr_in) + 16,
						sizeof(sockaddr_in) + 16,
						(SOCKADDR **)&pLocalAddr,
						&nLocalLen,
						(SOCKADDR **)&pRemoteAddr,
						&nRmoteLen);
					memcpy(&pClient->addrLocal, pLocalAddr, nLocalLen);
					memcpy(&pClient->addrRemote, pRemoteAddr, nRmoteLen);

					// [2010.1.15 add Lostyears][����KeepAlive����]
					BOOL bKeepAlive = TRUE;
					int nRet = ::setsockopt(pClient->s, SOL_SOCKET, SO_KEEPALIVE, (char*)&bKeepAlive, sizeof(bKeepAlive));
					if (nRet == SOCKET_ERROR)
					{
						CloseAConnection(pClient);
					}
					else
					{
						// ����KeepAlive����
						tcp_keepalive alive_in	= {0};
						tcp_keepalive alive_out	= {0};
						alive_in.keepalivetime		= 5000;	// ��ʼ�״�KeepAlive̽��ǰ��TCP����ʱ��
						alive_in.keepaliveinterval	= 1000;	// ����KeepAlive̽����ʱ����
						alive_in.onoff	= TRUE;
						unsigned long ulBytesReturn	= 0;
						nRet = ::WSAIoctl(pClient->s, SIO_KEEPALIVE_VALS, &alive_in, sizeof(alive_in),
							&alive_out, sizeof(alive_out), &ulBytesReturn, NULL, NULL);
						if (nRet == SOCKET_ERROR)
						{
							CloseAConnection(pClient);
						}
						else
						{
							// ���������ӵ���ɶ˿ڶ���
							::CreateIoCompletionPort((HANDLE)pClient->s, m_hCompletion, (DWORD)pClient, 2);

							// ֪ͨ�û�
							pBuffer->nLen = dwTrans;
							OnConnectionEstablished(pClient, pBuffer);

							// ��������Ͷ�ݼ���Read������Щ�ռ����׽��ֹرջ����ʱ�ͷ�
							for(int i=0; i<m_nInitialReads; i++) // [2009.8.21 mod Lostyears][������ֵ��Ϊm_nInitialReads]
							{
								CIOCPBuffer *p = AllocateBuffer(BUFFER_SIZE);
								if(p != NULL)
								{
									if(!PostRecv(pClient, p))
									{
										CloseAConnection(pClient);
										break;
									}
								}
							}
						}
					}

					//// ���������ӵ���ɶ˿ڶ���
					//::CreateIoCompletionPort((HANDLE)pClient->s, m_hCompletion, (DWORD)pClient, 0);
					//
					//// ֪ͨ�û�
					//pBuffer->nLen = dwTrans;
					//OnConnectionEstablished(pClient, pBuffer);
					//
					//// ��������Ͷ�ݼ���Read������Щ�ռ����׽��ֹرջ����ʱ�ͷ�
					//for(int i=0; i<m_nInitialReads; i++) // [2009.8.22 mod Lostyears][old: i<5]
					//{
					//	CIOCPBuffer *p = AllocateBuffer(BUFFER_SIZE);
					//	if(p != NULL)
					//	{
					//		if(!PostRecv(pClient, p))
					//		{
					//			CloseAConnection(pClient);
					//			break;
					//		}
					//	}
					//}
				}
				else	// ���������������ر�����
				{
					CloseAConnection(pClient);
					ReleaseContext(pClient);
				}
			}
			else
			{
				// ��Դ���㣬�ر���ͻ������Ӽ���
				::closesocket(pBuffer->sClient);
				pBuffer->sClient = INVALID_SOCKET;
			}
		}

		// Accept������ɣ��ͷ�I/O������
		ReleaseBuffer(pBuffer);	

		// ֪ͨ�����̼߳�����Ͷ��һ��Accept����
		::InterlockedIncrement(&m_nRepostCount);
		::SetEvent(m_hRepostEvent);
	}
	else if(pBuffer->nOperation == OP_READ)
	{
		if(dwTrans == 0)	// �Է��ر��׽���
		{
			// ��֪ͨ�û�
			pBuffer->nLen = 0;
			NotifyConnectionClosing(pContext, pBuffer);	

			// �ٹر�����
			CloseAConnection(pContext);
			// �ͷſͻ������ĺͻ���������
			if(pContext->nOutstandingRecv == 0 && pContext->nOutstandingSend == 0)
			{		
				ReleaseContext(pContext);
			}
			ReleaseBuffer(pBuffer);	
		}
		else
		{
			pBuffer->nLen = dwTrans;
			// ����I/OͶ�ݵ�˳���ȡ���յ�������
			CIOCPBuffer *p = GetNextReadBuffer(pContext, pBuffer);
			while(p != NULL)
			{
				// ֪ͨ�û�
				OnReadCompleted(pContext, p);
				// ����Ҫ�������кŵ�ֵ
				::InterlockedIncrement((LONG*)&pContext->nCurrentReadSequence);
				// �ͷ��������ɵ�I/O
				ReleaseBuffer(p);
				p = GetNextReadBuffer(pContext, NULL);
			}

			// ����Ͷ��һ���µĽ�������
			pBuffer = AllocateBuffer(BUFFER_SIZE);
			if(pBuffer == NULL || !PostRecv(pContext, pBuffer))
			{
				CloseAConnection(pContext);
			}
		}
	}
	else if(pBuffer->nOperation == OP_WRITE)
	{

		if(dwTrans == 0)	// �Է��ر��׽���
		{
			// ��֪ͨ�û�
			pBuffer->nLen = 0;
			NotifyConnectionClosing(pContext, pBuffer);	

			// �ٹر�����
			CloseAConnection(pContext);

			// �ͷſͻ������ĺͻ���������
			if(pContext->nOutstandingRecv == 0 && pContext->nOutstandingSend == 0)
			{		
				ReleaseContext(pContext);
			}
			ReleaseBuffer(pBuffer);	
		}
		else
		{
			// д������ɣ�֪ͨ�û�
			pBuffer->nLen = dwTrans;
			OnWriteCompleted(pContext, pBuffer);
			// �ͷ�SendText��������Ļ�����
			ReleaseBuffer(pBuffer);
		}
	}
}

// ���׼��ֹرջ����ʱ֪ͨ
void CIOCPServer::NotifyConnectionClosing(CIOCPContext *pContext, CIOCPBuffer *pBuffer)
{
	::EnterCriticalSection(&m_CloseOrErrLock);
	if (!pContext->bNotifyCloseOrError)
	{
		pContext->bNotifyCloseOrError = true;
		OnConnectionClosing(pContext, pBuffer);
	}
	::LeaveCriticalSection(&m_CloseOrErrLock);
}

void CIOCPServer::NotifyConnectionError(CIOCPContext *pContext, CIOCPBuffer *pBuffer, int nError)
{
	::EnterCriticalSection(&m_CloseOrErrLock);
	if (!pContext->bNotifyCloseOrError)
	{
		pContext->bNotifyCloseOrError = true;
		OnConnectionError(pContext, pBuffer, nError);
	}
	::LeaveCriticalSection(&m_CloseOrErrLock);
}



BOOL CIOCPServer::SendText(CIOCPContext *pContext, char *pszText, int nLen)
{
	CIOCPBuffer *pBuffer = AllocateBuffer(nLen);
	if(pBuffer != NULL)
	{
		memcpy(pBuffer->buff, pszText, nLen);
		return PostSend(pContext, pBuffer);
	}
	return FALSE;
}


void CIOCPServer::OnConnectionEstablished(CIOCPContext *pContext, CIOCPBuffer *pBuffer)
{
}

void CIOCPServer::OnConnectionClosing(CIOCPContext *pContext, CIOCPBuffer *pBuffer)
{
}


void CIOCPServer::OnReadCompleted(CIOCPContext *pContext, CIOCPBuffer *pBuffer)
{
}

void CIOCPServer::OnWriteCompleted(CIOCPContext *pContext, CIOCPBuffer *pBuffer)
{
}

void CIOCPServer::OnConnectionError(CIOCPContext *pContext, CIOCPBuffer *pBuffer, int nError)
{
}
////////////////////////////////////////////////
// iocpserver.cpp�ļ�


// CIOCPServer��Ĳ��Գ���

//#include "iocp.h"
#include <stdio.h>
#include <windows.h>

class CMyServer : public CIOCPServer
{
public:

	void OnConnectionEstablished(CIOCPContext *pContext, CIOCPBuffer *pBuffer)
	{
		printf("���յ�һ���µ�����(%d): %s\n", 
			GetCurrentConnection(), ::inet_ntoa(pContext->addrRemote.sin_addr));
		printf("���ܵ�һ�����ݰ�, ���СΪ: %d�ֽ�\n", pBuffer->nLen);

		SendText(pContext, pBuffer->buff, pBuffer->nLen);
	}

	void OnConnectionClosing(CIOCPContext *pContext, CIOCPBuffer *pBuffer)
	{
		printf("һ�����ӹر�\n");
	}

	void OnConnectionError(CIOCPContext *pContext, CIOCPBuffer *pBuffer, int nError)
	{
		printf("һ�����ӷ�������: %d\n", nError);
	}

	void OnReadCompleted(CIOCPContext *pContext, CIOCPBuffer *pBuffer)
	{
		printf("���ܵ�һ�����ݰ�, ���СΪ: %d�ֽ�\n", pBuffer->nLen);
		SendText(pContext, pBuffer->buff, pBuffer->nLen);
	}

	void OnWriteCompleted(CIOCPContext *pContext, CIOCPBuffer *pBuffer)
	{
		printf("һ�����ݰ����ͳɹ�, ���СΪ: %d�ֽ�\n ", pBuffer->nLen);
	}
};

void main()
{
	CMyServer *pServer = new CMyServer;

	// ��������
	if(pServer->Start())
	{
		printf("�����������ɹ�...\n");
	}
	else
	{
		printf("����������ʧ��!\n");
		return;
	}

	// �����¼�������ServerShutdown�����ܹ��ر��Լ�
	HANDLE hEvent = ::CreateEvent(NULL, FALSE, FALSE, "ShutdownEvent");
	::WaitForSingleObject(hEvent, INFINITE);
	::CloseHandle(hEvent);

	// �رշ���
	pServer->Shutdown();
	delete pServer;

	printf("�������ر�\n ");

}