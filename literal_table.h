#ifndef _LITERAL_TABLE_H
#define _LITERAL_TABLE_H

#include "bof.h"

// Define `float_type` as float for consistency
typedef float float_type;

void literal_table_initialize();
unsigned int literal_table_add(float_type value);
unsigned int literal_table_size();
void literal_table_finalize(BOFFILE bf);
void literal_table_start_iteration();
void literal_table_end_iteration();
float_type literal_table_iteration_next();

#endif
