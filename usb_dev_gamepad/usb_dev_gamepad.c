//*****************************************************************************
//
// usb_dev_gamepad.c - Main routines for the gamepad example.
//
// Copyright (c) 2013-2017 Texas Instruments Incorporated.  All rights reserved.
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
#include <stdio.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"
#include "driverlib/adc.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "usblib/usblib.h"
#include "usblib/usbhid.h"
#include "usblib/device/usbdevice.h"
#include "usblib/device/usbdhid.h"
#include "usblib/device/usbdhidgamepad.h"
#include "usb_gamepad_structs.h"
#include "drivers/buttons.h"
#include "utils/uartstdio.h"
#include "ST7735.h"
#include "PLL.h"
#include "tm4c123gh6pm.h"

// Backlight (pin 10) connected to +3.3 V
// MISO (pin 9) unconnected
// SCK (pin 8) connected to PA2 (SSI0Clk)
// MOSI (pin 7) connected to PA5 (SSI0Tx)
// TFT_CS (pin 6) connected to PA3 (SSI0Fss)
// CARD_CS (pin 5) unconnected
// Data/Command (pin 4) connected to PA6 (GPIO)
// RESET (pin 3) connected to PA7 (GPIO)
// VCC (pin 2) connected to +3.3 V
// Gnd (pin 1) connected to ground

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>USB HID Gamepad Device (usb_dev_gamepad)</h1>
//!
//! This example application turns the evaluation board into USB game pad
//! device using the Human Interface Device gamepad class.  The buttons on the
//! board are reported as .
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
//! The X, Y, and Z coordinates are
//! reported using the ADC input on GPIO port E pins 1, 2, and 3.  The X input
//! is on PE3, the Y input is on PE2 and the Z input is on PE1.  These are
//! not connected to any real input so the values simply read whatever is on
//! the pins.  To get valid values the pins should have voltage that range
//! from VDDA(3V) to 0V.  The blue LED on PF5 is used to indicate gamepad
//! activity to the host and blinks when there is USB bus activity.
//
//*****************************************************************************

//*****************************************************************************
//
// The HID gamepad report that is returned to the host.
//
//*****************************************************************************
static tGamepadReport sReport;

typedef struct
{
    uint16_t i16XPos;
    uint16_t i16YPos;
    uint16_t i16RXPos;
    uint16_t i16RYPos;
    uint16_t ui16Buttons;
}
PACKED tCustomReport;
//*****************************************************************************
//
// The HID gamepad polled ADC data for the X/Y/Z coordinates.
//
//*****************************************************************************
static uint32_t g_pui32ADCData[3];



//*****************************************************************************
//
// This enumeration holds the various states that the gamepad can be in during
// normal operation.
//
//*****************************************************************************
volatile enum
{
    //
    // Not yet configured.
    //
    eStateNotConfigured,

    //
    // Connected and not waiting on data to be sent.
    //
    eStateIdle,

    //
    // Suspended.
    //
    eStateSuspend,

    //
    // Connected and waiting on data to be sent out.
    //
    eStateSending
}
g_iGamepadState;

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif

//*****************************************************************************
//
// Macro used to convert the 12-bit unsigned values to an eight bit signed
// value returned in the HID report.  This maps the values from the ADC that
// range from 0 to 2047 over to 127 to -128.
//
//*****************************************************************************
#define Convert8Bit(ui32Value)  ((int8_t)((0x7ff - ui32Value) >> 4))

//*****************************************************************************
//
// Handles asynchronous events from the HID gamepad driver.
//
// \param pvCBData is the event callback pointer provided during
// USBDHIDGamepadInit().  This is a pointer to our gamepad device structure
// (&g_sGamepadDevice).
// \param ui32Event identifies the event we are being called back for.
// \param ui32MsgData is an event-specific value.
// \param pvMsgData is an event-specific pointer.
//
// This function is called by the HID gamepad driver to inform the application
// of particular asynchronous events related to operation of the gamepad HID
// device.
//
// \return Returns 0 in all cases.
//
//*****************************************************************************
uint32_t
GamepadHandler(void *pvCBData, uint32_t ui32Event, uint32_t ui32MsgData,
               void *pvMsgData)
{
    switch (ui32Event)
    {
        //
        // The host has connected to us and configured the device.
        //
        case USB_EVENT_CONNECTED:
        {
            g_iGamepadState = eStateIdle;

            //
            // Update the status.
            //
            UARTprintf("\nHost Connected...\n");
            ST7735_OutString("\n  Host Connected...\n  MAME Controller\n  15 Buttons\n  Yellow:Print\n");
            break;
        }

        //
        // The host has disconnected from us.
        //
        case USB_EVENT_DISCONNECTED:
        {
            g_iGamepadState = eStateNotConfigured;

            //
            // Update the status.
            //
            UARTprintf("\nHost Disconnected...\n");
            ST7735_OutString("\nHost Disconnected...\n MAME Controller\n 14 Buttons\n");
            break;
        }

        //
        // This event occurs every time the host acknowledges transmission
        // of a report.  It is to return to the idle state so that a new report
        // can be sent to the host.
        //
        case USB_EVENT_TX_COMPLETE:
        {
            //
            // Enter the idle state since we finished sending something.
            //
            g_iGamepadState = eStateIdle;

            ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0);

            break;
        }

        //
        // This event indicates that the host has suspended the USB bus.
        //
        case USB_EVENT_SUSPEND:
        {
            //
            // Go to the suspended state.
            //
            g_iGamepadState = eStateSuspend;

            //
            // Suspended.
            //
            UARTprintf("\nBus Suspended\n");

            ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0);

            break;
        }

        //
        // This event signals that the host has resumed signaling on the bus.
        //
        case USB_EVENT_RESUME:
        {
            //
            // Go back to the idle state.
            //
            g_iGamepadState = eStateIdle;

            //
            // Resume signaled.
            //
            UARTprintf("\nBus Resume\n");

            break;
        }

        //
        // Return the pointer to the current report.  This call is
        // rarely if ever made, but is required by the USB HID
        // specification.
        //
        case USBD_HID_EVENT_GET_REPORT:
        {
            *(void **)pvMsgData = (void *)&sReport;

            break;
        }

        //
        // We ignore all other events.
        //
        default:
        {
            break;
        }
    }

    return(0);
}
void printButton();
//*****************************************************************************
//
// Configure the UART and its pins.  This must be called before UARTprintf().
//
//*****************************************************************************
void
ConfigureUART(void)
{
    //
    // Enable the GPIO Peripheral used by the UART.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    //
    // Enable UART0
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    //
    // Configure GPIO Pins for UART mode.
    //
    ROM_GPIOPinConfigure(GPIO_PA0_U0RX);
    ROM_GPIOPinConfigure(GPIO_PA1_U0TX);
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Use the internal 16MHz oscillator as the UART clock source.
    //
    UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);

    //
    // Initialize the UART for console I/O.
    //
    UARTStdioConfig(0, 115200, 16000000);
}

//*****************************************************************************
//
// Initialize the ADC inputs used by the game pad device.  This example uses
// the ADC pins on Port E pins 1, 2, and 3(AIN0-2).
//
//*****************************************************************************
void
ADCInit(void)
{
    int32_t ui32Chan;

    //
    // Enable the GPIOs and the ADC used by this example.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    SysCtlGPIOAHBEnable(SYSCTL_PERIPH_GPIOE);

    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    ROM_SysCtlPeripheralReset(SYSCTL_PERIPH_ADC0);

    //
    // Select the external reference for greatest accuracy.
    //
    ROM_ADCReferenceSet(ADC0_BASE, ADC_REF_EXT_3V);

    //
    // Configure the pins which are used as analog inputs.
    //
    ROM_GPIOPinTypeADC(GPIO_PORTE_AHB_BASE, GPIO_PIN_3 | GPIO_PIN_2 |
                                            GPIO_PIN_1);

    //
    // Configure the sequencer for 3 steps.
    //
    for(ui32Chan = 0; ui32Chan < 2; ui32Chan++)
    {
        //
        // Configure the sequence step
        //
        ROM_ADCSequenceStepConfigure(ADC0_BASE, 0, ui32Chan, ui32Chan);
    }

    ROM_ADCSequenceStepConfigure(ADC0_BASE, 0, 2, ADC_CTL_CH2 | ADC_CTL_IE |
                                                  ADC_CTL_END);
    //
    // Enable the sequence but do not start it yet.
    //
    ROM_ADCSequenceEnable(ADC0_BASE, 0);
}

//*****************************************************************************
//
// This is the main loop that runs the application.
//
//*****************************************************************************
int
main(void)
{
    PLL_Init();
    ST7735_InitR(INITR_REDTAB);
    ST7735_SetCursor(0,0);
    ST7735_FillScreen(0);
  //  uint8_t red, green, blue;
 //*****************************************************************************
 //
 // The HID gamepad report that is returned to the host.
 //
 //*****************************************************************************
    static tCustomReport sReportA;
//*****************************************************************************
//
// An activity counter to slow the LED blink down to a visible rate.
//
//*****************************************************************************
    static uint32_t g_ui32Updates;
    uint16_t ui16ButtonsChanged, ui16Buttons;
    bool bUpdate;

    //
    // Set the clocking to run from the PLL at 50MHz
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);

    //
    // Enable the GPIO port that is used for the on-board LED.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

    //
    // Enable the GPIO pin for the Blue LED (PF2).
    //
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_3);

    //
    // Open UART0 and show the application name on the UART.
    //
    ConfigureUART();

    UARTprintf("\033[2JTiva C Series USB gamepad device example\n");
    UARTprintf("---------------------------------\n\n");

    //
    // Not configured initially.
    //
    g_iGamepadState = eStateNotConfigured;

    //
    // Enable the GPIO peripheral used for USB, and configure the USB
    // pins.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    SysCtlGPIOAHBEnable(SYSCTL_PERIPH_GPIOD);
    ROM_GPIOPinTypeUSBAnalog(GPIO_PORTD_AHB_BASE, GPIO_PIN_4 | GPIO_PIN_5);

    //
    // Configure the GPIOS for the buttons.
    //
    ButtonsInit();

    //
    // Initialize the ADC channels.
    //
    ADCInit();

    //
    // Tell the user what we are up to.
    //
    UARTprintf("Configuring USB\n");

    //
    // Set the USB stack mode to Device mode.
    //
    USBStackModeSet(0, eUSBModeForceDevice, 0);

    //
    // Pass the device information to the USB library and place the device
    // on the bus.
    //
    USBDHIDGamepadInit(0, &g_sGamepadDevice);

    //
    // Zero out the initial report.
    //
    sReport.ui8Buttons = 0;
    sReport.i8XPos = 0;
    sReport.i8YPos = 0;
    sReport.i8ZPos = 0;
    //
    // Initialize the reports to 0.
    //
    sReportA.ui16Buttons = 0;
    sReportA.i16XPos = 0;
    sReportA.i16YPos = 0;
    sReportA.i16RXPos = 0;
    sReportA.i16YPos = 0;
    //
    // Tell the user what we are doing and provide some basic instructions.
    //
    UARTprintf("\nWaiting For Host...\n");

    //
    // Trigger an initial ADC sequence.
    //
    ADCProcessorTrigger(ADC0_BASE, 0);

    //
    // The main loop starts here.  We begin by waiting for a host connection
    // then drop into the main gamepad handling section.  If the host
    // disconnects, we return to the top and wait for a new connection.





    //
    while(1)
    {


        //
        // Wait here until USB device is connected to a host.
        //
        if(g_iGamepadState == eStateIdle)
        {
            //
            // No update by default.
            //
            bUpdate = false;

            //
            // See if the buttons updated.
            //
            ButtonsPoll(&ui16ButtonsChanged, &ui16Buttons);

            sReportA.ui16Buttons = 0;
            //Button0
            if(ui16Buttons & BUTTON0)
            {
                sReportA.ui16Buttons |= 0x0001;

            }
            //Button1
            if(ui16Buttons & BUTTON1)
            {
                sReportA.ui16Buttons |= 0x0002;

            }
            //Button2
            if(ui16Buttons & BUTTON2)
            {
                sReportA.ui16Buttons |= 0x0004;

            }
            //Button3
            if(ui16Buttons & BUTTON3)
            {
                sReportA.ui16Buttons |= 0x0008;

            }
            //Button4
            if(ui16Buttons & BUTTON4)
            {
                sReportA.ui16Buttons |= 0x0010;

            }
            //Button5
            if(ui16Buttons & BUTTON5)
            {
                sReportA.ui16Buttons |= 0x0020;

            }
            //Button6
            if(ui16Buttons & BUTTON6)
            {
                sReportA.ui16Buttons |= 0x0040;

            }
            //Button7
            if(ui16Buttons & BUTTON7)
            {
                sReportA.ui16Buttons |= 0x0080;

            }
            //Button8
            if( (ui16Buttons>>8) & 0x01)
            {
                sReportA.ui16Buttons |= 0x0100;

            }
            //Button9
            if((ui16Buttons>>8) & 0x02)
            {
                sReportA.ui16Buttons |= 0x0200;

            }
            //Button10
            if((ui16Buttons>>8) & 0x04)
            {
                sReportA.ui16Buttons |= 0x0400;

            }
            //Button11
            if((ui16Buttons>>8) & 0x08)
            {
                sReportA.ui16Buttons |= 0x0800;

            }
            //Button12 - PF so negative logic
            if(!((ui16Buttons>>8) & 0x10))
            {
                sReportA.ui16Buttons |= 0x1000;

            }
            //Button13 - PF so negative logic
            if(!((ui16Buttons>>8) & 0x20))
            {
                sReportA.ui16Buttons |= 0x2000;

            }
            //Button14
            if((ui16Buttons>>8) & 0x40)
            {
                sReportA.ui16Buttons |= 0x4000;
                printButton();

            }


            if(ui16ButtonsChanged)
            {
                bUpdate = true;
            }

            //
            // See if the ADC updated.
            //
            if(ADCIntStatus(ADC0_BASE, 0, false) != 0)
            {
                //
                // Clear the ADC interrupt.
                //
                ADCIntClear(ADC0_BASE, 0);

                //
                // Read the data and trigger a new sample request.
                //
                ADCSequenceDataGet(ADC0_BASE, 0, &g_pui32ADCData[0]);
                ADCProcessorTrigger(ADC0_BASE, 0);

                //
                // Update the report.
                //
                sReportA.i16XPos = Convert8Bit(g_pui32ADCData[0]);
                sReportA.i16YPos = Convert8Bit(g_pui32ADCData[1]);
                bUpdate = true;
            }

            //
            // Send the report if there was an update.
            //
            if(bUpdate)
            {


                USBDHIDGamepadSendReport(&g_sGamepadDevice, &sReportA,
                                         sizeof(sReportA));


                //
                // Now sending data but protect this from an interrupt since
                // it can change in interrupt context as well.
                //
                IntMasterDisable();
                g_iGamepadState = eStateSending;
                IntMasterEnable();

                //
                // Limit the blink rate of the LED.
                //
                if(g_ui32Updates++ == 40)
                {
                    //
                    // Turn on the Green LED.
                    //
                    ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, GPIO_PIN_3);

                    //
                    // Reset the update count.
                    //
                    g_ui32Updates = 0;
                }
            }
        }
    }
}

void printButton(){
    //*****************************************************************************
    //
    // The HID gamepad report that is returned to the host.
    //
    //*****************************************************************************
       static tCustomReport sReportA;
   //*****************************************************************************
   //
   // An activity counter to slow the LED blink down to a visible rate.
   //
   //*****************************************************************************
       static uint32_t g_ui32Updates;
       uint16_t ui16ButtonsChanged, ui16Buttons;
       bool bUpdate;
       ST7735_SetCursor(0,0);
       ST7735_FillScreen(0);
       ST7735_OutString("\n  PRINT BUTTON\n  Press Buttons\n");

    while (1){
    //
    // Wait here until USB device is connected to a host.
    //
    if(g_iGamepadState == eStateIdle)
    {
        //
        // No update by default.
        //
        bUpdate = false;

        //
        // See if the buttons updated.
        //
        ButtonsPoll(&ui16ButtonsChanged, &ui16Buttons);

        sReportA.ui16Buttons = 0;

        if(ui16Buttons & BUTTON0)
        {
            sReportA.ui16Buttons |= 0x0001;
            ST7735_OutString("  Button1\r");

        }

        if(ui16Buttons & BUTTON1)
        {
            sReportA.ui16Buttons |= 0x0002;
            ST7735_OutString("  Button2\r");
        }

        if(ui16Buttons & BUTTON2)
        {
            sReportA.ui16Buttons |= 0x0004;
            ST7735_OutString("  Button3\r");
        }
        if(ui16Buttons & BUTTON3)
        {
            sReportA.ui16Buttons |= 0x0008;
            ST7735_OutString("  Button4\r");
        }
        if(ui16Buttons & BUTTON4)
        {
            sReportA.ui16Buttons |= 0x0010;
            ST7735_OutString("  Button5\r");
        }
        if(ui16Buttons & BUTTON5)
        {
            sReportA.ui16Buttons |= 0x0020;
            ST7735_OutString("  Button6\r");
        }
        if(ui16Buttons & BUTTON6)
        {
            sReportA.ui16Buttons |= 0x0040;
            ST7735_OutString("  Button7\r");
        }
        if(ui16Buttons & BUTTON7)
        {
            sReportA.ui16Buttons |= 0x0080;
            ST7735_OutString("  Button8\r");
        }

        if( (ui16Buttons>>8) & 0x01)
        {
            sReportA.ui16Buttons |= 0x0100;
            ST7735_OutString("  Button9\r");
        }

        if((ui16Buttons>>8) & 0x02)
        {
            sReportA.ui16Buttons |= 0x0200;
            ST7735_OutString("  Button10\r");
        }
        if((ui16Buttons>>8) & 0x04)
        {
            sReportA.ui16Buttons |= 0x0400;
            ST7735_OutString("  Button11\r");
        }
        if((ui16Buttons>>8) & 0x08)
        {
            sReportA.ui16Buttons |= 0x0800;
            ST7735_OutString("  Button12\r");
        }
        if((ui16Buttons>>8) & 0x10)
        {
            sReportA.ui16Buttons |= 0x1000;
            ST7735_OutString("  Button13\r");
        }
        if((ui16Buttons>>8) & 0x20)
        {
            sReportA.ui16Buttons |= 0x2000;
            ST7735_OutString("  Button14\r");
        }

        if((ui16Buttons>>8) & 0x40)
        {
            sReportA.ui16Buttons |= 0x4000;
            ST7735_OutString("  Button15\r");
        }


        if(ui16ButtonsChanged)
        {
            bUpdate = true;
        }

        //
        // See if the ADC updated.
        //
        if(ADCIntStatus(ADC0_BASE, 0, false) != 0)
        {
            //
            // Clear the ADC interrupt.
            //
            ADCIntClear(ADC0_BASE, 0);

            //
            // Read the data and trigger a new sample request.
            //
            ADCSequenceDataGet(ADC0_BASE, 0, &g_pui32ADCData[0]);
            ADCProcessorTrigger(ADC0_BASE, 0);

            //
            // Update the report.
            //
            sReportA.i16XPos = Convert8Bit(g_pui32ADCData[0]);
            sReportA.i16YPos = Convert8Bit(g_pui32ADCData[1]);
            bUpdate = true;
        }

        //
        // Send the report if there was an update.
        //
        if(bUpdate)
        {


            USBDHIDGamepadSendReport(&g_sGamepadDevice, &sReportA,
                                     sizeof(sReportA));


            //
            // Now sending data but protect this from an interrupt since
            // it can change in interrupt context as well.
            //
            IntMasterDisable();
            g_iGamepadState = eStateSending;
            IntMasterEnable();

            //
            // Limit the blink rate of the LED.
            //
            if(g_ui32Updates++ == 40)
            {
                //
                // Turn on the Green LED.
                //
                ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, GPIO_PIN_3);

                //
                // Reset the update count.
                //
                g_ui32Updates = 0;
            }
        }
    }
}
}



