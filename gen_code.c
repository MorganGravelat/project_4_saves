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

// File pointer for the output file
//static FILE *output_file;
#define HALT_OPCODE 0xFF

// Forward declarations for recursive AST traversal
static void gen_code_block(BOFFILE bf, block_t block);
static void gen_code_const_decls(BOFFILE bf, const_decls_t const_decls);
static void gen_code_var_decls(BOFFILE bf, var_decls_t var_decls);
static void gen_code_proc_decls(BOFFILE bf, proc_decls_t proc_decls);
static void gen_code_stmts(BOFFILE bf, stmts_t stmts);
static void gen_code_stmt(BOFFILE bf, stmt_t stmt);
static void gen_code_expr(BOFFILE bf, expr_t expr);
static void gen_code_condition(BOFFILE bf, condition_t condition);

// Initialize the code generation module
void gen_code_initialize() {
    printf("Code generation module initialized.\n");
}


// Generate VM code for the program AST
void gen_code_program(BOFFILE bf, block_t program) {
    assert(bf.fileptr != NULL);
    printf("gen_code_program started for BOFFILE: %s\n", bf.filename);  // Debug filename

    // Prepare the BOF header
    BOFHeader header;
    memcpy(header.magic, "BO32", MAGIC_BUFFER_SIZE);
    header.magic[MAGIC_BUFFER_SIZE] = '\0';  // Ensure null termination
    header.text_start_address = 0;              // Text starts at the beginning
    header.data_start_address = 1024;           // Example offset for data section
    header.stack_bottom_addr = 2048;            // Example stack bottom address
    header.text_length = 0;
    header.data_length = 0;

    // Write a placeholder BOF header to the file
    printf("Writing BOF header to: %s\n", bf.filename);  // Debug filename
    bof_write_header(bf, header);

    // Generate the code for the program block
    code_seq main_code = code_utils_set_up_program();

    gen_code_block(bf, program);

    code_seq end_code = code_utils_tear_down_program();
    code_seq_concat(main_code, end_code);
    header.text_length = code_seq_size(main_code);
    for (code *instr = code_seq_first(main_code); instr != NULL; instr = instr->next) {
        instruction_write_bin_instr(bf, instr->instr);
    }
    // Compute data length
    header.data_length = literal_table_size();  // Number of literals

    // Rewind to the beginning and write the updated header
    printf("Rewinding to write updated header for: %s\n", bf.filename);  // Debug filename
    fseek(bf.fileptr, 0, SEEK_SET);
    bof_write_header(bf, header);

    printf("Header written for BOFFILE: %s\n", bf.filename);  // Debug filename
    printf("Header: magic=%s, text_start=%d, text_length=%d, data_start=%d, data_length=%d, stack_bottom=%d\n",
        header.magic, header.text_start_address, header.text_length, header.data_start_address,
        header.data_length, header.stack_bottom_addr);

    bof_close(bf);
    printf("Finished generating code for block and closed file: %s\n", bf.filename);  // Debug filename
}

// Generate code for a block
static void gen_code_block(BOFFILE bf, block_t block) {
    gen_code_const_decls(bf, block.const_decls);
    printf("Processed constant declarations.\n");

    gen_code_var_decls(bf, block.var_decls);
    gen_code_proc_decls(bf, block.proc_decls);
    gen_code_stmts(bf, block.stmts);
}

// Generate code for constant declarations
static void gen_code_const_decls(BOFFILE bf, const_decls_t const_decls) {
    const_decl_t *decl = const_decls.start;
    printf("Processing constant declarations.\n");
    while (decl) {
        const_def_list_t def_list = decl->const_def_list;
        const_def_t *def = def_list.start;
        while (def) {
            bof_write_word(bf, def->number.value);  // Write the constant value
            printf("CONST %s %d\n", def->ident.name, def->number.value);
            def = def->next;
        }
        decl = decl->next;
    }
}

// Generate code for variable declarations
static void gen_code_var_decls(BOFFILE bf, var_decls_t var_decls) {
    var_decl_t *decl = var_decls.var_decls;
    printf("Processing variable declarations.\n");
    while (decl) {
        ident_t *ident = decl->ident_list.start;
        while (ident) {
            bof_write_word(bf, 0);  // Placeholder for variable initialization
            printf("VAR %s\n", ident->name);
            ident = ident->next;
        }
        decl = decl->next;
    }
}
// Generate code for procedure declarations
static void gen_code_proc_decls(BOFFILE bf, proc_decls_t proc_decls) {
    proc_decl_t *decl = proc_decls.proc_decls;
    printf("Processing procedure declarations.\n");
    while (decl) {
        printf("PROC %s\n", decl->name);
        gen_code_block(bf, *(decl->block));
        printf("ENDPROC\n");
        decl = decl->next;
    }
}

// Generate code for statements
static void gen_code_stmts(BOFFILE bf, stmts_t stmts) {
    if (stmts.stmts_kind == empty_stmts_e) {
        return;
    }
    stmt_list_t stmt_list = stmts.stmt_list;
    stmt_t *stmt = stmt_list.start;
    while (stmt) {
        gen_code_stmt(bf, *stmt);
        stmt = stmt->next;
    }
}


// Generate code for a single statement
static void gen_code_stmt(BOFFILE bf, stmt_t stmt) {
    switch (stmt.stmt_kind) {
        case assign_stmt:
            printf("Generating code for assignment statement.\n");
            gen_code_expr(bf, *(stmt.data.assign_stmt.expr));
            bof_write_word(bf, stmt.data.assign_stmt.name[0]);  // Simulated output
            break;
        case call_stmt:
            printf("Generating code for call statement.\n");
            bof_write_word(bf, stmt.data.call_stmt.name[0]);  // Simulated output
            break;
        case if_stmt:
            printf("Generating code for if statement.\n");
            gen_code_condition(bf, stmt.data.if_stmt.condition);
            gen_code_stmts(bf, *(stmt.data.if_stmt.then_stmts));
            if (stmt.data.if_stmt.else_stmts) {
                gen_code_stmts(bf, *(stmt.data.if_stmt.else_stmts));
            }
            break;
        case while_stmt:
            printf("Generating code for while statement.\n");
            gen_code_condition(bf, stmt.data.while_stmt.condition);
            gen_code_stmts(bf, *(stmt.data.while_stmt.body));
            break;
        case read_stmt:
            printf("Generating code for read statement.\n");
            bof_write_word(bf, stmt.data.read_stmt.name[0]);  // Simulated output
            break;
        case print_stmt:
            printf("Generating code for print statement.\n");
            gen_code_expr(bf, stmt.data.print_stmt.expr);
            bof_write_word(bf, 0);  // Placeholder for print instruction
            break;
        case block_stmt:
            printf("Generating code for block statement.\n");
            gen_code_block(bf, *(stmt.data.block_stmt.block));
            break;
        default:
            bail_with_error("Unknown statement kind in code generation!");
    }
}

// Generate code for an expression
static void gen_code_expr(BOFFILE bf, expr_t expr) {
    switch (expr.expr_kind) {
        case expr_bin:
            gen_code_expr(bf, *(expr.data.binary.expr1));
            gen_code_expr(bf, *(expr.data.binary.expr2));
            bof_write_word(bf, expr.data.binary.arith_op.code);  // Simulated binary op
            break;
        case expr_negated:
            gen_code_expr(bf, *(expr.data.negated.expr));
            bof_write_word(bf, -1);  // Simulated negation
            break;
        case expr_ident:
            bof_write_word(bf, expr.data.ident.name[0]);  // Simulated identifier load
            break;
        case expr_number:
            bof_write_word(bf, expr.data.number.value);  // Write numeric literal
            break;
        default:
            bail_with_error("Unknown expression kind in code generation!");
    }
}

// Generate code for a condition
static void gen_code_condition(BOFFILE bf, condition_t condition) {
    switch (condition.cond_kind) {
        case ck_db:
            gen_code_expr(bf, condition.data.db_cond.dividend);
            gen_code_expr(bf, condition.data.db_cond.divisor);
            bof_write_word(bf, 0);  // Simulated divisibility check
            break;
        case ck_rel:
            gen_code_expr(bf, condition.data.rel_op_cond.expr1);
            gen_code_expr(bf, condition.data.rel_op_cond.expr2);
            bof_write_word(bf, condition.data.rel_op_cond.rel_op.code);  // Simulated relational op
            break;
        default:
            bail_with_error("Unknown condition kind in code generation!");
    }
}
