// EQPack.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "EQPack.h"

void ModelInventory();

int _tmain(int argc, _TCHAR* argv[])
{
	if (argc < 2)
  {
    DisplayUsage();

    return 0;
  }

  char   _sPackage[_MAX_PATH];
  size_t _iArgLen;
  WORD   _i;

  _iArgLen = wcslen(argv[1]);
  if (_iArgLen > (_MAX_PATH - 1))
  {
    DisplayUsage();

    return 1;
  }

  for (_i = 0; _i <= _iArgLen; _i++)
  {
    if (argv[1][_i] > 0xFF)
    {
      printf("Sorry, Unicode filenames are not currently supported.\n");

      return 2;
    }

    _sPackage[_i] = (argv[1][_i] & 0xFF);
  }

  if (strcmp(_sPackage, "inventory") == 0)
  {
    ModelInventory();

    return 0;
  }

  if (!FileExists(_sPackage))
  {
    printf("File not found: %s\n", _sPackage);

    return 3;
  }

  if (RightMatch(_sPackage, ".s3d") || RightMatch(_sPackage, ".eqg") || RightMatch(_sPackage, ".pak"))
  {
    if (FAILED(UnpackS3D(_sPackage)))
    {
      return 4;
    }
  }
  else if (RightMatch(_sPackage, ".wld"))
  {
    if (FAILED(UnpackWLD(_sPackage)))
    {
      return 4;
    }
  }

  return 0;
}

void DisplayUsage()
{
  printf(/*"Packs/"*/ "Unpacks S3D and EQG files\n"
         "\n"
         "USAGE: EQPack File\n"
         /*
         "\n"
         "If the specified file is found, its contents are unpacked into a directory named for the file.\n"
         "\n"
         "If the specified file is not found, the contents of the current directory are packed into it in\n"
         "  the parent directory.\n"
         "\n"
         "Examples:\n"
         "\n"
         "Unpacking: EQPack global_chr.s3d\n"
         "  - Creates a directory called global_chr, and unpacks the contents of global_chr.s3d into it\n"
         "\n"
         "Packing:   EQPack nektropolis.s3d\n"
         "  - Packs the contents of the current directory into nektropolis.s3d in the parent directory\n"
         */
         "\n");
}

HRESULT UnpackS3D(char* Filename)
{
  char*  _sExt;
  char   _sFolder[_MAX_PATH];
  WORD   _i;

  memcpy_s(_sFolder, sizeof(_sFolder), Filename, strlen(Filename) + 1);

  if (_sExt = strrchr(_sFolder, '.'))
  {
    _sExt[0] = NULL;
  }

  if (_mkdir(_sFolder))
  {
    if (errno != EEXIST)
    {
      printf("Problem creating folder %s\n", _sFolder);

      return E_FAIL;
    }
  }

  printf("Unpacking %s...\n", Filename);
  
  S3DPackage* _oS3D = new S3DPackage();
  FILE* _oFile;
  BYTE* _aBytes;
  char* _sFile;

  WORD _iFilesUnpacked = 0;

  _oS3D->Open(Filename);

  _chdir(_sFolder);
  
  for (_i = 0; _i < _oS3D->GetFileCount(); _i++)
  {
    _sFile = _oS3D->GetFilename(_i);
    if (fopen_s(&_oFile, _sFile, "wb"))
    {
      printf("Problem creating file: %s\n", _sFile);
    }
    else
    {
      if (!(_aBytes = _oS3D->LoadFile(_i)))
      {
        printf("Problem loading file: %s\n", _sFile);
      }
      else
      {
        fwrite(_aBytes, _oS3D->GetFileInfo(_i)->Size, 1, _oFile);
        
        delete _aBytes;

        _iFilesUnpacked++;
      }

      fclose(_oFile);
    }
  }

  _oS3D->Close();

  delete _oS3D;

  printf("%d files unpacked\n", _iFilesUnpacked);

  //_getch();

  return S_OK;
}

HRESULT UnpackWLD(char* Filename)
{
  WLDFile* _oWLD = new WLDFile();

  FILE* _oFile = NULL;
  DWORD _iFileSize;
  BYTE* _aBytes = NULL;
  HRESULT _iResult = S_OK;

  if (!_oWLD)
  {
    printf("Problem creating WLD object for %s\n", Filename);

    _iResult = E_OUTOFMEMORY;
  }
  
  if (_iResult == S_OK)
  {
    if (fopen_s(&_oFile, Filename, "rb"))
    {
      printf("Problem opening %s\n", Filename);

      _iResult = E_FAIL;
    }
  }

  if (_iResult == S_OK)
  {
    _iFileSize = _filelength(_fileno(_oFile));

    if (!(_aBytes = new BYTE[_iFileSize]))
    {
      printf("Out of memory loading %s\n", Filename);

      _iResult = E_OUTOFMEMORY;
    }
  }

  if (_iResult == S_OK)
  {
    if (fread_s(_aBytes, _iFileSize, _iFileSize, 1, _oFile) != 1)
    {
      printf("Problem reading %s\n", Filename);

      _iResult = E_FAIL;
    }
  }

  if (_iResult == S_OK)
  {
    if (FAILED(_oWLD->LoadFromMemory(_aBytes, _iFileSize, true, Filename, false)))
    {
      printf("Problem processing %s\n", Filename);
      
      _iResult = E_FAIL;
    }
  }

  if (_iResult == S_OK)
  {
    if (FAILED(_oWLD->ExportToTextFile()))
    {
      printf("Problem exporting %s to text\n", Filename, Filename);

      _iResult = E_FAIL;
    }
  }

  SAFE_DELETE(_oWLD);
  
  SAFE_DELETE(_aBytes);

  if (_oFile)
  {
    fclose(_oFile);
  }

  if (_iResult == S_OK)
  {
    printf("Exported %s to text file %s.txt\n", Filename);
  }

  return _iResult;
}

void ModelInventory()
{
    FILE* _oFile;
    intptr_t _iSearch;
    _finddata_t _oFileInfo;

    if (fopen_s(&_oFile, "ModelInventory.txt", "wt"))
    {
      return;
    }

    S3DPackage* _oS3D = new S3DPackage();
    WLDFile* _oWLD = new WLDFile();
    DWORD _iWLDFile;
    WLDFragment* _oMesh;
    BYTE* _oWLDFileBytes;
    char _sWLDFile[_MAX_PATH];
    char* _sExt;
    bool _bFoundMeshes;
    BYTE _iMeshType;
    
    int _l;
    int _i;

    if ((_iSearch = _findfirst("*_obj.s3d", &_oFileInfo)) != -1)
    {
      while (_iSearch != -1)
      {
        fprintf_s(_oFile, "%s\n", _oFileInfo.name);
        _sExt = strrchr(_oFileInfo.name, '.');
        _sExt[0] = '\0';

        strcpy_s(_sWLDFile, sizeof(_sWLDFile), _oFileInfo.name);
        strcat_s(_sWLDFile, sizeof(_sWLDFile), ".wld");

        _sExt[0] = '.';

        _oS3D->Open(_oFileInfo.name);

        _bFoundMeshes = false;
        if ((_iWLDFile = _oS3D->FindFile(_sWLDFile)) != S3D_INVALID)
        {
          if (_oWLDFileBytes = _oS3D->LoadFile(_iWLDFile))
          {
            if (SUCCEEDED(_oWLD->LoadFromMemory(_oWLDFileBytes, _oS3D->GetFileInfo(_iWLDFile)->Size, true, _oS3D->GetFilename(_iWLDFile), false)))
            {
              _oMesh = NULL;

              while (_oMesh = _oWLD->FindNextFragment(0x36, _oMesh))
              {
                if (!IsBlank(_oMesh->Name))
                {
                  if (RightMatch(_oMesh->Name, "_DMSPRITEDEF"))
                  {
                    _l = strlen(_oMesh->Name) - 12;
                    
                    _oMesh->Name[_l] = '\0';

                    _iMeshType = 1;

                    /*
                    if (_oMesh->Name[0] != 'R')
                    {
                      _iMeshType = 1;
                    }
                    else
                    {
                      for (_i = 1; _i < _l; _i++)
                      {
                        if (!IsNumeric(_oMesh->Name[_i]))
                        {
                          _iMeshType = 1;

                          break;
                        }
                      }
                    }
                    */
                  }
                  else
                  {
                    _iMeshType = 2;
                  }
                  
                  if (_iMeshType)
                  {
                    fprintf_s(_oFile, "  - %s%s\n", (_iMeshType == 2) ? "Abnormal: " : "", _oMesh->Name);
                  }

                  _bFoundMeshes = true;
                }
              }
            }
            delete _oWLDFileBytes;
          }

          // There is no public WLDFile::Close(). The previous session is automatically
          // closed upon the next LoadFromMemory() call or upon object destruction
        }

        if (_bFoundMeshes)
        {
          printf("x");
        }
        else
        {
          printf(".");
        }
        
        _oS3D->Close();

        if (_findnext(_iSearch, &_oFileInfo))
        {
          _iSearch = -1;
        }
      }
    }

    if ((_iSearch = _findfirst("*.eqg", &_oFileInfo)) != -1)
    {
      while (_iSearch != -1)
      {
        fprintf_s(_oFile, "%s\n", _oFileInfo.name);

        _oS3D->Open(_oFileInfo.name);

        _l = _oS3D->GetFileCount();
        _bFoundMeshes = false;
        for (_i = 0; _i < _l; _i++)
        {
          _sExt = _oS3D->GetFilename(_i);

          if (RightMatch(_sExt, ".mod") || RightMatch(_sExt, ".mds"))
          {
            _sExt[strlen(_sExt) - 4] = '\0';

            fprintf_s(_oFile, "  - %s\n", _sExt);

            _bFoundMeshes = true;
          }
        }

        _oS3D->Close();

        if (_bFoundMeshes)
        {
          printf("x");
        }
        else
        {
          printf(".");
        }

        if (_findnext(_iSearch, &_oFileInfo))
        {
          _iSearch = -1;
        }
      }
    }

    fclose(_oFile);

    printf("\n");

    delete _oWLD;
    delete _oS3D;
}