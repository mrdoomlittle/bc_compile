# include "bcc.h"
mdl_u8_t *to_free[212];
mdl_u8_t **tf_itr;

/*
	vec blocks dont get removed but are labeled as free space.
*/

# define PAGE_SIZE 500
void vec_place(struct vec_t*, void*, void*, mdl_uint_t);

void print_blk_desc(struct vec_blk_desc_t *__blk_desc, mdl_i8_t __dir) {
	usleep(100000);
	mdl_u8_t is_root = 0;
	mdl_u8_t static root = 0;
	if (!root) {
		is_root = 1;
		root = 1;
	}

	if (!__blk_desc) return;
	printf("blk_desc:");
	printf("    ptr: %p\n", __blk_desc->p);
	printf("    data: %p\n", __blk_desc->data);
	printf("	size: %u\n", __blk_desc->size);
	printf("    data_size: %u\n", __blk_desc->data_size);
	printf("    in_use: %u\n", __blk_desc->in_use);

	printf("children:\n");

	if (!__blk_desc->upper && !__blk_desc->lower) return;
	printf("{-------------------------------------------------}\n");
	if (__dir > 0) {
		if (__blk_desc->upper != __blk_desc)
			print_blk_desc(__blk_desc->upper, 1);
		else
			fprintf(stderr, "somthing is wrong with the vector block headers.\n");
	}

	if (__dir < 0) {
		if (__blk_desc->lower != __blk_desc)
			print_blk_desc(__blk_desc->lower, -1);
		else
			fprintf(stderr, "somthing is wrong with the vector block headers.\n");
	}

	printf("}-------------------------------------------------{\n");
	if (is_root) {
		printf("\n\n\n");
		root = 0;
	}
}

mdl_u8_t static tf_inited = 0;
void _vec_init(struct vec_t *__vec) {
	if (!tf_inited) {
		tf_itr = to_free;
		tf_inited = 1;
	}

	printf("vec inited.\n");
	__vec->size = 0;
	__vec->pc = 0;
	__vec->free.unused_blks = NULL;
	__vec->last_blk = NULL;
	__vec->linear = 0;

	__vec->p = (mdl_u8_t*)malloc(PAGE_SIZE);
	__vec->itr = __vec->p;
	__vec->pc++;

	__vec->tf = tf_itr;
	*tf_itr = __vec->p;
	tf_itr++;
}

void default_vec(struct vec_t *__vec) {
	__vec->linear = 1;
}

void static add_unused(struct vec_t *__vec, struct vec_blk_desc_t *__blk_desc) {
	vec_push(__vec->free.unused_blks, (void**)&__blk_desc->uul, sizeof(struct vec_blk_desc_t*));
	*__blk_desc->uul = __blk_desc;
}

void static rm_unused(struct vec_t *__vec, struct vec_blk_desc_t **__ptr) {
	struct vec_blk_desc_t **end = ((struct vec_blk_desc_t**)vec_end(__vec->free.unused_blks))-1;
	if (__ptr != end)
		vec_place(__vec, (void*)*__ptr, (void*)*end, sizeof(struct vec_blk_desc_t*));

	vec_pop(__vec->free.unused_blks, NULL, sizeof(struct vec_blk_desc_t*));
}

void vec_init(struct vec_t *__vec) {
	_vec_init(__vec);
	__vec->free.unused_blks = (struct vec_t*)malloc(sizeof(struct vec_t));
	_vec_init(__vec->free.unused_blks);
	default_vec(__vec->free.unused_blks);
}

void* vec_get_data(void *__ptr) {
	return ((struct vec_blk_desc_t*)__ptr)->data;
}

void *vec_upper(void *__ptr) {
	return ((struct vec_blk_desc_t*)__ptr)->upper->p;
}

void *vec_lower(void *__ptr) {
	return ((struct vec_blk_desc_t*)__ptr)->lower->p;
}

void *vec_itr(void *__ptr) {
	return ((struct vec_blk_desc_t*)((mdl_u8_t*)__ptr-sizeof(struct vec_blk_desc_t)))->lower->data;
}

void vec_push(struct vec_t *__vec, void **__data, mdl_uint_t __n) {
	if (!__vec->linear) {
	struct vec_blk_desc_t *_blk_desc = NULL;
	if (vec_size(__vec->free.unused_blks) > 0 && __vec->free.most_bytes >= __n) {
		struct vec_blk_desc_t **itr = (struct vec_blk_desc_t**)vec_get_data(vec_begin(__vec->free.unused_blks));
		struct vec_blk_desc_t **end = (struct vec_blk_desc_t**)vec_end(__vec->free.unused_blks);
		do {
			printf("size: %u / %u\n", (*itr)->data_size, __n);
			if ((*itr)->data_size == __n) {
				_blk_desc = *itr;
				break;
			}

			if ((*itr)->data_size > __n) {
				if (_blk_desc != NULL) {
					if (((*itr)->data_size-__n) < (_blk_desc->data_size-__n)) {
						_blk_desc = *itr;
					}
				} else
					_blk_desc = *itr;
			}

			if (itr != end-1)
				itr=(struct vec_blk_desc_t**)vec_itr(itr);
		} while(itr != end-1);

		printf("size: %u\n", _blk_desc->data_size);

		if (_blk_desc != NULL) {
			/* vec pop link chain */
			if (_blk_desc != __vec->last_blk) {
				_blk_desc->pop = __vec->last_blk;
				__vec->last_blk = _blk_desc;
			}

			// data
			*__data = _blk_desc->data;

			if (_blk_desc->data_size == __n) {
				// if not at end then swap pointers end-???
				rm_unused(__vec, itr);
			} else {
				struct vec_blk_desc_t *a = (struct vec_blk_desc_t*)(_blk_desc->data+__n);
				a->upper = _blk_desc;
				a->lower = _blk_desc->lower;
				_blk_desc->lower = a;

				a->data = _blk_desc->data+__n+sizeof(struct vec_blk_desc_t);
				a->in_use = 0;
				a->data_size = _blk_desc->data_size-__n;
				a->size = a->data_size+sizeof(struct vec_blk_desc_t);
				a->p = a->data+__n;
				_blk_desc->size = __n+sizeof(struct vec_blk_desc_t);
				_blk_desc->data_size = __n;
				_blk_desc->in_use = 1;
				vec_free(__vec, (void*)a->data, a->data_size);
			}

			print_blk_desc(_blk_desc, 1);
			return;
		}
	}
	}

	printf("---\n");
	mdl_u8_t do_resize = 0;

	for (;;) {
		if (__vec->pc*PAGE_SIZE < __vec->size+(__n+sizeof(struct vec_blk_desc_t))) {
			do_resize = 1;
			printf("resize\n");
			__vec->pc++;
		} else break;
	}

	if (do_resize) {
		mdl_uint_t fs = __vec->itr-__vec->p;
		__vec->p = (mdl_u8_t*)realloc(__vec->p, __vec->pc*PAGE_SIZE);
		*__vec->tf = __vec->p;
		__vec->itr = __vec->p+fs;
	}

	struct vec_blk_desc_t *blk_desc = (struct vec_blk_desc_t*)__vec->itr;
	blk_desc->pop = blk_desc->upper = blk_desc->lower = NULL;

	blk_desc->in_use = 1;
	blk_desc->p = __vec->itr;
	blk_desc->size = (blk_desc->data_size = __n)+sizeof(struct vec_blk_desc_t);

	__vec->itr += sizeof(struct vec_blk_desc_t);

	if (__vec->last_blk != NULL) {
		__vec->last_blk->lower = blk_desc;
		blk_desc->upper = __vec->last_blk;
	}

	// for non lin
	blk_desc->pop = __vec->last_blk;

	__vec->last_blk = blk_desc;

	*__data = blk_desc->data = __vec->itr;
	__vec->size += blk_desc->size;
	__vec->itr += __n;
	print_blk_desc(blk_desc, 1);
}

void vec_pop(struct vec_t *__vec, void *__data, mdl_uint_t __n) {
	mdl_u8_t do_resize = 0;

	struct vec_blk_desc_t *blk_desc = __vec->last_blk;
	if (__vec->linear) __vec->last_blk = blk_desc->upper;
	if (!__vec->linear) __vec->last_blk = blk_desc->pop;


	if (blk_desc->data_size != __n) {
		fprintf(stderr, "data size does not match in the size blk desc header, error. got: %u not %u\n", blk_desc->data_size, __n);
		return;
	}

	if (__data != NULL)
		byte_ncpy(__data, blk_desc->data, __n);

	if (!__vec->linear) {
		if (blk_desc->upper != blk_desc->pop) {
			printf("jdjfijdifijdijfij\n");
			vec_free(__vec, blk_desc->data, __n);
			return;
		}
	}

	for (;;) {
		if (__vec->size-blk_desc->size < (__vec->pc-1)*PAGE_SIZE && (__vec->pc-1) >= 1) {
			do_resize = 1;
			printf("no resize\n");
			__vec->pc--;
		} else break;
	}

	mdl_uint_t bytes = blk_desc->size;

	if (do_resize) {
		mdl_uint_t fs = __vec->itr-__vec->p;
		__vec->p = (mdl_u8_t*)realloc(__vec->p, __vec->pc*PAGE_SIZE);
		*__vec->tf = __vec->p;
		__vec->itr = __vec->p+fs;
	}

	__vec->size -= bytes;
    __vec->itr -= bytes;
}

void vec_free(struct vec_t *__vec, void *__data, mdl_uint_t __n) {
	if (__vec->linear) {
		fprintf(stderr, "while linear mode is active this function can't be used.\n");
		return;
	}

	struct vec_blk_desc_t *blk_desc = (struct vec_blk_desc_t*)(((mdl_u8_t*)__data)-sizeof(struct vec_blk_desc_t));
	if (blk_desc->data_size != __n) {
		fprintf(stderr, "data size does not match in the size blk desc header, error.\n");
		return;
	}

	mdl_u8_t rm_finished = 0, found_other = 0;
	for (;;) {
		usleep(100000);
		if (rm_finished == 3) break;
		printf("%d\n", rm_finished);
		if (blk_desc->upper != NULL) {
			if (!blk_desc->upper->in_use) {
				found_other = 1;

				blk_desc = blk_desc->upper;
				blk_desc->size += blk_desc->lower->size;
				blk_desc->data_size += blk_desc->lower->data_size;
				blk_desc->lower = blk_desc->lower->lower;
				printf("found free upper half.\n");
			} else rm_finished |= 1;
		} else rm_finished |= 1;

		if (blk_desc->lower != NULL) {
			if (!blk_desc->lower->in_use) {
				found_other = 0;
				blk_desc->size += blk_desc->lower->size;
				blk_desc->data_size += blk_desc->lower->data_size;

				rm_unused(__vec, blk_desc->lower->uul);

				blk_desc->lower = blk_desc->lower->lower;
				printf("found free lower half.\n");
			} else rm_finished |= 2;
		} else rm_finished |= 2;
	}

	blk_desc->pop = NULL;
	blk_desc->in_use = 0;

	if (blk_desc->data_size > __vec->free.most_bytes)
		__vec->free.most_bytes = blk_desc->data_size;

	printf("---------------------------> %u\n", blk_desc->data_size);

	if (!found_other) {
		printf("dujfidijfijdfjidjijij\n");
		add_unused(__vec, blk_desc);
	}
}

void vec_place(struct vec_t *__vec, void *__dest, void *__src, mdl_uint_t __n) {
	struct vec_blk_desc_t *dest = (struct vec_blk_desc_t*)((mdl_u8_t*)__dest-sizeof(struct vec_blk_desc_t));
	struct vec_blk_desc_t *src = (struct vec_blk_desc_t*)((mdl_u8_t*)__src-sizeof(struct vec_blk_desc_t));
	byte_ncpy(dest->data, src->data, __n);
}

void* vec_get(struct vec_t *__vec, mdl_uint_t __a) {
	return __vec->p+__a;
}

mdl_uint_t vec_size(struct vec_t *__vec) {
	return __vec->size;
}

void *vec_begin(struct vec_t *__vec) {
	return __vec->p;
}

void *vec_end(struct vec_t *__vec) {
	return __vec->itr;
}

void vec_de_init(struct vec_t *__vec) {
	free(__vec->p);
}

void vec_de_init_all() {
	for (mdl_u8_t **itr = to_free; itr != tf_itr; itr++) {
		free(*itr);
	}
}
