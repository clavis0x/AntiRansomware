
#include "stdafx.h"
#include "UtilityFilesys.h"


BOOL splitDevicePath(const wchar_t * path,
	wchar_t * devicename, size_t lenDevicename,
	wchar_t * dir, size_t lenDir,
	wchar_t * fname, size_t lenFname,
	wchar_t * ext, size_t lenExt)
{
	size_t pathLength;
	const wchar_t * pPos = path;

	StringCchLengthW(path, MAX_PATH, &pathLength);
	if (path == NULL || pathLength <= 8)
		return FALSE;

	//path EX: \Device\HarddiskVolume2\MyTest\123.txt 
	if (path == NULL)
		return FALSE;


	//STEP 1: 장치명 분리.. \Device\HarddiskVolume2
	//첫번째 \Device\를 넘기위해 pPos +8 해줌
	pPos = wcschr(pPos + 8, L'\\');
	if (pPos == NULL) {
		return FALSE;
	}
	if (devicename != NULL && lenDevicename > 0)
		wcsncpy_s(devicename, lenDevicename, path, pPos - path);

	//STEP 2: 나머지 경로분리.. \MyTest\123.txt
	_wsplitpath_s(pPos, NULL, 0, dir, lenDir, fname, lenFname, ext, lenExt);

	return TRUE;

}

//\Device\HarddiskVolume2 --> c: 로 변환.. 
BOOL GetDeviceNameToDriveLetter(const wchar_t * pDevicePath, wchar_t * bufDriveLetter, size_t bufLen)
{
	WCHAR szDriverString[MAX_PATH] = { 0, };
	WCHAR swTempDeviceName[MAX_PATH];
	WCHAR szDriveLetter[8];
	LPCWSTR pPos = NULL;

	if (pDevicePath == NULL || bufDriveLetter == NULL || bufLen <= 0)
		return FALSE;

	// 전체 드라이브 문자열을 구함. Ex) "A:\ C:\ D:\ "
	GetLogicalDriveStringsW(_countof(szDriverString), szDriverString);
	pPos = szDriverString;
	while (*pPos != TEXT('\0'))
	{
		StringCchPrintfW(szDriveLetter, _countof(szDriveLetter), L"%c:", *pPos);
		if (QueryDosDeviceW(szDriveLetter, swTempDeviceName, _countof(swTempDeviceName)) > 0)
		{
			//비교.. 
			if (wcsncmp(swTempDeviceName, pDevicePath, _countof(swTempDeviceName)) == 0)
			{
				StringCchCopyW(bufDriveLetter, bufLen, szDriveLetter);
				break;
			}
		}
		// 다음 디스크 (4 문자)
		pPos = &pPos[4];
	}

	return TRUE;
}

BOOL ConvertDevicePathToDrivePath(const wchar_t * pDevicePath, wchar_t *bufPath, size_t bufPathLen, PATH_INFO_EX *pPathInfo)
{
	wchar_t szDeviceName[128] = { 0 };
	wchar_t szDirPath[MAX_PATH] = { 0 };
	wchar_t szFileName[MAX_PATH] = { 0 };
	wchar_t szExt[32] = { 0 };
	wchar_t szDriveLetter[8] = { 0 };

	if (pDevicePath == NULL || bufPath == NULL || bufPathLen <= 0)
		return FALSE;

	//STEP 1 : device 경로를 드라이브 경로로 변경한다. 
	//ex : \Device\HarddiskVolume2\MyTest\123.txt --> C:\MyTest\123.txt

	//STEP 1 : 디바이스 경로 산산히 분해.. 
	if (splitDevicePath(pDevicePath, szDeviceName, 128, szDirPath, MAX_PATH, szFileName, MAX_PATH, szExt, 32) == FALSE)
		return FALSE;

	//SETP 2 : 디바이스 명을 드라이브 레터로 변경.. 
	//ex : \Device\HarddiskVolume2\ --> C:\

	// backup용 (임시)
	wcscpy(pPathInfo->szDeviceName, szDeviceName);
	
	if (GetDeviceNameToDriveLetter(szDeviceName, szDriveLetter, 8) == FALSE)
		return FALSE;

	// Reneme용 Drive Letter 저장 (임시)
	memcpy(pPathInfo->szLastDriveLetter, szDriveLetter, 8);

	// 가끔 알수없는 경로 때문에.. ex. DR
	if (wcslen(szDriveLetter) == 0 || wcslen(szDirPath) == 0)
		return FALSE;

	//STEP 3 : 경로 재조립.. 
	StringCchPrintfW(bufPath, bufPathLen, L"%s%s%s%s", szDriveLetter, szDirPath, szFileName, szExt);

	return TRUE;
}

void CreateFolder(CString p_strTargetRoot)
{
	/* 폴더가 있는지 조사하고 없으면 생성한다 */
	if (_waccess((LPWSTR)(LPCTSTR)p_strTargetRoot, 0) != 0) {
		BOOL l_bExtractSucc = TRUE;

		int j = 0;
		CString l_strDirPath = _T("");
		CString l_strNowPath = _T("");
		while (l_bExtractSucc)
		{
			/* 상위 폴더부터 생성해 나간다 */
			l_bExtractSucc = AfxExtractSubString(l_strNowPath, p_strTargetRoot, j, '\\');
			l_strDirPath += l_strNowPath + _T("\\");
			if (l_bExtractSucc && _waccess((LPWSTR)(LPCTSTR)l_strDirPath, 0) != 0) {
				CreateDirectory(l_strDirPath, NULL);
			}
			j++;
		}
	}
}

bool RefreshDesktopDirectory()
{
	char szPath[1024];
	LPITEMIDLIST pItemIDList;
	SHGetSpecialFolderLocation(GetDesktopWindow(), CSIDL_DESKTOP, &pItemIDList);
	SHGetPathFromIDList(pItemIDList, szPath);

	// 바탕화면 새로고침
	SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH | SHCNF_FLUSHNOWAIT, szPath, NULL);

	return true;
}