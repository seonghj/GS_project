#pragma once
#include "stdafx.h"
#include "MY_AI.h"
#include "CTimer.h"
#include "CMap.h"
#include "DB.h"

void wake_up_npc(int npc_id);
bool is_npc(int id);
void disconnect(int p_id);
void display_error(const char* msg, int err_no);
bool can_see(int id_a, int id_b);

void send_packet(int p_id, void* p);
void do_recv(int key);
int get_new_player_id(SOCKET p_socket);
void send_login_ok_packet(int p_id);
void send_move_packet(int c_id, int p_id);
void send_add_object(int c_id, int p_id);
void send_remove_object(int c_id, int p_id);
void send_chat(int to, int id, const char* message);

void do_move(int p_id, MOVE_DIR dir);
void process_packet(int p_id, unsigned char* p_buf);
void do_npc_move(OBJECT& npc, MOVE_DIR dir);
void worker(HANDLE h_iocp, SOCKET l_socket);

int AI_get_x(lua_State* L);
int AI_get_y(lua_State* L);
int AI_get_HP(lua_State* L);
int AI_get_type(lua_State* L);
int AI_get_LEVEL(lua_State* L);
int AI_send_mess(lua_State* L);
int AI_damage_to_player(lua_State* L);
int AI_move(lua_State* L);

int main();
