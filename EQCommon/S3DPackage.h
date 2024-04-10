// S3DPackage.h -- Handle EQ S3D package files

#pragma once

#include <windows.h>
#include <io.h>
#include <memory.h>
#include <string.h>
#include <stdio.h>
#include <zlib.h>
#include "Utils.h"

/*******************

  Pascal Data Types:

  Shortint  sint8
  Byte    uint8

  Smallint  sint16
  Word    uint16

  Integer    sint32
  Longint    sint32
  LongWord  DWORD
  Cardinal  DWORD

  Int64    sint64

  Single    float
  Double    double

*******************/

#define S3D_MAGICNUMBER   0x20534650
#define S3D_CRCPOLY       0x04C11DB7

#define S3D_INVALID       0xFFFF

#define EQGL_MAGICNUMBER  0x4C475145

class EQGLFile;
class S3DPackage;

struct EQGL_Header
{
  DWORD MagicNumber;
  DWORD Version;
  DWORD StringTableSize;
  DWORD NumEntries;
};

struct EQGL_Entry
{
  char* GetName(char* StringTable);
  char* GetFile1(char* StringTable);
  char* GetFile2(char* StringTable);
  
  DWORD NamePtr;
  DWORD File1Ptr;
  DWORD File2Ptr;
  DWORD Unknown1;       // Always 0xFFFFFFFF ?
  DWORD Unknown2;       // Always 0xFFFFFFFF ?
  DWORD Unknown3;       // Always 0xFFFFFFFF ?
  WORD Flags1;
  WORD Flags2;
  WORD Flags3;
  WORD Flags4;
};

class EQGLFile
{
public:
  EQGLFile(void);
  ~EQGLFile(void);

  void    Close();
  HRESULT LoadFromMemory(BYTE* Buffer, DWORD FileSize, bool StableBuffer, char* Filename);

  WORD        FilenameLen;
  char        Filename[_MAX_PATH];
  
  DWORD       FileSize;
  BYTE*       FileBytes;
  bool        FileCopied;

  DWORD       StringTableSize;
  char*       StringTable;

  DWORD       Entries;
  EQGL_Entry* Entry;
};

enum S3DPackage_VersionNumbers
{
  S3DP_Version2 = 0x20000
};

struct S3DFileInfo
{
  DWORD  FilenameLength;
  char  Filename[_MAX_PATH];
  DWORD  Size;
  DWORD  FileOffset;
  DWORD  CRC;
};

class S3DPackage
{
public:

  S3DPackage(void);
  ~S3DPackage(void);

  HRESULT        Open(char* Filename);
  HRESULT        Close();
  
  DWORD          GetVersion();
  DWORD          GetDateStamp();

  DWORD          GetFileCount();
  char*          GetFilename(DWORD FileNumber);

  S3DFileInfo*   GetFileInfo(DWORD FileNumber);
  DWORD          GetCRC(BYTE* Source, DWORD SourceSize);

  DWORD          FindFile(char* Filename);
  BYTE*          LoadFile(DWORD FileNumber);

private:  
  HRESULT        ReadFileEntry(DWORD FileOffset, DWORD SizeUncompressed, BYTE** DestBuffer);
  DWORD          ReadDWord();
  void          VerifyCRCTable();

  DWORD          iVersionNumber;
  DWORD          iDateStamp;

  DWORD          iFiles;
  S3DFileInfo*  oFileInfo;

  DWORD          iCRCTable[256];

  char          sPackage[_MAX_PATH];
  BYTE*          aPackageBytes;
  DWORD          iPackageSize;
  DWORD          iPackagePos;
};
