/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>
#include "fsl_sd.h"
#include "fsl_debug_console.h"
#include "ff.h"
#include "diskio.h"
#include "fsl_sd_disk.h"
#include "board.h"

#include "sdcard_pin_mux.h"
#include <stdbool.h>
#include "fsl_iocon.h"
#include "sdcard_fatfs.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define DIRECTORY "/latency"
#define TEST_FILE "cardTest.txt"
#define RESULTS_FILE "results.txt"

#define TEST_FILE_PATH DIRECTORY "/" TEST_FILE
#define RESULTS_FILE_PATH DIRECTORY "/" RESULTS_FILE

/* buffer size (in byte) for read/write operations */
#define BUFFER_SIZE (100U)
/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*!
 * @brief wait card insert function.
 */
static status_t sdcardWaitCardInsert(void);

/*******************************************************************************
 * Variables
 ******************************************************************************/
static FATFS g_fileSystem; /* File system object */
static FIL g_fileObject;   /* File object */

/* @brief decription about the read/write buffer
 * The size of the read/write buffer should be a multiple of 512, since SDHC/SDXC card uses 512-byte fixed
 * block length and this driver example is enabled with a SDHC/SDXC card.If you are using a SDSC card, you
 * can define the block length by yourself if the card supports partial access.
 * The address of the read/write buffer should align to the specific DMA data buffer address align value if
 * DMA transfer is used, otherwise the buffer address is not important.
 * At the same time buffer address/size should be aligned to the cache line size if cache is supported.
 */
SDK_ALIGN(uint8_t g_bufferWrite[SDK_SIZEALIGN(BUFFER_SIZE, SDMMC_DATA_BUFFER_ALIGN_CACHE)],
          MAX(SDMMC_DATA_BUFFER_ALIGN_CACHE, SDMMCHOST_DMA_BUFFER_ADDR_ALIGN));
SDK_ALIGN(uint8_t g_bufferRead[SDK_SIZEALIGN(BUFFER_SIZE, SDMMC_DATA_BUFFER_ALIGN_CACHE)],
          MAX(SDMMC_DATA_BUFFER_ALIGN_CACHE, SDMMCHOST_DMA_BUFFER_ADDR_ALIGN));
/*! @brief SDMMC host detect card configuration */
static const sdmmchost_detect_card_t s_sdCardDetect = {
#ifndef BOARD_SD_DETECT_TYPE
    .cdType = kSDMMCHOST_DetectCardByGpioCD,
#else
    .cdType = BOARD_SD_DETECT_TYPE,
#endif
    .cdTimeOut_ms = (~0U),
};

/*! @brief SDMMC card power control configuration */
#if defined DEMO_SDCARD_POWER_CTRL_FUNCTION_EXIST
static const sdmmchost_pwr_card_t s_sdCardPwrCtrl = {
    .powerOn          = BOARD_PowerOnSDCARD,
    .powerOnDelay_ms  = 500U,
    .powerOff         = BOARD_PowerOffSDCARD,
    .powerOffDelay_ms = 0U,
};
#endif
/*! @brief SDMMC card power control configuration */
#if defined DEMO_SDCARD_SWITCH_VOLTAGE_FUNCTION_EXIST
static const sdmmchost_card_switch_voltage_func_t s_sdCardVoltageSwitch = {
    .cardSignalLine1V8 = BOARD_USDHC_Switch_VoltageTo1V8,
    .cardSignalLine3V3 = BOARD_USDHC_Switch_VoltageTo3V3,
};
#endif
/*******************************************************************************
 * Code
 ******************************************************************************/
/* This function is used to init the SDIF unused data pin, DATA4 - DATA7, these pin should be configured
 * ,otherswise the SDIF will not work, please check the corresponding errata.
 */
void Board_InitSdifUnusedDataPin(void)
{
    IOCON_PinMuxSet(IOCON, 4, 29,
                    (IOCON_FUNC2 | IOCON_PIO_SLEW_MASK | IOCON_DIGITAL_EN | IOCON_MODE_PULLUP)); /* sd data[4] */
    IOCON_PinMuxSet(IOCON, 4, 30,
                    (IOCON_FUNC2 | IOCON_PIO_SLEW_MASK | IOCON_DIGITAL_EN | IOCON_MODE_PULLUP)); /* sd data[5] */
    IOCON_PinMuxSet(IOCON, 4, 31,
                    (IOCON_FUNC2 | IOCON_PIO_SLEW_MASK | IOCON_DIGITAL_EN | IOCON_MODE_PULLUP)); /* sd data[6] */
    IOCON_PinMuxSet(IOCON, 5, 0,
                    (IOCON_FUNC2 | IOCON_PIO_SLEW_MASK | IOCON_DIGITAL_EN | IOCON_MODE_PULLUP)); /* sd data[7] */
}


/*!
 * @brief Main function
 */
int sdCardInit(void)
{
    FRESULT error;
    DIR directory; /* Directory object */
    FILINFO fileInformation;
    UINT bytesWritten;
    UINT bytesRead;
    const TCHAR driverNumberBuffer[3U] = {SDDISK + '0', ':', '/'};
    volatile bool failedFlag           = false;
    char ch                            = '0';
    BYTE work[FF_MAX_SS];

    SD_BOARD_InitPins();
    /* This function is used to cover the IP bug which the DATA4-7 pin should be configured, otherwise the SDIF will not
     * work */
    Board_InitSdifUnusedDataPin();
    /* attach main clock to SDIF */
    CLOCK_AttachClk(BOARD_SDIF_CLK_ATTACH);

    PRINTF("\r\nPlease insert a card into board.\r\n");

    if (sdcardWaitCardInsert() != kStatus_Success)
    {
        return -1;
    }

    if (f_mount(&g_fileSystem, driverNumberBuffer, 0U))
    {
        PRINTF("Mount volume failed.\r\n");
        return -1;
    }

#if (FF_FS_RPATH >= 2U)
    error = f_chdrive((char const *)&driverNumberBuffer[0U]);
    if (error)
    {
        PRINTF("Change drive failed.\r\n");
        return -1;
    }
#endif

#if FF_USE_MKFS
    PRINTF("\r\nMake file system......The time may be long if the card capacity is big.\r\n");
    if (f_mkfs(driverNumberBuffer, FM_ANY, 0U, work, sizeof work))
    {
        PRINTF("Make file system failed.\r\n");
        return -1;
    }
#endif /* FF_USE_MKFS */

//    PRINTF("\r\nCreate directory......\r\n");
    error = f_mkdir(_T(DIRECTORY));
    if (error)
    {
        if (error == FR_EXIST)
        {
            PRINTF("Directory exists.\r\n");
        }
        else
        {
            PRINTF("Make directory failed.\r\n");
            return -1;
        }
    }

//    PRINTF("\r\nCreate a file in that directory......\r\n");
    error = f_open(&g_fileObject, _T(TEST_FILE_PATH), (FA_WRITE | FA_READ | FA_CREATE_ALWAYS));
    if (error)
    {
        if (error == FR_EXIST)
        {
            PRINTF("File exists.\r\n");
        }
        else
        {
            PRINTF("Open file failed.\r\n");
            return -1;
        }
    }

//    PRINTF("\r\nList the file in that directory......\r\n");
    if (f_opendir(&directory, DIRECTORY))
    {
        PRINTF("Open directory failed.\r\n");
        return -1;
    }

    for (;;)
    {
        error = f_readdir(&directory, &fileInformation);

        /* To the end. */
        if ((error != FR_OK) || (fileInformation.fname[0U] == 0U))
        {
            break;
        }
        if (fileInformation.fname[0] == '.')
        {
            continue;
        }
        if (fileInformation.fattrib & AM_DIR)
        {
//            PRINTF("Directory file : %s.\r\n", fileInformation.fname);
        }
        else
        {
//            PRINTF("General file : %s.\r\n", fileInformation.fname);
        }
    }

    memset(g_bufferWrite, 'a', sizeof(g_bufferWrite));
    g_bufferWrite[BUFFER_SIZE - 2U] = '\r';
    g_bufferWrite[BUFFER_SIZE - 1U] = '\n';

//    PRINTF("\r\nWrite/read file until encounters error......\r\n");
    while (true)
    {
        if (failedFlag || (ch == 'q'))
        {
            break;
        }

//        PRINTF("\r\nWrite to above created file.\r\n");
        error = f_write(&g_fileObject, g_bufferWrite, sizeof(g_bufferWrite), &bytesWritten);
        if ((error) || (bytesWritten != sizeof(g_bufferWrite)))
        {
            PRINTF("Write file failed. \r\n");
            failedFlag = true;
            continue;
        }

        /* Move the file pointer */
        if (f_lseek(&g_fileObject, 0U))
        {
            PRINTF("Set file pointer position failed. \r\n");
            failedFlag = true;
            continue;
        }

//        PRINTF("Read from above created file.\r\n");
        memset(g_bufferRead, 0U, sizeof(g_bufferRead));
        error = f_read(&g_fileObject, g_bufferRead, sizeof(g_bufferRead), &bytesRead);
        if ((error) || (bytesRead != sizeof(g_bufferRead)))
        {
            PRINTF("Read file failed. \r\n");
            failedFlag = true;
            continue;
        }

//        PRINTF("Compare the read/write content......\r\n");
        if (memcmp(g_bufferWrite, g_bufferRead, sizeof(g_bufferWrite)))
        {
            PRINTF("Compare read/write content isn't consistent.\r\n");
            failedFlag = true;
            continue;
        }
//        PRINTF("The read/write content is consistent.\r\n\r\n");
        PRINTF("SD card initialized successfully\r\n\r\n");
        break;
    }

    if (f_close(&g_fileObject))
    {
        PRINTF("\r\nClose file failed.\r\n");
        return -1;
    }

    return f_unlink(TEST_FILE_PATH);
}

bool sdCardCreateResultsFile(void)
{
	FRESULT error = f_open(&g_fileObject, _T(RESULTS_FILE_PATH), (FA_WRITE | FA_READ | FA_CREATE_ALWAYS));
    if (error)
    {
        if (error == FR_EXIST)
        {
            PRINTF("File exists.\r\n");
        }
        else
        {
            PRINTF("Open file failed.\r\n");
        }
        return false;
    }
    return true;
}

bool sdCardAppendResults(uint8_t *data, uint32_t size)
{
	UINT bytesWritten;
	FRESULT error = f_write(&g_fileObject, data, size, &bytesWritten);
    if ((error) || (bytesWritten != size))
    {
        PRINTF("Write results file failed. \r\n");
        return false;
    }
    return true;
}

bool sdCardCloseFile(void)
{
    if (f_close(&g_fileObject))
    {
        PRINTF("\r\nClose file failed.\r\n");
        return false;
    }
    return true;
}

static status_t sdcardWaitCardInsert(void)
{
    /* Save host information. */
    g_sd.host.base           = SD_HOST_BASEADDR;
    g_sd.host.sourceClock_Hz = SD_HOST_CLK_FREQ;
    /* card detect type */
    g_sd.usrParam.cd = &s_sdCardDetect;
#if defined DEMO_SDCARD_POWER_CTRL_FUNCTION_EXIST
    g_sd.usrParam.pwr = &s_sdCardPwrCtrl;
#endif
#if defined DEMO_SDCARD_SWITCH_VOLTAGE_FUNCTION_EXIST
    g_sd.usrParam.cardVoltage = &s_sdCardVoltageSwitch;
#endif
    /* SD host init function */
    if (SD_HostInit(&g_sd) != kStatus_Success)
    {
        PRINTF("\r\nSD host init fail\r\n");
        return kStatus_Fail;
    }
    /* power off card */
    SD_PowerOffCard(g_sd.host.base, g_sd.usrParam.pwr);
    /* wait card insert */
    if (SD_WaitCardDetectStatus(SD_HOST_BASEADDR, &s_sdCardDetect, true) == kStatus_Success)
    {
        PRINTF("\r\nCard inserted.\r\n");
        /* power on the card */
        SD_PowerOnCard(g_sd.host.base, g_sd.usrParam.pwr);
    }
    else
    {
        PRINTF("\r\nCard detect fail.\r\n");
        return kStatus_Fail;
    }

    return kStatus_Success;
}
