/*
    ChibiOS - Copyright (C) 2006..2015 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include <stdio.h>
#include <string.h>

#include "ch.h"
#include "hal.h"
#include "test.h"
#include "motor.h"
#include "mfrc522.h"
#include "rfidcodes.h"
#include "lcd.h"

#include "shell.h"
#include "chprintf.h"

/*===========================================================================*/
/* List of PINs.                                                     */
/*===========================================================================*/

#define PORT_LASER                            GPIOD
#define PIN_LASER                               1
#define PORT_MANUAL_BUTTON                    GPIOA
#define PIN_MANUAL_BUTTON                       0
#define PWM_MOTOR_PORT                        GPIOB
#define PWM_MOTOR_PIN                           11
#define PORT_SPI2_CS                          GPIOD
#define PIN_SPI2_CS                            11U
#define PORT_SPI2_MISO                        GPIOB
#define PIN_SPI2_MISO                           14
#define PORT_SPI2_MOSI                        GPIOB
#define PIN_SPI2_MOSI                          15
#define PORT_SPI2_SCK                         GPIOB
#define PIN_SPI2_SCK                            13


/*===========================================================================*/
/*LCD PINs.                                                     */
/*===========================================================================*/
#define LINE_RS                     PAL_LINE(GPIOA, 4U)
#define LINE_RW                     PAL_LINE(GPIOA, 1U)
#define LINE_E                      PAL_LINE(GPIOA, 3U)
#define LINE_A                      PAL_LINE(GPIOA, 8U)

#if !LCD_USE_4_BIT_MODE
#define LINE_D0                     PAL_LINE(GPIOC, 0U)
#define LINE_D1                     PAL_LINE(GPIOC, 1U)
#define LINE_D2                     PAL_LINE(GPIOC, 2U)
#define LINE_D3                     PAL_LINE(GPIOC, 3U)
#endif
#define LINE_D4                     PAL_LINE(GPIOB, 0U)
#define LINE_D5                     PAL_LINE(GPIOB, 10U)
#define LINE_D6                     PAL_LINE(GPIOC, 6U)
#define LINE_D7                     PAL_LINE(GPIOC, 7U)

/*===========================================================================*/
/* States                                                   */
/*===========================================================================*/


#define ACTIVE 0
#define GOING_UP_MANUAL 1
#define GOING_DOWN_MANUAL 2
#define GOING_UP_AUTOMATIC 3
#define GOING_DOWN_AUTOMATIC 4
#define GATE_OPENED 5

uint8_t state=ACTIVE;
RfidCodes codes;
MotorDriver MTRD1;
MFRC522Driver RFID1;
static MUTEX_DECL(CardIDMutex);
static MifareUID CardID;
static uint8_t txbuf[2];
static uint8_t rxbuf[2];


/*===========================================================================*/
/* Configuration Structures                                                   */
/*===========================================================================*/


/*
 *
 * LCD configuration structures.
 */

static const lcd_pins_t lcdpins = {
  LINE_RS,
  LINE_RW,
  LINE_E,
  LINE_A,
  {
#if !LCD_USE_4_BIT_MODE
   LINE_D0,
   LINE_D1,
   LINE_D2,
   LINE_D3,
#endif
   LINE_D4,
   LINE_D5,
   LINE_D6,
   LINE_D7
  }
};

static const LCDConfig lcdcfg = {
  LCD_CURSOR_OFF,           /* Cursor disabled */
  LCD_BLINKING_OFF,         /* Blinking disabled */
  LCD_SET_FONT_5X10,        /* Font 5x10 */
  LCD_SET_2LINES,           /* 2 lines */
  &lcdpins,                 /* pin map */
#if LCD_USE_DIMMABLE_BACKLIGHT
  &PWMD1,                   /* PWM Driver for back-light */
  &pwmcfg,                  /* PWM driver configuration for back-light */
  0,                        /* PWM channel */
#endif
  100,                      /* Back-light */
};




/*
 *
 * RFID1 configuration structure.
 */
static MFRC522Config RFID1_cfg = {
};

/*
 * SPI1 configuration structure.
 */
static const SPIConfig SPI2cfg = {
  NULL,
  /* HW dependent part.*/
  PORT_SPI2_CS,                                   /*   port of CS  */
  PIN_SPI2_CS,
  SPI_CR1_BR_0 | SPI_CR1_BR_1,
  SPI_CR2_DS_0|SPI_CR2_DS_1|SPI_CR2_DS_2
 };

/*
* Implement functions to write and read values using spi driver.
*/
void MFRC522WriteRegister(MFRC522Driver* mfrc522p, uint8_t addr, uint8_t val)
{
    (void)mfrc522p;
    spiSelect(&SPID2);
    txbuf[0] = (addr << 1) & 0x7E;
    txbuf[1] = val;
    spiSend(&SPID2, 2, txbuf);
    spiUnselect(&SPID2);
}

uint8_t MFRC522ReadRegister(MFRC522Driver* mfrc522p, uint8_t addr)
{
    (void)mfrc522p;
    spiSelect(&SPID2);
    txbuf[0] = ((addr << 1) & 0x7E) | 0x80;
    txbuf[1] = 0xff;
    spiExchange(&SPID2, 2, txbuf, rxbuf);
    spiUnselect(&SPID2);
    return rxbuf[1];
}

/*
* PWM & Motor Configuration
*/

static void pwmpcb(PWMDriver *pwmp) {

  (void)pwmp;
      palSetPad(PWM_MOTOR_PORT, PWM_MOTOR_PIN);
}


static void pwmc1cb(PWMDriver *pwmp) {

  (void)pwmp;
  palClearPad(PWM_MOTOR_PORT, PWM_MOTOR_PIN);
}

static PWMConfig pwmcfg = {
  MOTOR_FREQUENCY,                                    /* 1MHz PWM clock frequency.   */
  MOTOR_PERIOD,                                       /* Initial PWM period 20ms.   */
  pwmpcb,
  {
   {PWM_OUTPUT_ACTIVE_HIGH, pwmc1cb},
   {PWM_OUTPUT_DISABLED, NULL},
   {PWM_OUTPUT_DISABLED, NULL},
   {PWM_OUTPUT_DISABLED, NULL}
  },
  0,
  0
};

static MotorConfig motorcfg ={
   &PWMD1,
   &pwmcfg
};



/*===========================================================================*/
/* Shell Related code                                              */
/*===========================================================================*/

#define SHELL_WA_SIZE   THD_WORKING_AREA_SIZE(2048)
#define TEST_WA_SIZE    THD_WORKING_AREA_SIZE(256)



static void cmd_rmvcard(BaseSequentialStream *chp, int argc, char *argv[]){

  int8_t i;
  int8_t j;
  (void)argv;
  if (argc > 0) {
    chprintf(chp, "Usage: rmvcard\r\n");
    return;
  }
  chMtxLock(&CardIDMutex);
  chprintf(chp, "\r\n\Avvicinare la carta al lettore.\r\n");
  while(MifareCheck(&RFID1, &CardID) != MIFARE_OK){
    }

  i=codeIndex(&codes,&CardID);

  if(i!=-1){
      codes.size-=1;
      for(j=i;j<codes.size;j++){
          codes.validId[j]=codes.validId[j+1];
      }
      chprintf(chp, "\r\n\Carta eliminata correttamente.\r\n");
  }
  else{
    chprintf(chp, "\r\n\Carta non presente nel sistema.\r\n");
  }

  while(MifareCheck(&RFID1, &CardID) == MIFARE_OK){
    }
  CardID.size = 0;

  chMtxUnlock(&CardIDMutex);

}


static void cmd_codes(BaseSequentialStream *chp, int argc, char *argv[]){
  uint8_t i;
  uint8_t j;
  (void)argv;
  if (argc > 0) {
    chprintf(chp, "Usage: codes\r\n");
    return;
  }

  for(i=0;i<codes.size;i++){
    chprintf(chp,"Codice %d: ", i);
    for(j=0;j<codes.validId[i].size;j++){
      chprintf(chp,"%d",codes.validId[i].bytes[j]);
    }
    chprintf(chp,"\r\n");
  }
}


static void cmd_addcard(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)argv;
  if (argc > 0) {
    chprintf(chp, "Usage: addcard\r\n");

    return;
  }

  if(codes.size==MAX_ID_NUM){
    chprintf(chp,"\r\n Numero massimo di carte raggiunto.");
  }

  else{
  chMtxLock(&CardIDMutex);
  chprintf(chp, "\r\n\Avvicinare la carta al lettore.\r\n");
  while(MifareCheck(&RFID1, &CardID) != MIFARE_OK){
  }
  if(codeIndex(&codes,&CardID)==-1){
          codes.validId[codes.size]=CardID;
          codes.size++;
          chprintf(chp, "\r\n\Carta inserita correttamente.\r\n");
  }
  else{
    chprintf(chp,"\r\nCarta gia' presente\r\n");
  }
  while(MifareCheck(&RFID1, &CardID) == MIFARE_OK){
  }
  CardID.size=0;

  chMtxUnlock(&CardIDMutex);
  }
}

static void cmd_manual(BaseSequentialStream *chp, int argc, char *argv[]){
  (void)argv;
  if (argc > 0) {
    chprintf(chp, "Usage: addcard\r\n");
    return;
  }
  chprintf(chp,"addcard: aggiunge una nuova carta al sistema di accessi\r\nrmvcard: rimuove una carta dal sistema di accessi\n\rcodes: mostra tutti i codici al momento autorizzati\n\r");
}


static const ShellCommand commands[] = {
  {"addcard", cmd_addcard},
  {"rmvcard", cmd_rmvcard},
  {"codes", cmd_codes},
  {"manual", cmd_manual},
  {NULL, NULL}
};

static const ShellConfig shell_cfg1 = {
  (BaseSequentialStream *)&SD1,
  commands
};




/*===========================================================================*/
/* Application behaviour                                                           */
/*===========================================================================*/

/*
 * Orange LED blinker thread, times are in milliseconds.
 */
static THD_WORKING_AREA(waThread1, 128);
static THD_FUNCTION(Thread1, arg) {

  (void)arg;
  chRegSetThreadName("Blinker");
  while (true) {
    palClearPad(GPIOE, GPIOE_LED5_ORANGE);
    chThdSleepMilliseconds(500);
    palSetPad(GPIOE, GPIOE_LED5_ORANGE);
    chThdSleepMilliseconds(500);
  }
}


/*
 * This thread change the state accordingly to the position of the motor and to the laser
 */
static THD_WORKING_AREA(waThread2, 128);
static THD_FUNCTION(Thread2, arg) {

  (void)arg;
  chRegSetThreadName("Position Manager");
  while (true) {

    switch(state){

            case(GATE_OPENED):
                if(!palReadPad(PORT_LASER,PIN_LASER))
                state=GOING_DOWN_AUTOMATIC;
                  break;

            case(GOING_UP_AUTOMATIC):
                if(MTRD1.position==UP_POSITION){
                  state=GATE_OPENED;
                  chThdSleepMilliseconds(7000);
                }
                break;

            case(GOING_DOWN_AUTOMATIC):
                 if(MTRD1.position==DOWN_POSITION){
                   state=ACTIVE;
                   lcdWriteString(&LCDD1, "Buongiorno.     ", 0);
                   lcdWriteString(&LCDD1, "CAS17           ",40);

                 }
                break;

            case(GOING_DOWN_MANUAL):
                if(MTRD1.position==DOWN_POSITION){
                  state=ACTIVE;
                  lcdWriteString(&LCDD1, "Buongiorno.     ", 0);
                  lcdWriteString(&LCDD1, "CAS17           ",40);

                }
                break;

          }
    chThdSleepMilliseconds(100);

  }
}


/*
 * This thread polls the RFID reader for a valid tag.
 */
static THD_WORKING_AREA(waThread3, 128);
static THD_FUNCTION(Thread3, arg) {

  (void)arg;
  chRegSetThreadName("RFID manager");
  while (true) {


    chMtxLock(&CardIDMutex);
    if(state==ACTIVE){
       if (MifareCheck(&RFID1, &CardID) == MIFARE_OK) {

           if(codeIndex(&codes,&CardID)!=-1){
             state=GOING_UP_AUTOMATIC;

             lcdWriteString(&LCDD1, "Accesso Valido. ", 0);
             lcdWriteString(&LCDD1, "CAS17           ",40);

           }

           else{


             lcdWriteString(&LCDD1, "Accesso Negato. ", 0);
             lcdWriteString(&LCDD1, "CAS17           ",40);

           }


         } else {

             CardID.size = 0;
         }
    }
    chMtxUnlock(&CardIDMutex);
         if(state == GOING_UP_AUTOMATIC){
           chThdSleepMilliseconds(2000);
         }
         chThdSleepMilliseconds(100);
  }
}



/*
 * This thread polls the user button for manual usage.
 */
static THD_WORKING_AREA(waThread4, 128);
static THD_FUNCTION(Thread4, arg) {

  (void)arg;
  chRegSetThreadName("Button Manager");
  while (true) {


    if(palReadPad(PORT_MANUAL_BUTTON,PIN_MANUAL_BUTTON)){

      switch(state){

        case(ACTIVE):
             state=GOING_UP_MANUAL;
              break;
        case(GOING_UP_MANUAL):
            if(!palReadPad(PORT_LASER,PIN_LASER))
                  state=GOING_DOWN_MANUAL;
            break;

        case(GOING_DOWN_MANUAL):
            state=GOING_UP_MANUAL;
            break;

      }

    }
    chThdSleepMilliseconds(230);

  }
}




/*
 * This thread controls the motion of the motor.
 */
static THD_WORKING_AREA(waThread5, 128);
static THD_FUNCTION(Thread5, arg) {

  (void)arg;
  chRegSetThreadName("Motor Controller");
  while (true) {
    if((state==GOING_UP_AUTOMATIC||state==GOING_UP_MANUAL) && MTRD1.position<UP_POSITION){
      motorUp(&MTRD1);
      chThdSleepMilliseconds(5);
    }
    if((state==GOING_DOWN_AUTOMATIC||state==GOING_DOWN_MANUAL) && MTRD1.position>DOWN_POSITION){
      motorDown(&MTRD1);
      chThdSleepMilliseconds(5);
    }


    chThdSleepMilliseconds(100);

  }
}

/*
 * This thread polls the laser.
 */

static THD_WORKING_AREA(waThread6, 128);
static THD_FUNCTION(Thread6, arg) {

  (void)arg;
  chRegSetThreadName("Laser Manager");
  while (true) {


    if(palReadPad(PORT_LASER,PIN_LASER)){

      switch(state){

        case(GOING_DOWN_MANUAL):
             state=GOING_UP_MANUAL;
              break;
        case(GOING_DOWN_AUTOMATIC):
            state=GOING_UP_AUTOMATIC;
            break;

      }

    }
    chThdSleepMilliseconds(230);

  }
}



/*
 * Application entry point.
 */
int main(void) {
  thread_t *shelltp = NULL;

  codes.size=0;

  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();

  lcdInit();

#if LCD_USE_DIMMABLE_BACKLIGHT

  palSetLineMode(LINE_A, PAL_MODE_ALTERNATE(1));
#else

  palSetLineMode(LINE_A, PAL_MODE_OUTPUT_PUSHPULL |
                 PAL_STM32_OSPEED_HIGHEST);
#endif

  /* Configuring RW, RS and E PIN as Output Push Pull. Note that Data PIN are
     managed Internally */
  palSetLineMode(LINE_RW, PAL_MODE_OUTPUT_PUSHPULL |
                PAL_STM32_OSPEED_HIGHEST);
  palSetLineMode(LINE_RS, PAL_MODE_OUTPUT_PUSHPULL |
                PAL_STM32_OSPEED_HIGHEST);
  palSetLineMode(LINE_E, PAL_MODE_OUTPUT_PUSHPULL |
                PAL_STM32_OSPEED_HIGHEST);


  lcdStart(&LCDD1, &lcdcfg);

  lcdWriteString(&LCDD1, "Buongiorno.     ", 0);
  lcdWriteString(&LCDD1, "CAS17           ",40);

//Configuring PWM pin as Output Push Pull.
  palSetPadMode(PWM_MOTOR_PORT, PWM_MOTOR_PIN,PAL_MODE_OUTPUT_PUSHPULL);


  //Configuring SPI2 pins, required for comunication with RFID reader.
  palSetPadMode(PORT_SPI2_SCK, PIN_SPI2_SCK,
                PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_HIGHEST);    /* New SCK */
  palSetPadMode(PORT_SPI2_MISO, PIN_SPI2_MISO,
                PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_HIGHEST);    /* New MISO*/
  palSetPadMode(PORT_SPI2_MOSI, PIN_SPI2_MOSI,
                PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_HIGHEST);    /* New MOSI*/
  palSetPadMode(PORT_SPI2_CS, PIN_SPI2_CS,
                PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_HIGHEST); /* New CS*/
  palSetPad(PORT_SPI2_CS, PIN_SPI2_CS);

  //Initialzing and starting RFID driver
  MFRC522ObjectInit(&RFID1);
  spiStart(&SPID2, &SPI2cfg);
  MFRC522Start(&RFID1, &RFID1_cfg);

  //Initialzing and starting Motor driver
  motorObjectInit(&MTRD1);
  motorStart(&MTRD1,&motorcfg);


  /*
   * Initializes Serial Driver 1.
   */
  sdStart(&SD1, NULL);
  palSetGroupMode(GPIOC, PAL_PORT_BIT(4) | PAL_PORT_BIT(5), 0,
                  PAL_MODE_ALTERNATE(7));

  /*
   * Shell manager initialization.
   */
  shellInit();

  /*
   * Creates the threads.
   */
  chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);
  chThdCreateStatic(waThread2, sizeof(waThread2), NORMALPRIO, Thread2, NULL);
  chThdCreateStatic(waThread3, sizeof(waThread3), NORMALPRIO, Thread3, NULL);
  chThdCreateStatic(waThread4, sizeof(waThread4), NORMALPRIO, Thread4, NULL);
  chThdCreateStatic(waThread5, sizeof(waThread5), NORMALPRIO, Thread5, NULL);
  chThdCreateStatic(waThread6, sizeof(waThread6), NORMALPRIO, Thread6, NULL);



  /*
   * Normal main() thread activity, in this demo it does nothing except
   * sleeping in a loop and check the button state.
   */
  while (true) {
    if (!shelltp)
      shelltp = shellCreate(&shell_cfg1, SHELL_WA_SIZE, NORMALPRIO);
    else if (chThdTerminatedX(shelltp)) {
      chThdRelease(shelltp);    /* Recovers memory of the previous shell.   */
      shelltp = NULL;           /* Triggers spawning of a new shell.        */
    }
    chThdSleepMilliseconds(1000);
  }
}
