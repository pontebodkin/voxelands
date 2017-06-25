/************************************************************************
* Minetest-c55
* Copyright (C) 2010-2011 celeron55, Perttu Ahola <celeron55@gmail.com>
*
* content_nodemeta.cpp
* voxelands - 3d voxel world sandbox game
* Copyright (C) Lisa 'darkrose' Milne 2013-2014 <lisa@ltmnet.com>
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
#include "content_craftitem.h"
#include "content_toolitem.h"
#include "player.h"


/*
	ChestNodeMetadata
*/

// Prototype
ChestNodeMetadata proto_ChestNodeMetadata;

ChestNodeMetadata::ChestNodeMetadata()
{
	NodeMetadata::registerType(typeId(), create);

	m_is_locked = false;
	m_is_expanded = false;
	m_is_exo = false;
	m_expanded_slot_id = 0;

	m_inventory = new Inventory();
	m_inventory->addList("upgrades", 2);
	m_inventory->addList("main", 18);
	inventoryModified();
}
ChestNodeMetadata::~ChestNodeMetadata()
{
	delete m_inventory;
}
u16 ChestNodeMetadata::typeId() const
{
	return CONTENT_CHEST;
}
NodeMetadata* ChestNodeMetadata::create(std::istream &is)
{
	std::string s;
	ChestNodeMetadata *d = new ChestNodeMetadata();

	d->setOwner(deSerializeString(is));

	s = deSerializeString(is);
	d->m_is_exo = !!mystoi(s);

	s = deSerializeString(is);
	d->m_is_expanded = !!mystoi(s);

	s = deSerializeString(is);
	d->m_is_locked = !!mystoi(s);

	s = deSerializeString(is);
	d->m_expanded_slot_id = mystoi(s);

	if (d->m_is_expanded) {
		delete d->m_inventory;
		d->m_inventory = new Inventory();
		d->m_inventory->addList("upgrades", 2);
		d->m_inventory->addList("main", 36);
	}
	d->m_inventory->deSerialize(is);
	d->inventoryModified();

	return d;
}
NodeMetadata* ChestNodeMetadata::clone()
{
	ChestNodeMetadata *d = new ChestNodeMetadata();
	d->m_is_exo = m_is_exo;
	d->m_is_expanded = m_is_expanded;
	d->m_is_locked = m_is_locked;
	d->m_expanded_slot_id = m_expanded_slot_id;
	*d->m_inventory = *m_inventory;
	return d;
}
void ChestNodeMetadata::serializeBody(std::ostream &os)
{
	os<<serializeString(m_owner);
	os<<serializeString(itos(m_is_exo ? 1 : 0));
	os<<serializeString(itos(m_is_expanded ? 1 : 0));
	os<<serializeString(itos(m_is_locked ? 1 : 0));
	os<<serializeString(itos(m_expanded_slot_id));
	m_inventory->serialize(os);
}
std::wstring ChestNodeMetadata::infoText()
{
	char buff[256];
	if (m_is_locked) {
		snprintf(buff, 256, gettext("Locked Chest owned by '%s'"), m_owner.c_str());
	}else if (m_is_exo) {
		snprintf(buff, 256, gettext("Exo Chest"));
	}else{
		snprintf(buff, 256, gettext("Chest"));
	}
	return narrow_to_wide(buff);
}
Inventory* ChestNodeMetadata::getInventory()
{
	return m_inventory;
}
void ChestNodeMetadata::inventoryModified()
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
bool ChestNodeMetadata::nodeRemovalDisabled()
{
	/*
		Disable removal if chest contains something
	*/
	InventoryList *list = m_inventory->getList("main");
	if(list == NULL)
		return false;
	if(list->getUsedSlots() == 0)
		return false;
	return true;
}
std::string ChestNodeMetadata::getDrawSpecString(Player *player)
{
	std::string spec("size[9,10]");

	if (!m_is_exo) {
		InventoryList *l = m_inventory->getList("main");
		if (m_is_expanded && l && l->getUsedSlots() > 18) {
			if (m_expanded_slot_id == 0) {
				spec += "list[current_name;upgrades;1,0;1,1;1,1;]";
			}else{
				spec += "list[current_name;upgrades;0,0;1,1;0,1;]";
			}
		}else{
			spec += "list[current_name;upgrades;0,0;2,1;]";
		}
	}

	if (m_is_expanded) {
		spec += "list[current_name;main;0,1;9,4;]";
	}else if (m_is_exo) {
		spec += "list[current_player;exo;1.5,1.5;6,3;]";
	}else{
		spec += "list[current_name;main;1.5,1.5;6,3;]";
	}

	spec += "list[current_player;main;0.5,5.8;8,1;0,8;]";
	spec += "list[current_player;main;0.5,7;8,3;8,-1;]";

	return spec;
}
std::vector<NodeBox> ChestNodeMetadata::getNodeBoxes(MapNode &n)
{
	std::vector<NodeBox> boxes;

	if (m_is_locked) {
		boxes.push_back(NodeBox(
			v3s16(0,180,0),aabb3f(-0.125*BS,-0.3125*BS,-0.5*BS,0.125*BS,-0.125*BS,-0.4375*BS)
		));
		boxes.push_back(NodeBox(
			v3s16(0,180,0),aabb3f(0.0625*BS,-0.125*BS,-0.5*BS,0.09375*BS,0.0,-0.4375*BS)
		));
		boxes.push_back(NodeBox(
			v3s16(0,180,0),aabb3f(-0.09375*BS,-0.125*BS,-0.5*BS,-0.0625*BS,0.0,-0.4375*BS)
		));
	}else if (m_is_exo) {
		boxes.push_back(NodeBox(
			aabb3f(-0.125*BS,-0.3125*BS,0.4375*BS,0.125*BS,-0.1875*BS,0.5*BS)
		));
		boxes.push_back(NodeBox(
			aabb3f(-0.0625*BS,-0.375*BS,0.4375*BS,0.0625*BS,-0.3125*BS,0.5*BS)
		));
		boxes.push_back(NodeBox(
			aabb3f(-0.0625*BS,-0.1875*BS,0.4375*BS,0.0625*BS,-0.125*BS,0.5*BS)
		));
	}

	return boxes;
}
std::string ChestNodeMetadata::getOwner()
{
	if (m_is_locked)
		return m_owner;
	return "";
}
std::string ChestNodeMetadata::getInventoryOwner()
{
	if (m_is_locked)
		return m_owner;
	return "";
}
