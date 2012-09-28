#include "includes.h"

namelist includes = NULL;

int have_include (char *name)
{
    namelist inc = includes;
    while (inc) {
        if (!strcmp (inc->name, name))
            return 1;
        inc = inc->next;
    }
    return 0;
}

void add_include (char *name)
{
    namelist inc = includes;

    if (have_include (name))
        return;
    if (inc == NULL) {
        includes = NEW (namenode);
        inc = includes;
    } else {
        while (inc->next)
            inc = inc->next;
        inc->next = NEW (namenode);
        inc = inc->next;
    }
    inc->name = name;
    inc->next = NULL;
}

void push_include (umlclassnode *node)
{
    if (node->key->package != NULL) {
        umlpackagelist pkglist = make_package_list (node->key->package);
        add_include (pkglist->key->name);
    } else {
        add_include (node->key->name);
    }
}

void determine_includes (declaration *d, batch *b)
{
    if (d->decl_kind == dk_module) {
        declaration *inner = d->u.this_module->contents;
        while (inner != NULL) {
            determine_includes (inner, b);
            inner = inner->next;
        }
    } else {
        umlclasslist cl = list_classes (d->u.this_class, b);
        while (cl != NULL) {
            push_include (cl);
            cl = cl->next;
        }
    }
}

