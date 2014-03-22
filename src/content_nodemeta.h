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

#ifndef CONTENT_NODEMETA_HEADER
#define CONTENT_NODEMETA_HEADER

#include "nodemetadata.h"

class ServerEnvironment;
class Inventory;

class SignNodeMetadata : public NodeMetadata
{
public:
	SignNodeMetadata(std::string text);
	//~SignNodeMetadata();

	virtual u16 typeId() const;
	static NodeMetadata* create(std::istream &is);
	virtual NodeMetadata* clone();
	virtual void serializeBody(std::ostream &os);
	virtual std::string infoText();

	std::string getText(){ return m_text; }
	void setText(std::string t){ m_text = t; }
	virtual bool receiveFields(std::string formname, std::map<std::string, std::string> fields, Player *player)
	{
		m_text = fields["text"];
		return true;
	}
	virtual std::string getDrawSpecString();

private:
	std::string m_text;
};

class LockingSignNodeMetadata : public NodeMetadata
{
public:
	LockingSignNodeMetadata(std::string text);
	//~LockingSignNodeMetadata();

	virtual u16 typeId() const;
	static NodeMetadata* create(std::istream &is);
	virtual NodeMetadata* clone();
	virtual void serializeBody(std::ostream &os);
	virtual std::string infoText();

	virtual std::string getOwner(){ return m_owner; }
	virtual void setOwner(std::string t){ m_owner = t; }
	virtual std::string getInventoryOwner(){ return m_owner; }
	virtual void setInventoryOwner(std::string t){ m_owner = t; }

	std::string getText(){ return m_text; }
	void setText(std::string t){ m_text = t; }
	virtual bool receiveFields(std::string formname, std::map<std::string, std::string> fields, Player *player);
	virtual std::string getDrawSpecString();

private:
	std::string m_text;
	std::string m_owner;
};

class ChestNodeMetadata : public NodeMetadata
{
public:
	ChestNodeMetadata();
	~ChestNodeMetadata();

	virtual u16 typeId() const;
	static NodeMetadata* create(std::istream &is);
	virtual NodeMetadata* clone();
	virtual void serializeBody(std::ostream &os);
	virtual std::string infoText();
	virtual Inventory* getInventory() {return m_inventory;}
	virtual bool nodeRemovalDisabled();
	virtual std::string getDrawSpecString();

private:
	Inventory *m_inventory;
};

class LockingChestNodeMetadata : public NodeMetadata
{
public:
	LockingChestNodeMetadata();
	~LockingChestNodeMetadata();

	virtual u16 typeId() const;
	static NodeMetadata* create(std::istream &is);
	virtual NodeMetadata* clone();
	virtual void serializeBody(std::ostream &os);
	virtual std::string infoText();
	virtual Inventory* getInventory() {return m_inventory;}
	virtual bool nodeRemovalDisabled();
	virtual std::string getDrawSpecString();

	virtual std::string getOwner(){ return m_owner; }
	virtual void setOwner(std::string t){ m_owner = t; }
	virtual std::string getInventoryOwner(){ return m_owner; }
	virtual void setInventoryOwner(std::string t){ m_owner = t; }

private:
	Inventory *m_inventory;
	std::string m_owner;
};

class CreativeChestNodeMetadata : public NodeMetadata
{
public:
	CreativeChestNodeMetadata();
	~CreativeChestNodeMetadata();

	virtual u16 typeId() const;
	static NodeMetadata* create(std::istream &is);
	virtual NodeMetadata* clone();
	virtual void serializeBody(std::ostream &os);
	virtual std::string infoText();
	virtual Inventory* getInventory() {return m_inventory;}
	virtual bool nodeRemovalDisabled();
	virtual bool receiveFields(std::string formname, std::map<std::string, std::string> fields, Player *player);
	virtual std::string getDrawSpecString();

private:
	u16 m_page;
	Inventory *m_inventory;
};

class BorderStoneNodeMetadata : public NodeMetadata
{
public:
	BorderStoneNodeMetadata();
	~BorderStoneNodeMetadata();

	virtual u16 typeId() const;
	static NodeMetadata* create(std::istream &is);
	virtual NodeMetadata* clone();
	virtual void serializeBody(std::ostream &os);
	virtual std::string infoText();

	virtual std::string getOwner(){ return m_text; }
	virtual void setOwner(std::string t){ m_text = t; }

private:
	std::string m_text;
};

class LockedDoorNodeMetadata : public NodeMetadata
{
public:
	LockedDoorNodeMetadata();
	~LockedDoorNodeMetadata();

	virtual u16 typeId() const;
	static NodeMetadata* create(std::istream &is);
	virtual NodeMetadata* clone();
	virtual void serializeBody(std::ostream &os);
	virtual std::string infoText();

	virtual std::string getOwner(){ return m_text; }
	virtual void setOwner(std::string t){ m_text = t; }

private:
	std::string m_text;
};

class FurnaceNodeMetadata : public NodeMetadata
{
public:
	FurnaceNodeMetadata();
	~FurnaceNodeMetadata();

	virtual u16 typeId() const;
	virtual NodeMetadata* clone();
	static NodeMetadata* create(std::istream &is);
	virtual void serializeBody(std::ostream &os);
	virtual std::string infoText();
	virtual Inventory* getInventory() {return m_inventory;}
	virtual void inventoryModified();
	virtual bool step(float dtime, v3s16 pos, ServerEnvironment *env);
	virtual bool nodeRemovalDisabled();
	virtual std::string getDrawSpecString();

private:
	Inventory *m_inventory;
	float m_step_accumulator;
	float m_fuel_totaltime;
	float m_fuel_time;
	float m_src_totaltime;
	float m_src_time;
};

class LockingFurnaceNodeMetadata : public NodeMetadata
{
public:
	LockingFurnaceNodeMetadata();
	~LockingFurnaceNodeMetadata();

	virtual u16 typeId() const;
	virtual NodeMetadata* clone();
	static NodeMetadata* create(std::istream &is);
	virtual void serializeBody(std::ostream &os);
	virtual std::string infoText();
	virtual Inventory* getInventory() {return m_inventory;}
	virtual void inventoryModified();
	virtual bool step(float dtime, v3s16 pos, ServerEnvironment *env);
	virtual bool nodeRemovalDisabled();
	virtual std::string getDrawSpecString();

	virtual std::string getOwner(){ return m_owner; }
	virtual void setOwner(std::string t){ m_owner = t; }
	virtual std::string getInventoryOwner(){ return m_inv_owner; }
	virtual void setInventoryOwner(std::string t){ m_inv_owner = t; }

private:
	Inventory *m_inventory;
	float m_step_accumulator;
	float m_fuel_totaltime;
	float m_fuel_time;
	float m_src_totaltime;
	float m_src_time;
	std::string m_owner;
	std::string m_inv_owner;
	float m_lock;
};

class TNTNodeMetadata : public NodeMetadata
{
public:
	TNTNodeMetadata();
	~TNTNodeMetadata();

	virtual u16 typeId() const;
	static NodeMetadata* create(std::istream &is);
	virtual NodeMetadata* clone();
	virtual void serializeBody(std::ostream &os);
	virtual std::string infoText();
	virtual bool step(float dtime, v3s16 pos, ServerEnvironment *env);

	virtual bool energise(u8 level, v3s16 powersrc, v3s16 signalsrc, v3s16 pos)
	{
		if (level == 0) {
			m_armed = false;
			return true;
		}
		m_armed = true;
		m_time = 5.0;
		return true;
	}
	virtual u8 getEnergy()
	{
		if (!m_armed)
			return 0;
		return ENERGY_MAX-((u8)m_time);
	}

private:
	bool m_armed;
	float m_time;
};

class IncineratorNodeMetadata : public NodeMetadata
{
public:
	IncineratorNodeMetadata();
	~IncineratorNodeMetadata();

	virtual u16 typeId() const;
	virtual NodeMetadata* clone();
	static NodeMetadata* create(std::istream &is);
	virtual void serializeBody(std::ostream &os);
	virtual std::string infoText();
	virtual Inventory* getInventory() {return m_inventory;}
	virtual void inventoryModified();
	virtual bool nodeRemovalDisabled();
	virtual std::string getDrawSpecString();

private:
	Inventory *m_inventory;
};


class CraftGuideNodeMetadata : public NodeMetadata
{
public:
	CraftGuideNodeMetadata();
	~CraftGuideNodeMetadata();

	virtual u16 typeId() const;
	NodeMetadata* clone();
	static NodeMetadata* create(std::istream &is);
	virtual void serializeBody(std::ostream &os);
	virtual std::string infoText() {return std::string("Craft Guide");}
	virtual Inventory* getInventory() {return m_inventory;}
	virtual bool nodeRemovalDisabled();
	virtual void inventoryModified();
	virtual bool step(float dtime, v3s16 pos, ServerEnvironment *env);
	virtual bool import(NodeMetadata *meta);
	virtual bool receiveFields(std::string formname, std::map<std::string, std::string> fields, Player *player);
	virtual std::string getDrawSpecString();

	u16 getPage() {return m_page;}

private:
	Inventory *m_inventory;
	u16 m_page;
};

class BookNodeMetadata : public NodeMetadata
{
public:
	BookNodeMetadata();

	virtual u16 typeId() const;
	NodeMetadata* clone();
	static NodeMetadata* create(std::istream &is);
	virtual void serializeBody(std::ostream &os);
	virtual std::string infoText() {return m_title;}
	virtual bool nodeRemovalDisabled();
	virtual bool import(NodeMetadata *meta);
	virtual bool receiveFields(std::string formname, std::map<std::string, std::string> fields, Player *player);
	virtual std::string getDrawSpecString();

	std::string getContent() { return m_content; }

private:
	std::string m_title;
	std::string m_content;
};

class DiaryNodeMetadata : public NodeMetadata
{
public:
	DiaryNodeMetadata();

	virtual u16 typeId() const;
	NodeMetadata* clone();
	static NodeMetadata* create(std::istream &is);
	virtual void serializeBody(std::ostream &os);
	virtual std::string infoText() {return m_title;}
	virtual bool nodeRemovalDisabled();
	virtual bool import(NodeMetadata *meta);
	virtual bool receiveFields(std::string formname, std::map<std::string, std::string> fields, Player *player);
	virtual std::string getDrawSpecString();

	virtual std::string getOwner(){ return m_owner; }
	virtual void setOwner(std::string t){ m_owner = t; }
	std::string getContent() { return m_content; }

private:
	std::string m_owner;
	std::string m_title;
	std::string m_content;
};

class ClosedBookNodeMetadata : public NodeMetadata
{
public:
	ClosedBookNodeMetadata();

	virtual u16 typeId() const;
	NodeMetadata* clone();
	static NodeMetadata* create(std::istream &is);
	virtual void serializeBody(std::ostream &os);
	virtual std::string infoText() {return m_title;}
	virtual bool nodeRemovalDisabled();
	virtual bool import(NodeMetadata *meta);

	virtual std::string getOwner(){ return m_owner; }
	std::string getContent() { return m_content; }
	u16 getPage() {return m_page;}

private:
	std::string m_owner;
	std::string m_title;
	std::string m_content;
	u16 m_page;
};

#endif

