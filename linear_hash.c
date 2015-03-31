#include <stdio.h> /* puts, printf */
#include <limits.h> /* ULONG_MAX */

#include <stdlib.h> /* calloc, free */
#include <string.h> /* strcmp, strlen */
#include <stddef.h> /* size_t */

#include "linear_hash.h"

#ifdef LH_TEST
/* when lh_TEST is defined we want our internal functions to be
 * exposed so that our testing code can access them
 * otherwise (when ifndef lh_TEST) we want them to be static
 */

/* ignore missing prototype  warnings
 * but only during testing mode
 */
#pragma GCC diagnostic ignored "-Wmissing-prototypes"
#define LH_INTERNAL
#else
#define LH_INTERNAL static
#endif


/**********************************************
 **********************************************
 **********************************************
 ******** simple helper functions *************
 **********************************************
 **********************************************
 ***********************************************/

/* internal strdup equivalent
 *
 * returns char* to new memory containing a strcpy on success
 * returns 0 on error
 */
LH_INTERNAL char * lh_strdupn(char *str, size_t len){
    /* our new string */
    char *new_str = 0;

    if( ! str ){
        puts("lh_strdupn: str undef");
        return 0;
    }

    /* if len is 0 issue a warning and recalculate
     * note that if strlen is still 0 then all is well
     */
    if( len == 0 ){
        puts("lh_strdupn: provided len was 0, recalculating");
        len = strlen(str);
    }

    /* allocate our new string
     * len + 1 to fit null terminator
     */
    new_str = calloc(len + 1, sizeof(char));
    if( ! new_str ){
        puts("lh_strdupn: call to calloc failed");
        return 0;
    }

    /* perform copy */
    if( ! strncpy(new_str, str, len) ){
        puts("lh_strdupn: call to strncpy failed");
        return 0;
    }

    /* ensure null terminator
     * do not rely on calloc as we may switch
     * to alt. alloc. later
     */
    new_str[len] = '\0';

    return new_str;
}

/* initialise an existing lh_entry
 *
 * returns 1 on success
 * returns 0 on error
 */
LH_INTERNAL unsigned int lh_entry_init(struct lh_entry *entry,
                                       unsigned long int hash,
                                       char *key,
                                       size_t key_len,
                                       void *data ){

    if( ! entry ){
        puts("lh_entry_init: entry was null");
        return 0;
    }

    if( ! key ){
        puts("lh_entry_init: key was null");
        return 0;
    }

    /* we allow data to be null */

    /* we allow next to be null */

    /* if key_len is 0 we issue a warning and recalcualte */
    if( key_len == 0 ){
        puts("warning lh_entry_init: provided key_len was 0, recalcuating");
        key_len = strlen(key);
    }

    /* if hash is 0 we issue a warning and recalculate */
    if( hash == 0 ){
        puts("warning lh_entry_init: provided hash was 0, recalculating");
        hash = lh_hash(key, key_len);
    }

    /* setup our simple fields */
    entry->hash    = hash;
    entry->key_len = key_len;
    entry->data    = data;
    entry->state   = LH_ENTRY_OCCUPIED;

    /* we duplicate the string */
    entry->key = lh_strdupn(key, key_len);
    if( ! entry->key ){
        puts("lh_entry_init: call to lh_strdupn failed");
        return 0;
    }

    /* return success */
    return 1;
}

/* allocate and initialise a new lh_entry
 *
 * returns pointer on success
 * returns 0 on failure
 */
#if 0
LH_INTERNAL struct lh_entry * lh_entry_new(unsigned long int hash,
                                           char *key,
                                           size_t key_len,
                                           void *data ){
    struct lh_entry *she = 0;

    /* alloc */
    she = calloc(1, sizeof(struct lh_entry));
    if( ! she ){
        puts("lh_entry_new: call to calloc failed");
        return 0;
    }

    /* init */
    if( ! lh_entry_init(she, hash, key, key_len, data) ){
        puts("lh_entry_new: call to lh_entry_init failed");
        return 0;
    }

    return she;
}
#endif

/* destroy entry
 *
 * will only free *data if `free_data` is 1
 * will NOT free *next
 * will free all other values
 *
 * returns 1 on success
 * returns 0 on error
 */
LH_INTERNAL unsigned int lh_entry_destroy(struct lh_entry *entry, unsigned int free_data){
    if( ! entry ){
        puts("lh_entry_destroy: entry undef");
        return 0;
    }

    if( free_data && entry->data ){
        free(entry->data);
    }

    /* free key as strdup */
    free(entry->key);

    return 1;
}


/* find the lh_entry that should be holding this key
 *
 * returns a pointer to it on success
 * return 0 on failure
 */
LH_INTERNAL struct lh_entry * lh_find_entry(struct lh_table *table, char *key){
    /* our cur entry */
    struct lh_entry *cur = 0;

    /* hash */
    unsigned long int hash = 0;
    /* position in hash table */
    size_t pos = 0;
    /* iterator through entries */
    size_t i = 0;
    /* cached strlen */
    size_t key_len = 0;


    if( ! table ){
        puts("lh_find_entry: table undef");
        return 0;
    }

    if( ! key ){
        puts("lh_find_entry: key undef");
        return 0;
    }

    /* cache strlen */
    key_len = strlen(key);

    /* calculate hash */
    hash = lh_hash(key, key_len);

    /* calculate pos
     * we know table is defined here
     * so lh_pos cannot fail
     */
    pos = lh_pos(hash, table->size);

    /* search pos..size */
    for( i=pos; i < table->size; ++i ){
        cur = &(table->entries[i]);

        /* if this is an empty then we stop */
        if( cur->state == LH_ENTRY_EMPTY ){
            /* failed to find element */
            puts("lh_find_entry: failed to find key, encountered empty");
            return 0;
        }

        /* if this is a dummy then we skip but continue */
        if( cur->state == LH_ENTRY_DUMMY ){
            continue;
        }

        if( cur->hash != hash ){
            continue;
        }

        if( cur->key_len != key_len ){
            continue;
        }

        if( strncmp(key, cur->key, key_len) ){
            continue;
        }

        return cur;
    }

    /* search 0..pos */
    for( i=0; i < pos; ++i ){
        cur = &(table->entries[i]);

        /* if this is an empty then we stop */
        if( cur->state == LH_ENTRY_EMPTY ){
            /* failed to find element */
            puts("lh_find_entry: failed to find key, encountered empty");
            return 0;
        }

        /* if this is a dummy then we skip but continue */
        if( cur->state == LH_ENTRY_DUMMY ){
            continue;
        }

        if( cur->hash != hash ){
            continue;
        }

        if( cur->key_len != key_len ){
            continue;
        }

        if( strncmp(key, cur->key, key_len) ){
            continue;
        }

        return cur;
    }

    /* failed to find element */
#ifdef DEBUG
    puts("lh_find_entry: failed to find key");
#endif
    return 0;
}



/**********************************************
 **********************************************
 **********************************************
 ******** linear_hash.h implementation ********
 **********************************************
 **********************************************
 ***********************************************/


/* takes a char* representing a string
 *
 * will recalculate key_len if 0
 *
 * returns an unsigned long integer hash value on success
 * returns 0 on error
 */
unsigned long int lh_hash(char *key, size_t key_len){
    /* our hash value */
    unsigned long int hash = 0;
    /* our iterator through the key */
    size_t i = 0;

    if( ! key ){
        puts("lh_hash: key undef");
        return 0;
    }

    /* we allow key_len to be 0
     * we issue a warning and then recalculate
     */
    if( ! key_len ){
        puts("lh_hash: key_len was 0, recalculating");
        key_len = strlen(key);
    }

#ifdef DEBUG
    printf("lh_hash: hashing string '%s'\n", key);
#endif

    /* hashing time */
    for( i=0; i < key_len; ++i ){

#ifdef DEBUG
    printf("lh_hash: looking at i '%zd', char '%c'\n", i, key[i]);
#endif

        /* we do not have to worry about overflow doing silly things:
         *
         * C99 section 6.2.5.9 page 34:
         * A computation involving unsigned operands can never overﬂow,
         * because a result that cannot be represented by the resulting
         * unsigned integer type is reduced modulo the number that is one
         * greater than the largest value that can be represented by the
         * resulting type.
         */

        /* hash this character
         * http://www.cse.yorku.ca/~oz/hash.html
         * djb2
         */
        hash = ((hash << 5) + hash) + key[i];
    }

#ifdef DEBUG
    puts("lh_hash: success");
#endif
    return hash;
}

/* takes a table and a hash value
 *
 * returns the index into the table for this hash
 * returns 0 on error (if table is null)
 *
 * note the error value is indistinguishable from the 0th bucket
 * this function can only error if table is null
 * so the caller can distinguish these 2 cases
 */
size_t lh_pos(unsigned long int hash, size_t table_size){

    /* force hash value into a bucket */
    return hash % table_size;
}

/* allocate and initialise a new lh_table of size size
 *
 * returns pointer on success
 * returns 0 on error
 */
struct lh_table * lh_new(size_t size){
    struct lh_table *sht = 0;

    /* alloc */
    sht = calloc(1, sizeof(struct lh_table));
    if( ! sht ){
        puts("lh_new: calloc failed");
        return 0;
    }

    /* init */
    if( ! lh_init(sht, size) ){
        puts("lh_new: call to lh_init failed");
        return 0;
    }

    return sht;
}

/* free an existing lh_table
 * this will free all the sh entries stored
 * this will free all the keys (as they are strdup-ed)
 *
 * this will only free the *table pointer if `free_table` is set to 1
 * this will only free the *data pointers if `free_data` is set to 1
 *
 * returns 1 on success
 * returns 0 on error
 */
unsigned int lh_destroy(struct lh_table *table, unsigned int free_table, unsigned int free_data){
    /* iterator through table */
    size_t i = 0;

    if( ! table ){
        puts("lh_destroy: table undef");
        return 0;
    }

    /* iterate through `entries` list
     * calling lh_entry_destroy on each
     */
    for( i=0; i < table->size; ++i ){
        if( ! lh_entry_destroy( &(table->entries[i]), free_data ) ){
            puts("lh_destroy: call to lh_entry_destroy failed, continuing...");
        }
    }

    /* free entires table */
    free(table->entries);

    /* finally free table if asked to */
    if( free_table ){
        free(table);
    }

    return 1;
}

/* initialise an already allocated lh_table to size size
 *
 * returns 1 on success
 * returns 0 on error
 */
unsigned int lh_init(struct lh_table *table, size_t size){
    if( ! table ){
        puts("lh_init: table undef");
        return 0;
    }

    if( size == 0 ){
        puts("lh_init: specified size of 0, impossible");
        return 0;
    }

    table->size    = size;
    table->n_elems = 0;

    /* calloc our buckets (pointer to lh_entry) */
    table->entries = calloc(size, sizeof(struct lh_entry));
    if( ! table->entries ){
        puts("lh_init: calloc failed");
        return 0;
    }

    return 1;
}

/* resize an existing table to new_size
 * this will reshuffle all the buckets around
 *
 * you can use this to make a hash larger or smaller
 *
 * returns 1 on success
 * returns 0 on error
 */
unsigned int lh_resize(struct lh_table *table, size_t new_size){
    /* our new data area */
    struct lh_entry *new_entries = 0;
    /* the current entry we are copying across */
    struct lh_entry *cur = 0;
    /* our iterator through the old hash */
    size_t i = 0;
    /* our iterator through the new data */
    size_t j = 0;
    /* our new position for each element */
    size_t new_pos = 0;

    if( ! table ){
        puts("lh_resize: table was null");
        return 0;
    }

    if( new_size == 0 ){
        puts("lh_resize: asked for new_size of 0, impossible");
        return 0;
    }

    /* allocate an array of lh_entry */
    new_entries = calloc(new_size, sizeof(struct lh_entry));
    if( ! new_entries ){
        puts("lh_resize: call to calloc failed");
        return 0;
    }

    /* iterate through old data */
    for( i=0; i < table->size; ++i ){
        cur = &(table->entries[i]);

        /* if we are not occupied then skip */
        if( cur->state != LH_ENTRY_OCCUPIED ){
            continue;
        }

        /* our position within new entries */
        new_pos = lh_pos(cur->hash, new_size);

        for( j = new_pos; j < new_size; ++ j){
            /* skip if not empty */
            if( new_entries[j].state != LH_ENTRY_EMPTY ){
                continue;
            }
            goto LH_RESIZE_FOUND;
        }

        for( j = 0; j < new_pos; ++ j){
            /* skip if not empty */
            if( new_entries[j].state != LH_ENTRY_EMPTY ){
                continue;
            }
            goto LH_RESIZE_FOUND;
        }

        return 0;

LH_RESIZE_FOUND:
        new_entries[j].hash    = cur->hash;
        new_entries[j].key     = cur->key;
        new_entries[j].key_len = cur->key_len;
        new_entries[j].data    = cur->data;
        new_entries[j].state   = cur->state;
    }

    /* free old data */
    free(table->entries);

    /* swap */
    table->size = new_size;
    table->entries = new_entries;

    return 1;
}

/* check if the supplied key already exists in this hash
 *
 * returns 1 on success (key exists)
 * returns 0 if key doesn't exist or on error
 */
unsigned int lh_exists(struct lh_table *table, char *key){
    struct lh_entry *she = 0;

    if( ! table ){
        puts("lh_exists: table undef");
        return 0;
    }

    if( ! key ){
        puts("lh_exists: key undef");
        return 0;
    }

#ifdef DEBUG
    printf("lh_exist: called with key '%s', dispatching to lh_find_entry\n", key);
#endif

    /* find entry */
    she = lh_find_entry(table, key);
    if( ! she ){
        /* not found */
        return 0;
    }

    /* found */
    return 1;
}

/* insert `data` under `key`
 * this will only success if !lh_exists(table, key)
 *
 * returns 1 on success
 * returns 0 on error
 */
unsigned int lh_insert(struct lh_table *table, char *key, void *data){
    /* our new entry */
    struct lh_entry *she = 0;
    /* hash */
    unsigned long int hash = 0;
    /* position in hash table */
    size_t pos = 0;
    /* iterator through table */
    size_t i = 0;
    /* cached strlen */
    size_t key_len = 0;

    if( ! table ){
        puts("lh_insert: table undef");
        return 0;
    }

    if( ! key ){
        puts("lh_insert: key undef");
        return 0;
    }

#ifdef DEBUG
    printf("lh_insert: asked to insert for key '%s'\n", key);
#endif

    /* we allow data to be 0 */

#ifdef DEBUG
    puts("lh_insert: calling lh_exists");
#endif

    /* check for already existing key
     * insert only works if the key is not already present
     */
    if( lh_exists(table, key) ){
        puts("lh_insert: key already exists in table");
        return 0;
    }

    /* cache strlen */
    key_len = strlen(key);

    /* calculate hash */
    hash = lh_hash(key, key_len);

    /* calculate pos
     * we know table is defined here
     * so lh_pos cannot fail
     */
    pos = lh_pos(hash, table->size);

#ifdef DEBUG
    puts("lh_insert: calling lh_entry_new");
#endif

    /* iterate from pos to size */
    for( i=pos; i < table->size; ++i ){
        she = &(table->entries[i]);
        /* if taken keep searching */
        if( she->state == LH_ENTRY_OCCUPIED ){
            continue;
        }

        /* otherwise (empty or dummy) jump to found */
        goto LH_INSERT_FOUND;
    }

    /* iterate from 0 to pos */
    for( i=0; i < pos; ++i ){
        she = &(table->entries[i]);
        /* if taken keep searching */
        if( she->state == LH_ENTRY_OCCUPIED ){
            continue;
        }

        /* otherwise (empty or dummy) jump to found */
        goto LH_INSERT_FOUND;
    }

    /* no slot found */
    puts("lh_insert: unable to find insertion slot");
    return 0;

LH_INSERT_FOUND:
    /* she is already set! */

    /* construct our new lh_entry
     * lh_entry_new(unsigned long int hash,
     *              char *key,
     *              size_t key_len,
     *              void *data,
     *              struct lh_entry *next){
     *
     * only key needs to be defined
     *
     */
    /*                 (entry, hash, key, key_len, data) */
    if( ! lh_entry_init(she,   hash, key, key_len, data) ){
        puts("lh_insert: call to lh_entry_init failed");
        return 0;
    }

    /* increment number of elements */
    ++table->n_elems;

    /* return success */
    return 1;
}

/* set `data` under `key`
 * this will only succeed if lh_exists(table, key)
 *
 * returns old data on success
 * returns 0 on error
 */
void * lh_set(struct lh_table *table, char *key, void *data){
    struct lh_entry *she = 0;
    void * old_data = 0;

    if( ! table ){
        puts("lh_set: table undef");
        return 0;
    }

    if( ! key ){
        puts("lh_set: key undef");
        return 0;
    }

    /* allow data to be null */

    /* find entry */
    she = lh_find_entry(table, key);
    if( ! she ){
        /* not found */
        return 0;
    }

    /* save old data */
    old_data = she->data;

    /* overwrite */
    she->data = data;

    /* return old data */
    return old_data;
}

/* get `data` stored under `key`
 *
 * returns data on success
 * returns 0 on error
 */
void * lh_get(struct lh_table *table, char *key){
    struct lh_entry *she = 0;

    if( ! table ){
        puts("lh_get: table undef");
        return 0;
    }

    if( ! key ){
        puts("lh_get: key undef");
        return 0;
    }

    /* find entry */
    she = lh_find_entry(table, key);
    if( ! she ){
        /* not found */
        return 0;
    }

    /* found */
    return she->data;
}

/* delete entry stored under `key`
 *
 * returns data on success
 * returns 0 on error
 */
void * lh_delete(struct lh_table *table, char *key){
    /* our cur entry */
    struct lh_entry *cur = 0;
    /* hash */
    unsigned long int hash = 0;
    /* position in hash table */
    size_t pos = 0;
    /* iterator through has table */
    size_t i =0;
    /* cached strlen */
    size_t key_len = 0;

    /* our old data */
    void *old_data = 0;

    if( ! table ){
        puts("lh_delete: table undef");
        return 0;
    }

    if( ! key ){
        puts("lh_delete: key undef");
        return 0;
    }

    /* if we do not have this key then we consider this a failure */
    if( ! lh_exists(table, key) ){
        puts("lh_delete: key did not exist");
        return 0;
    }

    /* cache strlen */
    key_len = strlen(key);

    /* calculate hash */
    hash = lh_hash(key, key_len);

    /* calculate pos
     * we know table is defined here
     * so lh_pos cannot fail
     */
    pos = lh_pos(hash, table->size);


    /* starting at pos search for element
     * searches pos .. size
     */
    for( i = pos; i < table->size ; ++i ){
        cur = &(table->entries[i]);

        /* if this is an empty then we stop */
        if( cur->state == LH_ENTRY_EMPTY ){
            /* failed to find element */
            puts("lh_delete: failed to find key, encountered empty");
            return 0;
        }

        /* if this is a dummy then we skip but continue */
        if( cur->state == LH_ENTRY_DUMMY ){
            continue;
        }

        if( cur->hash != hash ){
            continue;
        }

        if( cur->key_len != key_len ){
            continue;
        }

        if( strncmp(key, cur->key, key_len) ){
            continue;
        }

        goto LH_DELETE_FOUND;
    }

    /* if we are here then we hit the end,
     * searches 0 .. pos
     */
    for( i = 0; i < pos; ++i ){
        cur = &(table->entries[i]);

        /* if this is an empty then we stop */
        if( cur->state == LH_ENTRY_EMPTY ){
            /* failed to find element */
            puts("lh_delete: failed to find key, encountered empty");
            return 0;
        }

        /* if this is a dummy then we skip but continue */
        if( cur->state == LH_ENTRY_DUMMY ){
            continue;
        }

        if( cur->hash != hash ){
            continue;
        }

        if( cur->key_len != key_len ){
            continue;
        }

        if( strncmp(key, cur->key, key_len) ){
            continue;
        }

        goto LH_DELETE_FOUND;
    }

    /* failed to find element */
    puts("lh_delete: failed to find key, both loops terminated");
    return 0;

LH_DELETE_FOUND:
        /* cur is already set! */

        /* save old data pointer */
        old_data = cur->data;

        /* clear out */
        cur->data = 0;
        cur->key = 0;
        cur->key_len = 0;
        cur->hash = 0;
        cur->state = LH_ENTRY_DUMMY;

        /* decrement number of elements */
        --table->n_elems;

        /* return old data */
        return old_data;
}

