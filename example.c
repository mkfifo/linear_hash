#include "linear_hash.h"

int main(void){
    /* create a hash
     * the hash will automatically manage
     * it's size
     */
    struct lh_table *t = lh_new();

    /* some data to store */
    int data_1 = 1;
    int data_2 = 2;

    /* return pointer */
    int *data;
    int *old_data;

    /* insert new data */
    lh_insert(t, "hello", &data_1);
    lh_insert(t, "world", &data_2);

    /* fetch */
    data = lh_get(t, "hello");

    /* delete existing data */
    lh_delete(t, "world");

    /* update existing data */
    old_data = lh_update(t, "hello", &data_2);

    /* either insert or update as need be*/
    lh_set(t, "boop", &data_2, &old_data);

    /* check a key exists */
    if( lh_exists(t, "hello") ){
    }

    /* tidy up
     * free table
     * but do not free stored data
     * destroy(table, free_table, free_data) */
    lh_destroy(t,     1,          0);

    return 0;
}

