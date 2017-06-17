# include <bcc.h>
int main(int __argc, char const *__argv[]) {
	struct bcc_t bcc = {
		.src_fpth = "main.bc"
	};

	bcc_init(&bcc);
	bcc_run(&bcc);
	bcc_dump_bc("a.out");
	bcc_de_init();
}
