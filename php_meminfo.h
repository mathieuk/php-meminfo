#ifndef PHP_MEMINFO_H
#define PHP_MEMINFO_H 1

#define MEMINFO_NAME "PHP Meminfo"
#define MEMINFO_VERSION "0.2"
#define MEMINFO_AUTHOR "Benoit Jacquemont"
#define MEMINFO_COPYRIGHT  "Copyright (c) 2010-2015 by Benoit Jacquemont"
#define MEMINFO_COPYRIGHT_SHORT "Copyright (c) 2011-2015"

PHP_FUNCTION(meminfo_structs_size);
PHP_FUNCTION(meminfo_objects_list);
PHP_FUNCTION(meminfo_objects_summary);
PHP_FUNCTION(meminfo_gc_roots_list);
PHP_FUNCTION(meminfo_symbol_table);
PHP_FUNCTION(meminfo_dependency_list);
PHP_FUNCTION(meminfo_dependency_list_summary);

char* get_type_label(zval* z);
const char *get_classname(zend_uint handle);
static void browse_hash(php_stream * stream, const char * zval_label, HashTable *ht, zend_bool is_object, HashTable *visited_items TSRMLS_DC);
void browse_zvals(php_stream * stream, zval * zv, HashTable *visited_items);
int visit_item(const char * item_label, HashTable * visited_items);
void inc_links_summary(const char * origin_class_name, const char * target_class_name, HashTable *links_summary);
void browse_zvals_summary(zval * zv, HashTable *visited_items, HashTable *links_summary, const char* origin_class_name);
static void browse_hash_summary(HashTable *ht, const char * class_name, HashTable *links_summary, HashTable *visited_items TSRMLS_DC);


static int instances_count_compare(const void *a, const void *b TSRMLS_DC);

extern zend_module_entry meminfo_entry;
#define phpext_meminfo_ptr &meminfo_module_entry

#endif
