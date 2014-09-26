/*
Minetest-c55
Copyright (C) 2010-2011 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "content_sao.h"
#include "content_mob.h"
#include "collision.h"
#include "environment.h"
#include "settings.h"
#include "profiler.h"

core::map<u16, ServerActiveObject::Factory> ServerActiveObject::m_types;

/* Some helper functions */

// Y is copied, X and Z change is limited
void accelerate_xz(v3f &speed, v3f target_speed, f32 max_increase)
{
	v3f d_wanted = target_speed - speed;
	d_wanted.Y = 0;
	f32 dl_wanted = d_wanted.getLength();
	f32 dl = dl_wanted;
	if(dl > max_increase)
		dl = max_increase;

	v3f d = d_wanted.normalize() * dl;

	speed.X += d.X;
	speed.Z += d.Z;
	speed.Y = target_speed.Y;
}


/*
	ItemSAO
*/

// Prototype
ItemSAO proto_ItemSAO(NULL, 0, v3f(0,0,0), "");

ItemSAO::ItemSAO(ServerEnvironment *env, u16 id, v3f pos,
		const std::string inventorystring):
	ServerActiveObject(env, id, pos),
	m_inventorystring(inventorystring),
	m_speed_f(0,0,0),
	m_last_sent_position(0,0,0)
{
	ServerActiveObject::registerType(getType(), create);
}

ServerActiveObject* ItemSAO::create(ServerEnvironment *env, u16 id, v3f pos,
		const std::string &data)
{
	std::istringstream is(data, std::ios::binary);
	char buf[1];
	// read version
	is.read(buf, 1);
	u8 version = buf[0];
	// check if version is supported
	if(version != 0)
		return NULL;
	std::string inventorystring = deSerializeString(is);
	infostream<<"ItemSAO::create(): Creating item \""
			<<inventorystring<<"\""<<std::endl;
	return new ItemSAO(env, id, pos, inventorystring);
}

void ItemSAO::step(float dtime, bool send_recommended)
{
	ScopeProfiler sp2(g_profiler, "ItemSAO::step avg", SPT_AVG);

	assert(m_env);

	const float interval = 0.2;
	if(m_move_interval.step(dtime, interval)==false)
		return;
	dtime = interval;

	core::aabbox3d<f32> box(-BS/3.,0.0,-BS/3., BS/3.,BS*2./3.,BS/3.);
	collisionMoveResult moveresult;
	// Apply gravity
	m_speed_f += v3f(0, -dtime*9.81*BS, 0);
	// Maximum movement without glitches
	f32 pos_max_d = BS*0.25;
	// Limit speed
	if(m_speed_f.getLength()*dtime > pos_max_d)
		m_speed_f *= pos_max_d / (m_speed_f.getLength()*dtime);
	v3f pos_f = getBasePosition();
	v3f pos_f_old = pos_f;
	v3f accel_f = v3f(0,0,0);
	moveresult = collisionMoveSimple(&m_env->getMap(), pos_max_d,
			box, 0.0, dtime, pos_f, m_speed_f, accel_f);

	if(send_recommended == false)
		return;

	if(pos_f.getDistanceFrom(m_last_sent_position) > 0.05*BS)
	{
		setBasePosition(pos_f);
		m_last_sent_position = pos_f;

		std::ostringstream os(std::ios::binary);
		char buf[6];
		// command (0 = update position)
		buf[0] = 0;
		os.write(buf, 1);
		// pos
		writeS32((u8*)buf, m_base_position.X*1000);
		os.write(buf, 4);
		writeS32((u8*)buf, m_base_position.Y*1000);
		os.write(buf, 4);
		writeS32((u8*)buf, m_base_position.Z*1000);
		os.write(buf, 4);
		// create message and add to list
		ActiveObjectMessage aom(getId(), false, os.str());
		m_messages_out.push_back(aom);
	}
}

std::string ItemSAO::getClientInitializationData()
{
	std::ostringstream os(std::ios::binary);
	char buf[6];
	// version
	buf[0] = 0;
	os.write(buf, 1);
	// pos
	writeS32((u8*)buf, m_base_position.X*1000);
	os.write(buf, 4);
	writeS32((u8*)buf, m_base_position.Y*1000);
	os.write(buf, 4);
	writeS32((u8*)buf, m_base_position.Z*1000);
	os.write(buf, 4);
	// inventorystring
	os<<serializeString(m_inventorystring);
	return os.str();
}

std::string ItemSAO::getStaticData()
{
	infostream<<__FUNCTION_NAME<<std::endl;
	std::ostringstream os(std::ios::binary);
	char buf[1];
	// version
	buf[0] = 0;
	os.write(buf, 1);
	// inventorystring
	os<<serializeString(m_inventorystring);
	return os.str();
}

InventoryItem * ItemSAO::createInventoryItem()
{
	try{
		std::istringstream is(m_inventorystring, std::ios_base::binary);
		InventoryItem *item = InventoryItem::deSerialize(is);
		infostream<<__FUNCTION_NAME<<": m_inventorystring=\""
				<<m_inventorystring<<"\" -> item="<<item
				<<std::endl;
		return item;
	}
	catch(SerializationError &e)
	{
		infostream<<__FUNCTION_NAME<<": serialization error: "
				<<"m_inventorystring=\""<<m_inventorystring<<"\""<<std::endl;
		return NULL;
	}
}

void ItemSAO::rightClick(Player *player)
{
	infostream<<__FUNCTION_NAME<<std::endl;
	InventoryItem *item = createInventoryItem();
	if(item == NULL)
		return;

	bool to_be_deleted = item->use(m_env, player);

	if(to_be_deleted)
		m_removed = true;
	else
		// Reflect changes to the item here
		m_inventorystring = item->getItemString();

	delete item;
}

static void get_random_u32_array(u32 a[], u32 len)
{
	u32 i, n;
	for(i=0; i<len; i++)
		a[i] = i;
	n = len;
	while(n > 1){
		u32 k = myrand() % n;
		n--;
		u32 temp = a[n];
		a[n] = a[k];
		a[k] = temp;
	}
}

#define PP(x) "("<<(x).X<<","<<(x).Y<<","<<(x).Z<<")"



/*
	MobSAO
*/

// Prototype
MobSAO proto_MobSAO(NULL, 0, v3f(0,0,0), CONTENT_IGNORE);

MobSAO::MobSAO(ServerEnvironment *env, u16 id, v3f pos, content_t type):
	ServerActiveObject(env, id, pos),
	m_content(type),
	m_speed(0,0,0),
	m_last_sent_position(0,0,0),
	m_oldpos(0,0,0),
	m_initial_pos(pos),
	m_yaw(0),
	m_touching_ground(false),
	m_falling(false),
	m_next_pos_exists(false),
	m_age(0),
	m_hp(10),
	m_disturb_timer(100000),
	m_random_disturb_timer(0),
	m_walk_around(false),
	m_walk_around_timer(0),
	m_shoot_reload_timer(0),
	m_shooting(false),
	m_shooting_timer(0),
	m_shoot_y(0)
{
	ServerActiveObject::registerType(getType(), create);
}
MobSAO::MobSAO(ServerEnvironment *env, u16 id, v3f pos, v3f speed, content_t type):
	ServerActiveObject(env, id, pos),
	m_content(type),
	m_speed(speed),
	m_last_sent_position(0,0,0),
	m_oldpos(0,0,0),
	m_initial_pos(pos),
	m_yaw(0),
	m_touching_ground(false),
	m_falling(false),
	m_next_pos_exists(false),
	m_age(0),
	m_hp(10),
	m_disturb_timer(100000),
	m_random_disturb_timer(0),
	m_walk_around(false),
	m_walk_around_timer(0),
	m_shoot_reload_timer(0),
	m_shooting(false),
	m_shooting_timer(0),
	m_shoot_y(0)
{
	ServerActiveObject::registerType(getType(), create);
}
MobSAO::~MobSAO()
{
}
ServerActiveObject* MobSAO::create(ServerEnvironment *env, u16 id, v3f pos, const std::string &data)
{
	std::istringstream is(data, std::ios::binary);
	char buf[1];
	// read version
	is.read(buf, 1);
	u8 version = buf[0];
	// check if version is supported
	if (version != 0)
		return NULL;
	v3f p = readV3F1000(is);
	content_t c = readU16(is);
	c = content_mob_features(c).content;
	if (c == CONTENT_IGNORE)
		return NULL;
	MobSAO *o = new MobSAO(env,id,pos,c);
	o->m_base_position = p;
	o->m_yaw = readF1000(is);
	o->m_speed = readV3F1000(is);
	o->m_age = readF1000(is);
	o->m_hp = readU8(is);
	return o;
}
std::string MobSAO::getStaticData()
{
	std::ostringstream os(std::ios::binary);
	// version
	writeU8(os, 0);
	// pos
	writeV3F1000(os, m_base_position);
	// content
	writeU16(os,m_content);
	// yaw
	writeF1000(os,m_yaw);
	// speed
	writeV3F1000(os, m_speed);
	// age
	writeF1000(os,m_age);
	// hp
	writeU8(os,m_hp);
	// shooting
	writeU8(os,(u8)m_shooting);
	return os.str();
}
std::string MobSAO::getClientInitializationData()
{
	return getStaticData();
}
void MobSAO::step(float dtime, bool send_recommended)
{
	MobFeatures m = content_mob_features(m_content);
	Player *disturbing_player = NULL;
	v3f disturbing_player_off = v3f(0,1,0);
	v3f disturbing_player_norm = v3f(0,1,0);
	float disturbing_player_distance = 1000000;
	float disturbing_player_dir = 0;

	m_age += dtime;

	if (m.lifetime > 0.0 && m_age >= m.lifetime) {
		m_removed = true;
		return;
	}

	m_random_disturb_timer += dtime;
	if (m.notices_player) {
		if (m_random_disturb_timer >= 5.0) {
			m_random_disturb_timer = 0;
			m_disturbing_player = "";
			// Check connected players
			core::list<Player*> players = m_env->getPlayers(true);
			for (core::list<Player*>::Iterator i = players.begin(); i != players.end(); i++) {
				Player *player = *i;
				v3f playerpos = player->getPosition();
				f32 dist = m_base_position.getDistanceFrom(playerpos);
				if (dist < BS*16) {
					if (myrand_range(0,3) == 0) {
						actionstream<<"Mob id="<<m_id<<" at "
								<<PP(m_base_position/BS)
								<<" got randomly disturbed by "
								<<player->getName()<<std::endl;
						m_disturbing_player = player->getName();
						m_disturb_timer = 0;
						break;
					}
				}
			}
		}
		if (m_disturbing_player != "") {
			disturbing_player = m_env->getPlayer(m_disturbing_player.c_str());
			if (disturbing_player) {
				disturbing_player_off = disturbing_player->getPosition() - m_base_position;
				disturbing_player_distance = disturbing_player_off.getLength();
				disturbing_player_norm = disturbing_player_off;
				disturbing_player_norm.normalize();
				disturbing_player_dir = 180./PI*atan2(disturbing_player_norm.Z,disturbing_player_norm.X);
			}
		}
		m_disturb_timer += dtime;

		if (!m_falling && m.attack_throw_object != CONTENT_IGNORE) {
			m_shooting_timer -= dtime;
			if (m_shooting_timer <= 0.0 && m_shooting) {
				m_shooting = false;
				v3f shoot_pos = m.attack_throw_offset * BS;
				if (0) {
					v3f dir(cos(m_yaw/180*PI),0,sin(m_yaw/180*PI));
					dir.Y = m_shoot_y;
					dir.normalize();
					v3f speed = dir * BS * 10.0;
					v3f pos = m_base_position + shoot_pos;
					infostream<<__FUNCTION_NAME<<": Mob id="<<m_id
							<<" shooting from "<<PP(pos)
							<<" at speed "<<PP(speed)<<std::endl;
					ServerActiveObject *obj = new MobSAO(m_env, 0, pos, speed, m.attack_throw_object);
					m_env->addActiveObject(obj);
				}
			}

			m_shoot_reload_timer += dtime;

			float reload_time = 15.0;
			if (m_disturb_timer <= 15.0)
				reload_time = 3.0;

			if (
				!m_shooting
				&& m_shoot_reload_timer >= reload_time
				&& !m_next_pos_exists
				&& m_disturb_timer <= 60.0
			) {
				m_shoot_y = 0;
				if (
					m_disturb_timer < 60.0
					&& disturbing_player
					&& disturbing_player_distance < 16*BS
					&& fabs(disturbing_player_norm.Y) < 0.8
				) {
					m_yaw = disturbing_player_dir;
					sendPosition();
					m_shoot_y += disturbing_player_norm.Y;
				}else{
					m_shoot_y = 0.01 * myrand_range(-30,10);
				}
				m_shoot_reload_timer = 0.0;
				m_shooting = true;
				m_shooting_timer = 1.5;
				{
					std::ostringstream os(std::ios::binary);
					// command (2 = shooting)
					writeU8(os, 2);
					// time
					writeF1000(os, m_shooting_timer + 0.1);
					// create message and add to list
					ActiveObjectMessage aom(getId(), false, os.str());
					m_messages_out.push_back(aom);
				}
			}
		}
	}

	if (m.motion != MM_CONSTANT && m.motion != MM_STATIC && !m_shooting) {
		m_walk_around_timer -= dtime;
		if (m_walk_around_timer <= 0.0) {
			m_walk_around = !m_walk_around;
			m_walk_around_timer = 1.0;
			//if (m_walk_around) {
				//m_walk_around_timer = 0.1*myrand_range(10,50);
			//}else{
				//m_walk_around_timer = 0.1*myrand_range(30,70);
			//}
		}
		if (m_next_pos_exists) {
			v3f pos_f = m_base_position;
			v3f next_pos_f = intToFloat(m_next_pos_i, BS);

			v3f v = next_pos_f - pos_f;
			m_yaw = atan2(v.Z, v.X) / PI * 180;

			v3f diff = next_pos_f - pos_f;
			v3f dir = diff;
			dir.normalize();
			float speed = BS * 0.5;
			if (m_falling)
				speed = BS * 3.0;
			dir *= dtime * speed;
			bool arrived = false;
			if (dir.getLength() > diff.getLength()) {
				dir = diff;
				arrived = true;
			}
			pos_f += dir;
			m_base_position = pos_f;

			if ((pos_f - next_pos_f).getLength() < 0.1 || arrived)
				m_next_pos_exists = false;
		}
	}

	if (m.motion == MM_WANDER) {
		stepMotionWander(dtime);
	}else if (m.motion == MM_SEEKER) {
		if (disturbing_player) {
			stepMotionWander(dtime);
		}else{
			stepMotionSeeker(dtime);
		}
	}else if (m.motion == MM_SENTRY) {
		stepMotionSentry(dtime);
	}else if (m.motion == MM_THROWN) {
		stepMotionThrown(dtime);
	}else if (m.motion == MM_CONSTANT) {
		stepMotionConstant(dtime);
	}

	if (send_recommended == false)
		return;

	if (m_base_position.getDistanceFrom(m_last_sent_position) > 0.05*BS)
		sendPosition();
}
void MobSAO::stepMotionWander(float dtime)
{
	MobFeatures m = content_mob_features(m_content);
	v3s16 pos_i = floatToInt(m_base_position, BS);
	v3s16 pos_size_off(0,0,0);
	v3f s = m.getSize();
	if (m.getSize().X >= 2.5) {
		pos_size_off.X = -1;
		pos_size_off.Y = -1;
	}

	if (m.motion_type == MMT_WALK) {
		if (!m_next_pos_exists) {
			/* Check whether to drop down */
			if (checkFreePosition(pos_i + pos_size_off + v3s16(0,-1,0))) {
				m_next_pos_i = pos_i + v3s16(0,-1,0);
				m_next_pos_exists = true;
				m_falling = true;
			}else{
				m_falling = false;
			}
		}

		if (m_walk_around && !m_next_pos_exists) {
			/* Find some position where to go next */
			v3s16 dps[3*3*3];
			int num_dps = 0;
			for (int dx=-1; dx<=1; dx++)
			for (int dy=-1; dy<=1; dy++)
			for (int dz=-1; dz<=1; dz++) {
				if (dx == 0 && dy == 0)
					continue;
				if (dx != 0 && dz != 0 && dy != 0)
					continue;
				dps[num_dps++] = v3s16(dx,dy,dz);
			}
			u32 order[3*3*3];
			get_random_u32_array(order, num_dps);
			for (int i=0; i<num_dps; i++) {
				v3s16 p = dps[order[i]] + pos_i;
				if (!checkFreeAndWalkablePosition(p + pos_size_off))
					continue;
				m_next_pos_i = p;
				m_next_pos_exists = true;
				break;
			}
		}
	}else if (m.motion_type == MMT_FLY) {
		bool falling = false;
		bool raising = false;
		if (!m_next_pos_exists) {
			u16 above;
			v3s16 p = pos_i + pos_size_off;
			for (above=0; above < 14; above++) {
				p.Y--;
				if (!checkFreePosition(p))
					break;
			}
			if (above > 12) {
				/* Check whether to drop down */
				if (checkFreePosition(pos_i + pos_size_off + v3s16(0,-1,0))) {
					m_next_pos_i = pos_i + v3s16(0,-1,0);
					falling = true;
				}
			}else if (above < 8) {
				/* Check whether to rise up */
				if (checkFreePosition(pos_i + pos_size_off + v3s16(0,1,0))) {
					m_next_pos_i = pos_i + v3s16(0,1,0);
					raising = true;
				}
			}
		}

		if (m_walk_around && !m_next_pos_exists) {
			/* Find some position where to go next */
			v3s16 dps[3*3*3];
			int num_dps = 0;
			for (int dx=-1; dx<=1; dx++)
			for (int dy=-1; dy<=1; dy++)
			for (int dz=-1; dz<=1; dz++) {
				if (dx == 0 && dy == 0)
					continue;
				if (dx != 0 && dz != 0 && dy != 0)
					continue;
				if (falling && dy > 0)
					continue;
				if (raising && dy < 0)
					continue;
				dps[num_dps++] = v3s16(dx,dy,dz);
			}
			u32 order[3*3*3];
			get_random_u32_array(order, num_dps);
			for (int i=0; i<num_dps; i++) {
				v3s16 p = dps[order[i]] + pos_i;
				if (!checkFreePosition(p + pos_size_off))
					continue;
				m_next_pos_i = p;
				m_next_pos_exists = true;
				break;
			}
		}
	}else if (m.motion_type == MMT_FLYLOW || m.motion_type == MMT_SWIM) {
		bool falling = false;
		if (!m_next_pos_exists) {
			/* Check whether to drop down */
			if (checkFreePosition(pos_i + pos_size_off + v3s16(0,-1,0))) {
				m_next_pos_i = pos_i + v3s16(0,-1,0);
				falling = true;
			}
		}

		if (m_walk_around && !m_next_pos_exists) {
			/* Find some position where to go next */
			v3s16 dps[3*3*3];
			int num_dps = 0;
			for (int dx=-1; dx<=1; dx++)
			for (int dy=-1; dy<=1; dy++)
			for (int dz=-1; dz<=1; dz++) {
				if (dx == 0 && dy == 0)
					continue;
				if (dx != 0 && dz != 0 && dy != 0)
					continue;
				if (falling && dy > 0)
					continue;
				dps[num_dps++] = v3s16(dx,dy,dz);
			}
			u32 order[3*3*3];
			get_random_u32_array(order, num_dps);
			for (int i=0; i<num_dps; i++) {
				v3s16 p = dps[order[i]] + pos_i;
				if (!checkFreePosition(p + pos_size_off))
					continue;
				m_next_pos_i = p;
				m_next_pos_exists = true;
				break;
			}
		}
	}
}
void MobSAO::stepMotionSeeker(float dtime)
{
	MobFeatures m = content_mob_features(m_content);
	v3s16 pos_i = floatToInt(m_base_position, BS);
	v3s16 pos_size_off(0,0,0);
	if (m.getSize().X >= 2.5) {
		pos_size_off.X = -1;
		pos_size_off.Y = -1;
	}
	Player *disturbing_player = m_env->getPlayer(m_disturbing_player.c_str());
	if (!disturbing_player) {
		m_next_pos_exists = false;
		return;
	}
	v3f player_pos = disturbing_player->getPosition();
	float distance = m_base_position.getDistanceFrom(player_pos);

	if (m.motion_type == MMT_WALK) {
		if (!m_next_pos_exists) {
			/* Check whether to drop down */
			if (checkFreePosition(pos_i + pos_size_off + v3s16(0,-1,0))) {
				m_next_pos_i = pos_i + v3s16(0,-1,0);
				m_next_pos_exists = true;
				m_falling = true;
			}else{
				m_falling = false;
			}
		}

		if (m_walk_around && !m_next_pos_exists) {
			/* Find some position where to go next */
			v3s16 dps[3*3*3];
			int num_dps = 0;
			for (int dx=-1; dx<=1; dx++)
			for (int dy=-1; dy<=1; dy++)
			for (int dz=-1; dz<=1; dz++) {
				if (dx == 0 && dy == 0)
					continue;
				if (dx != 0 && dz != 0 && dy != 0)
					continue;
				if ((m_base_position+intToFloat(v3s16(dx,dy,dz),BS)).getDistanceFrom(player_pos) > distance)
					continue;
				dps[num_dps++] = v3s16(dx,dy,dz);
			}
			u32 order[3*3*3];
			get_random_u32_array(order, num_dps);
			for (int i=0; i<num_dps; i++) {
				v3s16 p = dps[order[i]] + pos_i;
				if (!checkFreeAndWalkablePosition(p + pos_size_off))
					continue;
				m_next_pos_i = p;
				m_next_pos_exists = true;
				break;
			}
		}
	}else if (m.motion_type == MMT_FLY) {
		bool falling = false;
		bool raising = false;
		if (!m_next_pos_exists) {
			u16 above;
			v3s16 p = pos_i + pos_size_off;
			for (above=0; above < 14; above++) {
				p.Y--;
				if (!checkFreePosition(p))
					break;
			}
			if (above > 12) {
				/* Check whether to drop down */
				if (checkFreePosition(pos_i + pos_size_off + v3s16(0,-1,0))) {
					m_next_pos_i = pos_i + v3s16(0,-1,0);
					falling = true;
				}
			}else if (above < 8) {
				/* Check whether to rise up */
				if (checkFreePosition(pos_i + pos_size_off + v3s16(0,1,0))) {
					m_next_pos_i = pos_i + v3s16(0,1,0);
					raising = true;
				}
			}
		}

		if (m_walk_around && !m_next_pos_exists) {
			/* Find some position where to go next */
			v3s16 dps[3*3*3];
			int num_dps = 0;
			for (int dx=-1; dx<=1; dx++)
			for (int dy=-1; dy<=1; dy++)
			for (int dz=-1; dz<=1; dz++) {
				if (dx == 0 && dy == 0)
					continue;
				if (dx != 0 && dz != 0 && dy != 0)
					continue;
				if (falling && dy > 0)
					continue;
				if (raising && dy < 0)
					continue;
				if ((m_base_position+intToFloat(v3s16(dx,dy,dz),BS)).getDistanceFrom(player_pos) > distance)
					continue;
				dps[num_dps++] = v3s16(dx,dy,dz);
			}
			u32 order[3*3*3];
			get_random_u32_array(order, num_dps);
			for (int i=0; i<num_dps; i++) {
				v3s16 p = dps[order[i]] + pos_i;
				if (!checkFreePosition(p + pos_size_off))
					continue;
				m_next_pos_i = p;
				m_next_pos_exists = true;
				break;
			}
		}
	}else if (m.motion_type == MMT_FLYLOW || m.motion_type == MMT_SWIM) {
		bool falling = false;
		if (!m_next_pos_exists) {
			/* Check whether to drop down */
			if (checkFreePosition(pos_i + pos_size_off + v3s16(0,-1,0))) {
				m_next_pos_i = pos_i + v3s16(0,-1,0);
				falling = true;
			}
		}

		if (m_walk_around && !m_next_pos_exists) {
			/* Find some position where to go next */
			v3s16 dps[3*3*3];
			int num_dps = 0;
			for (int dx=-1; dx<=1; dx++)
			for (int dy=-1; dy<=1; dy++)
			for (int dz=-1; dz<=1; dz++) {
				if (dx == 0 && dy == 0)
					continue;
				if (dx != 0 && dz != 0 && dy != 0)
					continue;
				if (falling && dy > 0)
					continue;
				if ((m_base_position+intToFloat(v3s16(dx,dy,dz),BS)).getDistanceFrom(player_pos) > distance)
					continue;
				dps[num_dps++] = v3s16(dx,dy,dz);
			}
			u32 order[3*3*3];
			get_random_u32_array(order, num_dps);
			for (int i=0; i<num_dps; i++) {
				v3s16 p = dps[order[i]] + pos_i;
				if (!checkFreePosition(p + pos_size_off))
					continue;
				m_next_pos_i = p;
				m_next_pos_exists = true;
				break;
			}
		}
	}
}
void MobSAO::stepMotionSentry(float dtime)
{
	MobFeatures m = content_mob_features(m_content);
	v3s16 pos_i = floatToInt(m_base_position, BS);
	v3s16 pos_size_off(0,0,0);
	if (m.getSize().X >= 2.5) {
		pos_size_off.X = -1;
		pos_size_off.Y = -1;
	}

	if (m.motion_type == MMT_WALK) {
		if (!m_next_pos_exists) {
			/* Check whether to drop down */
			if (checkFreePosition(pos_i + pos_size_off + v3s16(0,-1,0))) {
				m_next_pos_i = pos_i + v3s16(0,-1,0);
				m_next_pos_exists = true;
				m_falling = true;
			}else{
				m_falling = false;
			}
		}

		if (m_walk_around && !m_next_pos_exists) {
			/* Find some position where to go next */
			v3s16 dps[3*3*3];
			int num_dps = 0;
			for (int dx=-1; dx<=1; dx++)
			for (int dy=-1; dy<=1; dy++)
			for (int dz=-1; dz<=1; dz++) {
				if (dx == 0 && dy == 0)
					continue;
				if (dx != 0 && dz != 0 && dy != 0)
					continue;
				if ((m_base_position+intToFloat(v3s16(dx,dy,dz),BS)).getDistanceFrom(m_initial_pos) > (10.0*BS))
					continue;
				dps[num_dps++] = v3s16(dx,dy,dz);
			}
			u32 order[3*3*3];
			get_random_u32_array(order, num_dps);
			for (int i=0; i<num_dps; i++) {
				v3s16 p = dps[order[i]] + pos_i;
				if (!checkFreeAndWalkablePosition(p + pos_size_off))
					continue;
				m_next_pos_i = p;
				m_next_pos_exists = true;
				break;
			}
		}
	}else if (m.motion_type == MMT_FLY) {
		bool falling = false;
		bool raising = false;
		if (!m_next_pos_exists) {
			u16 above;
			v3s16 p = pos_i + pos_size_off;
			for (above=0; above < 14; above++) {
				p.Y--;
				if (!checkFreePosition(p))
					break;
			}
			if (above > 12) {
				/* Check whether to drop down */
				if (checkFreePosition(pos_i + pos_size_off + v3s16(0,-1,0))) {
					m_next_pos_i = pos_i + v3s16(0,-1,0);
					falling = true;
				}
			}else if (above < 8) {
				/* Check whether to rise up */
				if (checkFreePosition(pos_i + pos_size_off + v3s16(0,1,0))) {
					m_next_pos_i = pos_i + v3s16(0,1,0);
					raising = true;
				}
			}
		}

		if (m_walk_around && !m_next_pos_exists) {
			/* Find some position where to go next */
			v3s16 dps[3*3*3];
			int num_dps = 0;
			for (int dx=-1; dx<=1; dx++)
			for (int dy=-1; dy<=1; dy++)
			for (int dz=-1; dz<=1; dz++) {
				if (dx == 0 && dy == 0)
					continue;
				if (dx != 0 && dz != 0 && dy != 0)
					continue;
				if (falling && dy > 0)
					continue;
				if (raising && dy < 0)
					continue;
				if ((m_base_position+intToFloat(v3s16(dx,dy,dz),BS)).getDistanceFrom(m_initial_pos) > (10.0*BS))
					continue;
				dps[num_dps++] = v3s16(dx,dy,dz);
			}
			u32 order[3*3*3];
			get_random_u32_array(order, num_dps);
			for (int i=0; i<num_dps; i++) {
				v3s16 p = dps[order[i]] + pos_i;
				if (!checkFreePosition(p + pos_size_off))
					continue;
				m_next_pos_i = p;
				m_next_pos_exists = true;
				break;
			}
		}
	}else if (m.motion_type == MMT_FLYLOW || m.motion_type == MMT_SWIM) {
		bool falling = false;
		if (!m_next_pos_exists) {
			/* Check whether to drop down */
			if (checkFreePosition(pos_i + pos_size_off + v3s16(0,-1,0))) {
				m_next_pos_i = pos_i + v3s16(0,-1,0);
				falling = true;
			}
		}

		if (m_walk_around && !m_next_pos_exists) {
			/* Find some position where to go next */
			v3s16 dps[3*3*3];
			int num_dps = 0;
			for (int dx=-1; dx<=1; dx++)
			for (int dy=-1; dy<=1; dy++)
			for (int dz=-1; dz<=1; dz++) {
				if (dx == 0 && dy == 0)
					continue;
				if (dx != 0 && dz != 0 && dy != 0)
					continue;
				if (falling && dy > 0)
					continue;
				if ((m_base_position+intToFloat(v3s16(dx,dy,dz),BS)).getDistanceFrom(m_initial_pos) > (10.0*BS))
					continue;
				dps[num_dps++] = v3s16(dx,dy,dz);
			}
			u32 order[3*3*3];
			get_random_u32_array(order, num_dps);
			for (int i=0; i<num_dps; i++) {
				v3s16 p = dps[order[i]] + pos_i;
				if (!checkFreePosition(p + pos_size_off))
					continue;
				m_next_pos_i = p;
				m_next_pos_exists = true;
				break;
			}
		}
	}
}
void MobSAO::stepMotionThrown(float dtime)
{
	MobFeatures m = content_mob_features(m_content);
	m_base_position += m_speed * dtime;
	m_base_position.Y -= 0.1*BS*dtime;

	v3s16 pos_i = floatToInt(m_base_position, BS);
	v3s16 pos_size_off(0,0,0);
	if (m.getSize().X >= 2.5) {
		pos_size_off.X = -1;
		pos_size_off.Y = -1;
	}
	if (!checkFreePosition(pos_i + pos_size_off)) {
		if (m.contact_explosion_diameter > 0)
			explodeSquare(pos_i, v3s16(m.contact_explosion_diameter,m.contact_explosion_diameter,m.contact_explosion_diameter));
		m_removed = true;
		return;
	}
}
void MobSAO::stepMotionConstant(float dtime)
{
	MobFeatures m = content_mob_features(m_content);
	m_base_position += m_speed * dtime;

	v3s16 pos_i = floatToInt(m_base_position, BS);
	v3s16 pos_size_off(0,0,0);
	if (m.getSize().X >= 2.5) {
		pos_size_off.X = -1;
		pos_size_off.Y = -1;
	}
	if (!checkFreePosition(pos_i + pos_size_off)) {
		if (m.contact_explosion_diameter > 0)
			explodeSquare(pos_i, v3s16(m.contact_explosion_diameter,m.contact_explosion_diameter,m.contact_explosion_diameter));
		m_removed = true;
		return;
	}
}
bool MobSAO::checkFreePosition(v3s16 p0)
{
	assert(m_env);
	Map *map = &m_env->getMap();
	v3s16 size = content_mob_features(m_content).getSizeBlocks();
	content_t clear = CONTENT_AIR;
	if (content_mob_features(m_content).motion_type == MMT_SWIM)
		clear = CONTENT_WATERSOURCE;
	for (int dx=0; dx<size.X; dx++)
	for (int dy=0; dy<size.Y; dy++)
	for (int dz=0; dz<size.Z; dz++) {
		v3s16 dp(dx, dy, dz);
		v3s16 p = p0 + dp;
		MapNode n = map->getNodeNoEx(p);
		if(n.getContent() != clear)
			return false;
	}
	return true;
}
bool MobSAO::checkWalkablePosition(v3s16 p0)
{
	assert(m_env);
	v3s16 p = p0 + v3s16(0,-1,0);
	MapNode n = m_env->getMap().getNodeNoEx(p);
	content_t clear = CONTENT_AIR;
	if (content_mob_features(m_content).motion_type == MMT_SWIM)
		clear = CONTENT_WATERSOURCE;
	if (n.getContent() != clear)
		return true;
	return false;
}
bool MobSAO::checkFreeAndWalkablePosition(v3s16 p0)
{
	if (!checkFreePosition(p0))
		return false;
	if (!checkWalkablePosition(p0))
		return false;
	return true;
}
void MobSAO::explodeSquare(v3s16 p0, v3s16 size)
{
	assert(m_env);
	Map *map = &m_env->getMap();
	core::map<v3s16, MapBlock*> modified_blocks;

	for (int dx=0; dx<size.X; dx++)
	for (int dy=0; dy<size.Y; dy++)
	for (int dz=0; dz<size.Z; dz++) {
		v3s16 dp(dx - size.X/2, dy - size.Y/2, dz - size.Z/2);
		v3s16 p = p0 + dp;
		MapNode n = map->getNodeNoEx(p);
		if (n.getContent() == CONTENT_IGNORE)
			continue;
		if (content_features(n).pressure_type == CST_SOLID && content_features(n).draw_type == CDT_CUBELIKE)
			continue;
		map->removeNodeAndUpdate(p, modified_blocks);
	}

	// Send a MEET_OTHER event
	MapEditEvent event;
	event.type = MEET_OTHER;
	for (core::map<v3s16, MapBlock*>::Iterator i = modified_blocks.getIterator(); i.atEnd() == false; i++) {
		v3s16 p = i.getNode()->getKey();
		event.modified_blocks.insert(p, true);
	}
	map->dispatchEvent(&event);
}
InventoryItem* MobSAO::createPickedUpItem()
{
	MobFeatures m = content_mob_features(m_content);
	if (m.punch_action != MPA_PICKUP) {
		if (!m_removed)
			return NULL;
	}
	if (m.dropped_item == "")
		return NULL;
	std::istringstream is(m.dropped_item, std::ios_base::binary);
	InventoryItem *item = InventoryItem::deSerialize(is);
	if (!m_removed)
		m_removed = true;
	return item;
}
u16 MobSAO::punch(const std::string &toolname, v3f dir, const std::string &playername)
{
	MobFeatures m = content_mob_features(m_content);
	if (m.punch_action == MPA_IGNORE)
		return 0;

	actionstream<<playername<<" punches mob id="<<m_id
			<<" with a \""<<toolname<<"\" at "
			<<PP(m_base_position/BS)<<std::endl;

	if (m.punch_action == MPA_HARM) {
		m_disturb_timer = 0;
		m_disturbing_player = playername;
		m_next_pos_exists = false; // Cancel moving immediately

		m_yaw = wrapDegrees_180(180./PI*atan2(dir.Z, dir.X) + 180.);
		v3f new_base_position = m_base_position + dir * BS;
		{
			v3s16 pos_i = floatToInt(new_base_position, BS);
			v3s16 pos_size_off(0,0,0);
			if (m.getSize().X >= 2.5) {
				pos_size_off.X = -1;
				pos_size_off.Y = -1;
			}
			bool free = checkFreePosition(pos_i + pos_size_off);
			if(free)
				m_base_position = new_base_position;
		}
		sendPosition();


		ToolItemFeatures f = content_toolitem_features(toolname);
		u16 amount = 2;
		if (f.type == TT_SWORD) {
			amount = 4*((f.hardness/100)+1);
		}else if (f.type == TT_AXE || f.type == TT_PICK) {
			amount = 2*((f.hardness/200)+1);
		}
		doDamage(amount);
	}else if (m.punch_action == MPA_DIE) {
		m_hp = 0;
		m_removed = true;
	}

	return 655;
}
u8 MobSAO::level()
{
	return content_mob_features(m_content).level;
}
void MobSAO::sendPosition()
{
	m_last_sent_position = m_base_position;

	std::ostringstream os(std::ios::binary);
	// command (0 = update position)
	writeU8(os, 0);
	// pos
	writeV3F1000(os, m_base_position);
	// yaw
	writeF1000(os, m_yaw);
	// create message and add to list
	ActiveObjectMessage aom(getId(), false, os.str());
	m_messages_out.push_back(aom);
}
void MobSAO::doDamage(u16 d)
{
	infostream<<"MobV2 hp="<<m_hp<<" damage="<<d<<std::endl;

	if (d < m_hp) {
		m_hp -= d;
	}else{
		actionstream<<"A "<<mobLevelS(content_mob_features(m_content).level)
				<<" mob id="<<m_id<<" dies at "<<PP(m_base_position)<<std::endl;
		// Die
		m_hp = 0;
		m_removed = true;
	}
	{
		std::ostringstream os(std::ios::binary);
		// command (1 = damage)
		writeU8(os, 1);
		// amount
		writeU16(os, d);
		// create message and add to list
		ActiveObjectMessage aom(getId(), false, os.str());
		m_messages_out.push_back(aom);
	}
}
