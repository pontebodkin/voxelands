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
#include "player.h"


/*
	ChestNodeMetadata
*/

// Prototype
ChestNodeMetadata proto_ChestNodeMetadata;

ChestNodeMetadata::ChestNodeMetadata()
{
	NodeMetadata::registerType(typeId(), create);

	m_inventory = new Inventory();
	m_inventory->addList("0", 8*4);
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
	ChestNodeMetadata *d = new ChestNodeMetadata();
	d->m_inventory->deSerialize(is);
	return d;
}
NodeMetadata* ChestNodeMetadata::clone()
{
	ChestNodeMetadata *d = new ChestNodeMetadata();
	*d->m_inventory = *m_inventory;
	return d;
}
void ChestNodeMetadata::serializeBody(std::ostream &os)
{
	m_inventory->serialize(os);
}
std::wstring ChestNodeMetadata::infoText()
{
	return narrow_to_wide(gettext("Chest"));
}
bool ChestNodeMetadata::nodeRemovalDisabled()
{
	/*
		Disable removal if chest contains something
	*/
	InventoryList *list = m_inventory->getList("0");
	if(list == NULL)
		return false;
	if(list->getUsedSlots() == 0)
		return false;
	return true;
}
std::string ChestNodeMetadata::getDrawSpecString(Player *player)
{
	return
		"size[8,9]"
		"list[current_name;0;0,0;8,4;]"
		"list[current_player;main;0,5;8,4;]";
}
bool ChestNodeMetadata::import(NodeMetadata *meta)
{
	if (meta->typeId() != CONTENT_LOCKABLE_CHEST)
		return false;
	LockingChestNodeMetadata *l = (LockingChestNodeMetadata*)meta;
	*m_inventory = *l->getInventory();
	return true;
}

/*
	LockingChestNodeMetadata
*/

// Prototype
LockingChestNodeMetadata proto_LockingChestNodeMetadata;

LockingChestNodeMetadata::LockingChestNodeMetadata()
{
	NodeMetadata::registerType(typeId(), create);

	m_inventory = new Inventory();
	m_inventory->addList("0", 8*4);
}
LockingChestNodeMetadata::~LockingChestNodeMetadata()
{
	delete m_inventory;
}
u16 LockingChestNodeMetadata::typeId() const
{
	return CONTENT_LOCKABLE_CHEST;
}
NodeMetadata* LockingChestNodeMetadata::create(std::istream &is)
{
	LockingChestNodeMetadata *d = new LockingChestNodeMetadata();
	d->setOwner(deSerializeString(is));
	d->m_inventory->deSerialize(is);
	return d;
}
NodeMetadata* LockingChestNodeMetadata::clone()
{
	LockingChestNodeMetadata *d = new LockingChestNodeMetadata();
	*d->m_inventory = *m_inventory;
	return d;
}
void LockingChestNodeMetadata::serializeBody(std::ostream &os)
{
	os<<serializeString(m_owner);
	m_inventory->serialize(os);
}
std::wstring LockingChestNodeMetadata::infoText()
{
	char buff[256];
	snprintf(buff, 256, gettext("Locking Chest owned by '%s'"), m_owner.c_str());
	return narrow_to_wide(buff);
}
bool LockingChestNodeMetadata::nodeRemovalDisabled()
{
	/*
		Disable removal if chest contains something
	*/
	InventoryList *list = m_inventory->getList("0");
	if(list == NULL)
		return false;
	if(list->getUsedSlots() == 0)
		return false;
	return true;
}
std::string LockingChestNodeMetadata::getDrawSpecString(Player *player)
{
	return
		"size[8,9]"
		"list[current_name;0;0,0;8,4;]"
		"list[current_player;main;0,5;8,4;]";
}
bool LockingChestNodeMetadata::import(NodeMetadata *meta)
{
	if (meta->typeId() != CONTENT_CHEST)
		return false;
	ChestNodeMetadata *l = (ChestNodeMetadata*)meta;
	*m_inventory = *l->getInventory();
	return true;
}

/*
	SafeNodeMetadata
*/

// Prototype
SafeNodeMetadata proto_SafeNodeMetadata;

SafeNodeMetadata::SafeNodeMetadata()
{
	NodeMetadata::registerType(typeId(), create);

	m_inventory = new Inventory();
	m_inventory->addList("0", 8*4);
}
SafeNodeMetadata::~SafeNodeMetadata()
{
	delete m_inventory;
}
u16 SafeNodeMetadata::typeId() const
{
	return CONTENT_SAFE;
}
NodeMetadata* SafeNodeMetadata::create(std::istream &is)
{
	SafeNodeMetadata *d = new SafeNodeMetadata();
	d->setOwner(deSerializeString(is));
	d->m_inventory->deSerialize(is);
	return d;
}
NodeMetadata* SafeNodeMetadata::clone()
{
	SafeNodeMetadata *d = new SafeNodeMetadata();
	*d->m_inventory = *m_inventory;
	return d;
}
void SafeNodeMetadata::serializeBody(std::ostream &os)
{
	os<<serializeString(m_owner);
	m_inventory->serialize(os);
}
std::wstring SafeNodeMetadata::infoText()
{
	char buff[256];
	snprintf(buff, 256, gettext("Safe owned by '%s'"), m_owner.c_str());
	return narrow_to_wide(buff);
}
bool SafeNodeMetadata::nodeRemovalDisabled()
{
	/*
		Disable removal if chest contains something
	*/
	InventoryList *list = m_inventory->getList("0");
	if(list == NULL)
		return false;
	if(list->getUsedSlots() == 0)
		return false;
	return true;
}
std::string SafeNodeMetadata::getDrawSpecString(Player *player)
{
	return
		"size[8,9]"
		"list[current_name;0;0,0;8,4;]"
		"list[current_player;main;0,5;8,4;]";
}
bool SafeNodeMetadata::import(NodeMetadata *meta)
{
	if (meta->typeId() != CONTENT_CHEST)
		return false;
	ChestNodeMetadata *l = (ChestNodeMetadata*)meta;
	*m_inventory = *l->getInventory();
	return true;
}
