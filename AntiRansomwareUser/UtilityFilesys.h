#pragma once

typedef struct sPathInfoEx { // 경로 세부 정보
	wchar_t szLastDriveLetter[8];	// Rename용 Driver Letter (임시)
	wchar_t szDeviceName[128];	// Backup용 Device Name (임시)
} PATH_INFO_EX;

BOOL splitDevicePath(const wchar_t * path,
	wchar_t * devicename, size_t lenDevicename,
	wchar_t * dir, size_t lenDir,
	wchar_t * fname, size_t lenFname,
	wchar_t * ext, size_t lenExt);
BOOL GetDeviceNameToDriveLetter(const wchar_t * pDevicePath,wchar_t * bufDriveLetter, size_t bufLen);
BOOL ConvertDevicePathToDrivePath(const wchar_t * pDevicePath, wchar_t *bufPath, size_t bufPathLen, PATH_INFO_EX *pPathInfo);
void CreateFolder(CString p_strTargetRoot);
bool RefreshDesktopDirectory();
bool GenerateMD5(md5_byte_t *md5_out, CString strText);