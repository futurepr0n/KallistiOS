/* KallistiOS ##version##

   scandir.c
   Copyright (C) 2004 Megan Potter
   Copyright (C) 2024 Falco Girgis
*/

#include <kos/dbglog.h>
#include <sys/dirent.h>

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

// make sure needed
int alphasort(const struct dirent **a, const struct dirent **b) {
   assert(a && b);
   return strcoll((*a)->d_name, (*b)->d_name);
}

static int push_back(struct dirent ***list, int *size, int *capacity,
                      const struct dirent *dir) {
    struct dirent *new_dir;
    int entry_size;
    struct dirent ***list_tmp = list;

    dbglog(DBG_CRITICAL, "PB - %s\n", dir->d_name);

    if(*size == *capacity) {
        if(*capacity)
            *capacity *= 2;
        else
            *capacity = 1;

        list_tmp = list;
        *list = realloc(list, *capacity * sizeof(struct dirent*));

        if(!list)
            goto out_of_memory;

    }

    entry_size = sizeof(struct dirent) + strlen(dir->d_name) + 1;
    new_dir = malloc(entry_size);

    if(!new_dir)
        goto out_of_memory;

    memcpy(new_dir, dir, entry_size);
    (*list)[++*size] = new_dir;

    return 1;

out_of_memory:
    for(int e = 0; e < *size; ++e)
        free((*list_tmp)[e]);

    free(*list_tmp);
    *list = NULL;
    *size = 0;
    *capacity = 0;

    return 0;
}

int scandir(const char *dirname, struct dirent ***namelist, 
            int(*filter)(const struct dirent *), 
            int(*compar)(const struct dirent ** , const struct dirent **)) {

    int size = 0, capacity = 0;
    DIR* dir;
    struct dirent *dirent;

    *namelist = NULL;

    assert(dirname);

    if(!(dir = opendir(dirname)))
        return ENOENT;

    while((dirent = readdir(dir))) {
        dbglog(DBG_CRITICAL, "%s\n", dirent->d_name);
        if(!filter || filter(dirent))
            if(!push_back(namelist, &size, &capacity, dirent)) {
                size = ENOMEM;
                break;
            }
    }

    if(size > 0 && compar)
        qsort(*namelist, size, sizeof(struct dirent*), (void *)compar);

   closedir(dir);

   return size;
}
