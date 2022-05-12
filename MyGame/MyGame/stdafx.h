#pragma once
#define SFML_STATIC 1
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <iostream>
#include <fstream>
#include <chrono>
using namespace std;

#ifdef _DEBUG
#pragma comment (lib, "lib/sfml-graphics-s-d.lib")
#pragma comment (lib, "lib/sfml-window-s-d.lib")
#pragma comment (lib, "lib/sfml-system-s-d.lib")
#pragma comment (lib, "lib/sfml-network-s-d.lib")
#else
#pragma comment (lib, "lib/sfml-graphics-s.lib")
#pragma comment (lib, "lib/sfml-window-s.lib")
#pragma comment (lib, "lib/sfml-system-s.lib")
#pragma comment (lib, "lib/sfml-network-s.lib")
#endif
#pragma comment (lib, "opengl32.lib")
#pragma comment (lib, "winmm.lib")
#pragma comment (lib, "ws2_32.lib")

#include "..\..\MyGameServer\MyGameServer\protocol.h"

sf::TcpSocket socket;
sf::RenderWindow* g_window;
sf::Font g_font;
sf::Font g_font2;

constexpr auto SCREEN_WIDTH = 20;
constexpr auto SCREEN_HEIGHT = 20;

constexpr auto TILE_WIDTH = 40;
constexpr auto WINDOW_WIDTH = TILE_WIDTH * SCREEN_WIDTH + 10;   // size of window
constexpr auto WINDOW_HEIGHT = TILE_WIDTH * SCREEN_WIDTH + 10;
constexpr auto BUF_SIZE = MAX_BUFSIZE;

constexpr int TREE_TOTAL = 100000;
constexpr int ROCK_TOTAL = 50000;

constexpr int battle_mass_ID = -1;
char battle_mass_buf[100];

int g_left_x;
int g_top_y;
int g_myid;

enum OBJECT_TYPE {
	player,
	monster
};

class EFFECT {
private:
	bool m_showing;
	sf::Sprite m_sprite01;
	sf::Sprite m_sprite02;
	sf::Sprite m_sprite03;
	sf::Sprite m_sprite04;
public:
	int m_x, m_y;
	int id;

	EFFECT(sf::Texture& t, int x, int y, int x2, int y2) {
		m_showing = false;
		m_sprite01.setTexture(t);
		m_sprite01.setTextureRect(sf::IntRect(x, y, x2, y2));
		m_sprite02.setTexture(t);
		m_sprite02.setTextureRect(sf::IntRect(x, y, x2, y2));
		m_sprite02.setRotation(90);
		m_sprite03.setTexture(t);
		m_sprite03.setTextureRect(sf::IntRect(x, y, x2, y2));
		m_sprite03.setRotation(180);
		m_sprite04.setTexture(t);
		m_sprite04.setTextureRect(sf::IntRect(x, y, x2, y2));
		m_sprite04.setRotation(270);
	}
	EFFECT() {
		m_showing = false;
	}
	void show(int x, int y)
	{
		m_x = x;
		m_y = y;
		m_showing = true;
	}
	void hide()
	{
		m_showing = false;
	}

	void move(int x, int y) {
		m_x = x;
		m_y = y;
	}
	void draw() {
		if (false == m_showing) return;
		float rx = (m_x - g_left_x) * TILE_WIDTH + 10;
		float ry = (m_y - g_top_y) * TILE_WIDTH + 10;
		m_sprite01.setPosition(rx + TILE_WIDTH, ry);
		m_sprite02.setPosition(rx + TILE_WIDTH, ry+TILE_WIDTH);
		m_sprite03.setPosition(rx, ry + TILE_WIDTH);
		m_sprite04.setPosition(rx, ry);
		/*m_sprite02.setPosition(rx, ry + TILE_WIDTH);
		m_sprite03.setPosition(rx - TILE_WIDTH, ry);
		m_sprite04.setPosition(rx, ry - TILE_WIDTH);*/
		g_window->draw(m_sprite01);
		g_window->draw(m_sprite02);
		g_window->draw(m_sprite03);
		g_window->draw(m_sprite04);
	}
};

class OBJECT {
private:
	bool m_showing;
	sf::Sprite m_sprite;
	sf::Text m_name;
	sf::Text m_chat;
	chrono::system_clock::time_point m_mess_end_time;
public:
	int m_x, m_y;
	int id;
	int HP, EXP, LEVEL;
	OBJECT_TYPE type;

	OBJECT(sf::Texture& t, int x, int y, int x2, int y2) {
		m_showing = false;
		m_sprite.setTexture(t);
		m_sprite.setTextureRect(sf::IntRect(x, y, x2, y2));
		m_mess_end_time = chrono::system_clock::now();
	}
	OBJECT() {
		m_showing = false;
	}
	void show()
	{
		m_showing = true;
	}
	void hide()
	{
		m_showing = false;
	}

	void a_move(int x, int y) {
		m_sprite.setPosition((float)x, (float)y);
	}

	void a_draw() {
		g_window->draw(m_sprite);
	}

	void move(int x, int y) {
		m_x = x;
		m_y = y;
	}
	void draw() {
		if (false == m_showing) return;
		float rx = (m_x - g_left_x) * TILE_WIDTH + 10;
		float ry = (m_y - g_top_y) * TILE_WIDTH + 10;
		m_sprite.setPosition(rx, ry);
		g_window->draw(m_sprite);
		if (m_mess_end_time < chrono::system_clock::now()) {
			m_name.setPosition(rx - 10, ry - 20);
			g_window->draw(m_name);
		}
		else {
			m_chat.setPosition(rx - 10, ry - 20);
			g_window->draw(m_chat);
		}
	}
	void set_name(const char str[]) {
		m_name.setFont(g_font);
		m_name.setString(str);
		m_name.setFillColor(sf::Color(255, 255, 0));
		m_name.setStyle(sf::Text::Bold);
	}
	void set_chat(const char str[]) {
		m_chat.setFont(g_font);
		m_chat.setString(str);
		m_chat.setFillColor(sf::Color(255, 255, 255));
		m_chat.setStyle(sf::Text::Bold);
		m_mess_end_time = chrono::system_clock::now() + chrono::seconds(3);
	}
};

class MAP_OBJECT {
private:
	bool m_showing;
	sf::Sprite m_sprite;
public:
	int type;
	int m_x, m_y;
	int id;

	MAP_OBJECT(sf::Texture& t, int x, int y, int x2, int y2) {
		m_showing = false;
		m_sprite.setTexture(t);
		m_sprite.setTextureRect(sf::IntRect(x, y, x2, y2));
	}
	MAP_OBJECT() {
		m_showing = false;
	}

	bool GetShowing()
	{
		return m_showing;
	}
	void show()
	{
		m_showing = true;
	}
	void hide()
	{
		m_showing = false;
	}

	void move(int x, int y) {
		m_x = x;
		m_y = y;
	}
	void draw() {
		if (false == m_showing) return;
		float rx = (m_x - g_left_x) * TILE_WIDTH + 10;
		float ry = (m_y - g_top_y) * TILE_WIDTH + 10;
		m_sprite.setPosition(rx, ry);
		g_window->draw(m_sprite);
	}
};

enum MOB_TYPE {
	EMPTY,
	TREE,
	ROCK,
	HOME
};


class MAP {
public:
	MAP_OBJECT trees[TREE_TOTAL];
	MAP_OBJECT rocks[ROCK_TOTAL];

	void load_map() {
		ifstream in{ "map.txt" };

		int tree_cnt = 0;
		int rock_cnt = 0;

		int type;
		int x = 0, y = 0;
		while (in >> type) {
			if (type == MOB_TYPE::TREE){
				trees[tree_cnt].m_x = x;
				trees[tree_cnt].m_y = y;
				trees[tree_cnt].type = type;
				++tree_cnt;
			}
			else if (type == MOB_TYPE::ROCK) {
				rocks[rock_cnt].m_x = x;
				rocks[rock_cnt].m_y = y;
				rocks[rock_cnt].type = type;
				++rock_cnt;
			}
			++x;
			if (x >= WORLD_WIDTH) {
				++y;
				x = 0;
			}
		}
		/*for (auto& t : trees) {
			cout << t.m_x << ", " << t.m_y << endl;
		}*/
	}

	vector<MAP_OBJECT> object_list;
};