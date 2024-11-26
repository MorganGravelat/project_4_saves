#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "literal_table.h"
#include "utilities.h"

typedef float float_type;  // Replace with `double` if needed

typedef struct literal_table_entry_s {
    struct literal_table_entry_s *next;
    float_type value;
    unsigned int offset;
} literal_table_entry_t;

static literal_table_entry_t *literal_table_head = NULL;
static literal_table_entry_t *literal_table_tail = NULL;
static unsigned int literal_offset = 0;
static bool iterating = false;
static literal_table_entry_t *iteration_next = NULL;

static void literal_table_okay();
static bool literal_table_empty();

void literal_table_initialize() {
    literal_table_head = NULL;
    literal_offset = 0;
    iterating = false;
    iteration_next = NULL;
    literal_table_okay();
}
//Keep a pointer to the tail
unsigned int literal_table_add(float_type value) {
    literal_table_okay();

    // Check if the literal is already in the table
    literal_table_entry_t *entry = literal_table_head;
    while (entry != NULL) {
        if (entry->value == value) {
            return entry->offset;  // Return existing offset
        }
        entry = entry->next;
    }

    // Add new literal to the table
    literal_table_entry_t *new_entry = (literal_table_entry_t *)malloc(sizeof(literal_table_entry_t)); //problematic
    if (new_entry == NULL) {
        bail_with_error("No space to allocate new literal table entry!");
    }

    new_entry->value = value;
    new_entry->offset = literal_offset++;
    new_entry->next = NULL;
    if (!literal_table_head) {
        literal_table_head = new_entry;
        literal_table_tail = new_entry;
    } else {
        literal_table_tail->next = new_entry;
    }
    literal_table_tail = new_entry;

    return new_entry->offset;  // Return new offset
}

void literal_table_finalize(BOFFILE bf) {
    printf("Finalizing literal table.\n");
    literal_table_start_iteration();
    while (!literal_table_empty() && iteration_next != NULL) {
        int value = literal_table_iteration_next();
        bof_write_word(bf, *(word_type *)&value);  // Write value to BOFFILE
    }
    literal_table_end_iteration();
    printf("Literal table finalized.\n");
}

unsigned int literal_table_size() {
    return literal_offset;
}

static bool literal_table_empty() {
    return literal_offset == 0;
}

void literal_table_start_iteration() {
    if (iterating) {
        bail_with_error("Attempt to start literal_table iterating when already iterating!");
    }
    literal_table_okay();
    iterating = true;
    iteration_next = literal_table_head;
}

void literal_table_end_iteration() {
    iterating = false;
}

float_type literal_table_iteration_next() {
    assert(iteration_next != NULL);
    float_type ret = iteration_next->value;
    iteration_next = iteration_next->next;
    return ret;
}

static void literal_table_okay() {
    bool emp = literal_table_empty();
    assert(emp == (literal_offset == 0));
    assert(emp == (literal_table_head == NULL));
}
