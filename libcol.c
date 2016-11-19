#include "libcol.h"

/*
 * Lookup Table Implementation
 */

lookup_table *_lookup_table_new(uint8_t, size_t);

lookup_table *
uint_lookup_table_new(uint8_t id, uint8_t value)
{
  lookup_table *_lookup_table = _lookup_table_new(id, sizeof(uint8_t));
  if (!_lookup_table)
    return 0;

  *((uint8_t *)(_lookup_table->value)) = value;
  return _lookup_table;
}

lookup_table *
uint_lookup_table_find(lookup_table *_lookup_table, uint8_t id)
{
  lookup_table *current = _lookup_table;
  while (current) {
    if (current->id == id)
      return current;

    current = current->next;
  }

  return 0;
}

lookup_table *
uint_lookup_table_add(lookup_table *_lookup_table, uint8_t id, uint8_t value)
{
  if (!_lookup_table)
    return 0;

  lookup_table *new_item = _lookup_table_new(id, sizeof(uint8_t));
  if (!new_item)
    return 0;

  *((uint8_t *)(new_item->value)) = value;
  new_item->prev = _lookup_table->last;
  _lookup_table->last->next = new_item;
  _lookup_table->last = new_item;

  return new_item;
}

inline
lookup_table *
_lookup_table_new(uint8_t id, size_t value_size)
{
  lookup_table *_lookup_table = (lookup_table *)malloc(sizeof(lookup_table));
  if (!_lookup_table)
    return 0;

  _lookup_table->value = malloc(value_size);
  if (!_lookup_table->value) {
    free(_lookup_table);
    return 0;
  }

  _lookup_table->next = 0;
  _lookup_table->prev = 0;
  _lookup_table->last = _lookup_table;

  return _lookup_table;
}
