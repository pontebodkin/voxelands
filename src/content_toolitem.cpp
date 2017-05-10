/************************************************************************
* content_toolitem.cpp
* voxelands - 3d voxel world sandbox game
* Copyright (C) Lisa Milne 2014 <lisa@ltmnet.com>
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
************************************************************************/

#include "content_toolitem.h"
#include "content_craftitem.h"
#include "content_clothesitem.h"
#include "content_mob.h"
#include "content_craft.h"
#include "content_list.h"
#include "content_mapnode.h"
#include "mineral.h"
#include <map>
#include "intl.h"
#include "enchantment.h"
#include "auth.h"

std::map<content_t,struct ToolItemFeatures> g_content_toolitem_features;

ToolItemFeatures & content_toolitem_features(content_t i)
{
	if ((i&CONTENT_TOOLITEM_MASK) != CONTENT_TOOLITEM_MASK)
		return g_content_toolitem_features[CONTENT_IGNORE];

	std::map<content_t,struct ToolItemFeatures>::iterator it = g_content_toolitem_features.find(i);
	if (it == g_content_toolitem_features.end())
		return g_content_toolitem_features[CONTENT_IGNORE];
	return it->second;
}

ToolItemFeatures & content_toolitem_features(std::string subname)
{
	for (std::map<content_t,struct ToolItemFeatures>::iterator i = g_content_toolitem_features.begin(); i!=g_content_toolitem_features.end(); i++) {
		if (i->second.name == subname)
			return i->second;
	}
	return g_content_toolitem_features[CONTENT_IGNORE];
}

int get_tool_use(tooluse_t *info, content_t target, uint16_t data, content_t toolid, uint16_t tooldata)
{
	ToolItemFeatures *t_features;
	bool type_match = false;
	bool is_material = false;
	bool is_mob = false;

	if (!info)
		return 1;

	info->wear = 1;
	info->diggable = false;
	info->data = 0.0;
	info->delay = 0.25;

	t_features = &content_toolitem_features(toolid);

	if ((target&CONTENT_TOOLITEM_MASK) == CONTENT_TOOLITEM_MASK) {
		return 1;
	}else if ((target&CONTENT_CLOTHESITEM_MASK) == CONTENT_CLOTHESITEM_MASK) {
		return 1;
	}else if ((target&CONTENT_CRAFTITEM_MASK) == CONTENT_CRAFTITEM_MASK) {
		return 1;
	}else if ((target&CONTENT_MOB_MASK) == CONTENT_MOB_MASK) {
/*		mobs should have some per-mob defense, or armour, or something
		MobFeatures *m_features;

		m_features = &content_mob_features(target);
*/
		is_mob = true;

		switch (t_features->type) {
		case TT_SPECIAL:
		case TT_SHOVEL:
		case TT_PICK:
		case TT_BUCKET:
		case TT_SHEAR:
		case TT_NONE:
			info->diggable = true;
			info->data = (5.0-t_features->diginfo.time)*0.5;
			info->delay = 1.0;
			break;
		case TT_AXE:
			type_match = true;
			info->diggable = true;
			info->data = (6.0-t_features->diginfo.time);
			info->delay = 1.0;
			break;
		case TT_SWORD:
			type_match = true;
			info->diggable = true;
			info->data = (6.0-t_features->diginfo.time)*0.9;
			info->delay = 0.5;
			break;
		case TT_SPEAR:
			type_match = true;
			info->diggable = true;
			info->data = (5.0-t_features->diginfo.time)*0.75;
			info->delay = 0.5;
			break;
		case TT_CLUB:
			type_match = true;
			info->diggable = true;
			info->data = (5.0-t_features->diginfo.time)*0.65;
			info->delay = 0.5;
			break;
		default:
			break;
		}
	}else{ /* material item */
		ContentFeatures *c_features;

		c_features = &content_features(target);

		is_material = true;
		info->diggable = true;
		if (
			(toolid&CONTENT_CLOTHESITEM_MASK) == CONTENT_CLOTHESITEM_MASK
			|| (c_features->type == CMT_STONE && t_features->type != TT_PICK)
			|| (c_features->type == CMT_TREE && t_features->type == TT_NONE)
		) {
			info->diggable = false;
		}

		switch (t_features->type) {
		case TT_SPECIAL:
			info->diggable = false;
			break;
		case TT_AXE:
			if (
				c_features->type == CMT_PLANT
				|| c_features->type == CMT_WOOD
				|| c_features->type == CMT_TREE
				|| c_features->type == CMT_GLASS
			) {
				type_match = true;
			}
			break;
		case TT_PICK:
			if (c_features->type == CMT_STONE && t_features->diginfo.level >= mineral_features(data).min_level) {
				type_match = true;
			}
			break;
		case TT_SHOVEL:
			if (c_features->type == CMT_DIRT) {
				type_match = true;
			}
			break;
		case TT_SWORD:
			if (c_features->type == CMT_PLANT)
				type_match = true;
			break;
		case TT_SHEAR:
			if (c_features->type == CMT_PLANT) {
				type_match = true;
			}
			break;
		case TT_BUCKET:
			if (c_features->type == CMT_LIQUID) {
				type_match = true;
			}
			break;
		case TT_SPEAR:
			if (c_features->type == CMT_DIRT)
				type_match = true;
			break;
		case TT_NONE:
		default:
			break;
		}

		if (type_match) {
			info->data = t_features->diginfo.time;
		}else{
			info->data = 4.0;
		}
	}

	if (type_match && data != 0 && info->diggable) {
		uint16_t r;
		EnchantmentInfo enchant;
		while (enchantment_get(&data,&enchant)) {
			switch (enchant.type) {
			case ENCHANTMENT_FAST:
				if (is_material) {
					info->data /= (float)((enchant.level*enchant.level)+1);
				}else if (is_mob) {
					info->delay /= (float)((enchant.level*enchant.level)+1);
				}
				break;
			case ENCHANTMENT_LONGLASTING:
			{
				r = myrand_range(0,100);
				if (r < (100/(enchant.level+1)))
					info->wear = 0;
				break;
			}
			case ENCHANTMENT_FLAME:
				r = myrand_range(0,100);
				if (r > 20)
					info->wear += 1;
				break;
			default:;
			}
		}
	}

	return 0;
}

std::string toolitem_overlay(content_t content, std::string ol)
{
	ToolItemFeatures t_features = content_toolitem_features(content);
	if (ol == "")
		return "";
	std::string base = "tool_overlay_";
	switch (t_features.type) {
	case TT_AXE:
		base += "axe_";
		break;
	case TT_PICK:
		base += "pick_";
		break;
	case TT_SHOVEL:
		base += "shovel_";
		break;
	case TT_SWORD:
		base += "sword_";
		break;
	case TT_SHEAR:
		base += "shear_";
		break;
	case TT_BUCKET:
		base += "bucket_";
		break;
	case TT_SPEAR:
		base += "spear_";
		break;
	case TT_SPECIAL:
	case TT_CLUB:
	case TT_NONE:
	default:
		return "";
		break;
	}

	base += ol;
	base += ".png";
	return base;
}

void content_toolitem_init()
{
	g_content_toolitem_features.clear();

	content_t i;
	ToolItemFeatures *f = NULL;

/* PICKS */

	i = CONTENT_TOOLITEM_SMALL_PICK;
	f = &g_content_toolitem_features[i];
	f->content = i;
	f->texture = "tool_smallpick.png";
	f->name = "WPick";
	f->description = gettext("Small Stone Pick");
	f->type = TT_PICK;
	f->diginfo.uses = 32;
	f->diginfo.time = 3.0;
	f->diginfo.level = 1;
	{
		content_t r[9] = {
			CONTENT_ROCK,	CONTENT_ROCK,			CONTENT_ROCK,
			CONTENT_IGNORE,	CONTENT_CRAFTITEM_WOOD_PLANK,	CONTENT_IGNORE,
			CONTENT_IGNORE,	CONTENT_IGNORE,			CONTENT_IGNORE
		};
		crafting::setRecipe(r,i,1);
		r[4] = CONTENT_CRAFTITEM_JUNGLE_PLANK;
		crafting::setRecipe(r,i,1);
	}
	content_list_add("craftguide",i,1,0);
	content_list_add("creative",i,1,0);

	i = CONTENT_TOOLITEM_STONEPICK;
	f = &g_content_toolitem_features[i];
	f->content = i;
	f->texture = "tool_stonepick.png";
	f->name = "STPick";
	f->description = gettext("Stone Pick");
	f->type = TT_PICK;
	f->diginfo.uses = 64;
	f->diginfo.time = 1.5;
	f->diginfo.level = 2;
	crafting::setPickRecipe(CONTENT_ROUGHSTONE,CONTENT_TOOLITEM_STONEPICK);
	content_list_add("craftguide",i,1,0);
	content_list_add("creative",i,1,0);

	i = CONTENT_TOOLITEM_FLINTPICK;
	f = &g_content_toolitem_features[i];
	f->content = i;
	f->texture = "tool_flintpick.png";
	f->name = "FPick";
	f->description = gettext("Flint Pick");
	f->type = TT_PICK;
	f->diginfo.uses = 128;
	f->diginfo.time = 1.75;
	f->diginfo.level = 2;
	crafting::setPickRecipe(CONTENT_CRAFTITEM_FLINT,CONTENT_TOOLITEM_FLINTPICK);
	content_list_add("craftguide",i,1,0);
	content_list_add("creative",i,1,0);

	i = CONTENT_TOOLITEM_STEELPICK;
	f = &g_content_toolitem_features[i];
	f->content = i;
	f->texture = "tool_steelpick.png";
	f->name = "SteelPick";
	f->description = gettext("Steel Pick");
	f->type = TT_PICK;
	f->diginfo.uses = 256;
	f->diginfo.time = 1.0;
	f->diginfo.level = 3;
	crafting::setPickRecipe(CONTENT_CRAFTITEM_STEEL_INGOT,CONTENT_TOOLITEM_STEELPICK);
	content_list_add("craftguide",i,1,0);
	content_list_add("creative",i,1,0);

	i = CONTENT_TOOLITEM_MITHRIL_RAW_PICK;
	f = &g_content_toolitem_features[i];
	f->content = i;
	f->texture = "tool_mithril_raw_pick.png";
	f->name = "mithril_raw_pick";
	f->description = gettext("Raw Mithril Pick");
	f->type = TT_PICK;
	f->diginfo.uses = 512;
	f->diginfo.time = 0.75;
	f->diginfo.level = 4;
	crafting::setPickRecipe(CONTENT_CRAFTITEM_MITHRIL_RAW,CONTENT_TOOLITEM_MITHRIL_RAW_PICK);
	content_list_add("craftguide",i,1,0);
	content_list_add("creative",i,1,0);

	i = CONTENT_TOOLITEM_MITHRIL_UNBOUND_PICK;
	f = &g_content_toolitem_features[i];
	f->content = i;
	f->texture = "tool_mithril_pick.png";
	f->name = "mithril_unbound_pick";
	f->description = gettext("Unbound Mithril Pick");
	f->type = TT_PICK;
	f->diginfo.uses = 1024;
	f->diginfo.time = 0.6;
	f->diginfo.level = 5;
	crafting::setPickRecipe(CONTENT_CRAFTITEM_MITHRIL_UNBOUND,CONTENT_TOOLITEM_MITHRIL_UNBOUND_PICK);
	content_list_add("craftguide",i,1,0);
	content_list_add("creative",i,1,0);

	i = CONTENT_TOOLITEM_MITHRIL_PICK;
	f = &g_content_toolitem_features[i];
	f->content = i;
	f->texture = "tool_mithril_pick.png";
	f->name = "mithril_pick";
	f->description = gettext("Mithril Pick");
	f->type = TT_PICK;
	f->param_type = CPT_ENCHANTMENT;
	f->diginfo.uses = 2048;
	f->diginfo.time = 0.4;
	f->diginfo.level = 5;
	f->has_punch_effect = false;
	crafting::setPickRecipe(CONTENT_CRAFTITEM_MITHRIL,CONTENT_TOOLITEM_MITHRIL_PICK);
	content_list_add("craftguide",i,1,0);

	i = CONTENT_TOOLITEM_CREATIVEPICK;
	f = &g_content_toolitem_features[i];
	f->content = i;
	f->texture = "tool_creativepick.png";
	f->name = "MesePick";
	f->description = gettext("Creative Pick");
	f->type = TT_PICK;
	f->diginfo.uses = 1000;
	f->diginfo.time = 0.1;
	f->diginfo.level = 4;
	f->has_punch_effect = false;
	content_list_add("player-creative",i,1,0);
	content_list_add("creative",i,1,0);

/* SHOVELS */

	i = CONTENT_TOOLITEM_TROWEL;
	f = &g_content_toolitem_features[i];
	f->content = i;
	f->texture = "tool_trowel.png";
	f->name = "WShovel";
	f->description = gettext("Stone Trowel");
	f->type = TT_SHOVEL;
	f->diginfo.uses = 32;
	f->diginfo.time = 3.0;
	f->diginfo.level = 1;
	crafting::set1over1Recipe(CONTENT_ROCK,CONTENT_CRAFTITEM_STICK,i);
	content_list_add("craftguide",i,1,0);
	content_list_add("creative",i,1,0);

	i = CONTENT_TOOLITEM_STONESHOVEL;
	f = &g_content_toolitem_features[i];
	f->content = i;
	f->texture = "tool_stoneshovel.png";
	f->name = "STShovel";
	f->description = gettext("Stone Shovel");
	f->type = TT_SHOVEL;
	f->diginfo.uses = 64;
	f->diginfo.time = 1.5;
	f->diginfo.level = 1;
	crafting::setShovelRecipe(CONTENT_ROUGHSTONE,CONTENT_TOOLITEM_STONESHOVEL);
	content_list_add("craftguide",i,1,0);
	content_list_add("creative",i,1,0);

	i = CONTENT_TOOLITEM_FLINTSHOVEL;
	f = &g_content_toolitem_features[i];
	f->content = i;
	f->texture = "tool_flintshovel.png";
	f->name = "FShovel";
	f->description = gettext("Flint Shovel");
	f->type = TT_SHOVEL;
	f->diginfo.uses = 128;
	f->diginfo.time = 1.75;
	f->diginfo.level = 2;
	crafting::setShovelRecipe(CONTENT_CRAFTITEM_FLINT,CONTENT_TOOLITEM_FLINTSHOVEL);
	content_list_add("craftguide",i,1,0);
	content_list_add("creative",i,1,0);

	i = CONTENT_TOOLITEM_STEELSHOVEL;
	f = &g_content_toolitem_features[i];
	f->content = i;
	f->texture = "tool_steelshovel.png";
	f->name = "SteelShovel";
	f->description = gettext("Steel Shovel");
	f->type = TT_SHOVEL;
	f->diginfo.uses = 256;
	f->diginfo.time = 1.0;
	f->diginfo.level = 3;
	crafting::setShovelRecipe(CONTENT_CRAFTITEM_STEEL_INGOT,CONTENT_TOOLITEM_STEELSHOVEL);
	content_list_add("craftguide",i,1,0);
	content_list_add("creative",i,1,0);

	i = CONTENT_TOOLITEM_MITHRIL_RAW_SHOVEL;
	f = &g_content_toolitem_features[i];
	f->content = i;
	f->texture = "tool_mithril_raw_shovel.png";
	f->name = "mithril_raw_shovel";
	f->description = gettext("Raw Mithril Shovel");
	f->type = TT_SHOVEL;
	f->diginfo.uses = 512;
	f->diginfo.time = 0.75;
	f->diginfo.level = 4;
	crafting::setShovelRecipe(CONTENT_CRAFTITEM_MITHRIL_RAW,CONTENT_TOOLITEM_MITHRIL_RAW_SHOVEL);
	content_list_add("craftguide",i,1,0);
	content_list_add("creative",i,1,0);

	i = CONTENT_TOOLITEM_MITHRIL_UNBOUND_SHOVEL;
	f = &g_content_toolitem_features[i];
	f->content = i;
	f->texture = "tool_mithril_shovel.png";
	f->name = "mithril_unbound_shovel";
	f->description = gettext("Unbound Mithril Shovel");
	f->type = TT_SHOVEL;
	f->diginfo.uses = 1024;
	f->diginfo.time = 0.6;
	f->diginfo.level = 5;
	crafting::setShovelRecipe(CONTENT_CRAFTITEM_MITHRIL_UNBOUND,CONTENT_TOOLITEM_MITHRIL_UNBOUND_SHOVEL);
	content_list_add("craftguide",i,1,0);
	content_list_add("creative",i,1,0);

	i = CONTENT_TOOLITEM_MITHRIL_SHOVEL;
	f = &g_content_toolitem_features[i];
	f->content = i;
	f->texture = "tool_mithril_shovel.png";
	f->name = "mithril_shovel";
	f->description = gettext("Mithril Shovel");
	f->type = TT_SHOVEL;
	f->param_type = CPT_ENCHANTMENT;
	f->diginfo.uses = 2048;
	f->diginfo.time = 0.4;
	f->diginfo.level = 5;
	crafting::setShovelRecipe(CONTENT_CRAFTITEM_MITHRIL,CONTENT_TOOLITEM_MITHRIL_SHOVEL);
	content_list_add("craftguide",i,1,0);

/* AXES */

	i = CONTENT_TOOLITEM_SMALL_AXE;
	f = &g_content_toolitem_features[i];
	f->content = i;
	f->texture = "tool_smallaxe.png";
	f->name = "WAxe";
	f->description = gettext("Small Stone Axe");
	f->type = TT_AXE;
	f->diginfo.uses = 32;
	f->diginfo.time = 3.0;
	f->diginfo.level = 1;
	{
		content_t r[9] = {
			CONTENT_ROCK,	CONTENT_ROCK,			CONTENT_IGNORE,
			CONTENT_IGNORE,	CONTENT_CRAFTITEM_STICK,	CONTENT_IGNORE,
			CONTENT_IGNORE,	CONTENT_IGNORE,			CONTENT_IGNORE
		};
		crafting::setRecipe(r,i,1);

		r[0] = CONTENT_IGNORE; r[2] = CONTENT_ROCK;
		crafting::setRecipe(r,i,1);
	}
	content_list_add("craftguide",i,1,0);
	content_list_add("creative",i,1,0);

	i = CONTENT_TOOLITEM_STONEAXE;
	f = &g_content_toolitem_features[i];
	f->content = i;
	f->texture = "tool_stoneaxe.png";
	f->name = "STAxe";
	f->description = gettext("Stone Axe");
	f->type = TT_AXE;
	f->diginfo.uses = 64;
	f->diginfo.time = 1.5;
	f->diginfo.level = 1;
	crafting::setAxeRecipe(CONTENT_ROUGHSTONE,CONTENT_TOOLITEM_STONEAXE);
	content_list_add("craftguide",i,1,0);
	content_list_add("creative",i,1,0);

	i = CONTENT_TOOLITEM_FLINTAXE;
	f = &g_content_toolitem_features[i];
	f->content = i;
	f->texture = "tool_flintaxe.png";
	f->name = "FAxe";
	f->description = gettext("Flint Axe");
	f->type = TT_AXE;
	f->diginfo.uses = 128;
	f->diginfo.time = 1.75;
	f->diginfo.level = 2;
	crafting::setAxeRecipe(CONTENT_CRAFTITEM_FLINT,CONTENT_TOOLITEM_FLINTAXE);
	content_list_add("craftguide",i,1,0);
	content_list_add("creative",i,1,0);

	i = CONTENT_TOOLITEM_STEELAXE;
	f = &g_content_toolitem_features[i];
	f->content = i;
	f->texture = "tool_steelaxe.png";
	f->name = "SteelAxe";
	f->description = gettext("Steel Axe");
	f->type = TT_AXE;
	f->diginfo.uses = 256;
	f->diginfo.time = 1.0;
	f->diginfo.level = 3;
	crafting::setAxeRecipe(CONTENT_CRAFTITEM_STEEL_INGOT,CONTENT_TOOLITEM_STEELAXE);
	content_list_add("craftguide",i,1,0);
	content_list_add("creative",i,1,0);

	i = CONTENT_TOOLITEM_MITHRIL_RAW_AXE;
	f = &g_content_toolitem_features[i];
	f->content = i;
	f->texture = "tool_mithril_raw_axe.png";
	f->name = "mithril_raw_axe";
	f->description = gettext("Raw Mithril Axe");
	f->type = TT_AXE;
	f->diginfo.uses = 512;
	f->diginfo.time = 0.75;
	f->diginfo.level = 4;
	crafting::setAxeRecipe(CONTENT_CRAFTITEM_MITHRIL_RAW,CONTENT_TOOLITEM_MITHRIL_RAW_AXE);
	content_list_add("craftguide",i,1,0);
	content_list_add("creative",i,1,0);

	i = CONTENT_TOOLITEM_MITHRIL_UNBOUND_AXE;
	f = &g_content_toolitem_features[i];
	f->content = i;
	f->texture = "tool_mithril_axe.png";
	f->name = "mithril_unbound_axe";
	f->description = gettext("Unbound Mithril Axe");
	f->type = TT_AXE;
	f->diginfo.uses = 1024;
	f->diginfo.time = 0.6;
	f->diginfo.level = 5;
	crafting::setAxeRecipe(CONTENT_CRAFTITEM_MITHRIL_UNBOUND,CONTENT_TOOLITEM_MITHRIL_UNBOUND_AXE);
	content_list_add("craftguide",i,1,0);
	content_list_add("creative",i,1,0);

	i = CONTENT_TOOLITEM_MITHRIL_AXE;
	f = &g_content_toolitem_features[i];
	f->content = i;
	f->texture = "tool_mithril_axe.png";
	f->name = "mithril_axe";
	f->description = gettext("Mithril Axe");
	f->type = TT_AXE;
	f->param_type = CPT_ENCHANTMENT;
	f->diginfo.uses = 2048;
	f->diginfo.time = 0.4;
	f->diginfo.level = 5;
	crafting::setAxeRecipe(CONTENT_CRAFTITEM_MITHRIL,CONTENT_TOOLITEM_MITHRIL_AXE);
	content_list_add("craftguide",i,1,0);

/* WEAPONS - CLUBS, BOWS */

	i = CONTENT_TOOLITEM_CLUB;
	f = &g_content_toolitem_features[i];
	f->content = i;
	f->texture = "tool_woodclub.png";
	f->name = "WSword";
	f->description = gettext("Wooden Club");
	f->type = TT_CLUB;
	f->diginfo.uses = 32;
	f->diginfo.time = 3.0;
	f->diginfo.level = 1;
	crafting::setCol1Recipe(CONTENT_CRAFTITEM_WOOD_PLANK,i);
	crafting::setCol1Recipe(CONTENT_CRAFTITEM_JUNGLE_PLANK,i);
	content_list_add("craftguide",i,1,0);
	content_list_add("creative",i,1,0);

	i = CONTENT_TOOLITEM_BOW;
	f = &g_content_toolitem_features[i];
	f->content = i;
	f->texture = "tool_bow.png";
	f->name = "bow";
	f->description = gettext("Bow");
	f->type = TT_SPECIAL;
	f->diginfo.uses = 256;
	f->diginfo.time = 1.0;
	f->diginfo.level = 2;
	f->thrown_item = CONTENT_CRAFTITEM_ARROW;
	{
		content_t r[9] = {
			CONTENT_CRAFTITEM_STRING,	CONTENT_CRAFTITEM_STICK,	CONTENT_IGNORE,
			CONTENT_CRAFTITEM_STRING,	CONTENT_IGNORE,			CONTENT_CRAFTITEM_STICK,
			CONTENT_CRAFTITEM_STRING,	CONTENT_CRAFTITEM_STICK,	CONTENT_IGNORE
		};
		crafting::setRecipe(r,i,1);
	}
	content_list_add("craftguide",i,1,0);
	content_list_add("creative",i,1,0);

/* SPEARS */

	i = CONTENT_TOOLITEM_STONESPEAR;
	f = &g_content_toolitem_features[i];
	f->content = i;
	f->texture = "tool_stonespear.png";
	f->name = "stone_spear";
	f->description = gettext("Stone Spear");
	f->type = TT_SPEAR;
	f->diginfo.uses = 64;
	f->diginfo.time = 1.5;
	f->diginfo.level = 1;
	crafting::setSpearRecipe(CONTENT_ROUGHSTONE,CONTENT_TOOLITEM_STONESPEAR);
	content_list_add("craftguide",i,1,0);
	content_list_add("creative",i,1,0);

	i = CONTENT_TOOLITEM_FLINTSPEAR;
	f = &g_content_toolitem_features[i];
	f->content = i;
	f->texture = "tool_flintspear.png";
	f->name = "flint_spear";
	f->description = gettext("Flint Spear");
	f->type = TT_SPEAR;
	f->diginfo.uses = 128;
	f->diginfo.time = 1.75;
	f->diginfo.level = 2;
	crafting::setSpearRecipe(CONTENT_CRAFTITEM_FLINT,CONTENT_TOOLITEM_FLINTSPEAR);
	content_list_add("craftguide",i,1,0);
	content_list_add("creative",i,1,0);

	i = CONTENT_TOOLITEM_STEELSPEAR;
	f = &g_content_toolitem_features[i];
	f->content = i;
	f->texture = "tool_steelspear.png";
	f->name = "steel_spear";
	f->description = gettext("Steel Spear");
	f->type = TT_SPEAR;
	f->diginfo.uses = 256;
	f->diginfo.time = 1.0;
	f->diginfo.level = 3;
	crafting::setSpearRecipe(CONTENT_CRAFTITEM_STEEL_INGOT,CONTENT_TOOLITEM_STEELSPEAR);
	content_list_add("craftguide",i,1,0);
	content_list_add("creative",i,1,0);

	i = CONTENT_TOOLITEM_MITHRIL_RAW_SPEAR;
	f = &g_content_toolitem_features[i];
	f->content = i;
	f->texture = "tool_mithril_raw_spear.png";
	f->name = "mithril_raw_spear";
	f->description = gettext("Raw Mithril Spear");
	f->type = TT_SPEAR;
	f->diginfo.uses = 512;
	f->diginfo.time = 0.75;
	f->diginfo.level = 4;
	crafting::setSpearRecipe(CONTENT_CRAFTITEM_MITHRIL_RAW,CONTENT_TOOLITEM_MITHRIL_RAW_SPEAR);
	content_list_add("craftguide",i,1,0);
	content_list_add("creative",i,1,0);

	i = CONTENT_TOOLITEM_MITHRIL_UNBOUND_SPEAR;
	f = &g_content_toolitem_features[i];
	f->content = i;
	f->texture = "tool_mithril_spear.png";
	f->name = "mithril_unbound_spear";
	f->description = gettext("Unbound Mithril Spear");
	f->type = TT_SPEAR;
	f->diginfo.uses = 1024;
	f->diginfo.time = 0.6;
	f->diginfo.level = 5;
	crafting::setSpearRecipe(CONTENT_CRAFTITEM_MITHRIL_UNBOUND,CONTENT_TOOLITEM_MITHRIL_UNBOUND_SPEAR);
	content_list_add("craftguide",i,1,0);
	content_list_add("creative",i,1,0);

	i = CONTENT_TOOLITEM_MITHRIL_SPEAR;
	f = &g_content_toolitem_features[i];
	f->content = i;
	f->texture = "tool_mithril_spear.png";
	f->name = "mithril_spear";
	f->description = gettext("Mithril Spear");
	f->type = TT_SPEAR;
	f->param_type = CPT_ENCHANTMENT;
	f->diginfo.uses = 2048;
	f->diginfo.time = 0.4;
	f->diginfo.level = 5;
	crafting::setSpearRecipe(CONTENT_CRAFTITEM_MITHRIL,CONTENT_TOOLITEM_MITHRIL_SPEAR);
	content_list_add("craftguide",i,1,0);

/* SWORDS */

	i = CONTENT_TOOLITEM_STONESWORD;
	f = &g_content_toolitem_features[i];
	f->content = i;
	f->texture = "tool_stonesword.png";
	f->name = "STSword";
	f->description = gettext("Stone Sword");
	f->type = TT_SWORD;
	f->diginfo.uses = 64;
	f->diginfo.time = 1.5;
	f->diginfo.level = 1;
	crafting::setSwordRecipe(CONTENT_ROUGHSTONE,CONTENT_TOOLITEM_STONESWORD);
	content_list_add("craftguide",i,1,0);
	content_list_add("creative",i,1,0);

	i = CONTENT_TOOLITEM_STEELSWORD;
	f = &g_content_toolitem_features[i];
	f->content = i;
	f->texture = "tool_steelsword.png";
	f->name = "SteelSword";
	f->description = gettext("Steel Sword");
	f->type = TT_SWORD;
	f->diginfo.uses = 256;
	f->diginfo.time = 1.0;
	f->diginfo.level = 3;
	crafting::setSwordRecipe(CONTENT_CRAFTITEM_STEEL_INGOT,CONTENT_TOOLITEM_STEELSWORD);
	content_list_add("craftguide",i,1,0);
	content_list_add("creative",i,1,0);

	i = CONTENT_TOOLITEM_MITHRIL_RAW_SWORD;
	f = &g_content_toolitem_features[i];
	f->content = i;
	f->texture = "tool_mithril_raw_sword.png";
	f->name = "mithril_raw_sword";
	f->description = gettext("Raw Mithril Sword");
	f->type = TT_SWORD;
	f->diginfo.uses = 512;
	f->diginfo.time = 0.75;
	f->diginfo.level = 4;
	crafting::setSwordRecipe(CONTENT_CRAFTITEM_MITHRIL_RAW,CONTENT_TOOLITEM_MITHRIL_RAW_SWORD);
	content_list_add("craftguide",i,1,0);
	content_list_add("creative",i,1,0);

	i = CONTENT_TOOLITEM_MITHRIL_UNBOUND_SWORD;
	f = &g_content_toolitem_features[i];
	f->content = i;
	f->texture = "tool_mithril_sword.png";
	f->name = "mithril_unbound_sword";
	f->description = gettext("Unbound Mithril Sword");
	f->type = TT_SWORD;
	f->diginfo.uses = 1024;
	f->diginfo.time = 0.6;
	f->diginfo.level = 5;
	crafting::setSwordRecipe(CONTENT_CRAFTITEM_MITHRIL_UNBOUND,CONTENT_TOOLITEM_MITHRIL_UNBOUND_SWORD);
	content_list_add("craftguide",i,1,0);
	content_list_add("creative",i,1,0);

	i = CONTENT_TOOLITEM_MITHRIL_SWORD;
	f = &g_content_toolitem_features[i];
	f->content = i;
	f->texture = "tool_mithril_sword.png";
	f->name = "mithril_sword";
	f->description = gettext("Mithril Sword");
	f->type = TT_SWORD;
	f->param_type = CPT_ENCHANTMENT;
	f->diginfo.uses = 2048;
	f->diginfo.time = 0.4;
	f->diginfo.level = 5;
	crafting::setSwordRecipe(CONTENT_CRAFTITEM_MITHRIL,CONTENT_TOOLITEM_MITHRIL_SWORD);
	content_list_add("craftguide",i,1,0);

/* SHEARS */

	i = CONTENT_TOOLITEM_FLINTSHEARS;
	f = &g_content_toolitem_features[i];
	f->content = i;
	f->texture = "tool_flintshears.png";
	f->name = "FShears";
	f->description = gettext("Flint Shears");
	f->type = TT_SHEAR;
	f->diginfo.uses = 128;
	f->diginfo.time = 1.5;
	f->diginfo.level = 2;
	crafting::setShearsRecipe(CONTENT_CRAFTITEM_FLINT,CONTENT_TOOLITEM_FLINTSHEARS);
	content_list_add("craftguide",i,1,0);
	content_list_add("creative",i,1,0);

	i = CONTENT_TOOLITEM_STEELSHEARS;
	f = &g_content_toolitem_features[i];
	f->content = i;
	f->texture = "tool_steelshears.png";
	f->name = "Shears";
	f->description = gettext("Steel Shears");
	f->type = TT_SHEAR;
	f->diginfo.uses = 256;
	f->diginfo.time = 1.0;
	f->diginfo.level = 2;
	crafting::setShearsRecipe(CONTENT_CRAFTITEM_STEEL_INGOT,CONTENT_TOOLITEM_STEELSHEARS);
	content_list_add("craftguide",i,1,0);
	content_list_add("creative",i,1,0);

/* BUCKETS */

	i = CONTENT_TOOLITEM_WBUCKET;
	f = &g_content_toolitem_features[i];
	f->content = i;
	f->texture = "tool_woodbucket.png";
	f->name = "WBucket";
	f->description = gettext("Wooden Bucket");
	f->liquids_pointable = true;
	f->type = TT_BUCKET;
	f->diginfo.uses = 64;
	f->diginfo.time = 1.5;
	f->diginfo.level = 1;
	f->damaging_nodes_diggable = false;
	crafting::setURecipe(CONTENT_CRAFTITEM_JUNGLE_PLANK,CONTENT_TOOLITEM_WBUCKET);
	crafting::setURecipe(CONTENT_CRAFTITEM_WOOD_PLANK,CONTENT_TOOLITEM_WBUCKET);
	content_list_add("craftguide",i,1,0);
	content_list_add("creative",i,1,0);

	i = CONTENT_TOOLITEM_TINBUCKET;
	f = &g_content_toolitem_features[i];
	f->content = i;
	f->texture = "tool_tinbucket.png";
	f->name = "TinBucket";
	f->description = gettext("Tin Bucket");
	f->liquids_pointable = true;
	f->type = TT_BUCKET;
	f->diginfo.uses = 128;
	f->diginfo.time = 1.75;
	f->diginfo.level = 2;
	f->damaging_nodes_diggable = false;
	crafting::setURecipe(CONTENT_CRAFTITEM_TIN_INGOT,CONTENT_TOOLITEM_TINBUCKET);
	content_list_add("craftguide",i,1,0);
	content_list_add("creative",i,1,0);

	i = CONTENT_TOOLITEM_STEELBUCKET;
	f = &g_content_toolitem_features[i];
	f->content = i;
	f->texture = "tool_steelbucket.png";
	f->name = "SteelBucket";
	f->description = gettext("Steel Bucket");
	f->liquids_pointable = true;
	f->type = TT_BUCKET;
	f->diginfo.uses = 256;
	f->diginfo.time = 1.0;
	f->diginfo.level = 3;
	crafting::setURecipe(CONTENT_CRAFTITEM_STEEL_INGOT,CONTENT_TOOLITEM_STEELBUCKET);
	content_list_add("craftguide",i,1,0);
	content_list_add("creative",i,1,0);

	i = CONTENT_TOOLITEM_WBUCKET_WATER;
	f = &g_content_toolitem_features[i];
	f->content = i;
	f->texture = "tool_woodbucket.png^bucket_water.png";
	f->name = "WBucket_water";
	f->description = gettext("Wooden Bucket of Water");
	f->type = TT_SPECIAL;
	f->onplace_node = CONTENT_WATERSOURCE;
	f->onplace_replace_item = CONTENT_TOOLITEM_WBUCKET;
	content_list_add("creative",i,1,0);

	i = CONTENT_TOOLITEM_TINBUCKET_WATER;
	f = &g_content_toolitem_features[i];
	f->content = i;
	f->texture = "tool_tinbucket.png^bucket_water.png";
	f->name = "TinBucket_water";
	f->description = gettext("Tin Bucket of Water");
	f->type = TT_SPECIAL;
	f->onplace_node = CONTENT_WATERSOURCE;
	f->onplace_replace_item = CONTENT_TOOLITEM_TINBUCKET;
	content_list_add("creative",i,1,0);

	i = CONTENT_TOOLITEM_STEELBUCKET_WATER;
	f = &g_content_toolitem_features[i];
	f->content = i;
	f->texture = "tool_steelbucket.png^bucket_water.png";
	f->name = "SteelBucket_water";
	f->description = gettext("Steel Bucket of Water");
	f->type = TT_SPECIAL;
	f->onplace_node = CONTENT_WATERSOURCE;
	f->onplace_replace_item = CONTENT_TOOLITEM_STEELBUCKET;
	content_list_add("creative",i,1,0);

	i = CONTENT_TOOLITEM_STEELBUCKET_LAVA;
	f = &g_content_toolitem_features[i];
	f->content = i;
	f->texture = "tool_steelbucket.png^bucket_lava.png";
	f->name = "SteelBucket_lava";
	f->description = gettext("Steel Bucket of Lava");
	f->onplace_node = CONTENT_LAVASOURCE;
	f->onplace_replace_item = CONTENT_TOOLITEM_STEELBUCKET;
	f->fuel_time = 80;
	f->type = TT_SPECIAL;
	content_list_add("creative",i,1,0);

/* SPECIAL TOOLS */

	i = CONTENT_TOOLITEM_FIRESTARTER;
	f = &g_content_toolitem_features[i];
	f->content = i;
	f->texture = "tool_fire_starter.png";
	f->name = "FireStarter";
	f->description = gettext("Fire Starter");
	f->liquids_pointable = true;
	f->type = TT_SPECIAL;
	f->diginfo.level = 3;
	f->has_fire_effect = true;
	crafting::set1Any2Recipe(CONTENT_CRAFTITEM_FLINT,CONTENT_CRAFTITEM_STEEL_INGOT,CONTENT_TOOLITEM_FIRESTARTER);
	content_list_add("craftguide",i,1,0);
	content_list_add("creative",i,1,0);

	i = CONTENT_TOOLITEM_CROWBAR;
	f = &g_content_toolitem_features[i];
	f->content = i;
	f->texture = "crowbar.png";
	f->name = "crowbar";
	f->description = gettext("Crowbar");
	f->type = TT_SPECIAL;
	f->diginfo.level = 3;
	f->has_rotate_effect = true;
	crafting::set1over1Recipe(CONTENT_CRAFTITEM_STEEL_INGOT,CONTENT_CRAFTITEM_STEEL_INGOT,CONTENT_TOOLITEM_CROWBAR);
	content_list_add("craftguide",i,1,0);
	content_list_add("creative",i,1,0);

	i = CONTENT_TOOLITEM_KEY;
	f = &g_content_toolitem_features[i];
	f->content = i;
	f->texture = "tool_key.png";
	f->name = "key";
	f->description = gettext("Key");
	f->type = TT_SPECIAL;
	f->diginfo.level = 4;
	f->has_unlock_effect = true;
	crafting::set1To1Recipe(CONTENT_CRAFTITEM_GOLD_INGOT,CONTENT_TOOLITEM_KEY);
	content_list_add("craftguide",i,1,0);

	i = CONTENT_TOOLITEM_MITHRIL_KEY;
	f = &g_content_toolitem_features[i];
	f->content = i;
	f->texture = "tool_mithril_key.png";
	f->name = "mithrilkey";
	f->description = gettext("Mithril Key");
	f->type = TT_SPECIAL;
	f->diginfo.level = 5;
	f->has_unlock_effect = true;
	f->has_super_unlock_effect = true;
	/* this can only be crafted by server admin */
	{
		content_t r[9] = {
			CONTENT_CRAFTITEM_MITHRIL_UNBOUND,	CONTENT_IGNORE,				CONTENT_IGNORE,
			CONTENT_IGNORE,				CONTENT_IGNORE,				CONTENT_IGNORE,
			CONTENT_IGNORE,				CONTENT_IGNORE,				CONTENT_IGNORE
		};
		crafting::setRecipe(r,i,1,PRIV_SERVER);
	}

	i = CONTENT_TOOLITEM_MOB_SPAWNER;
	f = &g_content_toolitem_features[i];
	f->content = i;
	f->texture = "tool_mob_spawner.png";
	f->name = "mob_spawner";
	f->description = gettext("Mob Spawner");
	f->type = TT_SPECIAL;
	f->diginfo.level = 4;
	f->param_type = CPT_DROP;
	{
		content_t r[9] = {
			CONTENT_IGNORE,				CONTENT_CRAFTITEM_MITHRIL_UNBOUND,	CONTENT_IGNORE,
			CONTENT_CRAFTITEM_MITHRIL_UNBOUND,	CONTENT_IGNORE,				CONTENT_CRAFTITEM_MITHRIL_UNBOUND,
			CONTENT_IGNORE,				CONTENT_CRAFTITEM_MITHRIL_UNBOUND,	CONTENT_IGNORE
		};
		crafting::setRecipe(r,i,1);
	}
	content_list_add("craftguide",i,1,0);
}
