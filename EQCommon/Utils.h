// Utils.h -- Common functions

#pragma once

#include <windows.h>
#include <direct.h>
#include <memory.h>
#include <string.h>
#include <io.h>
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#define  POINTERSIZE    sizeof(char*)

typedef unsigned __int32  Pointer32;
typedef unsigned __int64  Pointer64;

#ifdef _WIN64
// Win64
#endif

#ifndef uint32
typedef  unsigned  __int8    uint8;
typedef unsigned  __int16    uint16;
typedef unsigned  __int32    uint32;
typedef unsigned  __int64    uint64;

typedef  signed  __int8    sint8;
typedef signed  __int16    sint16;
typedef signed  __int32    sint32;
typedef signed  __int64    sint64;
#endif

#ifndef BYTE
typedef unsigned  __int8    BYTE;
#endif

#ifndef WORD
typedef unsigned  __int16    WORD;
#endif

#ifndef DWORD
typedef  unsigned  long      DWORD; // Couldn't use __int32 because of a conflict with WinDef.h
#endif

#ifndef QWORD
typedef  unsigned  __int64    QWORD;
#endif

#ifdef _DEBUG
#define BreakPoint  __asm int 3;
#else
#define BreakPoint  __asm nop;
#endif

#ifndef SAFE_DELETE
#define SAFE_DELETE(Pointer) if (Pointer) { delete Pointer; Pointer = NULL; }
#endif

#define SAFE_OUTCHAR(Byte) ((Byte >= 0x20) && (Byte <= 0x7F) ? Byte : '.')

bool LeftMatch(char* MainString, char* SearchString);
bool LeftMatch(char* MainString, char* SearchString, size_t MainLength, size_t SearchLength);

bool RightMatch(char* MainString, char* SearchString);
bool RightMatch(char* MainString, char* SearchString, size_t MainLength, size_t SearchLength);

void TrimString(char* String);
void TrimString(char* String, size_t* Length);

void SplitString(char* String, char Sep, char* OutBuffer, size_t BufSize, WORD* OutNumParts, char** OutParts, WORD MaxParts);

char* IfBlank(char* CheckString, char* ResultIfBlank);
bool IsBlank(char* CheckString);

bool IsNumeric(char Digit);

bool FileExists(char* FilePath);
size_t GetFileSize(char* FilePath);

static char sMakePath_TempPath[_MAX_PATH];

char* MakePath(char* InPath, char* InFile);
char* MakePath(char* InPath, char* InFile, char* OutPath, size_t OutPathSize);
size_t ReadLine(FILE* File, char* Buffer, size_t BufferSize);

bool StringSame(char* String1, char* String2);
bool StringSame(char* String1, char* String2, bool IgnoreCase);
bool StringSame(char* String1, size_t Len1, char* String2);
bool StringSame(char* String1, size_t Len1, char* String2, bool IgnoreCase);
bool StringSame(char* String1, size_t Len1, char* String2, size_t Len2);
bool StringSame(char* String1, size_t Len1, char* String2, size_t Len2, bool IgnoreCase);

class FileSearch;

class FileSearch
{
public:
  FileSearch(void);
  FileSearch(char* Pattern);

  ~FileSearch(void);

  HRESULT    FindFile();
  HRESULT    FindFile(char* Pattern);

  _finddata_t  FoundFile;

private:
  char      sPattern[_MAX_PATH];
  intptr_t  iSearchHandle;
};

enum LogEcho
{
  NOECHO = 0,
  QUIET = 0,
  ECHO = 1
};

class LogFile
{
public:
  LogFile(void);
  ~LogFile(void);

  HRESULT  Open(char* Filename, char* Header);
  HRESULT Close();
  HRESULT  SetEcho(LogEcho Echo);

  HRESULT Entry(char* Message);
  HRESULT  Entry(LogEcho Echo, char* Message);

  HRESULT Entry(char* Format, void* Param1);
  HRESULT Entry(char* Format, void* Param1, void* Param2);
  HRESULT Entry(char* Format, void* Param1, void* Param2, void* Param3);
  HRESULT Entry(LogEcho Echo, char* Format, void* Param1);
  HRESULT Entry(LogEcho Echo, char* Format, void* Param1, void* Param2);
  HRESULT Entry(LogEcho Echo, char* Format, void* Param1, void* Param2, void* Param3);
  HRESULT Entry(LogEcho Echo, char* Format, void* Param1, void* Param2, void* Param3, void* Param4);
  HRESULT Entry(LogEcho Echo, char* Format, void* Param1, void* Param2, void* Param3, void* Param4, void* Param5);
private:
  LogEcho  iEcho;
  FILE*    oLogFile;
  char    sFile[_MAX_PATH];
};