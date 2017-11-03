# include "bcc.h"
mdl_u8_t *to_free[400];
mdl_u8_t **tf_itr;

# define PAGE_SIZE 20
mdl_u8_t static tf_inited = 0;
void _vec_init(struct vec *__vec) {
	if (!tf_inited) {
		tf_itr = to_free;
		tf_inited = 1;
	}

	printf("vec inited.\n");
	__vec->size = 0;
	__vec->pc = 0;
	__vec->last_blk = 0;
	__vec->free.unused_blks = NULL;
	__vec->free.most_bytes = 0;
	__vec->first_blk = 0;

	__vec->p = (mdl_u8_t*)malloc(PAGE_SIZE);
	__vec->itr = __vec->p;
	__vec->pc++;
	__vec->blk_c = 0;

	__vec->tf = tf_itr;
	*tf_itr = __vec->p;
	tf_itr++;
}

void print_blk_desc(struct vec_blk_desc *__blk_desc) {
	printf("blk_desc:\n");
	printf("	data: %u\n", __blk_desc->data);
	printf("	size: %u\n", __blk_desc->size);
	printf("	data_size: %u\n", __blk_desc->data_size);
}

void vec_init(struct vec *__vec) {
	_vec_init(__vec);
	__vec->free.unused_blks = (struct vec*)malloc(sizeof(struct vec));
	_vec_init(__vec->free.unused_blks);
}

mdl_uint_t vec_get_off(struct vec *__vec, mdl_u8_t *__ptr) {
	return __ptr-__vec->p;
}

struct vec_blk_desc* get_blk_desc(void *__ptr) {
	return (struct vec_blk_desc*)(__ptr-sizeof(struct vec_blk_desc));
}

mdl_u8_t static _is_flag(mdl_u8_t __flags, mdl_u8_t __flag) {
	return (__flags & __flag) == __flag? 1:0;
}

mdl_uint_t vec_blk_c(struct vec *__vec) {
	return __vec->blk_c;
}

void vec_itr(void **__ptr, mdl_u8_t __flags, mdl_uint_t __ic) {
	while(__ic != 0) {
		struct vec_blk_desc *blk_desc = get_blk_desc(*__ptr);
		mdl_u8_t *p = ((mdl_u8_t*)(*__ptr))-blk_desc->data;

		if (_is_flag(__flags, VEC_ITR_UP)) {
			if (blk_desc->upper & 1)
				*__ptr = (void*)(p+(blk_desc->upper>>1)+sizeof(struct vec_blk_desc));
			else *__ptr = NULL;
		} else if (_is_flag(__flags, VEC_ITR_DOWN)) {
			if (blk_desc->lower & 1)
				*__ptr = (void*)(p+(blk_desc->lower>>1)+sizeof(struct vec_blk_desc));
			else *__ptr = NULL;
		}
		__ic--;
	}
}

void vec_place(struct vec *__vec, void *__dest, void *__src, mdl_uint_t __n) {
	struct vec_blk_desc *dest = (struct vec_blk_desc*)((mdl_u8_t*)__dest-sizeof(struct vec_blk_desc));
	struct vec_blk_desc *src = (struct vec_blk_desc*)((mdl_u8_t*)__src-sizeof(struct vec_blk_desc));
	byte_ncpy(__vec->p+dest->data, __vec->p+src->data, __n);
}

void add_unused(struct vec *__vec, struct vec_blk_desc *__blk_desc) {
	mdl_uint_t *uul;
	vec_push(__vec->free.unused_blks, (void**)&uul, sizeof(mdl_uint_t));
	__blk_desc->uul = vec_get_off(__vec->free.unused_blks, (mdl_u8_t*)uul);
	*uul = vec_get_off(__vec, (mdl_u8_t*)__blk_desc);
}

void rm_unused(struct vec *__vec, mdl_uint_t *__ptr) {
	mdl_uint_t *end = (mdl_uint_t*)vec_end(__vec->free.unused_blks);
	if (__ptr != end) {
//		printf("-------------------------\n");
		vec_place(__vec, (void*)__ptr, (void*)end, sizeof(mdl_uint_t));
	}
	vec_pop(__vec->free.unused_blks, NULL, sizeof(mdl_uint_t));
}

void vec_push(struct vec *__vec, void **__data, mdl_uint_t __n) {
	if (__vec->free.unused_blks != NULL) {
		if (vec_size(__vec->free.unused_blks) > 0 && __vec->free.most_bytes >= __n) {
			mdl_uint_t *_itr = (mdl_uint_t*)vec_begin(__vec->free.unused_blks), *itr_end;
			struct vec_blk_desc *itr = (struct vec_blk_desc*)(__vec->p+(*_itr));
			struct vec_blk_desc *blk_desc = itr;

			while(_itr != NULL) {
				if (itr->data_size == __n) {
					blk_desc = itr;
					itr_end = _itr;
					break;
				}

				if (itr->data_size > __n) {
					if ((itr->data_size-__n) < (blk_desc->data_size-__n))
						blk_desc = itr;
				}

				itr_end = _itr;
				vec_itr((void**)&_itr, VEC_ITR_DOWN, 1);
				if (_itr != NULL)
					itr = (struct vec_blk_desc*)(__vec->p+(*_itr));
			}

			//printf("]]]]]]]]]]]]]]]]]]]]]]]] %u\n", blk_desc->data);
			if (blk_desc != NULL) {
			usleep(1000000);
				blk_desc->upper = blk_desc->lower = 0;
				if (__vec->last_blk&0x1) {
				struct vec_blk_desc *last_blk_desc = (struct vec_blk_desc*)(__vec->p+(__vec->last_blk>>1));
			//	if (blk_desc != last_blk_desc) {
					last_blk_desc->lower = (vec_get_off(__vec, (mdl_u8_t*)blk_desc)<<1)|1;
					blk_desc->upper = __vec->last_blk;
				//	printf("----------------------------------------\n");
			//	}
				}

				__vec->last_blk = (vec_get_off(__vec, (mdl_u8_t*)blk_desc)<<1)|1;
				*__data = (void*)(__vec->p+blk_desc->data);

				rm_unused(__vec, itr_end);
				blk_desc->in_use = 1;
				printf("spare space: %s, data size: %u, %p\n", (blk_desc->data_size > __n)?"yes":"no", blk_desc->data_size, blk_desc);
				if (blk_desc->data_size > __n) {
					print_blk_desc(blk_desc);
					struct vec_blk_desc *spare = (struct vec_blk_desc*)(__vec->p+(blk_desc->data+__n));

					*spare = (struct vec_blk_desc) {
						.above = (vec_get_off(__vec, (mdl_u8_t*)blk_desc)<<1)|1,
						.below = blk_desc->below,
						.upper = 0,
						.lower = 0,
						.data = blk_desc->data+__n+sizeof(struct vec_blk_desc),
						.in_use = 0,
						.data_size = (blk_desc->data_size-__n)-sizeof(struct vec_blk_desc)
					};
					spare->size = blk_desc->data_size-__n;

					if (blk_desc->below&0x1)
						((struct vec_blk_desc*)(__vec->p+(blk_desc->below>>1)))->above = (vec_get_off(__vec, (mdl_u8_t*)spare)<<1)|1;

					blk_desc->below = (vec_get_off(__vec, (mdl_u8_t*)spare)<<1)|1;

					blk_desc->size = __n+sizeof(struct vec_blk_desc);
					blk_desc->data_size = __n;

			//		printf("%u - %u\n", ((struct vec_blk_desc*)(__vec->p+(spare->above>>1)))->in_use, blk_desc->in_use);
					printf("...spare\n");
					print_blk_desc(spare);
					vec_free(__vec, (void*)(__vec->p+spare->data), 0);
				}

				//print_blk_desc(blk_desc);
				return;
			}
		}
	}

	mdl_u8_t do_resize = 0;

	for (;;) {
		if (__vec->pc*PAGE_SIZE < __vec->size+(__n+sizeof(struct vec_blk_desc))) {
			do_resize = 1;
			__vec->pc++;
		} else break;
	}

	if (do_resize) {
		mdl_uint_t fs = __vec->itr-__vec->p;
		__vec->p = (mdl_u8_t*)realloc(__vec->p, __vec->pc*PAGE_SIZE);
		*__vec->tf = __vec->p;
		__vec->itr = __vec->p+fs;
	}

	struct vec_blk_desc *blk_desc = blk_desc = (struct vec_blk_desc*)__vec->itr;
	if (!(__vec->first_blk & 1))
		__vec->first_blk = (vec_get_off(__vec, (mdl_u8_t*)blk_desc))<<1|1;

	blk_desc->above = blk_desc->below = 0;
	blk_desc->upper = blk_desc->lower = 0;
	blk_desc->in_use = 1;
	blk_desc->off = vec_get_off(__vec, (mdl_u8_t*)blk_desc);

	blk_desc->size = __n+sizeof(struct vec_blk_desc);
	blk_desc->data_size = __n;
	if (__vec->last_blk & 1) {
		struct vec_blk_desc *last_blk_desc = (struct vec_blk_desc*)(__vec->p+(__vec->last_blk>>1));
		last_blk_desc->lower = last_blk_desc->below = (vec_get_off(__vec, (mdl_u8_t*)blk_desc)<<1)|1;
		blk_desc->upper = blk_desc->above = __vec->last_blk;
	}

	__vec->last_blk = (vec_get_off(__vec, __vec->itr)<<1)|1;

	__vec->itr += sizeof(struct vec_blk_desc);

	*__data = __vec->itr;
	blk_desc->data = vec_get_off(__vec, __vec->itr);
//	printf("::%u\n", blk_desc->data);

	__vec->size += __n+sizeof(struct vec_blk_desc);
	__vec->itr += __n;
	__vec->blk_c++;
}

void vec_pop(struct vec *__vec, void *__data, mdl_uint_t __n) {
	mdl_u8_t do_resize = 0;

	struct vec_blk_desc *blk_desc = (struct vec_blk_desc*)(__vec->p+(__vec->last_blk>>1));
	__vec->last_blk = blk_desc->upper;

	if (__data != NULL)
		byte_ncpy((mdl_u8_t*)__data, __vec->p+blk_desc->data, __n);

	if ((mdl_u8_t*)blk_desc != (__vec->itr-blk_desc->size)) return;

	for (;;) {
		if (__vec->size-blk_desc->size < (__vec->pc-1)*PAGE_SIZE && (__vec->pc-1) >= 1) {
			do_resize = 1;
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
	__vec->blk_c--;
}

void vec_unchain(struct vec *__vec, struct vec_blk_desc *__blk_desc) {
	struct vec_blk_desc *upper = (struct vec_blk_desc*)(__vec->p+(__blk_desc->upper>>1));
	struct vec_blk_desc *lower = (struct vec_blk_desc*)(__vec->p+(__blk_desc->lower>>1));
	upper->lower = __blk_desc->lower;
	lower->upper = __blk_desc->upper;
}

void vec_free(struct vec *__vec, void *__data, mdl_u8_t __unchain) {
	struct vec_blk_desc *blk_desc = (struct vec_blk_desc*)((mdl_u8_t*)__data-sizeof(struct vec_blk_desc));
	if (vec_get_off(__vec, (mdl_u8_t*)blk_desc) == __vec->first_blk>>1) {
		__vec->first_blk = blk_desc->lower;
	}

	struct vec_blk_desc *above, *upper;
	struct vec_blk_desc *below, *lower;

//	mdl_u8_t finished = 0, found_other = 0;

	if (__unchain)
		vec_unchain(__vec, blk_desc);

	above = (struct vec_blk_desc*)(__vec->p+(blk_desc->above>>1));

	if (blk_desc->above&0x1) {
	if (!above->in_use) {
	while(1) {
		printf("found free upper half, %p, size: %u\n", above, blk_desc->size);

		blk_desc->data_size += above->size;
		blk_desc->above = above->above;
		blk_desc->data = above->data;

		rm_unused(__vec, (mdl_uint_t*)(__vec->free.unused_blks->p+above->uul));

		if (!(blk_desc->above&0x1)) break;
		struct vec_blk_desc *next = (struct vec_blk_desc*)(__vec->p+(blk_desc->above>>1));
		if (next->in_use) break;
		above = next;
	}
	*above = *blk_desc;
	blk_desc = above;
	}
	}

	below = (struct vec_blk_desc*)(__vec->p+(blk_desc->below>>1));
	if (blk_desc->below&0x1) {
	if (!below->in_use) {
	while(1) {
		printf("found free lower half, %p, size: %u\n", below, blk_desc->size);
		blk_desc->data_size += below->size;
		blk_desc->below = below->below;

		rm_unused(__vec, (mdl_uint_t*)(__vec->free.unused_blks->p+below->uul));

		if (!(blk_desc->below&0x1)) break;
		struct vec_blk_desc *next = (struct vec_blk_desc*)(__vec->p+(blk_desc->below>>1));
		if (next->in_use) break;
		below = next;
	}
	}
	}

	mdl_uint_t off = vec_get_off(__vec, (mdl_u8_t*)blk_desc);
	if (blk_desc->above&0x1)
		((struct vec_blk_desc*)(__vec->p+(blk_desc->above>>1)))->below = (off<<1)|1;
	if (blk_desc->below&0x1)
		((struct vec_blk_desc*)(__vec->p+(blk_desc->below>>1)))->above = (off<<1)|1;

//	if (blk_desc != rblk) {
	//	blk_desc->data_size += rblk->data_size;
	//	blk_desc->size += rblk->size;
	//	blk_desc->above = above->above;
//	}
/*
	for (;;) {
		above = (struct vec_blk_desc*)(__vec->p+(blk_desc->above>>1));
		below = (struct vec_blk_desc*)(__vec->p+(blk_desc->below>>1));

		if (finished == 3) break;
		if (!(blk_desc->above & 1))
			finished |= 1;
		else {
			if (!above->in_use) {
				printf("found free upper half.\n");
				found_other = 1;
				above->below = blk_desc->below;
				above->size += blk_desc->size;
				above->data_size += blk_desc->data_size;
				((struct vec_blk_desc*)(__vec->p+(blk_desc->below>>1)))->above = blk_desc->above;
				blk_desc = above;
			} else
				finished |= 1;
		}

		if (!(blk_desc->below & 1))
			finished |= 2;
		else {
			if (!below->in_use) {
				printf("found free lower half.\n");
				blk_desc->size += below->size;
				blk_desc->data_size += below->data_size;
				blk_desc->below = below->below;
				((struct vec_blk_desc*)(__vec->p+(below->below>>1)))->above = below->above;
				rm_unused(__vec, (mdl_uint_t*)(__vec->free.unused_blks->p+below->uul));
			} else
				finished |= 2;
		}
	}
*/
//	vec_unchain(__vec, blk_desc);
	blk_desc->size = blk_desc->data_size+sizeof(struct vec_blk_desc);

	printf("|---------- free ------------|\n");
	print_blk_desc(blk_desc);

	if (blk_desc->data_size > __vec->free.most_bytes)
		__vec->free.most_bytes = blk_desc->data_size;

	blk_desc->in_use = 0;
	printf("unused added, %p\n", blk_desc);
	add_unused(__vec, blk_desc);
}

void* vec_get(struct vec *__vec, mdl_uint_t __a) {
	return __vec->p+__a;
}

mdl_uint_t vec_size(struct vec *__vec) {
	return __vec->size;
}

void* vec_first(struct vec *__vec) {
	if (!(__vec->first_blk & 1)) return NULL;
	return (void*)(__vec->p+(__vec->first_blk>>1)+sizeof(struct vec_blk_desc));
}

void* vec_last(struct vec *__vec) {
	if (!(__vec->last_blk & 1)) return NULL;
	return (void*)(__vec->p+(__vec->last_blk>>1)+sizeof(struct vec_blk_desc));
}

void* _vec_begin(struct vec *__vec) {
	return __vec->p+sizeof(struct vec_blk_desc);
}

void* vec_begin(struct vec *__vec) {
	if (!vec_size(__vec)) return NULL;
	struct vec_blk_desc *blk_desc = get_blk_desc(_vec_begin(__vec));
	for (;;) {
	//	printf("vec_begin: %u\n", blk_desc->in_use);
		if (blk_desc->in_use) break;
		blk_desc = (struct vec_blk_desc*)(__vec->p+(blk_desc->lower>>1));
	}

	return (void*)(__vec->p+blk_desc->data);
}

void* _vec_end(struct vec *__vec) {
	return __vec->p+(__vec->last_blk>>1)+sizeof(struct vec_blk_desc);
}

void* vec_end(struct vec *__vec) {
	if (!vec_size(__vec)) return NULL;
	struct vec_blk_desc *blk_desc = get_blk_desc(_vec_end(__vec));
	for (;;) {
		if (blk_desc->in_use) break;
		blk_desc = (struct vec_blk_desc*)(__vec->p+(blk_desc->upper>>1));
	}

	return (void*)(__vec->p+blk_desc->data);
}

void vec_de_init(struct vec *__vec) {
	free(__vec->p);
	for (mdl_u8_t **itr = to_free; itr != tf_itr; itr++) {
		if (*itr == __vec->p) {
			*itr = *(--tf_itr);
			break;
		}
	}
}

void vec_de_init_all() {
	for (mdl_u8_t **itr = to_free; itr != tf_itr; itr++)
		if (*itr != NULL) free(*itr);
}
