#include <linux/ktime.h>
#include <linux/time.h> 

/*
Режимы работы RTC. 

NORMAL_MODE     - обычный
SLOW_MODE       - замедленный
ACCELERATED_MODE- ускоренный 
RANDOM_MODE     - случайный
*/
#define NORMAL_MODE 0
#define SLOW_MODE 1
#define ACCELERATED_MODE 2
#define RANDOM_MODE 3

/* 
Допустимые значения 0<=rtc_mode<=2 
При rtc_mode=3 режим работы смениться на случайный в диапозоне 0<=rtc_mode<=2 
*/
static int rtc_mode = 0;

/*
Структуры, хранящие временные метки для рассчета времени RTC
*/
static struct timespec64 time_start_bp; // Точка перерасчета времени при вызове READ_TIME
static struct timespec64 diff_set_time; // Разница во времени, при смене режима работы RTCn

/*
Коэффициент ускорения/замедления времени RTCn
При factor_time = 0  - время останавливается до смены значения
* В случае, если (rtc_mode == normal && factor > 0)  данная переменная не используется
*/
static int factor_time = 1;

/* Установка rtc_mode и factor_time через INSMOD параметры */
module_param(rtc_mode, int, 0);
module_param(factor_time, int, 0);

// Получить реальное время системы
static void get_real_time_rtcN (struct timespec64* tm)
{
    ktime_get_real_ts64(tm);
}

// Установить новое время RTCn
static int set_rtcN_time (struct  timespec64 new_time)
{
    if (!timespec64_valid(&new_time))
    {
        return -EINVAL;
    }
    get_real_time_rtcN(&time_start_bp);
    diff_set_time = timespec64_sub(new_time, time_start_bp);
    return 0;
}

// Вычисляет разницу во времени между текущим временем и start
static struct timespec64 get_time_diff_rtcN (struct timespec64 start)
{
    struct timespec64 now;
    get_real_time_rtcN(&now);
    return (timespec64_sub(now, time_start_bp));
}

