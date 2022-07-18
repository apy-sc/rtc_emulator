#include "rtcN.h"
#include "rtcN_time.h"

/* 
Установка параметров
RAND_MOD: Генерирует случайное значение int rand -> factor %= 61 (можно поменять), mode устанавливается случайно, за счет попадание в отрезки uint rand: 0<MAX_UINT/3<MAX_UINT/3*2<MAX_UINT
*/
static int set_param (char param, int value)
{
    switch (param)
    {
        case MODE_RTC_PR:
            if (value < 0 || value > 3) return -EINVAL;
            if (value == 3)
            {
                unsigned int rand = 0;
                get_random_bytes(&rand, sizeof(rand));
            
                if (rand >= 0 && rand < UINT_MAX/3) rtc_mode = 0;
                else if (rand >= UINT_MAX/3 && rand < (UINT_MAX/3)*2) rtc_mode = 1;
                else rtc_mode = 2;

			    factor_time = rand % 61;
                
                break;
            }
            rtc_mode = value;
            break;
        
        case FACTOR_RTC_PR:
            factor_time = value;
            break;
    }
    return 0;
}

//Вернет прочитанное число с буффера
static int my_get_val (char  *buf, int *offset)
{
    int val = 0;
    char check_val = 0;
    while (*buf != '\0' && isspace(*buf))
    {
        buf++;
        (*offset)++;
    }

    if (*buf == '\0') 
        return -EINVAL;

    while (isdigit(*buf))
	{
        val = val * 10 + *buf - '0';
        buf++;
        (*offset)++;
        check_val = 1;
    }	

    if (check_val != 1)
        val = -EINVAL;
    return val;
}

// Считывает с буффера значение даты и времени в формате --- our:min:sec day.month.year !формат не менять
static int my_get_time (char *buf, int *offset, struct  timespec64 * time)
{
    struct timespec64 tm;
    int local_offset;
    int i;
    unsigned int a[6];
    for (i = 0; i < 6; i++)
    {
        local_offset = 0;
        a[i] = my_get_val (buf, &local_offset);
        if (a[i] == -EINVAL)
            return -EINVAL;
        buf += local_offset;
        if (*buf!= ':' && *buf!= ' ' && *buf!= '.' && i!=5)
            return -EINVAL;
        buf++;
        *offset = local_offset + 1;
    }

    tm.tv_sec = mktime64(a[5], a[4], a[3], a[0], a[1], a[2]);
    time->tv_sec = tm.tv_sec;
    time->tv_nsec = tm.tv_nsec;
    return 0;
}

// parser parametrov
static int update_config (char *buf, int lenb)
{
    int offset = 0; // При вызове функций контролируем сдвиг в буффере
    char param;
    int val;
    struct timespec64 tm;

    if (lenb < 3) return -EINVAL; // Минимум 4 символа для установки параметра // формат "X Y" x - param, y - val
    param = *buf;
    if (param != 'm' && param != 'f' && param != 't')
        return -EINVAL;
    buf++;

    if (!isspace(*buf))
        return -EINVAL;
    
    if (param  == 't') // Если настраиваем новое время
    {

        if (my_get_time (buf, &offset, &tm) == -EINVAL) // Считываем время с буффера
            return -EINVAL;
        
        if (set_rtcN_time(tm) == -EINVAL) // Устанавливаем время в RTCn
            return -EINVAL;

        return 1; // Возвращаем флаг смены времени
    }
    
    if ((val = my_get_val(buf, &offset)) == -EINVAL) // Считываем int с буффера
        return -EINVAL;
    
    if (offset != lenb)
    {
        printk (KERN_INFO "Other params dont check!\n");
    }
    return set_param (param, val); // установка параметров RTCn
}

// В переменную res кладет время RTCn
static int get_rtc_mode_time (struct timespec64 * res)
{
    struct timespec64 diff_time;
    struct timespec64 temp;
    struct timespec64 view_time;

    if (res == NULL) {
        printk (KERN_INFO "Cant read time. Res struct timespec64 pointer is NULL\n");
        return -EINVAL;
    }
    if (factor_time == 0) // Если время остановлено, то не считаем смещение относительно time_start_bp
    {
        diff_time.tv_sec = 0;
        diff_time.tv_nsec = 0;
    } else 
    {
        diff_time = get_time_diff_rtcN(time_start_bp);
        temp = diff_time;
        
        switch (rtc_mode)
        {
            case NORMAL_MODE:
                break;

            case SLOW_MODE:
                set_normalized_timespec64(&temp, temp.tv_sec / (time64_t)factor_time,
                    temp.tv_nsec / (long)factor_time);
                diff_time = temp;
                break;

            case ACCELERATED_MODE:
                set_normalized_timespec64(&temp, temp.tv_sec * (time64_t)factor_time,
                    temp.tv_nsec * (long)factor_time);
                diff_time = temp;
                break;
            
            default:
                printk (KERN_INFO "Cant read time. RTC mode is uncorrect. mode = %d\n", rtc_mode);
                return -EINVAL;
        }
    }
    view_time = timespec64_add(time_start_bp , timespec64_add (diff_time, diff_set_time)); // Itogovoe vremia = time of breakpoint + (offset s uchetom factor i mode) + diff_set_time
    res->tv_sec = view_time.tv_sec;
    res->tv_nsec  = view_time.tv_nsec ;
    return 0;
}

/*************************** PROC & DEV FUNCS **************************************/

static char * get_buf(void)
{
	static char buf_msg[BUF_SIZE+1];
	return buf_msg;
}

static ssize_t procf_read_time(struct file *filp, 
    char __user *buffer,
    size_t length, 
    loff_t * offset)
{
    char *kbuf = NULL;
    int res = 0;
    struct tm view_time;
    struct timespec64 cur_rtc_time;

    res = get_rtc_mode_time (&cur_rtc_time); // Узнаем время RTCn
    if (res != 0) 
        return res;

    time64_to_tm(cur_rtc_time.tv_sec, 0, &view_time); // Преобразуем ktime format из модуля в struct tm формат для пространства пользователя

    /* Передаем инфорорацию о времени в пространство пользователя */
    kbuf = get_buf();

    sprintf(kbuf, "Time: %02d:%02d:%02d\nDate: %02d.%02d.%ld\nMode: %d\nFactor: %d\n", 
        view_time.tm_hour, view_time.tm_min, view_time.tm_sec,
        view_time.tm_mday, view_time.tm_mon+1, view_time.tm_year+1900,
        rtc_mode, factor_time);
	if (*offset >= strlen(kbuf))
    {
		*offset = 0;
		return 0;
	}
	if (length > strlen(kbuf) - *offset)
        length = strlen(kbuf) - *offset;

	res = length - copy_to_user((void*)buffer, kbuf + *offset, length);
	*offset += res;
	return res;            
}

static ssize_t procf_set_conf(struct file *filp, 
    const char __user *buffer, 
    size_t len, 
    loff_t * offset)
{
    struct timespec64 cur_rtc_time;
    char *kbuf = NULL;
    int res, res_time;
   
    kbuf = get_buf();

	if (len > BUF_SIZE)
        return -ENOMEM;

	res = len - copy_from_user(kbuf, (void*)buffer, len);
    *offset += res;
	kbuf[res] = '\0';

    /* Сохраняем время до смены параметров, чтобы потом корректно вычислить diff_set_time */
    res_time = get_rtc_mode_time (&cur_rtc_time);
    if (res_time != 0) {
        return res_time;
    }

    /* parse and set param */
    res_time = update_config (kbuf, res);
    if (res_time != 0 &&  res_time != 1)
        return -EINVAL;
    
    /* Если была произведена установка нового времени, то diff_set_time и time_start_bp уже обновлены */
    if (res_time == 1)
        return res;

    /* Если сменили mode или factor, то сменим time_start_bp и diff_set_time */
    get_real_time_rtcN(&time_start_bp);
    diff_set_time = timespec64_sub (cur_rtc_time, time_start_bp); 
    return res;
}


int procf_open(struct inode *inode, struct file *file)
{
    if (is_open) // Если файл используется другим процессом, ограничим доступ
        return -EBUSY;
    is_open = 1;
    return 0;
}


int procf_close(struct inode *inode, struct file *file)
{
    is_open = 0;
    return 0;
}


static long dev_ioctl_rtc(
    struct file *file, 
    unsigned int command, // Смотреть IOCT DEFINES в rtcN.h
    unsigned long arg) // arg from user must be struct timespec64 *
{
    void __user *uarg = (void __user *)arg;
    int res = 0;
    struct tm view_time;
    struct timespec64 cur_rtc_time;
    if (command == RTCN_RD_TIME)
    {    
        res = get_rtc_mode_time (&cur_rtc_time);
        if (res != 0) 
        {
            return res;
        }    
        // Преобразуем ktime format из модуля в struct tm формат для пространства пользователя
        time64_to_tm(cur_rtc_time.tv_sec, 0, &view_time);

		if (copy_to_user(uarg, &view_time, sizeof(view_time)))
			res = -EFAULT;
		return res;
    }
    else if (command == RTCN_SET_TIME)
    {
        if (copy_from_user(&view_time, uarg, sizeof(view_time)))
			return -EFAULT;

        // Преобразуем struct tm формат из пространсттва пользователя в ktime format для модуля
        cur_rtc_time.tv_sec = mktime64(view_time.tm_year, view_time.tm_mon,  
            view_time.tm_mday, view_time.tm_hour, view_time.tm_min, view_time.tm_sec);
		
        return set_rtcN_time(cur_rtc_time);
    }
    else return -EINVAL;
}
/*************************** PROC & DEV FUNCS ENDS **************************************/

static int __init rtc_v2_init(void)
{
    int res = 0;
    char name [50]; //Имя устройства в файловой системе proc и dev (rtcnID)
    
	/* Регистрация RTCN в системе */
    printk(KERN_INFO "Trying to register rtcN device\n");

    if (rtcN_major) // Если major задан в параметрах
    {
        rtc_dev = MKDEV (rtcN_major, MY_MINOR); // Получаем номер устройства
        res = register_chrdev_region (rtc_dev, counter, DEVICE_NAME); // Регистрируем диапозон номеров
    }
    else { // Если major не установлен, то просим ядро выделить нам его динамически
        res = alloc_chrdev_region(&rtc_dev, MY_MINOR, counter, DEVICE_NAME); // Регистрируем диапозон номеров
		rtcN_major = MAJOR(rtc_dev); // Получаем выданный major 
    }

    if (res < 0) // Если создать не получилось
    {
		printk(KERN_INFO "Registering the RTC device failed. Cant register dev region with %d major number\n", rtcN_major);
		return res;
	}

    rtc_cdev = cdev_alloc();
    cdev_init(rtc_cdev, &rts_dev_ops); // Инициализируем cdev
    rtc_cdev->owner = THIS_MODULE;

    res = cdev_add(rtc_cdev, rtc_dev, counter); // Добавляем устройство в систему

    if (res < 0) // Не удалось добавить устройство
    {
		unregister_chrdev_region(MKDEV(rtcN_major, MY_MINOR), counter);
		printk(KERN_INFO "Registering the RTC device failed. Can't add char device with %d major number\n", rtcN_major);
		return res;
	}

    printk(KERN_INFO "Registering the RTC device success with %d major number\n", rtcN_major);

    /* УСТНОВКА ВРЕМЕНИ ПО УМОЛЧАНИЮ (на время в системе) */
    
    get_real_time_rtcN(&time_start_bp);
    diff_set_time.tv_sec = 0;
    diff_set_time.tv_nsec = 0;

    /* Добавляем устройство в /dev */
    rtc_dev_class = class_create(THIS_MODULE, "my_rtcN_class"); //Создание класса устройства
	device_create(rtc_dev_class, NULL, rtc_dev, NULL, "%s%d", PREFIX_NAME, MY_MINOR); // Создание файла в /dev
	
    /* ДОБАВЛЯЕМ В ФС PROC */
    printk(KERN_INFO "Trying to create /proc/rtcN file:\n");
    
    sprintf(name, "%s%d", PREFIX_NAME, MY_MINOR); // Генерируем имя rtcnID
    proc_f = proc_create((const char *) &name, S_IFREG|S_IRUGO|S_IWUGO, NULL, &rts_proc_ops);  //Добавляем файл в фс proc

    if (!proc_f) 
    {
        res = -ENOMEM;
        printk(KERN_INFO "Error: Could not initialize /proc/rtcN file!\n");
        return res;
    } 
    printk(KERN_INFO "Success create /proc/%s file!\n", name);

    printk(KERN_INFO "SUCCESS. RTCn module installed with (%d:%d) nums of device!\n", rtcN_major, MY_MINOR);

    return 0;
}

static void __exit rtc_v2_cleanup(void)
{
    proc_remove(proc_f); 
    device_destroy(rtc_dev_class, rtc_dev);
	class_destroy(rtc_dev_class);

    if (rtc_cdev)
        cdev_del(rtc_cdev);
    unregister_chrdev_region (rtcN_major, counter);

    printk (KERN_INFO "SUCCESS. RTCn module was removed. Goodbye!\n");
}