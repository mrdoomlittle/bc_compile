# include "bcc.h"
# include <stdarg.h>
char const* node_kind_as_str(mdl_u8_t);

mdl_uint_t padding = 0;
void static debug_print(char const *__s, ...) {
	va_list args;
	va_start(args, __s);

	fprintf(stdout, "|;");
	if (!padding) fprintf(stdout, " ");
	mdl_uint_t i = 0;
	for (;i != padding; i++)
		fprintf(stdout, "  ", padding);

	vfprintf(stdout, __s, args);
	va_end(args);
}

void print_padding(mdl_uint_t __amount) {
	for (mdl_uint_t o = 0; o != __amount; o++) printf("");}

void print_token(struct token *__tok) {
	if (__tok == NULL) return;

	debug_print("token info:\n");
	debug_print("	kind: %s\n", token_kind_as_str(__tok->kind));
	debug_print("	id: %s\n", token_id_as_str(__tok->id));
	debug_print("	val: %s\n", (char*)__tok->p);
}

mdl_u8_t trunk = 0;
mdl_uint_t depth = 0;
void print_node(struct node *__node) {
	mdl_u8_t is_trunk = 0;
	if (!trunk) {is_trunk = 1;trunk = 1;
		fprintf(stdout, "/---------------------------------------------------\\\n");
		padding++;
	}
	if (__node == NULL) return;
	debug_print("node_depth: %u\n", depth);
	debug_print("node_kind: %s\n", node_kind_as_str(__node->kind));

	if (__node->kind == AST_DEREF)
		debug_print("ptr type: %u, type: %u\n", __node->_type->bcit, (*__node->child_buff)->_type->bcit);

	debug_print("vec contents: \n");
	if (__node->kind == AST_COMPOUND_STMT) {
		debug_print("args: \n");
		padding++;
		struct node **itr = (struct node**)vec_begin(&__node->_vec);
		for (; itr != NULL; vec_itr((void**)&itr, VEC_ITR_DOWN, 1))
			if (*itr != NULL) print_node(*itr);
		padding--;
	}

	if (__node->kind == AST_FUNC) {
		debug_print("params: \n");
		padding++;
		struct node **itr = (struct node**)vec_begin(&__node->params);
		for (; itr != NULL; vec_itr((void**)&itr, VEC_ITR_DOWN, 1))
			if (*itr != NULL) print_node(*itr);
		padding--;
	}

	if (__node->kind == AST_FUNC_CALL) {
		padding++;
		for (struct node **itr = (struct node**)vec_begin(&__node->args); itr != NULL; vec_itr((void**)&itr, VEC_ITR_DOWN, 1))
			if (*itr != NULL) print_node(*itr);
		padding--;
	}

	debug_print("node l: %s\n", __node->l == NULL? "null":" ");

	padding++;
	if (__node->l != NULL) print_node(__node->l);
	if (__node->r != NULL) print_node(__node->r);
	padding--;

	debug_print("node r: %s\n", __node->r == NULL? "null":" ");

	padding = ++depth;
	debug_print("\x1B[32mchild nodes:\x1B[0m\n");
	struct node **itr = __node->child_buff;
	for (; itr != __node->child_itr; itr++)
		if (*itr != NULL) print_node(*itr);

	if (trunk && is_trunk) {padding = 0;trunk = 0;depth = 0;
		fprintf(stdout, "\\---------------------------------------------------/\n");
	}
}

char const* node_kind_as_str(mdl_u8_t __kind) {
	switch(__kind) {
		case OP_INCR:
			return "op incr";
		case OP_DECR:
			return "op decr";
		case AST_DECL:
			return "ast decl";
		case AST_EXIT:
			return "ast exit";
		case AST_LITERAL:
			return "ast literal";
		case AST_VAR:
			return "ast var";
		case AST_INIT:
			return "ast init";
		case OP_ADD:
			return "op add";
		case OP_SUB:
			return "op sub";
		case OP_MUL:
			return "op mul";
		case OP_DIV:
			return "op div";
		case OP_ASSIGN:
			return "op assign";
		case AST_IF:
			return "ast if";
		case OP_EQ:
			return "op eq";
		case OP_NEQ:
			return "op neq";
		case AST_COMPOUND_STMT:
			return "ast compound statment";
		case AST_LABEL:
			return "ast label";
		case AST_GOTO:
			return "ast goto";
		case AST_WHILE:
			return "ast while";
		case AST_FUNC_CALL:
			return "ast func call";
		case AST_FUNC:
			return "ast func";
		case AST_ADDROF:
			return "ast addrof";
		case AST_DEREF:
			return "ast deref";
		case AST_PRINT:
			return "ast print";
		case AST_CONV:
			return "ast conv";
		case OP_CAST:
			return "op cast";
		case AST_STRUCT_REF:
			return "ast struct ref";
		case OP_GT:
			return "op gt";
		case OP_LT:
			return "op lt";
	}
	return "no data";
}

char const* token_kind_as_str(mdl_u8_t __kind) {
	switch(__kind) {
		case TOK_KEYWORD:
			return "keyword";
		case TOK_NO:
			return "number";
		case TOK_CHR:
			return "character";
		case TOK_INVALID:
			return "invalid";
		case TOK_SPACE:
			return "space";
		case TOK_IDENT:
			return "identifier";
		case TOK_NEWLINE:
			return "newline";
	}
	return "no data";
}

char const* bca_blk_kind_as_str(mdl_u8_t __kind) {
	switch(__kind) {
		case BCA_VAR:
			return "bca var";
		case BCA_PRINT:
			return "bca print";
	}
	return "no data";
}

char const* token_id_as_str(mdl_u8_t __id) {
	switch(__id) {
		case INCR:
			return "incr";
		case DECR:
			return "decr";
		case EQ:
			return "equal";
		case NEQ:
			return "not equal";
		case EQEQ:
			return "assign";
		case ID_INVALID:
			return "invalid";
		case L_BRACE:
			return "left brace";
		case R_BRACE:
			return "right brace";
		case L_PAREN:
			return "left paren";
		case R_PAREN:
			return "right paren";
		case SEMICOLON:
			return "semicolon";
		case K_IF: case K_WHILE: case K_X8_T: case K_X16_T: case K_X32_T: case K_X64_T: case K_GOTO:
			return "expression";
		case K_EXIT:
			return "program exit";
		case PLUS:
			return "plus";
		case MINUS:
			return "minus";
		case GT:
			return "gt";
		case LT:
			return "lt";
	}
	return "no data";
}
