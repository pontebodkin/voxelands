/************************************************************************
* nvp.c
* tausaga - environmental survival game
* Copyright (C) Lisa 'darkrose' Milne 2016 <lisa@ltmnet.com>
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

#include "list.h"
#include "nvp.h"
#include "crypto.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* comparison functions:
 * return 0 to insert
 * return 1 to insert later
 * return -1 to not insert at all
 */
static int nvp_insert_cmp(void *e1, void *e2)
{
	nvp_t *n1 = e1;
	nvp_t *n2 = e2;

	if (n2->h > n1->h)
		return 0;
	return 1;
}

/* free memory of an nvp list, if data is non-zero, data is also freed */
void nvp_free(nvp_t **list, int data)
{
	nvp_t *n;
	while ((n = list_pop(list))) {
		if (n->name)
			free(n->name);
		if (n->value)
			free(n->value);
		if (data && n->data)
			free(n->data);
	}
}

/* get a name/value pair */
nvp_t *nvp_get(nvp_t **list, char* name)
{
	nvp_t *c = *list;
	unsigned int h = hash(name);
	while (c) {
		if (c->h > h)
			return NULL;
		if (c->h == h && !strcmp(c->name,name))
			return c;
		c = c->next;
	}
	return NULL;
}

/* get the value of a name/value pair */
char* nvp_get_str(nvp_t **list, char* name)
{
	nvp_t *c = nvp_get(list,name);
	if (c)
		return c->value;
	return NULL;
}

/* get the value of a name/value pair as an int value */
int nvp_get_int(nvp_t **list, char* name)
{
	nvp_t *c = nvp_get(list,name);
	if (c)
		return strtol(c->value,NULL,10);
	return 0;
}

/* get the value of a name/value pair as a float value */
float nvp_get_float(nvp_t **list, char* name)
{
	nvp_t *c = nvp_get(list,name);
	if (c)
		return strtof(c->value,NULL);
	return 0.0;
}

/* get the value of a name/value pair as a boolean value */
int nvp_get_bool(nvp_t **list, char* name)
{
	nvp_t *c = nvp_get(list,name);
	if (c && (!strcmp(c->value,"1") || !strcmp(c->value,"true")))
		return 1;
	return 0;
}

/* get a name/value pair's data value */
void *nvp_get_data(nvp_t **list, char* name)
{
	nvp_t *c = nvp_get(list,name);
	if (c)
		return c->data;
	return NULL;
}

/* set the value of a name/value pair */
void nvp_set(nvp_t **list, char* name, char* value, void *data)
{
	nvp_t *c = *list;
	unsigned int h = hash(name);

	while (c) {
		if (c->h > h)
			break;
		if (c->h == h && !strcmp(c->name,name)) {
			if (c->value) {
				free(c->value);
				c->value = NULL;
			}
			if (value)
				c->value = strdup(value);
			if (data)
				c->data = data;
			return;
		}
		c = c->next;
	}

	if (!value && !data)
		return;

	c = malloc(sizeof(nvp_t));
	c->name = strdup(name);
	c->h = h;
	if (value) {
		c->value = strdup(value);
	}else{
		c->value = NULL;
	}
	c->data = data;

	*list = list_insert_cmp(list,c,nvp_insert_cmp);
}

/* set a name/value pair to an int value */
void nvp_set_int(nvp_t **list, char* name, int value)
{
	char str[20];
	sprintf(str,"%d",value);
	nvp_set(list,name,str,NULL);
}

/* set a name/value pair to a float value */
void nvp_set_float(nvp_t **list, char* name, float value)
{
	char str[20];
	sprintf(str,"%f",value);
	nvp_set(list,name,str,NULL);
}
