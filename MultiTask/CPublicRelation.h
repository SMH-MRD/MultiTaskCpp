#pragma once
#include "CTaskObj.h"

extern vector<void*>	VectpCTaskObj;	//タスクオブジェクトのポインタ
extern ST_iTask g_itask;

class CPublicRelation :
	public CTaskObj
{
public:
	CPublicRelation();
	~CPublicRelation();
};

