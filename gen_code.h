#ifndef _GEN_CODE_H
#define _GEN_CODE_H

#include "ast.h"

// enum {
//     gp,sp,fp,r3,r4,r5,r6,ra;
// }
// Initialize the code generation module.
void gen_code_initialize();

// Generate VM code for the given program AST.
// This function is called after parsing and AST creation is complete.
void gen_code_program(BOFFILE bf, block_t program);

#endif
