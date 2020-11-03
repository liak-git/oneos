#ifndef __ONEPOS_INTERFACE_H__
#define __ONEPOS_INTERFACE_H__

#include <os_types.h>
#include "onepos_src_info.h"

typedef enum
{
    ONEPOS_CLOSING      =   0,
    ONEPOS_RUNING       =   1,
    ONEPOS_SIG_RUNING   =   2,
    ONEPOS_WILL_CLOSE   =   3,
    ONEPOS_MAX_STA      =   4
}onepos_serv_sta_t;

typedef enum
{
    ONEPOS_INVAILD_TYPE =  -1,
    ONEPOS_CIRC_RUN     =   0,
    ONEPOS_SIG_RUN      =   1,
    ONEPOS_MAX_TYPE     =   2
}onepos_serv_type;

#define IS_VAILD_SEV_TYPE(type) (type > ONEPOS_INVAILD_TYPE && type < ONEPOS_MAX_TYPE)

extern os_uint32_t onepos_get_sev_pos_err(void);
extern os_bool_t onepos_set_pos_err(os_uint32_t pos_err);
extern onepos_serv_sta_t onepos_get_server_sta(void);
extern onepos_serv_type onepos_get_server_type(void);
extern os_bool_t onepos_set_server_type(onepos_serv_type type);
extern onepos_sev_pro_t onepos_get_sev_pro(void);
extern os_bool_t onepos_set_sev_pro(onepos_sev_pro_t provider);
extern onepos_pos_mode_t onepos_get_pos_mode(void);
extern os_bool_t onepos_set_pos_mode(onepos_pos_mode_t mode);
extern os_uint32_t onepos_get_pos_interval(void);
extern os_bool_t onepos_set_pos_interval(os_int32_t interval);
extern void onepos_info_print(ops_src_info_t *src_info, onepos_pos_mode_t mode);
extern void onepos_start_server(void);
extern void onepos_stop_server(void);
extern os_bool_t onepos_get_latest_position(ops_src_info_t* src_info);

#endif

