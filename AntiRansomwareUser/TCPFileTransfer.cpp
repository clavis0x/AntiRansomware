
#include "stdafx.h"
#include "TCPFileTransfer.h"

#if _DEBUG
#define DEBUG_VIEW_MY(str) OutputDebugString(str)
#else
#define DEBUG_VIEW_MY(str) ( (void) 0 )
#endif

CFileTransfer::CFileTransfer()
{
	m_hSocket = NULL;
	m_isInitSock = false;
	m_isConnected = false;

	// Init Socket
	InitSock();
}

bool CFileTransfer::InitSock()
{
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		DEBUG_VIEW_MY("WSAStartup() error!");
		return false;
	}

	m_hSocket = socket(PF_INET, SOCK_STREAM, 0);
	if (m_hSocket == INVALID_SOCKET) {
		DEBUG_VIEW_MY("socket() error");
		return false;
	}

	m_isInitSock = true;

	return true;
}

bool CFileTransfer::ConnectToServer(char *strIP, char *strPort)
{
	// Init Socket
	if (m_isInitSock == false) {
		if (InitSock() == false)
			return false;
	}

	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_family = AF_INET;
	servAdr.sin_addr.s_addr = inet_addr(strIP);
	servAdr.sin_port = htons(atoi(strPort));

	if (connect(m_hSocket, (SOCKADDR*)&servAdr, sizeof(servAdr)) == SOCKET_ERROR) {
		DEBUG_VIEW_MY("connect() error!");
		return false;
	}
	else {
		m_isConnected = true;
		DEBUG_VIEW_MY("Connected!");
	}

	return true;
}

bool CFileTransfer::SendFile(char *strHeader, char *strFilePath)
{
	FILE* fpFile;
	long file_size;
	int nTotalSendCount;
	int nTotalSentBytes = 0;
	int nSentBytes = 0;
	int nBufferNum = 0;
	char buf[FILE_BUF_SIZE] = { 0 };
	char szTemp[1024];
	int nCount = 0;

	fpFile = fopen(strFilePath, "rb");
	if (fpFile == NULL)
		return false;

	fseek(fpFile, 0, SEEK_END);
	file_size = ftell(fpFile);
	nTotalSendCount = file_size / sizeof(buf) + 1;
	fseek(fpFile, 0, SEEK_SET);
	sprintf(szTemp, "%d", nTotalSendCount);
	DEBUG_VIEW_MY(szTemp);

	_snprintf(buf, sizeof(buf), "%s %d", strHeader, file_size);
	nSentBytes = send(m_hSocket, buf, sizeof(buf), 0);

	while ((nSentBytes = fread(buf, sizeof(char), sizeof(buf), fpFile))>0) {
		send(m_hSocket, buf, nSentBytes, 0);
		nBufferNum++;
		nTotalSentBytes += nSentBytes;
		sprintf(szTemp, "In progress: %d/%dByte(s) [%d%%]\n", nTotalSentBytes, file_size, ((nBufferNum * 100) / nTotalSendCount));
		DEBUG_VIEW_MY(szTemp);
	}

	fclose(fpFile);
	return true;
}

bool CFileTransfer::RecvFile(char *strHeader, char *strFilePath)
{
	FILE* fpFile;
	int nSentBytes = 0;
	int nRecvBytes = 0;
	int nTotalRecvBytes = 0;
	char buf[FILE_BUF_SIZE] = { 0 };
	char szTemp[1024];
	int nFileSize = 0;

	fpFile = fopen(strFilePath, "wb");
	if (fpFile == NULL)
		return false;

	_snprintf(buf, sizeof(buf), "UpdateDB");
	nSentBytes = send(m_hSocket, buf, sizeof(buf), 0);

	// Get File Size
	for(int i=0; i<10; i++){
		nRecvBytes = recv(m_hSocket, buf+i, 1, 0);
		if (*(buf + i) == '\0')
			break;
	}
	nFileSize = atoi(buf);
	ZeroMemory(buf, FILE_BUF_SIZE);

	nTotalRecvBytes = 0;
	while (nFileSize > nTotalRecvBytes) {
		nRecvBytes = recv(m_hSocket, buf, FILE_BUF_SIZE, 0);
		fwrite(buf, sizeof(char), nRecvBytes, fpFile);
		nTotalRecvBytes += nRecvBytes;
		sprintf(szTemp, "%d, %d\n", nTotalRecvBytes, nRecvBytes);
		DEBUG_VIEW_MY(szTemp);
	}

	fclose(fpFile);
	return true;
}

bool CFileTransfer::Disconnect()
{
	closesocket(m_hSocket);
	WSACleanup();

	m_isInitSock = false;
	m_isConnected = false;

	return true;
}