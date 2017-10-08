#pragma once
#include <Windows.h>
#include <winsock2.h>

#define FILE_BUF_SIZE 1024

class CFileTransfer
{
private:
	WSADATA wsaData;
	SOCKET m_hSocket;
	SOCKADDR_IN servAdr;

	bool m_isInitSock;
	bool m_isConnected;

	bool InitSock();

public:
	CFileTransfer();
	bool ConnectToServer(char *strIP, char *strPort);
	bool SendFile(char *strHeader, char *strFilePath);
	bool RecvFile(char *strHeader, char *strFilePath);
	bool Disconnect();
};