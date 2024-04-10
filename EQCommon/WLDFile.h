// WLDFile.h -- Limited handling of EQ WLD files

#pragma once

#include <windows.h>
#include <memory.h>
#include "Utils.h"

#define WLD_MAGICNUMBER     0x54503D02

#define WLD_MAXFILENAMELEN  63
#define WLD_MAXFRAGS_EACH   100

#define WLD_MAXFRAGTYPE     0x37

#define WLD_MAXMODELCODELEN   10
#define WLD_MAXVARIATIONS     30
  
enum WLDVersions
{
  WLDVer1 = 0x00015500,
  WLDVer2 = 0x1000C800
};

struct WLDFileHeader
{
  DWORD MagicNumber;
  DWORD Version;
  DWORD FragmentCount;
  DWORD Frag_0x22_Count;
  DWORD Unknown016;
  DWORD StringHashSize;
  DWORD Frag_Unknown_Count;
};

const BYTE WLD_XOR[] = {0x95, 0x3A, 0xC5, 0x2A, 0x95, 0x7A, 0x95, 0x6A};

typedef char* WLDTmpString;

const WLDTmpString WLD_FRAGTYPENAME[] = {
  /* 00 */ "",
  /* 01 */ "",
  /* 02 */ "",
  /* 03 */ "Texture Bitmap Filename",
  /* 04 */ "Texture Bitmap Info",
  /* 05 */ "Texture Bitmap Info Reference",
  /* 06 */ "Two-Dimensional Object",
  /* 07 */ "Two-Dimensional Object Reference",
  /* 08 */ "Camera",
  /* 09 */ "Camera Reference",
  /* 0A */ "",
  /* 0B */ "",
  /* 0C */ "",
  /* 0D */ "",
  /* 0E */ "",
  /* 0F */ "",
  /* 10 */ "Skeleton Track Set",
  /* 11 */ "Skeleton Track Set Reference",
  /* 12 */ "Mob Skeleton Track Piece",
  /* 13 */ "Mob Skeleton Track Piece Reference",
  /* 14 */ "Static or Animated Model Reference/Player Info",
  /* 15 */ "Object Location",
  /* 16 */ "Zone - Unknown",
  /* 17 */ "Polygon Animation?",
  /* 18 */ "Polygon Animation Reference?",
  /* 19 */ "",
  /* 1A */ "",
  /* 1B */ "Light Source",
  /* 1C */ "Light Source Reference",
  /* 1D */ "",
  /* 1E */ "",
  /* 1F */ "",
  /* 20 */ "",
  /* 21 */ "BSP Tree",
  /* 22 */ "BSP Tree Region",
  /* 23 */ "",
  /* 24 */ "",
  /* 25 */ "",
  /* 26 */ "",
  /* 27 */ "",
  /* 28 */ "Light Info",
  /* 29 */ "Region Flag",
  /* 2A */ "Ambient Light",
  /* 2B */ "",
  /* 2C */ "Alternate Mesh",
  /* 2D */ "Mesh Reference",
  /* 2E */ "",
  /* 2F */ "Mesh Animated Vertices Reference",
  /* 30 */ "Texture",
  /* 31 */ "Texture List",
  /* 32 */ "Vertex Color",
  /* 33 */ "Vertex Color Reference",
  /* 34 */ "",
  /* 35 */ "First Fragment",
  /* 36 */ "Mesh",
  /* 37 */ "Mesh Animated Vertices" };

class WLDFile;

struct WLDFragment
{
  HRESULT Export(FILE* File, bool BinaryMode, bool SkipHeader);
  HRESULT LoadFrag(WLDFile* ParentFile, BYTE* FragPointer, DWORD FragSize, DWORD FilePos, char* Name, BYTE FragType, DWORD FileIndex, DWORD FragIndex);
  HRESULT Hexport(FILE* File, bool BinaryMode, bool HeaderOnly);
  void    SetFlags(DWORD Flags, char* Buffer);

  BYTE    Frag03_GetFileCount();
  BYTE    Frag04_GetTextureCount();
  WLDFragment* Frag04_GetTexture(BYTE Index);
  BYTE    Frag10_GetMeshFrags(BYTE* NumMeshes, WLDFragment** MeshArray, BYTE MaxMeshes);
  WLDFragment* Frag14_GetMeshFrag();
  BYTE    Frag30_GetFileCount();
  
  char* Name;
  BYTE  NameLen;
  BYTE  FragType;
  BYTE* FragPointer;
  DWORD FragSize;
  DWORD FileIndex;
  DWORD FilePos;
  DWORD FragPos;
  DWORD FragIndex;
  WLDFile* ParentFile;

private:
  DWORD  _ReadDWord();
  WORD   _ReadWord();
  float  _ReadFloat();
  char   _ReadChar();
  BYTE   _ReadByte();
  sint16 _ReadShort();
  sint32 _ReadLong();
};

class WLDFile
{
public:
  WLDFile(void);
  ~WLDFile(void);

  HRESULT LoadFromMemory(BYTE* Buffer, DWORD FileSize, bool StableBuffer, char* Filename, bool StringsOnly);
  DWORD   GetStringCount();
  char*   GetString(DWORD StringNumber);
  HRESULT CopyString(DWORD StringNumber, char* Dest, DWORD DestSize);
  char*   GetFilename();
  DWORD   GetDataOffset();

  WLDFragment* GetFragment(DWORD Index);
  DWORD   FindFragmentRef(sint32 Reference);
  WLDFragment* FindNextFragment(BYTE Type, WLDFragment* Current);
  
  HRESULT ExportToTextFile();
  HRESULT ExportFragment(DWORD Index);

  DWORD   GetVersion();
  
  static HRESULT Decrypt(char* Buffer, DWORD Size, BYTE StartIndex, DWORD* Out_StringCount);

private:
  void        _Close();
  DWORD       _ReadDWord();
  DWORD       _ReadDWord(void* Buffer, DWORD* Pointer);
  WORD        _ReadWord();
  WORD        _ReadWord(void* Buffer, DWORD* Pointer);
  float       _ReadFloat();
  float       _ReadFloat(void* Buffer, DWORD* Pointer);

  size_t      iFilenameLen;
  char        sFilename[_MAX_PATH];
  FILE*       oExportFile;
  
  DWORD       iVersion;
  DWORD       iFragments;
  DWORD       iStringHashSize;
  char*       aStringHash;
  DWORD       iStrings;
  DWORD*      aStringLengths;
  DWORD*      aStringOffsets;

  DWORD       iFileSize;
  DWORD       iFilePos;
  BYTE*       aFileBytes;
  bool        bFileCopied;
  
  DWORD       iFragCountTotal;
  BYTE*       iFileFragType;
  DWORD*      iFileFragNext;
  DWORD       iFileFragFirst[WLD_MAXFRAGTYPE + 1];
  DWORD       iFileIndex;
  DWORD       iFragIndex[WLD_MAXFRAGTYPE + 1];
  DWORD       iFragCount[WLD_MAXFRAGTYPE + 1];

  WLDFragment* oFragment;
};
