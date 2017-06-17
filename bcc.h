# ifndef __bcc__h
# define __bcc__h
# include <sys/stat.h>
# include <string.h>
# include <fcntl.h>
# include <errno.h>
# include <stdint.h>
# include <stdlib.h>
# include <unistd.h>
# include <eint_t.h>
# include <bci.h>
# define BCC_SUCCESS 0
# define BCC_FAILURE -1
typedef mdl_u8_t bcc_flg_t;

struct bcc_t {
	char const *src_fpth;
	bcc_flg_t flgs;
};


typedef mdl_i8_t bcc_err_t;

bcc_err_t bcc_init(struct bcc_t*);
bcc_err_t bcc_de_init();
bcc_err_t bcc_run(struct bcc_t*);
bcc_err_t bcc_dump_bc(char const*);

mdl_u8_t bcc_is_flg(bcc_flg_t, bcc_flg_t);
void bcc_tog_flg(bcc_flg_t*, bcc_flg_t);
# endif /*__bcc__h*/
