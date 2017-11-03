# include "bcc.h"
bcc_err_t open_file(struct file_t *__file) {
	if ((__file->fd = open(__file->path, __file->flags,__file->mode)) < 0) {
		fprintf(stderr, "failed to open file '%s'\n", __file->path);
		return BCC_FAILURE;
	}

	if (stat(__file->path, &__file->st) < 0) {
		close(__file->fd);
		fprintf(stderr, "failed to stat file '%s'\n", __file->path);
		return BCC_FAILURE;
	}

	return BCC_SUCCESS;
}

mdl_uint_t fsize(struct file_t *__file) {
	return __file->st.st_size;
}

struct file_t src_file;
struct buff file_buff;

mdl_i8_t open_src_file(char *__src_fpth) {
	bcc_err_t any_err;
	src_file.path = __src_fpth;
	src_file.flags = O_RDONLY;
	src_file.mode = 0;

	printf("file -> %s\n", src_file.path);
	if ((any_err = open_file(&src_file)) != BCC_SUCCESS) return any_err;
	printf("file size: %u\n", fsize(&src_file));

	src_buff = (mdl_u8_t*)malloc(fsize(&src_file));
	src_itr = src_buff;

	read(src_file.fd, src_buff, fsize(&src_file));
	close(src_file.fd);
	return BCC_SUCCESS;
}

struct map macros;
struct vec type_defs;
struct vec static* inc_paths;
bcc_err_t bcc_init(struct bcc *__bcc, struct vec *__inc_paths) {
	if (access(__bcc->src_fpth, R_OK) < 0) {
		fprintf(stderr, "bcc, source file does not exist, errno: %d\n", errno);
		return BCC_FAILURE;
	}

	if (!access(__bcc->dst_fpth, F_OK) && access(__bcc->dst_fpth, W_OK) < 0) {
		fprintf(stderr, "bcc, don't have write access to destination file, errno: %d\n", errno);
		return BCC_FAILURE;
	}

	buff_init(&file_buff, sizeof(struct _file_t), 20);
	open_src_file((char*)__bcc->src_fpth);
	vec_init(&type_defs);
	map_init(&macros);
	inc_paths = __inc_paths;
	return BCC_SUCCESS;
}

void byte_ncpy(mdl_u8_t *__dst, mdl_u8_t *__src, mdl_uint_t __n) {
	mdl_u8_t *itr = __src;
	while(itr != __src+__n) {
		mdl_uint_t off = itr-__src;
		mdl_uint_t br = __n-off;
		if (br >> 3 > 0) {
			*((mdl_u64_t*)(__dst+off)) = *(mdl_u64_t*)itr;
			itr += sizeof(mdl_u64_t);
		} else if (br >> 2 > 0) {
			*((mdl_u32_t*)(__dst+off)) = *(mdl_u32_t*)itr;
			itr += sizeof(mdl_u32_t);
		} else if (br >> 1 > 0) {
			*((mdl_u16_t*)(__dst+off)) = *(mdl_u16_t*)itr;
			itr += sizeof(mdl_u16_t);
		} else
			*((mdl_u8_t*)__dst+off) = *(itr++);
	}
}

mdl_uint_t hex_to_int(char *__hex) {
	char *itr = __hex;
	mdl_uint_t l = strlen(__hex);
	char c;
	mdl_uint_t val = 0;
	while(itr++ != __hex+l) {
		c = *(itr-1);
		if (c >= '0' && c <= '9')
			val = (val<<4)|((c-'0')&0xF);
		else if (c >= 'A' && c <= 'F')
			val = (val<<4)|(((c-'A')+10)&0xF);
	}
	return val;
}

mdl_uint_t str_to_int(char *__str) {
	mdl_uint_t no_unit = 1, no = 0;
	for (mdl_uint_t ic = 0;; ic++) {
		if (__str[ic+1] == '\0') break;
		no_unit = no_unit*10;
	}

	for (mdl_u8_t *itr = __str;; itr++) {
		if (*itr == '\0') break;
		switch(*itr) {
			case '0': break;
			case '1': no += 1*no_unit; break;
			case '2': no += 2*no_unit; break;
			case '3': no += 3*no_unit; break;
			case '4': no += 4*no_unit; break;
			case '5': no += 5*no_unit; break;
			case '6': no += 6*no_unit; break;
			case '7': no += 7*no_unit; break;
			case '8': no += 8*no_unit; break;
			case '9': no += 9*no_unit; break;
			default: return 0;
		}

		if (no_unit != 1) no_unit = no_unit/10;
	}
	return no;
}

bcc_err_t bcc_de_init() {
	free(src_buff);
	buff_free(&file_buff);
//	vec_de_init(&type_defs);
	map_de_init(&macros);
}

# define init_token(__tok) \
	*__tok = (struct token){ \
		.kind = TOK_INVALID, \
		.id = ID_INVALID, \
		.p = NULL, \
		.val = NULL \
	};

mdl_u8_t maybe_keyword(struct token*);
char* strcmb(char *__a, char *__b) {
	mdl_uint_t a_len = strlen(__a);
	mdl_uint_t b_len = strlen(__b);

	char *nstr = malloc(a_len+b_len+1);
	byte_ncpy(nstr, __a, a_len);
	byte_ncpy(nstr+a_len, __b, b_len);
	*(nstr+a_len+b_len) = '\0';
	return nstr;
}

mdl_u8_t* mem_dupe(mdl_u8_t *__p, mdl_uint_t __bc) {
	mdl_u8_t *ret = (mdl_u8_t*)malloc(__bc);
	byte_ncpy(ret, __p, __bc);
	return ret;
}

char *str_dupe(char *__str) {
	return (char*)mem_dupe(__str, strlen(__str)+1);
}

void read_include() {
	mdl_u8_t std = 0;
	char *fpth = read_header_fpth(&std);

	struct _file_t file = {
		.file = src_file,
		.itr = src_itr,
		.p = src_buff
	};

	printf("push point: %u\n", src_itr-src_buff);

	// push current file into buffer
	buff_push(&file_buff, &file);

	if (!std) {
		if (!access(fpth, R_OK))
			open_src_file(fpth);
		else
			fprintf(stderr, "bcc, don't have access to file '%s', errno: %d\n", fpth, errno);
		return;
	}

	char **itr = (char**)vec_begin(inc_paths);
	for (;itr != NULL; vec_itr((void**)&itr, VEC_ITR_DOWN, 1)) {
		char *fr;
		char *_fpth = strcmb((fr = strcmb(*itr, "/")), fpth);

		if (!access(_fpth, R_OK)) {
			open_src_file(_fpth);
			free(fr);
			free(_fpth);
			return;
		} else
			fprintf(stderr, "bcc, don't have access to file '%s', errno: %d\n", fpth, errno);
	}
}

void read_define() {
	struct token *name;
	read_token(&name, 1);

	struct vec *toks = (struct vec*)malloc(sizeof(struct vec));
	memset(toks, 0, sizeof(struct vec));
	vec_init(toks);

	struct token *tok = read_token(NULL, 0);
	while (tok->kind != TOK_NEWLINE) {
		if (is_keyword(tok, BACKSLASH)) {
			while(tok->kind != TOK_NEWLINE)
				read_token(&tok, 0);
			read_token(&tok, 0);
		}

		struct token **_tok = NULL;
		vec_push(toks, (void**)&_tok, sizeof(struct token*));
		*_tok = tok;

		read_token(&tok, 0);
	}

	map_put(&macros, (char*)name->p, strlen((char*)name->p), (void*)toks);
}

void read_macro() {
	struct token *tok;
	read_token(&tok, 1);
	if (tok->kind != TOK_IDENT) return;
	if (!strcmp((char*)tok->p, "include")) read_include();
	if (!strcmp((char*)tok->p, "define")) read_define();
}

struct token *tmp_tok;
struct token* read_token(struct token **__tok, mdl_u8_t __sk_nl) {
	if (!__tok) __tok = &tmp_tok;

	for (;;) {
		lex(__tok);
		if ((*__tok)->kind == TOK_NEWLINE && __sk_nl) continue;
		if ((*__tok)->kind == TOK_IDENT && (*__tok)->p != NULL) {
			struct vec *toks = NULL;
			map_get(&macros, (char*)(*__tok)->p, strlen((char*)(*__tok)->p), (void**)&toks);

			if (toks != NULL) {
				struct token *tok = NULL;
				struct token **itr = (struct token**)vec_end(toks);
				while(itr != NULL) {
					if (itr != (struct token**)vec_begin(toks))
						ulex(*itr);
					else
						tok = *itr;

					print_token(*itr);
					vec_itr((void**)&itr, VEC_ITR_UP, 1);
				}

				*__tok = tok;
				break;
			}
		}

		if (is_keyword(*__tok, HASH)) {
			read_macro();
		} else break;
	}
	maybe_keyword(*__tok);
	return *__tok;
}

struct token* peek_token(struct token **__tok) {
	if (!__tok) __tok = &tmp_tok;
	read_token(__tok, 1);
	ulex(*__tok);
	return *__tok;
}

void to_keyword(struct token *__tok, mdl_u8_t __id) {
	__tok->id = __id;
	__tok->kind = TOK_KEYWORD;
}

mdl_u8_t is_ident(struct token *__tok, char *__s, mdl_u8_t __off) {
	return !strcmp((char*)__tok->p+__off, __s);
}

bcc_err_t expect_token(mdl_u8_t __kind, mdl_u8_t __id) {
	struct token *tok;
	read_token(&tok, 1);
	if (tok->kind == __kind && tok->id == __id)
		return BCC_SUCCESS;

	ulex(tok);
	fprintf(stderr, "expected error, kind: %s, id: %s\n", token_kind_as_str(__kind), token_id_as_str(__id));
	return BCC_FAILURE;
}

struct type_def {
	struct token *type;
	char *name;
};

void read_typedef() {
	struct token *type;
	struct token *name;

	read_token(&type, 1);
	read_token(&name, 1);

	struct type_def *_type_def;
	vec_push(&type_defs, (void**)&_type_def, sizeof(struct type_def));
	*_type_def = (struct type_def) {
		.type = type,
		.name = (char*)name->p
	};
	expect_token(TOK_KEYWORD, SEMICOLON);
}

mdl_u8_t maybe_keyword(struct token *__tok) {
	if (__tok->kind != TOK_IDENT) return 1;
	if (*(mdl_u32_t*)__tok->p == 0x5F616362) {
		__tok->p = (void*)((mdl_u8_t*)__tok->p+sizeof(mdl_u32_t));
		if (!maybe_keyword(__tok)) return 0;
		__tok->bca = 1;
		return 1;
	}

	if (is_ident(__tok, "if", 0))
		to_keyword(__tok, K_IF);
	else if (is_ident(__tok, "8_t", 1))
		to_keyword(__tok, K_X8_T);
	else if (is_ident(__tok, "16_t", 1))
		to_keyword(__tok, K_X16_T);
	else if (is_ident(__tok, "32_t", 1))
		to_keyword(__tok, K_X32_T);
	else if (is_ident(__tok, "64_t", 1))
		to_keyword(__tok, K_X64_T);
	else if (is_ident(__tok, "void", 0))
		to_keyword(__tok, K_VOID_T);
	else if (is_ident(__tok, "struct", 0))
		to_keyword(__tok, K_STRUCT);
	else if (is_ident(__tok, "return", 0))
		to_keyword(__tok, K_RETURN);
	else if (is_ident(__tok, "_exit", 0))
		to_keyword(__tok, K_EXIT);
	else if (is_ident(__tok, "while", 0))
		to_keyword(__tok, K_WHILE);
	else if (is_ident(__tok, "goto", 0))
		to_keyword(__tok, K_GOTO);
	else if (is_ident(__tok, "else", 0))
		to_keyword(__tok, K_ELSE);
	else if (is_ident(__tok, "typedef", 0))
		to_keyword(__tok, K_TYPEDEF);
	else if (is_ident(__tok, "bca", 0))
		to_keyword(__tok,  K_BCA);
	else if (is_ident(__tok, "sizeof", 0))
		to_keyword(__tok,  K_SIZEOF);
	else if (is_ident(__tok, "va_ptr", 0))
		to_keyword(__tok,  K_VA_PTR);
	else {
		struct type_def *itr = (struct type_def*)vec_begin(&type_defs);
		for (;itr != NULL; vec_itr((void**)&itr, VEC_ITR_DOWN, 1)) {
			if (!strcmp(itr->name, (char*)__tok->p))
				to_keyword(__tok, itr->type->id);
		}
		return 0;
	}
	return 1;
}

mdl_u8_t *src_buff = NULL;
mdl_u8_t *src_itr = NULL;
bcc_err_t bcc_run(struct bcc *__bcc) {
	lex_init(12000, 240);
	parse_init(12000);
	gen_init(12000);

	struct vec ni;
	vec_init(&ni);

	_next_file:
	do {
		printf("--| file: %s -------- %u %u - %u\n", src_file.path, src_itr-src_buff, fsize(&src_file), ib_empty());
		parse(&ni);
	} while((src_itr < src_buff+fsize(&src_file)) || !ib_empty());

	printf("ib empty: %s, s: %d/%d\n", ib_empty()? "yes":"no", src_itr-src_buff, fsize(&src_file));
	struct token *tok;
	read_token(&tok, 1);
	print_token(tok);

	if (buff_len(&file_buff) > 0) {
		struct _file_t file;
		buff_pop(&file_buff, &file);
		printf("loading file: %s\n", file.file.path);

		src_file = file.file;
		src_buff = file.p;
		src_itr = file.itr;

		printf("current point: %u,%u\n", src_itr-src_buff, fsize(&src_file));
		goto _next_file;
	}

	struct node **itr = (struct node**)vec_begin(&ni);
	for (;itr != NULL; vec_itr((void**)&itr, VEC_ITR_DOWN, 1)) {
		if (gen(*itr) != BCC_SUCCESS) break;
		print_node(*itr);
	}

	link_gotos();

	struct file_t file = {
		.path = __bcc->dst_fpth,
		.flags = O_WRONLY|O_CREAT,
		.mode = S_IRWXU|S_IRWXG|S_IRWXO
	};

	open_file(&file);

	gen_apply_sp_val();

	set_output_file(&file);
	write_output_file();
	close_output_file();

	gen_de_init();
	lex_de_init();
	parse_de_init();
	vec_de_init_all();
}
