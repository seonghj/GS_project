#pragma once
#include "stdafx.h"

struct TIMER_EVENT {
	int key;
	OP_TYPE e_type;
	chrono::system_clock::time_point start_time;
	int target_id;
	char message[MAX_BUFSIZE];

	constexpr bool operator< (const TIMER_EVENT& L) const {
		return start_time > L.start_time;
	}
};

class CTimer
{
public:
	CTimer() { }
	CTimer(HANDLE h) { m_hcp = h; }

	void add_event(int key, OP_TYPE ev_t, int delay_ms);
	void add_event(int kye, OP_TYPE ev_t, int delay_ms, int to);
	void add_event(int key, OP_TYPE ev_t, int delay_ms, char* buf);
	void add_move_event(int key, OP_TYPE ev_t, int delay_ms, int to, int dir);
	void do_timer();

	void Set_hcp(HANDLE h) { m_hcp = h; }

private:
	priority_queue<TIMER_EVENT> timer_queue;
	mutex timer_lock;
	HANDLE m_hcp;
};

