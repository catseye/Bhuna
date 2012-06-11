/*
 * parse.h
 * Parser structures and prototypes for Bhuna.
 * $Id: parse.h 54 2004-04-23 22:51:09Z catseye $
 */

#ifndef __PARSE_H_
#define __PARSE_H_

struct symbol_table;
struct symbol;
struct scan_st;
struct ast;
struct value;

struct ast	*parse_program(struct scan_st *, struct symbol_table *);
struct ast	*parse_block(struct scan_st *, struct symbol_table *,
			struct symbol_table **, int *);
struct ast	*parse_statement(struct scan_st *, struct symbol_table *,
			int *, int *);
struct ast	*parse_definition(struct scan_st *, struct symbol_table *, int,
			int *);
struct ast	*parse_command_or_assignment(struct scan_st *, struct symbol_table *,
			int *);
struct ast	*parse_expr_list(struct scan_st *, struct symbol_table *,
			struct symbol *, int *);
struct ast	*parse_expr(struct scan_st *, struct symbol_table *, int,
			struct symbol *, int *);
struct ast	*parse_primitive(struct scan_st *, struct symbol_table *,
			struct symbol *, int *);
struct ast	*parse_literal(struct scan_st *, struct symbol_table *);
struct ast	*parse_var(struct scan_st *, struct symbol_table *,
			   struct symbol **, int, int, struct value *);

#endif /* !__PARSE_H_ */