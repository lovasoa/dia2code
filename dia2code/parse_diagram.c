/***************************************************************************
                          parse_diagram.c  -  The parser of the Dia file
                             -------------------
    begin                : Sat Dec 16 2000
    copyright            : (C) 2000-2001 by Javier O'Hara
    email                : joh314@users.sourceforge.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "parse_diagram.h"

umlclasslist find(umlclasslist list, char *id ) {
    if ( id != NULL ) {
        while ( list != NULL ) {
            if ( ! strcmp(id, list->key->id) ) {
                return list;
            }
            list = list->next;
        }
    }
    return NULL;
}

/* the buffer must have room for 79 characters and a \0 */
void parse_dia_string(xmlNodePtr stringnode, char *buffer) {
    xmlChar *content;

    content = xmlNodeGetContent(stringnode);
    if ( sscanf(content, "#%79[^#]#", buffer) == 0) {
        buffer[0] = 0;
    }
    free(content);
}

int parse_boolean(xmlNodePtr booleannode) {
    xmlChar *val;
    int result;

    val = xmlGetProp(booleannode, "val");
    if ( val != NULL && ! strcmp(val, "true")) {
        result = 1;
    } else {
        result = 0;
    }
    free(val);
    return result;
}


void addparent(umlclasslist base, umlclasslist derived) {
    umlclasslist tmp;
    tmp = (umlclasslist) my_malloc ( sizeof(umlclassnode) );
    tmp->key = base->key;
    tmp->next = derived->parents;
    derived->parents = tmp;
}

void adddependency(umlclasslist dependent, umlclasslist dependee) {
    umlclasslist tmp;
    tmp = (umlclasslist) my_malloc ( sizeof(umlclassnode) );
    tmp->key = dependent->key;
    tmp->next = dependee->dependencies;
    dependee->dependencies = tmp;
}

void addaggregate(char * name, char composite, umlclasslist base, umlclasslist associate) {
    umlassoclist tmp;
    tmp = (umlassoclist) my_malloc ( sizeof(umlassocnode) );
    if (name != NULL) {
        sscanf(name, "#%79[^#]#", tmp->name);
    } else {
        printf("warning: unnamed association between %s and %s\n", base->key->name, associate->key->name);
        strcpy(tmp->name, "unnamed");
    }
    tmp->key = base->key;
    tmp->composite = composite;
    tmp->next = associate->associations;
    associate->associations = tmp;
}

void inherit_realize ( umlclasslist classlist, char * base, char * derived ) {
    umlclasslist umlbase, umlderived;
    umlbase = find(classlist, base);
    umlderived = find(classlist, derived);
    if ( umlbase != NULL && umlderived != NULL ) {
        addparent(umlbase, umlderived);
    }
}

void associate ( umlclasslist classlist, char * name, char composite, char * base, char * aggregate) {
    umlclasslist umlbase, umlaggregate;
    umlbase = find(classlist, base);
    umlaggregate = find(classlist, aggregate);
    if ( umlbase != NULL && umlaggregate != NULL) {
        addaggregate(name, composite, umlbase, umlaggregate);
    }
}

void make_depend ( umlclasslist classlist, char * dependent, char * dependee) {
    umlclasslist umldependent, umldependee;
    umldependent = find(classlist, dependent);
    umldependee = find(classlist, dependee);
    if ( umldependent != NULL && umldependee != NULL) {
        adddependency(umldependent, umldependee);
    }
}

/**
  * Inserts "n" into the list "l", in orderly fashion
*/
umlattrlist insert_attribute(umlattrlist n, umlattrlist l) {
    if ( l != NULL ) {
        if ( l->key.visibility <= n->key.visibility ) {
            l->next = insert_attribute(n, l->next);
            return l;
        } else {
            n->next = l;
            return n;
        }
    } else {
        return n;
    }
}

/**
  * Inserts "n" into the list "l", in orderly fashion
*/
umloplist insert_operation(umloplist n, umloplist l) {
    if ( l != NULL ) {
        if ( l->key.attr.visibility <= n->key.attr.visibility ) {
            l->next = insert_operation(n, l->next);
            return l;
        } else {
            n->next = l;
            return n;
        }
    } else {
        return n;
    }
}

umltemplatelist insert_template(umltemplatelist n, umltemplatelist l) {
    if ( l != NULL) {
        n->next = l;
        return n;
    } else {
        return n;
    }
}

void parse_attribute(xmlNodePtr node, umlattribute *tmp) {
    xmlChar *nodename;
    xmlChar *attrval;

    tmp->value[0] = 0;
    tmp->value[1] = 0;
    tmp->visibility = '0';
    tmp->kind     = '0';
    while ( node != NULL ) {
        nodename = xmlGetProp(node, "name");
        if ( ! strcmp("name", nodename) ) {
            parse_dia_string(node->xmlChildrenNode, tmp->name);
        } else if ( ! strcmp("value", nodename)) {
            if (node->xmlChildrenNode->xmlChildrenNode != NULL) {
                parse_dia_string(node->xmlChildrenNode, tmp->value);
            }
        } else if ( ! strcmp("type", nodename)) {
            if (node->xmlChildrenNode->xmlChildrenNode != NULL) {
                parse_dia_string(node->xmlChildrenNode, tmp->type);
            } else {
                tmp->type[0] = 0;
            }
        } else if ( ! strcmp("comment", nodename)) {
            if (node->xmlChildrenNode->xmlChildrenNode != NULL) {
               parse_dia_string(node->xmlChildrenNode, tmp->comment);
            } else {
               tmp->comment[0] = 0;
          }
        } else if ( ! strcmp("kind", nodename)) {
            attrval = xmlGetProp(node->xmlChildrenNode, "val");
            sscanf(attrval, "%c", &(tmp->kind));
            free(attrval);
        } else if ( ! strcmp("visibility", nodename)) {
            attrval = xmlGetProp(node->xmlChildrenNode, "val");
            sscanf(attrval, "%c", &(tmp->visibility));
            free(attrval);
        } else if ( ! strcmp("abstract", nodename)) {
            tmp->isabstract = parse_boolean(node->xmlChildrenNode);
        } else if ( ! strcmp("class_scope", nodename)) {
            tmp->isstatic = parse_boolean(node->xmlChildrenNode);
        }
        free(nodename);
        node = node->next;
    }
}

umlattrlist parse_attributes(xmlNodePtr node) {
    umlattrlist list = NULL, an;
    while ( node != NULL ) {
        an = (umlattrlist) my_malloc(sizeof(umlattrnode));
        an->next = NULL;
        parse_attribute(node->xmlChildrenNode, &(an->key));
        list = insert_attribute(an, list);
        node = node->next;
    }
    return list;
}

void parse_operation(xmlNodePtr node, umloperation *tmp) {
    xmlChar *nodename;
    parse_attribute(node, &(tmp->attr));
    while ( node != NULL ) {
        nodename = xmlGetProp(node, "name");
        if ( ! strcmp("parameters", nodename) ) {
            tmp->parameters = parse_attributes(node->xmlChildrenNode);
        }
        free(nodename);
        node = node->next;
    }
}

umloplist parse_operations(xmlNodePtr node) {
    umloplist list = NULL, on;
    while ( node != NULL ) {
        on = (umlopnode*) my_malloc(sizeof(umlopnode));
        on->next = NULL;
        on->key.implementation = NULL;
        parse_operation(node->xmlChildrenNode, &(on->key));
        list = insert_operation(on, list);
        node = node->next;
    }
    return list;
}

void parse_template(xmlNodePtr node, umltemplate *tmp) {
    sscanf(node->xmlChildrenNode->xmlChildrenNode->content, "#%79[^#]#", tmp->name);
    sscanf(node->next->xmlChildrenNode->xmlChildrenNode->content, "#%79[^#]#", tmp->type);
}

umltemplatelist parse_templates(xmlNodePtr node) {
    umltemplatelist list = NULL, tn;
    while ( node != NULL) {
        if ( node->xmlChildrenNode->xmlChildrenNode->xmlChildrenNode != NULL &&
                node->xmlChildrenNode->next->xmlChildrenNode->xmlChildrenNode != NULL ) {

            tn = (umltemplatenode*) my_malloc(sizeof(umltemplatenode));
            tn->next = NULL;
            parse_template(node->xmlChildrenNode, &(tn->key));
            list = insert_template(tn, list);
        }
        node = node->next;
    }
    return list;
}

/**
  * Adds get() (or is()) and set() methods for each attribute
  * myself MUST be != null
*/
void make_javabean_methods(umlclass *myself) {
    char *tmpname;
    umloplist operation;
    umlattrlist attrlist, parameter;

    attrlist = myself->attributes;
    while (attrlist != NULL) {
        if ( ! attrlist->key.isabstract) {
            /* The SET method */
            operation = (umloplist) my_malloc (sizeof(umlopnode));
            parameter = (umlattrlist) my_malloc (sizeof(umlattrnode));

            sprintf(parameter->key.name, "value");
            strncpy(parameter->key.type, attrlist->key.type, 79);
            parameter->key.value[0] = 0;
            parameter->key.isstatic = 0;
            parameter->key.isabstract = 0;
            parameter->key.visibility = '0';
            parameter->next = NULL;
            operation->key.parameters = parameter;

            operation->key.implementation = (char*) my_malloc (90);
            sprintf(operation->key.implementation, "    ");
            strncat(operation->key.implementation, attrlist->key.name, 76);
            strcat(operation->key.implementation, " = value;");

            tmpname = strtoupperfirst(attrlist->key.name);
            sprintf(operation->key.attr.name, "set");
            strncat(operation->key.attr.name, tmpname, 76);
            free(tmpname);
            operation->key.attr.isabstract = 0;
            operation->key.attr.isstatic = 0;
            operation->key.attr.visibility = '0';
            operation->key.attr.value[0] = 0;
            sprintf(operation->key.attr.type, "void");
            operation->next = NULL;

            myself->operations = insert_operation(operation, myself->operations);

            /* The GET or IS method */
            operation = (umloplist) my_malloc (sizeof(umlopnode));
            operation->key.parameters = NULL;
            tmpname = strtoupperfirst(attrlist->key.name);
            if ( ! strcmp(attrlist->key.type, "boolean") ) {
                sprintf(operation->key.attr.name, "is");
            } else {
                sprintf(operation->key.attr.name, "get");
            }
            strncat(operation->key.attr.name, tmpname, 76);
            free(tmpname);

            operation->key.implementation = (char*) my_malloc (90);
            sprintf(operation->key.implementation, "    return ");
            strncat(operation->key.implementation, attrlist->key.name, 76);
            strcat(operation->key.implementation, ";");

            operation->key.attr.isabstract = 0;
            operation->key.attr.isstatic = 0;
            operation->key.attr.visibility = '0';
            operation->key.attr.value[0] = 0;
            strncpy(operation->key.attr.type, attrlist->key.type, 79);
            operation->next = NULL;

            myself->operations = insert_operation(operation, myself->operations);
        }
        attrlist = attrlist->next;
    }
}

void parse_geom_position(xmlNodePtr attribute, geometry * geom ) {
    xmlChar *val;
    char * token;
    val = xmlGetProp(attribute, "val");
    token = strtok(val,",");
    sscanf ( token, "%f", &(geom->pos_x) );
    token = strtok(NULL,",");
    sscanf ( token, "%f", &(geom->pos_y) );
}

void parse_geom_width(xmlNodePtr attribute, geometry * geom ) {
    xmlChar *val;
    val = xmlGetProp(attribute, "val");
    sscanf ( val, "%f", &(geom->width) );
}

void parse_geom_height(xmlNodePtr attribute, geometry * geom ) {
    xmlChar *val;
    val = xmlGetProp(attribute, "val");
    sscanf ( val, "%f", &(geom->height) );
}


/*
   returns 1 if the position point of the object with geom2 is inside the object with geom1
           0 otherwise
*/
int is_inside(const geometry * geom1, const geometry * geom2) {
    return geom1->pos_x < geom2->pos_x && geom2->pos_x < (geom1->pos_x+geom1->width) &&
           geom1->pos_y < geom2->pos_y && geom2->pos_y < (geom1->pos_y+geom1->height);

}

umlpackagelist parse_package(xmlNodePtr package) {
    xmlNodePtr attribute;
    umlpackagelist listmyself;
    umlpackage *myself;
    xmlChar *attrname;

    listmyself = (umlpackagelist) my_malloc ( sizeof ( umlpackagenode));
    myself = (umlpackage*) my_malloc(sizeof(umlpackage));

    myself->parent = NULL;

    listmyself->next = NULL;
    listmyself->key = myself;

    attribute = package->xmlChildrenNode;
    while ( attribute != NULL ) {
        attrname = xmlGetProp(attribute, "name");
        if ( ! strcmp("name", attrname) ) {
            parse_dia_string(attribute->xmlChildrenNode, myself->name);
        } else if ( ! strcmp ( "obj_pos", attrname ) ) {
            parse_geom_position(attribute->xmlChildrenNode, &myself->geom );
        } else if ( ! strcmp ( "elem_width", attrname ) ) {
            parse_geom_width(attribute->xmlChildrenNode, &myself->geom );
        } else if ( ! strcmp ( "elem_height", attrname ) ) {
            parse_geom_height(attribute->xmlChildrenNode, &myself->geom );
        }
        attribute = attribute->next;
    }
    return listmyself;
}

umlclasslist parse_class(xmlNodePtr class) {
    xmlNodePtr attribute;
    xmlChar *attrname;
    umlclasslist listmyself;
    umlclass *myself;

    listmyself = (umlclasslist) my_malloc (sizeof (umlclassnode));
    myself = (umlclass*) my_malloc (sizeof(umlclass));

    myself->package = NULL;

    listmyself->key = myself;
    listmyself->parents = NULL;
    listmyself->associations = NULL;
    listmyself->dependencies = NULL;
    listmyself->next = NULL;

    attribute = class->xmlChildrenNode;
    while ( attribute != NULL ) {
        attrname = xmlGetProp(attribute, "name");
        if ( ! strcmp("name", attrname) ) {
            parse_dia_string(attribute->xmlChildrenNode, myself->name);
        } else if ( ! strcmp ( "obj_pos", attrname ) ) {
            parse_geom_position(attribute->xmlChildrenNode, &myself->geom );
        } else if ( ! strcmp ( "elem_width", attrname ) ) {
            parse_geom_width(attribute->xmlChildrenNode, &myself->geom );
        } else if ( ! strcmp ( "elem_height", attrname ) ) {
            parse_geom_height(attribute->xmlChildrenNode, &myself->geom );
        } else if ( ! strcmp("comment", attrname))  {
            if (attribute->xmlChildrenNode->xmlChildrenNode != NULL) {
               parse_dia_string(attribute->xmlChildrenNode, myself->comment);
            }  else {
               myself->comment[0] = 0;
            }
        } else if ( ! strcmp("stereotype", attrname) ) {
            if ( attribute->xmlChildrenNode->xmlChildrenNode != NULL ) {
                parse_dia_string(attribute->xmlChildrenNode, myself->stereotype);
            } else {
                myself->stereotype[0] = 0;
            }
        } else if ( ! strcmp("abstract", attrname) ) {
            myself->isabstract = parse_boolean(attribute->xmlChildrenNode);
        } else if ( ! strcmp("attributes", attrname) ) {
            myself->attributes = parse_attributes(attribute->xmlChildrenNode);
        } else if ( ! strcmp("operations", attrname) ) {
            myself->operations = parse_operations(attribute->xmlChildrenNode);
            if ( ! strcmp(myself->stereotype, "JavaBean")) {
                /* Javabean: we should now add a get() and set() methods
                for each attribute */
                make_javabean_methods(myself);
            }
        } else if ( ! strcmp("templates", attrname) ) {
            myself->templates = parse_templates(attribute->xmlChildrenNode);
        }
        free(attrname);
        attribute = attribute->next;
    }
    return listmyself;
}

/**
  Simple, non-compromising, implementation declaration.
  This function creates a plain vanilla interface (an
  umlclasslist) and associates it to the implementator.
  The implementator's code should contain the interface
  name, but the interface itself will not be inserted
  into the classlist, so no code can be generated for it.
*/
void lolipop_implementation(umlclasslist classlist, xmlNodePtr object) {
    xmlNodePtr attribute;
    xmlChar *id = NULL, *name = NULL;
    xmlChar *attrname;
    umlclasslist interface, implementator;

    attribute = object->xmlChildrenNode;
    while ( attribute != NULL ) {
        if ( ! strcmp("connections", attribute->name) ) {
            id = xmlGetProp(attribute->xmlChildrenNode, "to");
        } else {
            attrname = xmlGetProp(attribute, "name");
            if ( ! strcmp("text", attrname) && attribute->xmlChildrenNode != NULL &&
                    attribute->xmlChildrenNode->xmlChildrenNode != NULL ){
                name = attribute->xmlChildrenNode->xmlChildrenNode->content;
            } else {
                name = "";
            }
            free(attrname);
        }
        attribute = attribute->next;
    }
    implementator = find(classlist, id);
    free(id);
    if (implementator != NULL && strlen(name) > 2) {
        interface = (umlclasslist) my_malloc (sizeof (umlclassnode));
        interface->key = (umlclass *) my_malloc (sizeof(umlclass));
        interface->parents = NULL;
        interface->next = NULL;
        sprintf(interface->key->id, "00");
        sscanf(name, "#%79[^#]#", interface->key->name);
        sprintf(interface->key->stereotype, "Interface");
        interface->key->isabstract = 1;
        interface->key->attributes = NULL;
        interface->key->operations = NULL;
        addparent(interface, implementator);
        /* we MUST NOT free interface at this point */
    }
}

void ricursive_search(xmlNodePtr node, xmlNodePtr * object) {
    xmlNodePtr child;
    if ( *object != NULL ) {
        return;
    }
    if ( node != NULL ) {
        if ( ! strcmp(node->name,"object") ){
            *object = node;
            return;
        }
        child = node->xmlChildrenNode;
        while ( child != NULL ) {
            ricursive_search(child,object);
            child=child->next;
        }
    }
}

/* Gets the next "object" node. Basically, gets from->next.  When
   it is null it checks for from->parent->next.
   FIXME: the code is ugly */
xmlNodePtr getNextObject(xmlNodePtr from) {
    xmlNodePtr next = NULL;
    if ( from->next != NULL ){
        next = from->next;
        if ( ! strcmp(next->name,"group") ){
            next = next->xmlChildrenNode;
            while ( ! strcmp(next->name,"group") ){
                next=next->xmlChildrenNode;
            }
        }
        return next;
    }
    next = from->parent->next;
    if ( next != NULL ){
        if ( ! strcmp(next->name,"group") ){
            next = next->xmlChildrenNode;
            while ( ! strcmp(next->name,"group") ){
                next=next->xmlChildrenNode;
            }
            return next;
        }
        if ( ! strcmp(next->name,"layer") ){
            return next->xmlChildrenNode;
        }
        return next;
    }
    return NULL;
}

umlclasslist parse_diagram(char *diafile) {
    xmlDocPtr ptr;
    xmlChar *objtype, *attrtype, *tmptype, *tmptype2;
    xmlChar *objid;
    xmlChar *end1;
    xmlChar *end2;

    xmlNodePtr  object = NULL, attribute;
    umlclasslist classlist, tmplist, endlist = NULL;
    umlpackagelist packagelist, dummypcklist, tmppcklist, endpcklist = NULL;

    ptr = xmlParseFile(diafile);

    if (ptr == NULL) {
        fprintf(stderr, "That file does not exist or is not a Dia diagram\n");
        exit(2);
    }

    /* we search for the first "object" node */
    ricursive_search( ptr->xmlRootNode->xmlChildrenNode->next, &object );

    classlist = NULL;
    packagelist = NULL;

    while (object != NULL) {
        objtype = xmlGetProp(object, "type");
        /* Here we have a Dia object */
        if ( !strcmp("UML - Class", objtype) ) {
            /* Here we have a class definition */
            tmplist = parse_class(object);
            if (tmplist != NULL) {
                /* We get the ID of the object here*/
                objid = xmlGetProp(object, "id");
                sscanf(objid, "%79s", tmplist->key->id);
                free(objid);

                /* We insert it here*/
                if ( classlist == NULL ) {
                    classlist = endlist = tmplist;
                } else {
                    endlist->next = tmplist;
                    endlist = tmplist;
                }
            }
        } else if ( ! strcmp("UML - LargePackage",objtype) || ! strcmp("UML - SmallPackage",objtype)) {
            tmppcklist = parse_package(object);
            if ( tmppcklist != NULL ) {
                /* We get the ID of the object here*/
                objid = xmlGetProp(object, "id");
                sscanf(objid, "%79s", tmppcklist->key->id);
                free(objid);
            }
            /* We insert it here*/
            if ( packagelist == NULL ) {
                packagelist = endpcklist = tmppcklist;
            } else {
                endpcklist->next = tmppcklist;
                endpcklist = tmppcklist;
            }

        }
        free(objtype);
        object = getNextObject(object);
    }

    /* Second pass - Implementations and associations */

    /* The association is done as a queue, so we must first put in
         realizations (interfaces) and then generalizations (inheritance)
         so we will have the latter first and the former after (!)
         THIS STILL SUCKS !!! How soon is now? */

    /* we search for the first "object" node */
    ricursive_search( ptr->xmlRootNode->xmlChildrenNode->next, &object );

    while ( object != NULL ) {
        objtype = xmlGetProp(object, "type");
        if ( ! strcmp("UML - Association", objtype) ) {
            char * name = NULL;
            char direction = 0;
            char composite = 0;
            attribute = object->xmlChildrenNode;

            while ( attribute != NULL) {
                attrtype = xmlGetProp(attribute, "name");
                if ( attrtype != NULL && ! strcmp("ends", attrtype) ) {
                    tmptype = xmlGetProp(attribute->xmlChildrenNode->xmlChildrenNode->next->next->next->xmlChildrenNode, "val");
                    if ( ! strcmp("0", tmptype) ) {
                        direction = 1;
                        if (attribute->xmlChildrenNode->xmlChildrenNode->xmlChildrenNode->xmlChildrenNode != NULL) {
                            name = attribute->xmlChildrenNode->xmlChildrenNode->xmlChildrenNode->xmlChildrenNode->content;
                        }
                        tmptype2 = xmlGetProp(attribute->xmlChildrenNode->next->xmlChildrenNode->next->next->next->xmlChildrenNode, "val");
                        if (!strcmp("1", tmptype2)) {
                            composite = 1;
                        } else {
                            composite = 0;
                        }
                        free(tmptype2);
                    } else {
                        direction = 0;
                        if (attribute->xmlChildrenNode->next->xmlChildrenNode->xmlChildrenNode->xmlChildrenNode != NULL) {
                            name = attribute->xmlChildrenNode->next->xmlChildrenNode->xmlChildrenNode->xmlChildrenNode->content;
                        }
                        tmptype2 = xmlGetProp(attribute->xmlChildrenNode->xmlChildrenNode->next->next->next->xmlChildrenNode, "val");
                        if (!strcmp("1", tmptype2)) {
                            composite = 1;
                        } else {
                            composite = 0;
                        }
                        free(tmptype2);
                    }
                    free(tmptype);
                }
                free(attrtype);
                attribute = attribute->next;
            }
            attribute = object->xmlChildrenNode;
            while ( attribute != NULL ) {
                if ( ! strcmp("connections", attribute->name) ) {
                    end1 = xmlGetProp(attribute->xmlChildrenNode, "to");
                    end2 = xmlGetProp(attribute->xmlChildrenNode->next, "to");
                    if (direction == 1) {
                        associate(classlist, name, composite, end1, end2);
                    } else {
                        associate(classlist, name, composite, end2, end1);
                    }
                    free(end1);
                    free(end2);
                }
                attribute = attribute->next;
            }
        } else if ( ! strcmp("UML - Dependency", objtype) ) {
            attribute = object->xmlChildrenNode;
            while ( attribute != NULL ) {
                if ( ! strcmp("connections", attribute->name) ) {
                    end1 = xmlGetProp(attribute->xmlChildrenNode, "to");
                    end2 = xmlGetProp(attribute->xmlChildrenNode->next, "to");
                    make_depend(classlist, end1, end2);
                    free(end1);
                    free(end2);
                }
                attribute = attribute->next;
            }
        } else if ( ! strcmp("UML - Realizes", objtype) ) {
            attribute = object->xmlChildrenNode;
            while ( attribute != NULL ) {
                if ( ! strcmp("connections", attribute->name) ) {
                    end1 = xmlGetProp(attribute->xmlChildrenNode, "to");
                    end2 = xmlGetProp(attribute->xmlChildrenNode->next, "to");
                    inherit_realize(classlist, end1, end2);
                    free(end2);
                    free(end1);
                }
                attribute = attribute->next;
            }
        } else if ( ! strcmp("UML - Implements", objtype) ) {
            lolipop_implementation(classlist, object);
        }
        free(objtype);
        object = getNextObject(object);
    }


    /* Generalizations: we must put this AFTER all the interface
       implementations. generate_code_java relies on this. */
    ricursive_search( ptr->xmlRootNode->xmlChildrenNode->next, &object );
    while ( object != NULL ) {
        objtype = xmlGetProp(object, "type");
        if ( ! strcmp("UML - Generalization", objtype) ) {
            attribute = object->xmlChildrenNode;
            while ( attribute != NULL ) {
                if ( ! strcmp("connections", attribute->name) ) {
                    end1 = xmlGetProp(attribute->xmlChildrenNode, "to");
                    end2 = xmlGetProp(attribute->xmlChildrenNode->next, "to");
                    inherit_realize(classlist, end1, end2);
                    free(end2);
                    free(end1);
                }
                attribute = attribute->next;
            }
        }
        free(objtype);
        object = getNextObject(object);
    }

    /* Packages: we should scan the packagelist and then the classlist.
       Scanning the packagelist we'll build all relationships between
       packages.  Scanning the classlist we'll associate its own package
       to each class.

       FIXME: maybe we can do both in the same pass */

    /* Build the relationships between packages */
    dummypcklist = packagelist;
    while ( dummypcklist != NULL ) {
        tmppcklist = packagelist;
        while ( tmppcklist != NULL ) {
            if ( is_inside(&dummypcklist->key->geom,&tmppcklist->key->geom) ) {
                if ( tmppcklist->key->parent == NULL ) {
                    tmppcklist->key->parent = dummypcklist->key;
                } else {
                    if ( ! is_inside ( &dummypcklist->key->geom, &tmppcklist->key->parent->geom ) ) {
                        tmppcklist->key->parent = dummypcklist->key;
                    }
                }
            }
            tmppcklist = tmppcklist->next;
        }
        dummypcklist = dummypcklist->next;
    }

    /* Associate packages to classes */
    dummypcklist = packagelist;
    while ( dummypcklist != NULL ) {
        tmplist = classlist;
        while ( tmplist != NULL ) {
            if ( is_inside(&dummypcklist->key->geom,&tmplist->key->geom) ) {
                if ( tmplist->key->package == NULL ) {
                    tmplist->key->package = dummypcklist->key;
                } else {
                    if ( ! is_inside ( &dummypcklist->key->geom, &tmplist->key->package->geom ) ) {
                        tmplist->key->package = dummypcklist->key;
                    }
                }
            }
            tmplist = tmplist->next;
        }
        dummypcklist = dummypcklist->next;
    }

    return classlist;
}
