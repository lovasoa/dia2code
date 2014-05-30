/***************************************************************************
                          scan_tree.c
                             -------------------
    begin                : Wed Jan 17 2001
    copyright            : (C) 2001 by Ruben Lopez
    email                : ryu@gpul.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "dia2code.h"
#define NEW_KLASS(c) ((c*)malloc(sizeof(c)))

/* Adds an item to a name list */
namelist name_list_add(namelist l, const char *data) {
    namenode *n = NEW_KLASS(namenode);
    n->name = strdup(data);
    n->next = l;
    return n;
}

/* Returns a list with all the classes declared in the diagram */
namelist scan_tree_classes(batch *b) {
    umlclasslist tmplist;
    namelist result = NULL;
    tmplist = b->classlist;

    while ( tmplist != NULL ) {
        result = name_list_add(result, tmplist->key->name);
        tmplist = tmplist->next;
    }
    return result;
}

/* Returns a list with all the classes declared in the diagram that current
 * class references */
namelist find_classes(umlclasslist current_class, batch *b) {
    umlclasslist parents, dependencies;
    umlassoclist associations;
    umlattrlist umla, tmpa;
    umloplist umlo;
    namelist result = NULL;
    namelist classes = scan_tree_classes(b);

    umla = current_class->key->attributes;
    while ( umla != NULL) {
        if (is_present(classes, umla->key.type)
                && !is_present(result, umla->key.type)) {
            result = name_list_add(result, umla->key.type);
        }
        umla = umla->next;
    }

    umlo = current_class->key->operations;
    while ( umlo != NULL) {
        if (is_present(classes, umlo->key.attr.type)
                && !is_present(result, umlo->key.attr.type)) {
            result = name_list_add(result, umlo->key.attr.type);
        }
        tmpa = umlo->key.parameters;
        while (tmpa != NULL) {
            if (is_present(classes, tmpa->key.type)
                    && !is_present(result, tmpa->key.type)) {
                result = name_list_add(result, tmpa->key.type);
            }
            tmpa = tmpa->next;
        }

        umlo = umlo->next;
    }

    parents = current_class->parents;
    while ( parents != NULL ) {
        if ( is_present(classes, parents->key->name)
                && ! is_present(result, parents->key->name) ) {
            result = name_list_add(result, parents->key->name);
        }
        parents = parents->next;
    }

    dependencies = current_class->dependencies;
    while (dependencies != NULL) {
        if ( is_present(classes, dependencies->key->name)
                && ! is_present(result, dependencies->key->name) ) {
            result = name_list_add(result, dependencies->key->name);
        }
        dependencies = dependencies->next;
    }

    associations = current_class->associations;
    while (associations != NULL) {
        if ( is_present(classes, associations->key->name)
                && ! is_present(result, associations->key->name ) ) {
            result = name_list_add(result, associations->key->name);
        }
        associations = associations->next;
    }

    return result;
}

umlclasslist find_by_name(umlclasslist list, const char * name ) {
    if ( name != NULL && strlen(name) > 0 ) {
        while ( list != NULL ) {
            if ( ! strcmp(name, list->key->name) ) {
                return list;
            }
            list = list->next;
        }
    }
    return NULL;
}

/* Creates a new umlclassnode with class as the key, then appends it to
  the end of list */
umlclasslist append ( umlclasslist list, umlclassnode * class ) {
    umlclasslist tmplist = list;
    umlclassnode *tmpnode = NULL;

    tmpnode = (umlclassnode*) my_malloc ( sizeof(umlclassnode) );
    tmpnode->key = class->key;
    tmpnode->parents = class->parents;
    tmpnode->associations = class->associations;
    tmpnode->dependencies = class->dependencies;
    tmpnode->next = NULL;

    if ( tmplist != NULL ) {
        while ( tmplist->next != NULL ) {
            tmplist = tmplist->next;
        }
        tmplist->next = tmpnode;
        return list;
    } else {
        return tmpnode;
    }
}

/* Returns a freshly constructed list of the classes that are used
   by the given class AND are themselves in the classlist of the
   given batch */
umlclasslist list_classes(umlclasslist current_class, batch *b) {
    umlclasslist parents, dependencies;
    umlassoclist associations;
    umlattrlist umla, tmpa;
    umloplist umlo;
    umlclasslist result = NULL;
    umlclasslist classes = b->classlist;
    umlclasslist tmpnode = NULL;

    umla = current_class->key->attributes;
    while ( umla != NULL) {
        if ( strlen(umla->key.type) > 0 ) {
            tmpnode = find_by_name(classes, umla->key.type);
            if ( tmpnode && ! find_by_name(result, umla->key.type)) {
                result = append(result, tmpnode);
            }
        }
        umla = umla->next;
    }

    umlo = current_class->key->operations;
    while ( umlo != NULL) {
        tmpnode = find_by_name(classes, umlo->key.attr.type);
        if ( tmpnode && ! find_by_name(result, umlo->key.attr.type)) {
            result = append(result, tmpnode);
        }
        tmpa = umlo->key.parameters;
        while (tmpa != NULL) {
            tmpnode = find_by_name(classes, tmpa->key.type);
            if ( tmpnode && ! find_by_name(result, tmpa->key.type)) {
                result = append(result, tmpnode);
            }
            tmpa = tmpa->next;
        }
        umlo = umlo->next;
    }

    parents = current_class->parents;
    while ( parents != NULL ) {
        tmpnode = find_by_name(classes, parents->key->name);
        if ( tmpnode && ! find_by_name(result, parents->key->name) ) {
            result = append(result, tmpnode);
        }
        parents = parents->next;
    }

    dependencies = current_class->dependencies;
    while (dependencies != NULL) {
        tmpnode = find_by_name(classes, dependencies->key->name);
        if ( tmpnode && ! find_by_name(result, dependencies->key->name) ) {
            result = append(result, tmpnode);
        }
        dependencies = dependencies->next;
    }

    associations = current_class->associations;
    while (associations != NULL) {
        tmpnode = find_by_name(classes, associations->key->name);
        if ( tmpnode && ! find_by_name(result, associations->key->name ) ) {
            result = append(result, tmpnode);
        }
        associations = associations->next;
    }

    return result;

}
