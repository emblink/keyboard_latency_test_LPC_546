/*
 * Copyright (c) 2015 - 2016, Freescale Semiconductor, Inc.
 * Copyright 2016 - 2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "usb_host_config.h"
#include "usb_host.h"
#include "fsl_device_registers.h"
#include "usb_host_hid.h"
#include "board.h"
#include "host_keyboard_mouse.h"
#include "host_keyboard.h"
#include "host_mouse.h"
#include "fsl_common.h"
#if (defined(FSL_FEATURE_SOC_SYSMPU_COUNT) && (FSL_FEATURE_SOC_SYSMPU_COUNT > 0U))
#include "fsl_sysmpu.h"
#endif /* FSL_FEATURE_SOC_SYSMPU_COUNT */
#include "app.h"
#include "board.h"

#if ((!USB_HOST_CONFIG_KHCI) && (!USB_HOST_CONFIG_EHCI) && (!USB_HOST_CONFIG_OHCI) && (!USB_HOST_CONFIG_IP3516HS))
#error Please enable USB_HOST_CONFIG_KHCI, USB_HOST_CONFIG_EHCI, USB_HOST_CONFIG_OHCI, or USB_HOST_CONFIG_IP3516HS in file usb_host_config.
#endif

#include "pin_mux.h"
#include <stdbool.h>
#include "fsl_power.h"
#include "fsl_ctimer.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*!
 * @brief host callback function.
 *
 * device attach/detach callback function.
 *
 * @param deviceHandle          device handle.
 * @param configurationHandle   attached device's configuration descriptor information.
 * @param eventCode             callback event code, please reference to enumeration host_event_t.
 *
 * @retval kStatus_USB_Success              The host is initialized successfully.
 * @retval kStatus_USB_NotSupported         The application don't support the configuration.
 */
static usb_status_t USB_HostEvent(usb_device_handle deviceHandle,
		usb_host_configuration_handle configurationHandle, uint32_t eventCode);

/*!
 * @brief application initialization.
 */
static void USB_HostApplicationInit(void);

extern void USB_HostClockInit(void);
extern void USB_HostIsrEnable(void);
extern void USB_HostTaskFn(void *param);
void BOARD_InitHardware(void);

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*! @brief USB host mouse instance global variable */
extern usb_host_mouse_instance_t g_HostHidMouse;
/*! @brief USB host keyboard instance global variable */
extern usb_host_keyboard_instance_t g_HostHidKeyboard;
usb_host_handle g_HostHandle;

/*******************************************************************************
 * Code
 ******************************************************************************/
#if (defined(USB_HOST_CONFIG_OHCI) && (USB_HOST_CONFIG_OHCI > 0U))
void USB0_IRQHandler(void)
{
    USB_HostOhciIsrFunction(g_HostHandle);
}
#endif /* USB_HOST_CONFIG_OHCI */
#if (defined(USB_HOST_CONFIG_IP3516HS) && (USB_HOST_CONFIG_IP3516HS > 0U))
void USB1_IRQHandler(void) {
	USB_HostIp3516HsIsrFunction(g_HostHandle);
}
#endif /* USB_HOST_CONFIG_IP3516HS */

void USB_HostClockInit(void) {
#if ((defined USB_HOST_CONFIG_OHCI) && (USB_HOST_CONFIG_OHCI > 0U))
    CLOCK_EnableUsbfs0HostClock(kCLOCK_UsbSrcUsbPll, 48000000U);
#if ((defined FSL_FEATURE_USBFSH_USB_RAM) && (FSL_FEATURE_USBFSH_USB_RAM > 0U))
    for (int i = 0; i < (FSL_FEATURE_USBFSH_USB_RAM >> 2); i++)
    {
        ((uint32_t *)FSL_FEATURE_USBFSH_USB_RAM_BASE_ADDRESS)[i] = 0U;
    }
#endif
#endif /* USB_HOST_CONFIG_OHCI */

#if ((defined USB_HOST_CONFIG_IP3516HS) && (USB_HOST_CONFIG_IP3516HS > 0U))
	CLOCK_EnableUsbhs0HostClock(kCLOCK_UsbSrcUsbPll, 48000000U);
#if ((defined FSL_FEATURE_USBHSH_USB_RAM) && (FSL_FEATURE_USBHSH_USB_RAM > 0U))
	for (int i = 0; i < (FSL_FEATURE_USBHSH_USB_RAM >> 2); i++) {
		((uint32_t*) FSL_FEATURE_USBHSH_USB_RAM_BASE_ADDRESS)[i] = 0U;
	}
#endif
#endif /* USB_HOST_CONFIG_IP3511HS */
}

void USB_HostIsrEnable(void) {
	uint8_t irqNumber;
#if ((defined USB_HOST_CONFIG_OHCI) && (USB_HOST_CONFIG_OHCI > 0U))
    IRQn_Type usbHsIrqs[] = {(IRQn_Type)USB0_IRQn};
    irqNumber             = usbHsIrqs[CONTROLLER_ID - kUSB_ControllerOhci0];
#endif /* USB_HOST_CONFIG_OHCI */
#if ((defined USB_HOST_CONFIG_IP3516HS) && (USB_HOST_CONFIG_IP3516HS > 0U))
	IRQn_Type usbHsIrqs[] = { (IRQn_Type) USB1_IRQn };
	irqNumber = usbHsIrqs[CONTROLLER_ID - kUSB_ControllerIp3516Hs0];
#endif /* USB_HOST_CONFIG_IP3511HS */

	/* Install isr, set priority, and enable IRQ. */
#if defined(__GIC_PRIO_BITS)
	GIC_SetPriority((IRQn_Type)irqNumber, USB_HOST_INTERRUPT_PRIORITY);
#else
	NVIC_SetPriority((IRQn_Type) irqNumber, USB_HOST_INTERRUPT_PRIORITY);
#endif
	EnableIRQ((IRQn_Type) irqNumber);
}

void USB_HostTaskFn(void *param) {
#if ((defined USB_HOST_CONFIG_OHCI) && (USB_HOST_CONFIG_OHCI > 0U))
    USB_HostOhciTaskFunction(param);
#endif /* USB_HOST_CONFIG_OHCI */
#if ((defined USB_HOST_CONFIG_IP3516HS) && (USB_HOST_CONFIG_IP3516HS > 0U))
	USB_HostIp3516HsTaskFunction(param);
#endif /* USB_HOST_CONFIG_IP3516HS */
}

/*!
 * @brief USB isr function.
 */

static usb_status_t USB_HostEvent(usb_device_handle deviceHandle,
		usb_host_configuration_handle configurationHandle, uint32_t eventCode) {
	usb_status_t status1;
	usb_status_t status2;
	usb_status_t status = kStatus_USB_Success;

	switch (eventCode & 0x0000FFFFU) {
	case kUSB_HostEventAttach:
		status1 = USB_HostHidKeyboardEvent(deviceHandle, configurationHandle,
				eventCode);
		status2 = USB_HostHidMouseEvent(deviceHandle, configurationHandle,
				eventCode);
		if ((status1 == kStatus_USB_NotSupported)
				&& (status2 == kStatus_USB_NotSupported)) {
			status = kStatus_USB_NotSupported;
		}
		break;

	case kUSB_HostEventNotSupported:
		usb_echo("device not supported.\r\n");
		break;

	case kUSB_HostEventEnumerationDone:
		status1 = USB_HostHidKeyboardEvent(deviceHandle, configurationHandle,
				eventCode);
		status2 = USB_HostHidMouseEvent(deviceHandle, configurationHandle,
				eventCode);
		if ((status1 != kStatus_USB_Success)
				&& (status2 != kStatus_USB_Success)) {
			status = kStatus_USB_Error;
		}
		break;

	case kUSB_HostEventDetach:
		status1 = USB_HostHidKeyboardEvent(deviceHandle, configurationHandle,
				eventCode);
		status2 = USB_HostHidMouseEvent(deviceHandle, configurationHandle,
				eventCode);
		if ((status1 != kStatus_USB_Success)
				&& (status2 != kStatus_USB_Success)) {
			status = kStatus_USB_Error;
		}
		break;

	case kUSB_HostEventEnumerationFail:
		usb_echo("enumeration failed\r\n");
		break;

	default:
		break;
	}
	return status;
}

static void USB_HostApplicationInit(void) {
	usb_status_t status = kStatus_USB_Success;

	USB_HostClockInit();

#if ((defined FSL_FEATURE_SOC_SYSMPU_COUNT) && (FSL_FEATURE_SOC_SYSMPU_COUNT))
    SYSMPU_Enable(SYSMPU, 0);
#endif /* FSL_FEATURE_SOC_SYSMPU_COUNT */

	status = USB_HostInit(CONTROLLER_ID, &g_HostHandle, USB_HostEvent);
	if (status != kStatus_USB_Success) {
		usb_echo("host init error\r\n");
		return;
	}
	USB_HostIsrEnable();

	usb_echo("host init done\r\n");
}

#define TIMER CTIMER1
#define COUNT_PER_MEASUREMENT 10000
#define MEASUREMENTS_COUNT 10
#define MEASUREMENTS_PERIOD_MS 65
#define DELAY_BEFORE_START_MS 1000
#define USB_REPORT_MAX_SIZE 16
#define GPIO_PORT 0
#define GPIO_PIN 16

volatile uint32_t latency[COUNT_PER_MEASUREMENT] = { 0 };
volatile uint32_t idx = 0;
volatile bool isButtonPressed = false;
volatile bool processOutput = false;

void buttonPressEmulationCb(uint32_t foo)
{
	(void) foo;
	isButtonPressed = true;
	GPIO_PinWrite(GPIO, GPIO_PORT, GPIO_PIN, 0);
}

void startMeasurements(uint32_t foo)
{
	(void) foo;
	CTIMER_StopTimer(TIMER);
	const ctimer_match_config_t matchcofig = {
		.matchValue = MEASUREMENTS_PERIOD_MS * 1000,
		.enableCounterReset = true,
		.enableInterrupt = true
	};
	CTIMER_SetupMatch(TIMER, kCTIMER_Match_0, &matchcofig);
	static ctimer_callback_t cb_func = buttonPressEmulationCb;
	CTIMER_RegisterCallBack(TIMER, &cb_func, kCTIMER_SingleCallback);
	usb_echo("Start measurements\r\n");
	CTIMER_Reset(TIMER);
	buttonPressEmulationCb(0);
	CTIMER_StartTimer(TIMER);
}

void onUsbEnumerationDone(void)
{
	const ctimer_match_config_t matchcofig = {
		.matchValue = DELAY_BEFORE_START_MS * 1000,
		.enableCounterReset = true,
		.enableInterrupt = true
	};
	CTIMER_SetupMatch(TIMER, kCTIMER_Match_0, &matchcofig);
	static ctimer_callback_t cb_func = startMeasurements;
	CTIMER_RegisterCallBack(TIMER, &cb_func, kCTIMER_SingleCallback);
	usb_echo("Enumeration done\r\n");
	CTIMER_StartTimer(TIMER);
}

void receivedReportCb(uint8_t *data, uint32_t dataLength)
{
	if (isButtonPressed) {
		latency[idx] = TIMER->TC;
		idx++;

		// in case of corrupted report or release report
		if (data[7] != 2 || dataLength != USB_REPORT_MAX_SIZE) { /* Esc button */
			usb_echo("Unexpected Report\r\n");
			for (uint32_t i = 0; i < dataLength; i++) {
				usb_echo("[%d] = %d, ", i, data[i]);
				if (i == 10)
					usb_echo("\r\n");
			}
			usb_echo("length == %d\r\n", dataLength);
		}

		isButtonPressed = false;
		GPIO_PinWrite(GPIO, GPIO_PORT, GPIO_PIN, 1);

		if (idx >= COUNT_PER_MEASUREMENT)
			CTIMER_StopTimer(TIMER);
	} else {
		if (idx >= COUNT_PER_MEASUREMENT) {
			processOutput = true;
		}
	}
}

static void processDataOutput(void)
{
	if (!processOutput)
		return;

	processOutput = false;

	for (uint32_t i = 0; i < COUNT_PER_MEASUREMENT; i++) {
		usb_echo("%d\r\n", latency[i]);
	}
	static uint32_t count = 0;
	count++;
	if (count >= MEASUREMENTS_COUNT) {
		usb_echo("Measurement Finished\r\n", count);
		__asm("BKPT #255");
	}

	usb_echo("Start next measurement %d\r\n", count);
	idx = 0;

	CTIMER_Reset(TIMER);
	buttonPressEmulationCb(0);
	CTIMER_StartTimer(TIMER);
}

int main(void) {
	/* attach 12 MHz clock to FLEXCOMM0 (debug console) */
	CLOCK_AttachClk(BOARD_DEBUG_UART_CLK_ATTACH);

	BOARD_InitPins();
	BOARD_BootClockFROHF96M();
	BOARD_InitDebugConsole();
	POWER_DisablePD(kPDRUNCFG_PD_USB0_PHY); /*< Turn on USB Phy */
	POWER_DisablePD(kPDRUNCFG_PD_USB1_PHY); /*< Turn on USB Phy */

	GPIO_PortInit(GPIO, GPIO_PORT);
	gpio_pin_config_t pinConfig = {
		.pinDirection = kGPIO_DigitalOutput,
		.outputLogic = 1,
	};
	GPIO_PinInit(GPIO, GPIO_PORT, GPIO_PIN, &pinConfig);

	ctimer_config_t config;
	CTIMER_GetDefaultConfig(&config);
	config.prescale = SystemCoreClock / 1000000 - 1;
	CTIMER_Init(TIMER, &config);

	USB_HostApplicationInit();

	while (1) {
		USB_HostTaskFn(g_HostHandle);
		USB_HostHidKeyboardTask(&g_HostHidKeyboard);
		USB_HostHidMouseTask(&g_HostHidMouse);
		processDataOutput();
	}
}
