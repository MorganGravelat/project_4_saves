#ifndef _GEN_CODE_H
#define _GEN_CODE_H

#include "ast.h"

// Initialize the code generation module.
void gen_code_initialize(const char *output_filename);

// Generate VM code for the given program AST.
// This function is called after parsing and AST creation is complete.
void gen_code_program(block_t program);

#endif
