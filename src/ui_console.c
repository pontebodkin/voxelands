/************************************************************************
* ui_console.c
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
#include "list.h"
#include "thread.h"

#include <stdarg.h>
#include <string.h>

typedef struct chatline_s {
	struct chatline_s *prev;
	struct chatline_s *next;
	uint8_t type;
	float age;
	char buff[1024];
} chatline_t;

static struct {
	chatline_t *lines;
	mutex_t *mutex;
	int count;
	int changed;
	char buff[10240];
} ui_console_data = {
	NULL,
	NULL,
	0,
	1
};

void ui_console_addline(uint8_t type, char* txt)
{
	chatline_t *l;
	chatline_t *p;
	char* b;
	int s = 1024;

	if (!txt || !txt[0])
		return;

	l = malloc(sizeof(chatline_t));
	if (!l)
		return;

	l->type = type;
	b = l->buff;
	l->age = 0.0;

	switch (type) {
	case CN_ERROR:
		strcpy(l->buff,"ERROR: ");
		b += 7;
		s -= 7;
		break;
	case CN_WARN:
		strcpy(l->buff,"WARNING: ");
		b += 9;
		s -= 9;
		break;
	case CN_ACTION:
		strcpy(l->buff,"ACTION: ");
		b += 8;
		s -= 8;
		break;
	case CN_CHAT:
	case CN_INFO:
		break;
	case CN_DEBUG:
		strcpy(l->buff,"DEBUG: ");
		b += 7;
		s -= 7;
		break;
	default:;
	}

	if (snprintf(b,s,"%s",txt) >= s)
		return;

	if (!ui_console_data.mutex)
		ui_console_data.mutex = mutex_create();

	mutex_lock(ui_console_data.mutex);

	printf("CHATLINE: '%s'\n",l->buff);

	p = list_last(&ui_console_data.lines);
	if (p && p->type == l->type && !strcmp(p->buff,l->buff)) {
		p->age = 0.0;
		free(l);
	}else{
		ui_console_data.lines = list_push(&ui_console_data.lines,l);
		ui_console_data.changed = 1;
	}

	mutex_unlock(ui_console_data.mutex);
}

void ui_console_addlinef(uint8_t type, char* fmt, ...)
{
	char buff[1024];
	va_list ap;

	if (!fmt || !fmt[0])
		return;

	va_start(ap, fmt);
	if (vsnprintf(buff, 1024, fmt, ap) >= 1024) {
		va_end(ap);
		return;
	}

	va_end(ap);
}

void ui_console_clear()
{
	chatline_t *l;
	if (!ui_console_data.mutex)
		return;
	mutex_lock(ui_console_data.mutex);
	while ((l = list_pop(&ui_console_data.lines)) != NULL) {
		free(l);
	}
	mutex_unlock(ui_console_data.mutex);
	mutex_free(ui_console_data.mutex);

	ui_console_data.mutex = NULL;
	ui_console_data.count = 0;
	ui_console_data.changed = 1;
	ui_console_data.buff[0] = 0;
}

int ui_console_step(float dtime)
{
	chatline_t *line;
	chatline_t *rem;
	int changed = ui_console_data.changed;
	int count = 0;

	if (!ui_console_data.mutex)
		return 0;

	mutex_lock(ui_console_data.mutex);
	line = ui_console_data.lines;
	while (line) {
		if (line->age > 60.0) {
			changed = 1;
			rem = line;
			line = line->next;
			ui_console_data.lines = list_remove(&ui_console_data.lines,rem);
			free(rem);
			continue;
		}
		count++;
		line = line->next;
	}

	while (count > 5 && ui_console_data.lines) {
		changed = 1;
		rem = list_pull(&ui_console_data.lines);
		free(rem);
	}

	if (changed) {
		int s = 10239;
		int l;
		char* b = ui_console_data.buff;
		line = ui_console_data.lines;
		while (line && s > 0) {
			l = strlen(line->buff);
			if (snprintf(b,s,"%s\n",line->buff) < s) {
				l++;
				b += l;
				s -= l;
			}
			line = line->next;
		}
		*b = 0;
	}

	ui_console_data.changed = 0;
	mutex_unlock(ui_console_data.mutex);

	return changed;
}

/* TODO: this, but it's a C++ bridge, so not really */
char* ui_console_get()
{
	if (!ui_console_data.mutex)
		return "";

	mutex_lock(ui_console_data.mutex);
	return ui_console_data.buff;
	mutex_unlock(ui_console_data.mutex);
}
