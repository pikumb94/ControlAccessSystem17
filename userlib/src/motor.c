
#include "motor.h"

#if USERLIB_USE_MOTOR || defined(__DOXYGEN__)





void motorObjectInit(MotorDriver* motorp){
  motorp->config = NULL;
  motorp->position= DOWN_POSITION;
}
void motorStart(MotorDriver* motorp, const MotorConfig *config){
  motorp->config = config;
  pwmStart(motorp->config->pwmdriver, motorp->config->pwmcfg);
  pwmEnableChannel(motorp->config->pwmdriver, 0, PWM_PERCENTAGE_TO_WIDTH(motorp->config->pwmdriver, motorp->position));
  pwmEnableChannelNotification(motorp->config->pwmdriver, 0);
  pwmEnablePeriodicNotification(motorp->config->pwmdriver);
}

void motorUp(MotorDriver* motorp){
  motorp->position+=10;
  pwmEnableChannel(motorp->config->pwmdriver, 0, PWM_PERCENTAGE_TO_WIDTH(motorp->config->pwmdriver, motorp->position));

}

void motorDown(MotorDriver* motorp){
  motorp->position-=10;
  pwmEnableChannel(motorp->config->pwmdriver, 0, PWM_PERCENTAGE_TO_WIDTH(motorp->config->pwmdriver, motorp->position));

}


#endif
