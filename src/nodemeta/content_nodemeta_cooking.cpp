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

/*
	CrusherNodeMetadata
*/

// Prototype
CrusherNodeMetadata proto_CrusherNodeMetadata;

CrusherNodeMetadata::CrusherNodeMetadata()
{
	NodeMetadata::registerType(typeId(), create);

	m_inventory = new Inventory();
	m_inventory->addList("fuel", 1);
	m_inventory->addList("src", 1);
	m_inventory->addList("main", 4);
	m_inventory->addList("upgrades", 2);

	m_active_timer = 0.0;
	m_burn_counter = 0.0;
	m_burn_timer = 0.0;
	m_cook_timer = 0.0;
	m_step_interval = 1.0;
	m_is_locked = false;
	m_is_expanded = false;
	m_is_exo = false;
	m_cook_upgrade = 1.0;
	m_burn_upgrade = 1.0;
	m_expanded_slot_id = 0;

	inventoryModified();
}
CrusherNodeMetadata::~CrusherNodeMetadata()
{
	delete m_inventory;
}
u16 CrusherNodeMetadata::typeId() const
{
	return CONTENT_CRUSHER;
}
NodeMetadata* CrusherNodeMetadata::clone()
{
	CrusherNodeMetadata *d = new CrusherNodeMetadata();
	*d->m_inventory = *m_inventory;

	d->m_active_timer = m_active_timer;
	d->m_burn_counter = m_burn_counter;
	d->m_burn_timer = m_burn_timer;
	d->m_cook_timer = m_cook_timer;
	d->m_step_interval = m_step_interval;
	d->m_is_locked = m_is_locked;
	d->m_is_expanded = m_is_expanded;
	d->m_is_exo = m_is_exo;
	d->m_cook_upgrade = m_cook_upgrade;
	d->m_burn_upgrade = m_burn_upgrade;
	d->m_expanded_slot_id = m_expanded_slot_id;

	return d;
}
NodeMetadata* CrusherNodeMetadata::create(std::istream &is)
{
	std::string s;
	CrusherNodeMetadata *d = new CrusherNodeMetadata();

	d->setOwner(deSerializeString(is));

	s = deSerializeString(is);
	d->m_active_timer = mystof(s);

	s = deSerializeString(is);
	d->m_burn_counter = mystoi(s);

	s = deSerializeString(is);
	d->m_burn_timer = mystoi(s);

	s = deSerializeString(is);
	d->m_cook_timer = mystof(s);

	s = deSerializeString(is);
	d->m_step_interval = mystof(s);

	s = deSerializeString(is);
	d->m_is_locked = !!mystoi(s);

	s = deSerializeString(is);
	d->m_is_expanded = !!mystoi(s);

	s = deSerializeString(is);
	d->m_is_exo = !!mystoi(s);

	s = deSerializeString(is);
	d->m_cook_upgrade = mystof(s);

	s = deSerializeString(is);
	d->m_burn_upgrade = mystof(s);

	s = deSerializeString(is);
	d->m_expanded_slot_id = mystoi(s);

	if (d->m_is_expanded) {
		delete d->m_inventory;
		d->m_inventory = new Inventory();
		d->m_inventory->addList("fuel", 1);
		d->m_inventory->addList("src", 1);
		d->m_inventory->addList("main", 18);
		d->m_inventory->addList("upgrades", 2);
	}
	d->m_inventory->deSerialize(is);
	d->inventoryModified();

	return d;
}
void CrusherNodeMetadata::serializeBody(std::ostream &os)
{
	os<<serializeString(m_owner);
	os<<serializeString(ftos(m_active_timer));
	os<<serializeString(itos(m_burn_counter));
	os<<serializeString(itos(m_burn_timer));
	os<<serializeString(ftos(m_cook_timer));
	os<<serializeString(ftos(m_step_interval));
	os<<serializeString(itos(m_is_locked ? 1 : 0));
	os<<serializeString(itos(m_is_expanded ? 1 : 0));
	os<<serializeString(itos(m_is_exo ? 1 : 0));
	os<<serializeString(ftos(m_cook_upgrade));
	os<<serializeString(ftos(m_burn_upgrade));
	os<<serializeString(itos(m_expanded_slot_id));
	m_inventory->serialize(os);
}
std::wstring CrusherNodeMetadata::infoText()
{
	char buff[256];
	if (m_is_locked) {
		snprintf(buff, 256, gettext("Locked Crusher owned by '%s'"), m_owner.c_str());
	}else if (m_is_exo) {
		snprintf(buff, 256, gettext("Exo Crusher"));
	}else{
		snprintf(buff, 256, gettext("Crusher"));
	}
	return narrow_to_wide(buff);
}
bool CrusherNodeMetadata::nodeRemovalDisabled()
{
	/*
		Disable removal if crusher is not empty
	*/
	InventoryList *list[3] = {
		m_inventory->getList("src"),
		m_inventory->getList("main"),
		m_inventory->getList("fuel")
	};

	for (int i = 0; i < 3; i++) {
		if (list[i] == NULL)
			continue;
		if (list[i]->getUsedSlots() == 0)
			continue;
		return true;
	}
	return false;

}
void CrusherNodeMetadata::inventoryModified()
{
	int i;
	int k;
	int a[3] = {1,1,1};
	int b[3] = {0,0,0};
	Inventory *inv;
	InventoryList *il;
	InventoryList *im;
	InventoryItem *itm;
	InventoryList *l = m_inventory->getList("upgrades");
	InventoryList *m = m_inventory->getList("main");
	if (!l || !m)
		return;

	for (i=0; i<2; i++) {
		itm = l->getItem(i);
		if (!itm)
			continue;
		if (itm->getContent() == CONTENT_CHEST) {
			if (m_is_expanded) {
				b[0] = 1;
				continue;
			}
			if (m_is_exo)
				continue;
			inv = new Inventory();
			inv->addList("upgrades", 2);
			inv->addList("main", 36);
			il = inv->getList("upgrades");
			im = inv->getList("main");
			if (!il || !im) {
				delete inv;
				continue;
			}
			for (k=0; k<2; k++) {
				itm = l->changeItem(k,NULL);
				if (itm)
					il->addItem(k,itm);
			}
			for (k=0; k<18; k++) {
				itm = m->changeItem(k,NULL);
				if (itm)
					im->addItem(k,itm);
			}
			delete m_inventory;
			m_inventory = inv;
			l = il;
			m = im;
			a[2] = 0;
			b[0] = 1;
			m_is_expanded = true;
			m_expanded_slot_id = i;
		}else if (itm->getContent() == CONTENT_CRAFTITEM_PADLOCK) {
			if (m_is_exo)
				continue;
			a[2] = 0;
			b[1] = 1;
			m_is_locked = true;
		}else if (itm->getContent() == CONTENT_CRAFTITEM_OERKKI_DUST) {
			if (m_is_exo) {
				b[2] = 1;
				continue;
			}
			if (m->getUsedSlots() != 0)
				continue;
			if (l->getUsedSlots() != 1)
				continue;
			if (m_is_locked)
				continue;
			if (m_is_expanded)
				continue;
			m_is_exo = true;
			a[0] = 0;
			a[1] = 0;
			b[2] = 1;
		}
	}

	if (m_is_expanded && !b[0]) {
		inv = new Inventory();
		inv->addList("upgrades", 2);
		inv->addList("main", 18);
		il = inv->getList("upgrades");
		im = inv->getList("main");
		if (!il || !im) {
			delete inv;
		}else{
			for (k=0; k<2; k++) {
				itm = l->changeItem(k,NULL);
				if (itm)
					il->addItem(k,itm);
			}
			for (k=0; k<36; k++) {
				itm = m->changeItem(k,NULL);
				if (itm) {
					if (k > 17) {
						im->addItem(itm);
					}else{
						im->addItem(k,itm);
					}
				}
			}
			delete m_inventory;
			m_inventory = inv;
			l = il;
			m = im;
			m_is_expanded = false;
		}
	}

	if (m_is_locked && !b[1])
		m_is_locked = false;

	if (m_is_exo && !b[2])
		m_is_exo = false;

	if (m_is_expanded || m_is_locked || m->getUsedSlots() != 0)
		a[2] = 0;

	l->clearAllowed();
	if (a[0])
		l->addAllowed(CONTENT_CHEST);
	if (a[1])
		l->addAllowed(CONTENT_CRAFTITEM_PADLOCK);
	if (a[2])
		l->addAllowed(CONTENT_CRAFTITEM_OERKKI_DUST);
}
bool CrusherNodeMetadata::step(float dtime, v3s16 pos, ServerEnvironment *env)
{
	float cook_time;
	bool changed = false;
	bool is_cooking;
	bool cook_ongoing;
	bool room_available;
	InventoryList *dst_list;
	InventoryList *src_list;
	InventoryItem *src_item;
	InventoryList *fuel_list;
	InventoryItem *fuel_item;

	if (dtime > 60.0)
		vlprintf(CN_INFO,"Crusher stepping a long time (%f)",dtime);

	if (m_is_exo) {
		Player *p = env->getPlayer(m_owner.c_str());
		if (!p)
			return false;
		dst_list = p->inventory.getList("exo");
	}else{
		dst_list = m_inventory->getList("main");
	}
	if (!dst_list)
		return false;

	src_list = m_inventory->getList("src");
	if (!src_list)
		return false;

	m_active_timer += dtime;

	if (m_cook_upgrade < 1.0)
		m_cook_upgrade = 1.0;
	if (m_burn_upgrade < 1.0)
		m_burn_upgrade = 1.0;

	/* cook_upgrade/m_cook_time determines time to cook one item */
	/* burn_upgrade/m_burn_time determines number of items that fuel can cook */

	cook_time = 4.0/m_cook_upgrade;
	if (cook_time < 0.1)
		cook_time = 0.1;

	if (cook_time < m_step_interval)
		m_step_interval = cook_time;

	while (m_active_timer > m_step_interval) {
		m_active_timer -= m_step_interval;

		is_cooking = false;
		cook_ongoing = false;
		room_available = false;

		src_item = src_list->getItem(0);
		if (src_item && src_item->isCrushable(CRUSH_CRUSHER)) {
			is_cooking = true;
			room_available = dst_list->roomForCrushedItem(src_item);
			if (room_available && src_item->getCount() > 1)
				cook_ongoing = true;
		}else{
			m_cook_timer = 0.0;
		}

		if (m_cook_timer > cook_time)
			m_cook_timer = cook_time;

		if (m_burn_counter < 1.0 && is_cooking) {
			if (m_cook_timer+m_step_interval < cook_time || cook_ongoing) {
				fuel_list = m_inventory->getList("fuel");
				if (!fuel_list)
					break;
				fuel_item = fuel_list->getItem(0);
				if (fuel_item && fuel_item->isFuel()) {
					content_t c = fuel_item->getContent();
					float v = 0.0;
					if ((c&CONTENT_CRAFTITEM_MASK) == CONTENT_CRAFTITEM_MASK) {
						v = ((CraftItem*)fuel_item)->getFuelTime();
					}else if ((c&CONTENT_TOOLITEM_MASK) == CONTENT_TOOLITEM_MASK) {
						v = ((ToolItem*)fuel_item)->getFuelTime();
					}else{
						v = ((MaterialItem*)fuel_item)->getFuelTime();
					}
					fuel_list->decrementMaterials(1);
					if (c == CONTENT_TOOLITEM_STEELBUCKET_LAVA) {
						fuel_list->addItem(0,new ToolItem(CONTENT_TOOLITEM_STEELBUCKET,0,0));
					}
					m_burn_counter += v*m_burn_upgrade;
					changed = true;
				}
			}
		}

		if (m_burn_counter <= 0.0) {
			m_active_timer = 0.0;
			m_burn_counter = 0.0;
			m_burn_timer = 0.0;
			break;
		}

		if (m_burn_counter < 1.0)
			continue;

		m_burn_timer += m_step_interval;
		if (m_burn_timer >= cook_time) {
			m_burn_counter -= 1.0;
			m_burn_timer -= cook_time;
		}

		if (!is_cooking) {
			m_cook_timer = 0.0;
			continue;
		}

		m_cook_timer += m_step_interval;

		if (m_cook_timer >= cook_time) {
			m_cook_timer -= cook_time;
			if (src_item && src_item->isCrushable(CRUSH_CRUSHER)) {
				InventoryItem *crushresult = src_item->createCrushResult();
				dst_list->addItem(crushresult);
				src_list->decrementMaterials(1);
				changed = true;
			}
		}
	}

	return changed;
}
std::string CrusherNodeMetadata::getDrawSpecString(Player *player)
{
	float cook_time;

	if (m_cook_upgrade < 1.0)
		m_cook_upgrade = 1.0;
	cook_time = 4.0/m_cook_upgrade;
	if (cook_time < 0.1)
		cook_time = 0.1;

	std::string spec("size[8,9]");
	spec += "list[current_name;fuel;2,3;1,1;]";
	spec += "ring[2,3;1;#FF0000;";
	float v = 0;
	if (m_burn_counter > 0.0 && m_cook_timer > 0.0)
		v = 100.0-((100.0/cook_time)*m_cook_timer);
	spec += itos((int)v);
	spec += "]";
	spec += "list[current_name;src;2,1;1,1;]";
	spec += "list[current_name;main;5,1;2,2;]";
	spec += "list[current_player;main;0,5;8,4;]";
	return spec;
}
std::vector<NodeBox> CrusherNodeMetadata::getNodeBoxes(MapNode &n)
{
	std::vector<NodeBox> boxes;
	boxes.clear();

	if (m_burn_counter > 0.0) {
		boxes.push_back(NodeBox(
			-0.3125*BS,-0.25*BS,-0.4*BS,0.3125*BS,0.125*BS,-0.3*BS
		));
	}

	return transformNodeBox(n,boxes);
}
std::string CrusherNodeMetadata::getOwner()
{
	if (m_is_locked)
		return m_owner;
	return "";
}
std::string CrusherNodeMetadata::getInventoryOwner()
{
	if (m_is_locked)
		return m_owner;
	return "";
}
