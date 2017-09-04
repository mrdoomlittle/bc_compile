# include "bcc.h"
# define MAP_DEPTH_MAX 0
struct map_entry_t {
	mdl_u64_t part;
	mdl_u32_t val;
	void *data;
};

mdl_u64_t map_get_part(mdl_u8_t const* __key, mdl_uint_t __bc) {
	if (__bc >= sizeof(mdl_u64_t))
		return *(mdl_u64_t*)__key;
	else if (__bc >= sizeof(mdl_u32_t))
		return *(mdl_u32_t*)__key;
	else if (__bc >= sizeof(mdl_u32_t))
		return *(mdl_u16_t*)__key;
	return 0;
}

mdl_u32_t map_hash(mdl_u8_t const *__key, mdl_uint_t __bc) {
	mdl_u8_t const *itr = __key;
	mdl_u32_t ret_val = 1;
	while(itr != __key+__bc) {
		ret_val = ((ret_val>>(*itr&3))+(*itr*((itr-__key)+128)));
		itr++;
	}

	return ret_val;
}

struct map_entry_t* _map_find(struct map *__map, mdl_u8_t const *__key, mdl_uint_t __bc, mdl_u32_t __val) {
	struct vec **map_block = __map->table+*__key;
	struct map_entry_t *itr;

	if (*map_block == NULL) {
		if (__map->parent != NULL) goto _search_parent;
		return NULL;
	}

	itr = (struct map_entry_t*)vec_first(*map_block);

	if (!vec_size(*map_block)) {
		if (__map->parent != NULL) goto _search_parent;
		fprintf(stderr, "map table empty, no data to be found.\n");
		return NULL;
	}

	while(itr != NULL) {
		printf("comparing key: [%x] with [%x]\n", itr->val, __val);
		if (itr->val == __val && itr->part == map_get_part(__key+1, __bc-1)) return itr;
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
	for (mdl_uint_t e = 0; e != (mdl_u8_t)~0; e++) {
		*((void**)((mdl_u8_t*)__p+(e*sizeof(void*)))) = (p = (void*)f_malloc(((mdl_u8_t)~0)*sizeof(void*)));
		memset(p, 0, ((mdl_u8_t)~0)*sizeof(void*));
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

	for (mdl_uint_t e = 0; e != (mdl_u8_t)~0; e++) {
		void *p = *((void**)((mdl_u8_t*)__p+(e*sizeof(void*))));
		map_initr(p);
		map_initl(p);
		c = 0;
	}

}
*/

void map_init(struct map *__map) {
	__map->table = (struct vec**)malloc(((mdl_u8_t)~0)*sizeof(struct vec*));
	for (mdl_uint_t i = 0; i != ((mdl_u8_t)~0); i++) *(__map->table+i) = NULL;
	__map->parent = NULL;

//	__map->r = (void*)f_malloc(((mdl_u8_t)~0)*sizeof(void*));
//	void *p = __map->r;

//	map_initr(p);
//	map_initl(p);
}

void map_de_init(struct map *__map) {
	for (mdl_uint_t i = 0; i != ((mdl_u8_t)~0); i++) {
		struct vec **map_block = __map->table+i;
		if (*map_block != NULL) vec_de_init(*map_block);
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
	if (!*__data) {
		fprintf(stderr, "failed to locate key[%x].\n", __key);
	}
}

void map_put(struct map *__map, mdl_u8_t const *__key, mdl_uint_t __bc, void *__data) {
	mdl_u32_t hash = map_hash(__key, __bc);
	if (_map_find(__map, __key, __bc, hash) != NULL) {
		fprintf(stderr, "key[%x] already exists.\n", hash);
		return;
	}

	struct vec **map_block = __map->table+*__key;
	if (!(*map_block)) {
		*map_block = malloc(sizeof(struct vec));
		vec_init(*map_block);
	}

	struct map_entry_t *map_entry;
	vec_push(*map_block, (void**)&map_entry, sizeof(struct map_entry_t));

	printf("put with key[%x]. in map: %p\n", hash, __map);
	map_entry->part = map_get_part(__key+1, __bc-1);
	map_entry->val = hash;
	map_entry->data = __data;
}

void map_get_entry(struct map *__map, mdl_u8_t const *__key, mdl_uint_t __bc, struct map_entry_t **__map_entry) {
	mdl_u32_t hash = map_hash(__key, __bc);

	printf("get with key[%x]. in map: %p\n", hash, __map);

	if (!(*__map_entry = _map_find(__map, __key, __bc, hash))) {
		fprintf(stderr, "key[%x] not located.\n", hash);
		*__map_entry = NULL;
	} else
		printf("key[%x] was located.\n", hash);
}

void map_get(struct map *__map, mdl_u8_t const *__key, mdl_uint_t __bc, void **__data) {
	struct map_entry_t *map_entry = NULL;
	map_get_entry(__map, __key, __bc, &map_entry);
	if (map_entry != NULL) *__data = map_entry->data; else *__data = NULL;
}

void map_free(struct map *__map, mdl_u8_t const *__key, mdl_uint_t __bc) {
    struct map_entry_t *map_entry = NULL;
    map_get_entry(__map, __key, __bc, &map_entry);
	struct vec **map_block = __map->table+*__key;
	if (map_block != NULL)
		vec_free(*map_block, (void*)map_entry, 1);
}
