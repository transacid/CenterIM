/*
 * libyahoo2: yahoo_connections.c
 *
 * Copyright (C) 2002, Philip S Tellis <philip.tellis@iname.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * Linked list routines to handle multiple connections
 */

#include "pseudoglib.h"
#include "yahoo_connections.h"

/*
 * Name: add_to_list
 *      Adds a yahoo_data item to a connection list, creates
 *      the connection if it doesn't exist.
 *
 * Parameters:
 *      list - a pointer to the list
 *      yd   - the element to be added
 *
 * Returns:
 *      a pointer to the list   
 */
struct yahoo_connections * 
add_to_list(struct yahoo_connections * list, struct yahoo_data *yd)
{
	struct yahoo_connections *tmp, *new_item;

	new_item = g_new0(struct yahoo_connections, 1);
	new_item->yd = yd;
	new_item->next = NULL;

	if(!list)
		return new_item;
	
	for(tmp = list; tmp->next; tmp = tmp->next)
		;

	tmp->next = new_item;

	return list;
}

/*
 * Name: del_from_list
 *      Removes a yahoo_data item from a connection list.
 *      The yahoo_data item must be freed by the caller
 *      after this function returns
 *
 * Parameters:
 *      list - a pointer to the list
 *      yd   - the element to be removed
 *
 * Returns:
 *      a pointer to the list
 */
struct yahoo_connections * 
del_from_list(struct yahoo_connections * list, struct yahoo_data *yd)
{
	struct yahoo_connections *curr, *prev=list;

	for(curr = list; curr && curr->yd != yd; curr = curr->next)
		prev = curr;

	if(!curr)
		return list;

	if(curr->yd == yd) {
		if(curr == list)
			list = list->next;
		else
			prev->next = curr->next;
	}

	curr->next = NULL;
	curr->yd = NULL;        // should be freed by caller
	g_free(curr);

	return list;
}

/*
 * Name: find_conn_by_id
 *      Returns a yahoo_data item from this list identified
 *      by the specified id
 *
 * Parameters:
 *      list - a pointer to the list
 *      id   - the id that identifies the element to be returned
 *
 * Returns:
 *      a pointer to the data element or NULL if it isn't found
 */
struct yahoo_data * 
find_conn_by_id(struct yahoo_connections * list, guint32 id)
{
	struct yahoo_connections *curr;

	for(curr = list; curr && curr->yd->client_id != id; curr = curr->next)
		;

	if(!curr)
		return NULL;

	if(curr->yd->client_id == id)
		return curr->yd;

	return NULL;
}

