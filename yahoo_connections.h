#ifndef YAHOO_CONNECTIONS_H
#define YAHOO_CONNECTIONS_H

#include "libyahoo2.h"

struct yahoo_connections {
	struct yahoo_data *yd;
	struct yahoo_connections *next;
}; 

struct yahoo_connections * 
add_to_list(struct yahoo_connections * list, struct yahoo_data *yd);

struct yahoo_connections *
del_from_list(struct yahoo_connections * list, struct yahoo_data *yd);

struct yahoo_data * 
find_conn_by_id(struct yahoo_connections * list, guint32 fd);

#endif
