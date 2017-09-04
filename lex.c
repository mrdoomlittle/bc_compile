# include "bcc.h"
struct buff static tok_buff;
struct token static *tok_itr;

mdl_u8_t static is_next(mdl_u8_t);

mdl_u8_t is_white_space() {
	return (*src_itr == ' ' || *src_itr == '\t');
}

void skip_line() {
	for (;;) {
		if (*src_itr == '\n') break;
		else
			incr_src_itr();
	}
}

void skip_comment_block() {
	for (;;) {
		if (*src_itr == '*' && is_next('/')) break;
		else
			incr_src_itr();
	}

	incr_src_itr();
	incr_src_itr();
}

mdl_u8_t ignore_space() {
	if (src_itr >= src_buff+fsize(&src_file)) return 0;
	if (is_white_space()) return 1;

	if (*src_itr == '/') {
		if (is_next('*')) {
			skip_comment_block();
		} else if (is_next('/')) {
			skip_line();
		}
	}

	return 0;
}

# define puti(__a, __b) *__a = __b; __a++;
void static build_token(struct token *__tok, struct token *__tmpl) {
	*__tok = *__tmpl;
}

void static make_chr(struct token *__tok, char *__chr) {
	build_token(__tok, &(struct token){.kind=TOK_CHR, .p=(void*)__chr});}

void static make_keyword(struct token *__tok, mdl_u8_t __id) {
	build_token(__tok, &(struct token){.kind=TOK_KEYWORD, .id=__id});}

void static make_invalid(struct token *__tok) {
	build_token(__tok, &(struct token){.kind=TOK_INVALID});}

void static make_ident(struct token *__tok, char *__s) {
	build_token(__tok, &(struct token){.kind=TOK_IDENT, .p=(void*)__s});}

void static make_no(struct token *__tok, char *__s) {
	build_token(__tok, &(struct token){.kind=TOK_NO, .p=(void*)__s});}

void static make_str(struct token *__tok, char *__s, mdl_uint_t __bc) {
	build_token(__tok, &(struct token){.kind=TOK_STR, .p=(void*)__s, .bc=__bc});}

struct buff static tmp_buff;
char static* read_ident() {
	mdl_u8_t *tb_itr = (mdl_u8_t*)buff_begin(&tmp_buff);
	mdl_u8_t *si = src_itr;
	while((*src_itr >= 'a' && *src_itr <= 'z') || *src_itr == '_' ||(*src_itr >= '0' && *src_itr <= '9')) {
		puti(tb_itr, *src_itr);
		incr_src_itr();
	}

	puti(tb_itr, '\0');

	mdl_uint_t bc = tb_itr-(mdl_u8_t*)buff_begin(&tmp_buff);
	mdl_u8_t *p = (mdl_u8_t*)malloc(bc);
	byte_ncpy(p, (mdl_u8_t*)buff_begin(&tmp_buff), bc);
	return (char*)p;
}

char static* read_no() {
	mdl_u8_t *tb_itr = (mdl_u8_t*)buff_begin(&tmp_buff);
	mdl_u8_t *si = src_itr;
	while(*src_itr >= '0' && *src_itr <= '9') {
		puti(tb_itr, *src_itr);
		incr_src_itr();
	}

	puti(tb_itr, '\0');

	mdl_uint_t bc = tb_itr-(mdl_u8_t*)buff_begin(&tmp_buff);
	mdl_u8_t *p = (mdl_u8_t*)malloc(bc);
	byte_ncpy(p, (mdl_u8_t*)buff_begin(&tmp_buff), bc);
	return (char*)p;
}

mdl_u8_t read_escaped_char(mdl_u8_t __chr) {
	switch(__chr) {
		case '0': return '\0';
		case 'n': return '\n';
	}
	return 0;
}

char static* read_chr() {
	incr_src_itr();
	char *chr = (char*)malloc(sizeof(char));

	if (*src_itr == 0x5c) {
		incr_src_itr();
		*chr = read_escaped_char(*src_itr);
	}

	incr_src_itr();
	incr_src_itr();
	return chr;
}

char static* read_str(mdl_uint_t *__bc) {
	mdl_u8_t *tb_itr = (mdl_u8_t*)buff_begin(&tmp_buff);
	mdl_u8_t *si = src_itr;
	incr_src_itr();

	while(*src_itr != '"') {
		if (*src_itr == 0x5c) {
			incr_src_itr();
			puti(tb_itr, read_escaped_char(*src_itr));
			incr_src_itr();
			continue;
		}

		puti(tb_itr, *src_itr);
		incr_src_itr();
	}

	puti(tb_itr, '\0');
	incr_src_itr();

	mdl_uint_t bc = (tb_itr-(mdl_u8_t*)buff_begin(&tmp_buff));
	mdl_u8_t *p = (mdl_u8_t*)malloc(bc);
	byte_ncpy(p, (mdl_u8_t*)buff_begin(&tmp_buff), bc);
	*__bc = bc+1;
	return p;
}

mdl_u8_t static is_next(mdl_u8_t __chr) {return (*(src_itr+1) == __chr);}

void static _read_token(struct token *__tok) {
	if (ignore_space()) {
		__tok->kind = TOK_SPACE;
		incr_src_itr();
		return;
	}

	if (src_itr >= src_buff+fsize(&src_file)) {
		make_invalid(__tok);
		return;
	}

	switch(*src_itr) {
		case '=':
			make_keyword(__tok, is_next('=')? OP_EQ:OP_ASSIGN);
			if (is_next('=')) incr_src_itr();
			incr_src_itr();
		break;
		case '"': {
			mdl_uint_t bc;
			char *val = read_str(&bc);
			make_str(__tok, val, bc);
			break;
		}
		case '\x27':
			make_chr(__tok, read_chr());
		break;
		case '.':
			if (is_next('.')) {
				if (is_next('.')) {
					make_keyword(__tok, ELLIPSIS);
					incr_src_itr();
					incr_src_itr();
					incr_src_itr();
					break;
				}
			}
			make_keyword(__tok, PERIOD);
			incr_src_itr();
		break;
		case '+':
			if (is_next('+')) {
				make_keyword(__tok, OP_INCR);
				incr_src_itr();
			} else
				make_keyword(__tok, OP_PLUS);
			incr_src_itr();
		break;
		case '-':
			if (is_next('>')) {
				make_keyword(__tok, OP_ARROW);
				incr_src_itr();
			} else if (is_next('-')) {
				make_keyword(__tok, OP_DECR);
				incr_src_itr();
			} else
				make_keyword(__tok, OP_MINUS);
			incr_src_itr();
		break;
		case '[':
			make_keyword(__tok, L_BRACKET);
			incr_src_itr();
		break;
		case ']':
			make_keyword(__tok, R_BRACKET);
			incr_src_itr();
		break;
		case '{':
			make_keyword(__tok, L_BRACE);
			incr_src_itr();
		break;
		case '}':
			make_keyword(__tok, R_BRACE);
			incr_src_itr();
		break;
		case '(':
			make_keyword(__tok, L_PAREN);
			incr_src_itr();
		break;
		case ')':
			make_keyword(__tok, R_PAREN);
			incr_src_itr();
		break;
		case ';':
			make_keyword(__tok, SEMICOLON);
			incr_src_itr();
		break;
		case ':':
			make_keyword(__tok, COLON);
			incr_src_itr();
		break;
		case ',':
			make_keyword(__tok, COMMA);
			incr_src_itr();
		break;
		case '#':
			make_keyword(__tok, NOSIGN);
			incr_src_itr();
		break;
		case '*':
			make_keyword(__tok, ASTERISK);
			incr_src_itr();
		break;
		case '&':
			make_keyword(__tok, AMPERSAND);
			incr_src_itr();
		break;
		case '!':
			if (is_next('=')) {
				make_keyword(__tok, OP_NEQ);
				incr_src_itr();
			}
			incr_src_itr();
		break;
		case '>':
			make_keyword(__tok, GTS);
			incr_src_itr();
		break;
		case '<':
			make_keyword(__tok, LTS);
			incr_src_itr();
		break;
		case '\n': __tok->kind=TOK_NEWLINE; incr_src_itr(); break;
		default:
			if ((*src_itr >= 'a' && *src_itr <= 'z') || *src_itr == '_') {
				make_ident(__tok, read_ident());
				break;
			} else if (*src_itr >= '0' && *src_itr <= '9') {
				make_no(__tok, read_no());
				break;
			}

			make_invalid(__tok);
			incr_src_itr();
			break;
	}
}

char* read_header_fpth(mdl_u8_t *__std) {
	while(ignore_space()) incr_src_itr();

	if (*src_itr == '"' || (*__std = (*src_itr == '<'))) {
		incr_src_itr();

		mdl_u8_t *tb_itr = (mdl_u8_t*)buff_begin(&tmp_buff);
		while(*src_itr != '"' && *src_itr != '>') {
			puti(tb_itr, *src_itr);
			incr_src_itr();
		}

		puti(tb_itr, '\0');

		mdl_uint_t bc = tb_itr-(mdl_u8_t*)buff_begin(&tmp_buff);
		mdl_u8_t *buff = (mdl_u8_t*)malloc(bc);
		byte_ncpy(buff, (mdl_u8_t*)buff_begin(&tmp_buff), bc);

		incr_src_itr();
		printf("----------------------000||||||||||||||||||||||||||||||||||||||||||||||||||||||\ : %u\n", *src_itr);

		return (char*)buff_begin(&tmp_buff);
	}
	return NULL;
}

struct buff static inject_buff;
struct token static**ib_itr = NULL;

bcc_err_t lex_init(mdl_uint_t __tok_buff_size, mdl_uint_t __tok_inject_buff) {
	buff_init(&tok_buff, sizeof(struct token), __tok_buff_size);
	buff_init(&inject_buff, sizeof(struct token*), __tok_inject_buff);
	tok_itr = (struct token*)buff_begin(&tok_buff);
	ib_itr = (struct token**)buff_begin(&inject_buff);
	buff_init(&tmp_buff, sizeof(mdl_u8_t), 200);
}

bcc_err_t lex_de_init() {
	buff_free(&tok_buff);
	buff_free(&inject_buff);
	buff_free(&tmp_buff);
}

bcc_err_t ulex(struct token *__tok) {
	*(ib_itr++) = __tok;
	if (ib_itr > (struct token**)buff_end(&inject_buff))
		fprintf(stderr, "inject buff overflow!!!!!!!\n");
}

void get_token(struct token **__tok, mdl_uint_t __off) {*__tok = tok_itr-__off;}
mdl_u8_t ib_empty() {return (ib_itr == (struct token**)buff_begin(&inject_buff));}

void get_back_token(struct token **__tok, mdl_u8_t __sk_ib) {
	if (!__sk_ib) {
		if (ib_itr == (struct token**)buff_begin(&inject_buff))
			*__tok = *(ib_itr);
		else
			*__tok = *(ib_itr-1);
		return;
	}

	mdl_uint_t off = 1;
	while((*__tok)->kind == TOK_SPACE) {
		get_token(__tok, off);
	}
}

void bca_make_ident(struct bca_token *__tok, char *__s) {
	*__tok = (struct bca_token){.kind=TOK_IDENT, .p=(void*)__s};
}

void bca_make_no(struct bca_token *__tok, char *__s) {
	*__tok = (struct bca_token){.kind=TOK_NO, .p=(void*)__s};
}

mdl_u8_t bca_ignore_space(char __c) {
	if (__c == ' ') return 1;
	return 0;
}

struct bca_token bca_tok_buff[200];
struct bca_token *bca_tok_itr = bca_tok_buff;
char* bca_read_ident(char **__bca_itr) {
	mdl_u8_t *tb_itr = (mdl_u8_t*)buff_begin(&tmp_buff);
	while(((**__bca_itr >= 'a' && **__bca_itr <= 'z') || **__bca_itr == '_') || (**__bca_itr >= '0' && **__bca_itr <= '9')) {
		puti(tb_itr, **__bca_itr);
		(*__bca_itr)++;
	}

	puti(tb_itr, '\0');
	mdl_uint_t bc = tb_itr-(mdl_u8_t*)buff_begin(&tmp_buff);
	mdl_u8_t *p = (mdl_u8_t*)malloc(bc);
	byte_ncpy(p, (mdl_u8_t*)buff_begin(&tmp_buff), bc);
	return (char*)p;
}

char* bca_read_no(char **__bca_itr) {
	mdl_u8_t *tb_itr = (mdl_u8_t*)buff_begin(&tmp_buff);
	while(**__bca_itr >= '0' && **__bca_itr <= '9') {
		puti(tb_itr, **__bca_itr);
		(*__bca_itr)++;
	}

	puti(tb_itr, '\0');
	mdl_uint_t bc = tb_itr-(mdl_u8_t*)buff_begin(&tmp_buff);
	mdl_u8_t *p = (mdl_u8_t*)malloc(bc);
	byte_ncpy(p, (mdl_u8_t*)buff_begin(&tmp_buff), bc);
	return (char*)p;
}

void read_bca_token(struct bca_token **__tok, char **__itr) {
	while(bca_ignore_space(**__itr)) (*__itr)++;
	*__tok = bca_tok_itr++;

	switch(**__itr) {
		default:
			if ((**__itr >= 'a' && **__itr <= 'z') || **__itr == '_') {
				bca_make_ident(*__tok, bca_read_ident(__itr));
				return;
			} else if (**__itr >= '0' && **__itr <= '9') {
				bca_make_no(*__tok, bca_read_no(__itr));
				return;
			}
			bca_tok_itr--;
			*__tok = NULL;
	}
}

bcc_err_t lex(struct token **__tok) {
	if (tok_itr > (struct token*)buff_end(&tok_buff))
		fprintf(stderr, "token buff overflow!!!!!!\n");

	if (ib_itr != (struct token**)buff_begin(&inject_buff)) {
		*__tok = *(--ib_itr);
		return 0;
	}

	struct token *tok = tok_itr++;
	_read_token(tok);

	while(tok->kind == TOK_SPACE) _read_token(tok);
	*__tok = tok;
}
