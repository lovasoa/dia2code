#include "decls.h"

declaration *decls = NULL;
static namelist tmp_classes = NULL;

int use_corba = 0;

module *
create_nested_modules_from_pkglist (umlpackagelist pkglist, module *m)
{
    /* Expects pkglist and m to be non-NULL and m->contents to be NULL.
       Returns a reference to the innermost module created.  */
    while (pkglist->next != NULL) {
        declaration *d = NEW (declaration);
        d->decl_kind = dk_module;
        d->prev = d->next = NULL;
        d->u.this_module = NEW (module);
        m->contents = d;
        pkglist = pkglist->next;
        m = d->u.this_module;
        m->pkg = pkglist->key;
        m->contents = NULL;
    }
    return m;
}

module *
find_or_add_module (declaration **dptr, umlpackagelist pkglist)
{
    declaration *d = *dptr;
    module *m;

    if (pkglist == NULL)
        return NULL;
    if (d == NULL) {
        *dptr = NEW (declaration);
        d = *dptr;
    } else {
        declaration *dprev = NULL;
        while (d != NULL) {
            if (d->decl_kind == dk_module &&
                !strcmp (d->u.this_module->pkg->name, pkglist->key->name)) {
                m = d->u.this_module;
                if (pkglist->next == NULL)
                    return m;
                if (m->contents == NULL) {
                    return create_nested_modules_from_pkglist (pkglist, m);
                }
                return find_or_add_module (&m->contents, pkglist->next);
            }
            dprev = d;
            d = d->next;
        }
        if (dprev != NULL) {
            dprev->next = NEW (declaration);
            d = dprev->next;
        }
    }
    d->decl_kind = dk_module;
    d->next = NULL;
    d->u.this_module = NEW (module);
    m = d->u.this_module;
    m->pkg = pkglist->key;
    m->contents = NULL;
    return create_nested_modules_from_pkglist (pkglist, m);
}

module *
find_module (declaration *d, umlpackagelist pkglist)
{
    while (d != NULL) {
        if (d->decl_kind == dk_module) {
            module *m = d->u.this_module;
            if (!strcmp (m->pkg->name, pkglist->key->name)) {
                if (pkglist->next != NULL)
                    return find_module (m->contents, pkglist->next);
                else
                    return m;
            }
        }
        d = d->next;
    }
    return NULL;
}

declaration *
find_class (umlclassnode *node)
{
    declaration *d;

    if (node->key->package != NULL) {
        umlpackagelist pkglist = make_package_list (node->key->package);
        module *m = find_module (decls, pkglist);
        if (m == NULL || m->contents == NULL)
            return 0;
        d = m->contents;
    } else {
        d = decls;
    }

    while (d != NULL) {
        if (d->decl_kind == dk_class) {
            umlclassnode *cl = d->u.this_class;
            if (!strcmp (cl->key->name, node->key->name))
                return d;
        }
        d = d->next;
    }
    return NULL;
}

declaration *
append_decl (declaration *d)
{
    while (d->next != NULL) {
        d = d->next;
    }
    d->next = NEW (declaration);
    d->next->prev = d;
    d = d->next;
    return d;
}

void
push (umlclassnode *node, batch *b)
{
    umlclasslist used_classes, tmpnode;
    module *m;
    declaration *d;
    namelist l_tmp;

    if (node == NULL || find_class (node) != NULL) {
        return;
    }

    l_tmp = NEW (namenode);
    l_tmp->name = strdup (node->key->name);
    l_tmp->next = tmp_classes;
    tmp_classes = l_tmp;
    
    used_classes = list_classes (node, b);
    /* Make sure all classes that this one depends on are already pushed. */
    tmpnode = used_classes;
    while (tmpnode != NULL) {
        /* don't push this class !*/
        if (! eq (node->key->name, tmpnode->key->name) &&
            ! (is_present (tmp_classes, tmpnode->key->name) ^ b->mask)) {
            push (tmpnode, b);
        }
        tmpnode = tmpnode->next;
    }

    if (node->key->package != NULL) {
        umlpackagelist pkglist = make_package_list (node->key->package);
        m = find_or_add_module (&decls, pkglist);
        if (m->contents == NULL) {
            m->contents = NEW (declaration);
            d = m->contents;
            d->prev = NULL;
        } else {
            /* We can simply append because all classes that we depend on
               are already pushed. */
            d = append_decl (m->contents);
        }
    } else {
        if (decls == NULL) {
            decls = NEW (declaration);
            d = decls;
            d->prev = NULL;
        } else {
            d = append_decl (decls);
            /* We can simply append because all classes that we depend on
               are already pushed. */
        }
    }
    d->decl_kind = dk_class;
    d->next = NULL;
    d->u.this_class = NEW (umlclassnode);
    memcpy (d->u.this_class, node, sizeof(umlclassnode));
    if (strncmp (node->key->stereotype, "CORBA", 5) == 0)
        use_corba = 1;
}


