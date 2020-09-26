#ifndef _USERCONF_H_
#define _USERCONF_H_



/**
 * @brief   Enables the RFID subsystem.
 */
#if !defined(USERLIB_USE_MFRC522) || defined(__DOXYGEN__)
#define USERLIB_USE_MFRC522             TRUE
#endif



/**
 * @brief   Enables the Motor subsystem.
 */
#if !defined(USERLIB_USE_MOTOR) || defined(__DOXYGEN__)
#define USERLIB_USE_MOTOR             TRUE
#endif

/**
 * @brief   Enables the LCD subsystem.
 */
#if !defined(USERLIB_USE_LCD) || defined(__DOXYGEN__)
#define USERLIB_USE_LCD             TRUE
#endif


/**
 * @brief   Enables the RFID codes management library.
 */
#if !defined(USERLIB_USE_RFIDCODES) || defined(__DOXYGEN__)
#define USERLIB_USE_RFIDCODES             TRUE
#endif

/*===========================================================================*/
/* LCD driver related settings.                                              */
/*===========================================================================*/

/**
 * @brief   Enables 4 BIT mode.
 * @note    Enabling this option LCD uses only D4 to D7 pins
 */
#define LCD_USE_4_BIT_MODE          FALSE

/**
 * @brief   Enables backlight APIs.
 * @note    Enabling this option LCD requires a PWM driver
 */
#define LCD_USE_DIMMABLE_BACKLIGHT  FALSE

#endif


