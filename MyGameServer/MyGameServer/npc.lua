myid = -1
my_x = 0
my_y = 0
my_type = 0
my_level = 0
my_hp = 0

RIGHT = 2
LEFT = 3
UP = 4
DOWN = 5

function set_id(id)
	myid = id
end

function set_pos()
	my_x = AI_get_x(myid)
	my_y = AI_get_y(myid)
end

function set_info()
	my_level = AI_get_LEVEL(myid)
	my_hp = AI_get_HP(myid)
	my_type = AI_get_type(myid)
end

function damage_to_player(p_id)
	my_x = AI_get_x(myid)
	my_y = AI_get_y(myid)
	p_x = AI_get_x(p_id)
	p_y = AI_get_y(p_id)
	if (p_x == my_x) then
		if (p_y == my_y) then
			AI_damage_to_player(p_id, myid)
		end
	end
end

function move_to_player(m_id, p_id)
	m_x = AI_get_x(m_id)
	m_y = AI_get_y(m_id)
	p_x = AI_get_x(p_id)
	p_y = AI_get_y(p_id)

	if m_x < p_x then
		AI_move(m_id, p_id, RIGHT)
	end
	if p_x < m_x then
		AI_move(m_id, p_id, LEFT)
	end

	if m_y < p_y then
		AI_move(m_id, p_id, DOWN)
	end
	if p_y < m_y then
		AI_move(m_id, p_id, UP)
	end
end