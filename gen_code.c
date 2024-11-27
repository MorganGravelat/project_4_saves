#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "bof.h"
#include "code_seq.h"
#include "code.h"
#include "literal_table.h"
#include "gen_code.h"
#include "ast.h"
#include "utilities.h"

#define V0 2
// Forward declarations for recursive AST traversal
static code_seq gen_code_block(block_t block);
static code_seq gen_code_const_decls(const_decls_t const_decls);
static code_seq gen_code_var_decls(var_decls_t var_decls);
static code_seq gen_code_proc_decls(proc_decls_t proc_decls);
static code_seq gen_code_stmts(stmts_t stmts);
static code_seq gen_code_stmt(stmt_t stmt);
static code_seq gen_code_expr(expr_t expr);
static code_seq gen_code_condition(condition_t condition);

// Forward declarations for code output
static void gen_code_output_program(BOFFILE bf, code_seq main_code);
static BOFHeader gen_code_program_header(code_seq main_code);

// Initialize the code generation module
void gen_code_initialize() {
    printf("Code generation module initialized.\n");
    literal_table_initialize();  // Initialize the literal table
}

// Generate VM code for the program AST
void gen_code_program(BOFFILE bf, block_t program) {
    assert(bf.fileptr != NULL);
    printf("gen_code_program started.\n");

    // Generate the code for the program block
    code_seq main_code = gen_code_block(program);

    // Append an EXIT instruction to terminate the program
    printf("Appending EXIT instruction.\n");
    code_seq_add_to_end(&main_code, code_exit(0));  // Corrected

    // Output the program to the BOFFILE
    gen_code_output_program(bf, main_code);

    printf("Finished generating code for block.\n");
}

// Generate code for a block
static code_seq gen_code_block(block_t block) {
    code_seq result = code_seq_empty();
    code_seq_concat(&result, gen_code_const_decls(block.const_decls));
    code_seq_concat(&result, gen_code_var_decls(block.var_decls));
    code_seq_concat(&result, gen_code_proc_decls(block.proc_decls));
    code_seq_concat(&result, gen_code_stmts(block.stmts));
    return result;
}

// Generate code for constant declarations
static code_seq gen_code_const_decls(const_decls_t const_decls) {
    code_seq result = code_seq_empty();
    const_decl_t *decl = const_decls.start;
    while (decl) {
        const_def_list_t def_list = decl->const_def_list;
        const_def_t *def = def_list.start;
        while (def) {
            unsigned int literal_index = literal_table_add(def->number.value);
            code *literal_code = code_lit(0, 0, literal_index);  // Load literal into $r0
            code_seq_add_to_end(&result, literal_code);
            printf("CONST %s %d (literal index: %u)\n",
                   def->ident.name, def->number.value, literal_index);
            def = def->next;
        }
        decl = decl->next;
    }
    return result;
}

// Generate code for variable declarations
static code_seq gen_code_var_decls(var_decls_t var_decls) {
    code_seq result = code_seq_empty();
    var_decl_t *decl = var_decls.var_decls;
    while (decl) {
        ident_t *ident = decl->ident_list.start;
        while (ident) {
            printf("VAR %s\n", ident->name);
            ident = ident->next;
        }
        decl = decl->next;
    }
    return result;
}

// Generate code for procedure declarations
static code_seq gen_code_proc_decls(proc_decls_t proc_decls) {
    code_seq result = code_seq_empty();
    proc_decl_t *decl = proc_decls.proc_decls;
    while (decl) {
        printf("PROC %s\n", decl->name);
        code_seq proc_block_code = gen_code_block(*(decl->block));
        code_seq_concat(&result, proc_block_code);
        printf("ENDPROC\n");
        decl = decl->next;
    }
    return result;
}

// Generate code for statements
static code_seq gen_code_stmts(stmts_t stmts) {
    code_seq result = code_seq_empty();
    if (stmts.stmts_kind == empty_stmts_e) {
        return result;
    }

    stmt_list_t stmt_list = stmts.stmt_list;
    stmt_t *stmt = stmt_list.start;
    while (stmt) {
        code_seq_concat(&result, gen_code_stmt(*stmt));
        stmt = stmt->next;
    }
    return result;
}

static code_seq gen_code_stmt(stmt_t stmt) {
    code_seq ret = code_seq_empty(); // Initialize ret

    switch (stmt.stmt_kind) {
        case assign_stmt:
            printf("Generating code for assignment statement.\n");
            code_seq expr_code = gen_code_expr(*(stmt.data.assign_stmt.expr));
            code_seq_concat(&ret, expr_code);
            break;

        case call_stmt:
            printf("Generating code for call statement.\n");
            // Add logic for function calls here if applicable
            break;

        case if_stmt: {
            printf("Generating code for if statement.\n");

            // Generate condition code
            code_seq condition_code = gen_code_condition(stmt.data.if_stmt.condition);
            code_seq_concat(&ret, condition_code);

            // Generate "then" code
            code_seq then_code = gen_code_stmts(*(stmt.data.if_stmt.then_stmts));
            int then_len = code_seq_size(then_code);

            // Generate "else" code if it exists
            code_seq else_code = code_seq_empty();
            int else_len = 0;
            if (stmt.data.if_stmt.else_stmts) {
                else_code = gen_code_stmts(*(stmt.data.if_stmt.else_stmts));
                else_len = code_seq_size(else_code);
            }

            // Add conditional jump to skip "then" if false
            code_seq_add_to_end(&ret, code_beq(V0, 0, then_len + (else_len > 0 ? 1 : 0)));

            // Add "then" code
            code_seq_concat(&ret, then_code);

            // Add unconditional jump to skip "else" after "then"
            if (else_len > 0) {
                code_seq_add_to_end(&ret, code_jrel(else_len));
            }

            // Add "else" code
            code_seq_concat(&ret, else_code);

            break;
        }

        case while_stmt:
            printf("Generating code for while statement.\n");
            // Add logic for while statements
            break;

        case read_stmt:
            printf("Generating code for read statement.\n");
            // Add logic for read statements
            break;

        case print_stmt:
            printf("Generating code for print statement.\n");
            unsigned int literal_index = literal_table_add(stmt.data.print_stmt.expr.data.number.value);
            code_seq_add_to_end(&ret, code_pint(0, literal_index));
            break;

        case block_stmt:
            printf("Generating code for block statement.\n");
            code_seq block_code = gen_code_block(*(stmt.data.block_stmt.block));
            code_seq_concat(&ret, block_code);
            break;

        default:
            bail_with_error("Unhandled statement kind in code generation!");
    }

    return ret;
}


// Generate code for an expression
static code_seq gen_code_expr(expr_t expr) {
    code_seq result = code_seq_empty();
    switch (expr.expr_kind) {
        case expr_bin:
            
        case expr_number:
            printf("Generating code for numeric expression.\n");
            unsigned int literal_index = literal_table_add(expr.data.number.value);
            code *load_instr = code_lit(0, 0, literal_index);  // Load literal into $r0
            code_seq_add_to_end(&result, load_instr);
            break;

        default:
            bail_with_error("Unknown expression kind in code generation!");
    }
    return result;
}
/*
%token <token> eqeqsym    "=="
%token <token> neqsym     "!="
%token <token> ltsym      "<"
%token <token> leqsym     "<="
%token <token> gtsym      ">"
%token <token> geqsym     ">="

*/
static code_seq gen_code_bin_expr(binary_op_expr_t bExpr) {
    switch (bExpr.arith_op.code) {
        case plussym: {
            code* c1 = code_seq_add_to_end
        }
        case minussym:
        case multsym:
        case divsym:
        case eqeqsym:    
        case neqsym:     
        case ltsym:   
        case leqsym:  
        case gtsym:   
        case geqsym:  

        default:
    }
}

// Generate code for a condition
static code_seq gen_code_condition(condition_t condition) {
    code_seq result = code_seq_empty();
    // Logic to handle conditions based on their kind
    // Example: generating code for relational or logical operations
    return result;
}

// Output the BOFFILE for the program
static void gen_code_output_program(BOFFILE bf, code_seq main_code) {
    BOFHeader header = gen_code_program_header(main_code);
    bof_write_header(bf, header);
    while (!code_seq_is_empty(main_code)) {
        instruction_write_bin_instr(bf, code_seq_first(main_code)->instr);
        main_code = code_seq_rest(main_code);
    }
    literal_table_finalize(bf);
    bof_close(bf);
}

// Create the BOF header
static BOFHeader gen_code_program_header(code_seq main_code) {
    BOFHeader header;
    memcpy(header.magic, "BO32", MAGIC_BUFFER_SIZE);
    header.text_start_address = 0;
    header.text_length = code_seq_size(main_code);
    header.data_start_address = 1024;
    header.data_length = literal_table_size();
    header.stack_bottom_addr = 2048;
    return header;
}
