#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_meminfo.h"

#include "ext/standard/info.h"

#include "zend_extensions.h"
#include "zend_exceptions.h"

#include "zend.h"
#include "SAPI.h"
#include "zend_API.h"

const zend_function_entry meminfo_functions[] = {
    PHP_FE(meminfo_structs_size, NULL)
    PHP_FE(meminfo_objects_list, NULL)
    PHP_FE(meminfo_gc_roots_list, NULL)
    PHP_FE(meminfo_symbol_table, NULL)
    {NULL, NULL, NULL}
};

zend_module_entry meminfo_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
    STANDARD_MODULE_HEADER,
#endif
    "meminfo",
    meminfo_functions,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
#if ZEND_MODULE_API_NO >= 20010901
    MEMINFO_VERSION,
#endif
    STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_MEMINFO
ZEND_GET_MODULE(meminfo)
#endif

//TODO: use gettype directly ? Possible ?
char* get_type_label(zval* z) {
    switch (Z_TYPE_P(z)) {
        case IS_NULL:
            return "NULL";
            break;

        case IS_BOOL:
            return "boolean";
            break;

        case IS_LONG:
            return "integer";
            break;

        case IS_DOUBLE:
            return "double";
            break;
    
        case IS_STRING:
            return "string";
            break;
    
        case IS_ARRAY:
            return "array";
            break;

        case IS_OBJECT:
            return "object";
            break;

        case IS_RESOURCE:
            return "resource";
            break;

        default:
            return "Unknown type";
    }
}

/**
 * Return the class associated to the provided object handle
 *
 * @param zend_uint object handle
 *
 * @return char * class name
 */
const char *get_classname(zend_uint handle)
{
    zend_objects_store *objects = &EG(objects_store);
    zend_object *object;
    zend_class_entry *class_entry;
    const char* class_name;

    class_name = "";

    if (objects->object_buckets[handle].valid) {
        struct _store_object *obj = &objects->object_buckets[handle].bucket.obj;
        object =  (zend_object * ) obj->object;

        class_entry = object->ce;
        class_name = class_entry->name;
    }

    return class_name;
}

/**
 * Return the content of a property of an object handle
 *
 * @param zend_uint object handle
 * @param char *property
 * @param int *property_len
 *
 * @return char * property content
 */
char * get_property_as_string(zend_uint handle, char *property, int property_len)
{
    zend_objects_store *objects = &EG(objects_store);
    zend_object *object;
    zend_class_entry *class_entry;

    char *property_content;

    property_content = "";

    if (objects->object_buckets[handle].valid) {
        zval *z_obj;
        zval *z_property;
        zval z_property_copy;

        int use_copy = 0;
        struct _store_object *obj = &objects->object_buckets[handle].bucket.obj;

        object =  (zend_object * ) obj->object;

        class_entry = object->ce;

        MAKE_STD_ZVAL(z_obj);
        Z_TYPE_P(z_obj) = IS_OBJECT;
        Z_OBJ_HANDLE_P(z_obj) = handle;

        Z_OBJ_HT_P(z_obj) = obj->handlers;

        z_property = zend_read_property(class_entry, z_obj, property, property_len, 1 TSRMLS_CC);

        zend_make_printable_zval(z_property, &z_property_copy, &use_copy);

        if (use_copy) {
            property_content = Z_STRVAL(z_property_copy);
            zval_dtor(&z_property_copy);
        } else {
            property_content = Z_STRVAL_P(z_property);
        }
        zval_dtor(z_obj);
    }

    return property_content;
}


PHP_FUNCTION(meminfo_structs_size)
{
    zval *zval_stream;
    php_stream *stream;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zval_stream) == FAILURE) {
        return;
    }

    php_stream_from_zval(stream, &zval_stream);

    stream_printf(stream, "Structs size on this platform:\n");
    stream_printf(stream, "  Class (zend_class_entry): %ld bytes.\n", sizeof(zend_class_entry));
    stream_printf(stream, "  Object (zend_object): %ld bytes.\n", sizeof(zend_object));
    stream_printf(stream, "  Variable (zval): %ld bytes.\n", sizeof(zval));
    stream_printf(stream, "  Variable value (zvalue_value): %ld bytes.\n", sizeof(zvalue_value));
}

PHP_FUNCTION(meminfo_objects_list)
{
    zval *zval_stream;
    php_stream *stream;

    char *property;
    int property_len = 0;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r|s", &zval_stream, &property, &property_len) == FAILURE) {
        return;
    }

    php_stream_from_zval(stream, &zval_stream);

    stream_printf(stream, "Objects list:\n");

// TODO: check if object_buckets exists ? See gc_collect_roots from zend_gc.c
    zend_objects_store *objects = &EG(objects_store);
    zend_uint i;
    zend_uint total_objects_buckets = objects->top - 1;
    zend_uint current_objects = 0;
    zend_object * object;
    zend_class_entry * class_entry;
    char * property_content;

    for (i = 1; i < objects->top ; i++) {
        if (objects->object_buckets[i].valid) {
            stream_printf(stream, "  - Class %s, handle %d", get_classname(i), i);

            if (property_len > 0) {
                property_content = get_property_as_string(i, property, property_len);
                stream_printf(stream, ", property %s:%s\n", property, property_content);
            } else {
                stream_printf(stream, "\n");
            }

            current_objects++;
        }
     }

    stream_printf(stream, "Total object buckets: %d. Current objects: %d.\n", total_objects_buckets, current_objects);
}

PHP_FUNCTION(meminfo_gc_roots_list)
{
    zval *zval_stream;
    php_stream *stream;
    zval* pz;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zval_stream) == FAILURE) {
        return;
    }

    php_stream_from_zval(stream, &zval_stream);
    stream_printf(stream, "GC roots list:\n");

    gc_root_buffer *current = GC_G(roots).next;

    while (current != &GC_G(roots)) {
        pz = current->u.pz;
        stream_printf( stream, "  zval pointer: %p ", (void *) pz); 
        if (current->handle) {
            stream_printf(
                stream,
                "  Class %s, handle %d\n",
                get_classname(current->handle),
                current->handle);
        } else {
            stream_printf(stream, "  Type: %s", get_type_label(pz));
            stream_printf(stream, ", Ref count GC %d\n", pz->refcount__gc);

        }
        current = current->next;

    }
}

PHP_FUNCTION(meminfo_symbol_table)
{
    zval *zval_stream;
    php_stream *stream;
    HashTable main_symbol_table;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zval_stream) == FAILURE) {
        return;
    }

    php_stream_from_zval(stream, &zval_stream);

    main_symbol_table = EG(symbol_table);

    stream_printf(stream, "Nb elements in Symbol Table: %d\n",main_symbol_table.nNumOfElements);

    
}


