#include "CTimer.h"

void CTimer::add_event(int key, OP_TYPE ev_t, int delay_ms)
{
	using namespace chrono;
	TIMER_EVENT ev;
	ev.e_type = ev_t;
	ev.key = key;
	ev.start_time = system_clock::now() + milliseconds(delay_ms);
	ev.target_id = -1;
	//ZeroMemory(&ev.message, MAX_BUFSIZE);
	timer_lock.lock();
	timer_queue.push(ev);
	timer_lock.unlock();
}

void CTimer::add_event(int key, OP_TYPE ev_t, int delay_ms, int to)
{
	using namespace chrono;
	TIMER_EVENT ev;
	ev.e_type = ev_t;
	ev.key = key;
	ev.start_time = system_clock::now() + milliseconds(delay_ms);
	ev.target_id = to;
	//ZeroMemory(&ev.message, MAX_BUFSIZE);
	timer_lock.lock();
	timer_queue.push(ev);
	timer_lock.unlock();
}

void CTimer::add_event(int key, OP_TYPE ev_t, int delay_ms, char* buf)
{
	using namespace chrono;
	TIMER_EVENT ev;
	ev.e_type = ev_t;
	ev.key = key;
	ev.start_time = system_clock::now() + milliseconds(delay_ms);
	ev.target_id = -2;
	memcpy(ev.message, buf, buf[0]);
	timer_lock.lock();
	timer_queue.push(ev);
	timer_lock.unlock();
}

void CTimer::add_move_event(int key, OP_TYPE ev_t, int delay_ms, int to, int dir)
{
	using namespace chrono;
	TIMER_EVENT ev;
	event_move_to_player emtp;
	ev.e_type = ev_t;
	ev.key = key;
	ev.start_time = system_clock::now() + milliseconds(delay_ms);
	ev.target_id = -2;
	emtp.size = sizeof(event_move_to_player);
	emtp.dir = dir;
	emtp.to = to;
	memcpy(ev.message, reinterpret_cast<char*>(&emtp), emtp.size);
	timer_lock.lock();
	timer_queue.push(ev);
	timer_lock.unlock();
}

void CTimer::do_timer()
{
	using namespace chrono;

	while (1) {
		timer_lock.lock();
		if (false == timer_queue.empty()
			&& timer_queue.top().start_time <= system_clock::now()) {
			TIMER_EVENT ev = timer_queue.top();
			timer_queue.pop();
			timer_lock.unlock();
			EX_OVER* ex_over = new EX_OVER;
			ex_over->m_op = ev.e_type;
		    if (ev.target_id == -2)
				memcpy(ex_over->m_packetbuf, ev.message, ev.message[0]);
			PostQueuedCompletionStatus(m_hcp, 1, ev.key, (LPOVERLAPPED)&ex_over->m_over);
		}
		else {
			timer_lock.unlock();
			this_thread::sleep_for(10ms);
		}
	}
}