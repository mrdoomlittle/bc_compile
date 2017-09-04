# include "bcc.h"
void buff_init(struct buff *__buff, mdl_uint_t __blk_size, mdl_uint_t __blk_c) {
	__buff->blk_c = __blk_c;
	__buff->blk_size = __blk_size;
	__buff->p = (mdl_u8_t*)malloc(__blk_c*__blk_size);
	__buff->curr_blk = 0;
}

void buff_push(struct buff *__buff, void *__data) {
	buff_put(__buff, (__buff->curr_blk)++, __data);}

void buff_pop(struct buff *__buff, void *__data) {
	buff_get(__buff, --(__buff->curr_blk), __data);}

void buff_put(struct buff *__buff, mdl_uint_t __blk, void *__data) {
	byte_ncpy(__buff->p+(__blk*__buff->blk_size), (mdl_u8_t*)__data, __buff->blk_size);}

void buff_get(struct buff *__buff, mdl_uint_t __blk, void *__data) {
	byte_ncpy((mdl_u8_t*)__data, __buff->p+(__blk*__buff->blk_size), __buff->blk_size);}

void* buff_begin(struct buff *__buff) {return (void*)__buff->p;}
void* buff_end(struct buff *__buff) {
	return (void*)(__buff->p+((__buff->blk_c-1)*__buff->blk_size));}

void buff_free(struct buff *__buff) {
	free(__buff->p);}

mdl_uint_t buff_blk_size(struct buff *__buff){return __buff->blk_size;}
mdl_uint_t buff_blk_c(struct buff *__buff){return __buff->blk_c;}

mdl_uint_t buff_len(struct buff *__buff) {
	return __buff->curr_blk;}
