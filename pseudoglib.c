#include "pseudoglib.h"

#include <stdarg.h>

gpointer g_memdup(gconstpointer mem, guint byte_size) {
    gpointer new_mem;

    if(mem) {
	new_mem = g_malloc (byte_size);
	memcpy(new_mem, mem, byte_size);
    } else {
	new_mem = NULL;
    }

    return new_mem;
}

gchar** g_strsplit(const gchar *string, const gchar *delimiter, gint max_tokens) {
    GSList *string_list = NULL, *slist;
    gchar **str_array, *s;
    guint i, n = 1;

    if(max_tokens < 1) max_tokens = G_MAXINT;

    if(s = strstr(string, delimiter)) {
	guint delimiter_len = strlen (delimiter);

	do {
	    guint len;
	    gchar *new_string;

	    len = s - string;
	    new_string = g_new(gchar, len + 1);
	    strncpy (new_string, string, len);
	    new_string[len] = 0;
	    string_list = g_slist_prepend(string_list, new_string);
	    n++;
	    string = s + delimiter_len;
	    s = strstr (string, delimiter);
	} while(--max_tokens && s);
    }

    if(*string) {
	n++;
	string_list = g_slist_prepend (string_list, g_strdup (string));
    }

    str_array = g_new (gchar*, n);
    i = n - 1;

    str_array[i--] = NULL;

    for(slist = string_list; slist; slist = slist->next)
	str_array[i--] = slist->data;

    g_slist_free(string_list);

    return str_array;
}

gchar* g_strdup_vprintf(const gchar *format, va_list args1) {
    gchar *buffer;
    buffer = g_new(gchar, 2048);
    vsprintf(buffer, format, args1);
    return buffer;
}

gint g_snprintf(gchar *str, gulong n, gchar const *fmt, ...) {
    gchar *printed;
    va_list args;

    if(!str || n <= 0 || !fmt)
	return 0;

    va_start(args, fmt);
    printed = g_strdup_vprintf(fmt, args);
    va_end(args);

    strncpy (str, printed, n);
    str[n-1] = '\0';
    g_free(printed);

    return strlen (str);
}

void g_strfreev(gchar **str_array) {
    if(str_array) {
	int i;
	for(i = 0; str_array[i] != NULL; i++)
	  g_free(str_array[i]);

	g_free(str_array);
    }
}

gchar* g_strdup_printf(const gchar *format, ...) {
    gchar *buffer;
    va_list args;

    va_start(args, format);
    buffer = g_strdup_vprintf(format, args);
    va_end(args);

    return buffer;
}

// --[ slist functions ]-------------------------------------------------------

GSList* g_slist_prepend(GSList *list, gpointer data) {
    GSList *new_list;

    new_list = g_slist_alloc();
    new_list->data = data;
    new_list->next = list;

    return new_list;
}

GSList* g_slist_alloc() {
    GSList *list;

    list = (GSList *) malloc(sizeof(GSList));
    list->data = NULL;
    list->next = NULL;

    return list;
}

GSList* g_slist_last(GSList *list) {
    if(list) {
	while(list->next)
	    list = list->next;
    }
    return list;
}

GSList* g_slist_append(GSList *list, gpointer data) {
    GSList *new_list;
    GSList *last;

    new_list = g_slist_alloc ();
    new_list->data = data;

    if(list) {
	last = g_slist_last(list);
	last->next = new_list;
	return list;
    } else {
	return new_list;
    }
}

GSList* g_slist_remove(GSList *list, gpointer data) {
    GSList *tmp;
    GSList *prev;

    prev = NULL;
    tmp = list;

    while(tmp) {
	if(tmp->data == data) {
	    if(prev)
	      prev->next = tmp->next;
	    if(list == tmp)
	      list = list->next;

	    tmp->next = NULL;
	    g_slist_free (tmp);

	    break;
	}

	prev = tmp;
	tmp = tmp->next;
    }

    return list;
}

void g_slist_free(GSList *list) {
    if(list) {
	GSList *cl;

	while(list->next) {
	    cl = list->next;
	    g_free(list);
	    list = cl;
	}
    }
}

// --[ hash functions ]--------------------------------------------------------

#define HASH_TABLE_MIN_SIZE 11
#define HASH_TABLE_MAX_SIZE 13845163

typedef struct _GHashNode GHashNode;

struct _GHashNode {
    gpointer key;
    gpointer value;
    GHashNode *next;
};

struct _GHashTable {
    gint size;
    gint nnodes;
    guint frozen;
    GHashNode **nodes;
    GHashFunc hash_func;
    GCompareFunc key_compare_func;
};

static GHashNode* g_hash_node_new(gpointer key, gpointer value) {
    GHashNode *hash_node;

    hash_node = g_new(GHashNode, sizeof(GHashNode));
    hash_node->key = key;
    hash_node->value = value;
    hash_node->next = NULL;

    return hash_node;
}

static inline GHashNode** g_hash_table_lookup_node(GHashTable *hash_table, gconstpointer key) {
    GHashNode **node;

    node = &hash_table->nodes[(* hash_table->hash_func) (key) % hash_table->size];

    if(hash_table->key_compare_func)
      while(*node && !(*hash_table->key_compare_func) ((*node)->key, key))
	node = &(*node)->next;
    else
      while(*node && (*node)->key != key)
	node = &(*node)->next;

    return node;
}

gboolean g_hash_table_lookup_extended(GHashTable *hash_table, gconstpointer lookup_key, gpointer *orig_key, gpointer *value) {
    GHashNode *node;

    if(!hash_table)
	return 0;

    node = *g_hash_table_lookup_node(hash_table, lookup_key);

    if(node) {
	if(orig_key) *orig_key = node->key;
	if(value) *value = node->value;
	return 1;
    } else {
	return 0;
    }
}

void g_hash_table_insert(GHashTable *hash_table, gpointer key, gpointer value) {
    GHashNode **node;

    if(!hash_table) return;
    node = g_hash_table_lookup_node(hash_table, key);

    if(*node) (*node)->value = value; else {
	*node = g_hash_node_new(key, value);
	hash_table->nnodes++;
    }
}

static void g_hash_node_destroy(GHashNode *hash_node) {
    g_free(hash_node);
}

void g_hash_table_remove(GHashTable *hash_table, gconstpointer key) {
    GHashNode **node, *dest;

    if(!hash_table) return;  
    node = g_hash_table_lookup_node(hash_table, key);

    if(*node) {
	dest = *node;
	(*node) = dest->next;
	g_hash_node_destroy(dest);
	hash_table->nnodes--;
    }
}

gpointer g_hash_table_lookup(GHashTable *hash_table, gconstpointer key) {
    GHashNode *node;

    if(!hash_table)
	return NULL;

    node = *g_hash_table_lookup_node(hash_table, key);
    return node ? node->value : NULL;
}

#define GPOINTER_TO_UINT(p)     ((guint)(p))
guint g_direct_hash(gconstpointer v) {
    return GPOINTER_TO_UINT (v);
}

GHashTable* g_hash_table_new(GHashFunc hash_func, GCompareFunc key_compare_func) {
    GHashTable *hash_table;
    guint i;

    hash_table = g_new(GHashTable, 1);
    hash_table->size = HASH_TABLE_MIN_SIZE;
    hash_table->nnodes = 0;
    hash_table->frozen = 1;
    hash_table->hash_func = hash_func ? hash_func : g_direct_hash;
    hash_table->key_compare_func = key_compare_func;
    hash_table->nodes = g_new(GHashNode*, hash_table->size);

    for (i = 0; i < hash_table->size; i++)
	hash_table->nodes[i] = NULL;

    return hash_table;
}

guint g_hash_table_foreach_remove(GHashTable *hash_table, GHRFunc func, gpointer user_data) {
    GHashNode *node, *prev;
    guint i;
    guint deleted = 0;

    if(!hash_table || !func)
	return 0;

    for(i = 0; i < hash_table->size; i++) {
    restart:
	prev = NULL;
	for(node = hash_table->nodes[i]; node; prev = node, node = node->next) {
	    if((* func)(node->key, node->value, user_data)) {
		deleted += 1;
		hash_table->nnodes -= 1;

		if(prev) {
		    prev->next = node->next;
		    g_hash_node_destroy (node);
		    node = prev;
		} else {
		    hash_table->nodes[i] = node->next;
		    g_hash_node_destroy (node);
		    goto restart;
		}
	    }
	}
    }

    return deleted;
}

static void g_hash_nodes_destroy(GHashNode *hash_node) {
    if(hash_node) {
	GHashNode *node = hash_node, *old_node;

	while(node->next) {
	    g_free(node->key);
	    g_free(node->value);
	    old_node = node;
	    node = node->next;
	    g_free(old_node);
	}
    }
}

void g_hash_table_destroy(GHashTable *hash_table) {
    guint i;

    if(!hash_table)
	return;

    for(i = 0; i < hash_table->size; i++)
      g_hash_nodes_destroy(hash_table->nodes[i]);

    g_free(hash_table->nodes);
    g_free(hash_table);
}

guint g_str_hash(gconstpointer key) {
    const char *p = key;
    guint h = *p;

    if (h)
      for (p += 1; *p != '\0'; p++)
	h = (h << 5) - h + *p;

    return h;
}

gint g_str_equal(gconstpointer v, gconstpointer v2) {
    return strcmp ((const gchar*) v, (const gchar*)v2) == 0;
}
