#pragma once
#include "CTaskObj.h"


extern vector<void*>	VectpCTaskObj;	//タスクオブジェクトのポインタ
extern ST_iTask g_itask;

class CComDevice :
	public CTaskObj
{
public:
	CComDevice();
	~CComDevice();
};

