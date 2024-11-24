#ifndef _LITERAL_TABLE_H
#define _LITERAL_TABLE_H

#include <stdbool.h>

typedef float float_type;

// Initialize the literal table
void literal_table_initialize();

// Find the offset of a literal in the table; return -1 if not found
int literal_table_find_offset(const char *sought, float_type value);

// Return the size of the literal table
unsigned int literal_table_size();

// Check if a literal is present in the table
bool literal_table_present(const char *sought, float_type value);

// Lookup or add a literal to the table and return its offset
unsigned int literal_table_lookup(const char *text, float_type value);

// Start an iteration over the literal table
void literal_table_start_iteration();

// End the current iteration over the literal table
void literal_table_end_iteration();

// Get the next literal value during iteration
float_type literal_table_iteration_next();

#endif
