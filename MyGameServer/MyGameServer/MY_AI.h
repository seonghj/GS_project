#pragma once
#include "stdafx.h"
#include "iocp.h"
#include "CTimer.h"

class MY_AI{
public:

	void Set_m_pTimer(CTimer* t) { m_pTimer = t; }

private:
	CTimer* m_pTimer;
};

