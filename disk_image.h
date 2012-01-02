#ifndef DISK_IMAGE_h
#define DISK_IMAGE_h

#include <Arduino.h>
#include <SD.h>
#include "atari.h"

const byte TYPE_ATR = 1;
const byte TYPE_XFD = 2;

const unsigned long FORMAT_SS_SD_35 = 80640;
const unsigned long FORMAT_SS_SD_40 = 92160;
const unsigned long FORMAT_SS_ED_35 = 116480;
const unsigned long FORMAT_SS_ED_40 = 133120;
const unsigned long FORMAT_SS_DD_35 = 160896;
const unsigned long FORMAT_SS_DD_40 = 183936;

class DiskImage {
public:
  DiskImage();
  boolean setFile(File* file);
  unsigned long getSectorSize();
  SectorPacket* getSectorData(unsigned long sector);
  boolean writeSectorData(unsigned long, byte* data, unsigned long size);
  boolean format(File *file, int density);
  boolean isEnhancedDensity();
  boolean isDoubleDensity();
private:
  boolean loadFile(File* file);
  File*            m_fileRef;
  byte             m_type;
  unsigned long    m_fileSize;
  unsigned long    m_headerSize;
  unsigned long    m_sectorSize;
  SectorPacket m_sectorBuffer;
};

// ATR format

#define ATR_SIGNATURE 0x0296

struct ATRHeader {
  unsigned int signature;
  unsigned int pars;
  unsigned int secSize;
  byte parsHigh;
  unsigned long crc;
  unsigned long unused;
  byte flags;
};

#endif
