// S3DPackage.cpp -- Handle EQ S3D and EQG package files

#include "S3DPackage.h"

char* EQGL_Entry::GetName(char* StringTable)
{
  return (StringTable) ? &StringTable[this->NamePtr] : NULL;
}

char* EQGL_Entry::GetFile1(char* StringTable)
{
  return (StringTable) ? &StringTable[this->File1Ptr] : NULL;
}

char* EQGL_Entry::GetFile2(char* StringTable)
{
  return (StringTable) ? &StringTable[this->File2Ptr] : NULL;
}

EQGLFile::EQGLFile(void)
{
  ZeroMemory(this, sizeof(*this));
}

EQGLFile::~EQGLFile(void)
{
  this->Close();
}

void EQGLFile::Close()
{
  SAFE_DELETE(this->Entry);

  if (this->FileCopied)
  {
    SAFE_DELETE(this->FileBytes);
  }
}

HRESULT EQGLFile::LoadFromMemory(BYTE* Buffer, DWORD FileSize, bool StableBuffer, char* Filename)
{
  if ((!Buffer) || (!FileSize))
  {
    return E_INVALIDARG;
  }

  this->Close();
  
  if (IsBlank(Filename))
  {
    this->FilenameLen = 10;
    memcpy_s(this->Filename, sizeof(this->Filename), "NoName.lay", this->FilenameLen + 1);
  }
  else
  {
    this->FilenameLen = strlen(Filename);

    if (this->FilenameLen >= _MAX_PATH)
    {
      this->FilenameLen = _MAX_PATH - 1;
    }

    memcpy_s(this->Filename, sizeof(this->Filename), Filename, this->FilenameLen);
    this->Filename[this->FilenameLen] = 0x00;
  }

  if (FileSize < sizeof(EQGL_Header))
  {
    return E_FAIL;
  }

  if (StableBuffer)
  {
    this->FileCopied = false;
    this->FileBytes = Buffer;
  }
  else
  {
    this->FileCopied = true;
    if (!(this->FileBytes = new BYTE[FileSize]))
    {
      return E_OUTOFMEMORY;
    }

    memcpy_s(this->FileBytes, FileSize, Buffer, FileSize);
  }

  this->FileSize = FileSize;

  EQGL_Header* _oHeader = (EQGL_Header*)this->FileBytes;

  if (_oHeader->MagicNumber != EQGL_MAGICNUMBER)
  {
    // NOT an EQG .lay file!
    this->Close();
    
    return E_FAIL;
  }

  this->StringTable = (char*)&this->FileBytes[sizeof(EQGL_Header)];
  this->StringTableSize = _oHeader->StringTableSize;
  this->Entries = _oHeader->NumEntries;
  if (!(this->Entry = new EQGL_Entry[this->Entries]))
  {
    return E_OUTOFMEMORY;
  }

  memcpy_s( this->Entry,
            this->Entries * sizeof(EQGL_Entry),
            &this->FileBytes[this->StringTableSize + sizeof(EQGL_Header)],
            this->Entries * sizeof(EQGL_Entry));

  return S_OK;
}

S3DPackage::S3DPackage(void)
{
  ZeroMemory(this, sizeof(*this));

  // Build CRC Table
  DWORD _i;
  DWORD _j;
  DWORD _c;

  for (_i = 0; _i < 256; _i++)
  {
    _c = _i << 24;
    
    for (_j = 0; _j < 8; _j++)
    {
      _c = (_c & 0x80000000) ? ((_c << 1) ^ S3D_CRCPOLY) : (_c << 1);
    }

    this->iCRCTable[_i] = _c;
  }
}

S3DPackage::~S3DPackage(void)
{
  this->Close();
}

DWORD S3DPackage::GetVersion()
{  
  return this->iVersionNumber;
}

DWORD S3DPackage::GetDateStamp()
{
  return this->iDateStamp;
}

DWORD S3DPackage::GetFileCount()
{
  return this->iFiles;
}

char* S3DPackage::GetFilename(DWORD FileNumber)
{
  if (this->iFiles == 0)
  {
    return NULL;
  }

  if (FileNumber >= this->iFiles)
  {
    return NULL;
  }

  return this->oFileInfo[FileNumber].Filename;
}

S3DFileInfo*  S3DPackage::GetFileInfo(DWORD FileNumber)
{
  if (this->iFiles == 0)
  {
    return NULL;
  }

  if (FileNumber >= this->iFiles)
  {
    return NULL;
  }

  return &this->oFileInfo[FileNumber];
}

DWORD S3DPackage::GetCRC(BYTE *Source, DWORD SourceSize)
{
  if ((!Source) || (!SourceSize))
  {
    return 0;
  }

  DWORD _i;
  DWORD _c = 0;
  BYTE* _pos;
  BYTE* _last = &Source[SourceSize - 1];
  
  for (_pos = Source; _pos <= _last; _pos++)
  {
    _i = ((_c >> 24) ^ *_pos) & 0xFF;
    _c = ((_c << 8) ^ this->iCRCTable[_i]);
  }

  return _c;
}

HRESULT S3DPackage::Open(char* Filename)
{
  this->Close();

  if (!Filename)
  {
    return E_INVALIDARG;
  }
  
  if (Filename[0] == NULL)
  {
    return E_FAIL;
  }

  this->iVersionNumber = this->iDateStamp = 0;

  strncpy_s(this->sPackage, sizeof(this->sPackage), Filename, _TRUNCATE);

  FILE* _oFile;

  if (fopen_s(&_oFile, this->sPackage, "rb"))
  {
    return E_FAIL;
  }

  this->iPackageSize = _filelength(_fileno(_oFile));

  if ((this->aPackageBytes = new BYTE[this->iPackageSize]) == NULL)
  {
    fclose(_oFile);
    
    return E_OUTOFMEMORY;
  }

  // The largest S3D/EQG file I've seen is still under 37 MB.
  //
  // We'll process fastest by just reading the whole file into memory and working from there.
  if (fread(this->aPackageBytes, this->iPackageSize, 1, _oFile) != 1)
  {
    fclose(_oFile);
    
    return E_FAIL;
  }

  fclose(_oFile);

  bool      _bOK;
  DWORD      _iDirectoryOffset;
  DWORD      _iFiles;
  DWORD      _i;
  DWORD      _iFileListEntry = -1;
  DWORD      _iFileListOffset = 0;

  _iDirectoryOffset = ReadDWord();
  _bOK = (ReadDWord() == S3D_MAGICNUMBER);

  if (_bOK)
  {
    this->iVersionNumber = ReadDWord();
    
    if (_bOK = _iDirectoryOffset < this->iPackageSize)
    {
      this->iPackagePos = _iDirectoryOffset;
    }
  }

  if (_bOK)
  {
    _iFiles = ReadDWord();

    _bOK = (_iFiles > 0);
  }

  if (_bOK)
  {
    this->iFiles = _iFiles - 1;
    this->oFileInfo = new S3DFileInfo[_iFiles];

    _bOK = (this->oFileInfo != NULL);
  }

  if (_bOK)
  {
    ZeroMemory(this->oFileInfo, sizeof(S3DFileInfo) * _iFiles);
    
    for (_i = 0; _bOK && (_i < _iFiles); _i++)
    {
      this->oFileInfo[_i].CRC = ReadDWord();
      this->oFileInfo[_i].FileOffset = ReadDWord();
      this->oFileInfo[_i].Size = ReadDWord();

      _bOK = (this->oFileInfo[_i].FileOffset > 0);
      /*
      if (this->oFileInfo[_i].FileOffset > _iFileListOffset)
      {
        _iFileListOffset = this->oFileInfo[_i].FileOffset;
        _iFileListEntry = _i;
      }
      */
    }
  }

  /*
  if (_bOK)
  {
    _bOK = (_iFileListEntry >= 0);
  }
  */

  if (_bOK)
  {
    if ((this->iPackageSize - this->iPackagePos) > 5)
    {
      if (StringSame("STEVE", 5, (char*)&this->aPackageBytes[this->iPackagePos], true))
      {
        // Recognizable _oFooter.

        this->iPackagePos += 5;
        this->iDateStamp = ReadDWord();
      }
    }
  }

  // Resort directory entries by file offset to match expected archive behavior, instead of the S3D's convention of by CRC.
  if (_bOK)
  {
    DWORD _iLastOffset;
    DWORD _iSrcEntry;
    DWORD _iFile;
    S3DFileInfo _oSortEntry; // Swap Holder For Sorting
    
    for (_iFile = 0; _iFile < _iFiles; _iFile++)
    {
      _iLastOffset = this->oFileInfo[_iFile].FileOffset;
      _iSrcEntry = 0;
      
      for (_i = _iFile + 1; _i < _iFiles; _i++)
      {
        if (this->oFileInfo[_i].FileOffset < _iLastOffset)
        {
          _iSrcEntry = _i;
          _iLastOffset = this->oFileInfo[_i].FileOffset;
        }
      }

      if (_iSrcEntry > 0)
      {
        _oSortEntry = this->oFileInfo[_iFile];
        this->oFileInfo[_iFile] = this->oFileInfo[_iSrcEntry];
        this->oFileInfo[_iSrcEntry] = _oSortEntry;
      }
    }
  }

  // Read File List
  if (_bOK)
  {
    S3DFileInfo* _oFileList;

    _oFileList = &this->oFileInfo[_iFiles - 1];
    
    BYTE* _aFileList = NULL;
    DWORD _iFileListPos = 0;
    
    _bOK = SUCCEEDED(this->ReadFileEntry(_oFileList->FileOffset, _oFileList->Size, &_aFileList));

    if (_bOK)
    {
      // Get Filenames

      DWORD _iFilenameLen;
      DWORD _iFilenameCRC;
      DWORD _iFile;

      _i = *(DWORD*)&_aFileList[_iFileListPos];
      _iFileListPos += 4;

      if (_i < this->iFiles)
      {
        this->iFiles = _i;
      }

      for (_iFile = 0; _iFile < this->iFiles; _iFile++)
      {
        _iFilenameLen = *(DWORD*)&_aFileList[_iFileListPos];
        _iFileListPos += 4;
        _iFilenameCRC = GetCRC(&_aFileList[_iFileListPos], _iFilenameLen);

        if (this->oFileInfo[_iFile].CRC == _iFilenameCRC)
        {
          this->oFileInfo[_iFile].FilenameLength = _iFilenameLen - 1;

          memcpy_s(  this->oFileInfo[_iFile].Filename,
                    sizeof(this->oFileInfo[_iFile].Filename),
                    &_aFileList[_iFileListPos],
                    _iFilenameLen);
        }
        else
        {
          BreakPoint; // CRC order didn't match up?!
        }

        _iFileListPos += _iFilenameLen;
      }

      delete _aFileList;
    }
  }

  if (!_bOK)
  {
    Close();
  }
  
  return (_bOK) ? S_OK : E_FAIL;
}

HRESULT S3DPackage::Close()
{
  if (this->aPackageBytes)
  {
    delete this->aPackageBytes;

    this->aPackageBytes = NULL;
  }

  if (this->oFileInfo)
  {
    delete this->oFileInfo;

    this->oFileInfo = NULL;
  }

  // Preserve these, in case the user can retrieve usable data from them after an error or something.
  //
  // this->iVersionNumber = 0;
  // this->iDateStamp = 0;
  // this->sPackage[0] = NULL;

  this->iFiles = 0;
  this->iPackageSize = 0;
  this->iPackagePos = 0;

  return S_OK;
}

DWORD S3DPackage::FindFile(char* Filename)
{
  if (IsBlank(Filename))
  {
    return S3D_INVALID;
  }

  size_t _iFilenameLen = strlen(Filename);

  if ((_iFilenameLen < 3) || (_iFilenameLen > 63))
  {
    return S3D_INVALID;
  }

  char _sFilename[64];
  memcpy_s(_sFilename, sizeof(_sFilename), Filename, _iFilenameLen + 1);
  TrimString(_sFilename, &_iFilenameLen);
  
  WORD _i;
  S3DFileInfo* _oFile;

  for (_i = 0; _i < this->iFiles; _i++)
  {
    _oFile = &this->oFileInfo[_i];

    if (StringSame(_sFilename, _iFilenameLen, _oFile->Filename, _oFile->FilenameLength, true))
    {
      return _i;
    }
  }

  return S3D_INVALID;
}

HRESULT S3DPackage::ReadFileEntry(DWORD FileOffset, DWORD SizeUncompressed, BYTE** DestBuffer)
{
  if (!DestBuffer)
  {
    return E_INVALIDARG;
  }

  if (FileOffset >= this->iPackageSize)
  {
    return E_INVALIDARG;
  }

  if (!this->aPackageBytes)
  {
    return E_FAIL;
  }

  // Nothing to do
  if (SizeUncompressed == 0)
  {
    return S_OK;
  }

  BYTE* _aDest = *DestBuffer;
  bool _bAllocated = (_aDest == NULL);
  
  if (_bAllocated)
  {
    // In case of errors. Don't want to leave it undefined
    *DestBuffer = NULL;

    // Little padding to give us a little bit of leeway in the unlikely case of buggy zlib, corrupted data, etc.
    _aDest = new BYTE[SizeUncompressed + 1024];
    
    if (!_aDest)
    {
      return E_OUTOFMEMORY;
    }
  }

  this->iPackagePos = FileOffset;
  
  DWORD    _iDataBytesDecompressed = 0;
  DWORD    _iBlockSize_Compressed;
  DWORD    _iBlockSize_Uncompressed;
  uLongf  _iActualBlockSize_Uncompressed;

  bool    _bOK = true;

  while (_bOK && (_iDataBytesDecompressed < SizeUncompressed) && (this->iPackagePos < this->iPackageSize))
  {
    _iBlockSize_Compressed = ReadDWord();
    _iBlockSize_Uncompressed = ReadDWord();

    // Sanity Check 1
    if ((_iBlockSize_Uncompressed + _iDataBytesDecompressed) > SizeUncompressed)
    {
      if (_bAllocated)
      {
        delete _aDest;
      }

      return E_FAIL;
    }

    // Sanity Check 2
    if ((this->iPackagePos + _iBlockSize_Compressed) > this->iPackageSize)
    {
      if (_bAllocated)
      {
        delete _aDest;
      }

      return E_FAIL;
    }

    _iActualBlockSize_Uncompressed = _iBlockSize_Uncompressed;
    _bOK = (uncompress(  &_aDest[_iDataBytesDecompressed],
                        &_iActualBlockSize_Uncompressed,
                        &this->aPackageBytes[this->iPackagePos],
                        _iBlockSize_Compressed) == Z_OK);

    this->iPackagePos += _iBlockSize_Compressed;
    _iDataBytesDecompressed += _iActualBlockSize_Uncompressed;

    if (_iBlockSize_Uncompressed != _iActualBlockSize_Uncompressed)
    {
      BreakPoint;
    }
  }

  if (_bAllocated)
  {
    if (_bOK)
    {
      *DestBuffer = _aDest;
    }
    else
    {
      delete _aDest;
    }
  }

  return (_bOK) ? S_OK : E_FAIL;
}

BYTE* S3DPackage::LoadFile(DWORD FileNumber)
{
  if (!this->aPackageBytes)
  {
    return NULL;
  }

  if (this->iFiles == 0)
  {
    return NULL;
  }

  if (FileNumber > (this->iFiles - 1))
  {
    return NULL;
  }

  BYTE* _buf = NULL;

  if (SUCCEEDED(ReadFileEntry(this->oFileInfo[FileNumber].FileOffset, this->oFileInfo[FileNumber].Size, &_buf)))
  {
    return _buf;
  }
  else
  {
    if (_buf)
    {
      delete _buf;
    }

    return NULL;
  }
}

DWORD S3DPackage::ReadDWord()
{
  if (!this->aPackageBytes)
  {
    return 0;
  }

  if (this->iPackagePos >= this->iPackageSize)
  {
    return 0;
  }

  DWORD _iResult;
  DWORD _iSize;

  // How many bytes left in file? Don't want to read beyond the end!
  _iSize = this->iPackageSize - this->iPackagePos;

  switch (_iSize)
  {
    case 1:
      _iResult = aPackageBytes[this->iPackagePos];
      break;
    case 2:
      _iResult = *(WORD*)&aPackageBytes[this->iPackagePos];
      break;
    case 3:
      _iResult = *(WORD*)&aPackageBytes[this->iPackagePos] + (aPackageBytes[this->iPackagePos + 2] << 16);
      break;
    default:
      _iResult = *(DWORD*)&aPackageBytes[this->iPackagePos];
      _iSize = 4;
      break;
  }

  this->iPackagePos += _iSize;

  return _iResult;
}
