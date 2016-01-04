/************************************************************************
* Minetest-c55
* Copyright (C) 2010-2011 celeron55, Perttu Ahola <celeron55@gmail.com>
* Copyright (C) 2011 Ciaran Gultnieks <ciaran@ciarang.com>
* Copyright (C) 2011 teddydestodes <derkomtur@schattengang.net>
*
* guiSettingsMenu.cpp
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

#include "guiSettingsMenu.h"
#include "debug.h"
#include "serialization.h"
#include "main.h"
#include <string>
#include <IGUICheckBox.h>
#include <IGUIEditBox.h>
#include <IGUIButton.h>
#include <IGUIStaticText.h>
#include <IGUIFont.h>
#include <IGUIScrollBar.h>
#include <IGUIComboBox.h>
#include "settings.h"
#include "gui_colours.h"

GUISettingsMenu::GUISettingsMenu(
	gui::IGUIEnvironment* env,
	gui::IGUIElement*
	parent,
	s32 id,
	IMenuManager *menumgr,
	bool is_ingame
):
	GUIModalMenu(env, parent, id, menumgr),
	m_accepted(false),
	m_is_ingame(is_ingame)
{
	activeKey = -1;
	init_keys();
	m_data.mesh_detail = g_settings->getU16("mesh_detail");
	m_data.texture_detail = g_settings->getU16("texture_detail");
	m_data.light_detail = g_settings->getU16("light_detail");
	m_data.fullscreen = g_settings->getBool("fullscreen");
	m_data.particles = g_settings->getBool("enable_particles");
	m_data.mip_map = g_settings->getBool("mip_map");
	m_data.anisotropic_filter = g_settings->getBool("anisotropic_filter");
	m_data.bilinear_filter = g_settings->getBool("bilinear_filter");
	m_data.trilinear_filter = g_settings->getBool("trilinear_filter");
	m_data.hotbar = g_settings->getBool("old_hotbar");
	m_data.wield_index = g_settings->getBool("enable_wieldindex");
	m_data.volume = g_settings->getFloat("sound_volume");

	keynames[VLKC_FORWARD] = wgettext("Forward");
	keynames[VLKC_BACKWARD] = wgettext("Backward");
	keynames[VLKC_LEFT] = wgettext("Left");
	keynames[VLKC_RIGHT] = wgettext("Right");
	keynames[VLKC_JUMP] = wgettext("Jump");
	keynames[VLKC_SNEAK] = wgettext("Sneak");
	keynames[VLKC_INVENTORY] = wgettext("Inventory");
	keynames[VLKC_USE] = wgettext("Use Item");
	keynames[VLKC_CHAT] = wgettext("Chat");
	keynames[VLKC_COMMAND] = wgettext("Command");
	keynames[VLKC_RANGE] = wgettext("Range Select");
	keynames[VLKC_FREEMOVE] = wgettext("Toggle Fly");
	keynames[VLKC_UP] = wgettext("Up");
	keynames[VLKC_DOWN] = wgettext("Down");
	keynames[VLKC_RUN] = wgettext("Run");
	keynames[VLKC_EXAMINE] = wgettext("Examine/Open");
	keynames[VLKC_SCREENSHOT] = wgettext("Take Screenshot");
	keynames[VLKC_TOGGLE_HUD] = wgettext("Show/Hide HUD");
	keynames[VLKC_TOGGLE_CHAT] = wgettext("Show/Hide Chat");
	keynames[VLKC_TOGGLE_FOG] = wgettext("Toggle Fog");
	keynames[VLKC_TOGGLE_CAMERA] = NULL;
	keynames[VLKC_TOGGLE_DEBUG] = NULL;
	keynames[VLKC_TOGGLE_PROFILER] = NULL;
	keynames[VLKC_RANGE_PLUS] = wgettext("Increase Viewing Range");
	keynames[VLKC_RANGE_MINUS] = wgettext("Decrease Viewing Range");
	keynames[VLKC_PRINT_DEBUG] = NULL;
	keynames[VLKC_SELECT_PREV] = wgettext("Previous Item");
	keynames[VLKC_SELECT_NEXT] = wgettext("Next Item");
}

GUISettingsMenu::~GUISettingsMenu()
{
	removeChildren();
}

void GUISettingsMenu::removeChildren()
{
	const core::list<gui::IGUIElement*> &children = getChildren();
	core::list<gui::IGUIElement*> children_copy;
	for (core::list<gui::IGUIElement*>::ConstIterator i = children.begin(); i != children.end(); i++) {
		children_copy.push_back(*i);
	}
	for (core::list<gui::IGUIElement*>::Iterator i = children_copy.begin(); i != children_copy.end(); i++) {
		(*i)->remove();
	}
}

void GUISettingsMenu::save()
{
	// controls
	int m = VLKC_MAX;
	for (int i=0; i<m; i++) {
		saveKeySetting(keys[i],(KeyCode)i);
	}
	// graphics
	g_settings->set("mesh_detail", itos(m_data.mesh_detail));
	g_settings->set("texture_detail", itos(m_data.texture_detail));
	g_settings->set("light_detail", itos(m_data.light_detail));
	g_settings->set("old_hotbar", itos(m_data.hotbar));
	g_settings->set("enable_wieldindex", itos(m_data.wield_index));
	// video
	g_settings->set("mip_map", itos(m_data.mip_map));
	g_settings->set("anisotropic_filter", itos(m_data.anisotropic_filter));
	g_settings->set("bilinear_filter", itos(m_data.bilinear_filter));
	g_settings->set("trilinear_filter", itos(m_data.trilinear_filter));
	g_settings->set("fullscreen", itos(m_data.fullscreen));
	g_settings->set("enable_particles", itos(m_data.particles));
	Environment->getVideoDriver()->setTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS, m_data.mip_map);
	// sound
	g_settings->set("sound_volume",ftos(m_data.volume));
}

void GUISettingsMenu::regenerateGui(v2u32 screensize)
{
	u16 mesh_detail;
	u16 texture_detail;
	u16 light_detail;
	bool fullscreen;
	bool particles;
	bool mipmap;
	bool bilinear;
	bool trilinear;
	bool anisotropic;
	bool hotbar;
	bool wield_index;
	f32 volume;

	m_screensize = screensize;

	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_MESH_DETAIL_COMBO);
		if (e != NULL && e->getType() == gui::EGUIET_COMBO_BOX) {
			gui::IGUIComboBox *c = (gui::IGUIComboBox*)e;
			switch (c->getItemData(c->getSelected())) {
			case GUI_ID_MESH_DETAIL_LOW:
				mesh_detail = 1;
				break;
			case GUI_ID_MESH_DETAIL_MEDIUM:
				mesh_detail = 2;
				break;
			case GUI_ID_MESH_DETAIL_HIGH:
				mesh_detail = 3;
				break;
			default:
				mesh_detail = 3;
			}
		}else{
			mesh_detail = m_data.mesh_detail;
		}
	}
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_TEXTURE_DETAIL_COMBO);
		if (e != NULL && e->getType() == gui::EGUIET_COMBO_BOX) {
			gui::IGUIComboBox *c = (gui::IGUIComboBox*)e;
			switch (c->getItemData(c->getSelected())) {
			case GUI_ID_TEXTURE_DETAIL_LOW:
				texture_detail = 1;
				break;
			case GUI_ID_TEXTURE_DETAIL_MEDIUM:
				texture_detail = 2;
				break;
			case GUI_ID_TEXTURE_DETAIL_HIGH:
				texture_detail = 3;
				break;
			default:
				texture_detail = 3;
			}
		}else{
			texture_detail = m_data.texture_detail;
		}
	}
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_LIGHT_DETAIL_COMBO);
		if (e != NULL && e->getType() == gui::EGUIET_COMBO_BOX) {
			gui::IGUIComboBox *c = (gui::IGUIComboBox*)e;
			switch (c->getItemData(c->getSelected())) {
			case GUI_ID_LIGHT_DETAIL_LOW:
				light_detail = 1;
				break;
			case GUI_ID_LIGHT_DETAIL_MEDIUM:
				light_detail = 2;
				break;
			case GUI_ID_LIGHT_DETAIL_HIGH:
				light_detail = 3;
				break;
			default:
				light_detail = 3;
			}
		}else{
			light_detail = m_data.light_detail;
		}
	}
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_FULLSCREEN_CB);
		if(e != NULL && e->getType() == gui::EGUIET_CHECK_BOX)
			fullscreen = ((gui::IGUICheckBox*)e)->isChecked();
		else
			fullscreen = m_data.fullscreen;
	}
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_PARTICLES_CB);
		if(e != NULL && e->getType() == gui::EGUIET_CHECK_BOX)
			particles = ((gui::IGUICheckBox*)e)->isChecked();
		else
			particles = m_data.particles;
	}
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_MIPMAP_CB);
		if(e != NULL && e->getType() == gui::EGUIET_CHECK_BOX)
			mipmap = ((gui::IGUICheckBox*)e)->isChecked();
		else
			mipmap = m_data.mip_map;
	}
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_BILINEAR_CB);
		if(e != NULL && e->getType() == gui::EGUIET_CHECK_BOX)
			bilinear = ((gui::IGUICheckBox*)e)->isChecked();
		else
			bilinear = m_data.bilinear_filter;
	}
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_TRILINEAR_CB);
		if(e != NULL && e->getType() == gui::EGUIET_CHECK_BOX)
			trilinear = ((gui::IGUICheckBox*)e)->isChecked();
		else
			trilinear = m_data.trilinear_filter;
	}
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_ANISOTROPIC_CB);
		if(e != NULL && e->getType() == gui::EGUIET_CHECK_BOX)
			anisotropic = ((gui::IGUICheckBox*)e)->isChecked();
		else
			anisotropic = m_data.anisotropic_filter;
	}
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_HOTBAR_CB);
		if(e != NULL && e->getType() == gui::EGUIET_CHECK_BOX)
			hotbar = ((gui::IGUICheckBox*)e)->isChecked();
		else
			hotbar = m_data.hotbar;
	}
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_WIELDINDEX_CB);
		if(e != NULL && e->getType() == gui::EGUIET_CHECK_BOX)
			wield_index = ((gui::IGUICheckBox*)e)->isChecked();
		else
			wield_index = m_data.wield_index;
	}
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_VOLUME_SB);
		if(e != NULL && e->getType() == gui::EGUIET_SCROLL_BAR)
			volume = (float)((gui::IGUIScrollBar*)e)->getPos();
		else
			volume = m_data.volume;
	}
	/*
	 Remove stuff
	 */
	removeChildren();

	/*
	 Calculate new sizes and positions
	 */

	v2s32 size(800, 500);

	core::rect<s32> rect(
			screensize.X/2 - size.X/2,
			screensize.Y/2 - size.Y/2,
			screensize.X/2 + size.X/2,
			screensize.Y/2 + size.Y/2
	);

	DesiredRect = rect;
	recalculateAbsolutePosition(false);

	// Main Menu button
	{
		core::rect<s32> rect(0, 0, 200, 40);
		rect += v2s32(25, 200);
		Environment->addButton(rect, this, GUI_ID_TAB_MAINMENU, wgettext("Main Menu"));
	}

	// Controls Settings button
	{
		core::rect<s32> rect(0, 0, 180, 40);
		rect += v2s32(35, 260);
		Environment->addButton(rect, this, GUI_ID_TAB_SETTINGS_CONTROLS, wgettext("Controls"));
	}
	// Graphics Settings button
	{
		core::rect<s32> rect(0, 0, 180, 40);
		rect += v2s32(35, 305);
		Environment->addButton(rect, this, GUI_ID_TAB_SETTINGS_GRAPHICS, wgettext("Graphics"));
	}
	// Video Settings button
	{
		core::rect<s32> rect(0, 0, 180, 40);
		rect += v2s32(35, 350);
		Environment->addButton(rect, this, GUI_ID_TAB_SETTINGS_VIDEO, wgettext("Video"));
	}
	// Sound Settings button
	{
		core::rect<s32> rect(0, 0, 180, 40);
		rect += v2s32(35, 395);
		Environment->addButton(rect, this, GUI_ID_TAB_SETTINGS_SOUND, wgettext("Sound"));
	}

	v2s32 topleft_content(250, 0);

	if (m_data.selected_tab == TAB_SETTINGS_CONTROLS) {
		{
			core::rect<s32> rect(0, 0, 550, 20);
			rect += topleft_content + v2s32(0, 20);
			gui::IGUIStaticText *t = Environment->addStaticText(wgettext("Controls"), rect, false, true, this, -1);
			t->setTextAlignment(gui::EGUIA_CENTER, gui::EGUIA_UPPERLEFT);
		}

		v2s32 offset(0, 40);

		for (int i=0; i<VLKC_MAX; i++) {
			if (keynames[i] == NULL)
				continue;
			{
				core::rect < s32 > rect(0, 0, 150, 20);
				rect += topleft_content + offset;
				gui::IGUIStaticText *t = Environment->addStaticText(keynames[i], rect, false, true, this, -1);
				t->setTextAlignment(gui::EGUIA_LOWERRIGHT, gui::EGUIA_UPPERLEFT);
			}

			{
				core::rect < s32 > rect(0, 0, 110, 30);
				rect += topleft_content + offset + v2s32(155, -5);
				Environment->addButton(rect, this, GUI_ID_KEYSETTINGS_BASE+i, keys[i].guiName().c_str());
			}
			offset += v2s32(0, 33);
			if (offset.Y > 450) {
				offset.Y = 40;
				offset.X = 275;
			}
		}
	}else if (m_data.selected_tab == TAB_SETTINGS_GRAPHICS) {
		{
			core::rect<s32> rect(0, 0, 550, 20);
			rect += topleft_content + v2s32(0, 20);
			gui::IGUIStaticText *t = Environment->addStaticText(wgettext("Graphics"), rect, false, true, this, -1);
			t->setTextAlignment(gui::EGUIA_CENTER, gui::EGUIA_UPPERLEFT);
		}
		{
			core::rect<s32> rect(0, 0, 125, 20);
			rect += topleft_content + v2s32(40, 60);
			gui::IGUIStaticText *t = Environment->addStaticText(wgettext("Terrain Mesh Detail"), rect, false, true, this, -1);
			t->setTextAlignment(gui::EGUIA_LOWERRIGHT, gui::EGUIA_UPPERLEFT);
		}
		{
			core::rect<s32> rect(0, 0, 240, 25);
			rect += topleft_content + v2s32(175, 55);
			gui::IGUIComboBox *c = Environment->addComboBox(rect, this, GUI_ID_MESH_DETAIL_COMBO);
			u32 ld = c->addItem(wgettext("Low"),GUI_ID_MESH_DETAIL_LOW);
			u32 md = c->addItem(wgettext("Medium"),GUI_ID_MESH_DETAIL_MEDIUM);
			u32 hd = c->addItem(wgettext("High"),GUI_ID_MESH_DETAIL_HIGH);
			switch (mesh_detail) {
			case 1:
				c->setSelected(ld);
				break;
			case 2:
				c->setSelected(md);
				break;
			default:
				c->setSelected(hd);
				break;
			}
		}
		{
			core::rect<s32> rect(0, 0, 125, 20);
			rect += topleft_content + v2s32(40, 90);
			gui::IGUIStaticText *t = Environment->addStaticText(wgettext("Texture Detail"), rect, false, true, this, -1);
			t->setTextAlignment(gui::EGUIA_LOWERRIGHT, gui::EGUIA_UPPERLEFT);
		}
		{
			core::rect<s32> rect(0, 0, 240, 25);
			rect += topleft_content + v2s32(175, 85);
			gui::IGUIComboBox *c = Environment->addComboBox(rect, this, GUI_ID_TEXTURE_DETAIL_COMBO);
			u32 ld = c->addItem(wgettext("Low"),GUI_ID_TEXTURE_DETAIL_LOW);
			u32 md = c->addItem(wgettext("Medium"),GUI_ID_TEXTURE_DETAIL_MEDIUM);
			u32 hd = c->addItem(wgettext("High"),GUI_ID_TEXTURE_DETAIL_HIGH);
			switch (texture_detail) {
			case 1:
				c->setSelected(ld);
				break;
			case 2:
				c->setSelected(md);
				break;
			default:
				c->setSelected(hd);
				break;
			}
		}
		{
			core::rect<s32> rect(0, 0, 125, 20);
			rect += topleft_content + v2s32(40, 120);
			gui::IGUIStaticText *t = Environment->addStaticText(wgettext("Light Detail"), rect, false, true, this, -1);
			t->setTextAlignment(gui::EGUIA_LOWERRIGHT, gui::EGUIA_UPPERLEFT);
		}
		{
			core::rect<s32> rect(0, 0, 240, 25);
			rect += topleft_content + v2s32(175, 115);
			gui::IGUIComboBox *c = Environment->addComboBox(rect, this, GUI_ID_LIGHT_DETAIL_COMBO);
			u32 ld = c->addItem(wgettext("Low"),GUI_ID_LIGHT_DETAIL_LOW);
			u32 md = c->addItem(wgettext("Medium"),GUI_ID_LIGHT_DETAIL_MEDIUM);
			u32 hd = c->addItem(wgettext("High"),GUI_ID_LIGHT_DETAIL_HIGH);
			switch (light_detail) {
			case 1:
				c->setSelected(ld);
				break;
			case 2:
				c->setSelected(md);
				break;
			default:
				c->setSelected(hd);
				break;
			}
		}
		{
			core::rect<s32> rect(0, 0, 200, 30);
			rect += topleft_content + v2s32(80, 160);
			Environment->addCheckBox(hotbar, rect, this, GUI_ID_HOTBAR_CB, wgettext("Classic HUD"));
		}
		{
			core::rect<s32> rect(0, 0, 200, 30);
			rect += topleft_content + v2s32(290, 160);
			gui::IGUICheckBox *c = Environment->addCheckBox(wield_index, rect, this, GUI_ID_WIELDINDEX_CB, wgettext("Wieldring Index"));
			c->setEnabled(!hotbar);
		}
		if (m_is_ingame) {
			core::rect<s32> rect(0, 0, 550, 20);
			rect += topleft_content + v2s32(0, 220);
			gui::IGUIStaticText *t = Environment->addStaticText(wgettext("Some settings cannot be changed in-game."), rect, false, true, this, -1);
			t->setTextAlignment(gui::EGUIA_CENTER, gui::EGUIA_UPPERLEFT);
		}
	}else if (m_data.selected_tab == TAB_SETTINGS_VIDEO) {
		{
			core::rect<s32> rect(0, 0, 550, 20);
			rect += topleft_content + v2s32(0, 20);
			gui::IGUIStaticText *t = Environment->addStaticText(wgettext("Video"), rect, false, true, this, -1);
			t->setTextAlignment(gui::EGUIA_CENTER, gui::EGUIA_UPPERLEFT);
		}
		{
			core::rect<s32> rect(0, 0, 200, 30);
			rect += topleft_content + v2s32(80, 60);
			Environment->addCheckBox(fullscreen, rect, this, GUI_ID_FULLSCREEN_CB, wgettext("Fullscreen"));
		}
		{
			core::rect<s32> rect(0, 0, 200, 30);
			rect += topleft_content + v2s32(80, 90);
			Environment->addCheckBox(particles, rect, this, GUI_ID_PARTICLES_CB, wgettext("Particles"));
		}
		{
			core::rect<s32> rect(0, 0, 200, 30);
			rect += topleft_content + v2s32(80, 120);
			Environment->addCheckBox(mipmap, rect, this, GUI_ID_MIPMAP_CB, wgettext("Mip-Mapping"));
		}
		{
			core::rect<s32> rect(0, 0, 200, 30);
			rect += topleft_content + v2s32(80, 150);
			Environment->addCheckBox(bilinear, rect, this, GUI_ID_BILINEAR_CB, wgettext("Bi-Linear Filtering"));
		}
		{
			core::rect<s32> rect(0, 0, 200, 30);
			rect += topleft_content + v2s32(80, 180);
			Environment->addCheckBox(trilinear, rect, this, GUI_ID_TRILINEAR_CB, wgettext("Tri-Linear Filtering"));
		}
		{
			core::rect<s32> rect(0, 0, 200, 30);
			rect += topleft_content + v2s32(80, 210);
			Environment->addCheckBox(anisotropic, rect, this, GUI_ID_ANISOTROPIC_CB, wgettext("Anisotropic Filtering"));
		}
		if (m_is_ingame) {
			core::rect<s32> rect(0, 0, 550, 20);
			rect += topleft_content + v2s32(0, 250);
			gui::IGUIStaticText *t = Environment->addStaticText(wgettext("Some settings cannot be changed in-game."), rect, false, true, this, -1);
			t->setTextAlignment(gui::EGUIA_CENTER, gui::EGUIA_UPPERLEFT);
		}
	}else if (m_data.selected_tab == TAB_SETTINGS_SOUND) {
		{
			core::rect<s32> rect(0, 0, 550, 20);
			rect += topleft_content + v2s32(0, 20);
			gui::IGUIStaticText *t = Environment->addStaticText(wgettext("Sound"), rect, false, true, this, -1);
			t->setTextAlignment(gui::EGUIA_CENTER, gui::EGUIA_UPPERLEFT);
		}
		{
			core::rect<s32> rect(0, 0, 200, 15);
			rect += topleft_content + v2s32(80, 60);
			Environment->addStaticText(wgettext("Volume:"), rect, false, false, this, -1);
		}
		{
			core::rect<s32> rect(0, 0, 200, 15);
			rect += topleft_content + v2s32(290, 60);
			gui::IGUIScrollBar *sb = Environment->addScrollBar(true, rect, this, GUI_ID_VOLUME_SB);
			sb->setMin(0);
			sb->setMax(100);
			sb->setPos(volume);
		}
	}
}

void GUISettingsMenu::drawMenu()
{
	video::IVideoDriver* driver = Environment->getVideoDriver();

	{
		core::rect<s32> left(
			AbsoluteRect.UpperLeftCorner.X,
			AbsoluteRect.UpperLeftCorner.Y,
			AbsoluteRect.LowerRightCorner.X-550,
			AbsoluteRect.LowerRightCorner.Y
		);
		core::rect<s32> right(
			AbsoluteRect.UpperLeftCorner.X+250,
			AbsoluteRect.UpperLeftCorner.Y,
			AbsoluteRect.LowerRightCorner.X,
			AbsoluteRect.LowerRightCorner.Y
		);
		driver->draw2DRectangle(left, GUI_BG_BTM, GUI_BG_BTM, GUI_BG_BTM, GUI_BG_BTM, &AbsoluteClippingRect);
		driver->draw2DRectangle(right, GUI_BG_TOP, GUI_BG_BTM, GUI_BG_TOP, GUI_BG_BTM, &AbsoluteClippingRect);
		video::ITexture *texture = driver->getTexture(getTexturePath("menulogo.png").c_str());
		if (texture != 0) {
			const core::dimension2d<u32>& img_origsize = texture->getOriginalSize();
			core::rect<s32> logo(
				AbsoluteRect.UpperLeftCorner.X+25,
				AbsoluteRect.UpperLeftCorner.Y,
				AbsoluteRect.UpperLeftCorner.X+225,
				AbsoluteRect.UpperLeftCorner.Y+200
			);
			const video::SColor color(255,255,255,255);
			const video::SColor colors[] = {color,color,color,color};
			driver->draw2DImage(texture, logo, core::rect<s32>(core::position2d<s32>(0,0),img_origsize), NULL, colors, true);
		}
	}

	gui::IGUIElement::draw();
}

bool GUISettingsMenu::acceptInput()
{
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_MESH_DETAIL_COMBO);
		if (e != NULL && e->getType() == gui::EGUIET_COMBO_BOX) {
			gui::IGUIComboBox *c = (gui::IGUIComboBox*)e;
			switch (c->getItemData(c->getSelected())) {
			case GUI_ID_MESH_DETAIL_LOW:
				m_data.mesh_detail = 1;
				break;
			case GUI_ID_MESH_DETAIL_MEDIUM:
				m_data.mesh_detail = 2;
				break;
			case GUI_ID_MESH_DETAIL_HIGH:
				m_data.mesh_detail = 3;
				break;
			default:
				m_data.mesh_detail = 3;
			}
		}
	}
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_TEXTURE_DETAIL_COMBO);
		if (e != NULL && e->getType() == gui::EGUIET_COMBO_BOX) {
			gui::IGUIComboBox *c = (gui::IGUIComboBox*)e;
			switch (c->getItemData(c->getSelected())) {
			case GUI_ID_TEXTURE_DETAIL_LOW:
				m_data.texture_detail = 1;
				break;
			case GUI_ID_TEXTURE_DETAIL_MEDIUM:
				m_data.texture_detail = 2;
				break;
			case GUI_ID_TEXTURE_DETAIL_HIGH:
				m_data.texture_detail = 3;
				break;
			default:
				m_data.texture_detail = 3;
			}
		}
	}
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_LIGHT_DETAIL_COMBO);
		if (e != NULL && e->getType() == gui::EGUIET_COMBO_BOX) {
			gui::IGUIComboBox *c = (gui::IGUIComboBox*)e;
			switch (c->getItemData(c->getSelected())) {
			case GUI_ID_LIGHT_DETAIL_LOW:
				m_data.light_detail = 1;
				break;
			case GUI_ID_LIGHT_DETAIL_MEDIUM:
				m_data.light_detail = 2;
				break;
			case GUI_ID_LIGHT_DETAIL_HIGH:
				m_data.light_detail = 3;
				break;
			default:
				m_data.light_detail = 3;
			}
		}
	}
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_FULLSCREEN_CB);
		if(e != NULL && e->getType() == gui::EGUIET_CHECK_BOX)
			m_data.fullscreen = ((gui::IGUICheckBox*)e)->isChecked();
	}
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_PARTICLES_CB);
		if(e != NULL && e->getType() == gui::EGUIET_CHECK_BOX)
			m_data.particles = ((gui::IGUICheckBox*)e)->isChecked();
	}
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_MIPMAP_CB);
		if(e != NULL && e->getType() == gui::EGUIET_CHECK_BOX)
			m_data.mip_map = ((gui::IGUICheckBox*)e)->isChecked();
	}
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_BILINEAR_CB);
		if(e != NULL && e->getType() == gui::EGUIET_CHECK_BOX)
			m_data.bilinear_filter = ((gui::IGUICheckBox*)e)->isChecked();
	}
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_TRILINEAR_CB);
		if(e != NULL && e->getType() == gui::EGUIET_CHECK_BOX)
			m_data.trilinear_filter = ((gui::IGUICheckBox*)e)->isChecked();
	}
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_ANISOTROPIC_CB);
		if(e != NULL && e->getType() == gui::EGUIET_CHECK_BOX)
			m_data.anisotropic_filter = ((gui::IGUICheckBox*)e)->isChecked();
	}
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_HOTBAR_CB);
		if(e != NULL && e->getType() == gui::EGUIET_CHECK_BOX)
			m_data.hotbar = ((gui::IGUICheckBox*)e)->isChecked();
	}
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_WIELDINDEX_CB);
		if(e != NULL && e->getType() == gui::EGUIET_CHECK_BOX)
			m_data.wield_index = ((gui::IGUICheckBox*)e)->isChecked();
	}
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_VOLUME_SB);
		if(e != NULL && e->getType() == gui::EGUIET_SCROLL_BAR)
			m_data.volume = (float)((gui::IGUIScrollBar*)e)->getPos();
	}
	return true;
}
void GUISettingsMenu::init_keys()
{
	int m = VLKC_MAX;
	for (int i=0; i<m; i++) {
		keys[i] = getKeySetting((KeyCode)i);
	}
}

bool GUISettingsMenu::resetMenu()
{
	if (activeKey >= 0) {
		gui::IGUIElement *e = getElementFromId(activeKey);
		if (e != NULL && e->getType() == gui::EGUIET_BUTTON) {
			e->setEnabled(true);
			e->setText(keynames[activeKey-GUI_ID_KEYSETTINGS_BASE]);
		}
		activeKey = -1;
		return false;
	}
	return true;
}
bool GUISettingsMenu::OnEvent(const SEvent& event)
{
	if (event.EventType == EET_KEY_INPUT_EVENT && activeKey >= 0 && event.KeyInput.PressedDown) {
		KeyPress kp(event.KeyInput);
		gui::IGUIElement *e = getElementFromId(activeKey);
		if (e != NULL && e->getType() == gui::EGUIET_BUTTON) {
			e->setEnabled(true);
			e->setText(kp.guiName().c_str());
			keys[activeKey-GUI_ID_KEYSETTINGS_BASE] = kp;
		}
		activeKey = -1;
		return true;
	}
	if (event.EventType == EET_GUI_EVENT) {
		if (event.GUIEvent.EventType == gui::EGET_ELEMENT_FOCUS_LOST && isVisible()) {
			if (!canTakeFocus(event.GUIEvent.Element)) {
				dstream << "GUIMainMenu: Not allowing focus change."
						<< std::endl;
				// Returning true disables focus change
				return true;
			}
		}
		if (event.GUIEvent.EventType==gui::EGET_CHECKBOX_CHANGED) {
			acceptInput();
			m_accepted = false;
			regenerateGui(m_screensize);
		}
		if (event.GUIEvent.EventType == gui::EGET_BUTTON_CLICKED) {
			s32 id = event.GUIEvent.Caller->getID();
			if (id >= GUI_ID_KEYSETTINGS_BASE) {
				resetMenu();
				activeKey = id;
				gui::IGUIElement *e = getElementFromId(id);
				if (e != NULL && e->getType() == gui::EGUIET_BUTTON) {
					e->setText(wgettext("press Key"));
					e->setEnabled(false);
					return true;
				}
			}else{
				switch (event.GUIEvent.Caller->getID()) {
				case GUI_ID_TAB_SETTINGS_CONTROLS:
					acceptInput();
					m_accepted = false;
					m_data.selected_tab = TAB_SETTINGS_CONTROLS;
					regenerateGui(m_screensize);
					return true;
				case GUI_ID_TAB_SETTINGS_GRAPHICS:
					acceptInput();
					m_accepted = false;
					m_data.selected_tab = TAB_SETTINGS_GRAPHICS;
					regenerateGui(m_screensize);
					return true;
				case GUI_ID_TAB_SETTINGS_VIDEO:
					acceptInput();
					m_accepted = false;
					m_data.selected_tab = TAB_SETTINGS_VIDEO;
					regenerateGui(m_screensize);
					return true;
				case GUI_ID_TAB_SETTINGS_SOUND:
					acceptInput();
					m_accepted = false;
					m_data.selected_tab = TAB_SETTINGS_SOUND;
					regenerateGui(m_screensize);
					return true;
				case GUI_ID_TAB_MAINMENU: //back
					acceptInput();
					save();
					quitMenu();
					return true;
				}
			}
		}
		if (event.GUIEvent.EventType == gui::EGET_SCROLL_BAR_CHANGED) {
			switch (event.GUIEvent.Caller->getID()) {
			case GUI_ID_VOLUME_SB:
				gui::IGUIElement *vsb = getElementFromId(GUI_ID_VOLUME_SB);
				if(vsb != NULL && vsb->getType() == gui::EGUIET_SCROLL_BAR) {
					m_data.volume = (float)((gui::IGUIScrollBar*)vsb)->getPos();
					if (g_sound)
						g_sound->setListenerGain(m_data.volume/100.0);
				}
				return true;
			}
		}
		if (event.GUIEvent.EventType == gui::EGET_COMBO_BOX_CHANGED) {
			switch (event.GUIEvent.Caller->getID()) {
			case GUI_ID_MESH_DETAIL_COMBO:
			case GUI_ID_TEXTURE_DETAIL_COMBO:
			case GUI_ID_LIGHT_DETAIL_COMBO:
				acceptInput();
				m_accepted = false;
				regenerateGui(m_screensize);
				return true;
			}
		}
	}
	return Parent ? Parent->OnEvent(event) : false;
}

