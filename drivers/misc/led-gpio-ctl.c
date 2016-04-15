#include <linux/module.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <mach/a20_config.h>
#include <mach/platform.h>
#include <linux/timer.h>
#include <linux/sched.h>

u32 led_hdl[2];
char *main_key = "led_para";
char *led_key[2] = {
	"led0_gpio",
	"led1_gpio",
};

struct timer_list led_timer;
static bool led_status;

void set_gpio_status(int handler, bool status)
{
	a20_GPIO_DevWRITE_ONEPIN_DATA(handler, status, NULL);
}

static void set_led_status(unsigned long arg)
{
	int i;

	if (led_status)
		led_status = false;
	else
		led_status = true;
	
	for (i = 0; i < 2; i++)
		set_gpio_status(led_hdl[i], led_status);

	mod_timer(&led_timer, jiffies + HZ);
}

static int __init led_ctl_probe(struct platform_device *pdev)
{
	a20_gpio_set_t gpio[1];
	int ret;
	int i;

	for (i = 0; i < 2; i++) {
		ret = a20_Getsys_configData(main_key, led_key[i], (int *)gpio, sizeof(a20_gpio_set_t)/sizeof(int));
		if (ret != 0) {
			printk("fetch the [%s] %s failed!\n", main_key, led_key[i]);
		}
		led_hdl[i] = a20_GPIO_Request(gpio, 1);
		if (led_hdl[i] == 0) {
			printk("a20_GPIO_Request led_hdl[%d] failed.\n", i);
		}
	}

	return 0;
}

static int __exit led_ctl_remove(struct platform_device *pdev)
{
	int i;

	del_timer(&led_timer);
	for (i = 0; i < 2; i++) {
		a20_GPIO_Release(led_hdl[i], 0);
	}

	return 0;
}

static struct platform_driver led_ctl_driver = {
	.driver		= {
		.name	= "led_ctl",
		.owner	= THIS_MODULE,
	},
	.remove		= __exit_p(led_ctl_remove),
};

static int __init led_ctl_init(void)
{
	init_timer(&led_timer);
	led_timer.expires =jiffies + HZ;
	led_timer.data = 100;
	led_timer.function = set_led_status;
	add_timer(&led_timer);

	return platform_driver_probe(&led_ctl_driver, led_ctl_probe);
}

static void __exit led_ctl_exit(void)
{
	kfree(&led_timer);
	platform_driver_unregister(&led_ctl_driver);
}

module_init(led_ctl_init);
module_exit(led_ctl_exit);

MODULE_AUTHOR("Link Gou <link.gou@gmail.com>");
MODULE_DESCRIPTION("SW power gpio control driver");
MODULE_LICENSE("GPL");
