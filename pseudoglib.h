#ifndef __PSEUDOGLIB_H__
#define __PSEUDOGLIB_H__

#include <limits.h>
#include <string.h>

typedef void* gpointer;
typedef const void *gconstpointer;

typedef int gint32;
typedef unsigned int guint32;
typedef unsigned short guint16;

typedef char   gchar;
typedef unsigned char   guchar;
typedef unsigned int    guint;
typedef int    gint;
typedef unsigned long   gulong;
typedef gint   gboolean;
typedef float   gfloat;

typedef struct _GSList          GSList;
struct _GSList {
    gpointer data;
    GSList *next;
};

#define G_MAXINT        INT_MAX

#define g_free(mem)          {free(mem); mem = 0;}
#define g_malloc0(size)      ((gpointer) calloc(size, 1))
#define g_new0(type, count)  ((type *) g_malloc0 ((unsigned) sizeof (type) * (count)))
#define g_strdup(s)          (gchar *) strdup(s)
#define g_malloc(size)       ((gpointer) malloc(size))
#define g_renew(type, mem, count)   ((type *) g_realloc (mem, (unsigned) sizeof (type) * (count)))
#define g_realloc(mem, size)  ((gpointer) realloc(mem, size))
#define g_new(type, count)   ((type *) malloc((unsigned) sizeof (type) * (count)))
#define g_strncasecmp(s1, s2, n) strncasecmp(s1, s2, n)

#define TRUE 1
#define FALSE 0

#undef  MAX
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))

#undef  MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))

gpointer g_memdup(gconstpointer mem, guint byte_size);

void g_strfreev (gchar **str_array);
gchar* g_strdup_printf(const gchar *format, ...);
gchar** g_strsplit(const gchar *string, const gchar *delimiter, gint max_tokens);

GSList* g_slist_alloc();
GSList* g_slist_prepend(GSList *list, gpointer data);
GSList* g_slist_append(GSList *list, gpointer data);
GSList* g_slist_remove(GSList *list, gpointer data);
void g_slist_free(GSList *list);

typedef struct _GHashTable GHashTable;

typedef guint (*GHashFunc)(gconstpointer key);
typedef gint (*GCompareFunc)(gconstpointer a, gconstpointer b);
typedef gboolean (*GHRFunc)(gpointer key, gpointer value, gpointer user_data);

guint g_str_hash(gconstpointer v);
gint g_str_equal(gconstpointer v, gconstpointer v2);

void g_hash_table_insert(GHashTable *hash_table, gpointer key, gpointer value);
gpointer g_hash_table_lookup(GHashTable *hash_table, gconstpointer key);
GHashTable* g_hash_table_new(GHashFunc hash_func, GCompareFunc key_compare_func);
gboolean g_hash_table_lookup_extended(GHashTable *hash_table, gconstpointer lookup_key, gpointer *orig_key, gpointer *value);
void g_hash_table_insert(GHashTable *hash_table, gpointer key, gpointer value);
void g_hash_table_remove(GHashTable *hash_table, gconstpointer key);
guint g_hash_table_foreach_remove(GHashTable *hash_table, GHRFunc func, gpointer user_data);
void g_hash_table_destroy (GHashTable *hash_table);

#endif
