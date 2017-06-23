/************************************************************************
* Minetest-c55
* Copyright (C) 2010-2011 celeron55, Perttu Ahola <celeron55@gmail.com>
*
* content_nodemeta.cpp
* voxelands - 3d voxel world sandbox game
* Copyright (C) Lisa 'darkrose' Milne 2013-2017 <lisa@ltmnet.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>
*
* License updated from GPLv2 or later to GPLv3 or later by Lisa Milne
* for Voxelands.
************************************************************************/

#include "common.h"
#include "content_nodemeta.h"
#include "inventory.h"
#include "content_mapnode.h"
#include "environment.h"

/*
	FurnaceNodeMetadata
*/

// Prototype
FurnaceNodeMetadata proto_FurnaceNodeMetadata;

FurnaceNodeMetadata::FurnaceNodeMetadata()
{
	NodeMetadata::registerType(typeId(), create);

	m_inventory = new Inventory();
	m_inventory->addList("fuel", 1);
	m_inventory->addList("src", 1);
	m_inventory->addList("dst", 4);

	m_step_accumulator = 0;
	m_fuel_totaltime = 0;
	m_fuel_time = 0;
	m_src_totaltime = 0;
	m_src_time = 0;
}
FurnaceNodeMetadata::~FurnaceNodeMetadata()
{
	delete m_inventory;
}
u16 FurnaceNodeMetadata::typeId() const
{
	return CONTENT_FURNACE;
}
NodeMetadata* FurnaceNodeMetadata::clone()
{
	FurnaceNodeMetadata *d = new FurnaceNodeMetadata();
	*d->m_inventory = *m_inventory;
	d->m_fuel_totaltime = m_fuel_totaltime;
	d->m_fuel_time = m_fuel_time;
	d->m_src_totaltime = m_src_totaltime;
	d->m_src_time = m_src_time;
	return d;
}
NodeMetadata* FurnaceNodeMetadata::create(std::istream &is)
{
	FurnaceNodeMetadata *d = new FurnaceNodeMetadata();

	d->m_inventory->deSerialize(is);

	int temp;
	is>>temp;
	d->m_fuel_totaltime = (float)temp/10;
	is>>temp;
	d->m_fuel_time = (float)temp/10;

	return d;
}
void FurnaceNodeMetadata::serializeBody(std::ostream &os)
{
	m_inventory->serialize(os);
	os<<itos(m_fuel_totaltime*10)<<" ";
	os<<itos(m_fuel_time*10)<<" ";
}
std::wstring FurnaceNodeMetadata::infoText()
{
	//return "Furnace";
	if (m_fuel_time >= m_fuel_totaltime) {
		const InventoryList *src_list = m_inventory->getList("src");
		assert(src_list);
		const InventoryItem *src_item = src_list->getItem(0);

		if (src_item && src_item->isCookable(COOK_FURNACE)) {
			InventoryList *dst_list = m_inventory->getList("dst");
			if(!dst_list->roomForCookedItem(src_item))
				return narrow_to_wide(gettext("Furnace is overloaded"));
			return narrow_to_wide(gettext("Furnace is out of fuel"));
		}else{
			return narrow_to_wide(gettext("Furnace is inactive"));
		}
	}else{
		std::wstring s = narrow_to_wide(gettext("Furnace is active"));
		// Do this so it doesn't always show (0%) for weak fuel
		if (m_fuel_totaltime > 3) {
			s += L" (";
			s += itows(m_fuel_time/m_fuel_totaltime*100);
			s += L"%)";
		}
		return s;
	}
}
bool FurnaceNodeMetadata::nodeRemovalDisabled()
{
	/*
		Disable removal if furnace is not empty
	*/
	InventoryList *list[3] = {m_inventory->getList("src"),
	m_inventory->getList("dst"), m_inventory->getList("fuel")};

	for (int i = 0; i < 3; i++) {
		if (list[i] == NULL)
			continue;
		if (list[i]->getUsedSlots() == 0)
			continue;
		return true;
	}
	return false;

}
void FurnaceNodeMetadata::inventoryModified()
{
	vlprintf(CN_INFO,"Furnace inventory modification callback");
}
bool FurnaceNodeMetadata::step(float dtime, v3s16 pos, ServerEnvironment *env)
{
	{
		MapNode n = env->getMap().getNodeNoEx(pos).getContent();
		if (n.getContent() == CONTENT_FURNACE_ACTIVE) {
			n.param1 = n.param2;
			n.setContent(CONTENT_FURNACE);
			env->setPostStepNodeSwap(pos,n);
		}
	}

	if (dtime > 60.0)
		vlprintf(CN_INFO,"Furnace stepping a long time (%f)",dtime);
	// Update at a fixed frequency
	const float interval = 2.0;
	m_step_accumulator += dtime;
	bool changed = false;
	while (m_step_accumulator > interval) {
		m_step_accumulator -= interval;
		dtime = interval;

		InventoryList *dst_list = m_inventory->getList("dst");
		assert(dst_list);

		InventoryList *src_list = m_inventory->getList("src");
		assert(src_list);
		InventoryItem *src_item = src_list->getItem(0);

		bool room_available = false;

		if (src_item && src_item->isCookable(COOK_FURNACE))
			room_available = dst_list->roomForCookedItem(src_item);

		// Start only if there are free slots in dst, so that it can
		// accomodate any result item
		if (room_available) {
			m_src_totaltime = 3;
		}else{
			m_src_time = 0;
			m_src_totaltime = 0;
		}

		/*
			If fuel is burning, increment the burn counters.
			If item finishes cooking, move it to result.
		*/
		if (m_fuel_time < m_fuel_totaltime) {
			m_fuel_time += dtime;
			m_src_time += dtime;
			if (m_src_time >= m_src_totaltime && m_src_totaltime > 0.001 && src_item) {
				InventoryItem *cookresult = src_item->createCookResult();
				dst_list->addItem(cookresult);
				src_list->decrementMaterials(1);
				m_src_time = 0;
				m_src_totaltime = 0;
			}
			changed = true;

			// If the fuel was not used up this step, just keep burning it
			if (m_fuel_time < m_fuel_totaltime)
				continue;
		}

		/*
			Get the source again in case it has all burned
		*/
		src_item = src_list->getItem(0);

		/*
			If there is no source item, or the source item is not cookable,
			or the furnace is still cooking, or the furnace became overloaded, stop loop.
		*/
		if (
			src_item == NULL
			|| !room_available
			|| m_fuel_time < m_fuel_totaltime
			|| dst_list->roomForCookedItem(src_item) == false
		) {
			m_step_accumulator = 0;
			break;
		}

		//vlprintf(CN_INFO,"Furnace is out of fuel");

		InventoryList *fuel_list = m_inventory->getList("fuel");
		assert(fuel_list);
		InventoryItem *fuel_item = fuel_list->getItem(0);
		if (fuel_item && fuel_item->isFuel()) {
			if ((fuel_item->getContent()&CONTENT_CRAFTITEM_MASK) == CONTENT_CRAFTITEM_MASK) {
				m_fuel_totaltime = ((CraftItem*)fuel_item)->getFuelTime();
			}else if ((fuel_item->getContent()&CONTENT_TOOLITEM_MASK) == CONTENT_TOOLITEM_MASK) {
				m_fuel_totaltime = ((ToolItem*)fuel_item)->getFuelTime();
			}else{
				m_fuel_totaltime = ((MaterialItem*)fuel_item)->getFuelTime();
			}
			m_fuel_time = 0;
			content_t c = fuel_item->getContent();
			fuel_list->decrementMaterials(1);
			if (c == CONTENT_TOOLITEM_STEELBUCKET_LAVA) {
				fuel_list->addItem(0,new ToolItem(CONTENT_TOOLITEM_STEELBUCKET,0,0));
			}
			changed = true;
		}else{
			m_step_accumulator = 0;
		}
	}
	return changed;
}
std::string FurnaceNodeMetadata::getDrawSpecString(Player *player)
{
	std::string spec("size[8,9]");
	spec += "list[current_name;fuel;2,3;1,1;]";
	spec += "ring[2,3;1;#FF0000;";
	float v = 0;
	if (m_fuel_totaltime > 0.0)
		v = 100.0-((100.0/m_fuel_totaltime)*m_fuel_time);
	spec += itos((int)v);
	spec += "]";
	spec += "list[current_name;src;2,1;1,1;]";
	spec += "list[current_name;dst;5,1;2,2;]";
	spec += "list[current_player;main;0,5;8,4;]";
	return spec;
}
std::vector<NodeBox> FurnaceNodeMetadata::getNodeBoxes(MapNode &n)
{
	std::vector<NodeBox> boxes;
	boxes.clear();

	if (m_fuel_time < m_fuel_totaltime) {
		boxes.push_back(NodeBox(
			-0.3125*BS,-0.25*BS,-0.4*BS,0.3125*BS,0.125*BS,-0.3*BS
		));
	}

	return transformNodeBox(n,boxes);
}
bool FurnaceNodeMetadata::import(NodeMetadata *meta)
{
	if (meta->typeId() != CONTENT_LOCKABLE_FURNACE)
		return false;
	LockingFurnaceNodeMetadata *l = (LockingFurnaceNodeMetadata*)meta;
	*m_inventory = *l->getInventory();
	return true;
}

/*
	LockingFurnaceNodeMetadata
*/

// Prototype
LockingFurnaceNodeMetadata proto_LockingFurnaceNodeMetadata;

LockingFurnaceNodeMetadata::LockingFurnaceNodeMetadata()
{
	NodeMetadata::registerType(typeId(), create);

	m_inventory = new Inventory();
	m_inventory->addList("fuel", 1);
	m_inventory->addList("src", 1);
	m_inventory->addList("dst", 4);

	m_step_accumulator = 0;
	m_fuel_totaltime = 0;
	m_fuel_time = 0;
	m_src_totaltime = 0;
	m_src_time = 0;
	m_lock = 0;
}
LockingFurnaceNodeMetadata::~LockingFurnaceNodeMetadata()
{
	delete m_inventory;
}
u16 LockingFurnaceNodeMetadata::typeId() const
{
	return CONTENT_LOCKABLE_FURNACE;
}
NodeMetadata* LockingFurnaceNodeMetadata::clone()
{
	LockingFurnaceNodeMetadata *d = new LockingFurnaceNodeMetadata();
	*d->m_inventory = *m_inventory;
	d->m_fuel_totaltime = m_fuel_totaltime;
	d->m_fuel_time = m_fuel_time;
	d->m_src_totaltime = m_src_totaltime;
	d->m_src_time = m_src_time;
	return d;
}
NodeMetadata* LockingFurnaceNodeMetadata::create(std::istream &is)
{
	LockingFurnaceNodeMetadata *d = new LockingFurnaceNodeMetadata();

	d->m_inventory->deSerialize(is);
	d->setOwner(deSerializeString(is));
	d->setInventoryOwner(deSerializeString(is));

	int temp;
	is>>temp;
	d->m_fuel_totaltime = (float)temp/10;
	is>>temp;
	d->m_fuel_time = (float)temp/10;
	is>>temp;
	d->m_lock = (float)temp/10;

	return d;
}
void LockingFurnaceNodeMetadata::serializeBody(std::ostream &os)
{
	m_inventory->serialize(os);
	os<<serializeString(m_owner);
	os<<serializeString(m_inv_owner);
	os<<itos(m_fuel_totaltime*10)<<" ";
	os<<itos(m_fuel_time*10)<<" ";
	os<<itos(m_lock*10)<<" ";
}
std::wstring LockingFurnaceNodeMetadata::infoText()
{
	char buff[256];
	char* s;
	char e[128];
	std::string ostr = m_owner;

	e[0] = 0;

	if (m_inv_owner != "")
		ostr += ","+m_inv_owner;

	if (m_fuel_time >= m_fuel_totaltime) {
		const InventoryList *src_list = m_inventory->getList("src");
		assert(src_list);
		const InventoryItem *src_item = src_list->getItem(0);

		if(src_item && src_item->isCookable(COOK_FURNACE)) {
			InventoryList *dst_list = m_inventory->getList("dst");
			if (!dst_list->roomForCookedItem(src_item)) {
				s = gettext("Locking Furnace is overloaded");
			}else{
				s = gettext("Locking Furnace is out of fuel");
			}
		}else{
			s = gettext("Locking Furnace is inactive");
		}
	}else{
		s = gettext("Locking Furnace is active");
		// Do this so it doesn't always show (0%) for weak fuel
		if (m_fuel_totaltime > 3) {
			uint32_t tt = m_fuel_time/m_fuel_totaltime*100;
			snprintf(e,128, " (%d%%)",tt);
		}
	}

	snprintf(buff,256,"%s (%s)%s",s,ostr.c_str(),e);
	return narrow_to_wide(buff);
}
bool LockingFurnaceNodeMetadata::nodeRemovalDisabled()
{
	/*
		Disable removal if furnace is not empty
	*/
	InventoryList *list[3] = {m_inventory->getList("src"),
	m_inventory->getList("dst"), m_inventory->getList("fuel")};

	for(int i = 0; i < 3; i++) {
		if(list[i] == NULL)
			continue;
		if(list[i]->getUsedSlots() == 0)
			continue;
		return true;
	}
	return false;

}
void LockingFurnaceNodeMetadata::inventoryModified()
{
	vlprintf(CN_INFO,"LockingFurnace inventory modification callback");
}
bool LockingFurnaceNodeMetadata::step(float dtime, v3s16 pos, ServerEnvironment *env)
{
	{
		MapNode n = env->getMap().getNodeNoEx(pos);
		if (n.getContent() == CONTENT_LOCKABLE_FURNACE_ACTIVE) {
			n.param1 = n.param2;
			n.setContent(CONTENT_LOCKABLE_FURNACE);
			env->setPostStepNodeSwap(pos,n);
		}
	}
	if (dtime > 60.0)
		vlprintf(CN_INFO,"LockingFurnace stepping a long time (%f)",dtime);
	// Update at a fixed frequency
	const float interval = 2.0;
	m_step_accumulator += dtime;
	bool changed = false;
	while(m_step_accumulator > interval) {
		m_step_accumulator -= interval;
		dtime = interval;

		//infostream<<"Furnace step dtime="<<dtime<<std::endl;

		InventoryList *dst_list = m_inventory->getList("dst");
		assert(dst_list);

		InventoryList *src_list = m_inventory->getList("src");
		assert(src_list);
		InventoryItem *src_item = src_list->getItem(0);

		bool room_available = false;

		if (src_item && src_item->isCookable(COOK_FURNACE)) {
			room_available = dst_list->roomForCookedItem(src_item);
			m_lock = 300.0;
			changed = true;
		}else if (m_lock < 0.0) {
			setInventoryOwner("");
			changed = true;
		}else{
			m_lock -= dtime;
			changed = true;
		}

		// Start only if there are free slots in dst, so that it can
		// accomodate any result item
		if (room_available) {
			m_src_totaltime = 3;
		}else{
			m_src_time = 0;
			m_src_totaltime = 0;
		}

		/*
			If fuel is burning, increment the burn counters.
			If item finishes cooking, move it to result.
		*/
		if (m_fuel_time < m_fuel_totaltime) {
			m_fuel_time += dtime;
			m_src_time += dtime;
			if (m_src_time >= m_src_totaltime && m_src_totaltime > 0.001 && src_item) {
				InventoryItem *cookresult = src_item->createCookResult();
				dst_list->addItem(cookresult);
				src_list->decrementMaterials(1);
				m_src_time = 0;
				m_src_totaltime = 0;
			}
			changed = true;

			// If the fuel was not used up this step, just keep burning it
			if (m_fuel_time < m_fuel_totaltime)
				continue;
		}

		/*
			Get the source again in case it has all burned
		*/
		src_item = src_list->getItem(0);

		/*
			If there is no source item, or the source item is not cookable,
			or the furnace is still cooking, or the furnace became overloaded, stop loop.
		*/
		if (
			src_item == NULL
			|| !room_available ||
			m_fuel_time < m_fuel_totaltime
			|| dst_list->roomForCookedItem(src_item) == false
		) {
			m_step_accumulator = 0;
			break;
		}

		//vlprintf(CN_INFO,"Furnace is out of fuel");

		InventoryList *fuel_list = m_inventory->getList("fuel");
		assert(fuel_list);
		InventoryItem *fuel_item = fuel_list->getItem(0);
		if (fuel_item && fuel_item->isFuel()) {
			if ((fuel_item->getContent()&CONTENT_CRAFTITEM_MASK) == CONTENT_CRAFTITEM_MASK) {
				m_fuel_totaltime = ((CraftItem*)fuel_item)->getFuelTime();
			}else if ((fuel_item->getContent()&CONTENT_TOOLITEM_MASK) == CONTENT_TOOLITEM_MASK) {
				m_fuel_totaltime = ((ToolItem*)fuel_item)->getFuelTime();
			}else{
				m_fuel_totaltime = ((MaterialItem*)fuel_item)->getFuelTime();
			}
			m_fuel_time = 0;
			content_t c = fuel_item->getContent();
			fuel_list->decrementMaterials(1);
			if (c == CONTENT_TOOLITEM_STEELBUCKET_LAVA) {
				fuel_list->addItem(0,new ToolItem(CONTENT_TOOLITEM_STEELBUCKET,0,0));
			}
			changed = true;
		}else{
			m_step_accumulator = 0;
		}
	}
	return changed;
}
std::string LockingFurnaceNodeMetadata::getDrawSpecString(Player *player)
{
	std::string spec("size[8,9]");
	spec += "list[current_name;fuel;2,3;1,1;]";
	spec += "ring[2,3;1;#FF0000;";
	float v = 0;
	if (m_fuel_totaltime > 0.0)
		v = 100.0-((100.0/m_fuel_totaltime)*m_fuel_time);
	spec += itos((int)v);
	spec += "]";
	spec += "list[current_name;src;2,1;1,1;]";
	spec += "list[current_name;dst;5,1;2,2;]";
	spec += "list[current_player;main;0,5;8,4;]";
	return spec;
}
std::vector<NodeBox> LockingFurnaceNodeMetadata::getNodeBoxes(MapNode &n)
{
	std::vector<NodeBox> boxes;
	boxes.clear();

	if (m_fuel_time < m_fuel_totaltime) {
		boxes.push_back(NodeBox(
			-0.3125*BS,-0.25*BS,-0.4*BS,0.3125*BS,0.125*BS,-0.3*BS
		));
	}

	return transformNodeBox(n,boxes);
}
bool LockingFurnaceNodeMetadata::import(NodeMetadata *meta)
{
	if (meta->typeId() != CONTENT_FURNACE)
		return false;
	FurnaceNodeMetadata *l = (FurnaceNodeMetadata*)meta;
	*m_inventory = *l->getInventory();
	return true;
}

/*
	CampFireNodeMetadata
*/

// Prototype
CampFireNodeMetadata proto_CampFireNodeMetadata;

CampFireNodeMetadata::CampFireNodeMetadata()
{
	NodeMetadata::registerType(typeId(), create);

	m_inventory = new Inventory();
	m_inventory->addList("fuel", 1);
	m_inventory->addList("src", 1);
	m_inventory->addList("dst", 1);

	m_step_accumulator = 0;
	m_fuel_totaltime = 0;
	m_fuel_time = 0;
	m_src_totaltime = 0;
	m_src_time = 0;
}
CampFireNodeMetadata::~CampFireNodeMetadata()
{
	delete m_inventory;
}
u16 CampFireNodeMetadata::typeId() const
{
	return CONTENT_CAMPFIRE;
}
NodeMetadata* CampFireNodeMetadata::clone()
{
	CampFireNodeMetadata *d = new CampFireNodeMetadata();
	*d->m_inventory = *m_inventory;
	d->m_fuel_totaltime = m_fuel_totaltime;
	d->m_fuel_time = m_fuel_time;
	d->m_src_totaltime = m_src_totaltime;
	d->m_src_time = m_src_time;
	return d;
}
NodeMetadata* CampFireNodeMetadata::create(std::istream &is)
{
	CampFireNodeMetadata *d = new CampFireNodeMetadata();

	d->m_inventory->deSerialize(is);

	int temp;
	is>>temp;
	d->m_fuel_totaltime = (float)temp/10;
	is>>temp;
	d->m_fuel_time = (float)temp/10;

	return d;
}
void CampFireNodeMetadata::serializeBody(std::ostream &os)
{
	m_inventory->serialize(os);
	os<<itos(m_fuel_totaltime*10)<<" ";
	os<<itos(m_fuel_time*10)<<" ";
}
std::wstring CampFireNodeMetadata::infoText()
{
	char buff[256];
	char* s;
	char e[128];
	e[0] = 0;

	if (m_fuel_time >= m_fuel_totaltime) {
		const InventoryList *src_list = m_inventory->getList("src");
		assert(src_list);
		const InventoryItem *src_item = src_list->getItem(0);

		if (src_item && src_item->isCookable(COOK_FIRE)) {
			InventoryList *dst_list = m_inventory->getList("dst");
			if(!dst_list->roomForCookedItem(src_item)) {
				s = gettext("CampFire is overloaded");
			}else{
				s = gettext("CampFire is out of fuel");
			}
		}else{
			s = gettext("CampFire is inactive");
		}
	}else{
		s = gettext("CampFire is active");
		// Do this so it doesn't always show (0%) for weak fuel
		if (m_fuel_totaltime > 3) {
			uint32_t tt = m_fuel_time/m_fuel_totaltime*100;
			snprintf(e,128, " (%d%%)",tt);
		}
	}

	snprintf(buff,256,"%s%s",s,e);
	return narrow_to_wide(buff);
}
bool CampFireNodeMetadata::nodeRemovalDisabled()
{
	/*
		Disable removal if furnace is not empty
	*/
	InventoryList *list[3] = {m_inventory->getList("src"),
	m_inventory->getList("dst"), m_inventory->getList("fuel")};

	for (int i = 0; i < 3; i++) {
		if (list[i] == NULL)
			continue;
		if (list[i]->getUsedSlots() == 0)
			continue;
		return true;
	}
	return false;

}
void CampFireNodeMetadata::inventoryModified()
{
	vlprintf(CN_INFO,"CampFire inventory modification callback");
}
bool CampFireNodeMetadata::step(float dtime, v3s16 pos, ServerEnvironment *env)
{
	{
		MapNode n = env->getMap().getNodeNoEx(pos).getContent();
		if (n.getContent() == CONTENT_FURNACE_ACTIVE) {
			n.param1 = n.param2;
			n.setContent(CONTENT_FURNACE);
			env->setPostStepNodeSwap(pos,n);
		}
	}

	if (dtime > 60.0)
		vlprintf(CN_INFO,"CampFire stepping a long time (%f)",dtime);
	// Update at a fixed frequency
	const float interval = 2.0;
	m_step_accumulator += dtime;
	bool changed = false;
	while (m_step_accumulator > interval) {
		m_step_accumulator -= interval;
		dtime = interval;

		InventoryList *dst_list = m_inventory->getList("dst");
		assert(dst_list);

		InventoryList *src_list = m_inventory->getList("src");
		assert(src_list);
		InventoryItem *src_item = src_list->getItem(0);

		bool room_available = false;

		if (src_item && src_item->isCookable(COOK_FIRE))
			room_available = dst_list->roomForCookedItem(src_item);

		// Start only if there are free slots in dst, so that it can
		// accomodate any result item
		if (room_available) {
			m_src_totaltime = 3;
		}else{
			m_src_time = 0;
			m_src_totaltime = 0;
		}

		/*
			If fuel is burning, increment the burn counters.
			If item finishes cooking, move it to result.
		*/
		if (m_fuel_time < m_fuel_totaltime) {
			m_fuel_time += dtime;
			m_src_time += dtime;
			if (m_src_time >= m_src_totaltime && m_src_totaltime > 0.001 && src_item) {
				InventoryItem *cookresult = src_item->createCookResult();
				dst_list->addItem(cookresult);
				src_list->decrementMaterials(1);
				m_src_time = 0;
				m_src_totaltime = 0;
			}
			changed = true;

			// If the fuel was not used up this step, just keep burning it
			if (m_fuel_time < m_fuel_totaltime)
				continue;
		}

		/*
			Get the source again in case it has all burned
		*/
		src_item = src_list->getItem(0);

		/*
			If there is no source item, or the source item is not cookable,
			or the furnace is still cooking, or the furnace became overloaded, stop loop.
		*/
		if (
			src_item == NULL
			|| !room_available
			|| m_fuel_time < m_fuel_totaltime
			|| dst_list->roomForCookedItem(src_item) == false
		) {
			m_step_accumulator = 0;
			break;
		}

		//vlprintf(CN_INFO,"CampFire is out of fuel");

		InventoryList *fuel_list = m_inventory->getList("fuel");
		assert(fuel_list);
		InventoryItem *fuel_item = fuel_list->getItem(0);
		if (fuel_item && fuel_item->isFuel()) {
			if ((fuel_item->getContent()&CONTENT_CRAFTITEM_MASK) == CONTENT_CRAFTITEM_MASK) {
				m_fuel_totaltime = ((CraftItem*)fuel_item)->getFuelTime();
			}else if ((fuel_item->getContent()&CONTENT_TOOLITEM_MASK) == CONTENT_TOOLITEM_MASK) {
				m_fuel_totaltime = ((ToolItem*)fuel_item)->getFuelTime();
			}else{
				m_fuel_totaltime = ((MaterialItem*)fuel_item)->getFuelTime();
			}
			m_fuel_totaltime *= 2.0;
			m_fuel_time = 0;
			content_t c = fuel_item->getContent();
			fuel_list->decrementMaterials(1);
			if (c == CONTENT_TOOLITEM_STEELBUCKET_LAVA) {
				fuel_list->addItem(0,new ToolItem(CONTENT_TOOLITEM_STEELBUCKET,0,0));
			}
			changed = true;
		}else{
			m_step_accumulator = 0;
		}
	}
	return changed;
}
std::string CampFireNodeMetadata::getDrawSpecString(Player *player)
{
	std::string spec("size[8,9]");
	spec += "list[current_name;fuel;2,2;1,1;]";
	spec += "list[current_name;src;5,1;1,1;]";
	spec += "list[current_name;dst;5,3;1,1;]";
	spec += "list[current_player;main;0,5;8,4;]";
	return spec;
}
std::vector<NodeBox> CampFireNodeMetadata::getNodeBoxes(MapNode &n)
{
	std::vector<NodeBox> boxes;
	boxes.clear();

	if (m_fuel_time < m_fuel_totaltime) {
		boxes.push_back(NodeBox(
			-0.3125*BS,-0.25*BS,-0.4*BS,0.3125*BS,0.125*BS,-0.3*BS
		));
	}

	return transformNodeBox(n,boxes);
}

bool CampFireNodeMetadata::isActive()
{
	if (m_fuel_time >= m_fuel_totaltime)
		return false;
	return true;
}
