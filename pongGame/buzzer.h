#ifndef buzzer_included
#define buzzer_included

void buzzer_init();
void buzzer_set_period(short cycles,int duration);
void buzzer_stop();
void delayPlease(long delayTime);

#endif // included
