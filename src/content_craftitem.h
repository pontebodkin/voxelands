#ifndef CONTENT_CRAFTITEM_HEADER
#define CONTENT_CRAFTITEM_HEADER

#include "mapnode.h"

#define CONTENT_CRAFTITEM_MASK 0x8000

struct CraftItemFeatures {
	content_t content;
	std::string texture;
	// the old 'subname'
	std::string name;
	// tooltip used in inventory
	std::string gui_name;
	// the result of cooking this item
	std::string cook_result;
	// the fuel value of this item
	float fuel_time;
	// 0 if inedible, otherwise the value it improves hp by
	s16 edible;
	// the number dropped on right click, -1 for all
	s16 drop_count;

	CraftItemFeatures():
		content(CONTENT_IGNORE),
		texture("unknown_item.png"),
		name(""),
		gui_name(""),
		cook_result(""),
		fuel_time(0.0),
		edible(0),
		drop_count(-1)
	{}
};

void content_craftitem_init();
CraftItemFeatures & content_craftitem_features(content_t i);
CraftItemFeatures & content_craftitem_features(std::string subname);

#define CONTENT_CRAFTITEM_PAPER (CONTENT_CRAFTITEM_MASK | 0x01)
#define CONTENT_CRAFTITEM_BOOK (CONTENT_CRAFTITEM_MASK | 0x02)
#define CONTENT_CRAFTITEM_CHARCOAL (CONTENT_CRAFTITEM_MASK | 0x03)
#define CONTENT_CRAFTITEM_COAL (CONTENT_CRAFTITEM_MASK | 0x04)
#define CONTENT_CRAFTITEM_IRON (CONTENT_CRAFTITEM_MASK | 0x05)
#define CONTENT_CRAFTITEM_CLAY (CONTENT_CRAFTITEM_MASK | 0x06)
#define CONTENT_CRAFTITEM_TIN (CONTENT_CRAFTITEM_MASK | 0x07)
#define CONTENT_CRAFTITEM_COPPER (CONTENT_CRAFTITEM_MASK | 0x08)
#define CONTENT_CRAFTITEM_SILVER (CONTENT_CRAFTITEM_MASK | 0x09)
#define CONTENT_CRAFTITEM_GOLD (CONTENT_CRAFTITEM_MASK | 0x0A)
#define CONTENT_CRAFTITEM_QUARTZ (CONTENT_CRAFTITEM_MASK | 0x0B)
#define CONTENT_CRAFTITEM_TIN_INGOT (CONTENT_CRAFTITEM_MASK | 0x0C)
#define CONTENT_CRAFTITEM_COPPER_INGOT (CONTENT_CRAFTITEM_MASK | 0x0D)
#define CONTENT_CRAFTITEM_SILVER_INGOT (CONTENT_CRAFTITEM_MASK | 0x0E)
#define CONTENT_CRAFTITEM_GOLD_INGOT (CONTENT_CRAFTITEM_MASK | 0x0F)
#define CONTENT_CRAFTITEM_FLINT (CONTENT_CRAFTITEM_MASK | 0x10)
#define CONTENT_CRAFTITEM_STEEL_INGOT (CONTENT_CRAFTITEM_MASK | 0x11)
#define CONTENT_CRAFTITEM_CLAY_BRICK (CONTENT_CRAFTITEM_MASK | 0x12)
#define CONTENT_CRAFTITEM_RAT (CONTENT_CRAFTITEM_MASK | 0x13)
#define CONTENT_CRAFTITEM_COOKED_RAT (CONTENT_CRAFTITEM_MASK | 0x14)
#define CONTENT_CRAFTITEM_SCORCHED_STUFF (CONTENT_CRAFTITEM_MASK | 0x15)
#define CONTENT_CRAFTITEM_FIREFLY (CONTENT_CRAFTITEM_MASK | 0x16)
#define CONTENT_CRAFTITEM_APPLE (CONTENT_CRAFTITEM_MASK | 0x17)
#define CONTENT_CRAFTITEM_APPLE_IRON (CONTENT_CRAFTITEM_MASK | 0x18)
#define CONTENT_CRAFTITEM_DYE_BLUE (CONTENT_CRAFTITEM_MASK | 0x19)
#define CONTENT_CRAFTITEM_DYE_GREEN (CONTENT_CRAFTITEM_MASK | 0x1A)
#define CONTENT_CRAFTITEM_DYE_ORANGE (CONTENT_CRAFTITEM_MASK | 0x1B)
#define CONTENT_CRAFTITEM_DYE_PURPLE (CONTENT_CRAFTITEM_MASK | 0x1C)
#define CONTENT_CRAFTITEM_DYE_RED (CONTENT_CRAFTITEM_MASK | 0x1D)
#define CONTENT_CRAFTITEM_DYE_YELLOW (CONTENT_CRAFTITEM_MASK | 0x1E)
#define CONTENT_CRAFTITEM_DYE_WHITE (CONTENT_CRAFTITEM_MASK | 0x1F)
#define CONTENT_CRAFTITEM_DYE_BLACK (CONTENT_CRAFTITEM_MASK | 0x20)
#define CONTENT_CRAFTITEM_QUARTZ_DUST (CONTENT_CRAFTITEM_MASK | 0x21)
#define CONTENT_CRAFTITEM_SALTPETER (CONTENT_CRAFTITEM_MASK | 0x22)
#define CONTENT_CRAFTITEM_GUNPOWDER (CONTENT_CRAFTITEM_MASK | 0x23)
#define CONTENT_CRAFTITEM_SNOW_BALL (CONTENT_CRAFTITEM_MASK | 0x24)
#define CONTENT_CRAFTITEM_STICK (CONTENT_CRAFTITEM_MASK | 0x25)
#define CONTENT_CRAFTITEM_PINE_PLANK (CONTENT_CRAFTITEM_MASK | 0x26)
#define CONTENT_CRAFTITEM_WOOD_PLANK (CONTENT_CRAFTITEM_MASK | 0x27)
#define CONTENT_CRAFTITEM_JUNGLE_PLANK (CONTENT_CRAFTITEM_MASK | 0x28)
#define CONTENT_CRAFTITEM_TNT (CONTENT_CRAFTITEM_MASK | 0x29)
#define CONTENT_CRAFTITEM_ASH (CONTENT_CRAFTITEM_MASK | 0x2A)

#endif
