#pragma once

#include "CodeDef.h"
///# タスクオブジェクトインデックス構造体
typedef struct _st_iTask {
	int ana;
	int clerk;
	int comc;
	int comp;
	int ply;
	int pr;
	int mng;
}ST_iTask, *P_ST_iTask;

//### ORDER LIST ###
#define ORDER_MAX		8

/// JOB Order
typedef struct _stMotion_Element {	//運動要素
	int type;		//制御種別
	int status;		//制御種状態
	double _a;		//目標加減速度
	double _v;		//目標速度
	double _p;		//目標位置
	double _t;		//継続時間
	double vh_lim;	//速度制限High
	double vl_lim;	//速度制限Low
	double opt_d1;	//オプションdouble
	double opt_d2;	//オプションdouble
	int opt_i1;		//オプションint
	int opt_i2;		//オプションint
}ST_MOTION_ELEMENT, *LPST_MOTION_ELEMENT;

#define M_ELEMENT_MAX	32
#define M_AXIS			8	//動作軸
#define MH_AXIS			0	//主巻動作
#define TT_AXIS			1	//横行動作
#define GT_AXIS			2	//走行動作
#define BH_AXIS			3	//起伏動作
#define SLW_AXIS		4	//旋回動作
#define SKW_AXIS		5	//スキュー動作
#define LFT_AXIS		6	//吊具操作
typedef struct _stJOB_UNIT {	//作業要素（PICK、GROUND、PARK）
	int type;		//動作種別
	int mAct[M_AXIS];//実行対象　配列[0]から動作コードセット-1セットで終わり
	int iAct[M_AXIS];//実行中index -1で完了
	ST_MOTION_ELEMENT motions[M_AXIS][M_ELEMENT_MAX];
}ST_JOB_UNIT, *LPST_JOB_UNIT;

typedef struct _stJOB_Report {	//JOB完了報告フォーマット
	WORD status;
}ST_JOB_REPRORT, *LPST_JOB_REPRORT;

typedef struct _stJOB_ORDER {	//JOB　ORDER構造体
	WORD type;					//JOBタイプ
	WORD property;				//実行条件
	WORD status;				//実行ステータス　　-1：無効
	WORD result;				//実行結果
	ST_JOB_UNIT		from_recipe;//from運転パターン
	ST_JOB_UNIT		to_recipe;	//to運転パターン
	ST_JOB_REPRORT repo1;		//from完了報告
	ST_JOB_REPRORT repo2;		//to完了報告
}ST_JOB_ORDER, *LPST_JOB_ORDER;

/// MODE Order
typedef struct _stMODE_ORDER {	//MODE Order 構造体
	WORD type;				//MODEタイプ
	WORD property;			//実行条件
	WORD status;			//実行ステータス　-1：無効
	WORD result;			//実行結果
}ST_MODE_ORDER, *LPST_MODE_ORDER;

/// E-Stop Order
typedef struct _stESTOP_ORDER {
	WORD type;				//E-STOPタイプ
	WORD property;			//実行条件
	WORD status;			//実行ステータス
	WORD result;			//実行結果
}ST_ESTOP_ORDER, *LPST_ESTOP_ORDER;

/// 手動操作 Order
typedef struct _stMANUAL_ORDER {
	WORD type;				//手動操作タイプ
	WORD status;			//実行ステータス　-1：無効
	WORD mh;
	WORD tt;
	WORD gt;
	WORD bh;
	WORD slew;
}ST_MANUAL_ORDER, *LPST_MANUAL_ORDER;

/// ユーザインターフェイス Order
typedef struct _stUI_ORDER {
	WORD type;				//UI処理タイプ
	WORD status;			//実行ステータス　-1：無効
	WORD mh;
	WORD tt;
	WORD gt;
	WORD bh;
	WORD slew;
}ST_UI_ORDER, *LPST_UI_ORDER;

/// ステータス報告 Order
typedef struct _stSTAT_ORDER {
	WORD type;				//UI処理タイプ
	WORD status;			//実行ステータス　-1：無効
}ST_STAT_ORDER, *LPST_STAT_ORDER;


#define ORDER_MAX_ESTP	3
#define ORDER_MAX_MODE	3
#define ORDER_MAX_MANU	3
#define ORDER_MAX_JOB	3
#define ORDER_MAX_UI	3
#define ORDER_MAX_STAT	3

typedef struct _stORDERs {
	ST_ESTOP_ORDER	_ESTP[ORDER_MAX_ESTP];
	ST_MODE_ORDER	_MODE[ORDER_MAX_MODE];
	ST_MANUAL_ORDER _MANUAL[ORDER_MAX_JOB];
	ST_JOB_ORDER	_JOB[ORDER_MAX_JOB];
	ST_UI_ORDER		_UI[ORDER_MAX_UI];
	ST_STAT_ORDER	_STATUS[ORDER_MAX_STAT];
}ST_ORDERs, *LPST_ORDERs;
