#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "bof.h"
#include "gen_code.h"
#include "ast.h"
#include "utilities.h"

// File pointer for the output file
static FILE *output_file;

// Forward declarations for recursive AST traversal
static void gen_code_block(block_t block);
static void gen_code_const_decls(const_decls_t const_decls);
static void gen_code_var_decls(var_decls_t var_decls);
static void gen_code_proc_decls(proc_decls_t proc_decls);
static void gen_code_stmts(stmts_t stmts);
static void gen_code_stmt(stmt_t stmt);
static void gen_code_expr(expr_t expr);
static void gen_code_condition(condition_t condition);

// Initialize the code generation module
void gen_code_initialize(const char *output_filename) {
    printf("Opening output file: %s\n", output_filename); fflush(stdout);

    if (output_filename == NULL || strlen(output_filename) == 0) {
        bail_with_error("Output filename is null or empty.");
    }

    // Correct usage: output_filename is the file name, and "wb" is the mode
    output_file = fopen(output_filename, "wb");
    if (!output_file) {
        bail_with_error("Cannot open output file: %s", output_filename);
    }
}

// Generate VM code for the program AST
void gen_code_program(block_t program) {
    assert(output_file != NULL);
    printf("gen_code_program started.\n"); fflush(stdout);
    BOFHeader header;
    memcpy(header.magic, "BOF\n", MAGIC_BUFFER_SIZE);
    header.text_start_address = 0;
    header.text_length = 0;
    header.data_start_address = 0;
    header.data_length = 0;
    header.stack_bottom_addr = 1024;
    // Emit the BOF header
    fprintf(output_file, "BOF\n");
    printf("Writing BOF header.\n"); fflush(stdout);
    fwrite(&header, sizeof(BOFHeader), 1, output_file);

    gen_code_block(program);
    printf("Finished generating code for block.\n"); fflush(stdout);
    fprintf(output_file, "EOF\n");

    printf("Header details: magic=%s, text_start=%d, text_length=%d, data_start=%d, data_length=%d, stack_bottom=%d\n",
    header.magic, header.text_start_address, header.text_length, header.data_start_address, header.data_length, header.stack_bottom_addr); fflush(stdout);
    fseek(output_file,0,SEEK_SET);
    fwrite(&header, sizeof(BOFHeader), 1, output_file);
    fclose(output_file);
}

// Generate code for a block
static void gen_code_block(block_t block) {
    gen_code_const_decls(block.const_decls);
    printf("Processed constant declarations.\n"); fflush(stdout);
    gen_code_var_decls(block.var_decls);
    gen_code_proc_decls(block.proc_decls);
    gen_code_stmts(block.stmts);
}

// Generate code for constant declarations
static void gen_code_const_decls(const_decls_t const_decls) {
    const_decl_t *decl = const_decls.start;
    printf("Processing constant declaration.\n"); fflush(stdout);
    while (decl) {
        const_def_list_t def_list = decl->const_def_list;
        const_def_t *def = def_list.start;
        while (def) {
            fprintf(output_file, "CONST %s %d\n", def->ident.name, def->number.value);
            def = def->next;
        }
        decl = decl->next;
    }
}

// Generate code for variable declarations
static void gen_code_var_decls(var_decls_t var_decls) {
    var_decl_t *decl = var_decls.var_decls;
    printf("Processing variable declaration.\n"); fflush(stdout);
    while (decl) {
        ident_t *ident = decl->ident_list.start;
        while (ident) {
            fprintf(output_file, "VAR %s\n", ident->name);
            ident = ident->next;
        }
        decl = decl->next;
    }
}

// Generate code for procedure declarations
static void gen_code_proc_decls(proc_decls_t proc_decls) {
    proc_decl_t *decl = proc_decls.proc_decls;
    printf("Processing procedure declaration.\n"); fflush(stdout);
    while (decl) {
        fprintf(output_file, "PROC %s\n", decl->name);
        gen_code_block(*(decl->block));
        fprintf(output_file, "ENDPROC\n");
        decl = decl->next;
    }
}

// Generate code for statements
static void gen_code_stmts(stmts_t stmts) {
    printf("Processing gen_code_stmts.\n");
    if (stmts.stmts_kind == empty_stmts_e) {
        return;
    }
    stmt_list_t stmt_list = stmts.stmt_list;
    stmt_t *stmt = stmt_list.start;
    printf("Processing gen_code_stmts2.\n");
    while (stmt) {
        gen_code_stmt(*stmt);
        stmt = stmt->next;
    }
}

// Generate code for a single statement
static void gen_code_stmt(stmt_t stmt) {
    switch (stmt.stmt_kind) {
        case assign_stmt:
            fprintf(output_file, "LOAD %s\n", stmt.data.assign_stmt.name);
            gen_code_expr(*(stmt.data.assign_stmt.expr));
            fprintf(output_file, "STORE %s\n", stmt.data.assign_stmt.name);
            break;
        case call_stmt:
            fprintf(output_file, "CALL %s\n", stmt.data.call_stmt.name);
            break;
        case if_stmt:
            gen_code_condition(stmt.data.if_stmt.condition);
            fprintf(output_file, "IF\n");
            gen_code_stmts(*(stmt.data.if_stmt.then_stmts));
            if (stmt.data.if_stmt.else_stmts) {
                fprintf(output_file, "ELSE\n");
                gen_code_stmts(*(stmt.data.if_stmt.else_stmts));
            }
            fprintf(output_file, "ENDIF\n");
            break;
        case while_stmt:
            gen_code_condition(stmt.data.while_stmt.condition);
            fprintf(output_file, "WHILE\n");
            gen_code_stmts(*(stmt.data.while_stmt.body));
            fprintf(output_file, "ENDWHILE\n");
            break;
        case read_stmt:
            fprintf(output_file, "READ %s\n", stmt.data.read_stmt.name);
            break;
        case print_stmt:
            gen_code_expr(stmt.data.print_stmt.expr);
            fprintf(output_file, "PRINT\n");
            break;
        case block_stmt:
            gen_code_block(*(stmt.data.block_stmt.block));
            break;
        default:
            bail_with_error("Unknown statement kind in code generation!");
    }
}

// Generate code for an expression
static void gen_code_expr(expr_t expr) {
    switch (expr.expr_kind) {
        case expr_bin:
            gen_code_expr(*(expr.data.binary.expr1));
            gen_code_expr(*(expr.data.binary.expr2));
            fprintf(output_file, "OP %s\n", expr.data.binary.arith_op.text);
            break;
        case expr_negated:
            gen_code_expr(*(expr.data.negated.expr));
            fprintf(output_file, "NEG\n");
            break;
        case expr_ident:
            fprintf(output_file, "LOAD %s\n", expr.data.ident.name);
            break;
        case expr_number:
            fprintf(output_file, "PUSH %d\n", expr.data.number.value);
            break;
        default:
            bail_with_error("Unknown expression kind in code generation!");
    }
}

// Generate code for a condition
static void gen_code_condition(condition_t condition) {
    switch (condition.cond_kind) {
        case ck_db:
            gen_code_expr(condition.data.db_cond.dividend);
            gen_code_expr(condition.data.db_cond.divisor);
            fprintf(output_file, "DIVISIBLE\n");
            break;
        case ck_rel:
            gen_code_expr(condition.data.rel_op_cond.expr1);
            gen_code_expr(condition.data.rel_op_cond.expr2);
            fprintf(output_file, "REL %s\n", condition.data.rel_op_cond.rel_op.text);
            break;
        default:
            bail_with_error("Unknown condition kind in code generation!");
    }
}
