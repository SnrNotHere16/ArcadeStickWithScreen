//*****************************************************************************
//
// buttons.c - Evaluation board driver for push buttons.
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

#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "drivers/buttons.h"

//*****************************************************************************
//
//! \addtogroup buttons_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// Holds the current, debounced state of each button.  A 1 in a bit indicates
// that that button is currently pressed, otherwise it is released.
// We assume that we start with all the buttons released (though if one is
// pressed when the application starts, this will be detected).
//
//*****************************************************************************
static uint16_t g_ui16ButtonStates = ALL_BUTTONS;

//*****************************************************************************
//
//! Polls the current state of the buttons and determines which have changed.
//!
//! \param pui16Delta points to a character that will be written to indicate
//! which button states changed since the last time this function was called.
//! This value is derived from the debounced state of the buttons.
//! \param pui16RawState points to a location where the raw button state will
//! be stored.
//!
//! This function should be called periodically by the application to poll the
//! pushbuttons.  It determines both the current debounced state of the buttons
//! and also which buttons have changed state since the last time the function
//! was called.
//!
//! In order for button debouncing to work properly, this function should be
//! called at a regular interval, even if the state of the buttons is not needed
//! that often.
//!
//! If button debouncing is not required, the the caller can pass a pointer
//! for the \e pui16RawState parameter in order to get the raw state of the
//! buttons.  The value returned in \e pui16RawState will be a bit mask where
//! a 1 indicates the buttons is pressed.
//!
//! \return Returns the current debounced state of the buttons where a 1 in the
//! button ID's position indicates that the button is pressed and a 0
//! indicates that it is released.
//
//*****************************************************************************
uint16_t
ButtonsPoll(uint16_t *pui16Delta, uint16_t *pui16RawState)
{
    uint32_t ui32Delta;
    uint32_t ui32Data;
    static uint16_t ui16SwitchClockA = 0;
    static uint16_t ui16SwitchClockB = 0;

    //
    // Read the raw state of the push buttons.  Save the raw state
    //  if the caller supplied storage for the
    // raw value.
    //
    ui32Data  = (ROM_GPIOPinRead(BUTTONS_GPIO_BASE3,  GPIO_PIN_4)<<10) |
            (ROM_GPIOPinRead(BUTTONS_GPIO_BASE2,  GPIO_PIN_0)<<12) |
                (ROM_GPIOPinRead(BUTTONS_GPIO_BASE2,  GPIO_PIN_4)<<9)  |
               ((ROM_GPIOPinRead(BUTTONS_GPIO_BASE1, ALL_BUTTONS1))<<4)|
                (ROM_GPIOPinRead(BUTTONS_GPIO_BASE, ALL_BUTTONS));




    if(pui16RawState)
    {
        *pui16RawState =(uint16_t)ui32Data;
    }

    //
    // Determine the switches that are at a different state than the debounced
    // state.
    //
    ui32Delta = ui32Data ^ g_ui16ButtonStates;

    //
    // Increment the clocks by one.
    //
    ui16SwitchClockA ^= ui16SwitchClockB;
    ui16SwitchClockB = ~ui16SwitchClockB;

    //
    // Reset the clocks corresponding to switches that have not changed state.
    //
    ui16SwitchClockA &= ui32Delta;
    ui16SwitchClockB &= ui32Delta;

    //
    // Get the new debounced switch state.
    //
    g_ui16ButtonStates &= ui16SwitchClockA | ui16SwitchClockB;
    g_ui16ButtonStates |= (~(ui16SwitchClockA | ui16SwitchClockB)) & ui32Data;

    //
    // Determine the switches that just changed debounced state.
    //
    ui32Delta ^= (ui16SwitchClockA | ui16SwitchClockB);

    //
    // Store the bit mask for the buttons that have changed for return to
    // caller.
    //
    if(pui16Delta)
    {
        *pui16Delta = (uint16_t)ui32Delta;
    }

    //
    // Return the debounced buttons states to the caller.  Invert the bit
    // sense so that a '1' indicates the button is pressed, which is a
    // sensible way to interpret the return value.
    //
    return(~g_ui16ButtonStates);
}

//*****************************************************************************
//
//! Initializes the GPIO pins used by the board pushbuttons.
//!
//! This function must be called during application initialization to
//! configure the GPIO pins to which the pushbuttons are attached.  It enables
//! the port used by the buttons and configures each button GPIO as an input
//! with a weak pull-down.
//!
//! \return None.
//
//*****************************************************************************
void
ButtonsInit(void)
{
    //
    // Enable the GPIO port to which the pushbuttons are connected.
    //
    ROM_SysCtlPeripheralEnable(BUTTONS_GPIO_PERIPH);
    ROM_SysCtlPeripheralEnable(BUTTONS_GPIO_PERIPH1);
    ROM_SysCtlPeripheralEnable(BUTTONS_GPIO_PERIPH2);
    ROM_SysCtlPeripheralEnable(BUTTONS_GPIO_PERIPH3);

    //
    // Unlock PB0 & PC0 so we can change it to a GPIO input
    // Once we have enabled (unlocked) the commit register then re-lock it
    // to prevent further changes.  PF0 & PC0 is muxed with NMI thus a special case.
    //
    HWREG(BUTTONS_GPIO_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(BUTTONS_GPIO_BASE + GPIO_O_CR) |= 0x01;
    HWREG(BUTTONS_GPIO_BASE + GPIO_O_LOCK) = 0;

    HWREG(BUTTONS_GPIO_BASE1 + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(BUTTONS_GPIO_BASE1 + GPIO_O_CR) |= 0x01;
    HWREG(BUTTONS_GPIO_BASE1 + GPIO_O_LOCK) = 0;

    HWREG(BUTTONS_GPIO_BASE2 + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(BUTTONS_GPIO_BASE2 + GPIO_O_CR) |= 0x01;
    HWREG(BUTTONS_GPIO_BASE2 + GPIO_O_LOCK) = 0;

    HWREG(BUTTONS_GPIO_BASE3 + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(BUTTONS_GPIO_BASE3 + GPIO_O_CR) |= 0x01;
    HWREG(BUTTONS_GPIO_BASE3 + GPIO_O_LOCK) = 0;

    //
    // Set each of the button GPIO pins as an input with a pull-down.
    //
    ROM_GPIODirModeSet(BUTTONS_GPIO_BASE, ALL_BUTTONS, GPIO_DIR_MODE_IN);
    MAP_GPIOPadConfigSet(BUTTONS_GPIO_BASE, ALL_BUTTONS,
                         GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPD );

    ROM_GPIODirModeSet(BUTTONS_GPIO_BASE1, ALL_BUTTONS1, GPIO_DIR_MODE_IN);
    MAP_GPIOPadConfigSet(BUTTONS_GPIO_BASE1, ALL_BUTTONS1,
                         GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPD );

    ROM_GPIODirModeSet(BUTTONS_GPIO_BASE2, ALL_BUTTONS2, GPIO_DIR_MODE_IN);
    MAP_GPIOPadConfigSet(BUTTONS_GPIO_BASE2, ALL_BUTTONS2,
                         GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU );

    ROM_GPIODirModeSet(BUTTONS_GPIO_BASE3, ALL_BUTTONS3, GPIO_DIR_MODE_IN);
    MAP_GPIOPadConfigSet(BUTTONS_GPIO_BASE3, ALL_BUTTONS3,
                         GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPD );

    //
    // Initialize the debounced button state with the current state read from
    // the GPIO bank.
    //
    g_ui16ButtonStates =
            (ROM_GPIOPinRead(BUTTONS_GPIO_BASE3,  GPIO_PIN_4)<<10) |
            (ROM_GPIOPinRead(BUTTONS_GPIO_BASE2,  GPIO_PIN_0)<<12) |
            (ROM_GPIOPinRead(BUTTONS_GPIO_BASE2,  GPIO_PIN_4)<<9) |
            (ROM_GPIOPinRead(BUTTONS_GPIO_BASE1, ALL_BUTTONS1)<<4)|
            ROM_GPIOPinRead(BUTTONS_GPIO_BASE, ALL_BUTTONS);



}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
