# include "bcc.h"
# include <malloc.h>
# include <string.h>
typedef struct {
	mdl_u32_t val;
	void *data;
	mdl_uint_t bc;
	mdl_u8_t *key;
} map_entry_t;

mdl_u32_t map_hash(mdl_u8_t const *__key, mdl_uint_t __bc) {
	mdl_u8_t const *itr = __key;
	mdl_u32_t ret_val = 1;
	// mdl_u32_t ret_val = 2<<(__bc>>2);
	while(itr != __key+__bc) {
		// ret_val = (((ret_val>>1)+1)*(ret_val<<2))+(*itr*(((itr-__key)+1)<<1));
		ret_val = (((ret_val>>1)+1)*(ret_val>>1))+ret_val+((*itr*((itr-__key)+1))*(*itr>>1));
		ret_val = ((ret_val+128)^128)*((ret_val>>1)+1);
		itr++;
	}

	return ret_val;
}

map_entry_t* _map_find(struct map *__map, mdl_u8_t const *__key, mdl_uint_t __bc, mdl_u32_t __val) {
	struct vec **map_blk = __map->table+(__val&0xFF);
	map_entry_t *itr;

	if (*map_blk == NULL) {
		if (__map->parent != NULL) goto _search_parent;
		return NULL;
	}

	itr = (map_entry_t*)vec_first(*map_blk);

	if (!vec_blk_c(*map_blk)) {
		if (__map->parent != NULL) goto _search_parent;
		fprintf(stderr, "map table empty, no data to be found.\n");
		return NULL;
	}

	while(itr != NULL) {
		printf("comparing key{%x} with {%x}\n", itr->val, __val);
		if (itr->val == __val && itr->bc == __bc)
			if (!memcmp(itr->key, __key, __bc)) return itr;
		vec_itr((void**)&itr, VEC_ITR_DOWN, 1);
	}

	_search_parent:
	if (__map->parent != NULL) {
		printf("looking for key in parent map.\n");
		return _map_find(__map->parent, __key, __bc, __val);
	}
	return NULL;
}

/*
void **f;
void **f_itr;
mdl_uint_t static ba = 0;
void *f_malloc(mdl_uint_t __bc) {
	if (ba+__bc > 200000000) {
		printf("max memory. %u\n", __bc);
		return NULL;
	}

	ba+=__bc;
	void *p = malloc(__bc);
	*(f_itr++) = p;
	return p;
}

void static map_initr(void *__p) {
	void *p;
	for (mdl_uint_t e = 0; e != 0xFF; e++) {
		*((void**)((mdl_u8_t*)__p+(e*sizeof(void*)))) = (p = (void*)f_malloc((0xFF)*sizeof(void*)));
		memset(p, 0, (0xFF)*sizeof(void*));
	}
}

void static map_initl(void *__p) {
	mdl_u8_t static first = 0;

	mdl_uint_t static c = 0;
	if (c >= MAP_DEPTH_MAX && first) return;
	c++;
	if (!first) first = 1;

	usleep(10000);
	printf("%u\n", c);

	for (mdl_uint_t e = 0; e != 0xFF; e++) {
		void *p = *((void**)((mdl_u8_t*)__p+(e*sizeof(void*))));
		map_initr(p);
		map_initl(p);
		c = 0;
	}

}
*/

void map_init(struct map *__map) {
	__map->table = (struct vec**)malloc(0xFF*sizeof(struct vec*));
	struct vec **itr = __map->table;
	while(itr != __map->table+0xFF) *(itr++) = NULL;
	__map->parent = NULL;

//	__map->r = (void*)f_malloc((0xFF)*sizeof(void*));
//	void *p = __map->r;

//	map_initr(p);
//	map_initl(p);
}

void map_de_init(struct map *__map) {
	struct vec **itr = __map->table;
	while(itr != __map->table+0xFF) {
		if (*itr != NULL) vec_de_init(*itr);
		itr++;
	}

	free(__map->table);
//	vec_de_init(&__map->table);
//	printf("bytes used: %u\n", ba);
//	while(f_itr != f) {
//		free(*f_itr);
//
//		f_itr--;
//	}
//
//	free(f);
}

void map_set_parent(struct map *__map, struct map *__parent) {
	__map->parent = __parent;
}

void map_uset_parent(struct map *__map) {
	__map->parent = NULL;
}

struct map *map_get_parent(struct map *__map){
	return __map->parent;
}

void map_find(struct map *__map, mdl_u8_t const *__key, mdl_uint_t __bc, mdl_u32_t __val, void **__data) {
	*__data = _map_find(__map, __key, __bc, __val)->data;
	if (!*__data) fprintf(stderr, "failed to locate key{%x}.\n", __key);
}

void map_put(struct map *__map, mdl_u8_t const *__key, mdl_uint_t __bc, void *__data) {
	mdl_u32_t val = map_hash(__key, __bc);
	if (_map_find(__map, __key, __bc, val) != NULL) {
		fprintf(stderr, "key{%x} already exists.\n", val);
		return;
	}

	struct vec **map_blk = __map->table+(val&0xFF);
	if (!(*map_blk)) {
		*map_blk = (struct vec*)malloc(sizeof(struct vec));
		vec_init(*map_blk);
	}

	map_entry_t *map_entry;
	vec_push(*map_blk, (void**)&map_entry, sizeof(map_entry_t));

	printf("put with key{%x}. in map: %p\n", val, __map);
	map_entry->val = val;
	map_entry->data = __data;
	map_entry->key = (mdl_u8_t*)__key;
	map_entry->bc = __bc;
}

void map_get_entry(struct map *__map, mdl_u8_t const *__key, mdl_uint_t __bc, map_entry_t **__map_entry) {
	mdl_u32_t val = map_hash(__key, __bc);

	printf("get with key{%x}. in map: %p\n", val, __map);

	if (!(*__map_entry = _map_find(__map, __key, __bc, val))) {
		fprintf(stderr, "key{%x} not located.\n", val);
		*__map_entry = NULL;
	} else
		printf("key{%x} was located.\n", val);
}

void map_get(struct map *__map, mdl_u8_t const *__key, mdl_uint_t __bc, void **__data) {
	map_entry_t *map_entry = NULL;
	map_get_entry(__map, __key, __bc, &map_entry);
	if (map_entry != NULL) *__data = map_entry->data; else *__data = NULL;
}

void map_free(struct map *__map, mdl_u8_t const *__key, mdl_uint_t __bc) {
    map_entry_t *map_entry = NULL;
    map_get_entry(__map, __key, __bc, &map_entry);
	mdl_u32_t val = map_hash(__key, __bc);
	struct vec **map_blk = __map->table+(val&0xFF);
	if (map_blk != NULL)
		vec_free(*map_blk, (void*)map_entry, 1);
}
