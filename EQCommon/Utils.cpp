// Utils.cpp -- Common functions

#include "Utils.h"

bool LeftMatch(char* MainString, char* SearchString)
{
  if ((!MainString) || (!SearchString))
  {
    return false;
  }

  size_t _l1 = strlen(MainString);
  size_t _l2 = strlen(SearchString);

  if (_l1 < _l2)
  {
    return false;
  }

  return StringSame(MainString, _l2, SearchString, _l2, true);
}

bool LeftMatch(char* MainString, char* SearchString, size_t MainLength, size_t SearchLength)
{
  if ((!MainString) || (!SearchString) || (MainLength < SearchLength))
  {
    return false;
  }

  return StringSame(MainString, SearchLength, SearchString, SearchLength, true);
}

bool RightMatch(char* MainString, char* SearchString)
{
  if ((!MainString) || (!SearchString))
  {
    return false;
  }

  size_t _l1 = strlen(MainString);
  size_t _l2 = strlen(SearchString);

  if (_l1 < _l2)
  {
    return false;
  }

  return StringSame(&MainString[_l1 - _l2], _l2, SearchString, _l2, true);
}

bool RightMatch(char* MainString, char* SearchString, size_t MainLength, size_t SearchLength)
{
  if ((!MainString) || (!SearchString) || (MainLength < SearchLength))
  {
    return false;
  }

  return StringSame(&MainString[MainLength - SearchLength], SearchLength, SearchString, SearchLength, true);
}

bool IsNumeric(char Digit)
{
  return (Digit <= '9') && (Digit >= '0'); // Digit > '9' more likely, so check it first.
}

void TrimString(char* String)
{
  size_t _i = strlen(String);
  
  if (String)
  {
    TrimString(String, &_i);
  }
}

void TrimString(char* String, size_t* Length)
{
  if ((!String) || (!Length))
  {
    return;
  }
  
  size_t _iLen = *Length;
  char* _sTrimL = String;
  char* _sTrimR = &String[_iLen - 1];

  while (_sTrimL[0] == ' ')
  {
    _sTrimL++;
    _sTrimR--;
    _iLen--;
  }

  if (_sTrimL != String)
  {
    memmove_s(String, *Length, _sTrimL, _iLen);
  }

  while ((_sTrimR[0] == ' ') || (_sTrimR[0] == '\n') || (_sTrimR[0] == '\r'))
  {
    _sTrimR--;
    _iLen--;
  }

  if (_iLen != *Length)
  {
    String[_iLen] = 0x00;

    *Length = _iLen;
  }
}

char* IfBlank(char* CheckString, char* ResultIfBlank)
{
  return (IsBlank(CheckString)) ? ResultIfBlank : CheckString;
}

bool IsBlank(char *CheckString)
{
  if (CheckString)
  {
    return (CheckString[0] == 0);
  }

  return true;
}

bool FileExists(char* FilePath)
{
  if (!FilePath)
  {
    return false;
  }

  struct _stat32  _oStats;

  return (_stat32(FilePath, &_oStats) == 0);
}

size_t GetFileSize(char* FilePath)
{
  struct _stat32  _oStats;

  if (!FilePath)
  {
    return 0;
  }

  if (_stat32(FilePath, &_oStats))
  {
    return 0;
  }

  return (size_t)_oStats.st_size;
}

FileSearch::FileSearch(void)
{
  ZeroMemory(this, sizeof(*this));
}

FileSearch::FileSearch(char* Pattern)
{
  ZeroMemory(this, sizeof(*this));

  strncpy_s(this->sPattern, sizeof(this->sPattern), Pattern, sizeof(this->sPattern) - 1);
}

FileSearch::~FileSearch(void)
{
  if (this->iSearchHandle)
  {
    _findclose(this->iSearchHandle);
  }
}

HRESULT FileSearch::FindFile()
{
  HRESULT _result;

  if (this->sPattern[0] == NULL)
  {
    return E_ABORT;
  }

  if (this->iSearchHandle == NULL)
  {
    _result = ((this->iSearchHandle = _findfirst(this->sPattern, &this->FoundFile)) == -1) ? E_FAIL : S_OK;
  }
  else
  {
    _result = (_findnext(this->iSearchHandle, &this->FoundFile)) ? E_FAIL : S_OK;
  }

  if (FAILED(_result))
  {
    if (this->iSearchHandle != -1)
    {
      _findclose(iSearchHandle);
    }
    
    this->iSearchHandle = NULL;

    ZeroMemory(&this->FoundFile, sizeof(this->FoundFile));
  }

  return _result;
}

HRESULT FileSearch::FindFile(char* Pattern)
{
  if (this->iSearchHandle)
  {
    _findclose(this->iSearchHandle);
  }

  ZeroMemory(this, sizeof(*this));

  strncpy_s(this->sPattern, sizeof(this->sPattern), Pattern, sizeof(this->sPattern) - 1);

  return this->FindFile();
}

LogFile::LogFile()
{
  ZeroMemory(this, sizeof(*this));
}

LogFile::~LogFile()
{
  this->Close();
}

HRESULT LogFile::Open(char* Filename, char* Header)
{
  this->Close();
  
  if (fopen_s(&this->oLogFile, Filename, "wb"))
  {
    this->oLogFile = NULL;
    
    return E_FAIL;
  }

  char _sTemp[_MAX_PATH];
  _sTemp[0] = 0x00;
  
  if (Header)
  {
    // Write Header to file
    int _iHeaderLen = strlen(Header);

    fwrite(Header, _iHeaderLen, 1, this->oLogFile);
    fwrite("\r\n", 2, 1, this->oLogFile);
    
    if (_iHeaderLen > (sizeof(_sTemp) - 1))
    {
      _iHeaderLen = (sizeof(_sTemp) - 1);
    }

    int _i;
    for (_i = 0; _i < _iHeaderLen; _i++)
    {
      _sTemp[_i] = '-';
    }

    _sTemp[_iHeaderLen++] = '\r';
    _sTemp[_iHeaderLen++] = '\n';
    _sTemp[_iHeaderLen++] = '\0';

    fwrite(_sTemp, _iHeaderLen - 1, 1, this->oLogFile);
  }
  this->Entry(QUIET, "Session started");

  _getcwd(_sTemp, sizeof(_sTemp));
  this->Entry(QUIET, "Current directory: %s", _sTemp);

  printf("Logging to %s...\n", Filename);

  return S_OK;
}

HRESULT LogFile::Close()
{
  if (this->oLogFile)
  {
    this->Entry(QUIET, "Session closed. Good-bye!");

    fclose(this->oLogFile);

    this->oLogFile = NULL;
  }

  return S_OK;
}

HRESULT LogFile::SetEcho(LogEcho Echo)
{
  if ((Echo != QUIET) && (Echo != ECHO))
  {
    return E_FAIL;
  }

  this->iEcho = Echo;

  return S_OK;
}

HRESULT LogFile::Entry(char* Message)
{
  return this->Entry(this->iEcho, Message);
}

HRESULT LogFile::Entry(LogEcho Echo, char* Message)
{
  if (oLogFile)
  {
    char _sDateTime[22];
    time_t _iDateTime;
    tm _oDateTime;

    if (time(&_iDateTime) != -1)
    {
      if (!localtime_s(&_oDateTime, &_iDateTime))
      {
        _sDateTime[ 0] = '[';
        _sDateTime[ 1] = '0' + ((_oDateTime.tm_year + 1900) / 1000 % 10);
        _sDateTime[ 2] = '0' + ((_oDateTime.tm_year + 1900) / 100 % 10);
        _sDateTime[ 3] = '0' + (_oDateTime.tm_year / 10 % 10);
        _sDateTime[ 4] = '0' + (_oDateTime.tm_year % 10);
        _sDateTime[ 5] = '-';
        _sDateTime[ 6] = '0' + ((_oDateTime.tm_mon + 1) / 10 % 10);
        _sDateTime[ 7] = '0' + ((_oDateTime.tm_mon + 1) % 10);
        _sDateTime[ 8] = '-';
        _sDateTime[ 9] = '0' + (_oDateTime.tm_mday / 10 % 10);
        _sDateTime[10] = '0' + (_oDateTime.tm_mday % 10);
        _sDateTime[11] = ' ';
        _sDateTime[12] = '0' + (_oDateTime.tm_hour / 10 % 10);
        _sDateTime[13] = '0' + (_oDateTime.tm_hour % 10);
        _sDateTime[14] = ':';
        _sDateTime[15] = '0' + (_oDateTime.tm_min / 10 % 10);
        _sDateTime[16] = '0' + (_oDateTime.tm_min % 10);
        _sDateTime[17] = ':';
        _sDateTime[18] = '0' + (_oDateTime.tm_sec / 10 % 10);
        _sDateTime[19] = '0' + (_oDateTime.tm_sec % 10);
        _sDateTime[20] = ']';
        _sDateTime[21] = ' ';

        fwrite(&_sDateTime, 1, 22, oLogFile);
      }
    }
    fputs(Message, oLogFile);
    fwrite("\r\n", 1, 2, oLogFile);
  }

  if (Echo)
  {
    printf("%s\n", Message);
  }

  return S_OK;
}

HRESULT LogFile::Entry(char* Format, void* Param1)
{
  return this->Entry(this->iEcho, Format, Param1, NULL, NULL);
}

HRESULT LogFile::Entry(LogEcho Echo, char* Format, void* Param1)
{
  return this->Entry(Echo, Format, Param1, NULL, NULL);
}

HRESULT LogFile::Entry(char* Format, void* Param1, void* Param2)
{
  return this->Entry(this->iEcho, Format, Param1, Param2, NULL);
}

HRESULT LogFile::Entry(LogEcho Echo, char* Format, void* Param1, void* Param2)
{
  return this->Entry(Echo, Format, Param1, Param2, NULL);
}

HRESULT LogFile::Entry(char* Format, void* Param1, void* Param2, void* Param3)
{
  return this->Entry(this->iEcho, Format, Param1, Param2, Param3);
}

HRESULT LogFile::Entry(LogEcho Echo, char* Format, void* Param1, void* Param2, void* Param3)
{
  return Entry(Echo, Format, Param1, Param2, Param3, NULL, NULL);
}

HRESULT LogFile::Entry(LogEcho Echo, char* Format, void* Param1, void* Param2, void* Param3, void* Param4)
{
  return Entry(Echo, Format, Param1, Param2, Param3, Param4, NULL);
}

HRESULT LogFile::Entry(LogEcho Echo, char* Format, void* Param1, void* Param2, void* Param3, void* Param4, void* Param5)
{
  if (Format)
  {
    char _sLogEntry[1024];
    _sLogEntry[0] = 0x00;

    if (Param5)
    {
      sprintf_s(_sLogEntry, sizeof(_sLogEntry), Format, Param1, Param2, Param3, Param4, Param5);

      return this->Entry(Echo, _sLogEntry);
    }
    else if (Param4)
    {
      sprintf_s(_sLogEntry, sizeof(_sLogEntry), Format, Param1, Param2, Param3, Param4);

      return this->Entry(Echo, _sLogEntry);
    }
    else if (Param3)
    {
      sprintf_s(_sLogEntry, sizeof(_sLogEntry), Format, Param1, Param2, Param3);

      return this->Entry(Echo, _sLogEntry);
    }
    else if (Param2)
    {
      sprintf_s(_sLogEntry, sizeof(_sLogEntry), Format, Param1, Param2);

      return this->Entry(Echo, _sLogEntry);
    }
    else if (Param1)
    {
      sprintf_s(_sLogEntry, sizeof(_sLogEntry), Format, Param1);

      return this->Entry(Echo, _sLogEntry);
    }
    else
    {
      return this->Entry(Echo, Format);
    }
  }

  // Null format?
  return E_FAIL;
}

void SplitString(char* String, char Sep, char* OutBuffer, size_t BufSize, WORD* OutNumParts, char** OutParts, WORD MaxParts)
{
  // Verify usable arguments

  if ((!String) || (!OutBuffer) || (!BufSize) || (!OutNumParts) || (!OutParts) || (!MaxParts))
  {
    if (OutNumParts)
    {
      *OutNumParts = 0;
    }
    return;
  }

  WORD _i;
  size_t _iLen;
  WORD _iPrev;
  WORD _iParts;

  _iLen = strlen(String);
  
  if ((_iLen + 1) > BufSize)
  {
    _iLen = BufSize - 1;
  }

  memcpy_s(OutBuffer, BufSize, String, _iLen);
  OutBuffer[_iLen] = 0x00;

  _iParts = 0;
  _iPrev = 0;

  for (_i = 0; (_i < _iLen) && (_iParts < (MaxParts - 1)); _i++)
  {
    if (OutBuffer[_i] == Sep)
    {
      OutBuffer[_i] = 0x00;

      OutParts[_iParts++] = &OutBuffer[_iPrev];
      
      _iPrev = _i + 1;
    }
  }

  OutParts[_iParts++] = &OutBuffer[_iPrev];
  *OutNumParts = _iParts;
}

char* MakePath(char* InPath, char* InFile)
{
  return MakePath(InPath, InFile, NULL, 0);
}

char* MakePath(char* InPath, char* InFile, char* OutPath, size_t OutPathSize)
{
  size_t _iInPathLen;
  size_t _iInFileLen;
  char*  _sPath;

  _iInPathLen = (InPath) ? strlen(InPath) : 0;
  _iInFileLen = (InFile) ? strlen(InFile) : 0;

  if (_iInPathLen)
  {
    while ((_iInPathLen > 0) && (InPath[_iInPathLen - 1] == '\\'))
    {
      _iInPathLen--;
    }
  }

  if (OutPath)
  {
    _sPath = OutPath;
  }
  else
  {
    _sPath = sMakePath_TempPath;
    
    OutPathSize = sizeof(sMakePath_TempPath);
  }

  // Not enough space in the buffer
  if (OutPathSize < (_iInPathLen + 1 + _iInFileLen + 1))
  {
    _sPath[0] = 0x00;

    return NULL;
  }

  if (_iInPathLen)
  {
    memcpy_s(_sPath, OutPathSize, InPath, _iInPathLen);
    
    if (_iInFileLen)
    {
      _sPath[_iInPathLen++] = '\\';
    }
    else
    {
      _sPath[_iInPathLen] = 0x00;
    }
  }

  if (_iInFileLen)
  {
    memcpy_s(&_sPath[_iInPathLen], OutPathSize - _iInPathLen, InFile, _iInFileLen + 1);
  }

  return _sPath;
}

size_t ReadLine(FILE* File, char* Buffer, size_t BufferSize)
{
  if ((!File) || (!Buffer) || (!BufferSize))
  {
    return 0;
  }

  size_t _iLen = 0;
  size_t _iLen2;
  char   _sLine[256];
  
  while ((_iLen == 0) && (fgets(Buffer, BufferSize, File)))
  {
    if (_iLen = strlen(Buffer))
    {
      if ((Buffer[_iLen - 1] != '\n'))
      {
        // Line too long. Drop the rest of it.
      
        while (fgets(_sLine, sizeof(_sLine), File))
        {
          if (_iLen2 = strlen(_sLine))
          {
            if ((_sLine[_iLen2 - 1] == '\n'))
            {
              // Done. Found end of line.
              
              break;
            }
          }
        }
      }

      TrimString(Buffer, &_iLen);
    }
  }

  return _iLen;
}

bool StringSame(char* String1, char* String2)
{
  return StringSame(String1, String2, false);
}

bool StringSame(char* String1, char* String2, bool IgnoreCase)
{
  if (String1 == String2)
  {
    return true;
  }

  if ((!String1) || (!String2))
  {
    return false;
  }

  return StringSame(String1, strlen(String1), String2, strlen(String2), IgnoreCase);
}

bool StringSame(char* String1, size_t Len1, char* String2)
{
  return StringSame(String1, Len1, String2, false);
}

bool StringSame(char* String1, size_t Len1, char* String2, bool IgnoreCase)
{
  if (String1 == String2)
  {
    return true;
  }

  if (!String2)
  {
    return false;
  }

  return StringSame(String1, Len1, String2, strlen(String2), IgnoreCase);
}

bool StringSame(char* String1, size_t Len1, char* String2, size_t Len2)
{
  return StringSame(String1, Len1, String2, Len2, false);
}

bool StringSame(char* String1, size_t Len1, char* String2, size_t Len2, bool IgnoreCase)
{
  if (String1 == String2)
  {
    return true;
  }

  if (Len1 != Len2)
  {
    return false;
  }

  if ((!String1) || (!String2))
  {
    return false;
  }

  if (IgnoreCase)
  {
    // Quick case-insensitive comparison via ASM
    
    bool _bSame;

    __asm
    {
      push eax;
      push ebx;
      push ecx;
      push edx;
      push esi;
      push edi;

      mov esi, String1;
      mov edi, String2;
      mov ecx, Len1;
      mov ebx, 0x20202020;

      mov edx, ecx;
      and edx, 3;
      push edx;     // Stack holds trailing bytes to compare after

      shr ecx, 2;
      jz Trailing;

    MainLoop:
      lodsd;
      or eax, ebx;
      mov edx, [edi];
      or edx, ebx;
      add edi, 4;

      cmp eax, edx;
      jnz NotEqual1;

      loop MainLoop;

    Trailing:
      pop ecx;
      test ecx, ecx;
      jz Equal;

    TrailLoop:
      lodsb;
      or al, bl;
      mov ah, [edi];
      or ah, bl;
      inc edi;

      cmp al, ah;
      jnz NotEqual2;

      loop TrailLoop;

      jmp Equal;

    NotEqual1:
      pop ecx;

    NotEqual2:
      mov _bSame, 0;
      jmp Done;

    Equal:
      mov _bSame, 1;

    Done:
      pop edi;
      pop esi;
      pop edx;
      pop ecx;
      pop ebx;
      pop eax;
    }

    return _bSame;
  }
  else
  {
    if (String1[0] != String2[0])
    {
      return false;
    }

    return (memcmp(String1, String2, Len1) == 0);
  }
}
