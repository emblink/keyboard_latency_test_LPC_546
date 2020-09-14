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
#include "sdcard_fatfs.h"
#include "fsl_debug_console.h"
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

	usb_echo("Usb host init done, plug in keyboard\r\n");
}

/************************************** TEST CODE **************************************/
typedef enum {
	ButtonStatePressed = 0,
	ButtonStateReleased = 1,
} ButtonState;

#define TIMER CTIMER1
#define COUNT_PER_MEASUREMENT 1000
#define DELAY_BEFORE_START_MS 1000
#define GPIO_PORT 0
#define GPIO_PIN 16

static volatile char latency[COUNT_PER_MEASUREMENT * 8] = {'\0'};
static volatile uint32_t idx = 0;
static volatile uint32_t measured = 0;
static volatile bool pressedReportReceived = false;
static volatile bool releasedReportReceived = false;
static volatile bool processOutput = false;
static volatile ButtonState buttonState = ButtonStateReleased;
static uint32_t maxLatency = 0;
static uint8_t pressedReport[100] = {0};
static uint8_t releasedReport[100] = {0};
static bool keyReportTest = false;
static uint32_t usbReportLength = 0;
static bool unexpectedReport = false;

/* User input values */
static uint32_t measurementsQuantity = 0;
static uint32_t measurementsPeriodMs = 0;

static void setButtonState(ButtonState state)
{
	buttonState = state;
	GPIO_PinWrite(GPIO, GPIO_PORT, GPIO_PIN, state);
}

void buttonPressEmulationCb(uint32_t foo)
{
	(void) foo;
	if (!pressedReportReceived || !releasedReportReceived || buttonState != ButtonStateReleased) {
		usb_echo("buttonPressEmulationCb failed\r\n");
		usb_echo("pressedReportReceived %d, releasedReportReceived %d, button %s\r\n",
				  pressedReportReceived, releasedReportReceived,
				  buttonState == ButtonStatePressed ? "pressed" :"released");
		__asm("BKPT #255");
	}
	pressedReportReceived = false;
	releasedReportReceived = false;
	setButtonState(ButtonStatePressed);
}

void startMeasurements(uint32_t foo)
{
	(void) foo;
	pressedReportReceived = false;
	releasedReportReceived = false;

	const ctimer_match_config_t matchcofig = {
		.matchValue = measurementsPeriodMs * 1000,
		.enableCounterReset = true,
		.enableInterrupt = true
	};
	CTIMER_SetupMatch(TIMER, kCTIMER_Match_0, &matchcofig);
	static ctimer_callback_t cb_func = buttonPressEmulationCb;
	CTIMER_RegisterCallBack(TIMER, &cb_func, kCTIMER_SingleCallback);
	CTIMER_Reset(TIMER);

	setButtonState(ButtonStatePressed);
	CTIMER_StartTimer(TIMER);
}

void onUsbEnumerationDone(bool configured)
{
	if (configured) {
		keyReportTest = true;
	} else {
		CTIMER_StopTimer(TIMER);
		usb_echo("Keyboard Detached, test stoped");
		__asm("BKPT #255");
	}
}

void receivedReportCb(uint8_t *data, uint32_t dataLength)
{
	uint32_t timestamp = TIMER->TC;

	if (keyReportTest) {
		if (!pressedReportReceived) {
			usbReportLength = dataLength;
			memcpy(pressedReport, data, dataLength);
			pressedReportReceived = true;
		} else if (!releasedReportReceived) {
			memcpy(releasedReport, data, dataLength);
			releasedReportReceived = true;
		}
		return;
	}

	// verify received report
	if (memcmp(data, pressedReport, usbReportLength) == 0 && buttonState == ButtonStatePressed &&
		!releasedReportReceived && !pressedReportReceived) {
		if (timestamp > maxLatency)
			maxLatency = timestamp;
		// convert time stamp to string
		char number[15] = {'\0'};
		int i = 0;
		number[i++] = '\n';
		while (timestamp != 0) {
			number[i++] = timestamp % 10 + '0';
			timestamp /= 10;
		}

		// write reversed string to result array
		for (int j = i - 1; j >= 0; j--) {
			latency[idx++] = number[j];
		}

		measured++;
		pressedReportReceived = true;
		setButtonState(ButtonStateReleased);
	} else if (memcmp(data, releasedReport, usbReportLength) == 0 && buttonState == ButtonStateReleased &&
			   pressedReportReceived && !releasedReportReceived) {
		releasedReportReceived = true;
		if (measured >= COUNT_PER_MEASUREMENT) {
			CTIMER_StopTimer(TIMER);
			processOutput = true;
		}
	} else {
		CTIMER_StopTimer(TIMER);
		memcpy(releasedReport, data, dataLength);
		unexpectedReport = true;
	}
}

static void processDataOutput(void)
{
	if (unexpectedReport) {
		unexpectedReport = false;
		usb_echo("\r\nReceived unexpected report!\r\n");
		usb_echo("pressedReportReceived %d, releasedReportReceived %d, button %s\r\n",
				  pressedReportReceived, releasedReportReceived,
				  buttonState == ButtonStatePressed ? "pressed" :"released");
		for (uint32_t i = 0; i < usbReportLength; i++) {
			usb_echo("[%d] = %d, ", i, releasedReport[i]);
			if (i == 10)
				usb_echo("\r\n");
		}
		usb_echo("\r\nTest failed\r\n");
		__asm("BKPT #255");
	}

	if (!processOutput)
		return;

	processOutput = false;

	if (!sdCardAppendResults((uint8_t *) latency, idx)) {
		usb_echo("sdCardAppendResults failed");
		__asm("BKPT #255");
	}

	static uint32_t count = 0;
	count++;
	if (count >= measurementsQuantity) {
		usb_echo("\r\n************  Measurement Finished  ************\r\n", count);
		if (!sdCardCloseFile()) {
			usb_echo("sdCardCloseFile failed");
			__asm("BKPT #255");
		}
		usb_echo("max latency %u us", maxLatency);
		__asm("BKPT #255");
	}

	memset((uint8_t *) latency, '\0', sizeof(latency));
	measured = idx = 0;

	CTIMER_Reset(TIMER);
	CTIMER_StartTimer(TIMER);
}

static void processKeyReportTest(void)
{
	if (!keyReportTest)
		return;

	static bool waitPress = true;
	static bool waitRelease = true;

	if (!pressedReportReceived) {
		if (waitPress) {
			waitPress = false;
			usb_echo("\r\nPress and hold the key that will be triggered by optocoupler\r\n ");
		}
	} else if (!releasedReportReceived) {
		if (waitRelease) {
			waitRelease = false;
			usb_echo("Pressed report received, release key\r\n ");
		}
	} else {
		keyReportTest = false;
	    uint32_t testTimeMs = (measurementsQuantity  * 1000 * measurementsPeriodMs);
	    uint32_t hours = testTimeMs / 3600000;
	    uint32_t minutes = (testTimeMs % 3600000) / (60 * 1000);
	    usb_echo("\r\nTest will approximately take %u hours %u minutes\r\n ", hours, minutes);
		usb_echo("\r\nStart measurements\r\n");
		// 1 sec delay to ensure debounce will no affect measurements
		const ctimer_match_config_t matchcofig = {
			.matchValue = 1000 * 1000,
			.enableCounterStop = true,
			.enableInterrupt = true
		};
		CTIMER_SetupMatch(TIMER, kCTIMER_Match_0, &matchcofig);
		static ctimer_callback_t cb_func = startMeasurements;
		CTIMER_RegisterCallBack(TIMER, &cb_func, kCTIMER_SingleCallback);
		CTIMER_Reset(TIMER);
		CTIMER_StartTimer(TIMER);
	}
}

int main(void) {
    CLOCK_EnableClock(kCLOCK_InputMux);
    /* attach 12 MHz clock to FLEXCOMM0 (debug console) */
    CLOCK_AttachClk(BOARD_DEBUG_UART_CLK_ATTACH);
    BOARD_InitPins();
    BOARD_BootClockPLL180M();
    /* need call this function to clear the halt bit in clock divider register */
    CLOCK_SetClkDiv(kCLOCK_DivSdioClk, (uint32_t)(SystemCoreClock / FSL_FEATURE_SDIF_MAX_SOURCE_CLOCK + 1U), true);
    BOARD_InitDebugConsole();

    usb_echo("\r\nKeyboard latency test\r\n");
    do {
    	usb_echo("\r\nInput test quantity in thousands, 1 - 500\r\n");
    	if (SCANF("%u", &measurementsQuantity) != 1 || measurementsQuantity < 1 || measurementsQuantity > 500) {
    		usb_echo("Wrong input\r\n");
    		measurementsQuantity = 0;
    	} else {
    		usb_echo("test quantity == %u\r\n", measurementsQuantity);
    	}
    } while (measurementsQuantity == 0);

	do {
		usb_echo("\r\nInput button click period 65ms - 1000ms\r\n");
		if (SCANF("%u", &measurementsPeriodMs) != 1 || measurementsPeriodMs < 65 || measurementsPeriodMs > 1000) {
			usb_echo("Wrong input\r\n");
			measurementsPeriodMs = 0;
		} else {
			usb_echo("click period == %u\r\n", measurementsPeriodMs);
		}
	} while (measurementsPeriodMs == 0);

	if (sdCardInit() != 0)
		while(1);
	if (!sdCardCreateResultsFile())
		while(1);

	POWER_DisablePD(kPDRUNCFG_PD_USB0_PHY); /*< Turn on USB Phy */
	POWER_DisablePD(kPDRUNCFG_PD_USB1_PHY); /*< Turn on USB Phy */

	GPIO_PortInit(GPIO, GPIO_PORT);
	gpio_pin_config_t pinConfig = {
		.pinDirection = kGPIO_DigitalOutput,
		.outputLogic = 1,
	};
	GPIO_PinInit(GPIO, GPIO_PORT, GPIO_PIN, &pinConfig);
	IOCON->PIO[GPIO_PORT][GPIO_PIN] = 0x300;
	setButtonState(ButtonStateReleased);

	ctimer_config_t config;
	CTIMER_GetDefaultConfig(&config);
	config.prescale = SystemCoreClock / 1000000 - 1;
	CTIMER_Init(TIMER, &config);
	const ctimer_match_config_t matchcofig = {
		.matchValue = measurementsPeriodMs * 1000,
		.enableCounterReset = true,
		.enableInterrupt = true
	};
	CTIMER_SetupMatch(TIMER, kCTIMER_Match_0, &matchcofig);
	static ctimer_callback_t cb_func = buttonPressEmulationCb;
	CTIMER_RegisterCallBack(TIMER, &cb_func, kCTIMER_SingleCallback);

	USB_HostApplicationInit();

	while (1) {
		USB_HostTaskFn(g_HostHandle);
		USB_HostHidKeyboardTask(&g_HostHidKeyboard);
//		USB_HostHidMouseTask(&g_HostHidMouse);
		processDataOutput();
		processKeyReportTest();
	}
}
