#ifndef  __A20_CONFIG_SW__
#define  __A20_CONFIG_SW__


typedef struct
{        
    char    gpio_name[32];
    int     port; 
    int     port_num;
    int     mul_sel;
    int     pull;
    int     drv_level;
    int     data;
    int     gpio;
} a20_gpio_set_t;           


#if 0
a20_gpio_set_t  gpio_info[1];

ret = a20_Getsys_configData("lcd0_para","lcd_bl_en", (int *)gpio_info, sizeof(a20_gpio_set_t)/sizeof(int));
if(ret < 0)
{
    DE_INF("%s.lcd_bl_en not exist\n", primary_key);
}
else
{
    gpio_info->data = (gpio_info->data==0)?1:0;
    hdl = a20_GPIO_Request(gpio_info, 1);
    a20_GPIO_Release(hdl, 2);        
}

#endif
//-------------------------------------------------------------
extern int a20_Getsys_configData(char *main_name, char *sub_name, int value[], int count);

extern unsigned a20_GPIO_Request(a20_gpio_set_t *gpio_list, __u32 group_count_max);

extern int a20_GPIO_Release(u32 p_handler, __s32 if_release_to_default_status);

extern int a20_GPIO_DevSetONEPIN_IO_STATUS(u32 p_handler, __u32 if_set_to_output_status, const char *gpio_name);

extern int  a20_GPIO_DevREAD_ONEPIN_DATA(u32 p_handler, const char *gpio_name);

extern int a20_GPIO_DevWRITE_ONEPIN_DATA(u32 p_handler, __u32 value_to_gpio, const char *gpio_name);


#endif
