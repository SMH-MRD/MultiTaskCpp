#pragma once

//  共有メモリステータス
#define	OK_SHMEM			0			// 共有メモリ 生成/破棄正常
#define	ERR_SHMEM_CREATE	-1			// 共有メモリ Create異常
#define	ERR_SHMEM_VIEW		-2			// 共有メモリ View異常

#define	ON				1		// ON
#define	OFF				0		// OFF

using namespace std;

class CHelper
{
public:
	CHelper();
	~CHelper();
	static void Str2Wstr(const std::string &src, std::wstring &dest);
	static void put_bmp_built(HDC hdc, HBITMAP hbmp, POINT dst_pt, POINT src_pt);
	static void put_bmp_stretch(HDC hdc, HBITMAP hbmp, POINT dst_pt, POINT src_pt, int retio_persent);
	static int	cmnCreateShmem(LPCTSTR, DWORD, HANDLE*, LPVOID*, DWORD*);	//共有メモリ設定
	static int	cmnDeleteShMem(HANDLE*, LPVOID*);							//共有メモリ解除
};
