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
    PHP_FE(meminfo_objects_summary, NULL)
    PHP_FE(meminfo_gc_roots_list, NULL)
    PHP_FE(meminfo_symbol_table, NULL)
    PHP_FE(meminfo_dependency_list, NULL)
    PHP_FE(meminfo_dependency_list_summary, NULL)
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


PHP_FUNCTION(meminfo_structs_size)
{
    zval *zval_stream;
    php_stream *stream;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zval_stream) == FAILURE) {
        return;
    }

    php_stream_from_zval(stream, &zval_stream);

    php_stream_printf(stream, "Structs size on this platform:\n");
    php_stream_printf(stream, "  Class (zend_class_entry): %ld bytes.\n", sizeof(zend_class_entry));
    php_stream_printf(stream, "  Object (zend_object): %ld bytes.\n", sizeof(zend_object));
    php_stream_printf(stream, "  Variable (zval): %ld bytes.\n", sizeof(zval));
    php_stream_printf(stream, "  Variable value (zvalue_value): %ld bytes.\n", sizeof(zvalue_value));
}

PHP_FUNCTION(meminfo_objects_list)
{
    zval *zval_stream;
    php_stream *stream;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zval_stream) == FAILURE) {
        return;
    }

    php_stream_from_zval(stream, &zval_stream);

    php_stream_printf(stream, "Objects list:\n");

// TODO: check if object_buckets exists ? See gc_collect_roots from zend_gc.c
    zend_objects_store *objects = &EG(objects_store);
    zend_uint i;
    zend_uint total_objects_buckets = objects->top - 1;
    zend_uint current_objects = 0;
    zend_object * object;
    zend_class_entry * class_entry;

    for (i = 1; i < objects->top ; i++) {
        if (objects->object_buckets[i].valid) {
            struct _store_object *obj = &objects->object_buckets[i].bucket.obj;

            php_stream_printf(stream, "  - Class %s, handle %d, refCount %d\n", get_classname(i), i, obj->refcount);

            current_objects++;
        }
     }

    php_stream_printf(stream, "Total object buckets: %d. Current objects: %d.\n", total_objects_buckets, current_objects);
}

PHP_FUNCTION(meminfo_objects_summary)
{
    zval *zval_stream = NULL;
    php_stream *stream = NULL;
    HashTable *classes = NULL;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zval_stream) == FAILURE) {
        return;
    }

    ALLOC_HASHTABLE(classes);

    zend_hash_init(classes, 1000, NULL, NULL, 0);

    zend_objects_store *objects = &EG(objects_store);
    zend_uint i;
    zend_object *object;

    for (i = 1; i < objects->top ; i++) {
        if (objects->object_buckets[i].valid && !objects->object_buckets[i].destructor_called) {
            const char *class_name;
            zval **zv_dest;

            class_name = get_classname(i);

            if (zend_hash_find(classes, class_name, strlen(class_name)+1, (void **) &zv_dest) == SUCCESS) {
                Z_LVAL_PP(zv_dest) = Z_LVAL_PP(zv_dest) ++;
            } else {
                zval *zv_instances_count;
                MAKE_STD_ZVAL(zv_instances_count);
                ZVAL_LONG(zv_instances_count, 1);

                zend_hash_update(classes, class_name, strlen(class_name)+1, &zv_instances_count, sizeof(zval *), NULL);
            }
        }
    }

    zend_hash_sort(classes, zend_qsort, instances_count_compare, 0 TSRMLS_CC);

    php_stream_from_zval(stream, &zval_stream);
    php_stream_printf(stream, "Instances count by class:\n");

    php_stream_printf(stream, "%-12s %-12s %s\n", "num", "#instances", "class");
    php_stream_printf(stream, "-----------------------------------------------------------------\n");

    zend_uint num = 1;

    HashPosition position;
    zval **entry = NULL;

    for (zend_hash_internal_pointer_reset_ex(classes, &position);
         zend_hash_get_current_data_ex(classes, (void **) &entry, &position) == SUCCESS;
         zend_hash_move_forward_ex(classes, &position)) {

        char *class_name = NULL;
        uint  class_name_len;
        ulong index;

        zend_hash_get_current_key_ex(classes, &class_name, &class_name_len, &index, 0, &position);
        php_stream_printf(stream, "%-12d %-12d %s\n", num, Z_LVAL_PP(entry), class_name);

        num++;
    }

    zend_hash_destroy(classes);
    FREE_HASHTABLE(classes);
}

static int instances_count_compare(const void *a, const void *b TSRMLS_DC)
{
    const Bucket *first_bucket;
    const Bucket *second_bucket;

    first_bucket = *((const Bucket **) a);
    second_bucket = *((const Bucket **) b);

    zval *zv_first;
    zval *zv_second;

    zv_first = (zval *) first_bucket->pDataPtr;
    zv_second = (zval *) second_bucket->pDataPtr;


    zend_uint first;
    zend_uint second;

    first = Z_LVAL_P(zv_first);
    second = Z_LVAL_P(zv_second);

    if (first > second) {
        return -1;
    } else if (first == second) {
        return 0;
    } else {
        return 1;
    }
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
    php_stream_printf(stream, "GC roots list:\n");

    gc_root_buffer *current = GC_G(roots).next;

    while (current != &GC_G(roots)) {
        pz = current->u.pz;
        php_stream_printf( stream, "  zval pointer: %p ", (void *) pz);
        if (current->handle) {
            php_stream_printf(
                stream,
                "  Class %s, handle %d\n",
                get_classname(current->handle),
                current->handle);
        } else {
            php_stream_printf(stream, "  Type: %s", get_type_label(pz));
            php_stream_printf(stream, ", Ref count GC %d\n", pz->refcount__gc);

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

    php_stream_printf(stream, "Nb elements in Symbol Table: %d\n",main_symbol_table.nNumOfElements);
}

PHP_FUNCTION(meminfo_dependency_list)
{
    zval *zv;
    zval *zval_stream;
    php_stream *stream;
    HashTable *visited_items;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rz", &zval_stream, &zv) == FAILURE) {
        return;
    }

    php_stream_from_zval(stream, &zval_stream);

    ALLOC_HASHTABLE(visited_items);
    zend_hash_init(visited_items, 1000, NULL, NULL, 0);

    browse_zvals(stream, zv, visited_items);

    zend_hash_destroy(visited_items);
    FREE_HASHTABLE(visited_items);
}

/**
 * Generates a summary of links between classes.
 *
 * Internal dependency Format (the number is the link occurences number):
 * [
 *     "ClassA" : [
 *         "ClassB" : 5,
 *         "ClassC" : 2
 *     ]
 *     "ClassB" : [
 *         "ClassC" : 6,
 *     ]
 * ]
 *
 * Note that the summary represents only Object to Object link. In case of
 * indirect links between objects via array, the arrays are squashed and only
 * the link is shown.
 *
 * @param Zval zv        Root of the dependency tree
 * @param Zval zv_stream Stream to write summary to
 */
PHP_FUNCTION(meminfo_dependency_list_summary)
{
    zval *zv;
    zval *zval_stream;

    php_stream *stream;

    HashTable *visited_items;
    HashTable *links_summary;

    HashPosition summary_pos;
    HashPosition target_pos;
    HashTable **target_links;
    int **count;
    ulong summary_index;
    ulong target_index;
    char *origin_class_name;
    char *target_class_name;
    uint origin_class_name_length;
    uint target_class_name_length;
    char *rootName = "_ROOT_";

    char from[201];
    char to[201];

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rz", &zval_stream, &zv) == FAILURE) {
        return;
    }

    php_stream_from_zval(stream, &zval_stream);

    ALLOC_HASHTABLE(visited_items);
    zend_hash_init(visited_items, 1000, NULL, NULL, 0);

    ALLOC_HASHTABLE(links_summary);
    zend_hash_init(links_summary, 1000, NULL, NULL, 0);

    browse_zvals_summary(zv, visited_items, links_summary, rootName);

    zend_hash_destroy(visited_items);
    FREE_HASHTABLE(visited_items);

    zend_hash_internal_pointer_reset_ex(links_summary, &summary_pos);

    while (zend_hash_get_current_data_ex(links_summary, (void **) &target_links, &summary_pos) == SUCCESS) {
        zend_hash_get_current_key_ex(links_summary, &origin_class_name, &origin_class_name_length, &summary_index, 0, &summary_pos);

        zend_hash_internal_pointer_reset_ex(*target_links, &target_pos);
        while (zend_hash_get_current_data_ex(*target_links, (void **) &count, &target_pos) == SUCCESS) {
            zend_hash_get_current_key_ex(*target_links, &target_class_name, &target_class_name_length, &target_index, 0, &target_pos);

            strncpy(from, origin_class_name, origin_class_name_length);
            strncpy(to, target_class_name, target_class_name_length);

            from[origin_class_name_length] = 0;
            to[target_class_name_length] = 0;

            php_stream_printf(stream, "%s,%d,%s\n", from, *count, to);

            zend_hash_move_forward_ex(*target_links, &target_pos);
        }
        zend_hash_destroy(*target_links);
        FREE_HASHTABLE(*target_links);

        zend_hash_move_forward_ex(links_summary, &summary_pos);
    }

    zend_hash_destroy(links_summary);
    FREE_HASHTABLE(links_summary);
}

int visit_item(const char * item_label, HashTable *visited_items)
{
    int found = 0;
    int isset = 1;


    if (zend_hash_exists(visited_items, item_label, strlen(item_label))) {
        found = 1;
    } else {
        zend_hash_update(visited_items, item_label, strlen(item_label), &isset, sizeof(int), NULL);
    }

    return found;
}

void inc_links_summary(const char * origin_class_name, const char * target_class_name, HashTable *links_summary)
{
    int found = 0;
    int inc_start = 1;

    HashTable **target_links;

    if (!zend_hash_exists(links_summary, origin_class_name, strlen(origin_class_name))) {
        HashTable * new_target_links;

        ALLOC_HASHTABLE(new_target_links);
        zend_hash_init(new_target_links, 100, NULL, NULL, 0);

        zend_hash_update(links_summary, origin_class_name, strlen(origin_class_name), &new_target_links, sizeof(HashTable *), NULL);
    }

    zend_hash_find(links_summary, origin_class_name, strlen(origin_class_name), (void **) &target_links);

    if (zend_hash_exists(*target_links, target_class_name, strlen(target_class_name))) {
        int* count;

        zend_hash_find(*target_links, target_class_name, strlen(target_class_name), (void **) &count);

        (*count)++;
    } else {
        zend_hash_update(*target_links, target_class_name, strlen(target_class_name), &inc_start, sizeof(int), NULL);
    }
}

static void browse_hash(php_stream * stream, const char *zval_label, HashTable *ht, zend_bool is_object, HashTable *visited_items TSRMLS_DC)
{
    zval **tmp;
    char *string_key;
    HashPosition pos;
    ulong num_key;
    uint str_len;
    int i;

    zend_hash_internal_pointer_reset_ex(ht, &pos);

    while (zend_hash_get_current_data_ex(ht, (void **) &tmp, &pos) == SUCCESS) {
        switch (zend_hash_get_current_key_ex(ht, &string_key, &str_len, &num_key, 0, &pos)) {
            case HASH_KEY_IS_STRING:
                if (is_object) {
                    const char *prop_name, *class_name;
                    int mangled = zend_unmangle_property_name(string_key, str_len - 1, &class_name, &prop_name);

                    php_stream_printf(stream, "%s,%s,", zval_label, prop_name);
                } else {
                    php_stream_printf(stream, "%s,%s,", zval_label, string_key);
                }
                break;
            case HASH_KEY_IS_LONG:
                {
                    char key[25];
                    snprintf(key, sizeof(key), "%ld", num_key);
                    php_stream_printf(stream, "%s,%s,", zval_label, key);
                }
                break;
        }
        browse_zvals(stream, *tmp, visited_items);
        zend_hash_move_forward_ex(ht, &pos);
    }
}

void browse_zvals(php_stream * stream, zval * zv, HashTable *visited_items)
{
    char zval_label[500];

    switch (Z_TYPE_P(zv)) {
        case IS_ARRAY:
            snprintf(zval_label, 100, "Array (%p)", Z_ARRVAL_P(zv));
            php_stream_printf(stream, "%s\n", zval_label);
            if (!visit_item(zval_label, visited_items)) {
                browse_hash(stream, zval_label, Z_ARRVAL_P(zv), 0, visited_items);
            }
            break;
        case IS_OBJECT:
            {
                HashTable *properties;
                const char *class_name = NULL;
                zend_uint clen;
                int is_temp;


                if (Z_OBJ_HANDLER_P(zv, get_class_name)) {
                    Z_OBJ_HANDLER_P(zv, get_class_name)(zv, &class_name, &clen, 0 TSRMLS_CC);
                }
                if (class_name) {
                    snprintf(zval_label, 499, "Object %s (#%d)", class_name, zv->value.obj.handle);
                    efree((char*)class_name);
                } else {
                    snprintf(zval_label, 499, "Object *unknown class* (#%d)", zv->value.obj.handle);
                }
                php_stream_printf(stream, "%s\n", zval_label);

                if ((properties = Z_OBJDEBUG_P(zv, is_temp)) == NULL) {
                    break;
                }
                if (!visit_item(zval_label, visited_items)) {
                    browse_hash(stream, zval_label, properties, 1, visited_items);
                }
                if (is_temp) {
                    zend_hash_destroy(properties);
                    efree(properties);
                }
                break;
            }
        default:
            php_stream_printf(stream, "%s (%p)\n", get_type_label(zv), zv);
            break;
        }
}

void browse_zvals_summary(zval * zv, HashTable *visited_items, HashTable *links_summary, const char *origin_class_name)
{
    char class_name[501];
    char item_key[501];

    switch (Z_TYPE_P(zv)) {
        case IS_ARRAY:

            snprintf(item_key, 501, "A-%p", Z_ARRVAL_P(zv));

            if (!visit_item(item_key, visited_items)) {
                browse_hash_summary(Z_ARRVAL_P(zv), origin_class_name, links_summary, visited_items);
            }
            break;
        case IS_OBJECT:
            {
                HashTable *properties;
                const char *class_name = NULL;
                zend_uint clen;
                int is_temp;

                if (Z_OBJ_HANDLER_P(zv, get_class_name)) {
                    Z_OBJ_HANDLER_P(zv, get_class_name)(zv, &class_name, &clen, 0 TSRMLS_CC);
                }

                if (!class_name) {
                    class_name = "*unknow class*";
                }

                inc_links_summary(origin_class_name, class_name, links_summary);

                if ((properties = Z_OBJDEBUG_P(zv, is_temp)) == NULL) {
                    break;
                }

                snprintf(item_key, 501, "O-%d",  zv->value.obj.handle);

                if (!visit_item(item_key, visited_items)) {
                    browse_hash_summary(properties, class_name, links_summary, visited_items);
                }
                if (is_temp) {
                    zend_hash_destroy(properties);
                    efree(properties);
                }

                if (class_name) {
                    efree((char*)class_name);
                }
                break;
            }
    }
}

static void browse_hash_summary(HashTable *ht, const char *class_name, HashTable *links_summary, HashTable *visited_items TSRMLS_DC)
{
    zval **zval;
    char *string_key;
    HashPosition pos;
    ulong num_key;
    uint str_len;
    int i;


    zend_hash_internal_pointer_reset_ex(ht, &pos);

    while (zend_hash_get_current_data_ex(ht, (void **) &zval, &pos) == SUCCESS) {
        browse_zvals_summary(*zval, visited_items, links_summary, class_name);
        zend_hash_move_forward_ex(ht, &pos);
    }
}

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
