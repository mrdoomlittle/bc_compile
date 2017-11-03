# include "bcc.h"
struct buff static node_buff;
struct node *node_itr;

struct type *type_void = &(struct type){.bcit=_bcit_void, .kind=T_KIND_VOID, .size=0, .flags=0};

struct type *type_8l_u = &(struct type){.bcit=_bcit_8l, .kind=T_KIND_X8, .size=1, .flags=UNSIGNED};
struct type *type_16l_u = &(struct type){.bcit=_bcit_16l, .kind=T_KIND_X16, .size=2, .flags=UNSIGNED};
struct type *type_32l_u = &(struct type){.bcit=_bcit_32l, .kind=T_KIND_X32, .size=4, .flags=UNSIGNED};
struct type *type_64l_u = &(struct type){.bcit=_bcit_64l, .kind=T_KIND_X64, .size=8, .flags=UNSIGNED};

struct type *type_8l_s = &(struct type){.bcit=_bcit_8l, .kind=T_KIND_X8, .size=1, .flags=SIGNED};
struct type *type_16l_s = &(struct type){.bcit=_bcit_16l, .kind=T_KIND_X16, .size=2, .flags=SIGNED};
struct type *type_32l_s = &(struct type){.bcit=_bcit_32l, .kind=T_KIND_X32, .size=4, .flags=SIGNED};
struct type *type_64l_s = &(struct type){.bcit=_bcit_64l, .kind=T_KIND_X64, .size=8, .flags=SIGNED};

struct type *type_ptr = &(struct type){.bcit=_bcit_addr, .kind=T_KIND_PTR, .size=2, .flags=UNSIGNED};

struct map static global_env;
struct map static *local_env = NULL;
struct map static bca_env;

struct map static tags;
struct type *curr_ret_type;

void read_declarator(struct type**, struct type*, struct vec*, char**);
void make_array_type(struct type**, struct type*, mdl_uint_t);
void read_postfix_expr(struct node**);
void read_unary_expr(struct node**);
void read_decl_spec(struct type**, struct token*);
void read_var_or_func(struct node**, char*);
void read_cast_expr(struct node**);
void make_array_type(struct type**, struct type*, mdl_uint_t);

struct bca_blk bca_blk_buff[200];
struct bca_blk *bca_blk_itr = bca_blk_buff;

void bca_var(struct bca_blk *__blk, char *__name, mdl_uint_t __val) {
	*__blk = (struct bca_blk){.kind=BCA_VAR, .name=__name, .val = __val};
}

void bca_print(struct bca_blk *__blk, mdl_u8_t __bcit, bci_addr_t __addr, mdl_u8_t __flags) {
	*__blk = (struct bca_blk){.kind=BCA_PRINT, .flags=__flags, .bcit = __bcit, .addr = __addr};
}

void bca_mov(struct bca_blk *__blk, mdl_u8_t __bcit, bci_addr_t __dst_addr, bci_addr_t __src_addr, mdl_u8_t __flags) {
	*__blk = (struct bca_blk){.kind=BCA_MOV, .flags=__flags, .bcit=__bcit, .dst_addr=__dst_addr, .src_addr=__src_addr};
}

void bca_assign(struct bca_blk *__blk, bci_addr_t __addr, mdl_uint_t __val, mdl_u8_t __bcit, mdl_u8_t __flags) {
	*__blk = (struct bca_blk){.kind=BCA_ASSIGN, .flags=__flags, .addr=__addr, .val=__val, .bcit=__bcit};
}

void bca_nop(struct bca_blk *__blk, mdl_u8_t __flags) {
	*__blk = (struct bca_blk){.kind=BCA_NOP, .flags=__flags};
}

void bca_incr_or_decr(struct bca_blk *__blk, mdl_u8_t __kind, mdl_u8_t __bcit, bci_addr_t __addr, mdl_uint_t __bc, mdl_u8_t __flags) {
	*__blk = (struct bca_blk){.kind=__kind, .bcit=__bcit, .addr=__addr, .bc=__bc, .flags=__flags};
}

void bca_eeb_init(struct bca_blk *__blk, mdl_u8_t __eeb_c, mdl_u8_t __flags) {
	*__blk = (struct bca_blk){.kind=BCA_EEB_INIT, .eeb_c=__eeb_c, .flags=__flags};
}

void bca_eeb_put(struct bca_blk *__blk, mdl_u8_t __eeb_id, bci_addr_t __b_addr, bci_addr_t __e_addr, mdl_u8_t __flags) {
	*__blk = (struct bca_blk){.kind=BCA_EEB_PUT, .eeb_id=__eeb_id, .b_addr=__b_addr, .e_addr=__e_addr, .flags=__flags};
}

void bca_fld(struct bca_blk *__blk, bci_addr_t __addr, bci_addr_t *__addr_p) {
	*__blk = (struct bca_blk){.kind=BCA_FLD, .addr=__addr, .p=(void*)__addr_p};
}

void bca_fst(struct bca_blk *__blk, mdl_u8_t __bcit, bci_addr_t *__addr_p, bci_addr_t __addr) {
	*__blk = (struct bca_blk){.kind=BCA_FST, .bcit=__bcit, .p=(void*)__addr_p, .addr=__addr};
}

void bca_dr(struct bca_blk *__blk, mdl_u8_t __bcit, bci_addr_t __dst_addr, bci_addr_t __src_addr, mdl_u8_t __flags) {
	*__blk = (struct bca_blk){.kind=BCA_DR, .bcit=__bcit, .dst_addr=__dst_addr, .src_addr=__src_addr, .flags=__flags};
}

void bca_env_add_var_int(char *__name, mdl_uint_t __val) {
	char *name = (char*)str_dupe(__name);
	printf("-------> %s\n", name);
	struct bca_blk *var = bca_blk_itr++;
	bca_var(var, name, __val);
	map_put(&bca_env, (mdl_u8_t const*)name, strlen(name), var);
}

void bca_init() {
	map_init(&bca_env);
	bca_env_add_var_int("bcit_8l", _bcit_8l);
	bca_env_add_var_int("bcit_16l", _bcit_16l);
	bca_env_add_var_int("bcit_32l", _bcit_32l);
	bca_env_add_var_int("bcit_64l", _bcit_64l);
	bca_env_add_var_int("bcit_addr", _bcit_addr);

	bca_env_add_var_int("rg_8a", RG_8A);
	bca_env_add_var_int("rg_8b", RG_8B);
	bca_env_add_var_int("rg_8c", RG_8C);

	bca_env_add_var_int("rg_16a", RG_16A);
	bca_env_add_var_int("rg_16b", RG_16B);
	bca_env_add_var_int("rg_16c", RG_16C);

	bca_env_add_var_int("rg_32a", RG_32A);
	bca_env_add_var_int("rg_32b", RG_32B);
	bca_env_add_var_int("rg_32c", RG_32C);

	bca_env_add_var_int("rg_64a", RG_64A);
	bca_env_add_var_int("rg_64b", RG_64B);
	bca_env_add_var_int("rg_64c", RG_64C);

	bca_env_add_var_int("sp", RG_SP);
	bca_env_add_var_int("bp", RG_BP);
}

bcc_err_t parse_init(mdl_uint_t __node_buff_size) {
	buff_init(&node_buff, sizeof(struct node), __node_buff_size);
	node_itr = (struct node*)buff_begin(&node_buff);
	map_init(&global_env);
	map_init(&tags);
	bca_init();
}

bcc_err_t parse_de_init() {
	buff_free(&node_buff);
}

# define incr_node_itr node_itr++;
# define decr_node_itr node_itr--;

struct node* next_node();
void read_stmt(struct node **);
void read_expr(struct node**);
bcc_err_t read_decl(struct vec*, mdl_u8_t);

mdl_u8_t next_token_is(mdl_u8_t __id) {
	struct token *tok;
	read_token(&tok, 1);

	if (is_keyword(tok, __id)) {
		return 1;
	}

	ulex(tok);
	return 0;
}

void skip_token() {read_token(NULL, 1);}

struct node static* env_lookup(char *__name) {
	struct node *node = NULL;

	if (local_env != NULL) {
		map_get(local_env, (mdl_u8_t const*)__name, strlen(__name), (void**)&node);
		if (node != NULL) return node;
	}

	map_get(&global_env, (mdl_u8_t const*)__name, strlen(__name), (void**)&node);
	if (node != NULL) return node;

	fprintf(stderr, "env lookup failed, for: %s\n", __name);
	return NULL;
}

bcc_err_t read_exit_stmt(struct token*, struct node**);

void add_child(struct node *__m_node, struct node *__child) {
	*__m_node->child_itr = __child;
	__m_node->child_itr++;
}

void init_node(struct node *__node) {
	__node->child_itr = __node->child_buff;
	__node->l = NULL;
	__node->r = NULL;
}

void build_ast(struct node **__node, struct node *__tmpl) {
	*__node = next_node();
	struct node *node = *__node;
	if (__tmpl != NULL)
		**__node = *__tmpl;
	init_node(node);
}

void ast_var(struct node **__node, struct type *__type, char *__name) {
	build_ast(__node, &(struct node){.kind=AST_VAR, ._type=__type, .p=(void*)__name});
}

void ast_decl(struct node **__node, struct node *__var, struct node *__init) {
	build_ast(__node, &(struct node){.kind=AST_DECL});
	add_child(*__node, __var);
	add_child(*__node, __init);
}

void ast_exit(struct node **__node) {
	build_ast(__node, &(struct node){.kind=AST_EXIT});
}

void ast_int_type(struct node **__node, struct type *__type, mdl_u8_t *__val) {
	build_ast(__node, &(struct node){.kind=AST_LITERAL, ._type=__type});
	*((mdl_u64_t*)(*__node)->val) = 0;
	byte_ncpy((*__node)->val, __val, bcit_sizeof(__type->bcit));
}

void ast_init(struct node **__node, struct type *__type) {
	build_ast(__node, &(struct node){.kind=AST_INIT, ._type=__type});
}

void ast_binop(struct node **__node, struct type *__type, mdl_u8_t __kind, struct node *__l, struct node *__r) {
	build_ast(__node, &(struct node){.kind=__kind, ._type=__type});
 	(*__node)->l = __l;
	(*__node)->r = __r;
}

void ast_if(struct node **__node, struct node *__cond, struct node *__block, struct node *__else) {
	build_ast(__node, &(struct node){.kind=AST_IF});
	add_child(*__node, __cond);
	add_child(*__node, __block);
	add_child(*__node, __else);
}

void ast_while(struct node **__node, struct node *__if, struct node *__else, struct node *__label) {
	build_ast(__node, &(struct node){.kind=AST_WHILE});
	add_child(*__node, __if);
	*(__if->child_buff+2) = __else;
	add_child(*__node, __label);
}

void ast_compound_stmt(struct node **__node, struct vec __vec) {
	build_ast(__node, &(struct node){.kind=AST_COMPOUND_STMT, ._vec=__vec});
}

void ast_label(struct node **__node, char *__label) {
	build_ast(__node, &(struct node){.kind=AST_LABEL, .p=(void*)__label});
}

void ast_goto(struct node **__node, char *__label) {
	build_ast(__node, &(struct node){.kind=AST_GOTO, .p=(void*)__label});
}

void ast_func_call(struct node **__node, struct type *__ret_type, struct node *__func, struct node *__goto, struct vec __args) {
	build_ast(__node, &(struct node){.kind=AST_FUNC_CALL, ._type=__ret_type, .args=__args});
	add_child(*__node, __func);
	add_child(*__node, __goto);
}

void ast_func(struct node **__node, char *__name, struct type *__type, struct node *__body, struct vec __params, struct node *__label) {
	build_ast(__node, &(struct node){.kind=AST_FUNC, .p=(void*)__name, ._type=__type, .params=__params});
	add_child(*__node, __label);
	add_child(*__node, __body);
}

void ast_uop(mdl_u8_t __kind, struct node **__node, struct type *__type, struct node *__val) {
	build_ast(__node, &(struct node){.kind=__kind, ._type=__type});
	add_child(*__node, __val);
}

void ast_return(struct node **__node, struct node *__ret_val) {
	build_ast(__node, &(struct node){.kind=AST_RETURN});
	add_child(*__node, __ret_val);
}

void ast_bca(struct node **__node, struct vec __vec) {
	build_ast(__node, &(struct node){.kind=AST_BCA, ._vec = __vec});
}

void ast_str(struct node **__node, char *__s, mdl_uint_t __cc) {
	struct type *_type = NULL;
	make_array_type(&_type, type_8l_u, __cc);

	build_ast(__node, &(struct node){.kind=AST_LITERAL, ._type=_type, .p=(char*)__s});
}

void ast_conv(struct node **__node, struct type *__to_type, struct node *__val) {
	build_ast(__node, &(struct node){.kind=AST_CONV, ._type=__to_type});
	add_child(*__node, __val);
}

void ast_struct_ref(struct node **__node, struct node *__struct, struct type *__type, char *__name) {
	build_ast(__node, &(struct node){.kind=AST_STRUCT_REF, ._type=__type, .p=(void*)__name});
	add_child(*__node, __struct);
}

void ast_va_ptr(struct node **__node, struct node *__arg, struct type *__type) {
	build_ast(__node, &(struct node){.kind=AST_VA_PTR, ._type=__type});
	add_child(*__node, __arg);
}

void build_type(struct type **__type, struct type *__tmpl) {
	struct type *_type = (struct type*)malloc(sizeof(struct type));
	memset(_type, 0, sizeof(struct type));
	if (__tmpl != NULL)
		*_type = *__tmpl;
	*__type = _type;
}

void make_array_type(struct type **__type, struct type *__base_type, mdl_uint_t __len) {
	build_type(__type, &(struct type){.kind=T_KIND_ARRAY, .bcit=_bcit_addr, .ptr=__base_type, .size=(__len*__base_type->size), .len=__len});
}

void make_struct_type(struct type **__type, struct map *__fields, mdl_uint_t __size) {
	build_type(__type, &(struct type){.kind=T_KIND_STRUCT, .fields=__fields, .size=__size});
}

void make_func_type(struct type **__type, struct type *__ret_type, struct vec __params, mdl_u8_t __hva) {
	build_type(__type, &(struct type){.kind=T_KIND_FUNC, .ret_type = __ret_type, .params = __params, .hva = __hva});
}

void make_notype(struct type **__type, mdl_u8_t __kind, mdl_u8_t __flags) {
	struct type *_type;
	build_type(&_type, NULL);

	_type->flags = __flags;
	switch((_type->kind = __kind)) {
		case T_KIND_X8: _type->size = 1; _type->bcit = _bcit_8l; break;
		case T_KIND_X16: _type->size = 2; _type->bcit = _bcit_16l; break;
		case T_KIND_X32: _type->size = 4; _type->bcit = _bcit_32l; break;
		case T_KIND_X64: _type->size = 8; _type->bcit = _bcit_64l; break;
	}

	*__type = _type;
}

void make_ptr_type(struct type **__type, struct type *__ptr) {
	build_type(__type, &(struct type){.kind=T_KIND_PTR, .ptr=__ptr, .size=bcit_sizeof(_bcit_addr), .bcit=_bcit_addr});
}

void read_struct_fields(struct vec *__vec) {
	vec_init(__vec);

	expect_token(TOK_KEYWORD, L_BRACE);
	if (next_token_is(R_BRACE)) return;

	struct type *base_type;
	struct type *_type;

	struct node *var;

	struct token *tok;
	char *name;
	for (;;) {
		read_token(&tok, 1);

		read_decl_spec(&base_type, tok);

		_again:
		printf(":::;;;\n");
		read_declarator(&_type, base_type, NULL, &name);

		struct type *t = (struct type*)malloc(sizeof(struct type));
		*t = *_type;

		struct pair_t *pair;
		vec_push(__vec, (void**)&pair, sizeof(struct pair_t));
		pair->first = name;
		pair->second = t;

		if (next_token_is(COMMA)) goto _again;
		expect_token(TOK_KEYWORD, SEMICOLON);

		if (next_token_is(R_BRACE)) break;
	}
}

void read_struct_decl(struct map *__fields, mdl_uint_t *__size) {
	map_init(__fields);
	struct vec vec;
	read_struct_fields(&vec);

	mdl_uint_t off = 0;
	struct pair_t *itr = (struct pair_t*)vec_begin(&vec);
	for (;itr != NULL; vec_itr((void**)&itr, VEC_ITR_DOWN, 1)) {
		struct type *_type = (struct type*)itr->second;

		_type->offset = off;
		off += _type->size;

		printf(";;;;;;;;;;;;;;;;;;;;; put: %s, offset: %u\n", itr->first, _type->offset);
		map_put(__fields, (char*)itr->first, strlen((char*)itr->first), _type);
	}

	*__size = off;
}

void read_struct_spec(struct type**__type) {
	struct token *tok;
	read_token(&tok, 1);

	if (tok->kind != TOK_IDENT) {
		fprintf(stderr, "dident get ident.\n");
	}

	char *tag = (char*)tok->p;

	map_get(&tags, (mdl_u8_t const*)tag, strlen(tag), (void**)__type);
	if (*__type != NULL) return;

	mdl_uint_t size = 0;
	struct map *fields = (struct map*)malloc(sizeof(struct map));
	read_struct_decl(fields, &size);

	make_struct_type(__type, fields, size);
	map_put(&tags, (mdl_u8_t const*)tag, strlen(tag), *__type);

	printf("struct tag: %s\n", tag);
}

void read_decl_spec(struct type **__type, struct token *__tok) {
	switch(__tok->id) {
		case K_X8_T:
			make_notype(__type, T_KIND_X8, (*(mdl_u8_t*)__tok->p)&0xC);
		break;
		case K_X16_T:
			make_notype(__type, T_KIND_X16, (*(mdl_u8_t*)__tok->p)&0xC);
		break;
		case K_X32_T:
			make_notype(__type, T_KIND_X32, (*(mdl_u8_t*)__tok->p)&0xC);
		break;
		case K_X64_T:
			make_notype(__type, T_KIND_X64, (*(mdl_u8_t*)__tok->p)&0xC);
		break;
		case K_STRUCT:
			read_struct_spec(__type);
		break;
	}
}

mdl_u8_t is_int_type(struct type *__type) {
	switch(__type->kind) {
		case T_KIND_X8: case T_KIND_X16:
		case T_KIND_X32: case T_KIND_X64:
			return 1;
		break;
	}
	return 0;
}

mdl_u8_t is_keyword(struct token *__tok, mdl_u8_t __id) {
	return ((__tok->kind == TOK_KEYWORD) && (__tok->id == __id));
}

mdl_u8_t is_arith_type(struct type *__type) {
	return is_int_type(__type);
}

void read_stmt(struct node**);
bcc_err_t read_assign_expr(struct node**);
void read_decl_or_stmt(struct vec *__vec) {
	struct token *tok;
	peek_token(&tok);

	if (is_keyword(tok, K_X8_T) || is_keyword(tok, K_X16_T) || is_keyword(tok, K_X32_T) || is_keyword(tok, K_X64_T) || is_keyword(tok, K_STRUCT)) {
		read_decl(__vec, 1);
	} else {
		struct node *stmt = NULL;
		read_stmt(&stmt);

		if (stmt != NULL) {
			struct node **node;
			vec_push(__vec, (void**)&node, sizeof(struct node*));
			*node = stmt;
		}
	}
}

void read_compound_stmt(struct node **__node) {
	struct vec vec;
	struct map env;
	map_init(&env);
	map_set_parent(&env, local_env);
	local_env = &env;

	vec_init(&vec);

	for (;;) {
		if (next_token_is(R_BRACE)) {
			break;
		}

		read_decl_or_stmt(&vec);
	}

	ast_compound_stmt(__node, vec);
	local_env = map_get_parent(&env);
	map_de_init(&env);
}

struct node* next_node() {
	struct node *node = node_itr;
	incr_node_itr
	return node;
}

void make_label(char **label, char *key, mdl_uint_t __id) {
	char static buff[200];

	sprintf(buff, "%u\0", __id);
	mdl_uint_t id_len = strlen(buff);

	mdl_uint_t key_len = strlen(key);
	*label = (char*)malloc(id_len+key_len+1+12);
	char *itr = *label;

	strncpy(itr, key, key_len);
	itr+=key_len;

	*itr = '_';
	itr++;

	strncpy(itr, buff, id_len);
	itr+=id_len;

	*itr = '\0';
}

void read_if_stmt(struct node **__node) {
	expect_token(TOK_KEYWORD, L_PAREN);
	struct node *cond = NULL;
	read_expr(&cond);
	expect_token(TOK_KEYWORD, R_PAREN);

	struct node *block = NULL;
	read_stmt(&block);

	if (!next_token_is(K_ELSE)) {
		ast_if(__node, cond, block, NULL);
		return;
	}

	struct node *_else = NULL;
	read_stmt(&_else);

	ast_if(__node, cond, block, _else);
}

mdl_uint_t static while_id = 0;
void read_while_stmt(struct node **__node) {
	struct node *stmt = NULL;
	struct node *_goto = NULL;
	struct node *_label = NULL;

	char *label;
	make_label(&label, "while", while_id++);

	read_if_stmt(&stmt);

	ast_goto(&_goto, label);
	ast_label(&_label, label);

	ast_while(__node, stmt, _goto, _label);
}

void read_label(struct node **__node, struct token *__tok) {
	char *label = (char*)__tok->p;
	printf("lable found. %s\n", label);

	struct vec vec;
	vec_init(&vec);

	struct node *_label = NULL;
	ast_label(&_label, label);

	struct node **node = NULL;

	struct node *stmt = NULL;
	read_stmt(&stmt);

	vec_push(&vec, (void**)&node, sizeof(struct node*));
	*node = _label;

	vec_push(&vec, (void**)&node, sizeof(struct node*));
	*node = stmt;

	ast_compound_stmt(__node, vec);
}

void read_goto_stmt(struct node **__node) {
	struct token *tok;
	read_token(&tok, 1);

	if (tok->kind != TOK_IDENT) {
		fprintf(stderr, "error\n");
	}

	char *label = (char*)tok->p;

	ast_goto(__node, label);

	expect_token(TOK_KEYWORD, SEMICOLON);
}

void read_return_stmt(struct node **__node) {
	struct node *ret_val = NULL;
	read_expr(&ret_val);

	struct node *node = ret_val;
	if (ret_val->_type->bcit != curr_ret_type->bcit)
		ast_conv(&node, curr_ret_type, ret_val);

	ast_return(__node, node);
	expect_token(TOK_KEYWORD, SEMICOLON);
}

mdl_uint_t bca_read_literal(char **__itr) {
	struct bca_token *tok;
	read_bca_token(&tok, __itr);

	if (tok->kind == TOK_NO)
		if (tok->is_hex)
			return hex_to_int((char*)tok->p);
		else
			return str_to_int((char*)tok->p);
	else if (tok->kind == TOK_IDENT) {
		struct bca_blk *blk;
		map_get(&bca_env, (mdl_u8_t const*)tok->p, strlen((char*)tok->p), (void**)&blk);
		return blk->val;
	}
	return 0;
}

void bca_read_print_stmt(struct bca_blk *__blk, char **__itr) {
	mdl_u8_t bcit = bca_read_literal(__itr);
	bci_addr_t addr = bca_read_literal(__itr);
	mdl_u8_t flags = bca_read_literal(__itr);

	bca_print(__blk, bcit, addr, flags);
}

void bca_read_mov_stmt(struct bca_blk *__blk, char **__itr) {
	mdl_u8_t bcit = bca_read_literal(__itr);
	bci_addr_t dst_addr = bca_read_literal(__itr);
	bci_addr_t src_addr = bca_read_literal(__itr);
	mdl_u8_t flags = bca_read_literal(__itr);

	bca_mov(__blk, bcit, dst_addr, src_addr, flags);
}

void bca_read_assign_stmt(struct bca_blk *__blk, char **__itr) {
	bci_addr_t addr = bca_read_literal(__itr);
	mdl_uint_t val = bca_read_literal(__itr);
	mdl_u8_t bcit = bca_read_literal(__itr);
	mdl_u8_t flags = bca_read_literal(__itr);

	bca_assign(__blk, addr, val, bcit, flags);
}

void bca_read_nop_stmt(struct bca_blk *__blk, char **__itr) {
	mdl_u8_t flags = bca_read_literal(__itr);
	bca_nop(__blk, flags);
}

void bca_read_incr_or_decr_stmt(struct bca_blk *__blk, char **__itr, mdl_u8_t __kind) {
	mdl_u8_t bcit = bca_read_literal(__itr);
	bci_addr_t addr = bca_read_literal(__itr);
	mdl_uint_t bc = bca_read_literal(__itr);
	mdl_u8_t flags = bca_read_literal(__itr);
	bca_incr_or_decr(__blk, __kind, bcit, addr, bc, flags);
}

void bca_read_eeb_init(struct bca_blk *__blk, char **__itr) {
	mdl_u8_t blk_c = bca_read_literal(__itr);
	mdl_u8_t flags = bca_read_literal(__itr);
	bca_eeb_init(__blk, blk_c, flags);
}

void bca_read_eeb_put(struct bca_blk *__blk, char **__itr) {
	mdl_u8_t blk_id = bca_read_literal(__itr);
	bci_addr_t b_addr = bca_read_literal(__itr);
	bci_addr_t e_addr = bca_read_literal(__itr);
	mdl_u8_t flags = bca_read_literal(__itr);
	bca_eeb_put(__blk, blk_id, b_addr, e_addr, flags);
}

char static* bca_read_ident(char **__itr) {
	struct bca_token *tok;
	read_bca_token(&tok, __itr);
	return (char*)tok->p;
}

void bca_read_fld(struct bca_blk *__blk, char **__itr) {
	bci_addr_t addr = bca_read_literal(__itr);
	char *name = bca_read_ident(__itr);
	bca_fld(__blk, addr, &env_lookup(name)->off);
}

void bca_read_fst(struct bca_blk *__blk, char **__itr) {
	char *name = bca_read_ident(__itr);
	bci_addr_t addr = bca_read_literal(__itr);
	struct node *_node = env_lookup(name);
	bca_fst(__blk, _node->_type->bcit, &_node->off, addr);
}

void bca_read_dr_stmt(struct bca_blk *__blk, char **__itr) {
	mdl_u8_t bcit = bca_read_literal(__itr);
	bci_addr_t dst_addr = bca_read_literal(__itr);
	bci_addr_t src_addr = bca_read_literal(__itr);
	mdl_u8_t flags = bca_read_literal(__itr);

	bca_dr(__blk, bcit, dst_addr, src_addr, flags);
}

struct vec read_bca(char *__bca, mdl_uint_t __cc) {
	struct vec _vec;
	vec_init(&_vec);

	char *itr = __bca;
	while(itr < __bca+__cc) {
		struct bca_token *tok;
		read_bca_token(&tok, &itr);
		if (!tok) {
			itr++;
			continue;
		}

		struct bca_blk *blk;
		vec_push(&_vec, (void**)&blk, sizeof(struct bca_blk));

		if (!strcmp((char*)tok->p, "print"))
			bca_read_print_stmt(blk, &itr);
		else if (!strcmp((char*)tok->p, "mov"))
			bca_read_mov_stmt(blk, &itr);
		else if (!strcmp((char*)tok->p, "assign"))
			bca_read_assign_stmt(blk, &itr);
		else if (!strcmp((char*)tok->p, "nop"))
			bca_read_nop_stmt(blk, &itr);
		else if (!strcmp((char*)tok->p, "incr"))
			bca_read_incr_or_decr_stmt(blk, &itr, BCA_INCR);
		else if (!strcmp((char*)tok->p, "decr"))
			bca_read_incr_or_decr_stmt(blk, &itr, BCA_DECR);
		else if (!strcmp((char*)tok->p, "eeb_init"))
			bca_read_eeb_init(blk, &itr);
		else if (!strcmp((char*)tok->p, "eeb_put"))
			bca_read_eeb_put(blk, &itr);
		else if (!strcmp((char*)tok->p, "fld"))
			bca_read_fld(blk, &itr);
		else if (!strcmp((char*)tok->p, "fst"))
			bca_read_fst(blk, &itr);
		else if (!strcmp((char*)tok->p, "dr"))
			bca_read_dr_stmt(blk, &itr);
		else
			itr++;
	}
	return _vec;
}

void read_bca_stmt(struct node **__node) {
	expect_token(TOK_KEYWORD, L_PAREN);

	struct token *tok;
	read_token(&tok, 1);
	ast_bca(__node, read_bca((char*)tok->p, tok->bc));

	expect_token(TOK_KEYWORD, R_PAREN);
	expect_token(TOK_KEYWORD, SEMICOLON);
}

void read_sizeof_operand(struct node **__node) {
	expect_token(TOK_KEYWORD, L_PAREN);

	struct type *base_type, *_type;
	read_decl_spec(&base_type, read_token(NULL, 1));
	read_declarator(&_type, base_type, NULL, NULL);

	struct type *t;
	make_notype(&t, T_KIND_X16, 0);
	ast_int_type(__node, t, (mdl_u8_t*)&_type->size);

	expect_token(TOK_KEYWORD, R_PAREN);
}

void read_va_ptr_operand(struct node **__node) {
	expect_token(TOK_KEYWORD, L_PAREN);

	struct type *_type;
	struct node *_node;
	read_expr(&_node);

	build_type(&_type, &(struct type){.bcit=_bcit_void, .kind=T_KIND_VOID, .size=0, .flags = 0});
	make_ptr_type(&_type, _type);
	ast_va_ptr(__node, _node, _type);

	expect_token(TOK_KEYWORD, R_PAREN);
}

void read_stmt(struct node **__node) {
	struct token *tok;
	read_token(&tok, 1);

	if (tok->kind == TOK_KEYWORD) {
		switch(tok->id) {
			case L_BRACE: read_compound_stmt(__node); return;
			case K_IF: read_if_stmt(__node); return;
			case K_EXIT: read_exit_stmt(tok, __node); return;
			case K_WHILE: read_while_stmt(__node); return;
			case K_GOTO: read_goto_stmt(__node); return;
			case K_TYPEDEF: read_typedef(); return;
			case K_RETURN: read_return_stmt(__node); return;
			case K_BCA: read_bca_stmt(__node); return;
		}
	}

	if (tok->kind == TOK_IDENT && next_token_is(COLON)) {
		read_label(__node, tok);
		return;
	}

	ulex(tok);

	read_assign_expr(__node);
	expect_token(TOK_KEYWORD, SEMICOLON);
}

void read_no(struct node **__node, char *__s, mdl_u8_t __hex) {
	mdl_uint_t val;
	if (__hex)
		val = hex_to_int(__s);
	else
		val = str_to_int(__s);

	struct type *_type = type_8l_u;
	if (val >= 0 && val <= (mdl_u8_t)~0)
		_type = type_8l_u;
	else if (val > (mdl_u8_t)~0 && val <= (mdl_u16_t)~0)
		_type = type_16l_u;
	else if (val > (mdl_u16_t)~0 && val <= (mdl_u32_t)~0)
		_type = type_32l_u;
	else if (val > (mdl_u32_t)~0 && val <= (mdl_u64_t)~0)
		_type = type_64l_u;
	ast_int_type(__node, _type, (mdl_u8_t*)&val);
}

void read_primary_expr(struct node**);
void read_additive_expr(struct node**);

void read_func_param(char **__name, struct type **__type) {
	struct token *tok;
	read_token(&tok, 1);

	struct type *base_type = NULL;
	read_decl_spec(&base_type, tok);

	read_declarator(__type, base_type, NULL, __name);
}

struct vec read_func_args() {
	struct vec args;
	vec_init(&args);

	for (;;) {
		if (next_token_is(R_PAREN)) break;

		struct node **node;
		vec_push(&args, (void**)&node, sizeof(struct node*));

		struct node *arg = NULL;
		read_stmt(&arg);
		*node = arg;

		expect_token(TOK_KEYWORD, COMMA);
	}

	return args;
}

void read_declarator_params(struct vec *__params, mdl_u8_t *__ellipsis) {
	struct token *tok;
	if (__ellipsis != NULL) *__ellipsis = 0;
	for (;;) {
		if (next_token_is(R_PAREN)) break;
		if (next_token_is(ELLIPSIS)) {
			if (__ellipsis != NULL) *__ellipsis = 1;
			expect_token(TOK_KEYWORD, R_PAREN);
			break;
		}

		struct node **node;
		vec_push(__params, (void**)&node, sizeof(struct node*));
		*node = NULL;

		//struct node *param = NULL;

		char *name;
		struct type *_type;
		read_func_param(&name, &_type);

		struct node *var;
		ast_var(&var, _type, name);
		*node = var;

		map_put(local_env, (mdl_u8_t const*)name, strlen(name), var);

		if (!next_token_is(R_PAREN)) {
			if (expect_token(TOK_KEYWORD, COMMA) < 0) {
				fprintf(stderr, "expected a comma.\n");
			}
		} else break;
	}
}

struct type* bcit_to_type(mdl_u8_t __bcit, mdl_u8_t __sgnd) {
	switch(__bcit) {
		case _bcit_void: return type_void;
		case _bcit_8l: return (__sgnd? type_8l_s:type_8l_u);
		case _bcit_16l: return (__sgnd? type_16l_s:type_16l_u);
		case _bcit_32l: return (__sgnd? type_32l_s:type_32l_u);
		case _bcit_64l: return (__sgnd? type_64l_s:type_64l_u);
		case _bcit_addr: return type_ptr;
	}
	return NULL;
}

void read_func_call(struct node **__node, struct node *__func) {
	struct node *_goto = NULL;
	struct vec args = read_func_args();
	if (!strcmp((char*)__func->p, "_print")) {
		if (!vec_size(&args)) {
			fprintf(stderr, "print function need one argument.\n");
		}

		ast_func_call(__node, NULL, NULL, NULL, args);
		(*__node)->kind = AST_PRINT;
	} else if (!strcmp((char*)__func->p, "extern_call")) {
		ast_func_call(__node, __func->_type->ret_type, NULL, NULL, args);
		(*__node)->kind = AST_EXTERN_CALL;
	} else {
		ast_goto(&_goto, (void*)__func->p);
		if (vec_blk_c(&__func->params) > 0 && vec_blk_c(&args) > 0) {
			struct node **p1 = vec_begin(&__func->params);
			struct node **p2 = vec_begin(&args);

			while(p1 != NULL) {
				if ((*p2)->_type->bcit != (*p1)->_type->bcit) {
					ast_conv(p2, bcit_to_type((*p1)->_type->bcit, 0), *p2);
				}

				vec_itr((void**)&p1, VEC_ITR_DOWN, 1);
				vec_itr((void**)&p2, VEC_ITR_DOWN, 1);
			}
		}

		ast_func_call(__node, __func->_type->ret_type, __func, _goto, args);
	}
}

void read_struct_field(struct node **__node, struct node *__struct) {
	struct token *tok;
	read_token(&tok, 1);

	struct type *_type;
	map_get(__struct->_type->fields, tok->p, strlen(tok->p), (void**)&_type);

	ast_struct_ref(__node, __struct, _type, tok->p);
}

void read_primary_expr(struct node **__node) {
	if (next_token_is(L_PAREN)) {
		read_expr(__node);
		expect_token(TOK_KEYWORD, R_PAREN);
		return;
	}

	struct token *tok;
	read_token(&tok, 1);

	switch(tok->kind) {
		case TOK_IDENT:
			read_var_or_func(__node, (char*)tok->p);
		break;
		case TOK_NO:
			read_no(__node, (char*)tok->p, tok->hex);
		break;
		case TOK_KEYWORD:
			ulex(tok);
		break;
		case TOK_STR:
			ast_str(__node, (char*)tok->p, tok->bc);
		break;
		case TOK_CHR:
			ast_int_type(__node, type_8l_u, (mdl_u8_t*)tok->p);
		break;
		default:
			ulex(tok);
			printf("error. got:\n");
			print_token(tok);
	}
}

void read_unary_addrof(struct node **__node) {
	struct node *val = NULL;
	read_cast_expr(&val);

	struct type *_type = NULL;
	make_ptr_type(&_type, val->_type);
	print_node(val);
	ast_uop(AST_ADDROF, __node, _type, val);
}

void read_unary_deref(struct node **__node) {
	struct node *val = NULL;
	read_cast_expr(&val);

	ast_uop(AST_DEREF, __node, val->_type->ptr, val);
}

void read_unary_expr(struct node **__node) {
	struct token *tok;
	read_token(&tok, 1);

	if (tok->kind == TOK_KEYWORD){
		switch(tok->id) {
			case K_SIZEOF: read_sizeof_operand(__node); return;
			case K_VA_PTR: read_va_ptr_operand(__node); return;
			case ASTERISK: read_unary_deref(__node); return;
			case AMPERSAND: read_unary_addrof(__node); return;
		}
	}

	ulex(tok);
	read_primary_expr(__node);
}

void read_subscript_expr(struct node **__node, struct node *__lval) {
	struct node *add = NULL, *no = NULL;
	read_expr(&no);

	if (no->_type->bcit != type_ptr->bcit)
		ast_conv(&no, type_ptr, no);

	ast_binop(&add, __lval->_type, OP_ADD, __lval, no);

	ast_uop(AST_DEREF, __node, add->_type->ptr, add);
	expect_token(TOK_KEYWORD, R_BRACKET);
	if (next_token_is(PERIOD))
		read_struct_field(__node, *__node);
}

void read_postfix_expr(struct node **__node) {
	struct node *node = NULL;
	read_unary_expr(&node);

	if (next_token_is(L_PAREN)) {
		read_func_call(__node, node);
		return;
	} else if (next_token_is(L_BRACKET)) {
		read_subscript_expr(__node, node);
		return;
	} else if (next_token_is(PERIOD)) {
		read_struct_field(__node, node);
		return;
	} else if (next_token_is(ARROW)) {
		printf("op:------------------------------------====: %u\n", node->_type->ptr->bcit);
		print_node(node);
		ast_uop(AST_DEREF, &node, node->_type->ptr, node);
		read_struct_field(__node, node);
		return;
	} else if (next_token_is(INCR) || next_token_is(DECR)) {
		struct token *tok;
		get_back_token(&tok, 0);

		if (is_keyword(tok, INCR))
			ast_uop(OP_INCR, __node, node->_type, node);
		else
			ast_uop(OP_DECR, __node, node->_type, node);
		return;
	}

	*__node = node;
}

void read_cast_type(struct type **__type) {
	struct token *tok;
	read_token(&tok, 1);
	struct type *base_type;
	read_decl_spec(&base_type, tok);

	read_declarator(__type, base_type, NULL, NULL);
}

mdl_u8_t is_type(struct token *__tok) {
	switch(__tok->id) {
		case K_VOID_T:
		case K_X8_T: case K_X16_T:
		case K_X32_T: case K_X64_T:
			return 1;
	}
	return 0;
}

void read_cast_expr(struct node **__node) {
	struct token *tok;
	read_token(&tok, 1);

	if (is_keyword(tok, L_PAREN)) {
		struct token *peek;
		peek_token(&peek);
		if (is_type(peek) || peek->id == K_STRUCT) {
			struct type *_type = NULL;
			read_cast_type(&_type);

			expect_token(TOK_KEYWORD, R_PAREN);

			struct node *node;
			read_cast_expr(&node);

			ast_uop(OP_CAST, __node, _type, node);
			return;
		}
	}

	ulex(tok);

	read_postfix_expr(__node);
}

struct type* usual_arith_conv(struct type *__l, struct type *__r) {
	struct type *_type = bcit_to_type(__l->bcit, (__l->flags&SIGNED) == SIGNED);

	if (bcit_sizeof(__l->bcit) > bcit_sizeof(__r->bcit)) {
		_type = bcit_to_type(__l->bcit, (__l->flags&SIGNED) == SIGNED);
	} else if (bcit_sizeof(__l->bcit) < bcit_sizeof(__r->bcit)) {
		_type = bcit_to_type(__r->bcit, (__r->flags&SIGNED) == SIGNED);
	}
	return _type;
}

void same_arith_conv(struct node **__node, struct type *__type) {
	if ((*__node)->_type->bcit != __type->bcit)
		ast_conv(__node, __type, *__node);
}

void read_multiplicative_expr(struct node **__node) {
	read_cast_expr(__node);

	mdl_u8_t op;
	if (next_token_is(ASTERISK))
		op = OP_MUL;
	else
		return;

	struct node *l = *__node, *r = NULL;
	read_multiplicative_expr(&r);
	struct type *_type = usual_arith_conv(l->_type, r->_type);
	same_arith_conv(&l, _type);
	same_arith_conv(&r, _type);

	ast_binop(__node, l->_type, op, l, r);
}

void read_additive_expr(struct node **__node) {
	read_multiplicative_expr(__node);

	mdl_u8_t op;
	if (next_token_is(PLUS))
		op = OP_ADD;
	else if (next_token_is(MINUS))
		op = OP_SUB;
	else
		return;

	struct node *l = *__node, *r = NULL;
	read_additive_expr(&r);

	struct type *_type = usual_arith_conv(l->_type, r->_type);
	same_arith_conv(&l, _type);
	same_arith_conv(&r, _type);

	ast_binop(__node, l->_type, op, l, r);
}

void read_eq_or_rela_expr(struct node **__node) {
	read_additive_expr(__node);

	mdl_u8_t op;
	if (next_token_is(EQEQ))
		op = OP_EQ;
	else if (next_token_is(NEQ))
		op = OP_NEQ;
	else if (next_token_is(GT))
		op = OP_GT;
	else if (next_token_is(LT))
		op = OP_LT;
	else if (next_token_is(GEQ))
		op = OP_GEQ;
	else if (next_token_is(LEQ))
		op = OP_LEQ;
	else return;

	struct node *l = *__node, *r = NULL;
	read_expr(&r);

	struct type *_type = usual_arith_conv(l->_type, r->_type);
	same_arith_conv(&l, _type);
	same_arith_conv(&r, _type);

	ast_binop(__node, _type, op, l, r);
}

bcc_err_t read_assign_expr(struct node **__node) {
	struct node *l = NULL;
    read_eq_or_rela_expr(&l);
//	printf("---------------------------------------------------------> %s\n", node == NULL? "NULL":" ");

	struct token *tok;
	read_token(&tok, 1);

	if (is_keyword(tok, EQ)) {
		struct node *r = NULL;
		read_assign_expr(&r);

		if (l->_type->bcit != r->_type->bcit)
			ast_conv(&r, l->_type, r);

		ast_binop(__node, l->_type, OP_ASSIGN, l, r);
		return 0;
	}

	ulex(tok);
	//printf("-----> %s\n", token_id_as_str(tok->id));
	*__node = l;
}

void read_expr(struct node **__node) {
	read_assign_expr(__node);
}

mdl_uint_t read_int_expr() {
	struct token *tok;
	read_token(&tok, 1);

	if (tok->kind == TOK_NO)
		return str_to_int((char*)tok->p);
	else ulex(tok);
	return 0;
}

void read_var_or_func(struct node **__node, char *__name) {
	struct node *node = NULL;

	if (!strcmp(__name, "_print")) {
		ast_func(__node, __name, NULL, NULL, (struct vec){}, NULL);
		return;
	} else if (!strcmp(__name, "extern_call")) {
		struct type *_type;
		make_func_type(&_type, type_8l_u, (struct vec){}, 0);
		ast_func(__node, __name, _type, NULL, (struct vec){}, NULL);
		return;
	}

	node = env_lookup(__name);

	if (node == NULL)
		fprintf(stderr, "failed to get var. error !!!!!!!!!!!!!!!!! -- %s\n", __name);

	*__node = node;
}

void read_arr_initializer(struct vec *__list) {
	expect_token(TOK_KEYWORD, L_BRACE);

	for(;;) {
		struct node **node_p;
		vec_push(__list, (void**)&node_p, sizeof(struct node*));
		read_expr(node_p);

		expect_token(TOK_KEYWORD, COMMA);
		if (next_token_is(R_BRACE)) {
			break;
		}
	}
}

bcc_err_t read_decl_init(struct node **__node, struct type *__type) {
	struct token *tok;
	peek_token(&tok);

	if (is_keyword(tok, L_BRACE) & __type->kind == T_KIND_ARRAY){
		ast_init(__node, __type);
		struct node *node = *__node;
		vec_init(&node->_vec);
		read_arr_initializer(&node->_vec);
		return BCC_SUCCESS;
	}

	struct node *node = NULL;
	read_assign_expr(&node);

	*__node = NULL;
	if (node != NULL) {
		if (is_arith_type(node->_type) && node->_type->bcit != __type->bcit) {
			printf("--------------------------------------:: %d\n", node->_type->kind);
			print_node(node);
			ast_conv(&node, __type, node);
		}

		ast_init(__node, __type);
		add_child((*__node), node);
	}

	return BCC_SUCCESS;
}

void read_array_declarator(struct type **__type, struct type *__base_type) {
	mdl_uint_t len = 0;
	if (!next_token_is(R_BRACKET)) {
		len = read_int_expr();
		expect_token(TOK_KEYWORD, R_BRACKET);
	}

	make_array_type(__type, __base_type, len);
}

struct type* read_func_params(struct vec *__params, struct type *__ret_type) {
	mdl_u8_t ellipsis;
	read_declarator_params(__params, &ellipsis);
	struct type *_type;
	make_func_type(&_type, __ret_type, *__params, ellipsis);
	return _type;
}

void read_declarator(struct type **__type, struct type *__base_type, struct vec *__params, char **__name) {
	if (next_token_is(ASTERISK)) {
		struct type *_type = NULL;
		make_ptr_type(&_type, __base_type);
		read_declarator(__type, _type, __params, __name);
		printf("now ptr type -------------------------------,\n");
		return;
	}

	struct token *tok;
	read_token(&tok, 1);
	if (tok->kind == TOK_IDENT) {
		*__name = tok->p;
		print_token(tok);
		*__type = __base_type;
		printf("var name: %s\n", *__name);
	} else ulex(tok);

	if (next_token_is(L_BRACKET)) {
		read_array_declarator(__type, __base_type);
		return;
	}

	if (next_token_is(L_PAREN)) {
		*__type = read_func_params(__params, __base_type);
		return;
	}

	*__type = __base_type;
}

bcc_err_t read_decl(struct vec *__vec, mdl_u8_t __is_local) {
	struct node *var = NULL, *init = NULL;
	struct type *base_type = NULL;
	struct type *_type = NULL;
	struct node **node;
	mdl_u8_t bca = 0;
	struct token *_tok;
	read_token(&_tok, 1);
	bca = _tok->bca;
	read_decl_spec(&base_type, _tok);

	if (next_token_is(SEMICOLON))return 0;

	struct token *tok;

	for (;;) {
		init = NULL;
		var = NULL;
		_type = NULL;
		char *name = NULL;

		peek_token(&tok); // debug
		read_declarator(&_type, base_type, NULL, &name);
		printf("------- dest _ type: %d\n", _type->bcit);

		ast_var(&var, _type, name);

		map_put(__is_local? local_env:&global_env, (mdl_u8_t*)name, strlen(name), var);
		bca_env_add_var_int(name, RG_16A);

		printf("name ----- %s\n", name);

		if(next_token_is(EQ))
			read_decl_init(&init, _type);

		vec_push(__vec, (void**)&node, sizeof(struct node*));
		ast_decl(node, var, init);

//		printf("-------------------------> name: %s\n", name);

		if (next_token_is(COMMA)) continue;
		expect_token(TOK_KEYWORD, SEMICOLON);
		break;
	}

	return BCC_SUCCESS;
}


bcc_err_t read_exit_stmt(struct token *__tok, struct node **__node) {
	if (!next_token_is(SEMICOLON)) {
		fprintf(stderr, "expected ending token.\n");
		return BCC_FAILURE;
	}

	ast_exit(__node);
}

void read_funcdef(struct node **__node) {
	struct type *base_type = NULL;
	struct type *_type = NULL;
	struct token *tok;
	read_token(&tok, 1);

	read_decl_spec(&base_type, tok);

	struct vec params;
	vec_init(&params);

	struct map env;
	map_init(&env);
	map_set_parent(&env, &global_env);
	local_env = &env;
	struct node *enode;

	char *name = NULL;
	read_declarator(&_type, base_type, &params, &name);
	curr_ret_type = _type->ret_type;

	struct node *body = NULL;
	read_stmt(&body);

	struct node *label = NULL;
	ast_label(&label, name);

	ast_func(__node, name, _type, body, params, label);
	map_put(&global_env, (mdl_u8_t const*)name, strlen(name), *__node);

	local_env = NULL;
	map_de_init(&env);
}

mdl_u8_t is_funcdef() {
	struct vec vec;
	vec_init(&vec);

	struct token *tok;
	mdl_u8_t func = 0;
	for (;;) {
		struct token **_tok;
		read_token(&tok, 1);

		vec_push(&vec, (void**)&_tok, sizeof(struct token*));
		*_tok = tok;

		if (is_keyword(tok, SEMICOLON)) break;
		if (is_keyword(tok, EQ)) break;

		if (is_keyword(tok, L_PAREN)) {
			func = 1;
			break;
		}
	}

	struct token **itr = (struct token**)vec_end(&vec);
	while(itr != NULL) {
		ulex(*itr);
		print_token(*itr);
		vec_itr((void**)&itr, VEC_ITR_UP, 1);
	}

	return func;
}

bcc_err_t parse(struct vec *__vec) {
	if (node_itr > (struct node*)buff_end(&node_buff)) {
		fprintf(stderr, "node buff overflow!!!!!!\n");
	}

	struct node **node;

	struct token *tok;
	peek_token(&tok);

	printf("-------------------------------------------------------------------------\n");
	print_token(tok);
	if (tok->kind == TOK_KEYWORD) {
		if ((tok->id >= K_X8_T && tok->id <= K_X64_T) || tok->id == K_STRUCT || tok->id == K_VOID_T) {
			if (is_funcdef()) {
				vec_push(__vec, (void**)&node, sizeof(struct node*));
				read_funcdef(node);

				return BCC_SUCCESS;
			}

			if (read_decl(__vec, 0) != BCC_SUCCESS) {
				fprintf(stderr, "read_decl failed.\n");
			} else {
				printf("read_decl success.\n");
			}

			return BCC_SUCCESS;
		}

		switch(tok->id) {
			case K_IF: case K_EXIT: case K_WHILE: case COLON: case K_GOTO: case K_TYPEDEF: case ASTERISK: case L_BRACE: case L_PAREN: case K_BCA:
				vec_push(__vec, (void**)&node, sizeof(struct node*));
				read_stmt(node);
			break;
		}
	} else if (tok->kind == TOK_IDENT) {
		vec_push(__vec, (void**)&node, sizeof(struct node*));
		read_stmt(node);
	} else {
		// skip unknown tokens e.g. invalid toks
		printf("skiped token as it unknown.\n");
		print_token(tok);
		skip_token();
	}
}
