#
# Makefile for the mach-sun7i
#

obj-y=core.o printk.o devices.o timer.o sunxi_dump_reg.o security_id.o
obj-y += clock/
obj-y += dma/
obj-y += gpio/
obj-y += sys_config.o
obj-$(CONFIG_CPU_FREQ) += cpu-freq/
obj-$(CONFIG_PM) += pm/
obj-$(CONFIG_NET) += rf/
obj-$(CONFIG_AW_TIME_DELAY)	+= delay.o
obj-$(CONFIG_SW_POWERNOW) += powernow.o
obj-y += a20_config/a20_config.o

ifeq ($(CONFIG_SMP),y)
obj-y += platsmp.o headsmp.o
obj-$(CONFIG_HOTPLUG_CPU) += hotplug.o

ifeq ($(CONFIG_LOCAL_TIMERS),y)
obj-y += localtimer.o
endif

endif
