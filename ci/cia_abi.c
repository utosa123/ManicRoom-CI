#include "cia_update_api.h"
#include <stddef.h>
_Static_assert(sizeof(manic_cia_result_v1)==32,"CIA result size");
_Static_assert(offsetof(manic_cia_result_v1,title_id)==8,"CIA title offset");
_Static_assert(offsetof(manic_cia_result_v1,content_bytes)==24,"CIA bytes offset");
int32_t sample(const char *r,const char *p,manic_cia_result_v1 *v,uint32_t s,
               manic_cia_cancel_v1 c,void *x);
manic_install_update_cia_v1 abi_check = &sample;
