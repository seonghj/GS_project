#include "stdafx.h"
#include <time.h>

OBJECT avatar;
OBJECT objects[MAX_USER];
MAP_OBJECT home_obj;

MAP* g_pMap = new MAP;

OBJECT tile1;
OBJECT tile2;
OBJECT tile3;
OBJECT tile4;

EFFECT attack_effect;

float message_time = 0;

float move_delay = 0.1f;
float attack_delay = 0.5f;
float message_delay = 5.f;

sf::Texture* block1;
sf::Texture* block2;
sf::Texture* block3;
sf::Texture* block4;
sf::Texture* character;
sf::Texture* monster1;
sf::Texture* monster2;
sf::Texture* tree;
sf::Texture* rock;
sf::Texture* home;

sf::Texture* attack01;

void client_initialize()
{
	block1 = new sf::Texture;
	block2 = new sf::Texture;
	block3 = new sf::Texture;
	block4 = new sf::Texture;
	character = new sf::Texture;
	monster1 = new sf::Texture;
	monster2 = new sf::Texture;
	tree = new sf::Texture;
	rock = new sf::Texture;
	home = new sf::Texture;

	attack01 = new sf::Texture;

	if (false == g_font.loadFromFile("arial.ttf")) {
		cout << "Font Loading Error!\n";
		while (true);
	}

	if (false == g_font2.loadFromFile("KoPubWorld Batang Light.ttf")) {
		cout << "Font Loading Error!\n";
		while (true);
	}

	block1->loadFromFile("image/block1.png");
	block2->loadFromFile("image/block2.png");
	block3->loadFromFile("image/block3.jpg");
	block4->loadFromFile("image/block4.jpg");
	character->loadFromFile("image/char.png");
	monster1->loadFromFile("image/mon1.png");
	monster2->loadFromFile("image/mon2.png");
	tree->loadFromFile("image/tree.png");
	rock->loadFromFile("image/rock.png");
	home->loadFromFile("image/home.png");
	attack01->loadFromFile("image/attack01.png");

	tile1 = OBJECT{ *block1, 0, 0, TILE_WIDTH, TILE_WIDTH };
	tile2 = OBJECT{ *block2, 0, 0, TILE_WIDTH, TILE_WIDTH };
	tile3 = OBJECT{ *block3, 0, 0, TILE_WIDTH, TILE_WIDTH };
	tile4 = OBJECT{ *block4, 0, 0, TILE_WIDTH, TILE_WIDTH };
	
	avatar = OBJECT{ *character, 0, 0, 40, 40 };

	attack_effect = EFFECT{ *attack01, 0, 0, 40, 40 };

	int cnt = 0;
	for (auto& pl : objects) {
		pl.id = cnt;
		if (pl.id >= NPC_ID_START) {
			switch (rand() % 2) {
			case 0:
				pl = OBJECT{ *monster1, 0, 0, 40, 40 };
				break;
			case 1:
				pl = OBJECT{ *monster2, 0, 0, 40, 40 };
				break;
			}
			pl.type = OBJECT_TYPE::monster;
		}
		else {
			pl = OBJECT{ *character, 0, 0, 40, 40 };
			pl.type = OBJECT_TYPE::player;
		}
		++cnt;
	}

	for (auto& obj : g_pMap->trees) {
		obj = MAP_OBJECT{ *tree, 0, 0, 36, 40 };
	}

	for (auto& obj : g_pMap->rocks) {
		obj = MAP_OBJECT{ *rock, 0, 0, 40, 28 };
	}
	home_obj = MAP_OBJECT{ *home, 0, 0, 120, 120 };
	home_obj.m_x = 14;
	home_obj.m_y = 13;
}

void client_finish()
{
	delete block1;
	delete block2;
	delete character;
	delete monster1;
	delete monster2;
	delete tree;
	delete rock;

	delete attack01;
}

void ProcessPacket(char* ptr)
{
	static bool first_time = true;
	switch (ptr[1])
	{
	case SC_LOGIN_OK:
	{
		sc_packet_login_ok* packet = reinterpret_cast<sc_packet_login_ok*>(ptr);
		g_myid = packet->id;
		avatar.id = packet->id;
		avatar.m_x = packet->x;
		avatar.m_y = packet->y;
		g_left_x = packet->x - SCREEN_WIDTH / 2;
		g_top_y = packet->y - SCREEN_HEIGHT / 2;
		avatar.move(packet->x, packet->y);
		avatar.show();
	}
	break;
	case SC_ADD_OBJECT:
	{
		sc_packet_add_object* my_packet = reinterpret_cast<sc_packet_add_object*>(ptr);
		int id = my_packet->id;

		if (id < MAX_USER) {
			objects[id].id = my_packet->id;
			if (id <= NPC_ID_START)
				objects[id].set_name(my_packet->name);
			objects[id].move(my_packet->x, my_packet->y);
			objects[id].show();
		}
		break;
	}
	case SC_POSITION:
	{
		sc_packet_position* my_packet = reinterpret_cast<sc_packet_position*>(ptr);
		int id = my_packet->id;
		if (id == g_myid) {
			avatar.move(my_packet->x, my_packet->y);
			g_left_x = my_packet->x - SCREEN_WIDTH / 2;
			g_top_y = my_packet->y - SCREEN_HEIGHT / 2;
			// 장애물 그리기
			for (auto& obj : g_pMap->trees) {
				if ((avatar.m_x - 11 < obj.m_x && obj.m_x < avatar.m_x + 10)
					&& (avatar.m_y - 11 < obj.m_y && obj.m_y < avatar.m_y + 10)) {
					if (obj.GetShowing() == false) {
						obj.show();
						obj.draw();
					}
				}
				else {
					if (obj.GetShowing() == true)
						obj.hide();
				}
			}
			for (auto& obj : g_pMap->rocks) {
				if ((avatar.m_x - 11 < obj.m_x && obj.m_x < avatar.m_x + 10)
					&& (avatar.m_y - 11 < obj.m_y && obj.m_y < avatar.m_y + 10)) {
					if (obj.GetShowing() == false) {
						obj.show();
						obj.draw();
					}
				}
				else {
					if (obj.GetShowing() == true)
						obj.hide();
				}
			}

			if ((avatar.m_x - 11 < home_obj.m_x && home_obj.m_x < avatar.m_x + 10)
				&& (avatar.m_y - 11 < home_obj.m_y && home_obj.m_y < avatar.m_y + 10)) {
				if (home_obj.GetShowing() == false) {
					home_obj.show();
					home_obj.draw();
				}
			}
			else {
				if (home_obj.GetShowing() == true)
					home_obj.hide();
			}

		}
		else if (id < MAX_USER) {
			objects[id].move(my_packet->x, my_packet->y);
		}
		else {
			//npc[other_id - NPC_START].x = my_packet->x;
			//npc[other_id - NPC_START].y = my_packet->y;
		}

		break;
	}

	case SC_REMOVE_OBJECT:
	{
		sc_packet_remove_object* my_packet = reinterpret_cast<sc_packet_remove_object*>(ptr);
		int id = my_packet->id;
		if (id == g_myid) {
			avatar.hide();
		}
		else if (id < MAX_USER) {
			objects[id].hide();
		}
		else {
			//		npc[other_id - NPC_START].attr &= ~BOB_ATTR_VISIBLE;
		}
		break;
	}
	case SC_CHAT:
	{
		sc_packet_chat* my_packet = reinterpret_cast<sc_packet_chat*>(ptr);
		int id = my_packet->id;
		if (id == g_myid) {
			avatar.set_chat(my_packet->message);
		}
		else if ( 0 < id && id < MAX_USER) {
			objects[id].set_chat(my_packet->message);
		}
		else if (id == battle_mass_ID) {
			sprintf_s(battle_mass_buf, my_packet->message);
			message_time = 0;
		}
		break;
	}
	case SC_STAT_CHANGE:
	{
		sc_packet_stat_change* my_packet = reinterpret_cast<sc_packet_stat_change*>(ptr);
		int id = my_packet->id;
		if (my_packet->id == g_myid) {
			avatar.HP = my_packet->HP;
			avatar.EXP = my_packet->EXP;
			avatar.LEVEL = my_packet->LEVEL;
		}
		else {
			objects[id].HP = my_packet->HP;
			objects[id].EXP = my_packet->EXP;
			objects[id].LEVEL = my_packet->LEVEL;
		}
		break;
	}
	default:
		printf("Unknown PACKET type [%d]\n", ptr[1]);
	}
}

void process_data(char* net_buf, size_t io_byte)
{
	char* ptr = net_buf;
	static size_t in_packet_size = 0;
	static size_t saved_packet_size = 0;
	static char packet_buffer[BUF_SIZE];

	while (0 != io_byte) {
		if (0 == in_packet_size) in_packet_size = ptr[0];
		if (io_byte + saved_packet_size >= in_packet_size) {
			memcpy(packet_buffer + saved_packet_size, ptr, in_packet_size - saved_packet_size);
			ProcessPacket(packet_buffer);
			ptr += in_packet_size - saved_packet_size;
			io_byte -= in_packet_size - saved_packet_size;
			in_packet_size = 0;
			saved_packet_size = 0;
		}
		else {
			memcpy(packet_buffer + saved_packet_size, ptr, io_byte);
			saved_packet_size += io_byte;
			io_byte = 0;
		}
	}
}

void UI() 
{
	sf::Text position;
	sf::Text HP_info;
	sf::Text EXP_info;
	sf::Text LEVEL_info;

	sf::Text Global_message;

	position.setFont(g_font);
	HP_info.setFont(g_font);
	EXP_info.setFont(g_font);
	LEVEL_info.setFont(g_font);
	Global_message.setFont(g_font);

	char position_buf[100];
	sprintf_s(position_buf, "POS %d, %d", avatar.m_x, avatar.m_y);
	position.setString(position_buf);
	position.setCharacterSize(20);
	position.setFillColor(sf::Color::White);

	char HP_buf[100];
	sprintf_s(HP_buf, "HP: %d", avatar.HP);
	HP_info.setString(HP_buf);
	HP_info.setPosition(20, 650);
	HP_info.setFillColor(sf::Color::Red);
	HP_info.setStyle(sf::Text::Bold);

	char EXP_buf[100];
	sprintf_s(EXP_buf, "EXP: %d", avatar.EXP);
	EXP_info.setString(EXP_buf);
	EXP_info.setPosition(HP_info.getPosition().x, HP_info.getPosition().y + 50);
	EXP_info.setFillColor(sf::Color::Black);
	EXP_info.setStyle(sf::Text::Bold);

	char LEVEL_buf[100];
	sprintf_s(LEVEL_buf, "LEVEL: %d", avatar.LEVEL);
	LEVEL_info.setString(LEVEL_buf);
	LEVEL_info.setPosition(EXP_info.getPosition().x, EXP_info.getPosition().y + 50);
	LEVEL_info.setFillColor(sf::Color::Blue);
	LEVEL_info.setStyle(sf::Text::Bold);

	Global_message.setString(battle_mass_buf);
	Global_message.setPosition(200, 150);
	Global_message.setCharacterSize(20);
	Global_message.setFillColor(sf::Color::Black);
	Global_message.setStyle(sf::Text::Bold);


	g_window->draw(position);
	g_window->draw(HP_info);
	g_window->draw(EXP_info);
	g_window->draw(LEVEL_info);
	if (message_time < message_delay) {
		g_window->draw(Global_message);
	}
}

void client_main()
{
	char net_buf[BUF_SIZE];
	size_t	received;

	auto recv_result = socket.receive(net_buf, BUF_SIZE, received);
	if (recv_result == sf::Socket::Error)
	{
		wcout << L"Recv 에러!";
		while (true);
	}
	if (recv_result != sf::Socket::NotReady)
		if (received > 0) process_data(net_buf, received);

	for (int i = 0; i < SCREEN_WIDTH; ++i)
		for (int j = 0; j < SCREEN_HEIGHT; ++j)
		{
			int x = i + g_left_x;
			int y = j + g_top_y;
			if (x < 30 && y < 30) {
				if ((x < 0) || (y < 0)) continue;
				if ((((x / 3) + (y / 3)) % 2) == 1) {
					tile3.a_move(TILE_WIDTH * i + 7, TILE_WIDTH * j + 7);
					tile3.a_draw();
				}
				else
				{
					tile4.a_move(TILE_WIDTH * i + 7, TILE_WIDTH * j + 7);
					tile4.a_draw();
				}
			}
			else {
				if ((x < 0) || (y < 0)) continue;
				if ((((x / 3) + (y / 3)) % 2) == 1) {
					tile1.a_move(TILE_WIDTH * i + 7, TILE_WIDTH * j + 7);
					tile1.a_draw();
				}
				else
				{
					tile2.a_move(TILE_WIDTH * i + 7, TILE_WIDTH * j + 7);
					tile2.a_draw();
				}
			}
		}
	avatar.draw();
	for (auto& pl : objects) pl.draw();
	for (auto& obj : g_pMap->trees) obj.draw();
	for (auto& obj : g_pMap->rocks) obj.draw();
	home_obj.draw();
	attack_effect.draw();

	UI();
}

void send_move_packet(MOVE_DIR dr)
{
	cs_packet_move packet;
	packet.size = sizeof(packet);
	packet.type = CS_MOVE;
	packet.direction = dr;
	size_t sent = 0;
	socket.send(&packet, sizeof(packet), sent);
}

void send_login_packet(string& name)
{
	cs_packet_login packet;
	packet.size = sizeof(packet);
	packet.type = CS_LOGIN;
	strcpy_s(packet.player_id, name.c_str());
	size_t sent = 0;
	socket.send(&packet, sizeof(packet), sent);
}

void send_attack_packet()
{
	cs_packet_attack packet;
	packet.size = sizeof(cs_packet_attack);
	packet.type = CS_ATTACK;
	packet.id = g_myid;
	size_t sent = 0;
	socket.send(&packet, sizeof(packet), sent);
}

int main()
{
	wcout.imbue(locale("korean"));

	string IP{};
	string name{};

	sf::Socket::Status status = sf::Socket::Error;

	while (status == sf::Socket::Error) {
		cout << "IP insert: ";
		cin >> IP;
		cout << "ID insert: ";
		cin >> name;
		//sf::Socket::Status status = socket.connect("127.0.0.1", SERVER_PORT);
		status = socket.connect(IP, SERVER_PORT);
		if (status != sf::Socket::Done) {
			wcout << L"Server connect FAIL\n";
		}
	}

	socket.setBlocking(false);

	client_initialize();
	int tt = chrono::duration_cast<chrono::milliseconds>
		(chrono::system_clock::now().
			time_since_epoch()).count();
	name += to_string(tt % 1000);
	send_login_packet(name);
	//avatar.set_name(name.c_str());
	sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "2016180045");
	g_window = &window;

	g_pMap->load_map();

	float move_timer = 0;
	float attack_timer = 0;

	sprintf_s(battle_mass_buf, " ");

	sf::Clock clock;

	while (window.isOpen())
	{
		float time = clock.getElapsedTime().asSeconds();
		clock.restart();
		move_timer += time;
		attack_timer += time;
		message_time += time;

		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
			if (event.type == sf::Event::KeyPressed) {
				MOVE_DIR p_type = MOVE_DIR::NO;
				switch (event.key.code) {
				case sf::Keyboard::Left:
					p_type = MOVE_DIR::LEFT;
					if (move_timer > move_delay) {
						move_timer = 0;
						send_move_packet(p_type);
					}
					break;
				case sf::Keyboard::Right:
					p_type = MOVE_DIR::RIGHT;
					if (move_timer > move_delay) {
						move_timer = 0;
						send_move_packet(p_type);
					}
					break;
				case sf::Keyboard::Up:
					p_type = MOVE_DIR::UP;
					if (move_timer > move_delay) {
						move_timer = 0;
						send_move_packet(p_type);
					}
					break;
				case sf::Keyboard::Down:
					p_type = MOVE_DIR::DOWN;
					if (move_timer > move_delay) {
						move_timer = 0;
						send_move_packet(p_type);
					}
					break;
				case sf::Keyboard::Escape:
					window.close();
					break;
				case sf::Keyboard::A:
					if (attack_timer > attack_delay) {
						attack_timer = 0;
						send_attack_packet();
						attack_effect.show(avatar.m_x, avatar.m_y);
					}
					break;
				}
				//if (MOVE_DIR::NO != p_type && MOVE_DIR::RANDOM != p_type /*&& move_delay > 10*/) send_move_packet(p_type);
			}
		}
		if (attack_timer > attack_delay / 2)
			attack_effect.hide();

		window.clear();
		client_main();
		window.display();
	}
	client_finish();

	return 0;
}