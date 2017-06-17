# include "bcc.h"
bci_addr_t addr = 0;
void incr_addr() {addr++;}

bcc_err_t open_file(char const *__fpth, int *__fd, struct stat *__st, int __flags, mode_t __mode) {
	if ((*__fd = open(__fpth, __flags, __mode)) < 0) {
		fprintf(stderr, "failed to open file at '%s'\n", __fpth);
		return BCC_FAILURE;
	}

	if (stat(__fpth, __st) < 0) {
		close(*__fd);
		fprintf(stderr, "failed to stat file at '%s'\n", __fpth);
		return BCC_FAILURE;
	}
	return BCC_SUCCESS;
}

void bcb_put_w8(mdl_u8_t **__bcb_point, mdl_u8_t __val) {
	**__bcb_point = __val;
	(*__bcb_point)++;
	incr_addr();
}

void bcb_put_w16(mdl_u8_t **__bcb_point, mdl_u16_t __val) {
	bcb_put_w8(__bcb_point, __val);
	bcb_put_w8(__bcb_point, __val >> 8);
}

mdl_u8_t bcb_get_w8(mdl_u8_t *__bc_buff, bci_addr_t __addr) {
	return *(__bc_buff+__addr);}


void bcb_put(mdl_u8_t **__bcb_point, mdl_u8_t *__val, mdl_uint_t __bc) {
	for (mdl_u8_t *itr = __val; itr != __val+__bc; itr++) bcb_put_w8(__bcb_point, *itr);}

void bcb_addr_put(mdl_u8_t **__bcb_point, bci_addr_t __addr) {
	bcb_put(__bcb_point, (mdl_u8_t*)&__addr, sizeof(bci_addr_t));}

void bci_assign(mdl_u8_t **__bcb_point, bci_addr_t __addr, mdl_u8_t *__val, mdl_u8_t __type) {
	bcb_put_w8(__bcb_point, _bcii_assign);
	bcb_put_w8(__bcb_point, __type);
	bcb_addr_put(__bcb_point, __addr);
	bcb_put(__bcb_point, __val, bci_sizeof(__type));
}

void bci_mov(mdl_u8_t **__bcb_point, mdl_u8_t __dest_type, mdl_u8_t __src_type, bci_addr_t __dest_addr, bci_addr_t __src_addr) {
	bcb_put_w8(__bcb_point, _bcii_mov);

	bcb_put_w8(__bcb_point, __dest_type);
	bcb_put_w8(__bcb_point, __src_type);

	bcb_addr_put(__bcb_point, __dest_addr);
	bcb_addr_put(__bcb_point, __src_addr);
}

void bci_print(mdl_u8_t **__bcb_point, mdl_u8_t __type, bci_addr_t __addr) {
	bcb_put_w8(__bcb_point, _bcii_print);
	bcb_put_w8(__bcb_point, __type);
	bcb_addr_put(__bcb_point, __addr);
}

void bci_exit(mdl_u8_t **__bcb_point) {
	bcb_put_w8(__bcb_point, _bcii_exit);
}

mdl_uint_t str_to_no(mdl_u8_t *__str, mdl_u8_t *__is_no) {
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
			default:
				if (__is_no != NULL) *__is_no = 0;
				return 0;
		};

		if (no_unit != 1) no_unit = no_unit/10;
	}
	if (__is_no != NULL) *__is_no = 1;
	return no;
}

struct obj_context_t {
	mdl_u8_t type;
	void *name, *addr;
	mdl_uint_t name_len;
};

struct obj_context_t* objc_indx = NULL;
mdl_uint_t obj_c = 1;

mdl_u8_t *src = NULL, *bc_buff = NULL;
mdl_u8_t *bcb_point;
struct stat src_fst;
bcc_err_t bcc_init(struct bcc_t *__bcc) {
	bcc_err_t any_err;
	int fd;
	if ((any_err = open_file(__bcc-> src_fpth, &fd, &src_fst, O_RDONLY, 0)) != BCC_SUCCESS) return any_err;

	src = (mdl_u8_t*)malloc(src_fst.st_size);
	read(fd, src, src_fst.st_size);
	close(fd);

	bc_buff = (mdl_u8_t*)malloc(128);
	bcb_point = bc_buff;
	objc_indx = (struct obj_context_t*)malloc(sizeof(struct obj_context_t*));
	return BCC_SUCCESS;
}

struct obj_context_t *obj_put(struct obj_context_t **__objc_indx, mdl_uint_t *__obj_c) {
	*__objc_indx = (struct obj_context_t*)realloc(*__objc_indx, (++(*__obj_c))*sizeof(struct obj_context_t));
	(*__objc_indx+*__obj_c-1)-> name = NULL;
	(*__objc_indx+*__obj_c-1)-> addr = NULL;
	return (*__objc_indx+*__obj_c-2);
}

void obj_init(struct obj_context_t *__obj_context, mdl_u8_t *__name, mdl_u16_t __addr, mdl_u8_t *__tmp_buff) {
	mdl_uint_t name_len = __name - __tmp_buff;
	__obj_context-> name = malloc(name_len*sizeof(mdl_u8_t));
	strncpy((mdl_u8_t*)__obj_context-> name, __tmp_buff, name_len);
	__obj_context-> name_len = name_len - 1;

	__obj_context-> addr = malloc(sizeof(mdl_u16_t));
	*(mdl_u16_t*)__obj_context-> addr = __addr;
}

struct obj_context_t *obj_lookup(struct obj_context_t *__objc_indx, mdl_uint_t __obj_c, char const *__name) {
	mdl_uint_t name_len = strlen(__name);
	for (struct obj_context_t *itr = __objc_indx; itr != __objc_indx+(__obj_c-1); itr++) {
		if (itr-> name_len == name_len) if (!strcmp(itr-> name, __name)) return itr;}
	return NULL;
}

bcc_err_t bcc_de_init() {
	free(src);
	free(bc_buff);
}

mdl_u8_t no_data(mdl_u8_t __val) {
	if (__val == ' ' || __val == '\n' || __val == '\0') return 1;
	return 0;
}

mdl_u8_t is_str_eq(char const *__str, mdl_u8_t **__src_itr, mdl_u8_t *__src, mdl_uint_t __st_size, mdl_u8_t __ndc) {
	mdl_uint_t str_len = 0;
	for (mdl_u8_t *itr = (char*)__str;; itr++)
		if (*itr != '\0') str_len++; else break;

	if (((*__src_itr)-__src)+str_len > __st_size) return 0;
	for (mdl_u8_t *itr = *__src_itr; itr != (*__src_itr)+str_len; itr++)
		if (*itr != *(__str++)) return 0;

	if (__ndc)
		if (!no_data(*((*__src_itr)+str_len))) return 0;

	*__src_itr += str_len - 1;
	return 1;
}

mdl_u8_t out_buff[128];

typedef struct {
	mdl_u8_t l_chr, r_chr;
} op_sign_t;

op_sign_t op_signs[] = {
	{.l_chr='=', .r_chr='='}
};

op_sign_t op_sign_chr(mdl_u8_t __l_chr, mdl_u8_t __r_chr) {
	op_sign_t _op_sign = {
		.l_chr = __l_chr,
		.r_chr = __r_chr
	};
	return _op_sign;
}

mdl_u8_t wait_for(op_sign_t __op_sign, mdl_u8_t **__src_itr, mdl_u8_t *__src, mdl_uint_t __st_size) {
	if ((*__src_itr)-__src > __st_size) {
		fprintf(stderr, "error.\n");
		return 0;
	}

	(*__src_itr)++;
	if (__op_sign.l_chr != '\0') {
		if (__op_sign.r_chr != '\0') {
			if (**__src_itr == __op_sign.l_chr && *((*__src_itr)+1) == __op_sign.r_chr) return 0;
		} else
			if (**__src_itr == __op_sign.l_chr) return 0;
	} else return 0;

	return 1;
}

mdl_u8_t is_op_sign(op_sign_t __op_sign) {
	for (op_sign_t *itr = op_signs; itr != op_signs + sizeof(op_signs)/sizeof(op_sign_t); itr++)
		if (__op_sign.l_chr == itr-> l_chr && __op_sign.r_chr == itr-> r_chr) return 1;
	return 0;
}

bcc_err_t bcc_dump_bc(char const *__fpth) {
	bcc_err_t any_err;
	int fd;
	struct stat st;
	if ((any_err = open_file(__fpth, &fd, &st, O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO)) != BCC_SUCCESS) return any_err;

	write(fd, bc_buff, bcb_point-bc_buff);
	close(fd);
	return BCC_SUCCESS;
}

mdl_u8_t tmp_buff[128];
mdl_u8_t *tmp_ptr = NULL;

# define CSEC_MODE 0b10000000
bcc_err_t bcc_run(struct bcc_t *__bcc) {
	mdl_u8_t *itr = src;
	mdl_u8_t rt_flgs = 0b00000000;
	do {
		if (is_str_eq("\x2f\x2a", &itr, src, src_fst.st_size, 0)) bcc_tog_flg(&rt_flgs, CSEC_MODE);
		if (is_str_eq("\x2a\x2f", &itr, src, src_fst.st_size, 0)) {bcc_tog_flg(&rt_flgs, CSEC_MODE);itr++;printf("exit\n");}
		if (bcc_is_flg(rt_flgs, CSEC_MODE)) {goto end;}

		if (is_str_eq((tmp_ptr = "w8"), &itr, src, src_fst.st_size, 1) || is_str_eq((tmp_ptr = "w16"), &itr, src, src_fst.st_size, 1) ||
			is_str_eq((tmp_ptr = "w32"), &itr, src, src_fst.st_size, 1) || is_str_eq((tmp_ptr = "w64"), &itr, src, src_fst.st_size, 1)) {
			mdl_u8_t type;
			if (!strcmp(tmp_ptr, "w8"))
				type = _bcit_w8;
			else if (!strcmp(tmp_ptr, "w16"))
				type = _bcit_w16;
			else if (!strcmp(tmp_ptr, "w32"))
				type = _bcit_w32;
			else if (!strcmp(tmp_ptr, "w64"))
				type = _bcit_w64;

			op_sign_t op_sign = op_sign_chr('=', '\0');

			mdl_u8_t *name = tmp_buff, no_val = 0;
			while(wait_for(op_sign, &itr, src, src_fst.st_size)) {
				if (*itr == ';') {
					if (itr-src == 0) {
						fprintf(stderr, "an object requires a name.\n");
						return BCC_FAILURE;
					} else {
						no_val = 1;
						break;
					}
				}

				if (!no_data(*itr)) {*name = *itr; name++;}
			}

			*name = '\0';
			name++;

			printf("name: %s\n", tmp_buff);
			if (obj_lookup(objc_indx, obj_c, tmp_buff) != NULL) {
				fprintf(stderr, "can't have same named objects '%s'\n", tmp_buff);
				return BCC_FAILURE;
			}

			struct obj_context_t *obj = obj_put(&objc_indx, &obj_c);
			obj_init(obj, name, addr, tmp_buff);
			obj-> type = type;

			if (!no_val) {
				mdl_u8_t *val = tmp_buff;
				op_sign = op_sign_chr(';', '\0');
				while(wait_for(op_sign, &itr, src, src_fst.st_size)) {
					if (!no_data(*itr)) {*val = *itr; val++;}}
				*val = '\0';
				val++;

				mdl_u8_t is_no = 0;
				mdl_uint_t no =  str_to_no(tmp_buff, &is_no);
				if (is_no)
					bci_assign(&bcb_point, addr, (mdl_u8_t*)&no, type);
				else {
					if ((obj = obj_lookup(objc_indx, obj_c, tmp_buff)) == NULL) {
						fprintf(stderr, "obj lookup failed to find '%s'\n", tmp_buff);
						return BCC_FAILURE;
					}

					bci_mov(&bcb_point, type, obj->type, addr, *(bci_addr_t*)obj->addr);
				}

				printf("w8 found, name: %s, val: %d, type: %u\n", (char*)obj->name, str_to_no(tmp_buff, NULL), type);
			}
		} else if (is_str_eq("print", &itr, src, src_fst.st_size, 1)) {
			mdl_u8_t *name = tmp_buff;
			op_sign_t op_sign = op_sign_chr(';', '\0');
			while(wait_for(op_sign, &itr, src, src_fst.st_size)) {
				if (!no_data(*itr)) {*name = *itr; name++;}}
			*name = '\0';
			name++;

			struct obj_context_t *obj;
			if ((obj = obj_lookup(objc_indx, obj_c, tmp_buff)) == NULL) {
				fprintf(stderr, "obj lookup failed to find '%s'\n", tmp_buff);
				return BCC_FAILURE;
			}

			bci_print(&bcb_point, obj->type, *(bci_addr_t*)obj->addr);
		} else if (is_str_eq("incr", &itr, src, src_fst.st_size, 1)) {
		} else if (is_str_eq("decr", &itr, src, src_fst.st_size, 1)) {
		} else if (is_str_eq("exit", &itr, src, src_fst.st_size, 1))
			bci_exit(&bcb_point);

		end:
		itr++;
	} while(itr != src+src_fst.st_size && itr < src+src_fst.st_size);
}

mdl_u8_t bcc_is_flg(bcc_flg_t __flgs, bcc_flg_t __flg) {
	if ((__flgs & __flg) == __flg) return 1; return 0;}

void bcc_tog_flg(bcc_flg_t *__flgs, bcc_flg_t __flg) {
	if (bcc_is_flg(*__flgs, __flg))
		(*__flgs) ^= __flg;
	else
		(*__flgs) |= __flg;
}
