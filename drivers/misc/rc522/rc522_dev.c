#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <asm/uaccess.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <asm/atomic.h>
#include <mach/sys_config.h>
#include <linux/mutex.h>
#include <linux/spi/spi.h>
#include <linux/gpio.h>

#include "rc522_dev.h"
#include "rc522.h"
#include "card_number_buffer.h"
static volatile int flags = 1, have_card_number = 0, can_read_card = 0, poll_start = 0;
extern int spi2_clk, spi2_miso, spi2_mosi, spi2_cs0, spi2_reset; 
static DECLARE_WAIT_QUEUE_HEAD(read_waitq);
static DECLARE_WAIT_QUEUE_HEAD(queue_waitq);
static DECLARE_WAIT_QUEUE_HEAD(rc522_wait);

//                ---------A密钥---不可读出-------- ****控制字***** -----B密钥----可以读出-------- 
uchar NewKey[16]={0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x07,0x80,0x69,0x00,0x00,0x00,0x00,0x00,0x00};

static unsigned char Read_Data[16]={0x00};
static unsigned char read_data_buff[16];
static unsigned char send_buffer[18];
//static uchar PassWd[6]={0x99, 0x99, 0x99, 0x99, 0x99, 0x99};
//static uchar PassWd[6]={0x88, 0x88, 0x88, 0x88, 0x88, 0x88};
static uchar PassWd[6]={0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
static uchar WriteData[16];
static unsigned char RevBuffer[30];
static unsigned char MLastSelectedSnr[4];
static enum IO_CMD {
    READ_CARD     = 1 << 0,
    CHANGE_PASSWD = 1 << 2,
    CHANGE_BLOCK  = 1 << 3,
    SET_RW_TIME   = 1 << 4,
    WRITE_CARD    = 1 << 5,
    READ_SN       = 1 << 6,
};
#define CARD_SN_LEN 4
static int card_operation = READ_CARD;

static int read_time = 20;

uchar uart_count,uart_comp;

uint KeyNum,KuaiN;
uchar bWarn,bPass;
uchar KeyTime,WaitTimes,SysTime;
uchar oprationcard,bSendID;

struct workqueue_struct *rc522_wq;
struct work_struct rc522_work;

struct spi_device *rc522_spi;
struct timer_list beep_timer, poll_timer;  
static  DEFINE_SEMAPHORE(data_lock);//定义互斥锁
static atomic_t write_buffer = ATOMIC_INIT(0);
static atomic_t read_buffer = ATOMIC_INIT(0);



static void delay_ms(uint tms)
{
   mdelay(tms);
}
#if 0 //sclu add
void Warn(void)
{
    uchar ii;
    for(ii=0;ii<3;ii++)
    {
        SET_BEEP;
        delay_ms(120);
        CLR_BEEP;
        delay_ms(120);
    }
}

void Pass(void)
{
    SET_BEEP;
    delay_ms(700);
    CLR_BEEP;
}
#endif

void InitPort(void)
{
#if 0
    spi2_miso = gpio_request_ex("rc522_para", "rc522_miso");
    if(!spi2_miso)
        printk("request spi2_miso failure.\n");
    spi2_mosi = gpio_request_ex("rc522_para", "rc522_mosi");
    if(!spi2_mosi)
        printk("request spi2_mosi failure.\n");
    spi2_clk = gpio_request_ex("rc522_para", "rc522_sclk");
    if(!spi2_clk)
        printk("request spi2_clk failure.\n");
    spi2_reset = gpio_request_ex("rc522_para", "rc522_enable");
    if(!spi2_reset)
        printk("request spi2_reset failure.\n");
    spi2_cs0 = gpio_request_ex("rc522_para", "rc522_cs0");
    if(!spi2_cs0)
        printk("request spi2_cs0 failure.\n");
#endif
#if 0
    spi2_reset = gpio_request_ex("spi2_para","spi_enable"); 
    if (!spi2_reset) {
        printk("request rc522 reset error\n");
    }   
    spi2_clk = gpio_request_ex("spi2_para","spi_sclk"); 
    if (!spi2_clk) {
        printk("request rc522 clk error\n");
    }   
    spi2_mosi = gpio_request_ex("spi2_para","spi_mosi"); 
    if (!spi2_reset) {
        printk("request rc522 mosi error\n");
    }   
    spi2_miso = gpio_request_ex("spi2_para","spi_miso"); 
    if (!spi2_reset) {
        printk("request rc522 miso error\n");
    }   
    spi2_cs0 = gpio_request_ex("spi2_para","spi_cs0"); 
    if (!spi2_cs0) {
        printk("request rc522 cs error\n");
    }   
    script_item_u gpio_set;
    int ret, type;

    type = script_get_item("rc522_para", "rc522_enable", &gpio_set);
    if (type == SCIRPT_ITEM_VALUE_TYPE_PIO) {
        printk("get rc522_enable err");
        return ;
    }
    ret = gpio_request(gpio_set.gpio.gpio, "rc522_enable");
    if(ret != 0)
        printk("request spi2_reset failure.\n");
    spi2_reset = gpio_set.gpio.gpio;
#endif
}

/**********************************************************************
functionName:putChar(BYTE sentData)
description:通过串口发送数据sentData
 **********************************************************************/
void InitRc522(void)
{
    PcdReset();
    PcdAntennaOff();  
    PcdAntennaOn();
    M500PcdConfigISOType( 'A' );
}
void InitAll(void)
{
    InitPort();
    InitRc522();
}

void ctrlprocess(void)
{
    char status;

    PcdReset();
    status=PcdRequest(PICC_REQIDL,&RevBuffer[0]);//寻天线区内未进入休眠状态的卡，返回卡片类型 2字节
    if(status!=MI_OK)
    {
        return;
    }
    status=PcdAnticoll(&RevBuffer[2]);//防冲撞，返回卡的序列号 4字节
    if(status!=MI_OK)
    {
        return;
    }
    memcpy(MLastSelectedSnr,&RevBuffer[2],4); 
    status=PcdSelect(MLastSelectedSnr);//选卡
    if(status!=MI_OK)
    {
        return;
    }
    if(oprationcard==KEYCARD)//修改密码
    {
        oprationcard=0;    
        status=PcdAuthState(PICC_AUTHENT1A,KuaiN,PassWd,MLastSelectedSnr);//
        if(status!=MI_OK)
        {
            bWarn=1;
            return;
        }
        status=PcdWrite(KuaiN,&NewKey[0]);
        if(status!=MI_OK)
        {
            bWarn=1;
            return;
        }
        bPass=1;
        PcdHalt();		
    }
    else if(oprationcard==READCARD)//读卡
    {
        oprationcard=0;
        status=PcdAuthState(PICC_AUTHENT1A,KuaiN,PassWd,MLastSelectedSnr);//
        if(status!=MI_OK)
        {
            bWarn=1;
            return;
        }
        status=PcdRead(KuaiN,Read_Data);
        if(status!=MI_OK)
        {
            bWarn=1;
            return;
        }
#if 0 //sclu add
        for(ii=0;ii<16;ii++)
        {
            sendchar1(Read_Data[ii]);
        }
        
#endif
        memset(send_buffer, 0, sizeof(send_buffer));
        send_buffer[0] = 0xaa; //card data
        send_buffer[1] = 0xbb;
        memcpy(&send_buffer[2], Read_Data, sizeof(Read_Data));
        atomic_inc(&read_buffer);
        memset(Read_Data, 0, sizeof(Read_Data));
        bPass=1;
        PcdHalt();	
    }
    else if(oprationcard==WRITECARD)//写卡
    {
        oprationcard=0;
        status=PcdAuthState(PICC_AUTHENT1A,KuaiN,PassWd,MLastSelectedSnr);//
        if(status!=MI_OK)
        {
            bWarn=1;
            return;
        }
        status=PcdWrite(KuaiN,&WriteData[0]);
        if(status!=MI_OK)
        {
            bWarn=1;
            return;
        }	
        bPass=1;
        PcdHalt();	
    } 
    else if(oprationcard==SENDID)//发送卡号
    {
        oprationcard=0;
#if 0 //sclu add
        for(ii=0;ii<4;ii++)
        {
            sendchar1(MLastSelectedSnr[ii]);
        }
#endif
        memset(send_buffer, 0, sizeof(send_buffer));
        send_buffer[0] = 0xcc; //card number
        send_buffer[1] = 0xdd;
        memcpy(&send_buffer[2], MLastSelectedSnr, sizeof(MLastSelectedSnr));
        atomic_inc(&read_buffer);
        memset(MLastSelectedSnr, 0, sizeof(MLastSelectedSnr));
        bPass=1;
    }       					
}

#if 0 //sclu add 
void ctrl_uart(void)
{
    uchar ii;
    uart_comp=0;
    uart_count=0;
}
void rc522_main(void)
{
    InitAll();
    while(1)
    {
        KeyNum=GetKey();//取得键盘值   
        if(KeyNum==N_1)
        {
            KeyTime=15;
            sendchar1(0xaa);//串口发送数据
            oprationcard=SENDID;
        }
        if(bWarn)
        {
            bWarn=0;
            Warn();
        }
        if(bPass)
        {
            bPass=0;
            Pass();
        }
        if(uart_comp)
        {
            ctrl_uart();     
        }
        if(SysTime>=2)
        {
            SysTime=0;
            ctrlprocess();
        }
    }  
}
#endif
static int rc522_loop_work(void)
{
    int ii, ret = -1;
    char *pdata = read_data_buff;
    char status;
    /*每张卡有16个扇区，编号为0~15,每个扇区有4个块，编号为0~3；其中0扇区0块为供应商信息，不能写； 
      每个扇区的块3存放密钥A、读写控制位和密钥B，所以实际可用块共47块; 每个扇区可以设置不同密钥，所以可以实现一卡通功能*/ 

    PcdReset();
    //sclu add for test --start
    while (poll_start) {
        msleep(10);
        status=PcdRequest(PICC_REQIDL,&RevBuffer[0]);//寻天线区内未进入休眠状态的卡，返回卡片类型 2字节
        if(status!=MI_OK)
        {
            continue;
        }
        status=PcdAnticoll(&RevBuffer[2]);//防冲撞，返回卡的序列号 4字节
        if(status!=MI_OK)
        {
            printk("get card nu: no number\n");
            continue;
        } else if (card_operation == READ_SN) {
            memcpy(pdata, &RevBuffer[2], CARD_SN_LEN);
            ret = 0;
            mod_timer(&beep_timer, jiffies); 
            break;
        } 
        memcpy(MLastSelectedSnr,&RevBuffer[2],4);

        status=PcdSelect(MLastSelectedSnr);//选卡
        if(status!=MI_OK)
        {
            printk("select card: no card\n");
            continue;
        } 
        if (card_operation == WRITE_CARD) {//write card
            status=PcdAuthState(PICC_AUTHENT1A,KuaiN,PassWd,MLastSelectedSnr);
            if(status!=MI_OK)
            {
                printk("write card authorize err \n");
                continue;
            }
            status=PcdWrite(KuaiN,&WriteData[0]);
            if(status!=MI_OK)
            {
                printk("write card err, block[%d]\n", KuaiN);
                continue;
            } else {//写数据成功
                mod_timer(&beep_timer, jiffies); 
                printk("write data sucess[block %d]", KuaiN);
                ret = 0;
                break;
            } 
        } else if (card_operation == READ_CARD){//read card	
            status=PcdAuthState(PICC_AUTHENT1A,KuaiN,PassWd,MLastSelectedSnr);//读卡
            if(status!=MI_OK)
            {
                printk("read authorize card err\n");
                continue;
            }
            status=PcdRead(KuaiN,Read_Data);
            if(status!=MI_OK)
            {
                printk("read card err\n");
                continue;
            } else {//读数据成功
                int i;
                memcpy(pdata, Read_Data, sizeof(Read_Data));
                mod_timer(&beep_timer, jiffies); 
                printk("read block %d info:", KuaiN);
                for(i = 0; i < 16; i++) {
                    printk("%2.2X",pdata[i]);
                }
                printk("\n");
                ret = 0;
            }
        }
    }
    PcdHalt();	

    return ret;
    //sclu add for test --end




    while(1) {
        if (atomic_read(&write_buffer) == 1) {
            atomic_dec(&write_buffer);
            down(&data_lock);
            memcpy(RevBuffer, pdata + 2, sizeof(RevBuffer));
            up(&data_lock);
        }
        switch(RevBuffer[1])
        {
            case 0xa0:
                oprationcard=SENDID;
                break;
            case 0xa1://读数据
                oprationcard=READCARD;
                for(ii=0;ii<6;ii++)
                {
                    PassWd[ii]=RevBuffer[ii+2];
                } 
                KuaiN=RevBuffer[8];
                break;
            case 0xa2://写数据
                oprationcard=WRITECARD;
                for(ii=0;ii<6;ii++)
                {
                    PassWd[ii]=RevBuffer[ii+2];
                } 
                KuaiN=RevBuffer[8];
                for(ii=0;ii<16;ii++)
                {
                    WriteData[ii]=RevBuffer[ii+9];
                }
                break;  
            case 0xa3:
                oprationcard=KEYCARD; 
                for(ii=0;ii<6;ii++)
                {
                    PassWd[ii]=RevBuffer[ii+2];
                } 
                KuaiN=RevBuffer[8];
                for(ii=0;ii<6;ii++)
                {
                    NewKey[ii]=RevBuffer[ii+9];
                    NewKey[ii+10]=RevBuffer[ii+9];
                }
                break;
            default:
                break;                     
        }

        ctrlprocess();
    }
}

static void start_beep(unsigned long data)
{
    unsigned long time = data;

}

static void poll_time(unsigned long data)
{
    poll_start = 0;
    printk("%s out\n", __func__);
}

static int rc522_open(struct inode *inode,struct file *filp)
{

    InitAll();
    printk("rc522 open!\n");
    return 0;
}

static ssize_t rc522_read (struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
    int ret = 0;

    if (!(card_operation & (READ_CARD | READ_SN))) {
        printk("%s: card operation[%d] no conrrect\n", __func__, card_operation);
        return -EFAULT;
    }

    poll_start = 1;
    mod_timer(&poll_timer, jiffies + read_time * HZ);
    if (rc522_loop_work() == 0) {
        int len = card_operation == READ_CARD ? sizeof(read_data_buff) : CARD_SN_LEN;
        if (copy_to_user(buf, read_data_buff, len)) {
            printk("copy card number to userspace err\n");
            ret = -EFAULT;
        } else 
            ret = len;
    } else 
        ret = -ETIME;

    card_operation = READ_CARD;

    return ret; 
}

static ssize_t rc522_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
    int len, ret = 0;
    char temp_buf[128] = {0};
    char java_char[4] = {0xaa, 0xac, 0xce, 0xbb};

    len = sizeof(java_char);

    if (copy_from_user(temp_buf, buf, count)) {
        printk("%s, [line %d] copy from user err.\n", __FILE__, __LINE__);
        ret = -EFAULT;
    }

    if (!memcmp(java_char, buf, len)) {
        char arg = temp_buf[len + 1]; 

        switch (temp_buf[len]) {
            case CHANGE_PASSWD:
                card_operation = CHANGE_PASSWD;
                break;
            case CHANGE_BLOCK:
                if (arg >= 0 && arg < 64)
                    KuaiN = (int)arg;
                else 
                    ret = -EINVAL;
                break;
            case SET_RW_TIME:
                if (arg > 0)
                    read_time = (int)arg;
                else
                    ret = -EINVAL;
                break;
            case WRITE_CARD:
                card_operation = WRITE_CARD;
                break;
            case READ_CARD:
                card_operation = READ_CARD;
                break;
            case READ_SN:
                card_operation = READ_SN;
                break;
            default:
                ret = -EINVAL;
                break;
        }
    } else {
        if (card_operation == WRITE_CARD) {
            if (count > 16 || count <= 0) {
                printk("write card data larger than a block[16 bytes]\n");
                ret = -EINVAL;
            } else if (KuaiN == 0) {
                printk("block[0] is read only\n");
                ret = -EACCES;
            } else if (KuaiN < 0 || KuaiN > 63) {
                printk("block[%d] unreachable, please set the write block first", KuaiN);
                ret = -EFAULT;
            } else if ((KuaiN % 4) == 3) {
                printk("block[%d] is key block, not data block\n", KuaiN);
                ret = -EACCES;
            } else {
                memcpy(WriteData, temp_buf, count);
                poll_start = 1;
                mod_timer(&poll_timer, jiffies + read_time * HZ);
                if (rc522_loop_work() != 0)
                    ret =  -ETIME;
            }
        } else if(card_operation == CHANGE_PASSWD) {
            if (count != sizeof(PassWd)) {
                printk("pass word format err\n");
                ret = -EINVAL;
            } else {
                memcpy(PassWd, temp_buf, sizeof(PassWd));
                printk("set new key sucess\n");
            }
        } else {
            printk("%s, please used ioctl config operation first\n", __func__);
            ret = -EPERM;
        }
        card_operation = READ_CARD;
    }

    return ret;
}

static int rc522_release(struct inode *inode,struct file *filp)
{
    printk("%s\n", __func__);
    poll_start = 0;
    return 0;
}


static unsigned int rc522_poll(struct file *file, poll_table *wait)
{
    unsigned int mask = 0;

    mask |= POLLIN | POLLRDNORM;

    return mask;
}

static int rc522_probe(struct spi_device *spi)
{
    //默认读第一块(可选0 ~ 63)
    KuaiN = 1; 
    printk("%s\n", __func__);
    rc522_spi = spi;
    return 0;
}

static int rc522_remove(struct spi_device *spi)
{
    return 0;
}
static long rc522_ioctl(struct file *file, unsigned int cmd, unsigned long arg) 
{
    int ret = 0;

    printk("%s,cmd=%d, arg=%lu\n", __func__, cmd, arg);
    switch(cmd) {
        case CHANGE_PASSWD:
            card_operation = CHANGE_PASSWD;
            break;
        case CHANGE_BLOCK:
            if (arg >= 0 && arg < 64)
                KuaiN = (int)arg;
            else 
                ret = -EINVAL;
            break;
        case SET_RW_TIME:
            if (arg > 0)
                read_time = (int)arg;
            else
                ret = -EINVAL;
            break;
        case WRITE_CARD:
            card_operation = WRITE_CARD;
            break;
        case READ_CARD:
            card_operation = READ_CARD;
            break;
        case READ_SN:
            card_operation = READ_SN;
            break;
        default:
            ret = -EINVAL;
            break;
    }
    return ret;
}

static struct file_operations rc522_fops = {
    .owner           = THIS_MODULE,
    .open	         = rc522_open,
    .release	     = rc522_release,	
    .read	         = rc522_read,
    .write	         = rc522_write,
    .poll            = rc522_poll,   
    .unlocked_ioctl  = rc522_ioctl,

};

static struct miscdevice rc522_misc_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "rfid_rc522_dev",
    .fops = &rc522_fops,
};

static struct spi_driver rc522_driver = {
    .probe = rc522_probe,
    .remove = rc522_remove,
    .driver = {
        .name = "rfid_rc522",
    },
};


static int __init rfid_rc522_init(void)
{
    int res;

    /* Register the character device (atleast try) */
    printk("rfid_rc522 module init. \n");

    res =  misc_register(&rc522_misc_device);
    if(res < 0) {
        printk("device register failed with %d.\n",res);
        return res;
    }

    //rc522_wq = create_singlethread_workqueue("rfid_rc522_work");
    //INIT_WORK(&rc522_work, rc522_loop_work); 
    init_timer(&beep_timer);  
    init_timer(&poll_timer);  
    beep_timer.function = start_beep;
    poll_timer.function = poll_time;

#if 1
    res = spi_register_driver(&rc522_driver);   
    if(res < 0){
        printk("spi register %s fail\n", __FUNCTION__);
        return res;
    }
#endif
    return 0;

}

static void __exit rfid_rc522_exit(void)
{
    flags = 0;
    del_timer(&poll_timer);
    //flush_work_sync(&rc522_work);
    //destroy_workqueue(rc522_wq);

    spi_unregister_driver(&rc522_driver);
    misc_deregister(&rc522_misc_device);

    printk("rfid_rc522 exit success.\n");
}

module_init(rfid_rc522_init);
module_exit(rfid_rc522_exit);
MODULE_DESCRIPTION("rfid_rc522 driver for A10");
MODULE_LICENSE("GPL"); 
