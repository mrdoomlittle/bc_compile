# include "bcc.h"
char const* node_kind_as_str(mdl_u8_t);

void print_padding(mdl_uint_t __amount) {
	for (mdl_uint_t o = 0; o != __amount; o++) printf("   ");}

mdl_uint_t padding = 0;
void print_token(struct token *__tok) {
	if (__tok == NULL) return;

	print_padding(padding);
	printf("token info:\n");

	print_padding(padding);
	printf("	kind: %s\n", token_kind_as_str(__tok->kind));

	print_padding(padding);
	printf("	id: %s\n", token_id_as_str(__tok->id));

	print_padding(padding);
	printf("	val: %s\n", (char*)__tok->p);
}

mdl_u8_t trunk = 0;
mdl_uint_t depth = 0;
void print_node(struct node *__node) {
	mdl_u8_t is_trunk = 0;
	if (!trunk) {is_trunk = 1;trunk = 1;}
	if (__node == NULL) return;
	print_padding(padding);
	printf("node_depth: %u\n", depth);
	print_padding(padding);
	printf("node_kind: %s\n", node_kind_as_str(__node->kind));

	if (__node->kind == AST_DEREF) {
		print_padding(padding);
		printf("ptr type: %u, type: %u\n", __node->_type->bcit, (*__node->child_buff)->_type->bcit);
	}

	print_padding(padding);
	printf("vec contents: \n");
	if (__node->kind == AST_COMPOUND_STMT) {
		padding++;
		for (struct node **itr = (struct node**)vec_begin(&__node->_vec); itr != NULL; vec_itr((void**)&itr, VEC_ITR_DOWN, 1))
			if (*itr != NULL) print_node(*itr);
		padding--;
	}

	if (__node->kind == AST_FUNC) {
		padding++;
		for (struct node **itr = (struct node**)vec_begin(&__node->params); itr != NULL; vec_itr((void**)&itr, VEC_ITR_DOWN, 1))
			if (*itr != NULL) print_node(*itr);
		padding--;
	}

	if (__node->kind == AST_FUNC_CALL) {
		padding++;
		for (struct node **itr = (struct node**)vec_begin(&__node->args); itr != NULL; vec_itr((void**)&itr, VEC_ITR_DOWN, 1))
			if (*itr != NULL) print_node(*itr);
		padding--;
	}

	print_padding(padding);
	printf("node l: %s\n", __node->l == NULL? "null":" ");

	padding++;
	if (__node->l != NULL) print_node(__node->l);
	if (__node->r != NULL) print_node(__node->r);
	padding--;

	print_padding(padding);
	printf("node r: %s\n", __node->r == NULL? "null":" ");

	print_padding(padding);
	padding = ++depth;
	printf("\x1B[32mchiled nodes:\x1B[0m\n");
	for (struct node **itr = __node->child_buff; itr != __node->child_itr; itr++)
		if (*itr != NULL) print_node(*itr);

	if (trunk && is_trunk) {padding = 0;trunk = 0;depth = 0;}
}

char const* node_kind_as_str(mdl_u8_t __kind) {
	switch(__kind) {
		case AST_INCR:
			return "ast incr";
		case AST_DECR:
			return "ast decr";
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
		case AST_ADD:
			return "ast add";
		case AST_SUB:
			return "ast sub";
		case AST_ASSIGN:
			return "ast assign";
		case AST_IF:
			return "ast if";
		case AST_OP_EQ:
			return "ast op eq";
		case AST_OP_NEQ:
			return "ast op neq";
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
		case OP_INCR:
			return "op incr";
		case OP_DECR:
			return "op decr";
		case OP_EQ:
			return "equal";
		case OP_NEQ:
			return "not equal";
		case OP_ASSIGN:
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
		case OP_PLUS:
			return "plus oporator";
	}

	return "no data";
}
