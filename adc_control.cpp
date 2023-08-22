/*
 * Программа для работы со спектрометром под Linux. Предполагает наличие установленных драйверов FTD2xx
 *
 * To build use the following gcc statement 
 * (assuming you have the d2xx library in the /usr/local/lib directory).
 * gcc -o simple main.c -L. -lftd2xx -Wl,-rpath /usr/local/lib
 */
#include "adc_control.h"
#include "ftd2xx.h"
//#include <QDateTime>

extern unsigned long adc_out_buff[4096];	// буфер памяти АЦП

/* Приблуды для работы с устройством АЦП */
FT_HANDLE ftHandle;					// указатель на устройство АЦП
FT_STATUS ftStatus;
DWORD BytesWritten;
DWORD BytesReceived;

/* Открытие устройства АЦП (lvl 3) */
int adc_begin(bool val_init_flag, uint* opt_array, bool force_dev, bool force_rmd) {
    if (force_rmd) system("sudo rmmod ftdi_sio usbserial > /dev/null 2>&1");
	static FT_PROGRAM_DATA Data;
	static FT_DEVICE ftDevice;
    char OriginDev_1ch[64] = "USB_spectrometric_ADC_4K";	//искомое имя старого устройства (1ch)
	char OriginDev_2ch[64] = "USB_spectrometric_4ADC_4K";	//искомое имя нового устройства (2ch)

	/* Открываем девайс на чипе FT на порту, указанном в settings.ini */
    //ftStatus = FT_Open(opt_array[0], &ftHandle);
    if((ftStatus = FT_Open(opt_array[0], &ftHandle)) != FT_OK) return ftStatus;	//Error FT - Что-то пошло не так при открытии порта
	
    if (val_init_flag) {						//Если инициализация первая (начальная), то узнаём подробности о подключённом девайсе
        bool flag_not_found = 1;

        if ((ftStatus = FT_GetDeviceInfo(ftHandle, &ftDevice, NULL, NULL, NULL, NULL)) != FT_OK) return ftStatus;     //Error 401 - Не удалось получить информацию о чипе FT
		Data.Signature1 = 0x00000000;				//MUST set Signature1 and 2 before calling FT_EE_Read
		Data.Signature2 = 0xffffffff;
		Data.Manufacturer = (char *)malloc(256);	// E.g. "Parsek"
		Data.ManufacturerId = (char *)malloc(256);	// E.g. "FT"
		Data.Description = (char *)malloc(256);		// E.g. "USB_spectrometric_ADC_4K"
        Data.SerialNumber = (char *)malloc(256);	// E.g. "FT000001" if fixed, or NULL

        if (Data.Manufacturer == NULL || Data.ManufacturerId == NULL || Data.Description == NULL || Data.SerialNumber == NULL) return 402; //Error 402 - Не удалось выделить память под данные с чипа FT
        if((ftStatus = FT_EE_Read(ftHandle, &Data)) != FT_OK) return ftStatus;                  //Error 403 - Не удалось прочитать данные с чипа FT

        /*qDebug() << "=========== FT CHIP INFO =============";
        qDebug() << "Signature1:     " << Data.Signature1;
        qDebug() << "Signature2:     " << Data.Signature2;
        qDebug() << "Manufacturer:   " << Data.Manufacturer;
        qDebug() << "ManufacturerId: " << Data.ManufacturerId;
        qDebug() << "Description:    " << Data.Description;
        qDebug() << "SerialNumber:   " << Data.SerialNumber;
        qDebug() << "=========== ============ =============";*/

		if (strcmp(OriginDev_1ch, Data.Description) == 0) {		//найдено старое устройствo на один канал (1ch)
            if (!force_dev) opt_array[1] = 0;                   //автопереопределение канала
			flag_not_found = 0;									//устройство найдено, флаг сброшен
            //qDebug() << "1ch_found";
		}
		if (strcmp(OriginDev_2ch, Data.Description) == 0) {		//найдено новое устройствo на два канала (2ch)
            if (!force_dev) if (opt_array[1] == 0) opt_array[1] = 1; //автопереопределение канала
			flag_not_found = 0;									//устройство найдено, флаг сброшен
            //qDebug() << "2ch_found";
		}
        if (flag_not_found) return 404;                         //Error 404 - Не удалось найти АЦП по имени
		
		/* сброс девайса, иначе мы продолжим общаться с чипом FT, а не с подключённым АЦП */
        if ((ftStatus = FT_ResetDevice(ftHandle)) != FT_OK) return ftStatus;	// Не удалось ресетнуть чип FT
	}

	/* Настраиваем параметры порта */
    //if (FT_SetBaudRate(ftHandle, 115200) != FT_OK) return 406;
    if((ftStatus = FT_SetBaudRate(ftHandle, opt_array[4])) != FT_OK) return ftStatus;
    if((ftStatus = FT_SetDataCharacteristics(ftHandle, FT_BITS_8, FT_STOP_BITS_1, FT_PARITY_NONE)) != FT_OK) return ftStatus;
    if((ftStatus = FT_SetFlowControl(ftHandle, FT_FLOW_RTS_CTS, 0, 0)) != FT_OK) return ftStatus;
    if((ftStatus = FT_SetDtr(ftHandle)) != FT_OK) return ftStatus;
    if((ftStatus = FT_SetRts(ftHandle)) != FT_OK) return ftStatus;

    //qDebug() << "CONNECTION - OK!";

	return 0;
}

/* Закрытие устройства АЦП (lvl 3) */
int adc_close() {
    if ((ftStatus = FT_Close(ftHandle)) != FT_OK) return ftStatus;
    return 0;
}

/* Остановка АЦП (lvl 3) */
int adc_stop(uint val_ch) {
    uchar ADC_STOP;
    if (val_ch == 2) ADC_STOP = 2;      //стоп 2-го входа
    else ADC_STOP = 0;                  //стоп 1-го входа
    if ((ftStatus = FT_Write(ftHandle, &ADC_STOP, 1, &BytesWritten)) != FT_OK) return ftStatus;
	return 0;
}

/*Запуск АЦП (lvl 3) */
int adc_start(uint val_ch) {
    uchar ADC_START;
    if (val_ch == 2) ADC_START = 3;     //старт 2-го входа
    else ADC_START = 1;                 //старт 1-го входа
    if ((ftStatus = FT_Write(ftHandle, &ADC_START, 1, &BytesWritten)) != FT_OK) return ftStatus;
	return 0;
}

/* Очистка памяти АЦП (lvl 3) */
int adc_clear_mem(uint val_ch) {
    uchar ADC_WRITE;
    DWORD mem_size;          // размер памяти АЦП в байтах

    if (val_ch == 0) {      //на 1 байт больше!
        mem_size = 16385;
        ADC_WRITE = 16;
    } else {
        mem_size = 32769;
        ADC_WRITE = 32;
    }

    uchar* cls = new uchar[mem_size];
    //uchar cls[mem_size];
    memset(cls,0,mem_size);

    if ((ftStatus = FT_Write(ftHandle, &ADC_WRITE, 1, &BytesWritten)) != FT_OK) return ftStatus;      // Не удалось отправить команду на очистку памяти
    if ((ftStatus = FT_Write(ftHandle, cls, mem_size, &BytesWritten)) != FT_OK) return ftStatus;      // Не удалось очистить память
    if (BytesWritten != mem_size) return 20;	// Error 20 - Память очистилась не полностью

	return 0;
}

int adc_write_mem(ulong* Array, uint size, uint val_ch) {
    uchar ADC_WRITE;
    DWORD mem_size;              // размер памяти АЦП в байтах

    if (val_ch == 0) {          //на 1 байт больше!
        mem_size = 16385;
        ADC_WRITE = 16;
    } else {
        mem_size = 32769;
        ADC_WRITE = 32;
    }

    uchar* wrt = new uchar[mem_size];
    memset(wrt,0,mem_size);
    union {BYTE bt[4]; DWORD chan;} t_uni;

    if (val_ch == 0 || val_ch == 1) {
        for (uint i = 0; i < size; i++) {
            t_uni.chan = Array[i];
            wrt[i+12288] = t_uni.bt[3];
            wrt[i+8192] = t_uni.bt[2];
            wrt[i+4096] = t_uni.bt[1];
            wrt[i] = t_uni.bt[0];
        }
    } else {
        for (uint i = 0; i < size; i++) {
            t_uni.chan = Array[i];
            wrt[i+28672] = t_uni.bt[3];
            wrt[i+24576] = t_uni.bt[2];
            wrt[i+20480] = t_uni.bt[1];
            wrt[i+16384] = t_uni.bt[0];
        }
    }

    if ((ftStatus = FT_Write(ftHandle, &ADC_WRITE, 1, &BytesWritten)) != FT_OK) return ftStatus;     // Не удалось отправить команду на запись памяти
    if ((ftStatus = FT_Write(ftHandle, wrt, mem_size, &BytesWritten)) != FT_OK) return ftStatus;     // Не удалось записать память
    if (BytesWritten != mem_size) return 20;    // Error 20 - Память записалась не полностью

    return 0;
}

/* Чтение памяти АЦП (lvl 3) */
int adc_read_mem(ulong (&Array)[4096], ulong *summ, uint size, uint val_ch) {
    uchar ADC_READ;
    DWORD mem_size;          // размер памяти АЦП в байтах

    if (val_ch == 0) {
        mem_size = 16384;
        ADC_READ = 8;
    } else {
        mem_size = 32768;
        ADC_READ = 16;
    }

    uchar* buf = new uchar[mem_size];
    //uchar buf[mem_size];
    memset(buf,0,mem_size);
    union {BYTE bt[4]; DWORD chan;} t_uni;
    (*summ) = 0;

    if ((ftStatus = FT_Write(ftHandle, &ADC_READ, 1, &BytesWritten)) != FT_OK) return ftStatus;		// Не удалось отправить команду на чтение памяти АЦП
    if ((ftStatus = FT_SetTimeouts(ftHandle,1000,0)) != FT_OK) return ftStatus;		//PORT READ TIMEOUT
    if ((ftStatus = FT_Read(ftHandle, buf, mem_size, &BytesReceived)) == FT_OK) {
        if (BytesReceived == mem_size) {		// FT_Read OK
            (*summ) = 0;
            /* DUMP FOR DEBUG */
            /*char filenamedump[31];
            qint64 curr_sec = QDateTime::currentMSecsSinceEpoch();
            sprintf(filenamedump,"dump-%d-%lld.txt", val_ch, curr_sec);
            FILE *filedump;
            filedump = fopen(filenamedump, "w");*/
            /*****************/
            for(uint i=0; i<size; i++) {
                if (val_ch == 2) {
                    t_uni.bt[0] = buf[i+16384];
                    t_uni.bt[1] = buf[i+20480];
                    t_uni.bt[2] = buf[i+24576];
                    t_uni.bt[3] = buf[i+28672];
                    Array[i] = t_uni.chan;
                    (*summ) += Array[i];
                    /* DUMP */
                    //fprintf(filedump, "%04d | %02x %02x %02x %02x | %lu\n", i, t_uni.bt[0], t_uni.bt[1], t_uni.bt[2], t_uni.bt[3], t_uni.chan);
                    /********/
                } else {
                    t_uni.bt[0] = buf[i];
                    t_uni.bt[1] = buf[i+4096];
                    t_uni.bt[2] = buf[i+8192];
                    t_uni.bt[3] = buf[i+12288];
                    Array[i] = t_uni.chan;
                    (*summ) += t_uni.chan;
                    /* DUMP */
                    //fprintf(filedump, "%04d | %02x %02x %02x %02x | %lu\n", i, t_uni.bt[0], t_uni.bt[1], t_uni.bt[2], t_uni.bt[3], t_uni.chan);
                    /********/
                }
            }
            /* DUMP */
            //fclose (filedump);
            /********/
		} else {
            return 21;				// Error 21 - Не удалось прочитать память АЦП (Read Timeout)
		}
	} else {
        return ftStatus;            // Не удалось прочитать память АЦП (Read Failed)
	}

    return 0;
}

/* Сброс логики АЦП (lvl 3) */
int adc_logic_reset() {
    uchar ADC_RESET = 128;			//байт на ресет
    if ((ftStatus = FT_Write(ftHandle, &ADC_RESET, 1, &BytesWritten)) != FT_OK) return ftStatus;         // Не удалось отправить команду на сброс логики (ADC_RESET)
	return 0;
}

/* Сброс порогов АЦП (lvl 3) */
int adc_thld_reset(uint val_ch) {
    uchar ADC_CHANGE = 64;  //изменение порога на один шаг из 128 (0-127)
    uchar ADC_GRUBO;        //изменение порога грубо
    uchar ADC_TOCHN;        //изменение порога точно
    uchar ADC_MINUS;        //изменение порога в минус (только для 2ch ADC)
    uchar ADC_SEL_CH;       //выбор входа

	if (val_ch == 0) {
		ADC_GRUBO = 0;		//изменение порога грубо в минус
		ADC_TOCHN = 4;		//изменение порога точно в минус
	}
	if (val_ch == 1) {
		ADC_GRUBO = 12;		//изменение порога грубо
		ADC_TOCHN = 13;		//изменение порога точно
		ADC_MINUS = 8;		//изменение порога в минус
		ADC_SEL_CH = 10;	//выбор 1-го входа
	}
	if (val_ch == 2) {
		ADC_GRUBO = 12;		//изменение порога грубо
		ADC_TOCHN = 13;		//изменение порога точно
		ADC_MINUS = 8;		//изменение порога в минус
		ADC_SEL_CH = 11;	//выбор 2-го входа
	}

	int itr;
	/* Сброс порога "грубо" */
    if (val_ch != 0) {		//дополнительные команды для 2ch ADC
        if ((ftStatus = FT_Write(ftHandle, &ADC_MINUS, 1, &BytesWritten)) == FT_OK) {
            if ((ftStatus = FT_Write(ftHandle, &ADC_SEL_CH, 1, &BytesWritten)) != FT_OK) return ftStatus; // Не удалось отправить команду на сброс порога (ADC_SEL_CH)
		} else {
            return ftStatus; // Не удалось отправить команду на сброс порога (ADC_MINUS)
		}
	}
    if ((ftStatus = FT_Write(ftHandle, &ADC_GRUBO, 1, &BytesWritten)) == FT_OK) for(itr=0;itr<130;itr++) FT_Write(ftHandle, &ADC_CHANGE, 1, &BytesWritten);
    else return ftStatus;    // Не удалось отправить команду на сброс порога "грубо" (ADC_GRUBO)
	
	/* Сброс порога "точно" */
    if (val_ch != 0) {		//дополнительные команды для 2ch ADC
        if ((ftStatus = FT_Write(ftHandle, &ADC_MINUS, 1, &BytesWritten)) == FT_OK) {
            if ((ftStatus = FT_Write(ftHandle, &ADC_SEL_CH, 1, &BytesWritten)) != FT_OK) return ftStatus; // Не удалось отправить команду на сброс порога (ADC_SEL_CH)
		} else {
            return ftStatus; // Не удалось отправить команду на сброс порога (ADC_MINUS)
		}
	}
    if ((ftStatus = FT_Write(ftHandle, &ADC_TOCHN, 1, &BytesWritten)) == FT_OK) for(itr=0;itr<130;itr++) FT_Write(ftHandle, &ADC_CHANGE, 1, &BytesWritten);
    else return ftStatus;    // Не удалось отправить команду на сброс порога "грубо" (ADC_TOCHN)

	return 0;						// Нормальное завершение.
}

/* Установка порогов АЦП (lvl 3) */
int adc_thld_set(uint val_ch, uint val_thld_grubo, uint val_thld_tochn) {
    uchar ADC_CHANGE = 64;			//изменение порога, один шаг из 128 (0-127)
    uchar ADC_GRUBO;			//изменение порога грубо
    uchar ADC_TOCHN;			//изменение порога точно
    uchar ADC_PLUS;			//изменение порога в плюс (только для 2ch ADC)
    uchar ADC_SEL_CH;		//выбор входа
	
	if (val_ch == 0) {
		ADC_GRUBO = 2;		//изменение порога грубо в плюс
		ADC_TOCHN = 6;		//изменение порога точно в плюс
	}
	if (val_ch == 1) {
		ADC_GRUBO = 12;		//изменение порога грубо
		ADC_TOCHN = 13;		//изменение порога точно
		ADC_PLUS = 9;		//изменение порога в плюс
		ADC_SEL_CH = 10;	//выбор 1-го входа
	}
	if (val_ch == 2) {
		ADC_GRUBO = 12;		//изменение порога грубо
		ADC_TOCHN = 13;		//изменение порога точно
		ADC_PLUS = 9;		//изменение порога в плюс
		ADC_SEL_CH = 11;	//выбор 2-го входа
	}
	
	/* Установка порога "грубо" */
    if (val_ch != 0) {		//дополнительные команды для 2ch ADC
        if ((ftStatus = FT_Write(ftHandle, &ADC_PLUS, 1, &BytesWritten)) == FT_OK) {
            if ((ftStatus = FT_Write(ftHandle, &ADC_SEL_CH, 1, &BytesWritten)) != FT_OK) return ftStatus; // Не удалось отправить команду на установку порога (ADC_SEL_CH)
        } else return ftStatus; // Не удалось отправить команду на установку порога (ADC_PLUS)
    }
    if ((ftStatus = FT_Write(ftHandle, &ADC_GRUBO, 1, &BytesWritten)) == FT_OK) {
        for(uint its=0;its<val_thld_grubo;its++) FT_Write(ftHandle, &ADC_CHANGE, 1, &BytesWritten);
    } else return ftStatus;     // Не удалось отправить команду на установку порога "грубо" (ADC_GRUBO)

	/* Установка порога "точно" */
    if (val_ch != 0) {		//дополнительные команды для 2ch ADC
        if ((ftStatus = FT_Write(ftHandle, &ADC_PLUS, 1, &BytesWritten)) == FT_OK) {
            if ((ftStatus = FT_Write(ftHandle, &ADC_SEL_CH, 1, &BytesWritten)) != FT_OK) return ftStatus; // Не удалось отправить команду на установку порога (ADC_SEL_CH)
        } else return ftStatus; // Не удалось отправить команду на установку порога (ADC_PLUS)
    }
    if ((ftStatus = FT_Write(ftHandle, &ADC_TOCHN, 1, &BytesWritten)) == FT_OK) for(uint its=0;its<val_thld_tochn;its++) FT_Write(ftHandle, &ADC_CHANGE, 1, &BytesWritten);
    else return ftStatus;		// Не удалось отправить команду на установку порога "точно" (ADC_TOCHN)

	return 0;
}

int settings_ini(uint* opt_array) {
    char filenameini[31];
    sprintf(filenameini,"settings.ini");
    FILE *fileini;
    fileini = fopen(filenameini, "r");

    if(fileini == NULL){		//если файла нет, то используем стандартные значения
        //qDebug() << "SETTING.INI not found |" << fileini;
        opt_array[0] = 0;   // номер устройства FTDI на порту
        opt_array[1] = 0;   // номер рабочего канала АЦП
        opt_array[2] = 16;  // значение порога "грубо"
        opt_array[3] = 0;   // значение порога "точно"
        opt_array[4] = 921600;   // baudrate
        fileini = fopen(filenameini, "w");				//пересоздаём файл с настройками заново
        fprintf(fileini, "%01d             //номер устройства FTDI на порту (обычно 0)\n", opt_array[0]);
        fprintf(fileini, "%07d       //baudrate (default is 921600)\n", opt_array[4]);
        fprintf(fileini, "%01d             //количество каналов в устройстве\n", 1);
        fprintf(fileini, "%01d             //выбранный канал анализатора\n", 1);
        fprintf(fileini, "%03d           //порог АЦП (грубо)\n", opt_array[2]);
        fprintf(fileini, "%03d           //порог АЦП (точно)\n", opt_array[3]);
        fclose (fileini);								//закрываем файл с настройками
        return 501;										//Error 501 - Нет файла настроек, используем настройки по умолчанию
    } else {
        //qDebug() << "SETTING.INI found |" << fileini;
        bool errflag = 0;
        int opt_dev;
        int opt_chan;
        char pr_str [2];
        char ch_str [2];
        char spec_str [3];
        char coarse_str [4];
        char fine_str [4];
        char baudrate_str [8];
        char tmp_buff [100];

        if (fgets(pr_str, 2, fileini) != NULL ) {		//читаем первую сторку (номер устройства FTDI на порту)
            opt_array[0] = atoi(pr_str);				//переводим считанный текст вы цифру
            fgets (tmp_buff, 100, fileini);				//дочитываем строку до конца
        } else {
            opt_array[0] = 0;							//а если не прочиталось, то используем стандартное значение
            errflag = 1;
        }
        if (fgets(baudrate_str, 8, fileini) != NULL ) {		//читаем сторку (baudrate)
            opt_array[4] = atoi(baudrate_str);				//переводим считанный текст вы цифру
            fgets (tmp_buff, 100, fileini);				//дочитываем строку до конца
        } else {
            opt_array[4] = 921600;							//а если не прочиталось, то используем стандартное значение
            errflag = 1;
        }
        if (fgets(ch_str, 2, fileini) != NULL ) {		//читаем вторую сторку (количество каналов)
            opt_dev = atoi(ch_str);                     //переводим считанный текст вы цифру
            fgets (tmp_buff, 100, fileini);				//дочитываем строку до конца
        } else {
            opt_dev = 1;                                //а если не прочиталось, то используем стандартное значение
            errflag = 1;
        }
        if (fgets(spec_str, 3, fileini) != NULL ) {		//читаем третью сторку (номер рабочего канала)
            opt_chan = atoi(spec_str);                  //переводим считанный текст вы цифру
            fgets (tmp_buff, 100, fileini);				//дочитываем строку до конца
        } else {
            opt_chan = 1;                               //а если не прочиталось, то используем стандартное значение
            errflag = 1;
        }
        if (fgets(coarse_str, 4, fileini) != NULL ) {	//читаем четвёртую сторку ("грубо")
            opt_array[2] = atoi(coarse_str);			//переводим считанный текст вы цифру
            fgets (tmp_buff, 100, fileini);				//дочитываем строку до конца
        } else {
            opt_array[2] = 16;							//а если не прочиталось, то используем стандартное значение
            errflag = 1;
        }
        if (fgets(fine_str, 4, fileini) != NULL) {		//читаем пятую строку ("точно")
            opt_array[3] = atoi(fine_str);				//переводим считанный текст вы цифру
            fgets (tmp_buff, 100, fileini);				//дочитываем строку до конца
        } else {
            opt_array[3] = 0;							//а если не прочиталось, то используем стандартное значение
            errflag = 1;
        }

        switch(opt_dev) {                               //создаём првильный val_ch из сочетания количества каналов и выбранного канала
        case 1:
            opt_array[1] = 0;
            break;
        case 2:
            switch(opt_chan) {
            case 1:
                opt_array[1] = 1;
                break;
            case 2:
                opt_array[1] = 2;
                break;
            default:
                opt_array[1] = 1;
                break;
            }
            break;
        default:
            opt_array[1] = 0;
            break;
        }

        fflush(NULL);
        fclose (fileini);								//закрываем файл с настройками

        if (errflag) {
            remove(filenameini);						//удаляем повреждённый файл настроек совсем
            fileini = fopen(filenameini, "w");			//пересоздаём файл с текущими настройками
            fprintf(fileini, "%01d             //номер устройства FTDI на порту (обычно 0)\n", opt_array[0]);
            fprintf(fileini, "%07d       //baudrate (default is 921600)\n", opt_array[4]);
            fprintf(fileini, "%01d             //количество каналов в устройстве\n", opt_dev);
            fprintf(fileini, "%01d             //выбранный канал анализатора\n", opt_chan);
            fprintf(fileini, "%03d           //порог АЦП (грубо)\n", opt_array[2]);
            fprintf(fileini, "%03d           //порог АЦП (точно)\n", opt_array[3]);
            fflush(NULL);
            fclose (fileini);							//закрываем файл с настройками
            return 502;									//Error 502 - Файл настроек прочитался с ошибками, файл пересоздан
        }
    }
    fflush(NULL);
    return 0;											//Нормальное завершение. Ну, или что-то на него похожее.
}

int settings_save(uint* opt_array) {
    char filenameini[31];
    sprintf(filenameini,"settings.ini");
    FILE *fileini;
    int opt_dev;
    int opt_chan;
    switch(opt_array[1]) {          //разворачиваем количество каналов и выбранный канал из val_ch
    case 0:
        opt_dev = 1;
        opt_chan = 1;
        break;
    case 1:
        opt_dev = 2;
        opt_chan = 1;
        break;
    case 2:
        opt_dev = 2;
        opt_chan = 2;
        break;
    default:
        opt_dev = 1;
        opt_chan = 1;
        break;
    }
    remove(filenameini);						//удаляем старый файл настроек
    fileini = fopen(filenameini, "w");			//пересоздаём файл с текущими настройками
    fprintf(fileini, "%01d             //номер устройства FTDI на порту (обычно 0)\n", opt_array[0]);
    fprintf(fileini, "%07d       //baudrate (default is 921600)\n", opt_array[4]);
    fprintf(fileini, "%01d             //количество каналов в устройстве\n", opt_dev);
    fprintf(fileini, "%02d            //выбранный канал анализатора\n", opt_chan);
    fprintf(fileini, "%03d           //порог АЦП (грубо)\n", opt_array[2]);
    fprintf(fileini, "%03d           //порог АЦП (точно)\n", opt_array[3]);
    fflush(NULL);
    fclose (fileini);							//закрываем файл с настройками
    return 0;
}
