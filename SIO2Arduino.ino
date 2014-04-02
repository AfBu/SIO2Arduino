/*
 * sio2arduino.ino - An Atari 8-bit device emulator for Arduino.
 *
 * Copyright (c) 2012 Whizzo Software LLC (Daniel Noguerol)
 *
 * This file is part of the SIO2Arduino project which emulates
 * Atari 8-bit SIO devices on Arduino hardware.
 *
 * SIO2Arduino is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * SIO2Arduino is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with SIO2Arduino; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include "config.h"
#include <SdFat.h>
#include <SdFatUtil.h>
#include "atari.h"
#include "sio_channel.h"
#include "disk_drive.h"
#ifdef LCD_DISPLAY
#include <LiquidCrystal.h>
#ifdef SHOW_FREE_MEMORY
#include <MemoryFree.h>
#endif
#endif

/**
 * Global variables
 */
DriveAccess driveAccess(getDeviceStatus, readSector, writeSector, format);
DriveControl driveControl(getFileList, mountFileIndex, changeDirectory);
SIOChannel sioChannel(PIN_ATARI_CMD, &SIO_UART, &driveAccess, &driveControl);
Sd2Card card;
SdVolume volume;
SdFile currDir;
SdFile files[DRIVES_COUNT];
DiskDrive drives[DRIVES_COUNT];
String names[DRIVES_COUNT];
#ifdef SELECTOR_BUTTON
unsigned long lastSelectionPress;
boolean selectionPress = false;
unsigned long lastSelectionDrivePress;
#endif
#ifdef RESET_BUTTON
unsigned long lastResetPress;
#endif
#ifdef LCD_DISPLAY
LiquidCrystal lcd(PIN_LCD_RD,PIN_LCD_ENABLE,PIN_LCD_DB4,PIN_LCD_DB5,PIN_LCD_DB6,PIN_LCD_DB7);
boolean lcdUpdate = true;
#endif

unsigned int selectedDrive = 0;
int selectedFileIndex = 0;
boolean mounted = false;

void setup() {
  #ifdef DEBUG
    // set up logging serial port
    LOGGING_UART.begin(115200);
  #endif

  // initialize serial port to Atari
  SIO_UART.begin(19200);

  // set pin modes
  #ifdef SELECTOR_BUTTON
  pinMode(PIN_SELECTOR, INPUT);
  pinMode(PIN_SELECTOR_DRIVE, INPUT);
  #endif
  #ifdef RESET_BUTTON
  pinMode(PIN_RESET, INPUT);
  #endif
  #ifdef ACTIVITY_LED
  pinMode(PIN_ACTIVITY_LED, OUTPUT);
  #endif

  #ifdef LCD_DISPLAY
  // set up LCD if appropriate
  lcd.begin(16, 2);
  lcd.print("SIO2Arduino+");
  lcd.setCursor(0,1);
  #endif

  // initialize SD card
  LOG_MSG(TXT_SDINIT);
  pinMode(PIN_SD_CS, OUTPUT);

  if (!card.init(SPI_HALF_SPEED, PIN_SD_CS)) {
    LOG_MSG_CR(TXT_FAILED);
    #ifdef LCD_DISPLAY
      lcd.print(TXT_SDERROR);
    #endif     
    return;
  }
  
  if (!volume.init(&card)) {
    LOG_MSG_CR(TXT_FAILED);
    #ifdef LCD_DISPLAY
      lcd.print(TXT_SDVOLERR);
    #endif     
    return;
  }

  if (!currDir.openRoot(&volume)) {
    LOG_MSG_CR(TXT_FAILED);
    #ifdef LCD_DISPLAY
      lcd.print(TXT_SDROOTERR);
    #endif     
    return;
  }

  LOG_MSG_CR(TXT_DONE);
  #ifdef LCD_DISPLAY
    lcd.print(TXT_READY);
  #endif
}

void loop() {
  // let the SIO channel do its thing
  sioChannel.runCycle();
  
  #ifdef SELECTOR_BUTTON
  processSelector();
  #endif
  
  #ifdef RESET_BUTTON
  // watch the reset button
  if (digitalRead(PIN_RESET) == HIGH && millis() - lastResetPress > 250) {
    lastResetPress = millis();
    mountFilename(0, "AUTORUN.ATR");
  }
  #endif
  
  #ifdef LCD_DISPLAY
  if (lcdUpdate) {
    lcdUpdate = false;
    updateLcd();
  }
  #endif
  
  #ifdef ARDUINO_TEENSY
    if (SIO_UART.available())
      SIO_CALLBACK();
  #endif
}

boolean isSelectedDir()
{
  if (selectedFileIndex == -1) return true;
  
  FileEntry entries[1];
  char name[13];
  if (getFileList(selectedFileIndex, 1, entries) <= 0) return false;
  return entries[0].isDirectory;
}

String getSelectedFile()
{
  if (selectedFileIndex == -1) return "..";
  
  FileEntry entries[1];
  char name[13];
  if (getFileList(selectedFileIndex, 1, entries) <= 0) return "/EOD";
  createFilename(name, entries[0].name);
  
  return String(name);
}

#ifdef LCD_DISPLAY
void updateLcd()
{
  lcd.clear();
  lcd.print("D" + String(selectedDrive+1) + ":" + names[selectedDrive]);
  if (selectedFileIndex >= 0) {
    lcd.setCursor(0, 1);
    lcd.print(selectedFileIndex, DEC);
  }
  lcd.setCursor(3, 1);
  lcd.print(getSelectedFile());
  #ifdef SHOW_FREE_MEMORY
  lcd.setCursor(0, 0);
  lcd.print(freeMemory(), DEC);
  #endif
}
#endif

#ifdef SELECTOR_BUTTON
void processSelector()
{
  if (digitalRead(PIN_SELECTOR_DRIVE) == HIGH) {
    if (!selectionPress) {
      selectionPress = true;
      lastSelectionPress = millis();
    } else {
      if (millis() - lastSelectionPress > 1000 && !mounted) {
        mounted = true;
        if (isSelectedDir()) {
          changeDirectory(selectedFileIndex);
          selectedFileIndex = 0;
        } else {
          mountFileIndex(selectedDrive, selectedFileIndex);
        }
        lcdUpdate = true;
        lastSelectionPress = millis() + 1000;
      }
    }
  } else {
    if (selectionPress) {
      if (millis() - lastSelectionPress < 250) {
        selectedDrive++;
        if (selectedDrive == DRIVES_COUNT) selectedDrive = 0;
        lcdUpdate = true;
      }
      selectionPress = false;
      mounted = false;
    }
  }

  if (digitalRead(PIN_SELECTOR) == HIGH && millis() - lastSelectionPress > 250) {
    lastSelectionPress = millis();
    selectedFileIndex++;
    if (getSelectedFile() == "/EOD") selectedFileIndex = (currDir.isRoot() ? 0 : -1);
    lcdUpdate = true;
  }
}
#endif

void SIO_CALLBACK() {
  // inform the SIO channel that an incoming byte is available
  sioChannel.processIncomingByte();
}

DriveStatus* getDeviceStatus(int deviceId) {
  return drives[deviceId].getStatus();
}

SectorDataInfo* readSector(int deviceId, unsigned long sector, byte *data) {
  if (drives[deviceId].hasImage()) {
    return drives[deviceId].getSectorData(sector, data);
  } else {
    return NULL;
  }
}

boolean writeSector(int deviceId, unsigned long sector, byte* data, unsigned long length) {
  if (deviceId > 0) deviceId--;
  if (drives[deviceId].hasImage()) {
    return (drives[deviceId].writeSectorData(sector, data, length) == length);
  } else {
    return false;
  }
}

boolean format(int deviceId, int density) {
  if (deviceId > 0) deviceId--;

  char name[13];
  
  // get current filename
  files[deviceId].getFilename(name);

  // close and delete the current file
  files[deviceId].close();
  files[deviceId].remove();

  LOG_MSG("Remove old file: ");
  LOG_MSG_CR(name);

  // open new file for writing
  files[deviceId].open(&currDir, name, O_RDWR | O_SYNC | O_CREAT);

  LOG_MSG("Created new file: ");
  LOG_MSG_CR(name);

  // allow the virtual drive to format the image (and possibly alter its size)
  if (drives[deviceId].formatImage(&files[deviceId], density)) {
    // set the new image file for the drive
    drives[deviceId].setImageFile(&files[deviceId]);
    return true;
  } else {
    return false;
  }  
}

boolean isValidFilename(char *s) {
  return (  !(s[0] == 'S' && s[1] == 'Y' && s[2] == 'S' && s[3] == 'T' && s[4] == 'E' && s[5] == 'M' /*&& s[6] == '~'*/ && s[7] == '1') && // ignore windows system directory
            s[0] != '.' &&    // ignore hidden files 
            s[0] != '_' && (  // ignore bogus files created by OS X
             (s[8] == 'A' && s[9] == 'T' && s[10] == 'R')
          || (s[8] == 'X' && s[9] == 'F' && s[10] == 'D')
#ifdef PRO_IMAGES             
          || (s[8] == 'P' && s[9] == 'R' && s[10] == 'O')
#endif          
#ifdef ATX_IMAGES              
          || (s[8] == 'A' && s[9] == 'T' && s[10] == 'X')
#endif              
#ifdef XEX_IMAGES
          || (s[8] == 'X' && s[9] == 'E' && s[10] == 'X')
#endif              
          )
        );
}

void createFilename(char* filename, char* name) {
  for (int i=0; i < 8; i++) {
    if (name[i] != ' ') {
      *(filename++) = name[i];
    }
  }
  if (name[8] != ' ') {
    *(filename++) = '.';
    *(filename++) = name[8];
    *(filename++) = name[9];
    *(filename++) = name[10];
  }
  *(filename++) = '\0';
}

/**
 * Returns a list of files in the current directory.
 *
 * startIndex = the first valid file in the directory to start from
 * count = how many files to return
 * entries = a pointer to the a FileEntry array to hold the returned data
 */
int getFileList(int startIndex, int count, FileEntry *entries) {
  dir_t dir;
  int currentEntry = 0;

  currDir.rewind();

  int ix = 0;
  while (ix < count) {
    if (currDir.readDir((dir_t*)&dir) < 1) {
      break;
    }
    // dir.attributes != 22 hides windows's system folders 
    if (isValidFilename((char*)&dir.name) || (DIR_IS_SUBDIR(&dir) && dir.name[0] != '.' && dir.attributes != 22)) {
      if (currentEntry >= startIndex) {
        memcpy(entries[ix].name, dir.name, 11);
        if (DIR_IS_SUBDIR(&dir)) {
          entries[ix].isDirectory = true;
        } else {
          entries[ix].isDirectory = false;
        }
        ix++;
      }
      currentEntry++;
    }
  }
  
  return ix;
}

/**
 * Changes the SD card directory.
 *
 * ix = index number (or -1 to go to parent directory)
 */
void changeDirectory(int ix) {
  FileEntry entries[1];
  char name[13];
  SdFile subDir;

  if (ix > -1) {  
    getFileList(ix, 1, entries);
    createFilename(name, entries[0].name);
    if (subDir.open(&currDir, name, O_READ)) {
      currDir = subDir;
    }
  } else {
    if (subDir.openRoot(&volume)) {
      currDir = subDir;
    }
  }
}

/**
 * Mount a file with the given index number.
 *
 * deviceId = the drive ID
 * ix = the index of the file to mount
 */
void mountFileIndex(int deviceId, int ix) {
  FileEntry entries[1];
  char name[13];

  // figure out what filename is associated with the index
  getFileList(ix, 1, entries);

  // build a full 8.3 filename
  createFilename(name, entries[0].name);

  // mount the image
  mountFilename(deviceId, name);
}

/**
 * Mount a file with the given name.
 *
 * deviceId = the drive ID
 * name = the name of the file to mount
 */
boolean mountFilename(int deviceId, char *name) {
  // close previously open file
  if (files[deviceId].isOpen()) {
    files[deviceId].close();
  }
  
  if (files[deviceId].open(&currDir, name, O_RDWR | O_SYNC) && drives[deviceId].setImageFile(&files[deviceId])) {
    LOG_MSG_CR(name);

    names[deviceId] = String(name);

    #ifdef LCD_DISPLAY
    lcdUpdate = true;
    #endif

    return true;
  }
  
  return false;
}
