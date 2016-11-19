#include <stdlib.h>
#include <stdint.h>

typedef struct _lookup_table {
  uint8_t               id;
  void                 *value;
  struct _lookup_table *next;
  struct _lookup_table *prev;
  struct _lookup_table *last;
} lookup_table;

lookup_table *uint_lookup_table_new(uint8_t, uint8_t);
lookup_table *uint_lookup_table_find(lookup_table *, uint8_t);
lookup_table *uint_lookup_table__add(lookup_table *, uint8_t, uint8_t);

