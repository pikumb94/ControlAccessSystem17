/*This library provides a driver to control a micro servo SG 90 motor.
 *
 */

#ifndef _MOTOR_H_
#define _MOTOR_H_

#include "hal.h"
#include "userconf.h"

#if USERLIB_USE_MOTOR || defined(__DOXYGEN__)



#define UP_POSITION                         680
#define DOWN_POSITION                           300
#define MOTOR_FREQUENCY                       1000000
#define MOTOR_PERIOD                          20000


typedef struct {
  PWMDriver *pwmdriver;
  PWMConfig *pwmcfg;
}MotorConfig;

typedef struct {
  unsigned position;
  const MotorConfig    *config;
} MotorDriver;


#ifdef __cplusplus
extern "C" {
#endif


void motorObjectInit(MotorDriver* motorp);
void motorStart(MotorDriver* motorp, const MotorConfig *config);
void motorUp(MotorDriver* motorp);
void motorDown(MotorDriver* motorp);


#ifdef __cplusplus
}
#endif
#endif
#endif


