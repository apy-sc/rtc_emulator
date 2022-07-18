#include <linux/module.h>
#include <linux/types.h>
#include <linux/random.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/limits.h>
#include <linux/ctype.h>

#include <linux/ioctl.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/cdev.h>


#define DEVICE_NAME "rtcN"   
#define PREFIX_NAME "rtcn" 
#define MY_MINOR 0

#define BUF_SIZE 255

#define MODE_RTC_PR 'm'
#define FACTOR_RTC_PR 'f'
#define TIME_RTC_PR 't'

/* IOCTL COMMANDS */
#define RTCN_RD_TIME _IOR(rtcN_major, 0, struct tm)
#define RTCN_SET_TIME _IOW(rtcN_major, 1, struct tm)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tretyak A.V.");
MODULE_DESCRIPTION("My RTC-emulate module without rtc_device (rtc.h). Test on kernel version - 5.13.0-52-generic.");
MODULE_VERSION("2.1"); 

static int rtcN_major = 0;
static int counter = 1;
static int is_open = 0;

/* Установка rtcN_major через INSMOD параметр */
module_param(rtcN_major, int, 0);

static struct cdev *rtc_cdev;
static dev_t rtc_dev;
static struct class *rtc_dev_class;
static struct proc_dir_entry *proc_f;


static int __init rtc_v2_init(void);
static void __exit rtc_v2_cleanup(void);

module_init(rtc_v2_init);
module_exit(rtc_v2_cleanup);


static ssize_t procf_read_time(
    struct file *filp, 
    char *buffer,     
    size_t length,    
    loff_t * offset);

static ssize_t procf_set_conf(
    struct file *filp, 
    const char *buff, 
    size_t len, 
    loff_t * off);

static int procf_open(struct inode *inode, struct file *file);

static int procf_close(struct inode *inode, struct file *file);

/*
IOCTL функция для использования приложениями
Смотрите IOCTL COMMANDS defines перед использованием
*/
static long dev_ioctl_rtc(
    struct file *file, 
    unsigned int command, 
    unsigned long arg);


static const struct proc_ops rts_proc_ops = 
{
	.proc_read = procf_read_time,
	.proc_write = procf_set_conf,
    .proc_open = procf_open,
    .proc_release = procf_close,
};

static const struct file_operations rts_dev_ops = 
{
	.owner = THIS_MODULE,
	.read = procf_read_time,
	.write = procf_set_conf, 
    .open = procf_open,
    .release = procf_close,
    .unlocked_ioctl = dev_ioctl_rtc,

};

