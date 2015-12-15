/************************************************************************
* guiMultiplayerMenu.cpp
* voxelands - 3d voxel world sandbox game
* Copyright (C) Lisa 'darkrose' Milne 2013-2015 <lisa@ltmnet.com>
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

#include "guiMultiplayerMenu.h"
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
#include "settings.h"
#include "gui_colours.h"

GUIMultiplayerMenu::GUIMultiplayerMenu(
	gui::IGUIEnvironment* env,
	gui::IGUIElement*
	parent,
	s32 id,
	MainMenuData *data,
	IMenuManager *menumgr,
	IGameCallback *gamecallback
):
	GUIModalMenu(env, parent, id, menumgr),
	m_accepted(false),
	m_gamecallback(gamecallback)
{
	m_data.mmdata = data;
}

GUIMultiplayerMenu::~GUIMultiplayerMenu()
{
	removeChildren();
}

void GUIMultiplayerMenu::removeChildren()
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

void GUIMultiplayerMenu::regenerateGui(v2u32 screensize)
{
	std::wstring text_name;
	std::wstring text_address;
	std::wstring text_port;

	m_screensize = screensize;

	// Client options
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_NAME_INPUT);
		if (e != NULL) {
			text_name = e->getText();
		}else{
			text_name = m_data.mmdata->name;
		}
		if (text_name == L"")
			text_name = narrow_to_wide(porting::getUser());
	}
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_ADDRESS_INPUT);
		if (e != NULL) {
			text_address = e->getText();
		}else{
			text_address = m_data.mmdata->address;
		}
	}
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_PORT_INPUT);
		if (e != NULL) {
			text_port = e->getText();
		}else{
			text_port = m_data.mmdata->port;
		}
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
		Environment->addButton(rect, this, GUI_ID_TAB_MP_MAINMENU, wgettext("Main Menu"));
	}

	// Dynamic List button
	{
		core::rect<s32> rect(0, 0, 180, 40);
		rect += v2s32(35, 260);
		Environment->addButton(rect, this, GUI_ID_TAB_MP_LIST, wgettext("All Servers"));
	}
	// Favourites List button
	{
		core::rect<s32> rect(0, 0, 180, 40);
		rect += v2s32(35, 305);
		Environment->addButton(rect, this, GUI_ID_TAB_MP_FAVOURITES, wgettext("Favourite Servers"));
	}
	// Custom Connect button (the old multiplayer menu)
	{
		core::rect<s32> rect(0, 0, 180, 40);
		rect += v2s32(35, 350);
		Environment->addButton(rect, this, GUI_ID_TAB_MP_CUSTOM, wgettext("Custom Connect"));
	}

	v2s32 topleft_content(250, 0);
	if (m_data.selected_tab == TAB_MP_CUSTOM) {
		{
			core::rect<s32> rect(0, 0, 550, 20);
			rect += topleft_content + v2s32(0, 20);
			gui::IGUIStaticText *t = Environment->addStaticText(wgettext("Multi Player"), rect, false, true, this, -1);
			t->setTextAlignment(gui::EGUIA_CENTER, gui::EGUIA_UPPERLEFT);
		}

		// Nickname + password
		{
			core::rect<s32> rect(0, 0, 110, 20);
			rect += topleft_content + v2s32(120, 60);
			Environment->addStaticText(wgettext("Name/Password"), rect, false, true, this, -1);
		}
		{
			core::rect<s32> rect(0, 0, 230, 30);
			rect += topleft_content + v2s32(135, 90);
#if USE_FREETYPE
			new gui::intlGUIEditBox(text_name.c_str(), true, Environment, this, GUI_ID_NAME_INPUT, rect);
#else
			Environment->addEditBox(text_name.c_str(), rect, false, this, GUI_ID_NAME_INPUT);
#endif
		}
		{
			core::rect<s32> rect(0, 0, 230, 30);
			rect += topleft_content + v2s32(135, 125);
			gui::IGUIEditBox *e;
#if USE_FREETYPE
			e = (gui::IGUIEditBox *) new gui::intlGUIEditBox(L"", true, Environment, this, GUI_ID_PW_INPUT, rect);
#else
			e = Environment->addEditBox(L"", rect, false, this, GUI_ID_PW_INPUT);
#endif
			e->setPasswordBox(true);
			Environment->setFocus(e);

		}
		// Address + port
		{
			core::rect<s32> rect(0, 0, 110, 20);
			rect += topleft_content + v2s32(120, 170);
			Environment->addStaticText(wgettext("Address/Port"), rect, false, true, this, -1);
		}
		{
			if (text_address == L"")
				text_address = L"servers.voxelands.com";
			core::rect<s32> rect(0, 0, 230, 30);
			rect += topleft_content + v2s32(135, 200);
#if USE_FREETYPE
			new gui::intlGUIEditBox(text_address.c_str(), true, Environment, this, GUI_ID_ADDRESS_INPUT, rect);
#else
			Environment->addEditBox(text_address.c_str(), rect, false, this, GUI_ID_ADDRESS_INPUT);
#endif
		}
		{
			core::rect<s32> rect(0, 0, 120, 30);
			rect += topleft_content + v2s32(245, 240);
#if USE_FREETYPE
			new gui::intlGUIEditBox(text_port.c_str(), true, Environment, this, GUI_ID_PORT_INPUT, rect);
#else
			Environment->addEditBox(text_port.c_str(), rect, false, this, GUI_ID_PORT_INPUT);
#endif
		}
		// Start game button
		{
			core::rect<s32> rect(0, 0, 180, 30);
			rect += topleft_content + v2s32(160, 290);
			Environment->addButton(rect, this, GUI_ID_CONNECT_BUTTON, wgettext("Connect"));
		}
	}else if (m_data.selected_tab == TAB_MP_FAVOURITES) {
	}else{
	}
}

void GUIMultiplayerMenu::drawMenu()
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

bool GUIMultiplayerMenu::acceptInput()
{
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_NAME_INPUT);
		if (e != NULL)
			m_data.mmdata->name = e->getText();
		if (m_data.mmdata->name == L"")
			m_data.mmdata->name = narrow_to_wide(porting::getUser());
	}
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_PW_INPUT);
		if (e != NULL)
			m_data.mmdata->password = e->getText();
	}
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_ADDRESS_INPUT);
		if (e != NULL)
			m_data.mmdata->address = e->getText();
	}
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_PORT_INPUT);
		if (e != NULL)
			m_data.mmdata->port = e->getText();
	}
	return true;
}
bool GUIMultiplayerMenu::OnEvent(const SEvent& event)
{
	if (event.EventType==EET_KEY_INPUT_EVENT) {
		if (
			event.KeyInput.PressedDown
			&& (
				event.KeyInput.Key == KEY_ESCAPE
				|| event.KeyInput.Key == KEY_RETURN
			)
		) {
			acceptInput();
			quitMenu();
			return true;
		}
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
			switch (event.GUIEvent.Caller->getID()) {
			case GUI_ID_CONNECT_BUTTON: // Start game
				acceptInput();
				m_gamecallback->startGame();
				quitMenu();
				return true;
			case GUI_ID_TAB_MP_LIST:
				acceptInput();
				m_accepted = false;
				m_data.selected_tab = TAB_MP_LIST;
				regenerateGui(m_screensize);
				return true;
			case GUI_ID_TAB_MP_FAVOURITES:
				acceptInput();
				m_accepted = false;
				m_data.selected_tab = TAB_MP_FAVOURITES;
				regenerateGui(m_screensize);
				return true;
			case GUI_ID_TAB_MP_CUSTOM:
				acceptInput();
				m_accepted = false;
				m_data.selected_tab = TAB_MP_CUSTOM;
				regenerateGui(m_screensize);
				return true;
			case GUI_ID_TAB_MP_MAINMENU: //back
				acceptInput();
				quitMenu();
				return true;
			}
		}
		if (event.GUIEvent.EventType == gui::EGET_SCROLL_BAR_CHANGED) {
			//switch (event.GUIEvent.Caller->getID()) {
			//}
		}
		if (event.GUIEvent.EventType == gui::EGET_EDITBOX_ENTER) {
			switch (event.GUIEvent.Caller->getID()) {
			case GUI_ID_ADDRESS_INPUT:
			case GUI_ID_PORT_INPUT:
			case GUI_ID_NAME_INPUT:
			case GUI_ID_PW_INPUT:
				m_gamecallback->startGame();
				acceptInput();
				quitMenu();
				return true;
			}
		}
	}
	return Parent ? Parent->OnEvent(event) : false;
}

