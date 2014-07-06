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

#ifndef MIN
#define MIN(x, y) (x < y ? x : y)
#endif

/* In case of unnamed associations, the attribute will get the name
   "unnamed_" followed by the anon_cnt converted to string:  */
static unsigned anon_cnt = 0;

static char *sscanfmt()
{
    static char buf[16];
    sprintf (buf, "#%%%d[^#]#", SMALL_BUFFER - 1);
    return buf;
}

umlclasslist find(umlclasslist list, char *id ) {
    if ( id != NULL ) {
        while ( list != NULL ) {
            if ( eq(id, list->key->id) ) {
                return list;
            }
            list = list->next;
        }
    }
    return NULL;
}

/* the buffer must have room for SMALL_BUFFER characters */
void parse_dia_string(xmlNodePtr stringnode, char *buffer) {
    xmlChar *content;

    content = xmlNodeGetContent(stringnode);
    if (sscanf(content, sscanfmt(), buffer) == 0) {
        buffer[0] = 0;
    }
    free(content);
}

/* the buffer must have room for LARGE_BUFFER characters */
void parse_dia_string_large(xmlNodePtr stringnode, char *buffer) {
    xmlChar *content;
    char fmt[16];

    content = xmlNodeGetContent(stringnode);
    strncpy (buffer, content + 1, MIN(strlen(content) - 2, LARGE_BUFFER - 1));
    free(content);
}

int parse_boolean(xmlNodePtr booleannode) {
    xmlChar *val;
    int result;

    val = xmlGetProp(booleannode, "val");
    if ( val != NULL && eq(val, "true")) {
        result = 1;
    } else {
        result = 0;
    }
    free(val);
    return result;
}


void addparent(umlclasslist base, umlclasslist derived) {
    umlclasslist tmp;
    tmp = NEW (umlclassnode);
    tmp->key = base->key;
    tmp->next = derived->parents;
    derived->parents = tmp;
}

void adddependency(umlclasslist dependent, umlclasslist dependee) {
    umlclasslist tmp;
    tmp = NEW (umlclassnode);
    tmp->key = dependent->key;
    tmp->next = dependee->dependencies;
    dependee->dependencies = tmp;
}

void addaggregate(char *name, char composite, umlclasslist base,
                  umlclasslist associate, char *multiplicity) {
    umlassoclist tmp;
    tmp = NEW (umlassocnode);
    if (name != NULL) {
        sscanf(name, sscanfmt(), tmp->name);
        if (tmp->name[0] == '\0') {
            ++anon_cnt;
            sprintf (tmp->name, "unnamed_%d", anon_cnt);
        }
    } else {
        printf("warning: unnamed association between %s and %s\n", base->key->name, associate->key->name);
        strcpy(tmp->name, "unnamed");
    }
    if (multiplicity != NULL)
        sscanf(multiplicity, sscanfmt(), tmp->multiplicity);
    else
        sprintf(tmp->multiplicity, "1");
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

void associate ( umlclasslist classlist, char * name, char composite,
                 char * base, char * aggregate, char *multiplicity) {
    umlclasslist umlbase, umlaggregate;
    umlbase = find(classlist, base);
    umlaggregate = find(classlist, aggregate);
    if ( umlbase != NULL && umlaggregate != NULL) {
        addaggregate(name, composite, umlbase, umlaggregate, multiplicity);
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
        if ( eq("name", nodename) ) {
            parse_dia_string(node->xmlChildrenNode, tmp->name);
        } else if ( eq("value", nodename)) {
            if (node->xmlChildrenNode->xmlChildrenNode != NULL) {
                parse_dia_string(node->xmlChildrenNode, tmp->value);
            }
        } else if ( eq("type", nodename)) {
            if (node->xmlChildrenNode->xmlChildrenNode != NULL) {
                parse_dia_string(node->xmlChildrenNode, tmp->type);
            } else {
                tmp->type[0] = 0;
            }
        } else if ( eq("comment", nodename)) {
            if (node->xmlChildrenNode->xmlChildrenNode != NULL) {
               parse_dia_string_large(node->xmlChildrenNode, tmp->comment);
            } else {
               tmp->comment[0] = 0;
          }
        } else if ( eq("kind", nodename)) {
            attrval = xmlGetProp(node->xmlChildrenNode, "val");
            sscanf(attrval, "%c", &(tmp->kind));
            free(attrval);
        } else if ( eq("visibility", nodename)) {
            attrval = xmlGetProp(node->xmlChildrenNode, "val");
            sscanf(attrval, "%c", &(tmp->visibility));
            free(attrval);
        } else if ( eq("abstract", nodename)) {
            tmp->isabstract = parse_boolean(node->xmlChildrenNode);
        } else if ( eq("class_scope", nodename)) {
            tmp->isstatic = parse_boolean(node->xmlChildrenNode);
        } else if ( eq("query", nodename)) {
            tmp->isconstant = parse_boolean(node->xmlChildrenNode);
        }
        free(nodename);
        node = node->next;
    }
}

umlattrlist parse_attributes(xmlNodePtr node) {
    umlattrlist list = NULL, an;
    while ( node != NULL ) {
        an = NEW (umlattrnode);
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
        if ( eq("parameters", nodename) ) {
            tmp->parameters = parse_attributes(node->xmlChildrenNode);
        }
        free(nodename);
        node = node->next;
    }
}

umloplist parse_operations(xmlNodePtr node) {
    umloplist list = NULL, on;
    while ( node != NULL ) {
        on = NEW (umlopnode);
        on->next = NULL;
        on->key.implementation = NULL;
        parse_operation(node->xmlChildrenNode, &(on->key));
        list = insert_operation(on, list);
        node = node->next;
    }
    return list;
}

void parse_template(xmlNodePtr node, umltemplate *tmp) {
    sscanf(node->xmlChildrenNode->xmlChildrenNode->content, sscanfmt(), tmp->name);
    sscanf(node->next->xmlChildrenNode->xmlChildrenNode->content, sscanfmt(), tmp->type);
}

umltemplatelist parse_templates(xmlNodePtr node) {
    umltemplatelist list = NULL, tn;
    while ( node != NULL) {
        if ( node->xmlChildrenNode->xmlChildrenNode->xmlChildrenNode != NULL &&
                node->xmlChildrenNode->next->xmlChildrenNode->xmlChildrenNode != NULL ) {
            tn = NEW (umltemplatenode);
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
            operation = NEW (umlopnode);
            parameter = NEW (umlattrnode);

            sprintf(parameter->key.name, "value");
            strncpy(parameter->key.type, attrlist->key.type, SMALL_BUFFER);
            parameter->key.value[0] = 0;
            parameter->key.isstatic = 0;
            parameter->key.isconstant = 0;
            parameter->key.isabstract = 0;
            parameter->key.visibility = '0';
            parameter->next = NULL;
            operation->key.parameters = parameter;

            operation->key.implementation = (char*) my_malloc (BIG_BUFFER);
            sprintf(operation->key.implementation, "    ");
            strcat(operation->key.implementation, attrlist->key.name);
            strcat(operation->key.implementation, " = value;");

            tmpname = strtoupperfirst(attrlist->key.name);
            sprintf(operation->key.attr.name, "set");
            strncat(operation->key.attr.name, tmpname, SMALL_BUFFER - 1);
            free(tmpname);
            operation->key.attr.isabstract = 0;
            operation->key.attr.isstatic = 0;
            operation->key.attr.isconstant = 0;
            operation->key.attr.visibility = '0';
            operation->key.attr.value[0] = 0;
            sprintf(operation->key.attr.type, "void");
            operation->next = NULL;

            myself->operations = insert_operation(operation, myself->operations);

            /* The GET or IS method */
            operation = NEW (umlopnode);
            operation->key.parameters = NULL;
            tmpname = strtoupperfirst(attrlist->key.name);
            if ( eq(attrlist->key.type, "boolean") ) {
                sprintf(operation->key.attr.name, "is");
            } else {
                sprintf(operation->key.attr.name, "get");
            }
            strncat(operation->key.attr.name, tmpname, SMALL_BUFFER - 1);
            free(tmpname);

            operation->key.implementation = (char*) my_malloc (BIG_BUFFER);
            sprintf(operation->key.implementation, "    return ");
            strcat(operation->key.implementation, attrlist->key.name);
            strcat(operation->key.implementation, ";");

            operation->key.attr.isabstract = 0;
            operation->key.attr.isstatic = 0;
            operation->key.attr.isconstant = 0;
            operation->key.attr.visibility = '0';
            operation->key.attr.value[0] = 0;
            strncpy(operation->key.attr.type, attrlist->key.type, SMALL_BUFFER - 1);
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
    //debug( 4, "parse_package %s", package->name );

    listmyself = NEW (umlpackagenode);
    myself = NEW (umlpackage);

    myself->parent = NULL;

    listmyself->next = NULL;
    listmyself->key = myself;

    attribute = package->xmlChildrenNode;
    while ( attribute != NULL ) {
        attrname = xmlGetProp(attribute, "name");
        /* fix a segfault - dia files contains *also* some rare tags without any "name" attribute : <dia:parent  for ex.  */
        if( attrname != NULL ) {
            if ( eq("name", attrname) ) {
                parse_dia_string(attribute->xmlChildrenNode, myself->name);
                //debug( 4, "name is %s \n", myself->name );
            } else if ( eq ( "obj_pos", attrname ) ) {
                parse_geom_position(attribute->xmlChildrenNode, &myself->geom );
            } else if ( eq ( "elem_width", attrname ) ) {
                parse_geom_width(attribute->xmlChildrenNode, &myself->geom );
            } else if ( eq ( "elem_height", attrname ) ) {
                parse_geom_height(attribute->xmlChildrenNode, &myself->geom );
            }
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

    listmyself = NEW (umlclassnode);
    myself = NEW (umlclass);
    myself->package = NULL;

    listmyself->key = myself;
    listmyself->parents = NULL;
    listmyself->associations = NULL;
    listmyself->dependencies = NULL;
    listmyself->next = NULL;

    attribute = class->xmlChildrenNode;
    while ( attribute != NULL ) {
        attrname = xmlGetProp(attribute, "name");
        /* fix a segfault - dia files contains *also* some rare tags without any "name" attribute : <dia:parent  for ex.  */
        if( attrname == NULL ) {
            attribute = attribute->next;
            continue;
        }
        if ( eq("name", attrname) ) {
            parse_dia_string(attribute->xmlChildrenNode, myself->name);
        } else if ( eq ( "obj_pos", attrname ) ) {
            parse_geom_position(attribute->xmlChildrenNode, &myself->geom );
        } else if ( eq ( "elem_width", attrname ) ) {
            parse_geom_width(attribute->xmlChildrenNode, &myself->geom );
        } else if ( eq ( "elem_height", attrname ) ) {
            parse_geom_height(attribute->xmlChildrenNode, &myself->geom );
        } else if ( eq("comment", attrname))  {
            if (attribute->xmlChildrenNode->xmlChildrenNode != NULL) {
               parse_dia_string_large(attribute->xmlChildrenNode, myself->comment);
            }  else {
               myself->comment[0] = 0;
            }
        } else if ( eq("stereotype", attrname) ) {
            if ( attribute->xmlChildrenNode->xmlChildrenNode != NULL ) {
                parse_dia_string(attribute->xmlChildrenNode, myself->stereotype);
            } else {
                myself->stereotype[0] = 0;
            }
        } else if ( eq("abstract", attrname) ) {
            myself->isabstract = parse_boolean(attribute->xmlChildrenNode);
        } else if ( eq("attributes", attrname) ) {
            myself->attributes = parse_attributes(attribute->xmlChildrenNode);
        } else if ( eq("operations", attrname) ) {
            myself->operations = parse_operations(attribute->xmlChildrenNode);
            if ( eq(myself->stereotype, "JavaBean")) {
                /* Javabean: we should now add a get() and set() methods
                for each attribute */
                make_javabean_methods(myself);
            }
        } else if ( eq("templates", attrname) ) {
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
        if ( eq("connections", attribute->name) ) {
            id = xmlGetProp(attribute->xmlChildrenNode, "to");
        } else {
            attrname = xmlGetProp(attribute, "name");
            if ( attrname != NULL && eq("text", attrname) && attribute->xmlChildrenNode != NULL &&
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
        interface = NEW (umlclassnode);
        interface->key = NEW (umlclass);
        interface->parents = NULL;
        interface->next = NULL;
        sprintf(interface->key->id, "00");
        sscanf(name, sscanfmt(), interface->key->name);
        sprintf(interface->key->stereotype, "Interface");
        interface->key->isabstract = 1;
        interface->key->attributes = NULL;
        interface->key->operations = NULL;
        addparent(interface, implementator);
        /* we MUST NOT free interface at this point */
    }
}

void recursive_search(xmlNodePtr node, xmlNodePtr * object) {
    xmlNodePtr child;
    if ( *object != NULL ) {
        return;
    }
    if ( node != NULL ) {
        if ( eq(node->name,"object") ){
            *object = node;
            return;
        }
        child = node->xmlChildrenNode;
        while ( child != NULL ) {
            recursive_search(child,object);
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
        if ( eq(next->name,"group") ){
            next = next->xmlChildrenNode;
            while ( eq(next->name,"group") ){
                next=next->xmlChildrenNode;
            }
        }
        return next;
    }
    next = from->parent->next;
    if ( next != NULL ){
        if ( eq(next->name,"group") ){
            next = next->xmlChildrenNode;
            while ( eq(next->name,"group") ){
                next=next->xmlChildrenNode;
            }
            return next;
        }
        if ( eq(next->name,"layer") ){
            return next->xmlChildrenNode;
        }
        return next;
    }
    return NULL;
}

umlclasslist parse_diagram(char *diafile) {
    xmlDocPtr ptr;
    xmlChar *end1 = NULL;
    xmlChar *end2 = NULL;

    xmlNodePtr object = NULL;
    umlclasslist classlist = NULL, endlist = NULL;
    umlpackagelist packagelist = NULL, dummypcklist, endpcklist = NULL;

    ptr = xmlParseFile(diafile);

    if (ptr == NULL) {
        fprintf(stderr, "That file does not exist or is not a Dia diagram\n");
        exit(2);
    }

    /* we search for the first "object" node */
    recursive_search( ptr->xmlRootNode->xmlChildrenNode->next, &object );

    while (object != NULL) {
        xmlChar *objtype = xmlGetProp(object, "type");
        /* Here we have a Dia object */
        if ( eq("UML - Class", objtype) ) {
            /* Here we have a class definition */
            umlclasslist tmplist = parse_class(object);
            if (tmplist != NULL) {
                /* We get the ID of the object here*/
                xmlChar *objid = xmlGetProp(object, "id");
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
        } else if ( eq("UML - LargePackage", objtype) || eq("UML - SmallPackage", objtype) ) {
            umlpackagelist tmppcklist = parse_package(object);
            if ( tmppcklist != NULL ) {
                /* We get the ID of the object here*/
                xmlChar *objid = xmlGetProp(object, "id");
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
    recursive_search( ptr->xmlRootNode->xmlChildrenNode->next, &object );

    while ( object != NULL ) {
        xmlChar *objtype = xmlGetProp(object, "type");
        if ( eq("UML - Association", objtype) ) {
            char *name = NULL, *name_a = NULL, *name_b = NULL;
            char *multiplicity_a = NULL, *multiplicity_b = NULL;
            char direction = 0;
            char composite = 0;
            xmlNodePtr attribute = object->xmlChildrenNode;

            while (attribute != NULL) {
                xmlChar *attrtype = xmlGetProp(attribute, "name");

                if (attrtype != NULL) {
                    xmlNodePtr child = attribute->xmlChildrenNode;
                    if ( eq("direction", attrtype) ) {
                        xmlChar *tmptype = xmlGetProp(child, "val");
                        if ( eq(tmptype, "0") ) {
                            direction = 1;
                        }
                        else {
                            direction = 0;
                        }
                        free(tmptype);
                    }
                    else if ( eq("assoc_type", attrtype) ) {
                        xmlChar *tmptype = xmlGetProp(child, "val");
                        if ( eq(tmptype, "1") ) {
                            composite = 0;
                        }
                        else {
                            composite = 1;
                        }
                        free(tmptype);
                    }
                    else if ( child->xmlChildrenNode ) {
                        xmlNodePtr grandchild = child->xmlChildrenNode;
                        if ( eq(attrtype, "name") ) {
                            name = grandchild->content;
                        }
                        else if ( eq(attrtype, "role_a") ) {
                            name_a = grandchild->content;
                        }
                        else if ( eq(attrtype, "role_b") ) {
                            name_b = grandchild->content;
                        }
                        else if ( eq(attrtype, "multipicity_a") ) {
                            multiplicity_a = grandchild->content;
                        }
                        else if ( eq(attrtype, "multipicity_b") ) {
                            multiplicity_b = grandchild->content;
                        } 
                        else if ( eq(attrtype, "ends") ) {
                            if ( eq(child->name, "composite") ) {
				while (grandchild) {
                                    xmlNodePtr ggchild = grandchild->xmlChildrenNode;
				    if (ggchild->xmlChildrenNode) {
                                        attrtype = xmlGetProp(grandchild, "name");
                                        if ( eq(attrtype, "role") ) {
                                            name_a = ggchild->xmlChildrenNode->content;
                                        }
                                        else if ( eq(attrtype, "multiplicity") ) {
                                            multiplicity_a = ggchild->xmlChildrenNode->content;
                                        }
                                        else if ( eq(attrtype, "aggregate") ) {
                                            /* todo */
                                        }
                                    }
				    grandchild = grandchild->next;
                                }
                            }
                            child = child->next;
                            if ( child != NULL && eq(child->name, "composite") ) {
                                grandchild = child->xmlChildrenNode;
				while (grandchild) {
                                    xmlNodePtr ggchild = grandchild->xmlChildrenNode;
				    if (ggchild->xmlChildrenNode) {
                                        attrtype = xmlGetProp(grandchild, "name");
                                        if ( eq(attrtype, "role") ) {
                                            name_b = ggchild->xmlChildrenNode->content;
                                        }
                                        else if ( eq(attrtype, "multiplicity") ) {
                                            multiplicity_b = ggchild->xmlChildrenNode->content;
                                        }
                                        else if ( eq(attrtype, "aggregate") ) {
                                            /* todo */
                                        }
                                    }
				    grandchild = grandchild->next;
                                }
                            }
                        }
                    }
                    free(attrtype);
                }
                else if ( eq(attribute->name, "connections") ) {
                    end1 = xmlGetProp(attribute->xmlChildrenNode, "to");
                    end2 = xmlGetProp(attribute->xmlChildrenNode->next, "to");
                }

                attribute = attribute->next;
            }

            if (end1 != NULL && end2 != NULL) {
                char *thisname = name;
                if (direction == 1) {
                    if (thisname == NULL || !*thisname || eq("##", thisname))
                        thisname = name_a;
                    associate(classlist, thisname, composite, end1, end2, multiplicity_a);
                } else {
                    if (thisname == NULL || !*thisname || eq("##", thisname))
                        thisname = name_b;
                    associate(classlist, thisname, composite, end2, end1, multiplicity_b);
                }
                free(end1);
                free(end2);
            }

        } else if ( eq("UML - Dependency", objtype) ) {
            xmlNodePtr attribute = object->xmlChildrenNode;
            while ( attribute != NULL ) {
                if ( eq("connections", attribute->name) ) {
                    end1 = xmlGetProp(attribute->xmlChildrenNode, "to");
                    end2 = xmlGetProp(attribute->xmlChildrenNode->next, "to");
                    make_depend(classlist, end1, end2);
                    free(end1);
                    free(end2);
                }
                attribute = attribute->next;
            }
        } else if ( eq("UML - Realizes", objtype) ) {
            xmlNodePtr attribute = object->xmlChildrenNode;
            while ( attribute != NULL ) {
                if ( eq("connections", attribute->name) ) {
                    end1 = xmlGetProp(attribute->xmlChildrenNode, "to");
                    end2 = xmlGetProp(attribute->xmlChildrenNode->next, "to");
                    inherit_realize(classlist, end1, end2);
                    free(end2);
                    free(end1);
                }
                attribute = attribute->next;
            }
        } else if ( eq("UML - Implements", objtype) ) {
            lolipop_implementation(classlist, object);
        }
        free(objtype);
        object = getNextObject(object);
    }


    /* Generalizations: we must put this AFTER all the interface
       implementations. generate_code_java relies on this. */
    recursive_search( ptr->xmlRootNode->xmlChildrenNode->next, &object );
    while ( object != NULL ) {
        xmlChar *objtype = xmlGetProp(object, "type");
        if ( eq("UML - Generalization", objtype) ) {
            xmlNodePtr attribute = object->xmlChildrenNode;
            while ( attribute != NULL ) {
                if ( eq("connections", attribute->name) ) {
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
        umlpackagelist tmppcklist = packagelist;
        while ( tmppcklist != NULL ) {
            if ( is_inside(&dummypcklist->key->geom, &tmppcklist->key->geom) ) {
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
        umlclasslist tmplist = classlist;
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
