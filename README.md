# Linear hash [![Build Status](https://travis-ci.org/mkfifo/linear_hash.svg)](https://travis-ci.org/mkfifo/linear_hash) [![Coverage Status](https://coveralls.io/repos/mkfifo/linear_hash/badge.svg?branch=master)](https://coveralls.io/r/mkfifo/linear_hash?branch=master) <a href="https://scan.coverity.com/projects/4850"> <img alt="Coverity Scan Build Status" src="https://scan.coverity.com/projects/4850/badge.svg"/> </a>

An implementation of a linear probing unordered hash table written in pure C99 with no external dependencies

Linear hash is licensed under the MIT license, see LICENSE for more details

Hashing function
----------------

linear_hash uses an implementation of djb2,
below is the reference implementation which can also be
found on [http://www.cse.yorku.ca/~oz/hash.html](http://www.cse.yorku.ca/~oz/hash.html)

    unsigned long
    hash(unsigned char *str)
    {
        unsigned long hash = 5381;
        int c;

        while (c = *str++)
            hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

        return hash;
    }

Example usage
--------------

    #include "linear_hash.h"

    #pragma GCC diagnostic ignored "-Wunused-but-set-variable"

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

        /* insert new data */
        lh_insert(t, "hello", &data_1);
        lh_insert(t, "world", &data_2);

        /* fetch */
        data = lh_get(t, "hello");

        /* delete existing data */
        lh_delete(t, "world");

        /* update existing data */
        lh_update(t, "hello", &data_2);

        /* either insert or update as need be*/
        lh_set(t, "boop", &data_2);

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

Internal implementation
-----------------------

Linear hash is an unhardened linear probing unordered hash backed by a dense array.

Every incoming key is first hashed, this hash is then taken and modulo-d to find
an appropriate slot for it, if this slot is already occupied then linear probing
is used to find the next available bucket.

The linear probe will search along the hash looking for an empty slot,
currently the probing step width is 1.

Linear hash will automatically resize when the loading factor hits a threshold,
currently this threshold can be set by the user (via `lh_tune_threshold`) and
it defaults to 60% full.

This threshold value was pulled out of thin-air and I plan to investigate 
different threshold in the future.

Upon hitting this threshold linear hash will double in size and rehash all
elements.

Linear hash is not hardened and so is not recommended for use cases which would
expose it to attackers.

Each of the slots within the backing array is an instance of lh_entry which is
marked by an enum to show if it is in use or not,
it may be worth breaking this struct out into 2 or so arrays to improve cache
lines.

A rough diagram of the internals of how a linear hash of size 8 would look:

      lh_table
    size    = 8
    n_elems = 4
    entries = *
              |
              v
              [ empty | occupied | occupied | occupied | empty | occupied | empty | empty ]
                        hash = X   hash = X   hash = Y           hash = Z
                        key  = *   key  = *   key  = *           key  = *
                        data = *   data = *   data = *           data = *

Here we can see an lh_table of size 8 containing 4 entries.

We can see that 2 of the entries collided on a hash of `X`,
the first to be inserted landed in the ideal bucket of [1]
the second to be inserted could not go here so linear probing begin
the linear probe starts stepping along the array and will insert the item into
the first empty slot is finds at [2].

If we assume that the hash `Y` also tried to use the same slot of [1]
then it too would have triggered a linear probe which would have stepped along
twice until it found the empty bucket at [3] which it was then inserted into.

From this table layout, if we assuming the X and Y collided, then we know
`X % 8 == Y % 8 == 1`
and
`Z % 8 == 5`.


