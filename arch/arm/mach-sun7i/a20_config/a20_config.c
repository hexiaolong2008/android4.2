#include <mach/sys_config.h>
#include <mach/gpio.h>
#include <linux/gpio.h>
#include <linux/types.h>
#include <mach/system.h>
#include <linux/errno.h>
#include <mach/a20_config.h>
#include <linux/string.h>

int a20_Getsys_configData(char *main_name, char *sub_name, int value[], int count)
{
    script_item_u   val;
    script_item_value_type_e  type;
    int ret = -1;

    if(count == 1)
    {
        type = script_get_item(main_name, sub_name, &val);
        if(SCIRPT_ITEM_VALUE_TYPE_INT == type)
        {
            ret = 0;
            *value = val.val;
            printk("%s.%s=%d\n", main_name, sub_name, *value);
        }
        else
        {
            ret = -1;
            printk("fetch script data %s.%s fail\n", main_name, sub_name);
        }
    }
    else if(count == sizeof(a20_gpio_set_t)/sizeof(int))
    {
        type = script_get_item(main_name, sub_name, &val);
        if(SCIRPT_ITEM_VALUE_TYPE_PIO == type)
        {
            a20_gpio_set_t *gpio_info = (a20_gpio_set_t *)value;

            ret = 0;
            gpio_info->gpio = val.gpio.gpio;
            gpio_info->mul_sel = val.gpio.mul_sel;
            gpio_info->pull = val.gpio.pull;
            gpio_info->drv_level = val.gpio.drv_level;
            gpio_info->data = val.gpio.data;
            memcpy(gpio_info->gpio_name, sub_name, strlen(sub_name)+1);
            printk("------%s.%s gpio=%d,mul_sel=%d,data:%d\n",main_name, sub_name, gpio_info->gpio, gpio_info->mul_sel, gpio_info->data);
        }
        else
        {
            ret = -1;
            printk("fetch script data %s.%s fail\n", main_name, sub_name);
        }
    }

    return ret;
}

EXPORT_SYMBOL(a20_Getsys_configData);

u32 a20_GPIO_Request(a20_gpio_set_t *gpio_list, __u32 group_count_max)
{    
    int ret = 0;
    struct gpio_config pcfg;

    if(gpio_list == NULL)
        return 0;

    pcfg.gpio = gpio_list->gpio;
    pcfg.mul_sel = gpio_list->mul_sel;
    pcfg.pull = gpio_list->pull;
    pcfg.drv_level = gpio_list->drv_level;
    pcfg.data = gpio_list->data;
    if(0 != gpio_request(pcfg.gpio, NULL))
    {
        printk("a20_GPIO_Request failed, gpio_name=%s, gpio=%d\n", gpio_list->gpio_name, gpio_list->gpio);
        return ret;
    }

    if(0 != sw_gpio_setall_range(&pcfg, group_count_max))
    {
        printk("---:setall_range fail,gpio_name=%s,gpio=%d,mul_sel=%d\n", gpio_list->gpio_name, gpio_list->gpio, gpio_list->mul_sel);
    }
    else
    {
        printk("--a20_GPIO_Request ok,gpio_name=%s,gpio=%d,mul_sel=%d,gpio_data=%d\n", gpio_list->gpio_name, gpio_list->gpio, gpio_list->mul_sel,gpio_list->data);
        ret = pcfg.gpio;
    }

    return ret;
}

EXPORT_SYMBOL(a20_GPIO_Request);


__s32 a20_GPIO_Release(u32 p_handler, __s32 if_release_to_default_status)
{
    if(p_handler)
    {
        gpio_free(p_handler);
    }
    else
    {
        printk("a20_GPIO_Release, hdl is NULL\n");
    }
    return 0;
}

EXPORT_SYMBOL(a20_GPIO_Release);


__s32 a20_GPIO_DevSetONEPIN_IO_STATUS(u32 p_handler, __u32 if_set_to_output_status, const char *gpio_name)
{
    int ret = -1;

    if(p_handler)
    {
        if(if_set_to_output_status)
        {
            ret = gpio_direction_output(p_handler, 0);
            if(ret != 0)
            {
                printk("gpio_direction_output fail!\n");
            }
        }
        else
        {
            ret = gpio_direction_input(p_handler);
            if(ret != 0)
            {
                printk("gpio_direction_input fail!\n");
            }
        }
    }
    else
    {
        printk("a20_GPIO_DevSetONEPIN_IO_STATUS, hdl is NULL\n");
        ret = -1;
    }
    return ret;
}
EXPORT_SYMBOL(a20_GPIO_DevSetONEPIN_IO_STATUS);


__s32 a20_GPIO_DevREAD_ONEPIN_DATA(u32 p_handler, const char *gpio_name)
{
    if(p_handler)
    {
        return __gpio_get_value(p_handler);
    }
    printk("a20_GPIO_DevREAD_ONEPIN_DATA, hdl is NULL\n");
    return -1;
}
EXPORT_SYMBOL(a20_GPIO_DevREAD_ONEPIN_DATA);


__s32 a20_GPIO_DevWRITE_ONEPIN_DATA(u32 p_handler, __u32 value_to_gpio, const char *gpio_name)
{
    if(p_handler)
    {
        __gpio_set_value(p_handler, value_to_gpio);
    }
    else
    {
        printk("a20_GPIO_DevWRITE_ONEPIN_DATA, hdl is NULL\n");
    }

    return 0;
}
EXPORT_SYMBOL(a20_GPIO_DevWRITE_ONEPIN_DATA);

