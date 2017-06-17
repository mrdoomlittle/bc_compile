# include "bc_compile.h"
mdl_u8_t no_data(mdl_u8_t __val);
mdl_u8_t is_str(char const *__str, mdl_u8_t *__file_point, mdl_uint_t *__ic, mdl_uint_t __st_size) {
	mdl_uint_t str_len = strlen(__str);
	if (*__ic + str_len > __st_size) return 1;

	for (mdl_u8_t *itor = __file_point; itor != __file_point + str_len; itor++)
		if (*itor != *(__str++)) return 1;
	if (!no_data(*(__file_point+str_len))) return 1;

	*__ic += str_len;
	return 0;
}

mdl_u8_t wait_until(mdl_u8_t __byte, mdl_u8_t **__file_point, mdl_u8_t *__file, mdl_uint_t *__ic, mdl_uint_t __st_size) {
	if (*__ic > __st_size) return 0;
	*__file_point = __file + *__ic;
	(*__ic)++;
	if (**__file_point == __byte) return 0;
	return 1;
}

mdl_uint_t str_to_no(mdl_u8_t *__str, mdl_u8_t *__is_no) {
	mdl_uint_t no_unit = 1, no = 0;
	for (mdl_uint_t ic = 0; ic != strlen(__str) - 1; ic++)
		no_unit = no_unit*10;

	for (mdl_u8_t *itor = __str;; itor++) {
		if (*itor == '\0') break;
		switch(*itor) {
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

mdl_uint_t bci_sizeof(mdl_u8_t __t) {
	switch(__t) {
		case _bci_w8:
			return sizeof(mdl_u8_t);
		case _bci_w16:
			return sizeof(mdl_u16_t);
	}
	return 0;
}

mdl_u16_t next_saddr = 2;

# define DREG_W8 0x0
void bcb_put_w8(mdl_u8_t **__bcb_point, mdl_u8_t __val) {
	**__bcb_point = __val;
	(*__bcb_point)++;
	next_saddr++;
}

mdl_u8_t bcb_get_w8(mdl_u8_t *__bc_buff, mdl_u16_t __addr) {
	return *(__bc_buff+__addr);
}

void bcb_put_w16(mdl_u8_t **__bcb_point, mdl_u16_t __val) {
	bcb_put_w8(__bcb_point, __val);
	bcb_put_w8(__bcb_point, __val >> 8);
}

void bci_push(mdl_u8_t **__bcb_point) {
   // bcb_put_w8(__bcb_point, _bci_push);
}

void bci_mov(mdl_u8_t **__bcb_point, mdl_u16_t __dest_addr, mdl_u16_t __src_addr, mdl_u8_t __t) {
	bcb_put_w8(__bcb_point, _bci_mov);
	bcb_put_w8(__bcb_point, __t);
	bcb_put_w16(__bcb_point, __dest_addr);
	bcb_put_w16(__bcb_point, __src_addr);
}

void bci_assign(mdl_u8_t **__bcb_point, mdl_u16_t __addr, void *__val, mdl_u8_t __t) {
	bcb_put_w8(__bcb_point, _bci_assign);
	bcb_put_w8(__bcb_point, __t);

	bcb_put_w16(__bcb_point, __addr);

	for (mdl_u8_t ic = 0; ic != bci_sizeof(__t); ic++)
		bcb_put_w8(__bcb_point, (*(mdl_u8_t*)__val) >> ic*8);
}

void bci_cmp(mdl_u8_t **__bcb_point, mdl_u16_t __addr_l, mdl_u16_t __addr_r, mdl_u8_t __t) {
	bcb_put_w8(__bcb_point, _bci_cmp);
	bcb_put_w8(__bcb_point, __t);

	bcb_put_w16(__bcb_point, __addr_l);
	bcb_put_w16(__bcb_point, __addr_r);
	bcb_put_w16(__bcb_point, DREG_W8);
}

void bci_jne(mdl_u8_t **__bcb_point, mdl_u16_t __addr) {
	bcb_put_w8(__bcb_point, _bci_jne);
	bcb_put_w16(__bcb_point, __addr);
	bcb_put_w16(__bcb_point, DREG_W8);
}

void bci_je(mdl_u8_t **__bcb_point, mdl_u16_t __addr) {
	bcb_put_w8(__bcb_point, _bci_je);
	bcb_put_w16(__bcb_point, __addr);
	bcb_put_w16(__bcb_point, DREG_W8);
}

void bci_print(mdl_u8_t **__bcb_point, mdl_u16_t __addr, mdl_u8_t __t) {
	bcb_put_w8(__bcb_point, _bci_print);
	bcb_put_w8(__bcb_point, __t);
	bcb_put_w16(__bcb_point, __addr);
}

void bci_incr(mdl_u8_t **__bcb_point, mdl_u16_t __addr, mdl_u8_t __t) {
	bcb_put_w8(__bcb_point, _bci_incr);
	bcb_put_w8(__bcb_point, __t);
	bcb_put_w16(__bcb_point, __addr);
}

void bci_jmp(mdl_u8_t **__bcb_point, mdl_u16_t __addr) {
	bcb_put_w8(__bcb_point, _bci_jmp);
	bcb_put_w16(__bcb_point, __addr);
}

struct obj_context_t {
	mdl_u8_t type;
	void *name, *addr;
	mdl_uint_t name_len;
};

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
	for (struct obj_context_t *itor = __objc_indx; itor != __objc_indx + (__obj_c-1); itor++) {
		if (itor-> name_len == name_len)
			if (!strcmp(itor-> name, __name)) return itor;
	}

	return NULL;
}

void reg_setup(struct obj_context_t **__objc_indx, mdl_uint_t *__obj_c) {
	struct obj_context_t *obj_context = obj_put(__objc_indx, __obj_c);
	obj_context-> type = _bci_w16;
	char *name = "sp";
	obj_init(obj_context, name+strlen(name)+1, next_saddr, name);
}

mdl_u8_t no_data(mdl_u8_t __val) {
	if (__val == ' ' || __val == '\n') return 1;
	return 0;
}

mdl_u8_t is_optflag(mdl_u8_t __optflags, mdl_u8_t __optflag) {
	if ((__optflags & __optflag) == __optflag) return 1; return 0;}

void tog_optflag(mdl_u8_t *__optflags, mdl_u8_t __optflag) {
	if (is_optflag(*__optflags, __optflag))
		(*__optflags) ^= __optflag;
	else
		(*__optflags) |= __optflag;
}

mdl_i8_t open_file(char const *__fpth, int *__fd, struct stat *__st, int __flags, mode_t __mode) {
	if ((*__fd = open(__fpth, __flags, __mode)) < 0) {
		fprintf(stderr, "failed to open file at '%s'\n", __fpth);
		return -1;
	}

	if (stat(__fpth, __st) < 0) {
		close(*__fd);
		fprintf(stderr, "failed to stat file at '%s'\n", __fpth);
		return -1;
	}
	return 0;
}

# define IF_BLOCK 0x0
# define WHILE_BLOCK 0x1
struct block_t {
	mdl_u8_t type;
	mdl_u16_t jmp_addr;
	struct obj_context_t obj[2];
	mdl_u8_t op_sign;
};


mdl_u8_t op_signs[] = {
	'#', '&'
};

mdl_u8_t is_op_sign(mdl_u8_t __val) {
	for (mdl_u8_t *itor = op_signs; itor != op_signs + sizeof(op_signs); itor++)
		if (__val == *itor) return 1;
	return 0;
}

mdl_u8_t _wait_for(mdl_u8_t *__list, mdl_uint_t __list_size, mdl_u8_t **__file_point, mdl_u8_t *__file, mdl_uint_t *__ic, mdl_uint_t __st_size, mdl_u8_t *__o) {
	if (*__ic > __st_size) return 0;
	*__file_point = __file + *__ic;
	(*__ic)++;

	for (mdl_u8_t *itor = __list; itor != __list + __list_size; itor++)
		if (**__file_point == *itor) {if (__o != NULL){*__o = *itor;}return 1;}
	return 0;
}

# define COMMENT_MODE_FLG 0b10000000
int main(int argc, char const *argv[]) {
	if (argc < 3) {
		fprintf(stderr, "usage: ./bc_compile 'src file' 'dest file'\n");
		return -1;
	}

	int src_fd, dest_fd;
	struct stat src_fst, dest_fst;
	char const *src_fpth = argv[1], *dest_fpth = argv[2];

	if ((src_fd = open(src_fpth, O_RDONLY)) < 0) {
		fprintf(stderr, "failed to open file at '%s'\n", src_fpth);
		return -1;
	}

	if (stat(src_fpth, &src_fst) < 0) {
		close(src_fd);
		fprintf(stderr, "failed to stat file at '%s'\n", src_fpth);
		return -1;
	}

	mdl_u8_t *file = (mdl_u8_t*)malloc(src_fst.st_size);
	read(src_fd, file, src_fst.st_size);
	close(src_fd);

	if ((dest_fd = open(dest_fpth, O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO)) < 0) {
		fprintf(stderr, "failed to open file at '%s'\n", dest_fpth);
		return -1;
	}

	struct obj_context_t* objc_indx = (struct obj_context_t*)malloc(sizeof(struct obj_context_t*));
	mdl_uint_t obj_c = 1;

	mdl_u8_t *bc_buff = (mdl_u8_t*)malloc(128);
	mdl_u8_t *bcb_point = bc_buff;

	mdl_u8_t tmp_buff[128];
	void *tmp_ptr = NULL;

	mdl_u8_t infunc = 0, fsec_end = 0, jmp_addr = 0;

	struct block_t block_buff[128];
	mdl_uint_t block_depth = 0, bc = 0;

	mdl_u8_t flags = 0;

	//reg_setup(&objc_indx, &obj_c);

	for (mdl_uint_t ic = 0; ic != src_fst.st_size; ic ++) {
		mdl_u8_t *file_point = file + ic;

		if (!is_str("\x2f\x2a", file_point, &ic, src_fst.st_size)) tog_optflag(&flags, COMMENT_MODE_FLG);
		if (!is_str("\x2a\x2f", file_point, &ic, src_fst.st_size)) tog_optflag(&flags, COMMENT_MODE_FLG);
		if (is_optflag(flags, COMMENT_MODE_FLG)) continue;

		if (!is_str(":", file_point, &ic, src_fst.st_size)) {
			mdl_u8_t *name = tmp_buff;
			while(wait_until(';', &file_point, file, &ic, src_fst.st_size)) {
				if (!no_data(*file_point)) {*name = *file_point;name++;}}

			*name = '\0';
			name++;

			struct obj_context_t *obj_context = obj_put(&objc_indx, &obj_c);
			obj_init(obj_context, name, next_saddr, tmp_buff);
		}

		if (!is_str("goto", file_point, &ic, src_fst.st_size)) {
			mdl_u8_t *name = tmp_buff;
			while(wait_until(';', &file_point, file, &ic, src_fst.st_size)) {
				if (!no_data(*file_point)) {*name = *file_point;name++;}}

			*name = '\0';
			name++;

			struct obj_context_t *obj;
			if ((obj = obj_lookup(objc_indx, obj_c, tmp_buff)) == NULL) {
				fprintf(stderr, "bc_compile: failed to find.\n");
				return -1;
			}

			bci_jmp(&bcb_point, *(mdl_u16_t*)obj-> addr);
		}

		if (!is_str("inc", file_point, &ic, src_fst.st_size)) {
		}


		if (!is_str((tmp_ptr = "w8"), file_point, &ic, src_fst.st_size) || !is_str((tmp_ptr = "w16"), file_point, &ic, src_fst.st_size)) {
			mdl_u8_t t = 0;

			if (!strcmp(tmp_ptr, "w8")) {
				t = _bci_w8;}
			else if (!strcmp(tmp_ptr, "w16")) {
				t = _bci_w16;}

			for (mdl_u8_t _ic = 0; _ic != bci_sizeof(t); _ic++)
				bci_push(&bcb_point);

			mdl_u8_t *name = tmp_buff;
			while(wait_until('=', &file_point, file, &ic, src_fst.st_size)) {
				if (*file_point == ';') {
					file_point--;
					ic--;
					break;
				}

				if (!no_data(*file_point)) {
					*name = *file_point;
					name++;
				}
			}

			*name = '\0';
			name++;

			if (obj_lookup(objc_indx, obj_c, tmp_buff) != NULL) {
				fprintf(stderr, "bc_compile: same name vars.\n");
				return -1;
			}

			struct obj_context_t *obj_context = obj_put(&objc_indx, &obj_c);
			obj_init(obj_context, name, next_saddr, tmp_buff);
			obj_context-> type = t; // set type

			mdl_u8_t *val = tmp_buff;
			mdl_u8_t no_val = 1;
			while(wait_until(';', &file_point, file, &ic, src_fst.st_size)) {
				if (!no_data(*file_point)) {
					*val = *file_point;
					val++;
					no_val = 0;
				}
			}

			if (!no_val) {
				*val = '\0';
				val++;

				mdl_u8_t is_no = 0;
				mdl_u8_t val_no = str_to_no(tmp_buff, &is_no);
				printf("-----> '%s'\n", tmp_buff);
				if (is_no)
					bci_assign(&bcb_point, next_saddr, &val_no, t);
				else {
					printf("hello\n");
					struct obj_context_t *obj;
					if ((obj = obj_lookup(objc_indx, obj_c, tmp_buff)) == NULL) {
						fprintf(stderr, "bc_compile: failed to find.\n");
						return -1;
					}

					if (t != obj-> type) {
						fprintf(stderr, "types dont match.\n");
						return -1;
					}

					bci_mov(&bcb_point, next_saddr, *(mdl_u16_t*)obj-> addr, t);
				}

//				printf("%d\n", str_to_no(tmp_buff));
			}
		}

		if (!is_str("incr", file_point, &ic, src_fst.st_size)) {
			mdl_u8_t *name = tmp_buff;
			while(wait_until(';', &file_point, file, &ic, src_fst.st_size)) {
				if (!no_data(*file_point)) {
					*name = *file_point;
					name++;
				}
			}

			*name = '\0';
			name++;

			struct obj_context_t *obj;
			if ((obj = obj_lookup(objc_indx, obj_c, tmp_buff)) == NULL) {
				fprintf(stderr, "failed to find. %s\n", tmp_buff);
				return;
			}

			bci_incr(&bcb_point, *(mdl_u16_t*)obj-> addr, obj-> type);
		}

		if (!is_str("print", file_point, &ic, src_fst.st_size)) {
			mdl_u8_t *name = tmp_buff;
			while(wait_until(';', &file_point, file, &ic, src_fst.st_size)) {
				if (!no_data(*file_point)) {
					*name = *file_point;
					name++;
				}
			}

			*name = '\0';
			name++;

			struct obj_context_t *obj;
			if ((obj = obj_lookup(objc_indx, obj_c, tmp_buff)) == NULL) {
				fprintf(stderr, "failed to find. %s\n", tmp_buff);
				return -1;
			}

			bci_print(&bcb_point, *(mdl_u16_t*)obj-> addr, obj-> type);
		}

		if (!is_str("if", file_point, &ic, src_fst.st_size)) {
			struct obj_context_t *obj_l, *obj_r;
			mdl_u8_t *name_l = tmp_buff, op_sign = '\0';

			while(!_wait_for(op_signs, sizeof(op_signs), &file_point, file, &ic, src_fst.st_size, &op_sign)) {
				if (!no_data(*file_point)) {
					*name_l = *file_point;
					name_l++;
				}
			}

			*name_l = '\0';
			name_l++;

			if ((obj_l = obj_lookup(objc_indx, obj_c, tmp_buff)) == NULL) {
				fprintf(stderr, "failed to find. %s\n", tmp_buff);
				return -1;
			}

			mdl_u8_t *name_r = tmp_buff;
			while(!_wait_for("{", 1, &file_point, file, &ic, src_fst.st_size, NULL)) {
				if (!no_data(*file_point)) {
					*name_r = *file_point;
					name_r++;
				}
			}

			*name_r = '\0';
			name_r++;

			if ((obj_r = obj_lookup(objc_indx, obj_c, tmp_buff)) == NULL) {
				fprintf(stderr, "failed to find. %s\n", tmp_buff);
				return -1;
			}

			if (obj_l-> type != obj_r-> type) {
				fprintf(stderr, "bc_compile: can't compare diffrent data type.\n");
				return -1;
			}

			bci_cmp(&bcb_point, *(mdl_u16_t*)obj_l-> addr, *(mdl_u16_t*)obj_r-> addr, obj_l-> type);

			if (op_sign == '&')
				bci_jne(&bcb_point, 0x0);
			else if (op_sign == '#')
				bci_je(&bcb_point, 0x0);

			struct block_t *block = block_buff+block_depth;
			block-> jmp_addr = next_saddr - 2;
			block-> type = IF_BLOCK;
			block_depth++;
		}

		if (!is_str("while", file_point, &ic, src_fst.st_size)) {
			struct obj_context_t *obj_l, *obj_r;
			mdl_u8_t *name_l = tmp_buff, op_sign = '\0';

			while(!_wait_for(op_signs, sizeof(op_signs), &file_point, file, &ic, src_fst.st_size, &op_sign)) {
				if (!no_data(*file_point)) {
					*name_l = *file_point;
					name_l++;
				}
			}

			*name_l = '\0';
			name_l++;

			if ((obj_l = obj_lookup(objc_indx, obj_c, tmp_buff)) == NULL) {
				fprintf(stderr, "failed to find. %s\n", tmp_buff);
				return -1;
			}

			mdl_u8_t *name_r = tmp_buff;
			while(!_wait_for("{", 1, &file_point, file, &ic, src_fst.st_size, NULL)) {
				if (!no_data(*file_point)) {
					*name_r = *file_point;
					name_r++;
				}
			}

			*name_r = '\0';
			name_r++;

			if ((obj_r = obj_lookup(objc_indx, obj_c, tmp_buff)) == NULL) {
				fprintf(stderr, "failed to find. %s\n", tmp_buff);
				return -1;
			}

			struct block_t *block = block_buff+block_depth;
			block-> type = WHILE_BLOCK;
			block-> obj[0] = *obj_l;
			block-> obj[1] = *obj_r;
			block-> op_sign = op_sign;
			block-> jmp_addr = next_saddr;
			block_depth++;
		}

		if (block_depth > 0) {
			if (*file_point == '}') {
				struct block_t *block = block_buff+(block_depth-1);
				switch(block-> type) {
					case IF_BLOCK:
						*(mdl_u16_t*)(bc_buff+block-> jmp_addr) = next_saddr;
						printf("block depth: %d, addr: %u\n", block_depth, block-> jmp_addr);
					break;
					case WHILE_BLOCK:
						bci_cmp(&bcb_point, *(mdl_u16_t*)block-> obj[0].addr, *(mdl_u16_t*)block-> obj[1].addr, block-> obj[0].type);
						if (block-> op_sign == '&')
							bci_jne(&bcb_point, block-> jmp_addr);
						else if(block-> op_sign == '#')
							bci_je(&bcb_point, block-> jmp_addr);
					break;
				}
				block_depth--;


			}
		}


//		if (infunc && *file_point == '}') {
//			*(mdl_u16_t*)(bc_buff+jmp_addr) = next_saddr;
//			infunc = 0;
//		}
/*
		if (ic == fsec_end && infunc) {
			*(mdl_u16_t*)(bc_buff+jmp_addr) = next_saddr;
			infunc = 0;
		}
*/
	}

	*bcb_point = _bci_exit;
	bcb_point++;

	printf("\n");
	for (int x = 0; x != next_saddr+12; x ++) {
		printf("----> %u\n", bc_buff[x]);
 	}

	for (mdl_uint_t ic = 0; ic != obj_c - 1; ic ++) {
		struct obj_context_t *obj_context = objc_indx+ic;
		printf("name: %s,   addr: %u, len: %u0\n", (char*)obj_context->name, *(mdl_u16_t*)obj_context->addr, obj_context->name_len);
		if (obj_context-> addr != NULL)
			free(obj_context-> addr);
		if (obj_context-> name != NULL)
			free(obj_context-> name);
	}

	free(objc_indx);

	write(dest_fd, bc_buff, bcb_point-bc_buff);

	free(bc_buff);
	free(file);

	close(dest_fd);
}

