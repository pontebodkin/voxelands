/************************************************************************
* world.c
* voxelands - 3d voxel world sandbox game
* Copyright (C) Lisa 'darkrose' Milne 2017 <lisa@ltmnet.com>
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

#include "common.h"
#include "path.h"

static int world_exists(char* name)
{
	char buff[2048];
	char nbuff[256];

	if (!name)
		return 0;

	name = trim(name);

	if (str_sanitise(nbuff,256,name) < 1)
		return 0;

	if (!path_get("worlds",nbuff,1,buff,2048))
		return 0;

	return 1;
}

int world_create(char* name)
{
	char buff[2048];
	char nbuff[256];
	char nbuff1[256];
	int i;

	if (!name)
		name = "New World";

	name = trim(name);
	if (!name[0])
		name = "New World";

	if (str_sanitise(nbuff,256,name) < 1)
		return 1;

	if (!path_get("worlds",nbuff,1,buff,2048)) {
		if (!path_get("worlds",nbuff,0,buff,2048))
			return 1;

		config_set("world.path",buff);
		config_set("world.name",name);

		return path_create("world","players");
	}

	for (i=1; i<100; i++) {
		snprintf(nbuff1,256,"%s_%d",nbuff,i);
		if (!path_get("worlds",nbuff1,1,buff,2048)) {
			if (!path_get("worlds",nbuff1,0,buff,2048))
				return 1;

			config_set("world.path",nbuff1);
			snprintf(nbuff,256,"%s %d",name,i);
			config_set("world.name",nbuff);

			return path_create("world","players");
		}
	}

	return 1;
}

int world_load(char* name)
{
	char buff[2048];
	char nbuff[256];

	config_clear("world");

	if (!name) {
#ifdef SERVER
		name = config_get("server.world");
#else
		name = config_get("client.world");
#endif
		if (!name)
			return 1;
	}

	name = trim(name);
	snprintf(nbuff,256,"%s",name);

	if (str_sanitise(buff,2048,nbuff) < 1)
		return 1;

	config_set("world.path",buff);
	config_set("world.name",nbuff);

	if (path_get("world","world.cfg",1,buff,2048)) {
		config_load("world","world.cfg");
	}else{
		vlprintf(CN_WARN,"Unknown world config: using defaults");
	}

	config_set("world.version",VERSION_STRING);

#ifdef SERVER
	config_set("server.world",nbuff);
#else
	config_set("client.world",nbuff);
#endif

	return 0;
}

/* save the world data, then clear all world.* config */
void world_unload()
{
	config_save("world","world","world.cfg");
	config_clear("world");
}

/* initialise and/or create a world */
int world_init(char* name)
{
	if (!world_exists(name)) {
		if (world_create(name))
			return 1;
	}

	if (world_load(name))
		return 1;

	/* TODO: init server/environment/etc */


	return 0;
}

/* shutdown, save, clear, and free the current world */
void world_exit()
{
	/* TODO: shutdown server/environment/etc */

	world_unload();
}
