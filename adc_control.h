/* подключаем нужные опции и заголовки */
#include <string.h>
#include <stdlib.h>              //system
#include <sys/stat.h>            //stat & mkdir
#include <stdio.h>
#include <time.h>
#include <QDebug>

extern int adc_begin(bool val_init_flag, uint* opt_array, bool force_dev, bool force_rmd);                                        // функция открытия устройства АЦП
extern int adc_read_mem(ulong (&Array)[4096], ulong* summ, uint size, uint val_ch);   // функция чтения памяти АЦП
extern int adc_write_mem(ulong* Array, uint size, uint val_ch);                 // функция чтения памяти АЦП
extern int adc_clear_mem(uint val_ch);                                          // функция очистки памяти АЦП
extern int adc_close();                                                         // функция закрытия устройства АЦП
extern int adc_start(uint val_ch);                                               // функция старта АЦП
extern int adc_stop(uint val_ch);                                                // функция остановки АЦП

extern int adc_logic_reset();                                                  // функция "сброса логики" АЦП (что бы это ни значило)
extern int adc_thld_reset(uint val_ch);                                         // функция сброса порогов счёта в нули
extern int adc_thld_set(uint val_ch, uint val_thld_grubo, uint val_thld_tochn);	// функция установки порогов АЦП

extern int settings_ini(uint* opt_array);                               // функция чтения настроек из файла settings.ini
extern int settings_save(uint* opt_array);                              // функция сохранения настроек в файл settings.ini
