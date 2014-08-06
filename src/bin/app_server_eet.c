#include <limits.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "app_server_eet.h"

struct _Term_Item {
    const char * id;
    const char * dir;
};

struct _Terminology_Item {
    unsigned int version;
    Eina_Hash * term_entries;
    const char *__eet_filename;
};

static const char TERM_ITEM_ENTRY[] = "term_item";
static const char TERMINOLOGY_ITEM_ENTRY[] = "terminology_item";

static Eet_Data_Descriptor *_term_item_descriptor = NULL;
static Eet_Data_Descriptor *_terminology_item_descriptor = NULL;

static void
_term_item_init(void)
{
    Eet_Data_Descriptor_Class eddc;

    if (_term_item_descriptor) return;

    EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Term_Item);
    _term_item_descriptor = eet_data_descriptor_stream_new(&eddc);

    EET_DATA_DESCRIPTOR_ADD_BASIC(_term_item_descriptor, Term_Item, "id", id, EET_T_STRING);
    EET_DATA_DESCRIPTOR_ADD_BASIC(_term_item_descriptor, Term_Item, "dir", dir, EET_T_STRING);
}

static void
_term_item_shutdown(void)
{
    if (!_term_item_descriptor) return;
    eet_data_descriptor_free(_term_item_descriptor);
    _term_item_descriptor = NULL;
}

Term_Item *
term_item_new(const char * id, const char * dir)
{
    Term_Item *term_item = calloc(1, sizeof(Term_Item));

    if (!term_item)
       {
          fprintf(stderr, "ERROR: could not calloc Term_Item\n");
          return NULL;
       }

    term_item->id = eina_stringshare_add(id ? id : "");
    term_item->dir = eina_stringshare_add(dir ? dir : "/");

    return term_item;
}

void
term_item_free(Term_Item *term_item)
{
    eina_stringshare_del(term_item->id);
    eina_stringshare_del(term_item->dir);
    free(term_item);
}

const char *
term_item_id_get(const Term_Item *term_item)
{
    return term_item->id;
}

void
term_item_id_set(Term_Item *term_item, const char *id)
{
    EINA_SAFETY_ON_NULL_RETURN(term_item);
    eina_stringshare_replace(&(term_item->id), id);
}

const char *
term_item_dir_get(const Term_Item *term_item)
{
    return term_item->dir;
}

void
term_item_dir_set(Term_Item *term_item, const char *dir)
{
    EINA_SAFETY_ON_NULL_RETURN(term_item);
    eina_stringshare_replace(&(term_item->dir), dir);
}


static void
_terminology_item_init(void)
{
    Eet_Data_Descriptor_Class eddc;

    if (_terminology_item_descriptor) return;

    EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Terminology_Item);
    _terminology_item_descriptor = eet_data_descriptor_stream_new(&eddc);

    EET_DATA_DESCRIPTOR_ADD_BASIC(_terminology_item_descriptor, Terminology_Item, "version", version, EET_T_UINT);
    EET_DATA_DESCRIPTOR_ADD_HASH(_terminology_item_descriptor, Terminology_Item, "term_entries", term_entries, _term_item_descriptor);
}

static void
_terminology_item_shutdown(void)
{
    if (!_terminology_item_descriptor) return;
    eet_data_descriptor_free(_terminology_item_descriptor);
    _terminology_item_descriptor = NULL;
}

Terminology_Item *
terminology_item_new(unsigned int version)
{
    Terminology_Item *terminology_item = calloc(1, sizeof(Terminology_Item));

    if (!terminology_item)
       {
          fprintf(stderr, "ERROR: could not calloc Terminology_Item\n");
          return NULL;
       }

    terminology_item->version = version;
    terminology_item->term_entries = eina_hash_stringshared_new(EINA_FREE_CB(term_item_free));

    return terminology_item;
}

void
terminology_item_free(Terminology_Item *terminology_item)
{
    if (terminology_item->term_entries) eina_hash_free(terminology_item->term_entries);
    free(terminology_item);
}

unsigned int
terminology_item_version_get(const Terminology_Item *terminology_item)
{
    return terminology_item->version;
}

void
terminology_item_version_set(Terminology_Item *terminology_item, unsigned int version)
{
    EINA_SAFETY_ON_NULL_RETURN(terminology_item);
    terminology_item->version = version;
}

void
terminology_item_term_entries_add(Terminology_Item *terminology_item, const char * id, Term_Item *term_item)
{
    EINA_SAFETY_ON_NULL_RETURN(terminology_item);
    eina_hash_add(terminology_item->term_entries, id, term_item);
}

void
terminology_item_term_entries_del(Terminology_Item *terminology_item, const char * id)
{
    EINA_SAFETY_ON_NULL_RETURN(terminology_item);
    eina_hash_del(terminology_item->term_entries, id, NULL);
}

Term_Item *
terminology_item_term_entries_get(const Terminology_Item *terminology_item, const char * id)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(terminology_item, NULL);
    return eina_hash_find(terminology_item->term_entries, id);
}

Eina_Hash *
terminology_item_term_entries_hash_get(const Terminology_Item *terminology_item)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(terminology_item, NULL);
    return terminology_item->term_entries;
}

void
terminology_item_term_entries_modify(Terminology_Item *terminology_item, const char * key, void *value)
{
    EINA_SAFETY_ON_NULL_RETURN(terminology_item);
    eina_hash_modify(terminology_item->term_entries, key, value);
}

Terminology_Item *
terminology_item_load(const char *filename)
{
    Terminology_Item *terminology_item = NULL;
    Eet_File *ef = eet_open(filename, EET_FILE_MODE_READ);
    if (!ef)
      {
        fprintf(stderr, "ERROR: could not open '%s' for read\n", filename);
        return NULL;
      }

    terminology_item = eet_data_read(ef, _terminology_item_descriptor, TERMINOLOGY_ITEM_ENTRY);
    if (!terminology_item) goto end;
    terminology_item->__eet_filename = eina_stringshare_add(filename);

    if (!terminology_item->term_entries) terminology_item->term_entries = eina_hash_stringshared_new(EINA_FREE_CB(term_item_free));

end:
    eet_close(ef);
    return terminology_item;
}

Eina_Bool
terminology_item_save(Terminology_Item *terminology_item, const char *filename)
{
    Eet_File *ef;
    Eina_Bool ret;

    if (filename) eina_stringshare_replace(&(terminology_item->__eet_filename), filename);
    else if (terminology_item->__eet_filename) filename = terminology_item->__eet_filename;
    else return EINA_FALSE;

    ef = eet_open(filename, EET_FILE_MODE_READ_WRITE);
    if (!ef)
       {
          fprintf(stderr, "ERROR: could not open '%s' for write\n", filename);
          return EINA_FALSE;
       }

    ret = !!eet_data_write(ef, _terminology_item_descriptor, TERMINOLOGY_ITEM_ENTRY, terminology_item, EINA_TRUE);
    eet_close(ef);

    return ret;
}

void
app_server_eet_init(void)
{
    _term_item_init();
    _terminology_item_init();
}

void
app_server_eet_shutdown(void)
{
    _term_item_shutdown();
    _terminology_item_shutdown();
}

