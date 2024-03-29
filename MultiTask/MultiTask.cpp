// MultiTask.cpp: アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"
#include "MultiTask.h"

#include "CTaskObj.h"		//タスクスレッドのクラス
#include "Helper.h"			//補助関数集合クラス

#include <windowsx.h> //コモンコントロール用
#include <commctrl.h> //コモンコントロール用

using namespace std;

#define MAX_LOADSTRING 100

///*****************************************************************************************************************
/// #ここにアプリケーション専用の関数を追加しています。

LRESULT CALLBACK TaskTabDlgProc(HWND, UINT, WPARAM, LPARAM);//個別タスク設定タブウィンドウ用メッセージハンドリング関数
HWND	CreateStatusbarMain(HWND);	//メインウィンドウステータスバー作成関数
VOID	CALLBACK alarmHandlar(UINT uID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2);	//マルチメディアタイマ処理関数　スレッドのイベントオブジェクト処理
int		Init_tasks(HWND hWnd);	//アプリケーション毎のタスクオブジェクトの初期設定
DWORD	knlTaskStartUp();	//実行させるタスクの起動処理
HWND	CreateTaskSettingWnd(HWND hWnd);

static unsigned __stdcall thread_gate_func(void * pObj) { //スレッド実行のためのゲート関数
	CTaskObj * pthread_obj = (CTaskObj *)pObj;  
	return pthread_obj->run(pObj); 
}
///*****************************************************************************************************************
/// #ここにアプリケーション専用のグローバル変数　または、スタティック変数を追加しています。
// グローバル変数:
HINSTANCE hInst;                                // 現在のインターフェイス
WCHAR szTitle[MAX_LOADSTRING];                  // タイトル バーのテキスト
WCHAR szWindowClass[MAX_LOADSTRING];            // メイン ウィンドウ クラス名
SYSTEMTIME gAppStartTime;						//アプリケーション開始時間
LPWSTR pszInifile;								// iniファイルのパス
wstring wstrPathExe;							// 実行ファイルのパス

vector<void*>	VectpCTaskObj;	//タスクオブジェクトのポインタ
ST_iTask g_itask;

// スタティック変数:
static HWND hWnd_status_bar;//ステータスバーのウィンドウのハンドル
static HWND hTabWnd;//操作パネル用タブコントロールウィンドウのハンドル
static ST_KNL_MANAGE_SET knl_manage_set;//マルチスレッド管理用構造体
static vector<HWND>	VectTweetHandle;	//メインウィンドウのスレッドツイートメッセージ表示Staticハンドル
static vector<HANDLE>	VectHevent;		//マルチスレッド用イベントのハンドル

static HIMAGELIST	hImgListTaskIcon;	//タスクアイコン用イメージリスト

static vector<HANDLE>	VecthSmemMutex;	//共有メモリミューテックスハンドル
static HANDLE			hSmemMapfile;	//共有メモリマップファイルオブジェクトハンドル
static LPVOID			pSmem[SMEM_MAX];		//共有メモリ先頭アドレス
static DWORD			SmemExist[SMEM_MAX];		//共有メモリ有無


///*****************************************************************************************************************

// このコード モジュールに含まれる関数の宣言を転送します:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);


// # 関数: wWinMain ************************************
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: ここにコードを挿入してください。

    // グローバル文字列を初期化しています。
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_MULTITASK, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // アプリケーションの初期化を実行します:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MULTITASK));

    MSG msg;

    // メイン メッセージ ループ:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}

// # 関数: MyRegisterClass ウィンドウ クラスを登録します。***
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MULTITASK));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_MULTITASK);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

// # 関数: InitInstance インスタンス ハンドルを保存して、メイン ウィンドウを作成します。***
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // グローバル変数にインスタンス処理を格納します。

   GetSystemTime(&gAppStartTime);//開始時刻取得


   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
	   MAIN_WND_INIT_POS_X, MAIN_WND_INIT_POS_Y,
	   TAB_DIALOG_W + 40, (MSG_WND_H + MSG_WND_Y_SPACE)*TASK_NUM + TAB_DIALOG_H + 110,
	   nullptr, nullptr, hInstance, nullptr);

   if (!hWnd) return FALSE;

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   // ini file path
   static WCHAR dstpath[_MAX_PATH], szDrive[_MAX_DRIVE], szPath[_MAX_PATH], szFName[_MAX_FNAME], szExt[_MAX_EXT];
   GetModuleFileName(NULL, dstpath, (sizeof(TCHAR) * _MAX_PATH));
   _wsplitpath_s(dstpath, szDrive, sizeof(szDrive), szPath, sizeof(szPath), szFName, sizeof(szFName), szExt, sizeof(szExt));

   wstrPathExe = szDrive; wstrPathExe += szPath;

   _wmakepath_s(dstpath, sizeof(dstpath), szDrive, szPath, NAME_OF_INIFILE, EXT_OF_INIFILE);
   pszInifile = dstpath;
   
   
   /// -共有メモリ設定
   TCHAR   name[128];
   DWORD	str_num = GetPrivateProfileString(SMEM_SECT_OF_INIFILE, SMEM_NAME0_KEY_OF_INIFILE, L"SMEMDEF", name, sizeof(name), PATH_OF_INIFILE);
   int nRet = CHelper::cmnCreateShmem(name, sizeof(CCOM_Table), &hSmemMapfile, &pSmem[SMEM_COM_TABLE_ID], &SmemExist[SMEM_COM_TABLE_ID]);
   if (nRet == OK_SHMEM) {
	   HANDLE hmu = CreateMutex(NULL, TRUE, name);
	   if (hmu != 0) VecthSmemMutex.push_back(hmu); //アクセス用Mutex
   }
   memset(pSmem[SMEM_COM_TABLE_ID], 0, sizeof(CCOM_Table));
   
	str_num = GetPrivateProfileString(SMEM_SECT_OF_INIFILE, SMEM_NAME1_KEY_OF_INIFILE, L"SMEMDEF", name, sizeof(name), PATH_OF_INIFILE);
	nRet = CHelper::cmnCreateShmem(name, sizeof(CMODE_Table), &hSmemMapfile, &pSmem[SMEM_MODE_ID], &SmemExist[SMEM_MODE_ID]);
   if (nRet == OK_SHMEM) {
	   HANDLE hmu = CreateMutex(NULL, TRUE, name);
	   if (hmu != 0) VecthSmemMutex.push_back(hmu); //アクセス用Mutex
   }
   memset(pSmem[SMEM_MODE_ID], 0, sizeof(CMODE_Table));

   str_num = GetPrivateProfileString(SMEM_SECT_OF_INIFILE, SMEM_NAME2_KEY_OF_INIFILE, L"SMEMDEF", name, sizeof(name), PATH_OF_INIFILE);
   nRet = CHelper::cmnCreateShmem(name, sizeof(CFAULT_Table), &hSmemMapfile, &pSmem[SMEM_FAULT_ID], &SmemExist[SMEM_FAULT_ID]);
   if (nRet == OK_SHMEM) {
	   HANDLE hmu = CreateMutex(NULL, TRUE, name);
	   if (hmu != 0) VecthSmemMutex.push_back(hmu); //アクセス用Mutex
   }
   memset(pSmem[SMEM_FAULT_ID], 0, sizeof(CFAULT_Table));

   str_num = GetPrivateProfileString(SMEM_SECT_OF_INIFILE, SMEM_NAME3_KEY_OF_INIFILE, L"SMEMDEF", name, sizeof(name), PATH_OF_INIFILE);
   nRet = CHelper::cmnCreateShmem(name, sizeof(CIO_Table), &hSmemMapfile, &pSmem[SMEM_IOTBL_ID], &SmemExist[SMEM_IOTBL_ID]);
   if (nRet == OK_SHMEM) {
	   HANDLE hmu = CreateMutex(NULL, TRUE, name);
	   if (hmu != 0) VecthSmemMutex.push_back(hmu); //アクセス用Mutex
   }
   memset(pSmem[SMEM_FAULT_ID], 0, sizeof(CFAULT_Table));

   /// -タスク設定	
   Init_tasks(hWnd);//タスク個別設定
      
	///WM_PAINTを発生させてアイコンを描画させる
   InvalidateRect(hWnd, NULL, FALSE);
   UpdateWindow(hWnd);


  /// -タスク起動									
   knlTaskStartUp(); 

   /// -マルチメディアタイマ起動
   {
	/// > マルチメディアタイマ精度設定
	TIMECAPS wTc;//マルチメディアタイマ精度構造体
	if (timeGetDevCaps(&wTc, sizeof(TIMECAPS)) != TIMERR_NOERROR) 	return((DWORD)FALSE);
	 knl_manage_set.mmt_resolution = MIN(MAX(wTc.wPeriodMin, TARGET_RESOLUTION), wTc.wPeriodMax);
	if (timeBeginPeriod(knl_manage_set.mmt_resolution) != TIMERR_NOERROR) return((DWORD)FALSE);

	 _RPT1(_CRT_WARN, "MMTimer Period = %d\n", knl_manage_set.mmt_resolution);

	 /// > マルチメディアタイマセット
	 knl_manage_set.KnlTick_TimerID = timeSetEvent(knl_manage_set.cycle_base, knl_manage_set.mmt_resolution, (LPTIMECALLBACK)alarmHandlar, 0, TIME_PERIODIC);

	 /// >マルチメディアタイマー起動失敗判定　メッセージBOX出してFALSE　returen
	 if (knl_manage_set.KnlTick_TimerID == 0) {	 //失敗確認表示
	   LPVOID lpMsgBuf;
	   FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
		   0, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), /* Default language*/(LPTSTR)&lpMsgBuf, 0, NULL);
	   MessageBox(NULL, (LPCWSTR)lpMsgBuf, L"MMT Failed!!", MB_OK | MB_ICONINFORMATION);// Display the string.
	   LocalFree(lpMsgBuf);// Free the buffer.
	   return((DWORD)FALSE);
	}
   }
   return TRUE;
}

// # 関数: メイン ウィンドウのメッセージを処理します。*********************************
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 選択されたメニューの解析:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }break;
	case WM_CREATE: {
		///メインウィンドウにステータスバー付加
		hWnd_status_bar = CreateStatusbarMain(hWnd);
		}break;

	case WM_PAINT:{
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
			//タスクツィートメッセージアイコン描画
			for (unsigned i = 0; i < knl_manage_set.num_of_task; i++) ImageList_Draw(hImgListTaskIcon, i, hdc, 0, i*(MSG_WND_H+ MSG_WND_Y_SPACE), ILD_NORMAL);

            EndPaint(hWnd, &ps);
        }break;
	case WM_SIZE:{

		SendMessage(hWnd_status_bar, WM_SIZE, wParam, lParam);//ステータスバーにサイズ変更を通知付加

		RECT rc;
		GetClientRect(hWnd, &rc);
		TabCtrl_AdjustRect(hTabWnd, FALSE, &rc);
		MoveWindow(hTabWnd, TAB_POS_X, TAB_POS_Y, TAB_DIALOG_W, TAB_DIALOG_H, TRUE);

		for (unsigned i = 0; i < VectpCTaskObj.size(); i++) {
			CTaskObj* pObj = (CTaskObj *) VectpCTaskObj[i];
			MoveWindow(pObj->inf.hWnd_opepane, TAB_POS_X, TAB_POS_Y + TAB_SIZE_H, TAB_DIALOG_W, TAB_DIALOG_H-TAB_SIZE_H, TRUE);
		}
	}break;
	case WM_NOTIFY:{
		int tab_index = TabCtrl_GetCurSel(((NMHDR *)lParam)->hwndFrom);
		for (unsigned i = 0; i < VectpCTaskObj.size(); i++) {

			CTaskObj * pObj = (CTaskObj *)VectpCTaskObj[i];
			MoveWindow(pObj->inf.hWnd_opepane, TAB_POS_X, TAB_POS_Y + TAB_SIZE_H, TAB_DIALOG_W, TAB_DIALOG_H-TAB_SIZE_H, TRUE);
			if ((VectpCTaskObj.size() - 1 - pObj->inf.index) == tab_index) {
				ShowWindow(pObj->inf.hWnd_opepane, SW_SHOW);
				HWND hname_static = GetDlgItem(pObj->inf.hWnd_opepane, IDC_TAB_TASKNAME);
				SetWindowText(hname_static, pObj->inf.name);
				pObj->set_panel_pb_txt();

				//実行関数の設定状況に応じてOption Checkボタンセット
				if (pObj->inf.work_select == THREAD_WORK_OPTION1) {
					SendMessage(GetDlgItem(pObj->inf.hWnd_opepane, IDC_TASK_OPTION_CHECK1), BM_SETCHECK, BST_CHECKED, 0L);
					SendMessage(GetDlgItem(pObj->inf.hWnd_opepane, IDC_TASK_OPTION_CHECK2), BM_SETCHECK, BST_UNCHECKED, 0L);
				}
				else if (pObj->inf.work_select == THREAD_WORK_OPTION2){
					SendMessage(GetDlgItem(pObj->inf.hWnd_opepane, IDC_TASK_OPTION_CHECK1), BM_SETCHECK, BST_UNCHECKED, 0L);
					SendMessage(GetDlgItem(pObj->inf.hWnd_opepane, IDC_TASK_OPTION_CHECK2), BM_SETCHECK, BST_CHECKED, 0L);
				}
				else {
					SendMessage(GetDlgItem(pObj->inf.hWnd_opepane, IDC_TASK_OPTION_CHECK1), BM_SETCHECK, BST_UNCHECKED, 0L);
					SendMessage(GetDlgItem(pObj->inf.hWnd_opepane, IDC_TASK_OPTION_CHECK2), BM_SETCHECK, BST_UNCHECKED, 0L);
				}
			}
			else {
				ShowWindow(pObj->inf.hWnd_opepane, SW_HIDE);
			}
		}
	}break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// # 関数:バージョン情報ボックスのメッセージ ハンドラーです。**************************
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

///# 関数: スレッドタスクの登録、設定 ***
int  Init_tasks(HWND hWnd) {

	HBITMAP hBmp;
	CTaskObj *ptempobj;
	int task_index = 0;

	InitCommonControls();//コモンコントロール初期化
	hImgListTaskIcon = ImageList_Create(32, 32, ILC_COLOR | ILC_MASK, 2, 0);//タスクアイコン表示用イメージリスト設定

	//###Task1 設定
	{
		/// -タスクインスタンス作成->リスト登録
		ptempobj = new CManager;
		VectpCTaskObj.push_back((void*)ptempobj);
		g_itask.mng = task_index;


		/// -タスクインデクスセット
		ptempobj->inf.index = task_index++;


		/// -イベントオブジェクトクリエイト->リスト登録	
		VectHevent.push_back(ptempobj->inf.hevents[ID_TIMER_EVENT] = CreateEvent(NULL, FALSE, FALSE, NULL));//自動リセット,初期値非シグナル

		/// -スレッド起動周期セット
		ptempobj->inf.cycle_ms = 1000;

		/// -ツイートメッセージ用iconセット
		hBmp = (HBITMAP)LoadBitmap(hInst, L"IDB_MAN1");//ビットマップ割り当て
		ImageList_AddMasked(hImgListTaskIcon, hBmp, RGB(255, 255, 255)); 
		DeleteObject(hBmp);

		 ///オブジェクト名セット
		DWORD	str_num = GetPrivateProfileString(OBJ_NAME_SECT_OF_INIFILE, MANAGER_KEY_OF_INIFILE, L"No Name", ptempobj->inf.name, sizeof(ptempobj->inf.name), PATH_OF_INIFILE);
				str_num = GetPrivateProfileString(OBJ_SNAME_SECT_OF_INIFILE, MANAGER_KEY_OF_INIFILE, L"No Name", ptempobj->inf.sname, sizeof(ptempobj->inf.sname), PATH_OF_INIFILE);

		///実行関数選択
				ptempobj->inf.work_select = THREAD_WORK_ROUTINE;
		///スレッド起動に使うイベント数（定周期タイマーのみの場合１）
				ptempobj->inf.n_active_events = 1;
	
	}
	//###Task2 設定
	{
		/// -タスクインスタンス作成->リスト登録
		ptempobj = new CPlayer;
		VectpCTaskObj.push_back((void*)ptempobj);
		g_itask.ply = task_index;

		/// -タスクインデクスセット
		ptempobj->inf.index = task_index++;

		/// -イベントオブジェクトクリエイト->リスト登録	
		VectHevent.push_back(ptempobj->inf.hevents[ID_TIMER_EVENT] = CreateEvent(NULL, FALSE, FALSE, NULL));//自動リセット,初期値非シグナル

																						   /// -スレッド起動周期セット
		ptempobj->inf.cycle_ms = 50;

		/// -ツイートメッセージ用iconセット
		hBmp = (HBITMAP)LoadBitmap(hInst, L"IDB_PLY1");//ビットマップ割り当て
		ImageList_AddMasked(hImgListTaskIcon, hBmp, RGB(255, 255, 255));
		DeleteObject(hBmp);

		///オブジェクト名セット
		DWORD	str_num = GetPrivateProfileString(OBJ_NAME_SECT_OF_INIFILE, PLAYER_KEY_OF_INIFILE, L"No Name", ptempobj->inf.name, sizeof(ptempobj->inf.name), PATH_OF_INIFILE);
		str_num = GetPrivateProfileString(OBJ_SNAME_SECT_OF_INIFILE, PLAYER_KEY_OF_INIFILE, L"No Name", ptempobj->inf.sname, sizeof(ptempobj->inf.sname), PATH_OF_INIFILE);

		///実行関数選択
		ptempobj->inf.work_select = THREAD_WORK_ROUTINE;

		///スレッド起動に使うイベント数（定周期タイマーのみの場合１）
		ptempobj->inf.n_active_events = 1;

	}
	

	//###Task3 設定
	{
		/// -タスクインスタンス作成->リスト登録
		ptempobj = new CComClient;
		VectpCTaskObj.push_back((void*)ptempobj);
		g_itask.comc = task_index;

		/// -タスクインデクスセット
		ptempobj->inf.index = task_index++;

		/// -イベントオブジェクトクリエイト->リスト登録	
		VectHevent.push_back(ptempobj->inf.hevents[ID_TIMER_EVENT] = CreateEvent(NULL, FALSE, FALSE, NULL));//自動リセット,初期値非シグナル

																											/// -スレッド起動周期セット
		ptempobj->inf.cycle_ms = 1000;

		/// -ツイートメッセージ用iconセット
		hBmp = (HBITMAP)LoadBitmap(hInst, L"IDB_CCOM");//ビットマップ割り当て
		ImageList_AddMasked(hImgListTaskIcon, hBmp, RGB(255, 255, 255));
		DeleteObject(hBmp);

		///オブジェクト名セット
		DWORD	str_num = GetPrivateProfileString(OBJ_NAME_SECT_OF_INIFILE, CLIENT_COM_KEY_OF_INIFILE, L"No Name", ptempobj->inf.name, sizeof(ptempobj->inf.name), PATH_OF_INIFILE);
		str_num = GetPrivateProfileString(OBJ_SNAME_SECT_OF_INIFILE, CLIENT_COM_KEY_OF_INIFILE, L"No Name", ptempobj->inf.sname, sizeof(ptempobj->inf.sname), PATH_OF_INIFILE);

		///実行関数選択
		ptempobj->inf.work_select = THREAD_WORK_ROUTINE;

		///スレッド起動に使うイベント数（定周期タイマーのみの場合１）
		ptempobj->inf.n_active_events = 1;

	}

	//###Task4 設定
	{
		/// -タスクインスタンス作成->リスト登録
		ptempobj = new CComDevice;
		VectpCTaskObj.push_back((void*)ptempobj);
		g_itask.comp = task_index;

		/// -タスクインデクスセット
		ptempobj->inf.index = task_index++;

		/// -イベントオブジェクトクリエイト->リスト登録	
		VectHevent.push_back(ptempobj->inf.hevents[ID_TIMER_EVENT] = CreateEvent(NULL, FALSE, FALSE, NULL));//自動リセット,初期値非シグナル

																											/// -スレッド起動周期セット
		ptempobj->inf.cycle_ms = 1000;

		/// -ツイートメッセージ用iconセット
		hBmp = (HBITMAP)LoadBitmap(hInst, L"IDB_PCOM");//ビットマップ割り当て
		ImageList_AddMasked(hImgListTaskIcon, hBmp, RGB(255, 255, 255));
		DeleteObject(hBmp);

		///オブジェクト名セット
		DWORD	str_num = GetPrivateProfileString(OBJ_NAME_SECT_OF_INIFILE, DEVICE_COM_KEY_OF_INIFILE, L"No Name", ptempobj->inf.name, sizeof(ptempobj->inf.name), PATH_OF_INIFILE);
		str_num = GetPrivateProfileString(OBJ_SNAME_SECT_OF_INIFILE, DEVICE_COM_KEY_OF_INIFILE, L"No Name", ptempobj->inf.sname, sizeof(ptempobj->inf.sname), PATH_OF_INIFILE);

		///実行関数選択
		ptempobj->inf.work_select = THREAD_WORK_ROUTINE;

		///スレッド起動に使うイベント数（定周期タイマーのみの場合１）
		ptempobj->inf.n_active_events = 1;

	}

	//###Task5 設定
	{
		/// -タスクインスタンス作成->リスト登録
		ptempobj = new CPublicRelation;
		VectpCTaskObj.push_back((void*)ptempobj);
		g_itask.pr = task_index;

		/// -タスクインデクスセット
		ptempobj->inf.index = task_index++;

		/// -イベントオブジェクトクリエイト->リスト登録	
		VectHevent.push_back(ptempobj->inf.hevents[ID_TIMER_EVENT] = CreateEvent(NULL, FALSE, FALSE, NULL));//自動リセット,初期値非シグナル

																											/// -スレッド起動周期セット
		ptempobj->inf.cycle_ms = 1000;

		/// -ツイートメッセージ用iconセット
		hBmp = (HBITMAP)LoadBitmap(hInst, L"IDB_PR");//ビットマップ割り当て
		ImageList_AddMasked(hImgListTaskIcon, hBmp, RGB(255, 255, 255));
		DeleteObject(hBmp);

		///オブジェクト名セット
		DWORD	str_num = GetPrivateProfileString(OBJ_NAME_SECT_OF_INIFILE, PR_KEY_OF_INIFILE, L"No Name", ptempobj->inf.name, sizeof(ptempobj->inf.name), PATH_OF_INIFILE);
		str_num = GetPrivateProfileString(OBJ_SNAME_SECT_OF_INIFILE, PR_KEY_OF_INIFILE, L"No Name", ptempobj->inf.sname, sizeof(ptempobj->inf.sname), PATH_OF_INIFILE);

		///実行関数選択
		ptempobj->inf.work_select = THREAD_WORK_ROUTINE;

		///スレッド起動に使うイベント数（定周期タイマーのみの場合１）
		ptempobj->inf.n_active_events = 1;

	}

	//###Task6 設定
	{
		/// -タスクインスタンス作成->リスト登録
		ptempobj = new CClerk;
		VectpCTaskObj.push_back((void*)ptempobj);
		g_itask.clerk = task_index;

		/// -タスクインデクスセット
		ptempobj->inf.index = task_index++;

		/// -イベントオブジェクトクリエイト->リスト登録	
		VectHevent.push_back(ptempobj->inf.hevents[ID_TIMER_EVENT] = CreateEvent(NULL, FALSE, FALSE, NULL));//自動リセット,初期値非シグナル

																											/// -スレッド起動周期セット
		ptempobj->inf.cycle_ms = 1000;

		/// -ツイートメッセージ用iconセット
		hBmp = (HBITMAP)LoadBitmap(hInst, L"IDB_CLERK");//ビットマップ割り当て
		ImageList_AddMasked(hImgListTaskIcon, hBmp, RGB(255, 255, 255));
		DeleteObject(hBmp);

		///オブジェクト名セット
		DWORD	str_num = GetPrivateProfileString(OBJ_NAME_SECT_OF_INIFILE, CLERK_KEY_OF_INIFILE, L"No Name", ptempobj->inf.name, sizeof(ptempobj->inf.name), PATH_OF_INIFILE);
		str_num = GetPrivateProfileString(OBJ_SNAME_SECT_OF_INIFILE, CLERK_KEY_OF_INIFILE, L"No Name", ptempobj->inf.sname, sizeof(ptempobj->inf.sname), PATH_OF_INIFILE);

		///実行関数選択
		ptempobj->inf.work_select = THREAD_WORK_ROUTINE;

		///スレッド起動に使うイベント数（定周期タイマーのみの場合１）
		ptempobj->inf.n_active_events = 1;

	}

	//###Task7 設定
	{
		/// -タスクインスタンス作成->リスト登録
		ptempobj = new CAnalyst;
		VectpCTaskObj.push_back((void*)ptempobj);
		g_itask.ana = task_index;

		/// -タスクインデクスセット
		ptempobj->inf.index = task_index++;

		/// -イベントオブジェクトクリエイト->リスト登録	
		VectHevent.push_back(ptempobj->inf.hevents[ID_TIMER_EVENT] = CreateEvent(NULL, FALSE, FALSE, NULL));//自動リセット,初期値非シグナル

																											/// -スレッド起動周期セット
		ptempobj->inf.cycle_ms = 1000;

		/// -ツイートメッセージ用iconセット
		hBmp = (HBITMAP)LoadBitmap(hInst, L"IDB_ANALYST");//ビットマップ割り当て
		ImageList_AddMasked(hImgListTaskIcon, hBmp, RGB(255, 255, 255));
		DeleteObject(hBmp);

		///オブジェクト名セット
		DWORD	str_num = GetPrivateProfileString(OBJ_NAME_SECT_OF_INIFILE, ANALYST_KEY_OF_INIFILE, L"No Name", ptempobj->inf.name, sizeof(ptempobj->inf.name), PATH_OF_INIFILE);
		str_num = GetPrivateProfileString(OBJ_SNAME_SECT_OF_INIFILE, ANALYST_KEY_OF_INIFILE, L"No Name", ptempobj->inf.sname, sizeof(ptempobj->inf.sname), PATH_OF_INIFILE);

		///実行関数選択
		ptempobj->inf.work_select = THREAD_WORK_ROUTINE;

		///スレッド起動に使うイベント数（定周期タイマーのみの場合１）
		ptempobj->inf.n_active_events = 1;

	}

	///各タスク用設定ウィンドウ作成
	InitCommonControls();	//コモンコントロール初期化
	hTabWnd = CreateTaskSettingWnd(hWnd);//タブウィンドウ作成
	
	//各タスク残パラメータセット
	knl_manage_set.num_of_task = (unsigned int)VectpCTaskObj.size();//タスク数登録
	for (unsigned i = 0; i < knl_manage_set.num_of_task; i++) {
		CTaskObj * pobj = (CTaskObj *)VectpCTaskObj[i];
	
		pobj->inf.index = i;	//タスクインデックスセット
		
		// -ツイートメッセージ用Static window作成->リスト登録	
		pobj->inf.hWnd_msgStatics = CreateWindow(L"STATIC", L"...", WS_CHILD | WS_VISIBLE, MSG_WND_ORG_X, MSG_WND_ORG_Y + MSG_WND_H * i + i * MSG_WND_Y_SPACE, MSG_WND_W, MSG_WND_H, hWnd, (HMENU)ID_STATIC_MAIN, hInst, NULL);
		VectTweetHandle.push_back(pobj->inf.hWnd_msgStatics);
		
		//その他設定
		pobj->inf.psys_counter = &knl_manage_set.sys_counter;
		pobj->inf.act_count = 0;//起動チェック用カウンタリセット
		 //起動周期カウント値
		if (pobj->inf.cycle_ms >= SYSTEM_TICK_ms)	pobj->inf.cycle_count = pobj->inf.cycle_ms / SYSTEM_TICK_ms;
		else pobj->inf.cycle_count = 1;

		//最後に初期化関数呼び出し
		pobj->init_task(pobj);
	}

	return 1;
}

///# 関数: マルチタスクスタートアップ処理関数 ***
DWORD knlTaskStartUp()	//実行させるオブジェクトのリストのスレッドを起動
{
	//機能	：[KNL]システム/ユーザタスクスタートアップ関数
	//処理	：自プロセスのプライオリティ設定，カーネルの初期設定,タスク生成，基本周期設定
	//戻り値：Win32APIエラーコード
	
	HANDLE	myPrcsHndl;	/* 本プログラムのプロセスハンドル */
						///# 自プロセスプライオリティ設定処理
						//-プロセスハンドル取得
	if ((myPrcsHndl = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_SET_INFORMATION, FALSE, _getpid())) == NULL)	return(GetLastError());
	_RPT1(_CRT_WARN, "KNL Priority For Windows(before) = %d \n", GetPriorityClass(myPrcsHndl));

	//-自プロセスのプライオリティを最優先ランクに設定
	if (SetPriorityClass(myPrcsHndl, REALTIME_PRIORITY_CLASS) == 0)	return(GetLastError());
	_RPT1(_CRT_WARN, "KNL Priority For NT(after) = %d \n", GetPriorityClass(myPrcsHndl));

	///# アプリケーションタスク数が最大数を超えた場合は終了
	if (VectpCTaskObj.size() >= MAX_APL_TASK)	return((DWORD)ERROR_BAD_ENVIRONMENT);

	///#    アプリケーションスレッド生成処理	
	for (unsigned i = 0; i < VectpCTaskObj.size(); i++) {

		CTaskObj * pobj = (CTaskObj *)VectpCTaskObj[i];

		// タスク生成(スレッド生成)
		// 他ﾌﾟﾛｾｽとの共有なし,スタック初期サイズ　デフォルト, スレッド実行関数　引数で渡すオブジェクトで対象切り替え,スレッド関数の引数（対象のオブジェクトのポインタ）, 即実行Createflags, スレッドID取り込み
		pobj->inf.hndl = (HANDLE)_beginthreadex((void *)NULL, 0, thread_gate_func, VectpCTaskObj[i], (unsigned)0, (unsigned *)&(pobj->inf.ID));

		// タスクプライオリティ設定
		if (SetThreadPriority(pobj->inf.hndl, pobj->inf.priority) == 0)
			return(GetLastError());
		_RPT2(_CRT_WARN, "Task[%d]_priority = %d\n", i, GetThreadPriority(pobj->inf.hndl));

		pobj->inf.act_count = 0;		// 基本ティックのカウンタ変数クリア
		pobj->inf.time_over_count = 0;	// 予定周期オーバーカウントクリア
	}

	return 1;
}

///# 関数: マルチタイマーイベント処理関数 ****************************************************
VOID CALLBACK alarmHandlar( UINT uID, UINT uMsg, DWORD	dwUser,	DWORD dw1, DWORD dw2){

	LONG64 tmttl;
	knl_manage_set.sys_counter++;

	//スレッド再開イベントセット処理
	for (unsigned i = 0; i < knl_manage_set.num_of_task; i++) {
		CTaskObj * pobj = (CTaskObj *)VectpCTaskObj[i];
		pobj->inf.act_count++;
		if (pobj->inf.act_count >= pobj->inf.cycle_count) {
			PulseEvent(VectHevent[i]);
			pobj->inf.act_count = 0;
			pobj->inf.total_act++;
		}
	}

	//Statusバーに経過時間表示
	if (knl_manage_set.sys_counter % 40 == 0) {// 1sec毎

		//起動後経過時間計算
		tmttl = knl_manage_set.sys_counter * knl_manage_set.cycle_base;//アプリケーション起動後の経過時間msec
		knl_manage_set.Knl_Time.wMilliseconds = (WORD)(tmttl % 1000); tmttl /= 1000;
		knl_manage_set.Knl_Time.wSecond = (WORD)(tmttl % 60); tmttl /= 60;
		knl_manage_set.Knl_Time.wMinute = (WORD)(tmttl % 60); tmttl /= 60;
		knl_manage_set.Knl_Time.wHour = (WORD)(tmttl % 60); tmttl /= 24;
		knl_manage_set.Knl_Time.wDay = (WORD)(tmttl % 24);

		TCHAR tbuf[32];
		wsprintf(tbuf, L"%3dD %02d:%02d:%02d", knl_manage_set.Knl_Time.wDay, knl_manage_set.Knl_Time.wHour, knl_manage_set.Knl_Time.wMinute, knl_manage_set.Knl_Time.wSecond);
		SendMessage(hWnd_status_bar, SB_SETTEXT, 5, (LPARAM)tbuf);
	}
}

///# 関数: ステータスバー作成 ***************
HWND CreateStatusbarMain(HWND hWnd)
{
	HWND hSBWnd;
	int sb_size[] = { 100,200,300,400,525,615 };//ステータス区切り位置

	InitCommonControls();
	hSBWnd = CreateWindowEx(
		0, //拡張スタイル
		STATUSCLASSNAME, //ウィンドウクラス
		NULL, //タイトル
		WS_CHILD | SBS_SIZEGRIP, //ウィンドウスタイル
		0, 0, //位置
		0, 0, //幅、高さ
		hWnd, //親ウィンドウ
		(HMENU)ID_STATUS, //ウィンドウのＩＤ
		hInst, //インスタンスハンドル
		NULL);
	SendMessage(hSBWnd, SB_SETPARTS, (WPARAM)6, (LPARAM)(LPINT)sb_size);//6枠で各枠の仕切り位置をパラーメータ指定
	ShowWindow(hSBWnd, SW_SHOW);
	return hSBWnd;
} 

///# 関数: タブ付タスクウィンドウ作成 *******
HWND CreateTaskSettingWnd(HWND hWnd) {
	
	RECT rc;
	TC_ITEM tc[TASK_NUM];

	GetClientRect(hWnd, &rc);
	HWND hTab = CreateWindowEx(0, WC_TABCONTROL, NULL, WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
		rc.left + TAB_POS_X, rc.top + TAB_POS_Y, TAB_DIALOG_W, TAB_DIALOG_H, hWnd, (HMENU)ID_TASK_SET_TAB, hInst, NULL);

		for (unsigned i = 0; i < VectpCTaskObj.size(); i++) {//Task Setting用タブ作成
		CTaskObj* pObj = (CTaskObj *)VectpCTaskObj[i];

		tc[i].mask = (TCIF_TEXT | TCIF_IMAGE);
		tc[i].pszText = pObj->inf.sname;
		tc[i].iImage = pObj->inf.index;
		SendMessage(hTab, TCM_INSERTITEM, (WPARAM)0, (LPARAM)&tc[i]);
		pObj->inf.hWnd_opepane = CreateDialog(hInst, L"IDD_DIALOG_TASKSET1", hWnd, (DLGPROC)TaskTabDlgProc);
		pObj->set_panel_pb_txt();
		MoveWindow(pObj->inf.hWnd_opepane, TAB_POS_X, TAB_POS_Y + TAB_SIZE_H, TAB_DIALOG_W, TAB_DIALOG_H - TAB_SIZE_H, TRUE);
	
		//初期値はindex 0のウィンドウを表示
		if (i == 0) {
			ShowWindow(pObj->inf.hWnd_opepane, SW_SHOW);
			SetWindowText(GetDlgItem(pObj->inf.hWnd_opepane, IDC_TAB_TASKNAME), pObj->inf.name);//タスク名をスタティックテキストに表示
		}
		else ShowWindow(pObj->inf.hWnd_opepane, SW_HIDE);
	}
	
	//タブコントロールにイメージリストセット
	SendMessage(hTab, TCM_SETIMAGELIST, (WPARAM)0, (LPARAM)hImgListTaskIcon);
	return hTab;
}

static LVCOLUMNA lvcol;

LRESULT CALLBACK TaskTabDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg) {
	case WM_INITDIALOG: {
	
		InitCommonControls();

		//メッセージ用リスト
		LVCOLUMN lvcol;
		LPTSTR strItem0[] = { (LPTSTR)(L"time"), (LPTSTR)(L"message") };//列ラベル
		int CX[] = { 60, 600 };//列幅

		//リストコントロール設定
		HWND hList = GetDlgItem(hDlg, IDC_LIST1);
		lvcol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		lvcol.fmt = LVCFMT_LEFT;
		for (int i = 0; i < 2; i++) {
			lvcol.cx = CX[i];             // 表示位置
			lvcol.pszText = strItem0[i];  // 見出し
			lvcol.iSubItem = i;           // サブアイテムの番号
			ListView_InsertColumn(hList, i, &lvcol);
		}
		//リスト行追加
		LVITEM item;
		item.mask = LVIF_TEXT;
		for (int i = 0; i < MSG_LIST_MAX; i++) {
			item.pszText = (LPWSTR)L".";   // テキスト
			item.iItem = i;               // 番号
			item.iSubItem = 0;            // サブアイテムの番号
			ListView_InsertItem(hList, &item);
		}
		return TRUE;
	}break;
	case WM_COMMAND:{
		CTaskObj * pObj = (CTaskObj *) VectpCTaskObj[VectpCTaskObj.size() - TabCtrl_GetCurSel(hTabWnd) - 1];
		pObj->PanelProc(hDlg, msg, wp, lp); 
	}break;
	}
	return FALSE;
}





