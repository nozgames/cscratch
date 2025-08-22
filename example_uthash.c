#include "uthash.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int id;                    /* key */
    char name[20];
    UT_hash_handle hh;         /* makes this structure hashable */
} user_t;

user_t *users = NULL;

void add_user(int user_id, const char *name) {
    user_t *s;

    HASH_FIND_INT(users, &user_id, s);  /* id already in the hash? */
    if (s == NULL) {
        s = (user_t*)malloc(sizeof *s);
        s->id = user_id;
        HASH_ADD_INT(users, id, s);  /* id: name of key field */
    }
    strcpy(s->name, name);
}

user_t *find_user(int user_id) {
    user_t *s;

    HASH_FIND_INT(users, &user_id, s);  /* s: output pointer */
    return s;
}

void delete_user(user_t *user) {
    HASH_DEL(users, user);  /* user: pointer to deletee */
    free(user);
}

void delete_all() {
    user_t *current_user, *tmp;

    HASH_ITER(hh, users, current_user, tmp) {
        HASH_DEL(users, current_user);  /* delete it (users advances to next) */
        free(current_user);            /* free it */
    }
}

void print_users() {
    user_t *s;

    for(s = users; s != NULL; s = s->hh.next) {
        printf("user id %d: name %s\n", s->id, s->name);
    }
}

int by_name(const user_t *a, const user_t *b) {
    return strcmp(a->name, b->name);
}

int by_id(const user_t *a, const user_t *b) {
    return (a->id - b->id);
}

void sort_by_name() {
    HASH_SORT(users, by_name);
}

void sort_by_id() {
    HASH_SORT(users, by_id);
}

void example_uthash() {
    int user_id = 1;
    user_t *user;

    add_user(1, "Alice");
    add_user(2, "Bob");
    add_user(3, "Charlie");

    printf("Users:\n");
    print_users();

    user = find_user(2);
    if (user) {
        printf("Found user: %s\n", user->name);
    }

    printf("\nSorted by name:\n");
    sort_by_name();
    print_users();

    printf("\nSorted by id:\n");
    sort_by_id();
    print_users();

    printf("\nHash has %d users\n", HASH_COUNT(users));

    delete_all();
}