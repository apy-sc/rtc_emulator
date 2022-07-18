2022, Artyom T.
Open source project. 

RTC emulator.

Описание
----------------
Модуль ядра Linux для виртуального устройства (класса RTC).
Функции модуля:
* эмулирует устройство с неравномерным подсчетом времени (ускоренное/замедленное/случайное)
* поддерживает операции установки/чтения значений
* имеет возможность настойки параметров и получения статистики через интерфейс /proc
* имеет возможность динамической загрузки (insmod/rmmod)
* встраивается в /dev как дополнительное устройство rtcN


MAKE
----------------
Для сборки модуля требуется прописать

	$ sudo make


LOAD
----------------
Для динамической загрузки модуля используйте

	$ sudo make load			(Загрузка без аргументов)

	$ sudo insmod rtcN.ko [arg=val]		(С использованием аргументов)
		ARG
			rtcN_major:	MAJOR номер устройства
			rtc_mode:	режим работы rtcn устройства
			factor_time: 	коэфициент ускорения/замедления времени
						 0 - остановка часов
		
		VAL
			arg == rtcN_major: 	val > 0 
			arg == rtc_mode:	0 < val < 4	(См. rtcN_time.h #define)
			arg == factor_time: 	0 <= val
			
		примеры: 	sudo insmod rtcN.ko rtcN_major=201
				sudo insmod rtcN.ko rtc_mode=2
				sudo insmod rtcN.ko factor_time=0	



READ RTC
----------------
Для получения информации о состоянии RTCn используйте /proc/rtcnID (ID = 0 для одного устройства)
	
	$ sudo cat /dev/rtcn0

Также, можно воспользоваться вызовом ioctl через /dev/rtcnID
	
	COMMAND:
		#define RTCN_RD_TIME _IOR(rtcN_major, 0, struct tm)


CONFIG (SET) RTC
----------------
Для настройки состояния RTCn используйте /proc/rtcnID (ID = 0 для одного устройства)

	$ sudo echo [param val]
		PARAM
			1. t: 	устаановка нового времени. 
					val format:  "OURS.MINS.SECS DAY:MON:YEAR" 
					пример: sudo echo t 12.07.01 18.07.2022 > /proc/rtc1
			
			2. f:	установка значения коэфициента.
					val format: "UNSIGNED_INT"
					пример: sudo echo f 100 > /proc/rtc1
					
			3. m:	установка значения режима работы RTCn.
					val format: "0<val<4"
					пример: sudo echo m 3 > /proc/rtc1

Также, можно воспользоваться вызовом ioctl для установки времени через /dev/rtcnID
	
	COMMAND:
		#define RTCN_SET_TIME _IOW(rtcN_major, 1, struct tm)
	
	
UNLOAD & CLEAN
----------------
	$ sudo make unload
	$ sudo make clean



2022, Artyom T.
