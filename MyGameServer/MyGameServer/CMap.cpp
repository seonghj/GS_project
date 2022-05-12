#include "CMap.h"
#include <fstream>

void MAP::make_map() {
	
	ofstream out{ "map.txt" };

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(0, WORLD_WIDTH - 1);

	int tree_count = 0;
	int rock_count = 0;

	for (int i = 0; i < WORLD_WIDTH; ++i) {
		for (int j = 0; j < WORLD_HEIGHT; ++j) {
			m_object[i][j].x = i;
			m_object[i][j].y = j;
			canmove[i][j] = true;
			m_object[i][j].type = MOB_TYPE::EMPTY;
		}
	}

	for (int i = home_x - 2; i < home_x+1; ++i) {
		for (int j = home_y - 2; j < home_y+1; ++j) {
			canmove[i][j] = false;
		}
	}
	m_object[home_x - 1][home_y - 1].type = MOB_TYPE::HOME;

	for (int j = 0; j < 3; ++j) {
		for (int i = 0; i < WORLD_WIDTH; ++i) {
			canmove[i][j] = false;
			m_object[i][j].type = MOB_TYPE::TREE;
			++tree_count;
		}
	}

	for (int i = WORLD_WIDTH - 3; i < WORLD_WIDTH; ++i) {
		for (int j = 0; j < WORLD_HEIGHT; ++j) {
			canmove[i][j] = false;
			m_object[i][j].type = MOB_TYPE::TREE;
			++tree_count;
		}
	}

	for (int j = WORLD_HEIGHT - 3; j < WORLD_HEIGHT; ++j) {
		for (int i = 0; i < WORLD_WIDTH; ++i) {
			canmove[i][j] = false;
			m_object[i][j].type = MOB_TYPE::TREE;
			++tree_count;
		}
	}

	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < WORLD_HEIGHT; ++j) {
			canmove[i][j] = false;
			m_object[i][j].type = MOB_TYPE::TREE;
			++tree_count;
		}
	}

	for (int j = 27; j < 30; ++j) {
		for (int i = 0; i < 30; ++i) {
			canmove[i][j] = false;
			m_object[i][j].type = MOB_TYPE::TREE;
			++tree_count;
		}
	}

	for (int i = 27; i < 30; ++i) {
		for (int j = 0; j < 10; ++j) {
			canmove[i][j] = false;
			m_object[i][j].type = MOB_TYPE::TREE;
			++tree_count;
		}
		for (int j = 20; j < 30; ++j) {
			canmove[i][j] = false;
			m_object[i][j].type = MOB_TYPE::TREE;
			++tree_count;
		}
	}

	while (tree_count < TREE_TOTAL) {
		int x = dis(rd);
		int y = dis(rd);
		if (/*m_object[x][y].type != MOB_TYPE::EMPTY || */canmove[x][y] == false)
			continue;
		if (x < 30 && y < 30)
			continue;
		canmove[x][y] = false;
		m_object[x][y].type = MOB_TYPE::TREE;
		++tree_count;
	}

	while (rock_count < ROCK_TOTAL) {
		int x = dis(rd);
		int y = dis(rd);
		if (/*m_object[x][y].type != MOB_TYPE::EMPTY ||*/ canmove[x][y] == false)
			continue;
		if (x < 30 && y < 30)
			continue;
		++rock_count;
		canmove[x][y] = false;
		m_object[x][y].type = MOB_TYPE::ROCK;
	}


	for (int j = 0; j < WORLD_HEIGHT; ++j) {
		for (int i = 0; i < WORLD_WIDTH; ++i) {
			out << m_object[i][j].type<<" ";
		}
		out << endl;
	}
}

void MAP::load_map() {
	ifstream in{ "map.txt" };

	int type;
	int x = 0, y = 0;
	while (in >> type) {
		m_object[x][y].type = (MOB_TYPE)type;
		if (type != MOB_TYPE::EMPTY)
			canmove[x][y] = false;
		else
			canmove[x][y] = true;
		++x;
		if (x >= WORLD_WIDTH) {
			++y;
			x = 0;
		}
	}

	for (int i = home_x - 2; i < home_x + 1; ++i) {
		for (int j = home_y - 2; j < home_y + 1; ++j) {
			canmove[i][j] = false;
		}
	}
}