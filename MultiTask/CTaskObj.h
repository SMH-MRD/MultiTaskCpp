#pragma once

#include "helper.h"
#include <process.h>
#include "resource.h"

#include "SharedObjects.h"
#include "CommonFormat.h"

using namespace std;

//#タスク条件設定部
#define TARGET_RESOLUTION			1
#define SYSTEM_TICK_ms				25
#define MAX_APL_TASK				8
#define TASK_NUM					7
#define INITIAL_TASK_STACK_SIZE		16384
#define DEFAUT_TASK_CYCLE			50
#define TASK_EVENT_MAX					8

//inf.thread_com用　スレッドループ制御用
#define REPEAT_INFINIT		0	//永久ループ
#define TERMINATE_THREAD	1	//スレッド中断
#define REPEAT_ONCE			-1	//逐次処理

//スレッドで事項する関数の選択用
#define THREAD_WORK_IDLE		0
#define THREAD_WORK_ROUTINE		1
#define THREAD_WORK_OPTION1		2
#define THREAD_WORK_OPTION2		3

#define MSG_LIST_MAX			14

#define ID_TIMER_EVENT			0	//タイマー用イベント配列の位置


/***********************************************************************
タスクオブジェクトの個別管理情報構造体
************************************************************************/
typedef struct {
	//-オブジェクト識別情報
	TCHAR			name[24];						//オブジェクト名
	TCHAR			sname[8];						//オブジェクト略称
	HBITMAP			hBmp;							//オブジェクト識別用ビットマップ

	//-スレッド設定内容
	int				index;							//スレッドIndex
	unsigned int	ID;								//スレッドID
	HANDLE			hndl;							//スレッドハンドル
	HANDLE			hevents[TASK_EVENT_MAX];		//イベントハンドル
	int				n_active_events = 1;			//有効なイベント数
	int				event_triggered;				//発生したイベントの番号
	unsigned int	cycle_ms;						//スレッド実行設定周期
	unsigned int	cycle_count;					//スレッド実行設定周期　Tick count（ms/system tick)
	int				trigger_type;					//スレッド起動条件　定周期orイベント
	int				priority;						//スレッドのプライオリティ
	int				thread_com;						//スレッド制御フラグ 　0:繰り返し　1:中断　それ以外：逐次処理:

	//-スレッドモニタ情報
	DWORD			start_time;						//現スキャンのスレッド開始時間
	DWORD			act_time;						//1スキャンのスレッド実処理時間
	DWORD			period;							//スレッドループ周期実績
	DWORD			act_count;						//スレッドループカウンタ
	DWORD			total_act;						//起動積算カウンタ
	unsigned int	time_over_count;				//予定周期をオーバーした回数

	//-関連ウィンドウハンドル
	HWND			hWnd_parent;					//親ウィンドウのハンドル
	HWND			hWnd_msgStatics;				//親ウィンドウメッセージ表示用ウィンドウへのハンドル
	HWND			hWnd_opepane;					//自メインウィンドウのハンドル（メインフレーム上に配置）
	HWND			hWnd_msgList;					//自メインウィンドウのメッセージ表示用リストコントロールへのハンドル
	HWND			hWnd_work;						//自専用作業用ウィンドウのハンドル

    //-操作パネル関連
	int				cnt_PNLlist_msg = 0;			//パネルメッセージリストのカウント数
	int				panel_func_id = 1;				//パネルfunctionボタンの選択内容
	int				panel_type_id = 1;				//パネルtypeボタンの選択内容
													
	//-外部インターフェース
	unsigned long	*psys_counter;					//メインシステムカウンターの参照先ポインタ
	unsigned		work_select=0;					//スレッド実行の関数の種類を指定

}ST_THREAD_INFO, *PST_THREAD_INFO;

/***********************************************************************
タスクスレッドオブジェクトベースクラス
************************************************************************/
class CTaskObj {
public:
	ST_THREAD_INFO inf;

	CTaskObj();
	virtual ~CTaskObj();//デストラクタ

	virtual void init_task(void *pobj);
	unsigned __stdcall run(void *param);//スレッド実行対象関数
		
	//スレッド実行対象関数内で呼ばれる処理 run()内でいずれかが選択実施される
	// >set_work()でいずれを実施するか設定する 
	virtual void routine_work(void *param);
	virtual void optional_work1(void *param);
	virtual void optional_work2(void *param);
	virtual void default_work(void *param) { Sleep(1000); };
	unsigned int set_work(int work_id) { inf.work_select = work_id; return(inf.work_select); };

	virtual void output_name() { const type_info &t = typeid(this); std::cout << "My type is " << t.name() << std::endl; }

	virtual HWND CreateOwnWindow(HWND h_parent_wnd);

	virtual LRESULT CALLBACK PanelProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp);
	virtual void set_panel_tip_txt();//タブパネルのStaticテキストを設定
	virtual void set_panel_pb_txt() { return; };//タブパネルのFunctionボタンのStaticテキストを設定
	virtual void set_PNLparam_value(float p1, float p2, float p3, float p4, float p5, float p6);//パラメータ初期表示値設定

protected:
	CHelper tool;
	ostringstream s;
	wostringstream ws;
	wstring wstr;
	string  str;

	void tweet2owner(const std::string &src);	//メインウィンドウへのショートメッセージ表示
	void tweet2owner(const std::wstring &srcw); //メインウィンドウへのショートメッセージ表示
	void txout2msg_listbox(const std::wstring str);//リストコントロールへのショートメッセージ表示
	void txout2msg_listbox(const std::string str);//リストコントロールへのショートメッセージ表示
};

