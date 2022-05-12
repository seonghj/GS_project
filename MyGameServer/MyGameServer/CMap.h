#pragma once
#include "stdafx.h"

constexpr int TREE_TOTAL = 100000;
constexpr int ROCK_TOTAL = 50000;
constexpr int home_x = 16;
constexpr int home_y = 15;

enum MOB_TYPE {
	EMPTY,
	TREE,
	ROCK,
	HOME
};

struct MAP_OBJECT {
	int x, y;
	MOB_TYPE type;
};

class MAP {
public:
	bool canmove[WORLD_WIDTH][WORLD_HEIGHT];
	MAP_OBJECT m_object[WORLD_WIDTH][WORLD_HEIGHT];

	void make_map();

	void load_map();

	vector<MAP_OBJECT> object_list;
};

