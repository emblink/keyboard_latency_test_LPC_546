/*
 * sdcard_fatfs.h
 *
 *  Created on: Jul 13, 2020
 *      Author: embli
 */

#ifndef SDCARD_FATFS_H_
#define SDCARD_FATFS_H_

int sdCardInit(void);
bool sdCardCreateResultsFile(void);
bool sdCardAppendResults(uint8_t *data, uint32_t size);
bool sdCardCloseFile(void);

#endif /* SDCARD_FATFS_H_ */
