# include "bcc.h"
# include <bci.h>
struct buff static bc_buff;
mdl_u8_t static *bcb_itr;

bci_addr_t get_addr() {
	return (bci_addr_t)(bcb_itr-(mdl_u8_t*)buff_begin(&bc_buff));
}

mdl_u8_t bcb_get_w8(bci_addr_t __addr) {
	return *((mdl_u8_t*)buff_begin(&bc_buff)+__addr);
}

void bcb_emit_w8(mdl_u8_t __val) {
	*(bcb_itr++) = __val;
}

void bcb_emit_w16(mdl_u16_t __val) {
	bcb_emit_w8(__val);
	bcb_emit_w8(__val >> 8);
}

void bcb_emit(mdl_u8_t *__ptr, mdl_uint_t  __n) {
	for (mdl_u8_t *itr = __ptr; itr != __ptr+__n; itr++) bcb_emit_w8(*itr);
}

void bcb_emit_null(mdl_uint_t __n) {
	mdl_uint_t val = 0x0;
	bcb_emit((mdl_u8_t*)&val, __n);
}

void bcb_emit_no(mdl_uint_t __no) {
	mdl_u8_t size, bcit;
	if (__no >= 0 && __no <= (mdl_u8_t)~0) {
		size = sizeof(mdl_u8_t);
		bcit = _bcit_w8;
	} else if (__no > (mdl_u8_t)~0 && __no <= (mdl_u16_t)~0) {
		size = sizeof(mdl_u16_t);
		bcit = _bcit_w16;
	} else if (__no > (mdl_u16_t)~0 && __no <= (mdl_u32_t)~0) {
		size = sizeof(mdl_u32_t);
		bcit = _bcit_w32;
	} else if (__no > (mdl_u32_t)~0 && __no <= (mdl_u64_t)~0) {
		size = sizeof(mdl_u64_t);
		bcit = _bcit_w64;
	}

	bcb_emit_w8(bcit);
	bcb_emit((mdl_u8_t*)&__no, size);
}

// addr size might change later
void bcb_emit_addr(bci_addr_t __addr) {
	bcb_emit((mdl_u8_t*)&__addr, sizeof(bci_addr_t));
}

void bcii_emit(mdl_u8_t __iid, bci_flag_t __flags) {
	bcb_emit_w8(__iid);
	bcb_emit_w8(__flags);
}

void bci_emit_assign(bci_addr_t __addr, mdl_u8_t **__val, mdl_u8_t __type, bci_flag_t __flags) {
	bcii_emit(_bcii_assign, __flags);
	bcb_emit_w8(__type);

	bcb_emit_addr(__addr);

	*__val = bcb_itr;
	bcb_emit_null(bcit_sizeof(__type));
}

void bci_emit_mov(mdl_u8_t __type, bci_addr_t __dst_addr, bci_addr_t __src_addr, bci_flag_t __flags) {
	bcii_emit(_bcii_mov, __flags);
	bcb_emit_w8(__type);
	bcb_emit_addr(__dst_addr);
	bcb_emit_addr(__src_addr);
}

void bci_emit_aop(mdl_u8_t __aop, mdl_u8_t __type, void *__l, void *__r, bci_addr_t __dst_addr, bci_flag_t __flags) {
	bcii_emit(_bcii_aop, __flags);

	bcb_emit_w8(__aop);
	bcb_emit_w8(__type);
	bcb_emit_addr(__dst_addr);

	if (is_flag(__flags, _bcii_aop_fl_pm))
		bcb_emit((mdl_u8_t*)__l, bcit_sizeof(__type));
	else
		bcb_emit_addr(*(bci_addr_t*)__l);

	if (is_flag(__flags, _bcii_aop_fr_pm))
		bcb_emit((mdl_u8_t*)__r, bcit_sizeof(__type));
	else
		bcb_emit_addr(*(bci_addr_t*)__r);
}

void bci_emit_exit() {
	bcii_emit(_bcii_exit, 0);
}

void bci_emit_cmp(mdl_u8_t __l_type, mdl_u8_t __r_type, bci_addr_t __l_addr, bci_addr_t __r_addr, bci_addr_t __dst_addr) {
	bcii_emit(_bcii_cmp, 0);

	bcb_emit_w8(__l_type);
	bcb_emit_w8(__r_type);

	bcb_emit_addr(__l_addr);
	bcb_emit_addr(__r_addr);

	bcb_emit_addr(__dst_addr);
}

void bci_emit_cond_jmp(mdl_u8_t __cond, bci_addr_t __jmpm_addr, bci_addr_t __cond_addr) {
	bcii_emit(_bcii_cjmp, 0);
	bcb_emit_w8(__cond);

	bcb_emit_addr(__jmpm_addr);
	bcb_emit_addr(__cond_addr);
}

void bci_emit_jmp(bci_addr_t __jmpm_addr) {
	bcii_emit(_bcii_jmp, 0);
	bcb_emit_addr(__jmpm_addr);
}

void bci_emit_print(mdl_u8_t __type, bci_addr_t __addr) {
	bcii_emit(_bcii_print, 0);
	bcb_emit_w8(__type);
	bcb_emit_addr(__addr);
}

void bci_emit_nop(mdl_u8_t __flags) {
	bcii_emit(_bcii_nop, __flags);
}

void bci_emit_dr(mdl_u8_t __type, bci_addr_t __src_addr, bci_addr_t __dst_addr) {
	bcii_emit(_bcii_dr, 0);
	bcb_emit_w8(__type);
	bcb_emit_addr(__src_addr);
	bcb_emit_addr(__dst_addr);
}

void bci_emit_conv(mdl_u8_t __to_type, mdl_u8_t __from_type, bci_addr_t __dst_addr, bci_addr_t __src_addr, mdl_u8_t __flags) {
	bcii_emit(_bcii_conv, __flags);
	bcb_emit_w8(__to_type);
	bcb_emit_w8(__from_type);
	bcb_emit_addr(__dst_addr);
	bcb_emit_addr(__src_addr);
}

void bci_emit_incr(mdl_u8_t __type, bci_addr_t __addr, bci_addr_t __bc, mdl_u8_t __flags) {
	bcii_emit(_bcii_incr, __flags);
	bcb_emit_w8(__type);
	bcb_emit_addr(__addr);
	is_flag(__flags, _bcii_iod_fbc_addr)? bcb_emit_addr(__bc):bcb_emit_no(__bc);
}

void bci_emit_decr(mdl_u8_t __type, bci_addr_t __addr, bci_addr_t __bc, mdl_u8_t __flags) {
	bcii_emit(_bcii_decr, __flags);
	bcb_emit_w8(__type);
	bcb_emit_addr(__addr);
	is_flag(__flags, _bcii_iod_fbc_addr)? bcb_emit_addr(__bc):bcb_emit_no(__bc);
}

void bci_emit_extern_call(mdl_u8_t __ret_type, bci_addr_t __ret_addr, bci_addr_t __id_addr, bci_addr_t __arg_addr) {
	bcii_emit(_bcii_extern_call, 0);
	bcb_emit_w8(__ret_type);
	bcb_emit_addr(__ret_addr);
	bcb_emit_addr(__id_addr);
	bcb_emit_addr(__arg_addr);
}

void bci_emit_eeb_init(mdl_u8_t __blk_c, mdl_u8_t __flags) {
	bcii_emit(_bcii_eeb_init, __flags);
	bcb_emit_w8(__blk_c);
}

void bci_emit_eeb_put(mdl_u8_t __blk_id, bci_addr_t __b_addr, bci_addr_t __e_addr, mdl_u8_t __flags) {
	bcii_emit(_bcii_eeb_put, __flags);
	bcb_emit_w8(__blk_id);
	bcb_emit_addr(__b_addr);
	bcb_emit_addr(__e_addr);
}

bci_addr_t static stack_addr = 0, base_addr = 0;

# define incr_stack_addr(__bc) stack_addr+=__bc;
# define decr_stack_addr(__bc) stack_addr-=__bc;

void incr_stack_ptr(bci_addr_t __bc) {
	bci_emit_incr(_bcit_addr, RG_SP, __bc, 0);
}

void decr_stack_ptr(bci_addr_t __bc) {
	bci_emit_decr(_bcit_addr, RG_SP, __bc, 0);
}

struct vec labels;
struct vec gotos;

struct label_t {
	char *name;
	bci_addr_t addr;
};

struct goto_t {
	char *name;
	bci_addr_t *jmp_addr;
};

bci_addr_t *sp, *bp;
bcc_err_t gen_init(mdl_uint_t __bc_buff_size) {
	buff_init(&bc_buff, sizeof(mdl_u8_t), __bc_buff_size);
	// note im using the word register wong as theres no other word i cound think of to use
	incr_stack_addr(bcit_sizeof(_bcit_w8)*3); // 8bit registers
	incr_stack_addr(bcit_sizeof(_bcit_w16)*6); // 16bit registers
	incr_stack_addr(bcit_sizeof(_bcit_w32)*3); // 32bit registers
	incr_stack_addr(bcit_sizeof(_bcit_w64)*3); // 64bit registers
    bcb_itr = (mdl_u8_t*)buff_begin(&bc_buff);
	bci_emit_assign(RG_BP, (mdl_u8_t**)&bp, _bcit_addr, 0);
	bci_emit_assign(RG_SP, (mdl_u8_t**)&sp, _bcit_addr, 0);
	vec_init(&gotos);
	vec_init(&labels);
	*bp = base_addr = stack_addr;
}

void gen_apply_sp_val() {
	*sp = stack_addr;
}

bcc_err_t gen_de_init() {
	buff_free(&bc_buff);
}

bci_addr_t get_rga_addr(mdl_u8_t);

void stack_push(mdl_u8_t __bcit) {
	bci_emit_mov(__bcit, RG_SP, get_rga_addr(__bcit), _bcii_mov_fdr_da);
	incr_stack_ptr(bcit_sizeof(__bcit));
}

void stack_pop(mdl_u8_t __bcit) {
	decr_stack_ptr(bcit_sizeof(__bcit));
	bci_emit_mov(__bcit, get_rga_addr(__bcit), RG_SP, _bcii_mov_fdr_sa);
}

mdl_u8_t kind_to_bcit(struct type *_type) {
	switch(_type->kind) {
		case T_KIND_VOID: return _bcit_void;
		case T_KIND_X8: return _bcit_w8;
		case T_KIND_X16: return _bcit_w16;
		case T_KIND_X32: return _bcit_w32;
		case T_KIND_X64: return _bcit_w64;
		case T_KIND_PTR: return _bcit_addr;
	}
	return 0;
}

bci_addr_t get_rga_addr(mdl_u8_t __type) {
	switch(__type) {
		case _bcit_w8: return RG_W8A;
		case _bcit_w16: case _bcit_addr: return RG_W16A;
		case _bcit_w32: return RG_W32A;
		case _bcit_w64: return RG_W64A;
	}
	return 0;
}

bci_addr_t get_rgb_addr(mdl_u8_t __type) {
	switch(__type) {
		case _bcit_w8: return RG_W8B;
		case _bcit_w16:	case _bcit_addr: return RG_W16B;
		case _bcit_w32: return RG_W32B;
		case _bcit_w64: return RG_W64B;
	}
	return 0;
}

bci_addr_t get_rgc_addr(mdl_u8_t __type) {
	switch(__type) {
		case _bcit_w8: return RG_W8C;
		case _bcit_w16: case _bcit_addr: return RG_W16C;
		case _bcit_w32: return RG_W32C;
		case _bcit_w64: return RG_W64C;
	}
	return 0;
}

mdl_u8_t size_to_bcit(mdl_u8_t);
# define get_child(__node, __fs) (*((__node)->child_buff+__fs))
void emit_expr(struct node*);
void emit_label(struct node*);
void emit_load_conv(mdl_u8_t, mdl_u8_t);

void static emit_aop(struct node *__node, mdl_u8_t __aop) {
	struct node *l = __node->l;
	struct node *r = __node->r;
	mdl_u8_t bcit = __node->_type->bcit;

	mdl_u8_t l_bcit = l->_type->bcit;
	mdl_u8_t r_bcit = r->_type->bcit;

	emit_expr(l);
	stack_push(l_bcit);

	emit_expr(r);
	bci_emit_mov(r_bcit, get_rgb_addr(r_bcit), get_rga_addr(r_bcit), 0);

	stack_pop(l_bcit);
	bci_addr_t l_addr = get_rga_addr(l_bcit);
	bci_addr_t r_addr = get_rgb_addr(r_bcit);

	mdl_u8_t _signed = (__node->_type->flags&SIGNED) == SIGNED;
	bci_emit_aop(__aop, _signed? bcit|_bcit_msigned:bcit, &l_addr, &r_addr, get_rga_addr(bcit), 0);
}

void static emit_cmp(struct node *__node) {
	struct node *l = __node->l;
	struct node *r = __node->r;

	mdl_u8_t l_bcit = l->_type->bcit;
	mdl_u8_t r_bcit = r->_type->bcit;

	emit_expr(l);
	bci_emit_mov(l_bcit, get_rgb_addr(l_bcit), get_rga_addr(l_bcit), 0);

	emit_expr(r);
	bci_emit_cmp(l_bcit, r_bcit, get_rgb_addr(l_bcit), get_rga_addr(r_bcit), get_rga_addr(_bcit_w8));
}

void static emit_binop(struct node *__node) {
	switch(__node->kind) {
		case AST_ADD:
			emit_aop(__node, _bci_aop_add);
		break;
		case AST_SUB:
			emit_aop(__node, _bci_aop_sub);
		break;
		case AST_OP_EQ: case AST_OP_NEQ:
			emit_cmp(__node);
		break;
		default:
			printf("--------------------------------------------------------------\n");
			return;
	}
/*

	struct node *l = __node->l;
	struct node *r = __node->r;
	mdl_u8_t bcit = size_to_bcit(sizeof(mdl_uint_t));

	mdl_u8_t lbcit = kind_to_bcit(l->_type);
	mdl_u8_t rbcit = kind_to_bcit(r->_type);

	emit_expr(l);
	if (l->kind == AST_VAR) {
		bci_emit_mov(bcit, lbcit, stack_addr, get_rga_addr(lbcit));
	} else {
		bci_emit_mov(bcit, bcit, stack_addr, get_rga_addr(bcit));
	}

	incr_stack_addr(bcit_sizeof(bcit));

	emit_expr(r);

	bci_emit_aop(aop, bcit, rbcit, stack_addr-bcit_sizeof(bcit), get_rga_addr(rbcit), bcit, get_rga_addr(bcit));
	decr_stack_addr(bcit_sizeof(bcit));
*/
}

void emit_var(struct node*);

void emit_save(mdl_u8_t, bci_addr_t);

void emit_incr_or_decr_deref() {

}

void emit_assign_deref(struct type *__val, struct node *__node, mdl_uint_t __off) {
	mdl_u8_t ptr_bcit = __node->_type->bcit;
	mdl_u8_t val_bcit = __val->bcit;

	bci_emit_mov(val_bcit, get_rgc_addr(val_bcit), get_rga_addr(val_bcit), 0);
	emit_expr(__node);

	bci_addr_t ptr_addr = get_rga_addr(ptr_bcit);

	if (__off != 0)
		bci_emit_aop(_bci_aop_add, ptr_bcit, &ptr_addr, &__off, ptr_addr, _bcii_aop_fr_pm);

	bci_emit_mov(val_bcit, ptr_addr, get_rgc_addr(val_bcit), _bcii_mov_fdr_da);
}

void emit_assign_struct_ref(struct node *__node) {
	struct node *var = get_child(__node, 0);
	printf("))))))))))))))))))))))))))))))))))))))))))))))))))offset: %u\n", __node->_type->offset+var->off);

	switch(var->kind) {
		case AST_DEREF:
			emit_assign_deref(__node->_type, get_child(var, 0), __node->_type->offset);
			printf("-----------------------------------ptr_bcit: %u, offset: %u\n", var->_type->bcit, __node->_type->offset);
		break;
		case AST_VAR:
			emit_save(__node->_type->bcit, __node->_type->offset+var->off);
		break;
	}
}

void emit_store(struct node *__node) {
	printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
	print_node(__node);
	switch(__node->kind) {
		case AST_STRUCT_REF:
			emit_assign_struct_ref(__node);
		break;
		case AST_DEREF:
			emit_assign_deref(__node->_type, get_child(__node, 0), 0);
		break;
		case AST_VAR:
			emit_save(__node->_type->bcit, __node->off);
		break;
	}
}

void static emit_decl_init(struct node *__init, bci_addr_t __off, mdl_u8_t __bcit) {
	mdl_u8_t bcit = __init->_type->bcit;
	if (__init->_type->kind == T_KIND_ARRAY) {
		mdl_uint_t blk_off = 0;
		for (struct node **itr = (struct node**)vec_begin(&__init->_vec); itr != NULL; vec_itr((void**)&itr, VEC_ITR_DOWN, 1)) {
			emit_expr(*itr);
		//	printf("ooooooooooooooooooooooooooooooooooooooooooooooooooooooooo\n");
//			mdl_u8_t *v;
//			bci_emit_assign(__off, &v, _bcit_w8, 0);
//			*v = 200;
			emit_load_conv(__bcit, (*itr)->_type->bcit);
			emit_save(__bcit, __off+((blk_off++)*bcit_sizeof(__bcit)));
		}

		return;
	}

	emit_expr(*__init->child_buff);
	emit_save(bcit, __off);
}

void static emit_decl(struct node *__node) {
	if (__node->child_buff == __node->child_itr) return;
	struct node *var = get_child(__node, 0);
	struct node *init = get_child(__node, 1);

	var->off = stack_addr-base_addr;
	incr_stack_addr(var->_type->size);
	printf("------------------------? size: %u, off: %u\n", var->_type->size, var->off);

	if (init != NULL) {
		emit_decl_init(init, var->off, init->_type->kind == T_KIND_ARRAY? var->_type->ptr->bcit:var->_type->bcit);
	}

//	emit_expr(var);
}

void static emit_exit() {
	bci_emit_exit();
}

void emit_literal(struct node *__node) {
	mdl_u8_t *val;
	if (__node->_type->kind == T_KIND_ARRAY) {
		__node->off = stack_addr-base_addr;

		for (mdl_u8_t *itr = (mdl_u8_t*)__node->p; itr != (mdl_u8_t*)__node->p+(__node->_type->len-1); itr++) {
			bci_emit_assign(stack_addr, (mdl_u8_t**)&val, _bcit_w8, 0);
			*val = *itr;
			incr_stack_addr(1);
		}

		bci_addr_t *addr;
		bci_emit_assign(get_rga_addr(__node->_type->bcit), (mdl_u8_t**)&addr, __node->_type->bcit, 0);
		*addr = base_addr+__node->off;
		return;
	}

	bci_emit_assign(get_rga_addr(__node->_type->bcit), (mdl_u8_t**)&val, __node->_type->bcit, 0);
	byte_ncpy(val, __node->val, bcit_sizeof(__node->_type->bcit));
}

struct file_t static *outfile;
void set_output_file(struct file_t *__file) {
	outfile = __file;
}

void write_output_file() {
	printf("writing to file: %s\n", outfile->path);
	if (write(outfile->fd, buff_begin(&bc_buff), bcb_itr-(mdl_u8_t*)buff_begin(&bc_buff)) < 0) {
		fprintf(stderr, "failed to write to file, errno: %d\n", errno);
		return;
	}
}

void close_output_file() {
	if (close(outfile->fd) < 0) {
		fprintf(stderr, "failed to close file, errno %d\n", errno);
		return;
	}
}

void emit_save(mdl_u8_t __bcit, bci_addr_t __dst_off) {
	bci_addr_t l_addr = RG_BP;
	bci_emit_aop(_bci_aop_add, _bcit_addr, &l_addr, &__dst_off, get_rgb_addr(_bcit_addr), _bcii_aop_fr_pm);
	bci_emit_mov(__bcit, get_rgb_addr(_bcit_addr), get_rga_addr(__bcit), _bcii_mov_fdr_da);
}

void emit_load_conv(mdl_u8_t __to_bcit, mdl_u8_t __from_bcit) {
	if (__to_bcit != __from_bcit) {
		bci_emit_conv(__to_bcit, __from_bcit, get_rga_addr(__to_bcit), get_rga_addr(__from_bcit), 0);
		//bci_emit_mov(__to_bcit, __from_bcit, get_rga_addr(__to_bcit), get_rga_addr(__from_bcit), 0);
	}
}

void emit_assign(struct node *__node) {
	struct node *l = __node->l;
	struct node *r = __node->r;

	mdl_u8_t l_bcit = l->_type->bcit;

	emit_expr(r);

	mdl_u8_t r_bcit = r->_type->bcit;
	emit_load_conv(l_bcit, r_bcit);
	emit_store(l);
//	printf("----------------------------> %u\n", r->kind == AST_VAR);
}

mdl_u8_t size_to_bcit(mdl_u8_t __size) {
	switch(__size) {
		case 1: return _bcit_w8;
		case 2: return _bcit_w16;
		case 4: return _bcit_w32;
		case 8: return _bcit_w64;
	}
}

void emit_load(mdl_u8_t __kind, mdl_u8_t __bcit, bci_addr_t __src_off) {
	bci_addr_t l_addr = RG_BP;
	bci_emit_aop(_bci_aop_add, _bcit_addr, &l_addr, &__src_off, get_rgb_addr(_bcit_addr), _bcii_aop_fr_pm);
	if (__kind == T_KIND_ARRAY) {
		bci_addr_t *val;
		bci_emit_assign(get_rga_addr(__bcit), (mdl_u8_t**)&val, _bcit_addr, 0);
		*val = base_addr+__src_off;
	} else
		bci_emit_mov(__bcit, get_rga_addr(__bcit), get_rgb_addr(_bcit_addr), _bcii_mov_fdr_sa);
}

void emit_var(struct node *__node) {
	struct type *_type = __node->_type;
	emit_load(_type->kind, _type->bcit, __node->off);
	printf("name: %s, addr: %u\n", (char*)__node->p, __node->off);
}

void emit_if(struct node *__node) {
	struct node *cond = *__node->child_buff;
	emit_expr(cond);

	bci_addr_t *jmp_addr;
	bci_emit_assign(RG_W16B, (mdl_u8_t**)&jmp_addr, _bcit_addr, 0);
	bci_addr_t jmpm_addr = RG_W16B;

	if (cond->kind == AST_OP_EQ)
		bci_emit_cond_jmp(_bcic_neq, jmpm_addr, get_rga_addr(_bcit_w8));
	if (cond->kind == AST_OP_NEQ)
		bci_emit_cond_jmp(_bcic_eq, jmpm_addr, get_rga_addr(_bcit_w8));

	emit_expr(*(__node->child_buff+1));

	bci_addr_t *eljmp_addr;
	bci_emit_assign(jmpm_addr, (mdl_u8_t**)&eljmp_addr, _bcit_w16, 0);
	bci_emit_jmp(jmpm_addr);

	*jmp_addr = bcb_itr-(mdl_u8_t*)buff_begin(&bc_buff);

	emit_expr(*(__node->child_buff+2));

	*eljmp_addr = bcb_itr-(mdl_u8_t*)buff_begin(&bc_buff);
}

void emit_while(struct node *__node) {
	bci_addr_t _jmp_addr = bcb_itr-(mdl_u8_t*)buff_begin(&bc_buff);

	emit_if(*__node->child_buff);

	bci_addr_t *jmp_addr;
    bci_emit_assign(RG_W16B, (mdl_u8_t**)&jmp_addr, _bcit_addr, 0);
    bci_emit_jmp(RG_W16B);
	*jmp_addr = _jmp_addr;

	emit_label(*(__node->child_buff+1));
}

void emit_label(struct node *__node) {
	struct label_t *label;

	vec_push(&labels, (void**)&label, sizeof(struct label_t));
	label->addr = bcb_itr-(mdl_u8_t*)buff_begin(&bc_buff);
	label->name = __node->p;
	printf("label: %s:\n", (char*)__node->p);
}

void emit_goto(struct node *__node) {
	printf("emit goto\n");
	struct goto_t *_goto;
	vec_push(&gotos, (void**)&_goto, sizeof(struct goto_t));

	bci_emit_assign(RG_W16B, (mdl_u8_t**)&_goto->jmp_addr, _bcit_addr, 0);
	bci_emit_jmp(RG_W16B);
	_goto->name = __node->p;
	printf("goto %s;\n", (char*)__node->p);
}

void emit_compound_stmt(struct node *__node) {
	for (struct node **itr = (struct node**)vec_begin(&__node->_vec); itr != NULL; vec_itr((void**)&itr, VEC_ITR_DOWN, 1)) {
		emit_expr(*itr);
	}
}

void emit_func(struct node *__node) {
	bci_addr_t *jmp_addr;
	bci_emit_assign(RG_W16B, (mdl_u8_t**)&jmp_addr, _bcit_addr, 0);
	bci_emit_jmp(RG_W16B);

//	__node->off = get_rga_addr(__node->_type->bcit);
	emit_expr(*__node->child_buff);

	printf("stack addr: %u\n", stack_addr);
	for (struct node **itr = (struct node**)vec_begin(&__node->params); itr != NULL; vec_itr((void**)&itr, VEC_ITR_DOWN, 1)) {
		printf("func var offset: %d\n", stack_addr-base_addr);
		(*itr)->off = stack_addr-base_addr;
		mdl_u8_t bcit = (*itr)->_type->bcit;
		incr_stack_addr(bcit_sizeof(bcit));
		stack_pop(bcit);
		emit_store(*itr);
	}

	if (__node->_type->hva) {
		stack_pop(_bcit_addr);
		bci_emit_mov(_bcit_addr, stack_addr, get_rga_addr(_bcit_addr), 0);
		incr_stack_addr(bcit_sizeof(_bcit_addr));
	}

	emit_expr(*(__node->child_buff+1));

	stack_pop(_bcit_addr);
	bci_emit_jmp(get_rga_addr(_bcit_addr));
	*jmp_addr = bcb_itr-(mdl_u8_t*)buff_begin(&bc_buff);
}

void emit_func_call(struct node *__node) {
	struct node *func = get_child(__node, 0);

	bci_addr_t *rjmp_addr;
	bci_emit_assign(get_rga_addr(_bcit_addr), (mdl_u8_t**)&rjmp_addr, _bcit_addr, 0);
	stack_push(_bcit_addr);

	mdl_u8_t hva = func->_type->hva;
	bci_addr_t *va_addr;
	if (hva) {
		bci_emit_assign(get_rga_addr(_bcit_addr), (mdl_u8_t**)&va_addr, _bcit_addr, 0);
		stack_push(_bcit_addr);
	}

	mdl_uint_t va_size = vec_blk_c(&__node->args)-vec_blk_c(&func->params);

	struct node **p = (struct node**)vec_end(&__node->args);
	vec_itr((void**)&p, VEC_ITR_UP, va_size);

	struct node **itr = p;
	for (;itr != NULL; vec_itr((void**)&itr, VEC_ITR_UP, 1)) {
		emit_expr(*itr);
		mdl_u8_t bcit = (*itr)->_type->bcit;
		stack_push((*itr)->_type->bcit);
		printf("00000000000000000000000000000000000000000000000000000000000000000000\n");
	}

	if (hva) *va_addr = stack_addr;

	vec_itr((void**)&p, VEC_ITR_DOWN, 1);

	itr = p;

	bci_addr_t addr = stack_addr;
	incr_stack_addr(va_size*bcit_sizeof(_bcit_w64));

	while(itr != NULL) {
		printf("=======================================================; %u\n", va_size);
		mdl_u8_t bcit = (*itr)->_type->bcit;
		mdl_u64_t *val;
		bci_emit_assign(addr, &val, _bcit_w64, 0);
		*val = 0;

		emit_expr(*itr);
		bci_emit_mov(bcit, addr, get_rga_addr(bcit), 0);
		addr+= bcit_sizeof(_bcit_w64);
		vec_itr((void**)&itr, VEC_ITR_DOWN, 1);
	}

	emit_expr(get_child(__node, 1));
	*rjmp_addr = bcb_itr-(mdl_u8_t*)buff_begin(&bc_buff);
}

void emit_print(struct node *__node) {
	struct node *arg = *(struct node**)vec_first(&__node->args);
	emit_expr(arg);
	mdl_u8_t bcit = arg->_type->bcit;

	mdl_u8_t _signed = (arg->_type->flags&SIGNED) == SIGNED;
	bci_emit_print(_signed? bcit|_bcit_msigned:bcit, get_rga_addr(bcit));
}

void emit_extern_call(struct node *__node) {
	struct node **args = (struct node**)vec_first(&__node->args);

	struct node *id = *args;
	emit_expr(id);
	stack_push(_bcit_w8);

	vec_itr((void**)&args, VEC_ITR_DOWN, 1);
	struct node *arg = *args;
	emit_expr(arg);

	mdl_u8_t arg_bcit = arg->_type->bcit;
	bci_emit_mov(arg_bcit, get_rgb_addr(arg_bcit), get_rga_addr(arg_bcit), 0);
	stack_pop(_bcit_w8);

	mdl_u8_t bcit = __node->_type->bcit;
	bci_emit_extern_call(bcit, get_rga_addr(bcit), get_rga_addr(_bcit_w8), get_rgb_addr(arg_bcit));
}

void emit_addrof(struct node *__node) {
	bci_addr_t *addr;
	mdl_u8_t bcit = __node->_type->bcit;
	bci_emit_assign(get_rga_addr(bcit), (mdl_u8_t**)&addr, bcit, 0);
	*addr = base_addr+get_child(__node, 0)->off;
}

void emit_deref(struct node *__node) {
	struct node *child = get_child(__node, 0);
	if (child->kind == AST_INCR || child->kind == AST_DECR)
		emit_incr_or_decr(child, 1);
	else emit_expr(child);

	struct type *_type = child->_type;
	mdl_u8_t bcit = __node->_type->bcit;

	bci_emit_dr(bcit, get_rga_addr(_type->bcit), get_rga_addr(bcit));
}

void emit_return(struct node *__node) {
	struct node *ret_val = get_child(__node, 0);
	struct type *ret_type = ret_val->_type;
	emit_expr(ret_val);
	bci_emit_mov(ret_type->bcit, get_rgb_addr(ret_type->bcit), get_rga_addr(ret_type->bcit), 0);

	stack_pop(_bcit_addr);
	bci_emit_mov(_bcit_addr, get_rgc_addr(_bcit_addr), get_rga_addr(_bcit_addr), 0);
	bci_emit_mov(ret_type->bcit, get_rga_addr(ret_type->bcit), get_rgb_addr(ret_type->bcit), 0);

	bci_emit_jmp(get_rgc_addr(_bcit_addr));
}

void emit_conv(struct node *__node) {
	mdl_u8_t to_bcit = __node->_type->bcit;
	mdl_u8_t from_bcit = (*__node->child_buff)->_type->bcit;

	emit_expr(*__node->child_buff);
	emit_load_conv(to_bcit, from_bcit);
}

void emit_cast(struct node *__node) {
	emit_expr(get_child(__node, 0));
	emit_load_conv(__node->_type->bcit, get_child(__node, 0)->_type->bcit);
}

void emit_load_struct_ref(struct node *__node) {
	struct node *var = get_child(__node, 0);

//	printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> %u\n", get_child(var, 0)->off+__node->_type->offset);

	switch(var->kind) {
		case AST_DEREF:
			emit_expr(get_child(var, 0));

			mdl_u8_t ptr_bcit = get_child(var, 0)->_type->bcit;
			bci_addr_t ptr_addr = get_rga_addr(ptr_bcit);
			bci_emit_aop(_bci_aop_add, ptr_bcit, &ptr_addr, &__node->_type->offset, ptr_addr, _bcii_aop_fr_pm);

			bci_emit_dr(__node->_type->bcit, get_rga_addr(ptr_bcit), get_rga_addr(__node->_type->bcit));
		break;
		case AST_VAR:
			emit_load(__node->_type->kind, __node->_type->bcit, __node->_type->offset+var->off);
		break;
	}

	printf(";;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;offset: %u\n", __node->_type->offset+var->off);
}

void emit_incr_or_decr(struct node *__node, mdl_u8_t __drefd) {
	emit_load(__node->_type->kind, __node->_type->bcit, get_child(__node, 0)->off);
	if (__node->kind == AST_INCR)
		bci_emit_incr(__node->_type->bcit, get_rga_addr(__node->_type->bcit), 1, 0);
	else
		bci_emit_decr(__node->_type->bcit, get_rga_addr(__node->_type->bcit), 1, 0);
	if (!__drefd) emit_save(__node->_type->bcit, get_child(__node, 0)->off);
}

void emit_va_ptr(struct node *__node) {
	struct node *arg = get_child(__node, 0);
	bci_addr_t addr = (base_addr+arg->off)+arg->_type->size;////bcit_sizeof(_bcit_addr);//__node->_type->size;
//	bci_emit_print(_bcit_addr, addr);
//	bci_emit_dr(_bcit_addr, addr, get_rga_addr(_bcit_addr));
	bci_emit_mov(_bcit_addr, get_rga_addr(_bcit_addr), addr, 0);

//	bci_emit_assign(get_rga_addr(_bcit_addr), (mdl_u8_t**)&addr, _bcit_addr, 0);

}

void emit_bca_print(struct bca_blk *__blk) {
	bci_emit_print(__blk->bcit, __blk->addr);
}

void emit_bca_mov(struct bca_blk *__blk) {
	bci_emit_mov(__blk->bcit, __blk->dst_addr, __blk->src_addr, __blk->flags);
}

void emit_bca_assign(struct bca_blk *__blk) {
	mdl_u8_t *val;
	bci_emit_assign(__blk->addr, &val, __blk->bcit, __blk->flags);
	*val = __blk->val;
}

void emit_bca_nop(struct bca_blk *__blk) {
	bci_emit_nop(__blk->flags);
}

void emit_bca_incr(struct bca_blk *__blk) {
	bci_emit_incr(__blk->bcit, __blk->addr, __blk->bc, 0);
}

void emit_bca_decr(struct bca_blk *__blk) {
	bci_emit_decr(__blk->bcit, __blk->addr, __blk->bc, 0);
}

void emit_bca_eeb_init(struct bca_blk *__blk) {
	bci_emit_eeb_init(__blk->eeb_c, __blk->flags);
}

void emit_bca_eeb_put(struct bca_blk *__blk) {
	bci_emit_eeb_put(__blk->eeb_id, __blk->b_addr, __blk->e_addr, __blk->flags);
}

void emit_bca_fld(struct bca_blk *__blk) {
	bci_addr_t *addr;
	bci_emit_assign(__blk->addr, (mdl_u8_t**)&addr, _bcit_addr, 0);
	*addr = base_addr+*(bci_addr_t*)__blk->p;
}

void emit_bca_fst(struct bca_blk *__blk) {
	bci_emit_mov(__blk->bcit, base_addr+*(bci_addr_t*)__blk->p, __blk->addr, 0);
}

void emit_bca_dr(struct bca_blk *__blk) {
	bci_emit_dr(__blk->bcit, __blk->src_addr, __blk->dst_addr);
}

void emit_bca(struct node *__node) {
	struct bca_blk *itr = (struct bca_blk*)vec_begin(&__node->_vec);
	while(itr != NULL) {
		printf("bca: ---------> %s\n", bca_blk_kind_as_str(itr->kind));
		switch(itr->kind) {
			case BCA_PRINT:
				emit_bca_print(itr);
			break;
			case BCA_MOV:
				emit_bca_mov(itr);
			break;
			case BCA_ASSIGN:
				emit_bca_assign(itr);
			break;
			case BCA_NOP:
				emit_bca_nop(itr);
			break;
			case BCA_INCR:
				emit_bca_incr(itr);
			break;
			case BCA_DECR:
				emit_bca_decr(itr);
			break;
			case BCA_EEB_INIT:
				emit_bca_eeb_init(itr);
			break;
			case BCA_EEB_PUT:
				emit_bca_eeb_put(itr);
			break;
			case BCA_FLD:
				emit_bca_fld(itr);
			break;
			case BCA_FST:
				emit_bca_fst(itr);
			break;
			case BCA_DR:
				emit_bca_dr(itr);
			break;
		}
		vec_itr((void**)&itr, VEC_ITR_DOWN, 1);
	}
}

void emit_expr(struct node *__node) {
	if (!__node) return;
	printf("gen: node_kind: %s\n", node_kind_as_str(__node->kind));
	switch(__node->kind) {
		case AST_INCR: case AST_DECR:
			emit_incr_or_decr(__node, 0);
		break;
		case AST_EXTERN_CALL:
			emit_extern_call(__node);
		break;
		case AST_CONV:
			emit_conv(__node);
		break;
		case AST_PRINT:
			emit_print(__node);
		break;
		case AST_ASSIGN:
			emit_assign(__node);
		break;
		case AST_LITERAL:
			emit_literal(__node);
		break;
		case AST_DECL:
			emit_decl(__node);
		break;
		case AST_EXIT:
			emit_exit();
		break;
		case AST_FUNC:
			emit_func(__node);
		break;
		case AST_FUNC_CALL:
			emit_func_call(__node);
		break;
		case AST_VAR:
			emit_var(__node);
		break;
		case AST_IF:
			emit_if(__node);
		break;
		case AST_WHILE:
			emit_while(__node);
		break;
		case AST_COMPOUND_STMT:
			emit_compound_stmt(__node);
		break;
		case AST_LABEL:
			emit_label(__node);
		break;
		case AST_GOTO:
			emit_goto(__node);
		break;
		case AST_ADDROF:
			emit_addrof(__node);
		break;
		case AST_DEREF:
			emit_deref(__node);
		break;
		case AST_RETURN:
			emit_return(__node);
		break;
		case OP_CAST:
			emit_cast(__node);
		break;
		case AST_STRUCT_REF:
			emit_load_struct_ref(__node);
		break;
		case AST_BCA:
			emit_bca(__node);
		break;
		case AST_VA_PTR:
			emit_va_ptr(__node);
		break;
		default:
			emit_binop(__node);
	}
}

void lable_lookup(struct label_t **__label, char *__name) {
	for (struct label_t *itr = (struct label_t*)vec_begin(&labels); itr != NULL; vec_itr((void**)&itr, VEC_ITR_DOWN, 1)) {
		if (!strcmp(itr->name, __name)) {*__label = itr;return;}
	}
}

void link_gotos() {
	for (struct goto_t *itr = (struct goto_t*)vec_begin(&gotos); itr != NULL; vec_itr((void**)&itr, VEC_ITR_DOWN, 1)) {
		struct label_t *label = NULL;
		lable_lookup(&label, itr->name);

		if (label != NULL) {
			printf("heuhuehehihehfheihfuehfiufehfiufheuifuheiufhj --> %u\n", label->addr);
			(*itr->jmp_addr) = label->addr;
		}
	}
}

bcc_err_t gen(struct node *__node) {
	if (__node == NULL) return BCC_SUCCESS;

	print_node(__node);
	emit_expr(__node);

	for (mdl_u8_t *itr = (mdl_u8_t*)buff_begin(&bc_buff); itr != bcb_itr; itr++) {
		mdl_u8_t *t = itr+8;
		for (;itr != t; itr++) {
			if (itr >= bcb_itr) {printf("bcb_size: %u\n", bcb_itr-(mdl_u8_t*)buff_begin(&bc_buff));return BCC_SUCCESS;}
			printf("%x,	", *itr);
		}
		printf("\n");
	}

	return BCC_SUCCESS;
}
