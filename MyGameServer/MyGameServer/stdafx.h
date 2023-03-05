#pragma once
using namespace std;
#include <iostream>
#include <unordered_set>
#include <thread>
#include <vector>
#include <mutex>
#include <array>
#include <WS2tcpip.h>
#include <MSWSock.h>
#include <queue>
#include <random>
#include <chrono>
#include "protocol.h"
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "MSWSock.lib")
#pragma comment(lib, "libmySQL.lib")

extern "C" { //C로 정의된 라이브러리인것을 명시(컴파일러에게 알려줌)
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}
#pragma comment(lib, "lua54.lib")

enum OP_TYPE { 
	OP_RECV, 
	OP_SEND, 
	OP_ACCEPT, 
	OP_RANDOM_MOVE, 
	OP_ATTACK,
	OP_PLAYER_APPROACH,
	OP_AI_MOVE,
	OP_AI_RUNAWAY,
	OP_AI_BYE,
	OP_AI_RESPAWN,
	OP_AI_MOVETOPLAYER
};

struct EX_OVER
{
	WSAOVERLAPPED	m_over;
	WSABUF			m_wsabuf[1];
	unsigned char	m_packetbuf[MAX_BUFSIZE];
	OP_TYPE			m_op;
	SOCKET			m_csocket;					
};

enum PL_STATE { PLST_FREE, PLST_CONNECTED, PLST_INGAME };

struct SECTOR {
	unordered_set <int> m_object_list;
	int x, y;
	mutex m_list_lock;
};

enum OBJECT_TYPE {
	player,
	peace,
	roming,
	agro
};

struct OBJECT
{
	atomic<PL_STATE> m_state;
	SOCKET m_socket;
	int		id;

	EX_OVER m_recv_over;
	int m_prev_size;

	mutex  m_info_lock;
	atomic<bool> is_active;

	char m_name[200];
	short	x, y;
	atomic<int> HP;
	atomic<short> LEVEL;
	atomic<int> EXP;

	int MAX_EXP;
	int MAX_HP;

	short type;

	int move_time;

	unordered_set <int> m_view_list;
	mutex m_vlist_lock;
	int sector_x;
	int sector_y;

	lua_State* L;
	mutex m_sl;
	atomic<bool> is_move;
};