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

// Forward declarations for recursive AST traversal
static code_seq gen_code_block(block_t block);
static code_seq gen_code_const_decls(const_decls_t const_decls);
static code_seq gen_code_var_decls(var_decls_t var_decls);
static code_seq gen_code_proc_decls(proc_decls_t proc_decls);
static code_seq gen_code_stmts(stmts_t stmts);
static code_seq gen_code_stmt(stmt_t stmt);
static code_seq gen_code_expr(expr_t expr);
static code_seq gen_code_condition(condition_t condition);

// Initialize the code generation module
void gen_code_initialize() {
    printf("Code generation module initialized.\n");
    literal_table_initialize();  // Initialize the literal table
}

// Generate VM code for the program AST
void gen_code_program(BOFFILE bf, block_t program) {
    assert(bf.fileptr != NULL);
    printf("gen_code_program started.\n");

    // Prepare the BOF header
    BOFHeader header;
    memcpy(header.magic, "BO32", MAGIC_BUFFER_SIZE);  // Ensure magic number is correct
    header.text_start_address = 0;                    // Text starts at the beginning
    header.data_start_address = 1024;                 // Example offset for data section
    header.stack_bottom_addr = 2048;                  // Example stack bottom address
    header.text_length = 0;
    header.data_length = 0;

    // Write a placeholder BOF header to the file
    printf("Writing BOF header.\n");
    bof_write_header(bf, header);

    // Generate the code for the program block
    code_seq main_code = gen_code_block(program);
    header.text_length = code_seq_size(main_code);  // Account for all instructions written NEW
    //add_i =
    if (code_seq_is_empty(main_code)) {
        printf("No code generated for the program block. Adding NOP.\n");
        code *nop_instr = code_nop();
        instruction_write_bin_instr(bf, nop_instr->instr);
        header.text_length = 1;  // Include the NOP instruction
    } else {
        printf("Writing generated instructions to BOF file.\n");
        for (code *instr = code_seq_first(main_code); instr != NULL; instr = instr->next) {
            instruction_write_bin_instr(bf, instr->instr);
        }
        //header.text_length = code_seq_size(main_code);  // Account for all instructions written
    }

    // Write all instructions in the code sequence to the BOFFILE
    // printf("Writing generated instructions to BOF file.\n");
    // for (code *instr = code_seq_first(main_code); instr != NULL; instr = instr->next) {
    //     instruction_write_bin_instr(bf, instr->instr);
    // }

    // Append an EXIT instruction to terminate the program
    printf("Appending EXIT instruction.\n");
    code *exit_instr = code_exit(0);  // Create an EXIT instruction
    instruction_write_bin_instr(bf, exit_instr->instr);
    header.text_length++;  // Include EXIT instruction

    // Finalize the literal table by writing all literals to the BOFFILE
    literal_table_finalize(bf);

    // Update header with data section size
    header.data_length = literal_table_size();

    // Rewind to the beginning and write the updated header
    fseek(bf.fileptr, 0, SEEK_SET);
    bof_write_header(bf, header);

    printf("Header: magic=%s, text_start=%d, text_length=%d, data_start=%d, data_length=%d, stack_bottom=%d\n",
        header.magic, header.text_start_address, header.text_length, header.data_start_address,
        header.data_length, header.stack_bottom_addr);

    bof_close(bf);
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
        const_def_t *def = def_list.start; // Declare `def` here
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

// Generate code for a single statement
static code_seq gen_code_stmt(stmt_t stmt) {
    code_seq result = code_seq_empty();
    switch (stmt.stmt_kind) {
        case assign_stmt:
            printf("Generating code for assignment statement.\n");
            code_seq expr_code = gen_code_expr(*(stmt.data.assign_stmt.expr));
            code_seq_concat(&result, expr_code);
            break;

        case call_stmt:
            printf("Generating code for call statement.\n");
            // Add logic for function calls
            break;

        case if_stmt:
            printf("Generating code for if statement.\n");
            code_seq condition_code = gen_code_condition(stmt.data.if_stmt.condition);
            code_seq_concat(&result, condition_code);
            code_seq then_code = gen_code_stmts(*(stmt.data.if_stmt.then_stmts));
            code_seq_concat(&result, then_code);
            if (stmt.data.if_stmt.else_stmts) {
                code_seq else_code = gen_code_stmts(*(stmt.data.if_stmt.else_stmts));
                code_seq_concat(&result, else_code);
            }
            break;

        case while_stmt:
            printf("Generating code for while statement.\n");
            code_seq while_condition = gen_code_condition(stmt.data.while_stmt.condition);
            code_seq_concat(&result, while_condition);
            code_seq while_body = gen_code_stmts(*(stmt.data.while_stmt.body));
            code_seq_concat(&result, while_body);
            break;

        case read_stmt:
            printf("Generating code for read statement.\n");
            // Add logic for read operations
            break;

        case print_stmt:
            printf("Generating code for print statement.\n");
            unsigned int literal_index = literal_table_add(stmt.data.print_stmt.expr.data.number.value);
            //code *load_instr = code_lit(0, 0, literal_index);
            //code_seq_add_to_end(&result, load_instr);
            code *print_instr = code_pint(0, literal_index);
            code_seq_add_to_end(&result, print_instr);
            break;

        case block_stmt:
            printf("Generating code for block statement.\n");
            code_seq block_code = gen_code_block(*(stmt.data.block_stmt.block));
            code_seq_concat(&result, block_code);
            break;

        default:
            bail_with_error("Unhandled statement kind in code generation!");
    }
    return result;
}


// Generate code for an expression
static code_seq gen_code_expr(expr_t expr) {
    code_seq result = code_seq_empty();
    switch (expr.expr_kind) {
        case expr_number:
            printf("Generating code for numeric expression.\n");
            unsigned int literal_index = literal_table_add(expr.data.number.value);
            code *load_instr = code_lit(0, 0, literal_index);
            code_seq_add_to_end(&result, load_instr);
            break;

        default:
            bail_with_error("Unknown expression kind in code generation!");
    }
    return result;
}

// Generate code for a condition
static code_seq gen_code_condition(condition_t condition) {
    code_seq result = code_seq_empty();
    // Add logic to handle conditions
    return result;
}
