
/*
 * mdb.h
 */

#ifndef _MDB_H_
#define _MDB_H_

#include <stdio.h>
#include "mylist.h"

struct MdbRec {
    char name[16];
    char  msg[24];
};

int loadmdb(FILE *fp, struct List *dest) 
{
    /*
     * read all records into memory
     */

    struct MdbRec r;
    struct Node *node = NULL;
    int count = 0;

    while (fread(&r, sizeof(r), 1, fp) == 1) {

        // allocate memory for a new record and copy into it the one
        // that was just read from the database.
        struct MdbRec *rec = (struct MdbRec *)malloc(sizeof(r));
        if (!rec)
            return -1;
        memcpy(rec, &r, sizeof(r));

        // add the record to the linked list.
        node = addAfter(dest, node, rec);
        if (node == NULL) 
            return -1;

        count++;
    }

    // see if fread() produced error
    if (ferror(fp)) 
        return -1;

    return count;
}

void freemdb(struct List *list) 
{
    // free all the records
    traverseList(list, &free);
    removeAllNodes(list);
}
#endif /* _MDB_H_ */

