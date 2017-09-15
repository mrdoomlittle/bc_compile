# include <bcc.h>
# include <string.h>
char extern* strcmb(char*, char*);
int main(int __argc, char const *__argv[]) {
/*
	struct map mp_a, mp_b;
	map_init(&mp_a);
	map_init(&mp_b);

	char *str = "Hello World!";
	map_put(&mp_a, str, strlen(str), str);

	str = "se!";
    map_put(&mp_a, str, strlen(str), str);

	str = "UDHFUFDHUFHDS";
	map_put(&mp_b, str, strlen(str), str);

	str = "UDHF";
    map_put(&mp_b, str, strlen(str), str);

	map_set_parent(&mp_a, &mp_b);

	str = "UDHFUFDHUFHDS";

	char *o;
	map_get(&mp_a, str, strlen(str), (void**)&o);
	printf("%s\n", o);

	map_de_init(&mp_a);
	map_de_init(&mp_b);

	return 0;
*/
/*
	struct test_struct {
		mdl_uint_t x, y;
	};

	struct vec_t vec;
	vec_init(&vec);

	printf("blk_desc_size: %u\n", sizeof(struct vec_blk_desc_t));
	printf("test_struct_size: %u\n", sizeof(struct test_struct));

	struct test_struct *a, *b, *c, *d, *e, *f;

	vec_push(&vec, (void**)&a, sizeof(struct test_struct));
	a->x = a->y = 1;

	vec_push(&vec, (void**)&b, sizeof(struct test_struct));
	b->x = b->y = 2;

	vec_push(&vec, (void**)&c, sizeof(struct test_struct));
	c->x = c->y = 3;

	vec_push(&vec, (void**)&d, sizeof(struct test_struct));
	d->x = d->y = 4;

//	for (int x = 0; x != 3; x++) {
//		struct test_struct o;
//		vec_pop(&vec, (void*)&o, sizeof(struct test_struct));
//		printf("%d\n", o.x);
//	}

	vec_free(&vec, (void*)b, 1);
	vec_free(&vec, (void*)a, 1);
//	vec_free(&vec, (void*)c);

	vec_push(&vec, (void**)&e, sizeof(struct test_struct));
	e->x = e->y = 5;

	vec_push(&vec, (void**)&f, sizeof(struct test_struct));
	f->x = f->y = 6;

	struct test_struct *itr = (struct test_struct*)vec_first(&vec);
	for (;itr != NULL; vec_itr((void**)&itr, VEC_ITR_DOWN)){
		usleep(100000);
		printf("x: %u, y: %u, %p - %p\n", itr->x, itr->y, itr, (struct test_struct*)vec_last(&vec));
	}
*/
	if (__argc < 5) {
		printf("usage:\n -o - dest file.\n -i - src file.\n");
		return -1;
	}

	struct vec inc_pths;
	vec_init(&inc_pths);

	char const *src_fpth = NULL, *dst_fpth = NULL;
	char const **arg_itr = __argv+1;
	while(arg_itr < __argv+__argc) {
		if (!strcmp(*arg_itr, "-o"))
			dst_fpth = *(++arg_itr);
		else if (!strcmp(*arg_itr, "-i"))
			src_fpth = *(++arg_itr);
		else if (!strcmp(*arg_itr, "-I")) {
			char **inc_pth;
			vec_push(&inc_pths, (void**)&inc_pth, sizeof(char*));
			*inc_pth = (char*)*(++arg_itr);
		}
		arg_itr++;
	}

	if (!src_fpth || !dst_fpth) {
		fprintf(stderr, "somthing went wrong.\n");
		return -1;
	}

	printf("src_fpth: %s, dst_fpth: %s\n", src_fpth, dst_fpth);

	struct bcc bcc = {
		.src_fpth = src_fpth,
		.dst_fpth = dst_fpth
	};

	bcc_init(&bcc, &inc_pths);
	bcc_run(&bcc);
	bcc_de_init();
}
