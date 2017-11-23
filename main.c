# include <mdl/bcc.h>
# include <string.h>
char extern* strcmb(char*, char*);
int main(int __argc, char const *__argv[]) {
//	struct vec my_vec;
//	vec_init(&my_vec);
//	mdl_u64_t *a, *b;

//	mdl_u8_t i = 0;
//	for (;i != 4;i++) {
//	vec_push(&my_vec, (void**)&a, sizeof(mdl_u64_t));
//	vec_push(&my_vec, (void**)&b, sizeof(mdl_u64_t));
//	vec_free(&my_vec, a, 0);
//	vec_free(&my_vec, b, 0);

//	printf("|--------------------------------------------------------------|\n\n");

//	for (;;){	
//	vec_push(&my_vec, (void**)&a, sizeof(mdl_u64_t));
//	vec_free(&my_vec, a, 0);
//	}
//	printf("|--------------------------------------------------------------|\n");

//	vec_push(&my_vec, (void**)&a, sizeof(mdl_u64_t));
//	vec_free(&my_vec, a, 0);
//	printf("\n\n");
//	}
//	vec_push(&my_vec, (void**)&a, sizeof(mdl_u64_t));
//	vec_free(&my_vec, a, 0);
//	}
//	}

//	vec_de_init(&my_vec);
	if (__argc < 5) {
		printf("usage:\n -o - dest file.\n -i - src file.\n");
		return -1;
	}

	struct vec inc_paths;
	vec_init(&inc_paths);

	char const *src_fpth = NULL, *dst_fpth = NULL;
	char const **arg_itr = __argv+1;
	while(arg_itr < __argv+__argc) {
		if (!strcmp(*arg_itr, "-o"))
			dst_fpth = *(++arg_itr);
		else if (!strcmp(*arg_itr, "-i"))
			src_fpth = *(++arg_itr);
		else if (!strcmp(*arg_itr, "-I")) {
			char **inc_pth;
			vec_push(&inc_paths, (void**)&inc_pth, sizeof(char*));
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

	if (bcc_init(&bcc, &inc_paths) != BCC_SUCCESS) return 1;
	bcc_run(&bcc);
	bcc_de_init();
	return 0;
}
