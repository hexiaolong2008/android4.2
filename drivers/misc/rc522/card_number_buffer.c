#include <linux/module.h>
#include <asm/atomic.h>
#include "card_number_buffer.h"

static unsigned char readwrite_buf[CARD_NUMBER_BUF_LEN][CARD_NUMBER_LEN];
static atomic_t read_buf  = ATOMIC_INIT(0); //定义原子变量并初始化为0
static atomic_t write_buf = ATOMIC_INIT(0); //定义原子变量并初始化为0

int isEmpty(void)
{
	if (atomic_read(&read_buf) == atomic_read(&write_buf))
		return 1;
	else
		return 0;
}

int isFull(void)
{
	int w = atomic_read(&write_buf);
	int r = atomic_read(&read_buf);

	if ((w + 1) % CARD_NUMBER_BUF_LEN == r)
		return 1;
	else
		return 0;
}

void putVal(unsigned char *val)
{
	int w = atomic_read(&write_buf);
	int r = atomic_read(&read_buf);

	//printk("putVal: r = %d, w = %d, val = 0x%x\n", r, w, val);

	if (isFull())
	{
		r = (r + 1) % CARD_NUMBER_BUF_LEN;
		atomic_set(&read_buf,  r);
	}	

	//readwrite_buf[w] = val;
    memcpy(&readwrite_buf[w][0], val, CARD_NUMBER_LEN);
	
	w = (w + 1) % CARD_NUMBER_BUF_LEN;
	atomic_set(&write_buf,  w);
}

unsigned char *getVal(unsigned char *val)
{
	int r = atomic_read(&read_buf);
	
	if (isEmpty())
		return 0;
	else
	{
		//val = readwrite_buf[r];
        memcpy(val, &readwrite_buf[r][0], CARD_NUMBER_LEN);
		
		r = (r + 1) % CARD_NUMBER_BUF_LEN;
		atomic_set(&read_buf,  r);
	}
	return val;
}


EXPORT_SYMBOL(isEmpty);
EXPORT_SYMBOL(isFull);
EXPORT_SYMBOL(putVal);
EXPORT_SYMBOL(getVal);

