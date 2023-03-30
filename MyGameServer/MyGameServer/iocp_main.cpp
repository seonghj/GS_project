#include "iocp.h"

constexpr int SERVER_ID = 0;
constexpr int battle_mass_ID = -1;

array <OBJECT, MAX_USER + 1> objects;

SECTOR sectors[WORLD_WIDTH / SECTOR_SIZE][WORLD_HEIGHT / SECTOR_SIZE];

HANDLE h_iocp;

CTimer* g_pTimer = new CTimer;
MY_AI* g_pAI = new MY_AI;
MAP* g_pMap = new MAP;
DB* g_pDB = new DB;

//#define test

void display_error(const char* msg, int err_no)
{
	WCHAR* lpMsgBuf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err_no, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	cout << msg;
	wcout << lpMsgBuf << endl;
	LocalFree(lpMsgBuf);
}

bool can_see(int id_a, int id_b)
{
	return VIEW_RADIUS >= abs(objects[id_a].x - objects[id_b].x) &&
		VIEW_RADIUS >= abs(objects[id_a].y - objects[id_b].y);
}

bool can_move(int x, int y, MOVE_DIR dir)
{
	switch (dir) {
	case UP: if (y > 0) {
			return g_pMap->canmove[x][y - 1];
	}break;
	case DOWN: if (y < (WORLD_HEIGHT - 1)) {
			return g_pMap->canmove[x][y + 1];
	}break;
	case LEFT: if (x > 0) {
			return g_pMap->canmove[x - 1][y];
	}break;
	case RIGHT: if (x < (WORLD_WIDTH - 1)) {
			return g_pMap->canmove[x + 1][y];
	}break;
	}
	return false;
}

void send_packet(int p_id, void* p)
{
	if (p_id == 0) return;
	int p_size = reinterpret_cast<unsigned char*>(p)[0];
	int p_type = reinterpret_cast<unsigned char*>(p)[1];
	//cout << "To client [" << p_id << "] : ";
	//cout << "Packet [" << p_type << "]\n";
	EX_OVER* s_over = new EX_OVER;
	s_over->m_op = OP_SEND;
	memset(&s_over->m_over, 0, sizeof(s_over->m_over));
	memcpy(s_over->m_packetbuf, p, p_size);
	s_over->m_wsabuf[0].buf = reinterpret_cast<CHAR*>(s_over->m_packetbuf);
	s_over->m_wsabuf[0].len = p_size;
	int ret = WSASend(objects[p_id].m_socket, s_over->m_wsabuf, 1,
		NULL, 0, &s_over->m_over, 0);
	if (0 != ret) {
		int err_no = WSAGetLastError();
		if (WSA_IO_PENDING != err_no) {
			printf("id: %d ", p_id);
			display_error("WSASend : ", WSAGetLastError());
			disconnect(p_id);
		}
	}
}

void do_recv(int key)
{

	objects[key].m_recv_over.m_wsabuf[0].buf =
		reinterpret_cast<char*>(objects[key].m_recv_over.m_packetbuf)
		+ objects[key].m_prev_size;
	objects[key].m_recv_over.m_wsabuf[0].len = MAX_BUFSIZE - objects[key].m_prev_size;
	memset(&objects[key].m_recv_over.m_over, 0, sizeof(objects[key].m_recv_over.m_over));
	DWORD r_flag = 0;
	int ret = WSARecv(objects[key].m_socket, objects[key].m_recv_over.m_wsabuf, 1,
		NULL, &r_flag, &objects[key].m_recv_over.m_over, NULL);
	if (0 != ret) {
		int err_no = WSAGetLastError();
		if (WSA_IO_PENDING != err_no)
			display_error("WSARecv : ", WSAGetLastError());
	}

}

int get_new_player_id(SOCKET p_socket)
{
	for (int i = SERVER_ID + 1; i <= MAX_USER; ++i) {
		objects[i].m_info_lock.lock();
		if (PLST_FREE == objects[i].m_state) {
			objects[i].m_state = PLST_CONNECTED;
			objects[i].m_socket = p_socket;
			objects[i].m_name[0] = 0;
			objects[i].m_info_lock.unlock();
			return i;
		}
		objects[i].m_info_lock.unlock();
	}
	return -1;
}

void send_login_ok_packet(int p_id)
{
	sc_packet_login_ok p;
	p.HP = 10;
	p.id = p_id;
	p.LEVEL = 2;
	p.EXP = 0;
	p.size = sizeof(p);
	p.type = SC_LOGIN_OK;
	p.x = objects[p_id].x;
	p.y = objects[p_id].y;
	send_packet(p_id, &p);
}

void send_login_fail_packet(int p_id)
{
	sc_packet_login_fail p;
	p.size = sizeof(p);
	p.type = SC_LOGIN_FAIL;
	printf("tlqkf\n");
	send_packet(p_id, &p);
}


void send_move_packet(int c_id, int p_id)
{
	sc_packet_position p;
	p.id = p_id;
	p.size = sizeof(p);
	p.type = SC_POSITION;
	p.x = objects[p_id].x;
	p.y = objects[p_id].y;
	p.move_time = objects[p_id].move_time;
	send_packet(c_id, &p);
}

void send_add_object(int c_id, int p_id)
{
	sc_packet_add_object p;
	p.id = p_id;
	p.size = sizeof(p);
	p.type = SC_ADD_OBJECT;
	p.x = objects[p_id].x;
	p.y = objects[p_id].y;
	strcpy_s(p.name, objects[p_id].m_name);
	send_packet(c_id, &p);
}

void send_remove_object(int to, int p_id)
{
	sc_packet_remove_object p;
	p.id = p_id;
	p.size = sizeof(p);
	p.type = SC_REMOVE_OBJECT;
	send_packet(to, &p);
}

void send_chat(int to, int id, const char* message)
{
	sc_packet_chat p;
	p.id = id;
	p.size = sizeof(sc_packet_chat);
	p.type = SC_CHAT;
	strcpy_s(p.message, message);
	send_packet(to, reinterpret_cast<char*>(&p));
}

void send_object_stat(int id)
{
	sc_packet_stat_change p;
	p.id = id;
	p.size = sizeof(sc_packet_stat_change);
	p.type = SC_STAT_CHANGE;
	p.HP = objects[id].HP;
	p.EXP = objects[id].EXP;
	p.LEVEL = objects[id].LEVEL;
	send_packet(id, reinterpret_cast<char*>(&p));
}

void disconnect(int p_id)
{
	objects[p_id].m_info_lock.lock();
	if (objects[p_id].m_state == PLST_FREE) {
		objects[p_id].m_info_lock.unlock();
		return;
	}
	closesocket(objects[p_id].m_socket);
	objects[p_id].m_state = PLST_FREE;
	objects[p_id].m_info_lock.unlock();

	for (auto& pl : objects) {
		if (false == is_npc(pl.id)) {
			pl.m_info_lock.lock();
			if (PLST_INGAME == pl.m_state)
				send_remove_object(pl.id, p_id);
			pl.m_info_lock.unlock();
		}
	}
}

void wake_up_npc(int npc_id)
{
	if (objects[npc_id].is_active == false) {
		bool old_state = false;
		if (true == atomic_compare_exchange_strong(&objects[npc_id].is_active, &old_state, true)) {
			if (objects[npc_id].type == roming)
				g_pTimer->add_event(npc_id, OP_RANDOM_MOVE, 1000);
		}
	}
}

bool is_npc(int id)
{
	if (id >= NPC_ID_START)
		return true;
	return false;
}

void do_move(int p_id, MOVE_DIR dir)
{
	auto& x = objects[p_id].x;
	auto& y = objects[p_id].y;

	if (x > 1000 || y > 1000) return;

	switch (dir) {
	case UP: if (can_move(x, y, dir) == true) y--; break;
	case DOWN: if (can_move(x, y, dir) == true) y++; break;
	case LEFT: if (can_move(x, y, dir) == true) x--; break;
	case RIGHT: if (can_move(x, y, dir) == true) x++; break;
	}

	// 주변섹터의 오브젝트 리스트
	unordered_set<int> near_object_list;
	for (int i = objects[p_id].sector_x - 1; i < objects[p_id].sector_x + 2; i++) {
		for (int j = objects[p_id].sector_y - 1; j < objects[p_id].sector_y + 2; j++) {
			if ((0 <= i && i < WORLD_HEIGHT / SECTOR_SIZE)
				&& (0 <= j && j < WORLD_WIDTH / SECTOR_SIZE)) {
				sectors[i][j].m_list_lock.lock();
				near_object_list.insert(sectors[i][j].m_object_list.begin()
					, sectors[i][j].m_object_list.end());
				sectors[i][j].m_list_lock.unlock();
			}
		}
	}

	unordered_set<int> old_vl;
	objects[p_id].m_vlist_lock.lock();
	old_vl = objects[p_id].m_view_list;
	objects[p_id].m_vlist_lock.unlock();

	unordered_set<int> new_vl;
	for (auto& pl : near_object_list) {
		if (pl == p_id) continue;
		if ((objects[pl].m_state == PLST_INGAME) && can_see(p_id, pl)) {
			new_vl.insert(pl);
		}
	}

	send_move_packet(p_id, p_id);

	for (auto pl : new_vl) {
		if (0 == old_vl.count(pl)) {		
			objects[p_id].m_vlist_lock.lock();
			objects[p_id].m_view_list.insert(pl);
			objects[p_id].m_vlist_lock.unlock();
			send_add_object(p_id, pl);

			if (false == is_npc(pl)) {
				objects[pl].m_vlist_lock.lock();
				if (0 == objects[pl].m_view_list.count(p_id)) {
					objects[pl].m_view_list.insert(p_id);
					objects[pl].m_vlist_lock.unlock();
					send_add_object(pl, p_id);
				}
				else {
					objects[pl].m_vlist_lock.unlock();
					send_move_packet(pl, p_id);
				}
			}
			else {
				wake_up_npc(pl);
				EX_OVER* ex_over = new EX_OVER;
				ex_over->m_op = OP_PLAYER_APPROACH;
				*reinterpret_cast<int*>(ex_over->m_packetbuf) = p_id;
				PostQueuedCompletionStatus(h_iocp, 1, pl, &ex_over->m_over);
			}
		}
		else {								
			if (false == is_npc(pl)) {
				objects[pl].m_vlist_lock.lock();
				if (0 == objects[pl].m_view_list.count(p_id)) {
					objects[pl].m_view_list.insert(p_id);
					objects[pl].m_vlist_lock.unlock();
					send_add_object(pl, p_id);
				}
				else {
					objects[pl].m_vlist_lock.unlock();
					send_move_packet(pl, p_id);
				}
				send_move_packet(pl, p_id);
			}
			else {
				EX_OVER* ex_over = new EX_OVER;
				ex_over->m_op = OP_PLAYER_APPROACH;
				*reinterpret_cast<int*>(ex_over->m_packetbuf) = p_id;
				PostQueuedCompletionStatus(h_iocp, 1, pl, &ex_over->m_over);
			}
		}
	}

	for (auto pl : old_vl) {
		if (0 == new_vl.count(pl)) {
			objects[p_id].m_vlist_lock.lock();
			objects[p_id].m_view_list.erase(pl);
			objects[p_id].m_vlist_lock.unlock();
			send_remove_object(p_id, pl);

			if (false == is_npc(pl)) {
				objects[pl].m_vlist_lock.lock();
				if (0 != objects[pl].m_view_list.count(p_id)) {
					objects[pl].m_view_list.erase(p_id);
					objects[pl].m_vlist_lock.unlock();
					send_remove_object(pl, p_id);
				}
				else
					objects[pl].m_vlist_lock.unlock();
			}
		}
	}

	int sx = objects[p_id].sector_x;
	int sy = objects[p_id].sector_y;
	if ((sx != (objects[p_id].x / SECTOR_SIZE))
		|| (sy != (objects[p_id].y / SECTOR_SIZE))) {
		sectors[sx][sy].m_list_lock.lock();
		sectors[sx][sy].m_object_list.erase(p_id);
		sectors[sx][sy].m_list_lock.unlock();
		sx = objects[p_id].sector_x = objects[p_id].x / SECTOR_SIZE;
		sy = objects[p_id].sector_y = objects[p_id].y / SECTOR_SIZE;
		sectors[sx][sy].m_list_lock.lock();
		sectors[sx][sy].m_object_list.insert(p_id);
		sectors[sx][sy].m_list_lock.unlock();
	}
}

void respawn_AI(int id) {
	while (1) {
		objects[id].x = rand() % WORLD_WIDTH;
		objects[id].y = rand() % WORLD_HEIGHT;
		if (g_pMap->canmove[objects[id].x][objects[id].y] == true)
			break;
	}

	int sx = objects[id].sector_x = objects[id].x / SECTOR_SIZE;
	int sy = objects[id].sector_y = objects[id].y / SECTOR_SIZE;
	sectors[sx][sy].m_list_lock.lock();
	sectors[sx][sy].m_object_list.insert(id);
	sectors[sx][sy].m_list_lock.unlock();

	objects[id].LEVEL = 1 + (objects[id].sector_x / 2) + (objects[id].sector_y / 2);
	objects[id].EXP = objects[id].LEVEL * objects[id].LEVEL * 2;
	objects[id].HP = objects[id].LEVEL * 5;

	printf("%d respawn\n", id);
}

void player_lvup(int id) 
{
	if (objects[id].EXP >= objects[id].MAX_EXP) {
		objects[id].LEVEL = objects[id].LEVEL + 1;
		objects[id].MAX_HP = objects[id].MAX_HP + 10;
		objects[id].MAX_EXP = objects[id].MAX_EXP * 2;
		objects[id].HP = objects[id].MAX_HP;
		objects[id].EXP = 0;

		send_object_stat(id);
	}
}

void respawn_player(int id)
{
	objects[id].x = 15;
	objects[id].y = 16;

#ifdef test
	objects[id].x = rand() % 2000;
	objects[id].y = rand() % 2000;
#endif 

	objects[id].sector_x = objects[id].x / SECTOR_SIZE;
	objects[id].sector_y = objects[id].y / SECTOR_SIZE;
	sectors[objects[id].sector_x][objects[id].sector_y].m_list_lock.lock();
	sectors[objects[id].sector_x][objects[id].sector_y]
		.m_object_list.insert(id);
	sectors[objects[id].sector_x][objects[id].sector_y].m_list_lock.unlock();

	objects[id].EXP = objects[id].EXP / 2;
	objects[id].HP = objects[id].MAX_HP;

	send_move_packet(id, id);
	send_object_stat(id);
}

void process_packet(int p_id, unsigned char* p_buf)
{
	switch (p_buf[1]) {
	case CS_LOGIN: {
		cs_packet_login* packet = reinterpret_cast<cs_packet_login*>(p_buf);
		char name[20];
		strcpy_s(name, packet->player_id);

		bool state = g_pDB->Search_ID(name);
		if (state) {
			send_login_fail_packet(p_id);
			std::cout << "fall\n";
			break;
		}
		
		state = g_pDB->Insert_ID(name);
		if (!state) {
			send_login_fail_packet(p_id);
			std::cout << "make ID fall / ID: " << packet->player_id << "\n";
			break;
		}

		objects[p_id].m_info_lock.lock();
		strcpy_s(objects[p_id].m_name, packet->player_id);

		objects[p_id].x = 15;
		objects[p_id].y = 16;

#ifdef test
		objects[p_id].x = rand() % 2000;
		objects[p_id].y = rand() % 2000;
#endif 

		send_login_ok_packet(p_id);

		send_object_stat(p_id);

		objects[p_id].m_state = PLST_INGAME;

		int sx = objects[p_id].sector_x = objects[p_id].x / SECTOR_SIZE;
		int sy = objects[p_id].sector_y = objects[p_id].y / SECTOR_SIZE;
		objects[p_id].m_info_lock.unlock();

		sectors[sx][sy].m_list_lock.lock();
		sectors[sx][sy].m_object_list.insert(p_id);
		sectors[sx][sy].m_list_lock.unlock();

		for (auto& pl : objects) {
			if (p_id != pl.id) {
				pl.m_info_lock.lock();
				if (PLST_INGAME == pl.m_state) {
					if (can_see(p_id, pl.id)) {
						objects[p_id].m_vlist_lock.lock();
						objects[p_id].m_view_list.insert(pl.id);
						objects[p_id].m_vlist_lock.unlock();
						send_add_object(p_id, pl.id);
						if (false == is_npc(pl.id)) {
							objects[pl.id].m_vlist_lock.lock();
							objects[pl.id].m_view_list.insert(p_id);
							objects[pl.id].m_vlist_lock.unlock();
							send_add_object(pl.id, p_id);
						}
						else {
							wake_up_npc(pl.id);
						}
					}
				}
				pl.m_info_lock.unlock();
			}
		}

		break;
	}
	case CS_MOVE: {
		cs_packet_move* packet = reinterpret_cast<cs_packet_move*>(p_buf);
		objects[p_id].move_time = packet->move_time;
		do_move(p_id, (MOVE_DIR)packet->direction);
		break;
	}
	case CS_ATTACK: {
		cs_packet_attack* packet = reinterpret_cast<cs_packet_attack*>(p_buf);
		unordered_set<int> list;
		int id = packet->id;
		objects[id].m_vlist_lock.lock();
		for (auto& i : objects[id].m_view_list) {
			if (i <= 0) continue;
			if (((objects[i].x == objects[id].x - 1) && (objects[i].y == objects[id].y))
				|| ((objects[i].x == objects[id].x + 1) && (objects[i].y == objects[id].y))
				|| ((objects[i].x == objects[id].x) && (objects[i].y == objects[id].y - 1))
				|| ((objects[i].x == objects[id].x) && (objects[i].y == objects[id].y + 1))
				||((objects[i].x == objects[id].x) && (objects[i].y == objects[id].y))
				) {
				objects[i].HP = objects[i].HP - (objects[id].LEVEL * 5);
				char buf[100];
				sprintf_s(buf, "Deal %d damage to monster %d (Left HP: %d)"
					, (objects[id].LEVEL * 5), i, objects[i].HP.load());
				send_chat(id, battle_mass_ID, buf);

				if (objects[i].HP <= 0) {
					printf("id: %d dead\n", i);
					objects[id].EXP = objects[id].EXP.load() + objects[i].EXP.load();
					if (objects[id].type == roming)
						objects[id].EXP = objects[id].EXP.load() + objects[i].EXP.load();
					objects[i].x = 10000;
					objects[i].y = 10000;
					objects[i].is_move = false;
					send_remove_object(id, i);
					send_object_stat(id);
					char buf[100];
					sprintf_s(buf, "Defeat monster %d to gain %d EXP", i, objects[i].EXP.load());
					send_chat(id, battle_mass_ID, buf);
					player_lvup(id);
					g_pTimer->add_event(i, OP_AI_RESPAWN, 30000);
					list.insert(i);
				}
				else {
					objects[i].m_sl.lock();
					lua_getglobal(objects[i].L, "move_to_player");
					lua_pushnumber(objects[i].L, i);
					lua_pushnumber(objects[i].L, id);
					lua_pcall(objects[i].L, 2, 0, 0);
					objects[i].m_sl.unlock();
				}
			}

		}
		for (auto& i : list) {
			objects[id].m_view_list.erase(i);
		}


		objects[id].m_vlist_lock.unlock();
		break;
	}
	default:
		cout << "Unknown Packet Type from Client[" << p_id;
		cout << "] Packet Type [" << p_buf[1] << "]";
		while (true);
	}
}

void do_npc_move(OBJECT& npc, MOVE_DIR dir)
{
	int x = npc.x;
	int y = npc.y;

	if (x >= 2000 || y >= 2000) return;

	unordered_set<int> near_object_list;
	for (int i = objects[npc.id].sector_x - 1; i < objects[npc.id].sector_x + 2; i++) {
		for (int j = objects[npc.id].sector_y - 1; j < objects[npc.id].sector_y + 2; j++) {
			if ((0 <= i && i < WORLD_WIDTH / SECTOR_SIZE)
				&& (0 <= j && j < WORLD_HEIGHT / SECTOR_SIZE)) {
				sectors[i][j].m_list_lock.lock();
				near_object_list.insert(sectors[i][j].m_object_list.begin()
					, sectors[i][j].m_object_list.end());
				sectors[i][j].m_list_lock.unlock();
			}
		}
	}

	unordered_set<int> old_vl;
	for (auto& obj : near_object_list) {
		if (PLST_INGAME != objects[obj].m_state) continue;
		if (true == is_npc(obj)) continue;
		if (true == can_see(npc.id, obj))
			old_vl.insert(obj);
	}

	switch (dir) {
	case MOVE_DIR::RANDOM:
		switch (rand() % 4) {
		case 0: if (can_move(x, y, MOVE_DIR::UP) == true) y--; break;
		case 1: if (can_move(x, y, MOVE_DIR::DOWN) == true) y++; break;
		case 2: if (can_move(x, y, MOVE_DIR::LEFT) == true) x--; break;
		case 3: if (can_move(x, y, MOVE_DIR::RIGHT) == true) x++; break;
		}
		break;
	case UP: if (can_move(x, y, dir) == true) y--; break;
	case DOWN: if (can_move(x, y, dir) == true) y++; break;
	case LEFT: if (can_move(x, y, dir) == true) x--; break;
	case RIGHT: if (can_move(x, y, dir) == true) x++; break;
	}

	npc.x = x;
	npc.y = y;

	npc.m_sl.lock();
	lua_getglobal(npc.L, "set_pos");
	lua_pcall(npc.L, 1, 0, 0);
	npc.m_sl.unlock();

	unordered_set<int> new_vl;
	for (auto& obj : near_object_list) {
		if (PLST_INGAME != objects[obj].m_state) continue;
		if (true == is_npc(obj)) continue;
		if (true == can_see(npc.id, obj))
			new_vl.insert(obj);
	}

	for (auto pl : new_vl) {
		if (0 == old_vl.count(pl)) {
			// 플레이어의 시야에 등장
			objects[pl].m_vlist_lock.lock();
			objects[pl].m_view_list.insert(npc.id);
			objects[pl].m_vlist_lock.unlock();
			if (pl < NPC_ID_START) {
				send_add_object(pl, npc.id);
			}
		}
		else {
			// 플레이어가 계속 보고있음
			if (pl < NPC_ID_START) {
				send_move_packet(pl, npc.id);
				EX_OVER* ex_over = new EX_OVER;
				ex_over->m_op = OP_PLAYER_APPROACH;
				*reinterpret_cast<int*>(ex_over->m_packetbuf) = pl;
				PostQueuedCompletionStatus(h_iocp, 1, npc.id, &ex_over->m_over);
			}
		}
	}

	for (auto pl : old_vl) {
		if (0 == new_vl.count(pl)) {
			// 플레이어의 시야에서 나감
			objects[pl].m_vlist_lock.lock();
			if (0 != objects[pl].m_view_list.count(pl)) {
				objects[pl].m_view_list.erase(npc.id);
				objects[pl].m_vlist_lock.unlock();
				send_remove_object(pl, npc.id);
			}
			else
				objects[pl].m_vlist_lock.unlock();
		}
	}

	if ((objects[npc.id].sector_x != (objects[npc.id].x / SECTOR_SIZE))
		|| (objects[npc.id].sector_y != (objects[npc.id].y / SECTOR_SIZE))) {

		sectors[objects[npc.id].sector_x][objects[npc.id].sector_y].m_list_lock.lock();
		sectors[objects[npc.id].sector_x][objects[npc.id].sector_y]
			.m_object_list.erase(npc.id);
		sectors[objects[npc.id].sector_x][objects[npc.id].sector_y].m_list_lock.unlock();
		objects[npc.id].sector_x = objects[npc.id].x / SECTOR_SIZE;
		objects[npc.id].sector_y = objects[npc.id].y / SECTOR_SIZE;
		sectors[objects[npc.id].sector_x][objects[npc.id].sector_y].m_list_lock.lock();
		sectors[objects[npc.id].sector_x][objects[npc.id].sector_y]
			.m_object_list.insert(npc.id);
		sectors[objects[npc.id].sector_x][objects[npc.id].sector_y].m_list_lock.unlock();
	}
}


void worker(HANDLE h_iocp, SOCKET l_socket)
{
	while (true) {
		DWORD num_bytes;
		ULONG_PTR ikey;
		WSAOVERLAPPED* over;
		BOOL ret = GetQueuedCompletionStatus(h_iocp, &num_bytes,
			&ikey, &over, INFINITE);

		int key = static_cast<int>(ikey);
		if (FALSE == ret) {
			if (SERVER_ID == key) {
				display_error("GQCS : ", WSAGetLastError());
				exit(-1);
			}
			else {
				display_error("GQCS : ", WSAGetLastError());
				disconnect(key);
			}
		}
		if ((key != SERVER_ID) && (0 == num_bytes)) {
			disconnect(key);
			continue;
		}
		EX_OVER* ex_over = reinterpret_cast<EX_OVER*>(over);

		switch (ex_over->m_op) {
		case OP_RECV: {
			unsigned char* packet_ptr = ex_over->m_packetbuf;
			int num_data = num_bytes + objects[key].m_prev_size;
			int packet_size = packet_ptr[0];

			while (num_data >= packet_size) {
				process_packet(key, packet_ptr);
				num_data -= packet_size;
				packet_ptr += packet_size;
				if (0 >= num_data) break;
				packet_size = packet_ptr[0];
			}
			objects[key].m_prev_size = num_data;
			if (0 != num_data)
				memcpy(ex_over->m_packetbuf, packet_ptr, num_data);
			do_recv(key);
		}
					break;
		case OP_SEND:
			delete ex_over;
			break;
		case OP_ACCEPT: {
			int c_id = get_new_player_id(ex_over->m_csocket);
			if (-1 != c_id) {
				objects[c_id].m_recv_over.m_op = OP_RECV;
				objects[c_id].m_prev_size = 0;
				CreateIoCompletionPort(
					reinterpret_cast<HANDLE>(objects[c_id].m_socket), h_iocp, c_id, 0);
				do_recv(c_id);
			}
			else {
				closesocket(objects[c_id].m_socket);
			}

			memset(&ex_over->m_over, 0, sizeof(ex_over->m_over));
			SOCKET c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
			ex_over->m_csocket = c_socket;
			AcceptEx(l_socket, c_socket,
				ex_over->m_packetbuf, 0, 32, 32, NULL, &ex_over->m_over);
		}
					  break;
		case OP_RANDOM_MOVE:
			do_npc_move(objects[key], MOVE_DIR::RANDOM);
			g_pTimer->add_event(key, OP_RANDOM_MOVE, 1000);
			delete ex_over;
			break;
		case OP_ATTACK: {
			break;
		}
		case OP_PLAYER_APPROACH: {
			objects[key].m_sl.lock();
			int p_id = *reinterpret_cast<int*>(ex_over->m_packetbuf);
			lua_State* L = objects[key].L;
			lua_getglobal(L, "damage_to_player");
			lua_pushnumber(L, p_id);
			lua_pcall(L, 1, 0, 0);
			objects[key].m_sl.unlock();
			delete ex_over;
			break;
		}
		case OP_AI_MOVE: {
			event_move_to_player* emtp = reinterpret_cast<event_move_to_player*>(ex_over->m_packetbuf);
			do_npc_move(objects[key], (MOVE_DIR)emtp->dir);
			objects[key].m_sl.lock();
			if (objects[key].HP > 0) {
				lua_getglobal(objects[key].L, "move_to_player");
				lua_pushnumber(objects[key].L, key);
				lua_pushnumber(objects[key].L, emtp->to);
				int res = lua_pcall(objects[key].L, 2, 0, 0);
			}
			objects[key].m_sl.unlock();
			delete ex_over;
			break;
		}
		case OP_AI_RESPAWN: {
			respawn_AI(key);
			delete ex_over;
			break;
		}
		}
	}

}

int AI_get_x(lua_State* L)
{
	int obj_id = lua_tonumber(L, -1);
	lua_pop(L, 2);
	int x = objects[obj_id].x;
	lua_pushnumber(L, x);
	return 1;
}

int AI_get_y(lua_State* L)
{
	int obj_id = lua_tonumber(L, -1);
	lua_pop(L, 2);
	int y = objects[obj_id].y;
	lua_pushnumber(L, y);
	return 1;
}

int AI_get_HP(lua_State* L)
{
	int obj_id = lua_tonumber(L, -1);
	lua_pop(L, 2);
	int hp = objects[obj_id].HP;
	lua_pushnumber(L, hp);
	return 1;
}

int AI_get_type(lua_State* L)
{
	int obj_id = lua_tonumber(L, -1);
	lua_pop(L, 2);
	int t = objects[obj_id].type;
	lua_pushnumber(L, t);
	return 1;
}

int AI_get_LEVEL(lua_State* L)
{
	int obj_id = lua_tonumber(L, -1);
	lua_pop(L, 2);
	int lev = objects[obj_id].LEVEL;
	lua_pushnumber(L, lev);
	return 1;
}

int AI_send_mess(lua_State* L)
{
	int p_id = lua_tonumber(L, -3);
	int o_id = lua_tonumber(L, -2);
	const char* mess = lua_tostring(L, -1);
	lua_pop(L, 4);
	send_chat(p_id, o_id, mess);
	return 1;
}

int AI_damage_to_player(lua_State* L)
{
	int p_id = lua_tonumber(L, -2);
	int o_id = lua_tonumber(L, -1);
	lua_pop(L, 3);

	objects[p_id].HP = objects[p_id].HP - (objects[o_id].LEVEL * 2);
	send_object_stat(p_id);
	char buf[100];
	sprintf_s(buf, "Monster %d took damage %d to player", o_id, (objects[o_id].LEVEL * 2));
	send_chat(p_id, battle_mass_ID, buf);

	if (objects[p_id].HP <= 0)
		respawn_player(p_id);

	return 1;
}

int AI_move(lua_State* L)
{
	int id = lua_tonumber(L, -3);
	int to = lua_tonumber(L, -2);
	int dir = lua_tonumber(L, -1);
	lua_pop(L, 4);
	if (id <= 0) return 0;
	if (to <= 0) return 0;
	g_pTimer->add_move_event(id, OP_AI_MOVE, 1000, to, dir);
	return 1;
}

void init_sector() 
{
	for (int i = 0; i < WORLD_WIDTH / SECTOR_SIZE; ++i) {
		for (int j = 0; j < WORLD_HEIGHT / SECTOR_SIZE; ++j) {
			sectors[i][j].x = i;
			sectors[i][j].y = j;
			sectors[i][j].m_object_list.clear();
		}
	}
}

void init_objects()
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(1, WORLD_WIDTH - 1);

	for (int i = 0; i < MAX_USER + 1; ++i) {
		auto& pl = objects[i];
		pl.id = i;
		pl.m_state = PLST_FREE;

		pl.LEVEL = 1;
		pl.EXP = 0;
		pl.HP = 10;

		pl.MAX_EXP = 100;
		pl.MAX_HP = 10;

		if (true == is_npc(i)) {
			sprintf_s(pl.m_name, "N%d", i);
			pl.m_state = PLST_INGAME;
			while (1) {
				pl.x = dis(rd);
				pl.y = dis(rd);
				if (pl.x > 31 || pl.y > 31)
					if (g_pMap->canmove[pl.x][pl.y] == true)
						break;
			}
			pl.sector_x = pl.x / SECTOR_SIZE;
			pl.sector_y = pl.y / SECTOR_SIZE;
			sectors[pl.sector_x][pl.sector_y]
				.m_object_list.insert(i);

			pl.LEVEL = 1 + (pl.sector_x / 2) + (pl.sector_y / 2);
			pl.EXP = pl.LEVEL * pl.LEVEL * 2;
			pl.HP = pl.LEVEL * 5;
			pl.type = rand()%2 + 1;


			pl.is_move = true;

			lua_State* L = pl.L = luaL_newstate();
			luaL_openlibs(L);               // 기본 라이브러리 로드
			luaL_loadfile(L, "npc.lua");      // 가상머신, 프로그램 코드, 프로그램 코드 길이, 실행방식(줄단위)
			int res = lua_pcall(L, 0, 0, 0);            // 실행

			lua_getglobal(L, "set_id");
			lua_pushnumber(L, pl.id);
			lua_pcall(L, 1, 0, 0);

			lua_register(L, "AI_get_x", AI_get_x);
			lua_register(L, "AI_get_y", AI_get_y);
			lua_register(L, "AI_get_HP", AI_get_HP);
			lua_register(L, "AI_get_type", AI_get_type);
			lua_register(L, "AI_get_LEVEL", AI_get_LEVEL);
			lua_register(L, "AI_send_mess", AI_send_mess);
			lua_register(L, "AI_damage_to_player", AI_damage_to_player);
			lua_register(L, "AI_move", AI_move);

			lua_getglobal(L, "set_pos");
			lua_pcall(L, 0, 0, 0);

			lua_getglobal(L, "set_info");
			lua_pcall(L, 0, 0, 0);
		}
	}
}

int main()
{
	init_sector();

	//g_pMap->make_map();
	g_pMap->load_map();
	g_pDB->Connection_ODBC();

	init_objects();

	cout << "ready" << endl;

	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 2), &WSAData);

	wcout.imbue(locale("korean"));
	h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	
	SOCKET listenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(listenSocket), h_iocp, SERVER_ID, 0);
	SOCKADDR_IN serverAddr;
	memset(&serverAddr, 0, sizeof(SOCKADDR_IN));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	::bind(listenSocket, (struct sockaddr*)&serverAddr, sizeof(SOCKADDR_IN));
	listen(listenSocket, SOMAXCONN);

	EX_OVER accept_over;
	accept_over.m_op = OP_ACCEPT;
	memset(&accept_over.m_over, 0, sizeof(accept_over.m_over));
	SOCKET c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	accept_over.m_csocket = c_socket;
	BOOL ret = AcceptEx(listenSocket, c_socket, accept_over.m_packetbuf, 0, 32, 32, NULL, &accept_over.m_over);
	if (FALSE == ret) {
		int err_num = WSAGetLastError();
		if (err_num != WSA_IO_PENDING)
			display_error("AcceptEx Error", err_num);
	}


	vector <thread> worker_threads;
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	for (int i = 0; i < (int)si.dwNumberOfProcessors * 2; ++i)
		worker_threads.emplace_back(worker, h_iocp, listenSocket);

	g_pTimer->Set_hcp(h_iocp);
	thread timer_thread{ &CTimer::do_timer,  g_pTimer };
	timer_thread.join();
	
	for (auto& th : worker_threads)
		th.join();
	
	closesocket(listenSocket);
	WSACleanup();
}
