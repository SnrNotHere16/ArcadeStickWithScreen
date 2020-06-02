//*****************************************************************************
//
// buttons.h - Prototypes for the evaluation board buttons driver.
//
// Copyright (c) 2012-2017 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
// 
// Texas Instruments (TI) is supplying this software for use solely and
// exclusively on TI's microcontroller products. The software is owned by
// TI and/or its suppliers, and is protected under applicable copyright
// laws. You may not combine this software with "viral" open-source
// software in order to form a larger program.
// 
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
// 
// This is part of revision 2.1.4.178 of the EK-TM4C123GXL Firmware Package.
//
//*****************************************************************************

#ifndef __BUTTONS_H__
#define __BUTTONS_H__

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

//*****************************************************************************
//
// Defines for the hardware resources used by the pushbuttons.
//
// The switches are on the following ports/pins:
//! Button0 - PB0
//! Button1 - PB1
//! Button2 - PB2
//! Button3 - PB3
//! Button4 - PB4
//! Button5 - PB5
//! Button6 - PB6
//! Button7 - PB7
//! Button8 - PC4
//! Button9 - PC5
//! Button10 - PC6
//! Button11 - PC7
//! Button12 - PF0
//! BUTTON13 - PF4
//! BUTTON14 - PA4
// The switches tie the GPIO to ground, so the GPIOs need to be configured
// with pull-ups, and a value of 1 means the switch is pressed.
//
//*****************************************************************************
#define BUTTONS_GPIO_PERIPH     SYSCTL_PERIPH_GPIOB
#define BUTTONS_GPIO_BASE       GPIO_PORTB_BASE

#define BUTTONS_GPIO_PERIPH1    SYSCTL_PERIPH_GPIOC
#define BUTTONS_GPIO_BASE1      GPIO_PORTC_BASE

#define BUTTONS_GPIO_PERIPH2    SYSCTL_PERIPH_GPIOF
#define BUTTONS_GPIO_BASE2      GPIO_PORTF_BASE

#define BUTTONS_GPIO_PERIPH3    SYSCTL_PERIPH_GPIOA
#define BUTTONS_GPIO_BASE3      GPIO_PORTA_BASE



#define NUM_BUTTONS            15
#define BUTTON0                GPIO_PIN_0
#define BUTTON1                GPIO_PIN_1
#define BUTTON2                GPIO_PIN_2
#define BUTTON3                GPIO_PIN_3
#define BUTTON4                GPIO_PIN_4
#define BUTTON5                GPIO_PIN_5
#define BUTTON6                GPIO_PIN_6
#define BUTTON7                GPIO_PIN_7

#define BUTTON8                GPIO_PIN_4
#define BUTTON9                GPIO_PIN_5
#define BUTTON10               GPIO_PIN_6
#define BUTTON11               GPIO_PIN_7
#define BUTTON12               GPIO_PIN_0
#define BUTTON13               GPIO_PIN_4
#define BUTTON14               GPIO_PIN_4


#define ALL_BUTTONS             (BUTTON0 | BUTTON1 | BUTTON2 | BUTTON3 | BUTTON4 |BUTTON5 | BUTTON6 |BUTTON7)
#define ALL_BUTTONS1            (BUTTON8 | BUTTON9 | BUTTON10 | BUTTON11 )
#define ALL_BUTTONS2            (BUTTON12 | BUTTON13)
#define ALL_BUTTONS3            (BUTTON14)

//*****************************************************************************
//
// Useful macros for detecting button events.
//
//*****************************************************************************
#define BUTTON_PRESSED(button, buttons, changed)                              \
        (((button) & (changed)) && ((button) & (buttons)))

#define BUTTON_RELEASED(button, buttons, changed)                             \
        (((button) & (changed)) && !((button) & (buttons)))

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

//*****************************************************************************
//
// Functions exported from buttons.c
//
//*****************************************************************************
extern void ButtonsInit(void);
extern uint16_t ButtonsPoll(uint16_t *pui16Delta,
                             uint16_t *pui16Raw);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

//*****************************************************************************
//
// Prototypes for the globals exported by this driver.
//
//*****************************************************************************

#endif // __BUTTONS_H__
