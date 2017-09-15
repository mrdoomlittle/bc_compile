# ifndef __bcc__h
# define __bcc__h
# include <sys/stat.h>
# include <string.h>
# include <fcntl.h>
# include <errno.h>
# include <stdint.h>
# include <stdlib.h>
# include <unistd.h>
# include <mdlint.h>
# include <bci.h>
# include <errno.h>
enum {
	TOK_KEYWORD,
	TOK_NO,
	TOK_CHR,
	TOK_INVALID,
	TOK_SPACE,
	TOK_IDENT,
	TOK_NEWLINE,
	TOK_STR
};

# define RG_W8A 0
# define RG_W8B 1
# define RG_W8C 2

# define RG_W16A 3
# define RG_W16B 5
# define RG_W16C 7

# define RG_W32A 9
# define RG_W32B 13
# define RG_W32C 17

# define RG_W64A 21
# define RG_W64B 29
# define RG_W64C 37

# define RG_RET_ADDR 45
# define RG_SP 47
# define RG_BP 49

enum {
	T_KIND_X8,
	T_KIND_X16,
	T_KIND_X32,
	T_KIND_X64,
	T_KIND_PTR,
	T_KIND_ARRAY,
	T_KIND_VOID,
	T_KIND_STRUCT,
	T_KIND_FUNC
};

enum {
	OP_INCR,
	OP_DECR,
	PERIOD,
	OP_EQ,
	OP_NEQ,
	OP_ASSIGN,
	ID_INVALID,
	L_BRACE,
	R_BRACE,
	L_PAREN,
	R_PAREN,
	SEMICOLON,
	COMMA,
	COLON,
	K_X8_T,
	K_X16_T,
	K_X32_T,
	K_X64_T,
	K_VOID_T,
	K_IF,
	K_GOTO,
	K_EXIT,
	K_WHILE,
	OP_PLUS,
	OP_MINUS,
	K_ELSE,
	K_TYPEDEF,
	NOSIGN,
	ASTERISK,
	AMPERSAND,
	K_RETURN,
	L_BRACKET,
	R_BRACKET,
	K_STRUCT,
	OP_ARROW,
	GTS,
	LTS,
	ELLIPSIS,
	K_BCA,
	K_SIZEOF,
	BACKSLASH
};

enum {
	BCA_VAR,
	BCA_PRINT,
	BCA_MOV,
	BCA_ASSIGN,
	BCA_NOP,
	BCA_INCR,
	BCA_DECR,
	BCA_EEB_INIT,
	BCA_EEB_PUT,
	BCA_FLD,
	BCA_FST,
	BCA_DR
};

enum {
	AST_INCR,
	AST_DECR,
	AST_DECL,
	AST_PRINT,
	AST_EXTERN_CALL,
	AST_EXIT,
	AST_LITERAL,
	AST_VAR,
	AST_INIT,
	AST_ASSIGN,
	AST_ADD,
	AST_SUB,
	AST_IF,
	AST_GOTO,
	AST_LABEL,
	AST_WHILE,
	AST_OP_EQ,
	AST_OP_NEQ,
	AST_COMPOUND_STMT,
	AST_FUNC_CALL,
	AST_FUNC,
	AST_DEREF,
	AST_ADDROF,
	AST_RETURN,
	AST_CONV,
	OP_CAST,
	AST_STRUCT_REF,
	AST_BCA
};

# define BCC_SUCCESS 0
# define BCC_FAILURE -1
typedef mdl_u8_t bcc_flag_t;

struct bcc {
	char const *src_fpth, *dst_fpth;
	bcc_flag_t flgs;
};

// type flags
# define SIGNED ('i' & 0xC)
# define UNSIGNED ('u' & 0xC)

typedef mdl_i8_t bcc_err_t;

struct file_t {
	int fd;
	char const *path;
	int flags;
	mode_t mode;
	struct stat st;
};

struct _file_t {
	struct file_t file;
	mdl_u8_t *itr;
	mdl_u8_t *p;
};

struct token {
	mdl_u8_t kind, id;
	void *p;
	mdl_uint_t bc;
	mdl_u8_t bca;
};

struct buff {
	mdl_u8_t *p;
	mdl_uint_t blk_c, blk_size;
	mdl_uint_t curr_blk;
};

struct vec_blk_desc {
	mdl_uint_t size, data_size;
	mdl_uint_t upper, lower;
	mdl_uint_t above, below;
	mdl_uint_t data, pop, off;
	mdl_u8_t in_use;
	mdl_uint_t uul;
};

struct vec_free {
	mdl_uint_t most_bytes;
	struct vec *unused_blks;
};

# define VEC_ITR_UP 0b10000000
# define VEC_ITR_DOWN 0b01000000
void vec_itr(void**, mdl_u8_t, mdl_uint_t);
struct vec {
	mdl_u8_t **tf;
	mdl_u8_t *p;
	mdl_uint_t pc;
	mdl_uint_t size;
	mdl_u8_t *itr;
	mdl_uint_t first_blk, last_blk;
	struct vec_free free;
};

struct map {
	struct vec **table;
	struct map *parent;
};

struct type {
	mdl_u8_t kind, bcit;
	mdl_uint_t size, len;
	mdl_uint_t offset;
	mdl_u8_t flags, hva;
	struct type *ptr, *ret_type;
	struct map *fields;
	struct vec params;
};

struct pair_t {
	void *first, *second;
};

struct node {
	mdl_u8_t kind;
	mdl_u8_t val[8];
	void *p;

	struct type *_type;

	struct node *child_buff[12];
	struct node **child_itr;

	struct vec params, args;

	struct node *l, *r;

	struct vec _vec;

	bci_addr_t off, bc;
};

struct bca_token {
	mdl_u8_t kind;
	void *p;
};

struct bca_blk {
	mdl_u8_t kind, bcit, flags;
	mdl_u8_t dst_bcit, src_bcit;
	bci_addr_t addr, dst_addr, src_addr;
	mdl_uint_t val, bc;
	char *name;
	bci_addr_t b_addr, e_addr;
	mdl_u8_t eeb_c, eeb_id;
	void *p;
};

// bcc.c
struct buff extern file_buff;

// buff.c
void buff_init(struct buff*, mdl_uint_t, mdl_uint_t);
void buff_push(struct buff*, void*);
void buff_pop(struct buff*, void*);
void buff_put(struct buff*, mdl_uint_t, void*);
void buff_get(struct buff*, mdl_uint_t, void*);
void* buff_begin(struct buff*);
void* buff_end(struct buff*);
mdl_uint_t buff_blk_size(struct buff*);
mdl_uint_t buff_blk_c(struct buff*);
void buff_free(struct buff*);
mdl_uint_t buff_len(struct buff*);

// map.c
void map_init(struct map*);
void map_put(struct map*, mdl_u8_t const*, mdl_uint_t, void*);
void map_get(struct map*, mdl_u8_t const*, mdl_uint_t, void**);
void map_de_init(struct map*);
void map_set_parent(struct map*, struct map*);
struct map* map_get_parent(struct map*);
mdl_u8_t is_keyword(struct token*, mdl_u8_t);

// src control
struct file_t extern src_file;
mdl_u8_t extern*src_buff;
mdl_u8_t extern*src_itr;

# define incr_src_itr() src_itr++;
mdl_uint_t fsize(struct file_t*);

// debug.c
void print_token(struct token*);
void print_node(struct node*);
char const* token_kind_as_str(mdl_u8_t);
char const* token_id_as_str(mdl_u8_t);
char const* bca_blk_kind_as_str(mdl_u8_t);

// file stuff
void set_output_file(struct file_t*);
void close_output_file();
void write_output_file();

void read_typedef();
mdl_u8_t ib_empty();
void link_gotos();

// gen.c
bcc_err_t gen(struct node*);
bcc_err_t gen_init(mdl_uint_t);
bcc_err_t gen_de_init();
void gen_apply_sp_val();

// vec.c
void *vec_begin(struct vec*);
void *vec_end(struct vec*);
mdl_uint_t vec_size(struct vec*);
void vec_init(struct vec*);
void vec_push(struct vec*, void**, mdl_uint_t);
void vec_pop(struct vec*, void*, mdl_uint_t);
void* vec_first(struct vec*);
void* vec_last(struct vec*);
void vec_free(struct vec*, void*, mdl_u8_t);

void* vec_get(struct vec*, mdl_uint_t);
void vec_de_init(struct vec*);
void vec_de_init_all();

struct token* read_token(struct token**, mdl_u8_t);
struct token* peek_token(struct token**);

// lex.c
bcc_err_t lex_init(mdl_uint_t, mdl_uint_t);
bcc_err_t ulex(struct token*);
bcc_err_t lex(struct token**);
bcc_err_t lex_de_init();
char* read_header_fpth(mdl_u8_t*);
void get_token(struct token **, mdl_uint_t);
void get_back_token(struct token**, mdl_u8_t);
void read_bca_token(struct bca_token**, char**);
// parse.c
bcc_err_t parse(struct vec *);
bcc_err_t parse_init(mdl_uint_t);
bcc_err_t parse_de_init();

// bcc.c
bcc_err_t expect_token(mdl_u8_t, mdl_u8_t);
void byte_ncpy(mdl_u8_t*, mdl_u8_t*, mdl_uint_t);

mdl_uint_t str_to_int(char*);

bcc_err_t open_file(struct file_t*);
bcc_err_t bcc_init(struct bcc*, struct vec*);
bcc_err_t bcc_de_init();
bcc_err_t bcc_run(struct bcc*);
bcc_err_t bcc_dump_bc(char const*);

mdl_u8_t bcc_is_flag(bcc_flag_t, bcc_flag_t);
void bcc_tog_flag(bcc_flag_t*, bcc_flag_t);
mdl_u8_t* mem_dupe(mdl_u8_t*, mdl_uint_t);
char* str_dupe(char*);
# endif /*__bcc__h*/
