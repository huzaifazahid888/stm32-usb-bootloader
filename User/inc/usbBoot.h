/*
 * usbBoot.h
 *      Author: Muhammad Huzaifa
 */

#ifndef INC_USBBOOT_H_
#define INC_USBBOOT_H_
#include "stdio.h"
#include "fatfs.h"
#include "bootConfig.h"
#define APP_BLOCK_TRANSFER_SIZE 512
#define SIZE_OF_U32 4

extern FIL myFile;
extern uint8_t appBuffer[512];
extern unsigned int  readBytes;
extern uint32_t appSize;
extern FRESULT fRes;

void jumpcode();
void CopyAppToUserMemory(void);
void copyHeaderToMemory();
void parseHeader(AppHeader_t *h);
uint32_t calCrc();
void flashEraseApplication(void);
uint8_t validStoredApp(void);
bool IsApplicationValid(void);
void PrintProgrammingProgress(uint32_t currentBytes, uint32_t totalBytes);

#endif /* INC_USBBOOT_H_ */
