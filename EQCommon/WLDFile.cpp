// WLDFile.cpp -- Limited handling of EQ WLD files

#include "WLDFile.h"

WLDFile::WLDFile(void)
{
  ZeroMemory(this, sizeof(*this));

  memset(this->iFileFragFirst, 0xFF, sizeof(this->iFileFragFirst));
}

WLDFile::~WLDFile(void)
{
  _Close();
}

void WLDFile::_Close()
{
  if (this->oExportFile)
  {
    fclose(this->oExportFile);

    this->oExportFile = NULL;
  }
  
  if (this->bFileCopied)
  {
    SAFE_DELETE(this->aFileBytes);
  }
  
  SAFE_DELETE(this->oFragment);
  SAFE_DELETE(this->iFileFragType);
  SAFE_DELETE(this->iFileFragNext);
  SAFE_DELETE(this->aStringOffsets);
  SAFE_DELETE(this->aStringLengths);

  ZeroMemory(this, sizeof(*this));

  memset(this->iFileFragFirst, 0xFF, sizeof(this->iFileFragFirst));
}

DWORD WLDFile::GetVersion()
{
  return this->iVersion;
}

DWORD WLDFile::GetDataOffset()
{
  return this->iStringHashSize + sizeof(WLDFileHeader);
}

HRESULT WLDFile::Decrypt(char* Buffer, DWORD Size, BYTE StartIndex, DWORD* Out_StringCount)
{
  DWORD _i;

  if (Out_StringCount)
  {
    *Out_StringCount = 0;
  }

  if ((Buffer) && (Size))
  {
    if (Out_StringCount)
    {
      for (_i = 0; _i < Size; _i++)
      {
        if ((*(Buffer++) ^= WLD_XOR[(_i + StartIndex) & 7]) == 0)
        {
          (*Out_StringCount)++;
        }
      }
    }
    else
    {
      // If we're not storing a StringCount somewhere, don't even bother counting. 

      for (_i = 0; _i < Size; _i++)
      {
        *(Buffer++) ^= WLD_XOR[(_i + StartIndex) & 7];
      }
    }

    return S_OK;
  }
  else
  {
    return E_INVALIDARG;
  }
}

WLDFragment* WLDFile::FindNextFragment(BYTE Type, WLDFragment* Current)
{
  DWORD _iStart;
  DWORD _i;

  if (Type > WLD_MAXFRAGTYPE)
  {
    return NULL;
  }

  if ((!Current) || ((Current->FragType != Type) && (Type > 0)))
  {
    // Start a new iteration, or perform a clumsy search.

    if (!Current)
    {
      // They want the first fragment of the type.
      if ((this->iFileFragFirst == NULL) || ((_i = this->iFileFragFirst[Type]) == 0xFFFFFFFF))
      {
        // No fragments of that type, or WLDFile not loaded.
        
        return NULL;
      }
      else
      {
        if (_i >= this->iFragCountTotal)
        {
          // Corrupted index
          BreakPoint;
          
          return NULL;
        }

        return &this->oFragment[_i];
      }
    }

    // Start after the current fragment, and search for one of the type
    _iStart = Current->FileIndex + 1;
    for (_i = _iStart; _i < this->iFragCountTotal; _i++)
    {
      if (this->iFileFragType[_i] == Type)
      {
        return &this->oFragment[_i];
      }
    }
  }
  else
  {
    // Continue a started iteration
    if (Current->FileIndex >= this->iFragCountTotal)
    {
      // FileIndex corrupted
      BreakPoint;
      
      return NULL;
    }

    if (_i = this->iFileFragNext[Current->FileIndex])
    {
      // Sanity checks.
      if ((_i < this->iFragCountTotal) && (this->iFileFragType[_i] == Current->FragType))
      {
        return &this->oFragment[_i];
      }
      else
      {
        // Corrupted index
        BreakPoint;

        return NULL;
      }
    }
  }

  // Didn't find any (more) fragments of the specified type.
  return NULL;
}

WLDFragment* WLDFile::GetFragment(DWORD Index)
{
  if ((Index == 0) || (Index >= this->iFragCountTotal))
  {
    return NULL;
  }

  return &this->oFragment[Index - 1];
}

HRESULT WLDFile::LoadFromMemory(BYTE* Buffer, DWORD FileSize, bool StableBuffer, char* Filename, bool StringsOnly)
{
  if ((!Buffer) || (!FileSize))
  {
    return E_INVALIDARG;
  }

  if (FileSize < sizeof(WLDFileHeader))
  {
    return E_FAIL;
  }

  WLDFileHeader* _oHeader;
  
  _oHeader = (WLDFileHeader*)Buffer;

  if (_oHeader->MagicNumber != WLD_MAGICNUMBER)
  {
    return E_FAIL; // Not a valid WLD file.
  }

  // Clean up from any previous loads and start with a clean slate.
  this->_Close();

  this->iFileSize = FileSize;

  this->iVersion = _oHeader->Version;

  if (Filename)
  {
    this->iFilenameLen = strlen(Filename);

    if (this->iFilenameLen >= _MAX_PATH)
    {
      return E_INVALIDARG;
    }
  }
  else
  {
    this->iFilenameLen = 10;
    Filename = "NoName.wld\0";
  }

  memcpy_s(this->sFilename, sizeof(this->sFilename), Filename, this->iFilenameLen + 1);

  if (StableBuffer)
  {
    // StableBuffer = We don't have to make a copy of the file. It'll stay in memory as long as we need it.
    
    this->aFileBytes = Buffer;

    this->bFileCopied = false;
  }
  else
  {
    // Non-Stable Buffer = We have no guarantee that the file will stay in memory. Make a copy.
    
    this->aFileBytes = new BYTE[FileSize];
 
    if (this->aFileBytes == NULL)
    {
      return E_OUTOFMEMORY; // Crud!
    }

    memcpy_s(this->aFileBytes, FileSize, Buffer, FileSize);
  
    this->bFileCopied = true;
  }

  DWORD _i;
  DWORD _j;
  DWORD _s;

  this->iStringHashSize = _oHeader->StringHashSize;
  this->aStringHash = (char*)&this->aFileBytes[sizeof(WLDFileHeader)];

  // Decrypt String Hash (and count strings while we're at it)
  WLDFile::Decrypt(this->aStringHash, _oHeader->StringHashSize, 0, &this->iStrings);

  this->aStringOffsets = new DWORD[this->iStrings];
  this->aStringLengths = new DWORD[this->iStrings];
  
  _j = 0;
  _s = 0;
  
  // Now find string offsets and lengths
  for (_i = 0; _i < _oHeader->StringHashSize; _i++)
  {
    if (!this->aStringHash[_i])
    {
      this->aStringOffsets[_s] = _j;
      this->aStringLengths[_s++] = _i - _j;
      _j = _i + 1;
    }
  }

  if (StringsOnly)
  {
    return S_OK;
  }

  // Load fragments
  DWORD _iSize;
  DWORD _iID;
  BYTE  _iFragType;
  DWORD _iFragPrev[WLD_MAXFRAGTYPE + 1];

  memset(_iFragPrev, 0xFF, sizeof(_iFragPrev));

  this->iFragCountTotal = _oHeader->FragmentCount;
  this->iFilePos = sizeof(WLDFileHeader) + this->iStringHashSize;
  this->oFragment = new WLDFragment[this->iFragCountTotal];
  if (!this->oFragment)
  {
    return E_OUTOFMEMORY;
  }

  ZeroMemory(this->oFragment, this->iFragCountTotal * sizeof(WLDFragment));

  if (!(this->iFileFragType = new BYTE[this->iFragCountTotal]))
  {
    return E_OUTOFMEMORY;
  }

  ZeroMemory(this->iFileFragType, this->iFragCountTotal * sizeof(BYTE));
  
  if (!(this->iFileFragNext = new DWORD[this->iFragCountTotal]))
  {
    return E_OUTOFMEMORY;
  }

  ZeroMemory(this->iFileFragNext, this->iFragCountTotal * sizeof(DWORD));
  
  sint32 _iNameRef;
  char*  _sNameRef;

  this->iFileIndex = 0;

  while (this->iFilePos < this->iFileSize)
  {
    _iSize = _ReadDWord();
    if (_iSize != 0xFFFFFFFF)
    {
      _iSize -= 4;
      _iID = _ReadDWord();
      _iNameRef = _ReadDWord();

      if (_iID > WLD_MAXFRAGTYPE)
      {
        // Unrecognized Fragment Type
        BreakPoint;
      }
      else
      {
        _iFragType = (BYTE)_iID;

        if (_iNameRef < 0)
        {
          _sNameRef = &this->aStringHash[-_iNameRef];
        }
        else
        {
          _sNameRef = NULL;
        }
        
        this->oFragment[this->iFileIndex].LoadFrag(this, &this->aFileBytes[this->iFilePos], _iSize, this->iFilePos - this->iStringHashSize - sizeof(WLDFileHeader), _sNameRef, _iFragType, this->iFileIndex, this->iFragIndex[_iFragType]);
        this->iFragIndex[_iFragType]++;
        this->iFragCount[_iFragType]++;
        this->iFileFragType[this->iFileIndex] = _iFragType;

        if (this->iFileFragFirst[_iFragType] == 0xFFFFFFFF)
        {
          this->iFileFragFirst[_iFragType] = this->iFileIndex;
        }
        else if (_iFragPrev[_iFragType] != 0xFFFFFFFF)
        {
          this->iFileFragNext[_iFragPrev[_iFragType]] = this->iFileIndex;
        }
        
        _iFragPrev[_iFragType] = this->iFileIndex++;
      }

      this->iFilePos += _iSize;
    }
  }

  return S_OK;
}

HRESULT WLDFile::ExportToTextFile()
{
  DWORD _i;
  char  _sFilename[_MAX_PATH];

  if (this->oExportFile)
  {
    fclose(this->oExportFile);
  }

  sprintf_s(_sFilename, sizeof(_sFilename), "%s.txt", this->sFilename);

  if (fopen_s(&this->oExportFile, _sFilename, "wb"))
  {
    this->oExportFile = NULL;
    
    return E_FAIL;
  }

  ZeroMemory(this->iFragIndex, sizeof(this->iFragIndex));
    
  fprintf_s(this->oExportFile,
            "WLD File Text Export\r\n"
            "--------------------\r\n"
            "Filename: %s\r\n"
            "Version: %s (0x%08x)\r\n"
            "Strings: %d\r\n",
            this->sFilename,
              (this->iVersion == WLDVer1) ? "Version 1" :
              (this->iVersion == WLDVer2) ? "Version 2" :
              "Unknown",
            this->iVersion,
            this->iStrings);

  fprintf_s(this->oExportFile, "Fragments: (Total %d)\r\n", this->iFragCountTotal);
  for (_i = 0; _i <= WLD_MAXFRAGTYPE; _i++)
  {
    if (this->iFragCount[_i])
    {
      fprintf_s(this->oExportFile, "- Type 0x%02x = %d\r\n", _i, this->iFragCount[_i]);
    }
  }

  fprintf_s(this->oExportFile, "\r\n");
  
  for (_i = 0; _i < this->iFragCountTotal; _i++)
  {
    this->oFragment[_i].Export(this->oExportFile, true, false);
  }

  fclose(this->oExportFile);

  return S_OK;
}

HRESULT WLDFile::ExportFragment(DWORD Index)
{
  if (!this->oExportFile)
  {
    return E_FAIL;
  }

  if (Index == 0)
  {
    fprintf_s(this->oExportFile, "-- Reference to Nothing\r\n");

    return S_OK;
  }
  else if (Index & 0x80000000)
  {
    fprintf_s(this->oExportFile, "-- Reference to string '%s'\r\n", (char*)(Index & 0x7FFFFFFF));

    return S_OK;
  }
  else if (Index >= this->iFragCountTotal)
  {
    return E_INVALIDARG;
  }
  else
  {
    //fprintf_s(this->oExportFile, "-- Reference to Fragment %d\r\n", Index);
    return this->oFragment[Index - 1].Export(this->oExportFile, true, true);

    return S_OK;
  }
}

char*  WLDFile::GetFilename()
{
  return this->sFilename;
}

DWORD  WLDFile::GetStringCount()
{
  return this->iStrings;
}

char*  WLDFile::GetString(DWORD StringNumber)
{
  if (StringNumber < this->iStrings)
  {
    return &this->aStringHash[this->aStringOffsets[StringNumber]];
  }
  else
  {
    return NULL;
  }
}

HRESULT WLDFile::CopyString(DWORD StringNumber, char* Dest, DWORD DestSize)
{
  if ((!Dest) || (StringNumber >= this->iStrings) || (DestSize == 0))
  {
    return E_INVALIDARG;
  }

  if (DestSize <= this->aStringLengths[StringNumber])
  {
    return E_FAIL;
  }

  memcpy_s(Dest, DestSize, &this->aStringHash[this->aStringOffsets[StringNumber]], this->aStringLengths[StringNumber] + 1);

  return S_OK;
}

DWORD WLDFile::_ReadDWord()
{
  this->iFilePos += sizeof(DWORD);

  return *((DWORD*)(&this->aFileBytes[this->iFilePos - sizeof(DWORD)]));
}

DWORD WLDFile::_ReadDWord(void* Buffer, DWORD* Pointer)
{
  if (!Buffer)
  {
    return 0;
  }

  if (Pointer)
  {
    *Pointer += sizeof(DWORD);
  }

  return *((DWORD*)Buffer);
}

WORD WLDFile::_ReadWord()
{
  this->iFilePos += sizeof(WORD);

  return *((WORD*)(&this->aFileBytes[this->iFilePos - sizeof(WORD)]));
}

WORD WLDFile::_ReadWord(void* Buffer, DWORD* Pointer)
{
  if (!Buffer)
  {
    return 0;
  }

  if (Pointer)
  {
    *Pointer += sizeof(WORD);
  }

  return *((WORD*)Buffer);
}

float WLDFile::_ReadFloat()
{
  this->iFilePos += sizeof(float);

  return *((float*)(&this->aFileBytes[this->iFilePos - sizeof(float)]));
}

float WLDFile::_ReadFloat(void* Buffer, DWORD* Pointer)
{
  if (!Buffer)
  {
    return 0.0f;
  }

  if (Pointer)
  {
    *Pointer += sizeof(float);
  }

  return *((float*)Buffer);
}

DWORD WLDFile::FindFragmentRef(sint32 Reference)
{
  DWORD _iRef;
  DWORD _i;
  char* _sName;
  
  if (Reference >= 0)
  {
    return Reference;
  }

  _iRef = (Reference ^ 0xFFFFFFFF);

  _sName = &this->aStringHash[_iRef];

  for (_i = 0; _i < this->iFragCountTotal; _i++)
  {
    if (this->oFragment[_i].Name == _sName)
    {
      return _i + 1;
    }
  }

  return (((DWORD)_sName) | 0x80000000) + 1;
}

HRESULT WLDFragment::Hexport(FILE* File, bool BinaryMode, bool HeaderOnly)
{
  if (!File)
  {
    return E_INVALIDARG;
  }

  char* _sLF;
  BYTE* _pBuf = this->FragPointer;
  char* _sFragType;

  if (this->FragType <= WLD_MAXFRAGTYPE)
  {
    _sFragType = (char*)WLD_FRAGTYPENAME[this->FragType];
  }
  else
  {
    _sFragType = "";
  }

  _sLF = (BinaryMode) ? "\r\n" : "\n";

  fprintf_s(File, "----------------------------------------------%s", _sLF);
  
  fprintf_s(File, "Fragment # %d (File Location 0x%06x), Type 0x%02x (%s)", this->FileIndex + 1, this->FilePos, this->FragType, IfBlank(_sFragType, "Unknown Type"));

  if (IsBlank(this->Name))
  {
    fprintf_s(File, "%s", _sLF);
  }
  else
  {
    fprintf_s(File, " - %s%s", this->Name, _sLF);
  }
  
  DWORD _iPos = 0;
  DWORD _iBytes = (HeaderOnly) ? 0 : this->FragSize;

  if (_iBytes)
  {
    fprintf_s(File, "%s", _sLF);
  }

  while (_iBytes > 15)
  {
    fprintf_s(File, "%06x: %02x %02x %02x %02x | %02x %02x %02x %02x - %02x %02x %02x %02x | %02x %02x %02x %02x    %c%c%c%c %c%c%c%c  %c%c%c%c %c%c%c%c%s", _iPos,
      _pBuf[_iPos + 0x0], _pBuf[_iPos + 0x1], _pBuf[_iPos + 0x2], _pBuf[_iPos + 0x3],
      _pBuf[_iPos + 0x4], _pBuf[_iPos + 0x5], _pBuf[_iPos + 0x6], _pBuf[_iPos + 0x7],
      _pBuf[_iPos + 0x8], _pBuf[_iPos + 0x9], _pBuf[_iPos + 0xA], _pBuf[_iPos + 0xB],
      _pBuf[_iPos + 0xC], _pBuf[_iPos + 0xD], _pBuf[_iPos + 0xE], _pBuf[_iPos + 0xF],
      SAFE_OUTCHAR(_pBuf[_iPos + 0x0]), SAFE_OUTCHAR(_pBuf[_iPos + 0x1]), SAFE_OUTCHAR(_pBuf[_iPos + 0x2]), SAFE_OUTCHAR(_pBuf[_iPos + 0x3]),
      SAFE_OUTCHAR(_pBuf[_iPos + 0x4]), SAFE_OUTCHAR(_pBuf[_iPos + 0x5]), SAFE_OUTCHAR(_pBuf[_iPos + 0x6]), SAFE_OUTCHAR(_pBuf[_iPos + 0x7]),
      SAFE_OUTCHAR(_pBuf[_iPos + 0x8]), SAFE_OUTCHAR(_pBuf[_iPos + 0x9]), SAFE_OUTCHAR(_pBuf[_iPos + 0xA]), SAFE_OUTCHAR(_pBuf[_iPos + 0xB]),
      SAFE_OUTCHAR(_pBuf[_iPos + 0xC]), SAFE_OUTCHAR(_pBuf[_iPos + 0xD]), SAFE_OUTCHAR(_pBuf[_iPos + 0xE]), SAFE_OUTCHAR(_pBuf[_iPos + 0xF]),
      _sLF);

    _iPos += 16;
    _iBytes -= 16;
  }

  if (_iBytes)
  {
    fprintf_s(File, "%06x:", _iPos);

    DWORD _i = 0;
    DWORD _j = 0;

    while (_i < 16)
    {
      if (_iBytes)
      {
        fprintf_s(File, " %02x", _pBuf[_iPos + _i]);

        _iBytes--;
      }
      else
      {
        fprintf_s(File, "   ");

        if (_j == 0)
        {
          _j = _i;
        }
      }

      switch (++_i)
      {
        case 4:
        case 12:
          fprintf_s(File, " |");
          break;
        case 8:
          fprintf_s(File, " -");
          break;
      }
    }

    fprintf_s(File, "    ");

    for (_i = 0; _i < _j; _i++)
    {
      fprintf_s(File, "%c", SAFE_OUTCHAR(_pBuf[_iPos + _i]));

      switch (_i)
      {
        case 3:
        case 11:
          fprintf_s(File, " ");
          break;
        case 7:
          fprintf_s(File, "  ");
          break;
      }
    }

    fprintf_s(File, "%s", _sLF);
  }

  fprintf_s(File, "%s", _sLF);

  return S_OK;
}

HRESULT WLDFragment::LoadFrag(WLDFile* ParentFile, BYTE* FragPointer, DWORD FragSize, DWORD FilePos, char* Name, BYTE FragType, DWORD FileIndex, DWORD FragIndex)
{
  WORD  _f;
  DWORD _p;
  WORD  _iLen;
  WORD  _iFiles;

  if ((!FragPointer) || (!FragSize))
  {
    return E_INVALIDARG;
  }

  this->Name = Name;
  if (Name)
  {
    this->NameLen = (BYTE)strlen(Name);
  }
  else
  {
    this->NameLen = 0;
  }
  this->FragType = FragType;
  this->FragPointer = FragPointer;
  this->FragSize = FragSize;
  this->FileIndex = FileIndex;
  this->FragIndex = FragIndex;
  this->FilePos = FilePos;
  this->ParentFile = ParentFile;

  switch (this->FragType)
  {
    case 0x03:
      _p = 4;
      _iFiles = *(WORD*)(&this->FragPointer[0]) + 1;
      for (_f = 0; _f < _iFiles; _f++)
      {
        _iLen = *(WORD*)(&this->FragPointer[_p]);
        _p += 2;

        WLDFile::Decrypt((char*)&this->FragPointer[_p], _iLen, 0, NULL);

        _p += _iLen;
      }
      break;
    default:
      break;
  }

  return S_OK;
}

HRESULT WLDFragment::Export(FILE* File, bool BinaryMode, bool SkipHeader)
{
  DWORD _iFiles;
  DWORD _iLen;
  char* _sLF;
  DWORD _f;
  DWORD _i;
  DWORD _j;
  DWORD _iFlags;
  DWORD _iRef = 0;
  DWORD _iRef1 = 0;
  DWORD _iRef2 = 0;
  DWORD _iRef3 = 0;
  DWORD _iRef4 = 0;
  char  _sFlags[33];

  DWORD _iSize;
  DWORD _iSize1;
  DWORD _iSize2;

  WORD _iNumVertices;
  WORD _iNumTexCoords;
  WORD _iNumNormals;
  WORD _iNumColors;
  WORD _iNumPolygons;
  WORD _iNumVertexPieces;
  WORD _iNumPolygonTexes;
  WORD _iNumVertexTexes;
  WORD _iSize9;
  WORD _iScale;

  WORD _iVertexIndex1;
  WORD _iVertexIndex2;
  float _fOffset;
  WORD _iData9Unknown;
  WORD _iData9Type;

  char _iC1 = 0;
  char _iC2 = 0;
  char _iC3 = 0;
  WORD _iW1 = 0;
  WORD _iW2 = 0;
  WORD _iW3 = 0;
  sint16 _iS1 = 0;
  sint16 _iS2 = 0;
  sint16 _iS3 = 0;
  sint32 _iL1 = 0;
  sint32 _iL2 = 0;
  sint32 _iL3 = 0;
  DWORD _iD1 = 0;
  DWORD _iD2 = 0;
  DWORD _iD3 = 0;
  DWORD _iD4 = 0;
  float _fF1 = 0.0f;
  float _fF2 = 0.0f;
  float _fF3 = 0.0f;

  char* _sS1 = NULL;

  _sLF = (BinaryMode) ? "\r\n" : "\n";
  this->FragPos = 0;

  if (SkipHeader)
  {
    fprintf_s(File, "-------- Reference Fragment %d (Type 0x%02x - %s, \"%s\", 0x%06x) --------%s", this->FileIndex + 1, this->FragType, WLD_FRAGTYPENAME[this->FragType], this->Name, this->FilePos + this->ParentFile->GetDataOffset(), _sLF);
  }

  switch (this->FragType)
  {
    case 0x03:
      if (!SkipHeader)
      {
        return S_OK;
        
        Hexport(File, BinaryMode, true);
      }

      _iFiles = _ReadDWord() + 1;

      //fprintf_s(File, "Texture Files: %d%s", _iFiles, _sLF);

      for (_f = 0; _f < _iFiles; _f++)
      {
        _iLen = _ReadWord();

        fprintf_s(File, "File %d = %s%s", _f + 1, &this->FragPointer[this->FragPos], _sLF);

        this->FragPos += _iLen;
      }

      //fprintf_s(File, _sLF);
      
      return S_OK;
      break;
    case 0x04:
      if (!SkipHeader)
      {
        return S_OK;
        
        Hexport(File, BinaryMode, true);
      }

      //Hexport(File, BinaryMode, false);

      _iFlags = _ReadDWord();
      this->SetFlags(_iFlags, _sFlags);
      
      fprintf_s(File, "Flags = %s%s%s%s", _sFlags, (_iFlags & 0x04) ? " (Animated)" : "", (_iFlags & 0x08) ? " (Texture References)" : "", _sLF);

      _iSize = _ReadDWord();

      if (_iFlags & 0x04)
      {
        _i = _ReadDWord();

        fprintf_s(File, "Unknown01 = 0x%02x (%d)%s", _i, _i, _sLF);
      }

      if (_iFlags & 0x08)
      {
        _i = _ReadDWord();

        fprintf_s(File, "Unknown02 = 0x%02x (%d)%s", _i, _i, _sLF);
      }

      for (_i = 0; _i < _iSize; _i++)
      {
        this->ParentFile->ExportFragment(this->ParentFile->FindFragmentRef(_ReadLong()));
        //fprintf_s(File, "Reference to Fragment03 at Fragment Index %d%s%s", _iRef, _sLF, _sLF);
      }

      if (_i == 0)
      {
        fprintf_s(File, _sLF);
      }
      break;
    case 0x05:
      if (!SkipHeader)
      {
        return S_OK;
        
        Hexport(File, BinaryMode, true);
      }

      _iRef = this->ParentFile->FindFragmentRef(_ReadLong());
      
      _iFlags = _ReadDWord();
      this->SetFlags(_iFlags, _sFlags);
      
      fprintf_s(File, "Flags = %s%s", _sFlags, _sLF);

      //fprintf_s(File, "Reference to Fragment04 at Fragment Index %d%s%s", _iRef, _sLF, _sLF);

      this->ParentFile->ExportFragment(_iRef);
      break;
    case 0x10:
      if (!SkipHeader)
      {
        return S_OK;
        
        Hexport(File, BinaryMode, false);
      }

      //return S_OK;

      _iFlags = _ReadDWord();
      SetFlags(_iFlags, _sFlags);
      fprintf_s(File, "Flags = %s%s", _sFlags, _sLF);

      _iSize = _ReadDWord();
      fprintf_s(File, _sLF);
      fprintf_s(File, "NumTrackRefs = %d%s", _iSize, _sLF);

      this->ParentFile->ExportFragment(this->ParentFile->FindFragmentRef(_ReadLong()));

      if (_iFlags & 0x01)
      {
        _iD1 = _ReadDWord();
        _iD2 = _ReadDWord();
        _iD3 = _ReadDWord();

        fprintf_s(File, "Unknown1A = 0x%08x (%d)%s", _iD1, _iD1, _sLF);
        fprintf_s(File, "Unknown1B = 0x%08x (%d)%s", _iD2, _iD2, _sLF);
        fprintf_s(File, "Unknown1C = 0x%08x (%d)%s", _iD3, _iD3, _sLF);
      }

      if (_iFlags & 0x02)
      {
        fprintf_s(File, "Unknown2 = %f%s", _ReadFloat(), _sLF);
      }

      fprintf_s(File, "TrackRefs - %d%s", _iSize, _sLF);
 
      for (_i = 0; _i < _iSize; _i++)
      {
        this->FragPos += 16;

        //fprintf_s(File, "TrackRef %d - %s%s", _i + 1, (this->ParentFile->FindFragmentRef(_ReadLong()) & 0x7FFFFFFF), _sLF);
        //_iD1 = _ReadDWord();
        //SetFlags(_iD1, _sFlags);
        //fprintf_s(File, "  Flags = %s%s", _sFlags, _sLF);
        //this->ParentFile->ExportFragment(this->ParentFile->FindFragmentRef(_ReadLong()));
        //fprintf_s(File, "  FragRef - %d%s", this->ParentFile->FindFragmentRef(_ReadLong()), _sLF);
        //this->ParentFile->ExportFragment(this->ParentFile->FindFragmentRef(_ReadLong()));
        //fprintf_s(File, "  FragRef - %d%s", this->ParentFile->FindFragmentRef(_ReadLong()), _sLF);
        _iD1 = _ReadDWord();
        //fprintf_s(File, "  NumTrackTreeNodes = %d%s", _iD1, _sLF);
        this->FragPos += _iD1 * 4;
      }

      if (_iFlags & 0x0200)
      {
        _iSize = _ReadDWord();
        fprintf_s(File, _sLF);
        fprintf_s(File, "MeshReferences: %d%s", _iSize, _sLF);

        for (_i = 0; _i < _iSize; _i++)
        {
          this->ParentFile->ExportFragment(this->ParentFile->FindFragmentRef(_ReadLong()));
        }

        for (_i = 0; _i < _iSize; _i++)
        {
          _iD1 = _ReadDWord();
          fprintf_s(File, "MeshRef Data %d = 0x%08x (%d)%s", _i + 1, _iD1, _iD1, _sLF);
        }
      }
      break;
    case 0x11:
      if (!SkipHeader)
      {
        return S_OK;
        
        Hexport(File, BinaryMode, false);
      }

      _iRef = this->ParentFile->ExportFragment(this->ParentFile->FindFragmentRef(_ReadLong()));

      _iD1 = _ReadDWord();
      fprintf_s(File, "Unknown = 0x%08x (%d)%s%s", _iD1, _iD1, _sLF, _sLF);
      break;
    case 0x13:
      if (!SkipHeader)
      {
        return S_OK;
        
        Hexport(File, BinaryMode, false);
      }

      //return S_OK;

      _iRef = this->ParentFile->ExportFragment(this->ParentFile->FindFragmentRef(_ReadLong()));

      _iFlags = _ReadDWord();
      SetFlags(_iFlags, _sFlags);
      fprintf_s(File, "Flags = %s%s", _sFlags, _sLF);
      if (_iFlags & 0x01)
      {
        _iD1 = _ReadDWord();
        fprintf_s(File, "Unknown = 0x%08x (%d)%s%s", _iD1, _iD1, _sLF, _sLF);
      }
      break;
    case 0x14:
      if (!SkipHeader)
      {
        //return S_OK;
        
        Hexport(File, BinaryMode, false);
      }

      //return S_OK;

      _iFlags = _ReadDWord();
      SetFlags(_iFlags, _sFlags);
      fprintf_s(File, "Flags = %s%s", _sFlags, _sLF);

      _iRef1 = this->ParentFile->FindFragmentRef(_ReadLong() - 1);
      this->ParentFile->ExportFragment(_iRef1);
      _iD1 = _ReadDWord();
      _iD2 = _ReadDWord();
      _iRef2 = this->ParentFile->FindFragmentRef(_ReadLong());
      this->ParentFile->ExportFragment(_iRef2);
      
      _iD3 = (_iFlags & 1) ? ((_iFlags & 2) ? 8 : 1) : ((_iFlags & 2) ? 7 : 0);
      for (_i = 0; _i < _iD3; _i++)
      {
        _iD3 = _ReadDWord();
        fprintf_s(File, "Unknown%d = 0x%08x (%d)%s", _i + 1, _iD3, _iD3, _sLF);
      }

      for (_i = 0; _i < _iD1; _i++)
      {
        _iD3 = _ReadDWord();
        for (_j = 0; _j < _iD3; _j++)
        {
          _iD4 = _ReadDWord();
          _fF1 = _ReadFloat();

          fprintf_s(File, "Entry1 %d:%d = %d, %f%s", _i, _j, _iD4, _fF1, _sLF);
        }
      }

      for (_i = 0; _i < _iD2; _i++)
      {
        _iRef3 = this->ParentFile->FindFragmentRef(_ReadLong());
        //fprintf_s(File, "Fragment Reference = %d%s", _iRef3, _sLF);

        this->ParentFile->ExportFragment(_iRef3);
      }

      _iD3 = _ReadDWord();
      if (_iD3)
      {
        _sS1 = (char*)(this->FilePos + this->FragPos + this->ParentFile->GetDataOffset());

        if (_sS1[_iD3] != 0x00)
        {
          WLDFile::Decrypt(_sS1, _iD3, 0, NULL);
        }
      }
      else
      {
        _sS1 = "";
      }

      fprintf_s(File, "UnknownString = '%s'%s", _sS1, _sLF);

      break;
    case 0x15:
      if (!SkipHeader)
      {
        return S_OK;

        Hexport(File, BinaryMode, true);
      }

      _iFlags = _ReadDWord();
      SetFlags(_iFlags, _sFlags);
      fprintf_s(File, "Flags = %s%s", _sFlags, _sLF);

      this->ParentFile->ExportFragment(this->ParentFile->FindFragmentRef(_ReadLong()));

      _fF1 = _ReadFloat();
      _fF2 = _ReadFloat();
      _fF3 = _ReadFloat();
      fprintf_s(File, "Location = (%f, %f, %f)%s", _fF1, _fF2, _fF3, _sLF);

      _fF1 = _ReadFloat();
      _fF2 = _ReadFloat();
      _fF3 = _ReadFloat();
      fprintf_s(File, "Rotation = (%f, %f, %f)%s", _fF1, _fF2, _fF3, _sLF);

      /*
      _fF1 = _ReadFloat();
      _fF2 = _ReadFloat();
      _fF3 = _ReadFloat();
      fprintf_s(File, "Unknown = (%f, %f, %f)%s", _fF1, _fF2, _fF3, _sLF);
      */

      _fF1 = _ReadFloat();
      _fF2 = _ReadFloat();
      fprintf_s(File, "Scale = %f x %f%s", _fF2, _fF3, _sLF);

      this->ParentFile->ExportFragment(this->ParentFile->FindFragmentRef(_ReadLong()));

      fprintf_s(File, "Unknown2 = %d%s", _ReadDWord(), _sLF);
      fprintf_s(File, _sLF);
      break;
    case 0x17:
      if (!SkipHeader)
      {
        return S_OK;

        Hexport(File, BinaryMode, true);
      }

      _iFlags = _ReadDWord();
      SetFlags(_iFlags, _sFlags);
      fprintf_s(File, "Flags = %s%s", _sFlags, _sLF);
      
      _iSize1 = _ReadDWord();
      _iSize2 = _ReadDWord();
      fprintf_s(File, "Unknown01 = %f%s", _ReadFloat(), _sLF);
      fprintf_s(File, "Unknown02 = %f%s", _ReadFloat(), _sLF);

      fprintf_s(File, "Entry1s (%d):%s", _iSize1, _sLF);
      for (_i = 0; _i < _iSize1; _i++)
      {
        _fF1 = _ReadFloat();
        _fF2 = _ReadFloat();
        _fF3 = _ReadFloat();

        fprintf_s(File, "  Entry1 # %d = (%f, %f, %f)%s", _i + 1, _fF1, _fF2, _fF3, _sLF);
      }

      fprintf_s(File, "Entry2s (%d):%s", _iSize2, _sLF);
      for (_i = 0; _i < _iSize2; _i++)
      {
        _iD1 = _ReadDWord();
        fprintf_s(File, "  Entry2 Indexes (%d):%s", _iD1, _sLF);

        for (_j = 0; _j < _iD1; _j++)
        {
          fprintf_s(File, "    Entry2 %d:%d = %d%s", _i, _j, _ReadDWord(), _sLF);
        }
      }
      break;
    case 0x18:
      if (!SkipHeader)
      {
        return S_OK;

        Hexport(File, BinaryMode, true);
      }

      _iRef = this->ParentFile->FindFragmentRef(_ReadLong());
      
      _iFlags = _ReadDWord();
      SetFlags(_iFlags, _sFlags);
      fprintf_s(File, "Flags = %s%s", _sFlags, _sLF);
      if (_iFlags & 2)
      {
        fprintf_s(File, "Unknown = %f%s", _ReadFloat(), _sLF);
      }

      this->ParentFile->ExportFragment(_iRef);
      break;
    case 0x2C:
      if (!SkipHeader)
      {
        return S_OK;

        Hexport(File, BinaryMode, true);
      }

      break;
    case 0x2D:
      if (!SkipHeader)
      {
        return S_OK;

        Hexport(File, BinaryMode, true);
      }

      _iRef = this->ParentFile->FindFragmentRef(_ReadLong());

      fprintf_s(File, "Unknown1 = %d%s", _ReadDWord(), _sLF);

      this->ParentFile->ExportFragment(_iRef);
      break;
    case 0x30:
      if (!SkipHeader)
      {
        //return S_OK;
        
        Hexport(File, BinaryMode, false);
      }

      _iFlags = _ReadDWord();
      this->SetFlags(_iFlags, _sFlags);

      fprintf_s(File, "Flags = %s%s", _sFlags, _sLF);

      _iFlags = _ReadDWord();
      this->SetFlags(_iFlags, _sFlags);
      
      switch (_iFlags)
      {
        case 0x80000001:
          break;
        case 0x80000014:
          break;
        default:
          fprintf_s(File, "Special ");
          break;
      };
      
      fprintf_s(File, "AlphaFlags = %s%s%s%s%s%s%s%s", _sFlags,
        (_iFlags & 0x01) ? " (No Alpha)" : "",
        (_iFlags & 0x02) ? " (Alpha Mask)" : "",
        (_iFlags & 0x04) ? " (Alpha Blend)" : "",
        (_iFlags & 0x08) ? " (Alpha Mask+Blend)" : "",
        (_iFlags & 0x10) ? " (Alpha Mask-NoBlend)" : "",
        (_iFlags & 0x80000000) ? " (Visible)" : "", _sLF);

      fprintf_s(File, "UnknownRGB = 0x%08x%s", _ReadDWord(), _sLF);

      fprintf_s(File, "UnknownF1 = %f%s", _ReadFloat(), _sLF);
      fprintf_s(File, "UnknownF2 = %f%s", _ReadFloat(), _sLF);

      _iRef = this->ParentFile->FindFragmentRef(_ReadLong());

      fprintf_s(File, "UnknownDP = %d %f%s", _ReadDWord(), _ReadFloat(), _sLF);

      this->ParentFile->ExportFragment(_iRef);

      fprintf_s(File, _sLF);

      break;
    case 0x31:
      if (!SkipHeader)
      {
        return S_OK;
        
        Hexport(File, BinaryMode, true);
      }

      Hexport(File, BinaryMode, false);

      _iFlags = _ReadDWord();
      SetFlags(_iFlags, _sFlags);

      fprintf_s(File, "Flags = %s%s", _sFlags, _sLF);

      _iSize = _ReadDWord();
      fprintf_s(File, "Refs = %d%s", _iSize, _sLF);
      for (_i = 0; _i < _iSize; _i++)
      {
        this->ParentFile->ExportFragment(this->ParentFile->FindFragmentRef(_ReadLong()));
      }

      fprintf_s(File, _sLF);
      break;
    case 0x36:
      if (!SkipHeader)
      {
        //return S_OK;

        Hexport(File, BinaryMode, true);
      }

      _iFlags = _ReadDWord();
      SetFlags(_iFlags, _sFlags);

      fprintf_s(File, "Flags = %s%s", _sFlags, _sLF);
      
      _iRef1 = this->ParentFile->FindFragmentRef(_ReadLong());
      _iRef2 = this->ParentFile->FindFragmentRef(_ReadLong());
      _iRef3 = this->ParentFile->FindFragmentRef(_ReadLong());
      _iRef4 = this->ParentFile->FindFragmentRef(_ReadLong());

      fprintf_s(File, "Mesh Center: ( %f", _ReadFloat());
      fprintf_s(File, " , %f , ", _ReadFloat());
      fprintf_s(File, "%f )%s", _ReadFloat(), _sLF);
      
      fprintf_s(File, "Unknown01 = %d%s", _ReadDWord(), _sLF);
      fprintf_s(File, "Unknown02 = %d%s", _ReadDWord(), _sLF);
      fprintf_s(File, "Unknown03 = %d%s", _ReadDWord(), _sLF);

      fprintf_s(File, "BubbleSize = %f%s", _ReadFloat(), _sLF);
      
      fprintf_s(File, "CoordsMin = ( %f", _ReadFloat());
      fprintf_s(File, " , %f , ", _ReadFloat());
      fprintf_s(File, "%f )%s", _ReadFloat(), _sLF);

      fprintf_s(File, "CoordsMax = ( %f", _ReadFloat());
      fprintf_s(File, " , %f , ", _ReadFloat());
      fprintf_s(File, "%f )%s", _ReadFloat(), _sLF);

      _iNumVertices = _ReadWord();
      _iNumTexCoords = _ReadWord();
      _iNumNormals = _ReadWord();
      _iNumColors = _ReadWord();
      _iNumPolygons = _ReadWord();
      _iNumVertexPieces = _ReadWord();
      _iNumPolygonTexes = _ReadWord();
      _iNumVertexTexes = _ReadWord();
      _iSize9 = _ReadWord();
      _iScale = _ReadWord();
      
      fprintf_s(File, "NumVertices = %d%s", _iNumVertices, _sLF);
      fprintf_s(File, "NumTexCoords = %d%s", _iNumTexCoords, _sLF);
      fprintf_s(File, "NumNormals = %d%s", _iNumNormals, _sLF);
      fprintf_s(File, "NumColors = %d%s", _iNumColors, _sLF);
      fprintf_s(File, "NumPolygons = %d%s", _iNumPolygons, _sLF);
      fprintf_s(File, "NumVertexPieces = %d%s", _iNumVertexPieces, _sLF);
      fprintf_s(File, "NumPolygonTexes = %d%s", _iNumPolygonTexes, _sLF);
      fprintf_s(File, "NumVertexTexes = %d%s", _iNumVertexTexes, _sLF);
      fprintf_s(File, "Size9 = %d%s", _iSize9, _sLF);
      fprintf_s(File, "Scale = %d%s", _iScale, _sLF);

      fprintf_s(File, "Ref_TextureList = %d%s", _iRef1, _sLF);
      fprintf_s(File, "Ref_AnimatedVertices = %d%s", _iRef2, _sLF);
      fprintf_s(File, "Ref_Unknown = %d%s", _iRef3, _sLF);
      fprintf_s(File, "Ref_TextureBitmapName = %d%s", _iRef4, _sLF);

      fprintf_s(File, "-- %d Vertices (0x%06x) --%s", _iNumVertices, this->FilePos + this->FragPos + this->ParentFile->GetDataOffset(), _sLF);
      this->FragPos += _iNumVertices * 6;
      /*
      for (_i = 0; _i < _iNumVertices; _i++)
      {
        _iS1 = _ReadShort();
        _iS2 = _ReadShort();
        _iS3 = _ReadShort();

        _fF1 = ((float)_iS1) / (float)(1 << _iScale);
        _fF2 = ((float)_iS2) / (float)(1 << _iScale);
        _fF3 = ((float)_iS3) / (float)(1 << _iScale);

        fprintf_s(File, "Vertex %d = (%f, %f, %f)%s", _i + 1, _fF1, _fF2, _fF3, _sLF);
      }
      */
      fprintf_s(File, "-- %d TexCoords (0x%06x) --%s", _iNumTexCoords, this->FilePos + this->FragPos + this->ParentFile->GetDataOffset(), _sLF);
      switch (this->ParentFile->GetVersion())
      {
        case WLDVer1:
          this->FragPos += _iNumTexCoords * 4;
          /*
          for (_i = 0; _i < _iNumTexCoords; _i++)
          {
            _iS1 = _ReadShort();
            _iS2 = _ReadShort();

            fprintf_s(File, "TexCoord %d = (%d, %d)%s", _i + 1, _iS1, _iS2, _sLF);
          }
          fprintf_s(File, _sLF);
          */
          break;
        case WLDVer2:
          this->FragPos += _iNumTexCoords * 8;
          /*
          for (_i = 0; _i < _iNumTexCoords; _i++)
          {
            _fF1 = _ReadFloat();
            _fF2 = _ReadFloat();

            fprintf_s(File, "TexCoord %d = (%f, %f)%s", _i + 1, _fF1, _fF2, _sLF);
          }
          fprintf_s(File, _sLF);
          */
          break;
        default:
          BreakPoint;
          break;
      }

      fprintf_s(File, "-- %d Normals (0x%06x) --%s", _iNumNormals, this->FilePos + this->FragPos + this->ParentFile->GetDataOffset(), _sLF);
      this->FragPos += _iNumNormals * 3;
      /*
      for (_i = 0; _i < _iNumNormals; _i++)
      {
        _iC1 = _ReadChar();
        _iC2 = _ReadChar();
        _iC3 = _ReadChar();

        _fF1 = ((float)_iC1) / 127.0f;
        _fF2 = ((float)_iC2) / 127.0f;
        _fF3 = ((float)_iC3) / 127.0f;

        fprintf_s(File, "Normal %d = (%f, %f, %f)%s", _i + 1, _fF1, _fF2, _fF3, _sLF);
      }
      */
      fprintf_s(File, "-- %d Colors (0x%06x) --%s", _iNumColors, this->FilePos + this->FragPos + this->ParentFile->GetDataOffset(), _sLF);
      this->FragPos += _iNumColors * 4;
      /*
      for (_i = 0; _i < _iNumColors; _i++)
      {
        _iD1 = _ReadDWord();

        fprintf_s(File, "Color %d = 0x%06x%s", _i + 1, _iD1, _sLF);
      }
      */
      fprintf_s(File, "-- %d Polygons (0x%06x) --%s", _iNumPolygons, this->FilePos + this->FragPos + this->ParentFile->GetDataOffset(), _sLF);
      this->FragPos += _iNumPolygons * 8;
      /*
      for (_i = 0; _i < _iNumPolygons; _i++)
      {
        _iFlags = _ReadWord();
        SetFlags(_iFlags, _sFlags);
        _iW1 = _ReadWord();
        _iW2 = _ReadWord();
        _iW3 = _ReadWord();

        fprintf_s(File, "Polygon %d (Flags %s) = %d -> %d -> %d%s", _i + 1, _sFlags, _iW1, _iW2, _iW3, _sLF);
      }
      */
      fprintf_s(File, "-- %d VertexPieces (0x%06x) --%s", _iNumVertexPieces, this->FilePos + this->FragPos + this->ParentFile->GetDataOffset(), _sLF);
      this->FragPos += _iNumVertexPieces * 4;
      /*
      for (_i = 0; _i < _iNumVertexPieces; _i++)
      {
        _iW1 = _ReadWord();
        _iW2 = _ReadWord();
        
        fprintf_s(File, "VertexPiece %d - Skeleton Track %d has %d vertices%s", _i + 1, _iW2, _iW1, _sLF);
      }
      */
      fprintf_s(File, "-- %d PolygonTexes (0x%06x) --%s", _iNumPolygonTexes, this->FilePos + this->FragPos + this->ParentFile->GetDataOffset(), _sLF);
      this->FragPos += _iNumPolygonTexes * 4;
      /*
      for (_i = 0; _i < _iNumPolygonTexes; _i++)
      {
        _iW1 = _ReadWord();
        _iW2 = _ReadWord();

        fprintf_s(File, "Texture %d is used for %d Polygons%s", _iW2, _iW1, _sLF);
      }
      */
      fprintf_s(File, "-- %d VertexTexes (0x%06x) --%s", _iNumVertexTexes, this->FilePos + this->FragPos + this->ParentFile->GetDataOffset(), _sLF);
      this->FragPos += _iNumVertexTexes * 4;
      /*
      for (_i = 0; _i < _iNumVertexTexes; _i++)
      {
        _iW1 = _ReadWord();
        _iW2 = _ReadWord();

        fprintf_s(File, "Texture %d is used for %d Vertices%s", _iW2, _iW1, _sLF);
      }
      */
      fprintf_s(File, "====== %d Data9 Entries (0x%06x) ======%s", _iSize9, this->FilePos + this->FragPos + this->ParentFile->GetDataOffset(), _sLF);
      _iSize9 = 0;
      for (_i = 0; _i < _iSize9; _i++)
      {
        _iVertexIndex1 = _ReadWord();
        _iVertexIndex2 = _ReadWord();
         this->FragPos -= 4;
        _fOffset = _ReadFloat();
        _iData9Unknown = _ReadByte();
        _iData9Type = _ReadByte();

        fprintf_s(File, "Data9 # %d", _i + 1);
        fprintf_s(File, " (Type %d)", _iData9Type);
        fprintf_s(File, ", Unknown = %d", _iData9Unknown);
        if (_iData9Type == 4)
        {
          fprintf_s(File, ", Offset = %f", _fOffset);
        }
        else
        {
          fprintf_s(File, ", VertexIndex1 = %d, VertexIndex2 = %d", _iVertexIndex1, _iVertexIndex2);
        }
        fprintf_s(File, _sLF);
      }
      
      this->ParentFile->ExportFragment(_iRef1);
      this->ParentFile->ExportFragment(_iRef2);
      this->ParentFile->ExportFragment(_iRef3);
      this->ParentFile->ExportFragment(_iRef4);
      break;
    default:
      if (!SkipHeader)
      {
        return S_OK;

        Hexport(File, BinaryMode, true);
      }

      return Hexport(File, BinaryMode, false);
      break;
  }

  return S_OK;
}

void WLDFragment::SetFlags(DWORD Flags, char* Buffer)
{
  BYTE _i;
  BYTE _j;

  if (Buffer)
  {
    if (!(Flags & 0xFFFFFF00))
    {
      _i = 24;
    }
    else if (!(Flags & 0xFFFF0000))
    {
      _i = 16;
    }
    else if (!(Flags & 0xFF000000))
    {
      _i = 8;
    }
    else
    {
      _i = 0;
    }

    for (_j = 0; _i < 32; _i++)
    {
      Buffer[_j++] = ((Flags >> (31 - _i)) & 1) ? '1' : '0';
    }

    Buffer[_j++] = NULL;
  }
}

char WLDFragment::_ReadChar()
{
  if ((this->FragPos + sizeof(char)) > this->FragSize)
  {
    BreakPoint;

    return 0;
  }

  this->FragPos += sizeof(char);

  return *((char*)&this->FragPointer[this->FragPos - sizeof(char)]);
}

BYTE WLDFragment::_ReadByte()
{
  if ((this->FragPos + sizeof(BYTE)) > this->FragSize)
  {
    BreakPoint;

    return 0;
  }

  this->FragPos += sizeof(BYTE);

  return *((BYTE*)&this->FragPointer[this->FragPos - sizeof(BYTE)]);
}

sint16 WLDFragment::_ReadShort()
{
  if ((this->FragPos + sizeof(sint16)) > this->FragSize)
  {
    BreakPoint;

    return 0;
  }

  this->FragPos += sizeof(sint16);

  return *((sint16*)&this->FragPointer[this->FragPos - sizeof(sint16)]);
}

sint32 WLDFragment::_ReadLong()
{
  if ((this->FragPos + sizeof(sint32)) > this->FragSize)
  {
    BreakPoint;

    return 0;
  }

  this->FragPos += sizeof(sint32);

  return *((sint32*)&this->FragPointer[this->FragPos - sizeof(sint32)]);
}

DWORD WLDFragment::_ReadDWord()
{
  if ((this->FragPos + sizeof(DWORD)) > this->FragSize)
  {
    BreakPoint;

    return 0;
  }

  this->FragPos += sizeof(DWORD);

  return *((DWORD*)&this->FragPointer[this->FragPos - sizeof(DWORD)]);
}

WORD WLDFragment::_ReadWord()
{
  if ((this->FragPos + sizeof(WORD)) > this->FragSize)
  {
    BreakPoint;

    return 0;
  }

  this->FragPos += sizeof(WORD);

  return *((WORD*)&this->FragPointer[this->FragPos - sizeof(WORD)]);
}

float WLDFragment::_ReadFloat()
{
  if ((this->FragPos + sizeof(float)) > this->FragSize)
  {
    BreakPoint;

    return 0.0f;
  }

  this->FragPos += sizeof(float);

  return *((float*)&this->FragPointer[this->FragPos - sizeof(float)]);
}

WLDFragment* WLDFragment::Frag14_GetMeshFrag()
{
  DWORD _iFlags;
  DWORD _iSize1;
  DWORD _iSize2;
  DWORD _iSize3;
  DWORD _i;

  if (this->FragType != 0x14)
  {
    return NULL;
  }

  this->FragPos = 0;

  _iFlags = _ReadDWord();
  this->FragPos += 4;
  _iSize1 = _ReadDWord();
  _iSize2 = _ReadDWord();
  this->FragPos += 4;  
  
  if (_iFlags & 1)
  {
    this->FragPos += 4;
  }

  if (_iFlags & 2)
  {
    this->FragPos += 4 * 7;
  }

  for (_i = 0; _i < _iSize1; _i++)
  {
    _iSize3 = _ReadDWord();
    
    this->FragPos += 8 * _iSize3;
  }

  return this->ParentFile->GetFragment(this->ParentFile->FindFragmentRef(_ReadLong()));
}

BYTE WLDFragment::Frag10_GetMeshFrags(BYTE* NumMeshes, WLDFragment** MeshArray, BYTE MaxMeshes)
{
  if ((!MeshArray) || (!MaxMeshes) || (this->FragType != 0x10))
  {
    return 0;
  }

  DWORD _iFlags;
  DWORD _iSize1;
  DWORD _iSize2;
  DWORD _iSize3;
  DWORD _i;
  WLDFragment* _oMeshFrag;
  DWORD _iMeshes;

  this->FragPos = 0;
  _iFlags = _ReadDWord();

  if (!(_iFlags & 0x200))
  {
    // No mesh references in this Frag10
    
    *NumMeshes = 0;
      
    return 0;
  }

  _iSize1 = _ReadDWord();
  this->FragPos += 4;

  if (_iFlags & 1)
  {
    this->FragPos += 12;
  }

  if (_iFlags & 2)
  {
    this->FragPos += 4;
  }

  for (_i = 0; _i < _iSize1; _i++)
  {
    this->FragPos += 16;

    _iSize3 = _ReadDWord();
    this->FragPos += 4 * _iSize3;
  }

  _iSize2 = _ReadDWord();

  if (_iSize2 > MaxMeshes)
  {
    _iSize2 = MaxMeshes;
  }

  _iMeshes = 0;
  for (_i = 0; _i < _iSize2; _i++)
  {
    _oMeshFrag = this->ParentFile->GetFragment(_ReadDWord());

    if (_oMeshFrag)
    {
      switch (_oMeshFrag->FragType)
      {
        case 0x2D:
          // FragRef, not an actual MeshFrag
          _oMeshFrag = this->ParentFile->GetFragment(*(DWORD*)_oMeshFrag->FragPointer);
          if (_oMeshFrag)
          {
            switch (_oMeshFrag->FragType)
            {
              case 0x2C:
              case 0x36:
                // Valid mesh fragment
                break;
              default:
                _oMeshFrag = NULL;
            }
          }
          break;
        case 0x2C:
        case 0x36:
          // Direct Mesh Fragment
          break;
        default:
          // Not a valid mesh fragment
          _oMeshFrag = NULL;
          break;
      }
    }

    if (_oMeshFrag)
    {
      MeshArray[_iMeshes++] = _oMeshFrag;
    }
  }

  if (NumMeshes)
  {
    *NumMeshes = (BYTE)_iMeshes;
  }

  return (BYTE)_iMeshes;
}

BYTE WLDFragment::Frag03_GetFileCount()
{
  if (this->FragType != 0x03)
  {
    return 0;
  }

  this->FragPos = 0;

  return (BYTE)_ReadDWord() + 1;
}

BYTE WLDFragment::Frag04_GetTextureCount()
{
  if (this->FragType != 0x04)
  {
    return 0;
  }

  this->FragPos = 0;
  _ReadDWord();

  return (BYTE)_ReadDWord();
}

WLDFragment* WLDFragment::Frag04_GetTexture(BYTE Index)
{
  if (this->FragType != 0x04)
  {
    return NULL;
  }

  this->FragPos = 0;
  DWORD _iFlags = _ReadDWord();
  DWORD _iFiles = _ReadDWord();
  DWORD _i;

  if (Index >= _iFiles)
  {
    return NULL;
  }

  if (_iFlags & 0x04)
  {
    _ReadDWord();
  }

  if (_iFlags & 0x08)
  {
    _ReadDWord();
  }

  for (_i = 0; _i < Index; _i++)
  {
    _ReadLong();
  }

  return this->ParentFile->GetFragment(this->ParentFile->FindFragmentRef(_ReadLong()));
}

BYTE WLDFragment::Frag30_GetFileCount()
{
  if (this->FragType != 0x30)
  {
    return 0;
  }
  
  this->FragPos = 20;

  WLDFragment* _oFrag;
  DWORD _iFiles = 0;

  if (!(_oFrag = this->ParentFile->GetFragment(this->ParentFile->FindFragmentRef(_ReadLong()))))
  {
    return 0;
  }

  if (!(_oFrag = this->ParentFile->GetFragment(this->ParentFile->FindFragmentRef(*(DWORD*)_oFrag->FragPointer))))
  {
    return 0;
  }
  
  DWORD _iFrags;
  DWORD _i;
  WLDFragment* _oFrag2;

  _iFrags = _oFrag->Frag04_GetTextureCount();
  for (_i = 0; _i < _iFrags; _i++)
  {
    if (_oFrag2 = _oFrag->Frag04_GetTexture((BYTE)_i))
    {
      _iFiles += _oFrag2->Frag03_GetFileCount();
    }
  }

  return (BYTE)_iFiles;
}