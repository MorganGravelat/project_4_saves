#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "literal_table.h"
#include "utilities.h"

typedef float float_type;  // Replace with `double` if needed

typedef struct literal_table_entry_s {
    struct literal_table_entry_s *next;
    const char *text;
    float_type value;
    unsigned int offset;
} literal_table_entry_t;

static literal_table_entry_t *literal_table_head = NULL;
static unsigned int literal_offset = 0;
static bool iterating = false;
static literal_table_entry_t *iteration_next = NULL;

static void literal_table_okay();
static bool literal_table_empty();

void literal_table_initialize() {
    literal_table_head = NULL;
    literal_offset = 0;
    literal_table_okay();
    iterating = false;
    iteration_next = NULL;
    literal_table_okay();
}

int literal_table_find_offset(const char *sought, float_type value) {
    literal_table_okay();
    literal_table_entry_t *entry = literal_table_head;
    while (entry != NULL) {
        if (strcmp(entry->text, sought) == 0) {
            return entry->offset;
        }
        entry = entry->next;
    }
    return -1;
}

static void literal_table_okay() {
    bool emp = literal_table_empty();
    assert(emp == (literal_offset == 0));
    assert(emp == (literal_table_head == NULL));
}

unsigned int literal_table_size() {
    return literal_offset;
}

static bool literal_table_empty() {
    return literal_offset == 0;
}

bool literal_table_present(const char *sought, float_type value) {
    literal_table_okay();
    return literal_table_find_offset(sought, value) >= 0;
}

unsigned int literal_table_lookup(const char *text, float_type value) {
    int ret = literal_table_find_offset(text, value);
    printf("literal_table_lookup: text=%s, value=%f, found=%d\n", text, value, ret); fflush(stdout);
    if (ret >= 0) {
        return ret;
    }

    literal_table_entry_t *new_entry = (literal_table_entry_t*)malloc(sizeof(literal_table_entry_t));
    if (new_entry == NULL) {
        bail_with_error("No space to allocate new literal table entry!");
    }
    new_entry->text = text;
    new_entry->value = value;
    new_entry->offset = literal_offset++;
    new_entry->next = literal_table_head;

    literal_table_head = new_entry;
    return new_entry->offset;
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
    printf("Iteration next value: %f\n", ret); fflush(stdout);
    iteration_next = iteration_next->next;
    return ret;
}
