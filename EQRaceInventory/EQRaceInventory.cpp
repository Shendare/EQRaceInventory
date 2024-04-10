// EQRaceInventory.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "EQRaceInventory.h"

int _tmain(int argc, _TCHAR* argv[])
{
  bool _bFoundFiles = false;
  Log = NULL;
  iProcessedFiles = 0;
  iProcessedBytes = 0;

  LoadSettings();

  if (_chdir(sWorkPath))
  {
    printf("Error: Could not change to working path:\n\n%s", sWorkPath);

    return 1;
  }

  if (!FileExists(sOutPath))
  {
    if (_mkdir(sOutPath))
    {
      printf("FAILURE: Could not find or create output path %s", sOutPath);
    }
  }

  printf("EverQuest Race Information Inventory v1.0\n");
  printf("-----------------------------------------\n");
  
  printf("Loading EQ Client from: %s\n", EQClientFile);
  if (FAILED(EQ::Client.Load(EQClientFile)))
  {
    int iClient = EQ::Client.GetVersionOf(EQClientFile);

    if (iClient < 0)
    {
      if (iClient == -3)
      {
        printf("FAILURE: %s is not a compatible version. Only Titanium and SoF are supported.\n", EQClientFile);
      }
      else
      {
        printf(  "FAILURE: %s not found.\n"
                "\n"
                "This program must presently be run from your EverQuest directory.\n", EQClientFile);
      }
    }
    else
    {
      printf("FAILURE: Unknown problem loading EQ client file: %s", EQClientFile);
    }

    return Quit(1);
  }

  printf("  EQGame.exe recognized as the %s version.\n\n", EQ::Client.VersionNick);

  ProcessedFile(EQ::Client.FileSize);

  LogOpen();
  
  printf("\n");
  Log->Entry("Loading String List from %s...", sStringListFile);
  if (FAILED(EQ::StringList.Load(sStringListFile)))
  {
    Log->Entry("FAILURE: Could not read String List from %s", sStringListFile);
    
    printf(  "\n"
            "This program must presently be run from your EverQuest directory.\n", EQClientFile);

    return Quit(2);
  }
  else
  {
    Log->Entry("  Found %d Strings. Highest StringID is %d", (void*)EQ::StringList.Count, (void*)EQ::StringList.MaxID);
  }

  ProcessedFile(sStringListFile);

  printf("\n");
  Log->Entry("Loading String Database from %s...", sDBStringFile);
  if (FAILED(EQ::DBStrings.Load(sDBStringFile)))
  {
    Log->Entry("FAILURE: Could not read String Database from %s", sDBStringFile);
    
    printf(  "\n"
            "This program must presently be run from your EverQuest directory.\n", EQClientFile);

    return Quit(3);
  }
  else
  {
    Log->Entry("  Found %d DB Strings, %d Types. Highest DBStringID is %d", (void*)EQ::DBStrings.Count, (void*)EQ::DBStrings.MaxType, (void*)EQ::DBStrings.MaxID);
  }

  ProcessedFile(sDBStringFile);

  printf("\n");
  Log->Entry("Loading Race Information...");
  if (FAILED(EQ::Client.LoadRaces()))
  {
    Log->Entry("FAILURE: Could not read race code information from client %s", EQClientFile);
    
    printf(  "\n"
            "This program must presently be run from your EverQuest directory.\n", EQClientFile);

    return Quit(4);
  }
  else
  {
    Log->Entry("  Found %d races, max RaceID is %d", (void*)EQ::Client.Races, (void*)EQ::Client.Race_MaxID);
  }

  printf("\n");
  Log->Entry("Loading Zone Information...");
  if (FAILED(EQ::Client.LoadZones()))
  {
    Log->Entry("FAILURE: Could not read zone information from client %s", EQClientFile);
    
    printf(  "\n"
            "This program must presently be run from your EverQuest directory.\n", EQClientFile);

    return Quit(5);
  }
  else
  {
    Log->Entry("  Found %d zones, max ZoneID is %d", (void*)EQ::Client.Zones, (void*)EQ::Client.Zone_MaxID);
  }

  printf("\n");
  Log->Entry("Loading Global Model Information...");
  if (FAILED(LoadGlobalModels()))
  {
    Log->Entry("  FAILURE: Could not load global model information.");
  }
  else
  {
    _bFoundFiles = true;
  }

  printf("\n");
  Log->Entry("Loading Model Information For Each Zone...");
  
  WORD    _iZone;
  WORD    _iZoneMax = EQ::Client.Zone_MaxID;
  EQZone* _oZone;
  for (_iZone = 0; _iZone < _iZoneMax; _iZone++)
  {
    if (_oZone = &EQ::Client.Zone[_iZone])
    {
      if (_oZone->Nick)
      {
        //printf("Loading models for %s (%s)...\n", _oZone->Name, _oZone->Nick);
        if (FAILED(LoadZoneModels(_oZone)))
        {
          Log->Entry("SKIPPED: Failed model loading for Zone %d (%s), '%s'", (void*)_iZone, _oZone->Nick, _oZone->Name);
        }
        else
        {
          _bFoundFiles = true;
        }
      }
    }
  }

  printf("\n");
  Log->Entry("Loading Orphan Model Information...");
  if (FAILED(LoadOrphanModels()))
  {
    Log->Entry("  FAILURE: Could not load orphan model information.");
  }
  else
  {
    _bFoundFiles = true;
  }

  printf("\n");
  Log->Entry("Exporting Race Information...");
  if (FAILED(ExportRaceInfo()))
  {
    Log->Entry("FAILURE: Problem exporting Race information to %s", sOutFile_Races);
  }

  printf("\n");
  Log->Entry("Exporting Zone Information...");
  if (FAILED(ExportZoneInfo()))
  {
    Log->Entry("FAILURE: Problem exporting Zone information to %s", sOutFile_Zones);
  }

  printf("\n");
  Log->Entry("Exporting Zone/Race Availability Information...");
  if (FAILED(ExportZoneRaceAvailability()))
  {
    //printf("PROBLEM: Could not write zone/race availability information to output files.\n");
  }

  printf("\n");
  Log->Entry("Logging Various Maximums Found...");
  EQ::LogMaximums(Log);

  if (!_bFoundFiles)
  {
    Log->Entry("FAILURE: No EverQuest race-related files were found in the current directory.");

    printf(  "\n"
            "This program must presently be run from your EverQuest directory.\n");

    return Quit(9);
  }

  char _sBuf1[32];
  sprintf_s(_sBuf1, sizeof(_sBuf1), "%1.2f", (double)iProcessedBytes/1048576.0f);

  char _sBuf2[32];
  sprintf_s(_sBuf2, sizeof(_sBuf2), "%1.2f", (double)clock() / CLOCKS_PER_SEC);

  Log->Entry("Processed %s MB in %d files in %s seconds.", _sBuf1, (void*)iProcessedFiles, _sBuf2);

  printf("\n");
  Log->Entry("Finished!");

  return Quit(0);
}

int Quit(int ReturnCode)
{
  if (Log)
  {
    delete Log;

    Log = NULL;
  }

  _getch();

  return ReturnCode;
}

HRESULT LoadGlobalModels()
{
  // If EQ Titanium
  
    // Titanium Hardcoded global file list

  // Else If EQ SoF

    // SOF Hardcoded global file list

  /*
  global4_chr
  global3_chr
  global2_chr
  load_chr
  global7_chr
  frog_mount_chr
  global5_chr
  global5_chr2
  global%s_chr    // 6428cc  (ref in 43ef4d)
  global%s_chr2    // 6428dc  (ref in 43e4b9)

  */

  // Else
    
    // Error

  // End If

  char        _sLine[1024];
  WORD        _iLineLen = 0;
  char        _sFilename[_MAX_PATH];
  FILE*       _oFile;
  EQSource*   _oSource = NULL;
  bool        _bEOF = false;
  char*       _sField;
  char*       _sSource;
  WORD        _j;
  WORD        _i;
  WORD        _iRaceID;
  BYTE        _iGender;

  _sLine[0] = NULL;
  
  Log->Entry("  Loading Hard-Coded Non-Luclin Global Models...");
  for (_j = 0; _j < 1; _j++)
  {
    switch (_j)
    {
      case 0:
        _sSource = "global7_chr";
        break;
    }

    for (_i = 0; _i < 2; _i++)
    {
      sprintf_s(_sLine, sizeof(_sLine), "%s.%s", _sSource, (_i == 1) ? "s3d" : "eqg");

      if (FileExists(_sLine))
      {
        Log->Entry(QUIET, "   Loading global models from %s", _sLine);
        _oSource = EQ::AddSource(_sLine, EQSA_GLOBAL_NONLUCLIN, false);

        if ((!_oSource) || FAILED(((_i == 1) ? LoadModels_S3D(_oSource) : LoadModels_EQG(_oSource))))
        {
          Log->Entry("    SKIPPED: Problem loading global models from %s", _sLine);
        }
        else
        {
          ProcessedFile(_sLine);
        }
      }
    }
  }
  
  Log->Entry("  Loading Luclin playable race model sources...");
  
  // Playable race Luclin globals (globalHUM_chr, globalHUM_chr2)
  for (_iRaceID = 0; _iRaceID <= EQ::Client.Race_MaxID; _iRaceID++)
  {
    if (EQ::Client.Race[_iRaceID].IsPlayable())
    {
      for (_iGender = EQRG_MALE; _iGender <= EQRG_NEUTRAL; _iGender++)
      {
        for (_i = 0; _i < 2; _i++)
        {
          sprintf_s(_sLine, sizeof(_sLine), "global%s_chr%s.s3d", EQ::Client.GetModelCode(_iRaceID, (EQGENDER)_iGender), (_i == 1) ? "2" : "");

          if (FileExists(_sLine))
          {
            Log->Entry(QUIET, "   Loading global models from %s", _sLine);
            _oSource = EQ::AddSource(_sLine, EQSA_GLOBAL_LUCLIN, false);

            if ((!_oSource) || FAILED(LoadModels_S3D(_oSource)))
            {
              Log->Entry("    SKIPPED: Problem loading global models from %s", _sLine);
            }
            else
            {
              ProcessedFile(_sLine);
            }
          }
        }
      }
    }
  }

  Log->Entry("  Loading Hard-Coded Luclin Global Models...");
  for (_j = 0; _j < 3; _j++)
  {
    switch (_j)
    {
      case 0:
        _sSource = "global5_chr2";
        break;
      case 1:
        _sSource = "global5_chr";
        break;
      case 2:
        _sSource = "frog_mount_chr";
        break;
    }

    for (_i = 0; _i < 2; _i++)
    {
      sprintf_s(_sLine, sizeof(_sLine), "%s.%s", _sSource, (_i == 1) ? "s3d" : "eqg");

      if (FileExists(_sLine))
      {
        Log->Entry(QUIET, "   Loading global models from %s", _sLine);
        _oSource = EQ::AddSource(_sLine, EQSA_GLOBAL_LUCLIN, false);

        if ((!_oSource) || FAILED(((_i == 1) ? LoadModels_S3D(_oSource) : LoadModels_EQG(_oSource))))
        {
          Log->Entry("    SKIPPED: Problem loading global models from %s", _sLine);
        }
        else
        {
          ProcessedFile(_sLine);
        }
      }
    }
  }
  
  Log->Entry("  Loading and parsing %s...", sGlobalLoadFile);

  // Resources\GlobalLoad.txt
  if (fopen_s(&_oFile, sGlobalLoadFile, "rt"))
  {
    Log->Entry("  SKIPPING: %s not found", sGlobalLoadFile);

    _bEOF = true;
    _oFile = NULL;
  }

  while (!_bEOF)
  {
    _sLine[0] = 0x00;
    while ((!_bEOF) && (_sLine[0] == 0x00))
    {
      if (!fgets(_sLine, sizeof(_sLine), _oFile))
      {
        _bEOF = true;
      }
      else
      {
        TrimString(_sLine);
      }
    }

    if (!_bEOF)
    {
      _sField = _sLine;
      for (_i = 0; _i < 4; _i++)
      {
        _sSource = _sField;
        if (!(_sField = strchr(_sField, ',')))
        {
          break;
        }
        else if (_i < 3)
        {
          // Next character = next column (unless we're at the last column)
          _sField++;
        }
      }
    }

    if ((!_bEOF) && (_i == 4))
    {
      _sField[0] = NULL;
      // _sSource has our file source.

      switch (EQ::Client.VersionNum)
      {
        case EQC_TITANIUM:
          if (_sLine[0] == '2')
          {
            _sSource = NULL;
          }
          else if ((!RightMatch(_sSource, ".eqg")) && 
                   (!RightMatch(_sSource, "_chr")) &&
                   (!RightMatch(_sSource, "_chr2")))
          {
            _sSource = NULL;
          }
          break;
        case EQC_SOF:
          _sSource -= 2;
          if ((_sSource[0] & 0xDF) == 'C')
          {
            _sSource += 2;
          }
          else
          {
            _sSource = NULL;
          }
          break;
      }

      if (!IsBlank(_sSource))
      {
        TrimString(_sSource);
        _strlwr_s(_sSource, strlen(_sSource) + 1);

        if (strchr(_sSource, '.') == NULL)
        {
          if (!IsBlank(_sSource))
          {
            sprintf_s(_sFilename, sizeof(_sFilename), "%s.eqg", _sSource);

            if (FileExists(_sFilename))
            {
              _sSource = _sFilename;
            }
            else
            {
              sprintf_s(_sFilename, sizeof(_sFilename), "%s.s3d", _sSource);

              if (FileExists(_sFilename))
              {
                _sSource = _sFilename;
              }
              else
              {
                Log->Entry("   SKIPPED: GlobalLoad - Could not find model source %s.*", _sSource);
                
                _sSource = NULL;
              }
            }
          }
        }
        else if (!FileExists(_sSource))
        {
          Log->Entry("   SKIPPED: GlobalLoad - Could not find model source %s", _sSource);

          _sSource = NULL;
        }
      }

      if (!IsBlank(_sSource))
      {
        if (RightMatch(_sSource, ".s3d"))
        {
          if (!(_oSource = EQ::AddSource(_sSource, EQSA_GLOBAL_TEXTFILE, false)))
          {
            Log->Entry("   SKIPPED: Could not add model source for %s", _sSource);
          }
          else
          {
            Log->Entry(QUIET, "   Loading global model source %s", _sSource);
            if (FAILED(LoadModels_S3D(_oSource)))
            {
              Log->Entry("   SKIPPED: Could not load model source %s", _sSource);
            }
            else
            {
              ProcessedFile(_sSource);
            }
          }
        }
        else if (RightMatch(_sSource, ".eqg"))
        {
          if (!(_oSource = EQ::AddSource(_sSource, EQSA_GLOBAL_TEXTFILE, false)))
          {
            Log->Entry("   SKIPPED: Could not add model source for %s", _sSource);
          }
          else
          {
            Log->Entry(QUIET, "   Loading global model source %s", _sSource);
            if (FAILED(LoadModels_EQG(_oSource)))
            {
              Log->Entry("   SKIPPED: Could not load model source %s", _sSource);
            }
            else
            {
              ProcessedFile(_sSource);
            }
          }
        }
        else
        {
          Log->Entry("   ODDITY: Did not recognize model source file extension for %s", _sSource);
        }
      }
    }
  }

  ProcessedFile(sGlobalLoadFile);

  if (_oFile)
  {
    fclose(_oFile);
  }
  
  return S_OK;
}

HRESULT LoadOrphanModels()
{
  char        _sSource[64];
  WORD        _iSources;
  EQSource*   _oSource;
  
  char*       _sModelCode;
  char*       _sModelCodePrev;

  WORD        _s;
  WORD        _iRaceID;
  BYTE        _iGender;

  bool        _bFound;

  _iSources = EQ::ModelSources;
  _sModelCodePrev = NULL;
  
  // Playable race Luclin globals (globalHUM_chr, globalHUM_chr2)
  for (_iRaceID = 0; _iRaceID <= EQ::Client.Race_MaxID; _iRaceID++)
  {
    for (_iGender = EQRG_MALE; _iGender <= EQRG_NEUTRAL; _iGender++)
    {
      _bFound = false;
      _sModelCode = EQ::Client.GetModelCode(_iRaceID, (EQGENDER)_iGender);

      if (!IsBlank(_sModelCode))
      {
        for (_s = 0; (!_bFound) && (_s < _iSources); _s++)
        {
          if (EQ::ModelSource[_s].HasModel(_iRaceID, (EQGENDER)_iGender, NULL))
          {
            _bFound = true;
          }
        }

        if (!_bFound)
        {
          // Orphan model code. Find its source!

          sprintf_s(_sSource, sizeof(_sSource), "%s.eqg", _sModelCode);
          if (!FileExists(_sSource))
          {
            sprintf_s(_sSource, sizeof(_sSource), "%s_chr.s3d", _sModelCode);
            if (!FileExists(_sSource))
            {
              // No recognizable source for this model code.
              if (_sModelCode != _sModelCodePrev)
              {
                Log->Entry(QUIET, "  ODDITY: Model code '%s' is truly orphaned (no sources)", _sModelCode);
              }

              _sModelCodePrev = _sModelCode;
              
              continue;
            }
          }

          if (!(_oSource = EQ::AddSource(_sSource, EQSA_LOCAL, true)))
          {
            Log->Entry("  FAILURE: Problem adding orphan model source %s", _sSource);

            continue;
          }

          if (RightMatch(_sSource, ".eqg"))
          {
            if (FAILED(LoadModels_EQG(_oSource)))
            {
              Log->Entry("  SKIPPED: Problem loading model information from %s", _sSource);
            }
            else
            {
              Log->Entry(QUIET, "  Loaded orphan model source %s", _sSource);
              
              ProcessedFile(_sSource);
            }
          }
          else
          {
            if (FAILED(LoadModels_S3D(_oSource)))
            {
              Log->Entry("  SKIPPED: Problem loading model information from %s", _sSource);
            }
            else
            {
              Log->Entry(QUIET, "  Loaded orphan model source %s", _sSource);
              
              ProcessedFile(_sSource);
            }
          }
        }
      }
    }
  }

  return S_OK;
}

HRESULT ExportModelVariations2(FILE* File, BYTE Count, BYTE* Variations)
{
  if ((!File) || (!Count) || (!Variations))
  {
    return E_INVALIDARG;
  }

  BYTE _i;
  BYTE _v;
  BYTE _p2 = 0;
  BYTE _p = 0;
  bool _bSkipped = false;

  fprintf_s(File, "00");

  for (_i = 0; _i < Count; _i++)
  {
    _v = Variations[_i];

    if (_v != (_p + 1))
    {
      if (_bSkipped)
      {
        fprintf_s(File, "%c%d%d,", (_p == (_p2 + 1)) ? ' ' : '-', _p / 10 % 10, _p % 10);

        _p2 = _v;
      }

      fprintf_s(File, " %d%d", _v / 10 % 10, _v % 10);

      _bSkipped = false;
    }
    else
    {
      _bSkipped = true;
    }

    _p = _v;
  }

  if (_bSkipped)
  {
    fprintf_s(File, "%c%d%d", (_p == (_p2 + 1)) ? ' ' : '-', _p / 10 % 10, _p % 10);
  }

  return S_OK;
}

HRESULT ExportModelVariations(FILE* File, EQModel* Model)
{
  if ((!File) || (!Model))
  {
    return E_INVALIDARG;
  }

  char* _sPrefix = " -";
  char* _sPrefix2 = "</b>,";

  if (Model->VarsTexture > 0)
  {
    fprintf_s(File, "%s Textures: <b>", _sPrefix);
    ExportModelVariations2(File, Model->VarsTexture, &Model->VarTexture[0]);
    _sPrefix = _sPrefix2;
  }

  if (Model->VarsHead > 0)
  {
    fprintf_s(File, "%s Heads: <b>", _sPrefix);
    ExportModelVariations2(File, Model->VarsHead, &Model->VarHead[0]);
    _sPrefix = _sPrefix2;
  }

  if (EQ::Client.Race[Model->RaceID].IsPlayable())
  {
    if (Model->VarsFace > 0)
    {
      fprintf_s(File, "%s Faces: <b>", _sPrefix);
      ExportModelVariations2(File, Model->VarsFace, &Model->VarFace[0]);
      _sPrefix = _sPrefix2;
    }

    if (Model->VarsHair > 0)
    {
      fprintf_s(File, "%s Hair: <b>", _sPrefix);
      ExportModelVariations2(File, Model->VarsHair, &Model->VarHair[0]);
      _sPrefix = _sPrefix2;
    }

    if (Model->VarsBeard > 0)
    {
      fprintf_s(File, "%s Beards: <b>", _sPrefix);
      ExportModelVariations2(File, Model->VarsBeard, &Model->VarBeard[0]);
      _sPrefix = _sPrefix2;
    }

    if (Model->VarsDetail > 0)
    {
      fprintf_s(File, "%s Detail: <b>", _sPrefix);
      ExportModelVariations2(File, Model->VarsDetail, &Model->VarDetail[0]);
      _sPrefix = _sPrefix2;
    }

    if (Model->VarsTattoo > 0)
    {
      fprintf_s(File, "%s Tattoo: <b>", _sPrefix);
      ExportModelVariations2(File, Model->VarsTattoo, &Model->VarTattoo[0]);
      _sPrefix = _sPrefix2;
    }

    if (Model->RaceID == 522)
    {
      // Drakkin.
      fprintf_s(File, "%s Heritage: <b>00-07", _sPrefix);
      _sPrefix = _sPrefix2;
    }
  }

  if (_sPrefix == _sPrefix2)
  {
    fprintf_s(File, "</b>");
  }

  return S_OK;
}

HRESULT ExportRaceInfo()
{
  FILE* _oOutFile;
  WORD  _iRaceID;
  char* _sModelCode[EQRACE_GENDERCOUNT];
  char* _sGenderCode = "MFNX";

  char _sFilename[_MAX_PATH];
  
  sprintf_s(_sFilename, sizeof(_sFilename), "%s_%s", EQ::Client.VersionNick, sOutFile_Codes);

  if (fopen_s(&_oOutFile, MakePath(sOutPath, _sFilename), "wb"))
  {
    Log->Entry("FAILURE: Could not write race codes to output file: %s", _sFilename);

    return E_FAIL;
  }

  Log->Entry("  Listing EQRace model codes in %s", _sFilename);
  
  fputs("RaceID\tRaceName\tModelCode_M\tModelCode_F\tModelCode_N\r\n", _oOutFile);

  for (_iRaceID = 0; _iRaceID <= EQRACE_MAXID; _iRaceID++)
  {
    _sModelCode[0] = IfBlank(EQ::Client.GetModelCode(_iRaceID, EQRG_MALE), "");
    _sModelCode[1] = IfBlank(EQ::Client.GetModelCode(_iRaceID, EQRG_FEMALE), "");
    _sModelCode[2] = IfBlank(EQ::Client.GetModelCode(_iRaceID, EQRG_NEUTRAL), "");

    if (_sModelCode[0][0] || _sModelCode[1][0] || _sModelCode[2][0])
    {
      fprintf_s(_oOutFile, "%d\t%s\t%s\t%s\t%s\r\n", _iRaceID, EQ::Client.Race[_iRaceID].GetName(), _sModelCode[0], _sModelCode[1], _sModelCode[2]);
    }
  }

  fclose(_oOutFile);
  
  // Make pretty Race output

  sprintf_s(_sFilename, sizeof(_sFilename), "%s_%s", EQ::Client.VersionNick, sOutFile_Races);

  if (fopen_s(&_oOutFile, MakePath(sOutPath, _sFilename), "wb"))
  {
    Log->Entry("FAILURE: Could not write race information to output file: %s", _sFilename);

    return E_FAIL;
  }

  Log->Entry("  Exporting EQRace information to %s", _sFilename);

  fprintf_s(_oOutFile,
    "<html>\r\n"
    "<head>\r\n"
    "<title>EverQuest Race Inventory</title>\r\n"
    "<style type=\"text/css\">\r\n"
    "<!--\r\n"
    "\r\n"
    "body { font-family:Calibri,Tahoma,sans-serif; cursor:default; margin:4px; }\r\n"
    "ul { margin-top:0; margin-bottom:8px; }\r\n"
    ".GenM { color:#4444ff; }\r\n"
    ".GenF { color:#ff6666; }\r\n"
    ".GenN { color:#339933; }\r\n"
    ".Global0 { color:#cc3333; }\r\n"
    ".Global1 { color:#999999; }\r\n"
    ".Global2 { color:#336699; }\r\n"
    ".Global3 { color:#3333cc; }\r\n"
    ".Global4 { color:#339933; }\r\n"
    "\r\n"
    "-->\r\n"
    "</style>\r\n"
    "<style type=\"text/css\">\r\n"
    "<!--\r\n"
    "\r\n"
    ".ScriptHide { display:none; }\r\n"
    ".ScriptShow { display:inline; }\r\n"
    ".FrameLeft { width:17%%; height:100%%; overflow:scroll; float:left; font-size:10pt; }\r\n"
    ".FrameRight { width:82%%; height:100%%; overflow:auto; float:right; padding-right:6px; }\r\n"
    "nobr a { cursor:pointer; }\r\n"
    "nobr a:hover { color:blue; }\r\n"
    "\r\n"
    "-->\r\n"
    "</style>\r\n"
    "<noscript>\r\n"
    "<style type=\"text/css\">\r\n"
    "<!--\r\n"
    "\r\n"
    ".ScriptHide { display:inline; }\r\n"
    ".ScriptShow { display:none; }\r\n"
    ".FrameRight { width:99%%; }\r\n"
    "\r\n"
    "-->\r\n"
    "</style>\r\n"
    "</noscript>\r\n"
    "<script language=\"javascript\">\r\n"
    "<!--\r\n"
    "\r\n"
    "  var iSaveRace = 1;\r\n"
    "  var iLastRace = 1;\r\n"
    "\r\n"
    "  function hideStyle(s)\r\n"
    "  {\r\n"
    "    if (document.styleSheets[1].rules)\r\n"
    "    {\r\n"
    "      document.styleSheets[1].rules[0].style.display=s;\r\n"
    "    }\r\n"
    "    else\r\n"
    "    {\r\n"
    "      document.styleSheets[1].cssRules[0].style.display=s;\r\n"
    "    }\r\n"
    "    \r\n"
    "    document.getElementById('HideShow').innerHTML = (s == 'none') ? 'Show All' : 'Show Single';\r\n"
    "  }\r\n"
    "  \r\n"
    "  function OverRace()\r\n"
    "  {\r\n"
    "    var el;\r\n"
    "    if ((el = window.event.srcElement).tagName == 'A')\r\n"
    "    {\r\n"
    "      el.style.color = 'blue';\r\n"
    "    }\r\n"
    "  }\r\n"
    "  \r\n"
    "  function OutRace()\r\n"
    "  {\r\n"
    "    var el;\r\n"
    "    if ((el = window.event.srcElement).tagName == 'A')\r\n"
    "    {\r\n"
    "      el.style.color = '';\r\n"
    "    }\r\n"
    "  }\r\n"
    "  \r\n"
    "  function ShowRace(e)\r\n"
    "  {\r\n"
    "    if (window.event) e = window.event;\r\n"
    "    var el = (e.srcElement) ? e.srcElement : e.target;\r\n"
    "    var i = el.innerHTML.indexOf('-');\r\n"
    "    if (el.tagName == 'A')\r\n"
    "    {\r\n"
    "      if (iLastRace > 0)\r\n"
    "      {\r\n"
    "        document.getElementById('Race' + iLastRace).style.display = '';\r\n"
    "      }\r\n"
    "\r\n"
    "      if (iLastRace == -1)\r\n"
    "      {\r\n"
    "        hideStyle('none');\r\n"
    "      }\r\n"
    "        \r\n"
    "      if (i == -1)\r\n"
    "      {\r\n"
    "        if (iLastRace == -1)\r\n"
    "        {\r\n"
    "          iLastRace = iSaveRace;\r\n"
    "          document.getElementById('Race' + iLastRace).style.display = 'inline';\r\n"
    "        }\r\n"
    "        else\r\n"
    "        {\r\n"
    "          hideStyle('inline');\r\n"
    "          iLastRace = -1;\r\n"
    "        }\r\n"
    "      }\r\n"
    "      else\r\n"
    "      {\r\n"
    "        var iRace = el.innerHTML.substr(0, el.innerHTML.indexOf('-') - 1);\r\n"
    "        document.getElementById('Race' + iRace).style.display = 'inline';\r\n"
    "        iLastRace = iRace;\r\n"
    "        iSaveRace = iLastRace;\r\n"
    "      }\r\n"
    "    }\r\n"
    "  }\r\n"
    "\r\n"
    "//-->\r\n"
    "</script>\r\n"
    "</head>\r\n"
    "<body>\r\n"
    "\r\n");

  WORD        _iZoneID;
  EQZone*     _oZone;
  char*       _sText1;
  char*       _sText2;
  char*       _sText3;
  char*       _sNotAvail = "N/A";
  char*       _sNotAvailB = "</b>N/A</b>";
  bool        _bZoneFound2;
  bool        _bZoneFound;
  BYTE        _iSourcesFound;
  bool        _bFound2[EQRACE_GENDERCOUNT];
  EQModel*    _oModel[EQRACE_GENDERCOUNT];

  BYTE        _iRaceModels;
  BYTE        _iSourceModels;
  BYTE        _iZoneModels;
  bool        _bSkinned;

  EQSource*   _oSource;
  WORD        _iSource;

  BYTE        _g;

  fprintf_s(_oOutFile,  
    "<span class=\"FrameLeft ScriptShow\">\r\n"
    "\r\n"
    "<nobr onclick=\"ShowRace(event);\" onmouseover=\"OverRace();\" onmouseout=\"OutRace();\">\r\n"
    "<p style=\"cursor:default\"><b>Choose Race:</b></p>\r\n"
    "<b><a id=\"HideShow\">Show All</a></b><br />\r\n");

  for (_iRaceID = 1; _iRaceID <= EQ::Client.Race_MaxID; _iRaceID++)
  {
    if (EQ::Client.Race[_iRaceID].ID != _iRaceID)
    {
      // Race does not exist
      
      continue;
    }
    _sModelCode[0] = IfBlank(EQ::Client.GetModelCode(_iRaceID, EQRG_MALE), _sNotAvailB);
    _sModelCode[1] = IfBlank(EQ::Client.GetModelCode(_iRaceID, EQRG_FEMALE), _sNotAvailB);
    _sModelCode[2] = IfBlank(EQ::Client.GetModelCode(_iRaceID, EQRG_NEUTRAL), _sNotAvailB);

    _sText1 = IfBlank(EQ::Client.Race[_iRaceID].GetName(), _sNotAvail);
    _sText2 = IfBlank(EQ::Client.Race[_iRaceID].GetPlural(), _sNotAvail);

    if ((_sModelCode[0] == _sNotAvailB) &&
        (_sModelCode[1] == _sNotAvailB) &&
        (_sModelCode[2] == _sNotAvailB) &&
        (_sText1 == _sNotAvail) &&
        (_sText2 == _sNotAvail))
    {
      // This race does not exist.

      // Sometimes there are no models (_sModelCodeX) for a race (so the client can't render it),
      // but there are still entries in the Race Name table of dbstr_us.txt (_sText1 & _sText2)
      
      continue;
    }

    fprintf_s(_oOutFile, "<a>%d - %s", _iRaceID, IfBlank(EQ::Client.Race[_iRaceID].GetName(), "Unknown Race"));
    
    if (EQ::Client.Race[_iRaceID].IsPlayable())
    {
      fprintf_s(_oOutFile, " (P)");
    }
    else if (EQ::Client.Race[_iRaceID].IsGlobal(NULL))
    {
      fprintf_s(_oOutFile, " (G)");
    }
    
    fprintf_s(_oOutFile, "</a><br />\r\n");
  }
  
  fprintf_s(_oOutFile,
    "</nobr>\r\n"
    "\r\n"
    "</span>\r\n"
    "<span class=\"FrameRight\">\r\n"
    "\r\n");

  for (_iRaceID = 1; _iRaceID <= EQ::Client.Race_MaxID; _iRaceID++)
  {
    if (EQ::Client.Race[_iRaceID].ID != _iRaceID)
    {
      // Race does not exist
      
      continue;
    }
    _sModelCode[0] = IfBlank(EQ::Client.GetModelCode(_iRaceID, EQRG_MALE), _sNotAvailB);
    _sModelCode[1] = IfBlank(EQ::Client.GetModelCode(_iRaceID, EQRG_FEMALE), _sNotAvailB);
    _sModelCode[2] = IfBlank(EQ::Client.GetModelCode(_iRaceID, EQRG_NEUTRAL), _sNotAvailB);

    _sText1 = IfBlank(EQ::Client.Race[_iRaceID].GetName(), _sNotAvail);
    _sText2 = IfBlank(EQ::Client.Race[_iRaceID].GetPlural(), _sNotAvail);

    if ((_sModelCode[0] == _sNotAvailB) &&
        (_sModelCode[1] == _sNotAvailB) &&
        (_sModelCode[2] == _sNotAvailB) &&
        (_sText1 == _sNotAvail) &&
        (_sText2 == _sNotAvail))
    {
      // This race does not exist.

      // Sometimes there are no models (_sModelCodeX) for a race (so the client can't render it),
      // but there are still entries in the Race Name table of dbstr_us.txt (_sText1 & _sText2)
      
      continue;
    }

    fprintf_s(_oOutFile,
      "<span id=\"Race%d\" class=\"ScriptHide\"%s>\r\n"
      "\r\n", _iRaceID, (_iRaceID == 1) ? " style=\"display:inline\"" : "");

    fprintf_s(_oOutFile, "<hr>\r\n");

    _sText3 = (EQ::Client.Race[_iRaceID].IsPlayable()) ? " (Playable)" : "";

    fprintf_s(_oOutFile, "<h3>Race # %d - %s, Plural '%s'%s</h3>\r\n\r\n", _iRaceID, _sText1, _sText2, _sText3);

    if (_sText1 = EQ::Client.Race[_iRaceID].GetDesc())
    {
      fprintf_s(_oOutFile, "<p><b>Character Creation Description:</b></p>\r\n\r\n<p>%s</p>\r\n\r\n", _sText1);
    }

    fprintf_s(_oOutFile,
      "<p><b>Model Codes:</b></p>\r\n"
      "<ul>\r\n"
      "<li>Male = <b class=\"GenM\">%s</b></li>\r\n"
      "<li>Female = <b class=\"GenF\">%s</b></li>\r\n"
      "<li>Neutral = <b class=\"GenN\">%s</b></li>\r\n"
      "</ul>\r\n", _sModelCode[0], _sModelCode[1], _sModelCode[2]);

    fprintf_s(_oOutFile, "<p><b>Model availability:</b></p>\r\n\r\n");
    fprintf_s(_oOutFile, "<ul>\r\n");

    _iSourcesFound = 0;
    _bZoneFound = false;

    _iRaceModels = 0;
    for (_g = 0; _g < EQRACE_GENDERCOUNT; _g++)
    {
      _sModelCode[_g] = EQ::Client.GetModelCode(_iRaceID, (EQGENDER)(_g + EQRG_MALE));
      if (!IsBlank(_sModelCode[_g]))
      {
        _iRaceModels++;
      }
    }

    for (_iSource = 0; _iSource < EQ::ModelSources; _iSource++)
    {
      _oSource = &EQ::ModelSource[_iSource];

      _iSourceModels = 0;
      _bZoneFound2 = false;

      if (_oSource->HasRace(_iRaceID))
      {
        fprintf_s(_oOutFile, "<li>Via Source: %s", _oSource->Filename);
        if (_oSource->Global != EQSA_LOCAL)
        {
          fprintf_s(_oOutFile, " (<span class=\"Global%d\">%s</span>)", _oSource->Global, EQGlobalName[_oSource->Global]);
        }
        fprintf_s(_oOutFile, "<br />\r\n");
        
        _bSkinned = false;

        for (_g = 0; _g < EQRACE_GENDERCOUNT; _g++)
        {
          if (_bFound2[_g] = (_oSource->HasModel(_iRaceID, (EQGENDER)(_g + EQRG_MALE), &_oModel[_g])))
          {
            if (_iSourceModels == 0)
            {
              _iSourcesFound++;
            }
            _iSourceModels++;

            if (!_bSkinned)
            {
              if ((_oModel[_g]->VarsTexture) || (_oModel[_g]->VarsHead))
              {
                _bSkinned = true;
              }
            }
          }
        }

        if (_iSourceModels)
        {
          for (_g = 0; _g < EQRACE_GENDERCOUNT; _g++)
          {
            if (_bFound2[_g])
            {
              if ((_iRaceModels != _iSourceModels) || (_bSkinned))
              {
                if ((_iSourceModels > 1) || (_iRaceModels != _iSourceModels))
                {
                  fprintf_s(_oOutFile, "- Model <b class=\"Gen%c\">%s</b>", _sGenderCode[_oModel[_g]->Gender - EQRG_MALE], _oModel[_g]->Code);
                }
                
                ExportModelVariations(_oOutFile, _oModel[_g]);
                fprintf_s(_oOutFile, "<br />\r\n");
              }
            }
          }
        }

        fprintf_s(_oOutFile, "<ul>\r\n");

        if (_oSource->Global != EQSA_LOCAL)
        {
          _bZoneFound = true;
          _bZoneFound2 = true;
          
          fprintf_s(_oOutFile, "<li><b>All Zones</b> (<b class=\"Global3\">Global</b>)</li>"); 
        }
        else
        {
          for (_iZoneID = 0; _iZoneID <= EQ::Client.Zone_MaxID; _iZoneID++)
          {
            _oZone = &EQ::Client.Zone[_iZoneID];
            
            if (_oZone->HasSource(_oSource))
            {
              _iZoneModels = 0;
              for (_g = 0; _g < EQRACE_GENDERCOUNT; _g++)
              {
                _bFound2[_g] = false;

                if (_oZone->HasModel(_iRaceID, (EQGENDER)(_g + EQRG_MALE), &_oModel[_g]))
                {
                  if (_oModel[_g]->Source == _oSource)
                  {
                    _bZoneFound = true;
                    _bZoneFound2 = true;
          
                    _bFound2[_g] = true;

                    _iZoneModels++;
                  }
                }
              }

              if (_iZoneModels)
              {
                _iSourcesFound = true;

                fprintf_s(_oOutFile, "<li>Zone # %d - <b>%s</b> (<b class=\"Global3\">%s</b>)", _oZone->ID, _oZone->Name, _oZone->Nick);

                if ((_oSource == _oZone->ModelSource1) || (_oSource == _oZone->ModelSource2))
                {
                  fprintf_s(_oOutFile, ", <span class=\"Global0\">Local</span>");
                }
                else
                {
                  fprintf_s(_oOutFile, ", <span class=\"Global4\">Imported</span>");
                }
                
                if (_iZoneModels != _iSourceModels)
                {
                  fprintf_s(_oOutFile, " -");

                  for (_g = 0; _g < EQRACE_GENDERCOUNT; _g++)
                  {
                    if (_bFound2[_g])
                    {
                      fprintf_s(_oOutFile, " <b class=\"Gen%c\">%s</b>", _sGenderCode[_g], _sModelCode[_g]);
                    }
                  }
                }
                
                fprintf_s(_oOutFile, "</li>\r\n");
              }
            }
          }
        }
        
        if ((_iSourceModels) && (!_bZoneFound2))
        {
          fprintf_s(_oOutFile, "<li>No zones found using this source.</li>\r\n");
        }

        fprintf_s(_oOutFile, "</ul>\r\n</li>\r\n");
      }
    }

    if (!_iSourcesFound)
    {
      fprintf_s(_oOutFile, "None! No model sources found!\r\n");
    }
    else if (!_bZoneFound)
    {
      if (_iSourcesFound > 1)
      {
        fprintf_s(_oOutFile, "No zones found using this race.\r\n");
      }
    }

    fprintf_s(_oOutFile, "</ul>\r\n");
    fprintf_s(_oOutFile,
      "</span>\r\n");
  }

  fprintf_s(_oOutFile,
    "\r\n"
    "</span>\r\n");
  
  fprintf_s(_oOutFile,
    "\r\n"
    "</body>\r\n"
    "</html>\r\n");

  fclose(_oOutFile);

  return S_OK;
}

HRESULT ExportZoneInfo()
{
  FILE*       _oOutFile;
  char        _sFilename[_MAX_PATH];
  WORD        _iZoneID;
  EQZone*     _oZone;
  char*       _sGenderCode = "MFNX";

  bool        _bFound;
  DWORD       _i;
  DWORD       _j;
  char*       _sText1;
  char*       _sText2;
  char*       _sNotAvail = "N/A";
  char        _sBuffer[256];
  DWORD       _iMax;
  SourceID    _iSourceID;
  EQSource*   _oSource;
  EQModel*    _oModel;

  sprintf_s(_sFilename, sizeof(_sFilename), "%s_%s", EQ::Client.VersionNick, sOutFile_ZoneDB);

  if (fopen_s(&_oOutFile, MakePath(sOutPath, _sFilename), "wb"))
  {
    Log->Entry("FAILURE: Could not write zone information to output file: %s", _sFilename);

    return E_FAIL;
  }

  Log->Entry("  Listing EQ Zone Information in %s", _sFilename);
  
  fputs("ZoneID\tZoneNick\tZoneName\tStringID\tExpansion\tFlags\tMinLevel\tUnknown7\tUnknown8\r\n", _oOutFile);

  for (_iZoneID = 0; _iZoneID <= EQ::Client.Zone_MaxID; _iZoneID++)
  {
    _oZone = &EQ::Client.Zone[_iZoneID];
    
    if ((_oZone) && (_oZone->ID == _iZoneID))
    {
      fprintf_s(_oOutFile, "%d\t%s\t%s\t%d\t%d\t%d\t%d\t%d\t%d\r\n",
        _oZone->ID,
        _oZone->Nick,
        _oZone->Name,
        _oZone->DBStrNameID,
        _oZone->Expansion,
        _oZone->Flags,
        _oZone->MinLevel,
        _oZone->Unknown7,
        _oZone->Unknown8);
    }
  }

  fclose(_oOutFile);

  sprintf_s(_sFilename, sizeof(_sFilename), "%s_%s", EQ::Client.VersionNick, sOutFile_Zones);

  if (fopen_s(&_oOutFile, MakePath(sOutPath, _sFilename), "wb"))
  {
    Log->Entry("FAILURE: Could not write formatted zone information to output file: %s", _sFilename);

    return E_FAIL;
  }

  Log->Entry("  Listing formatted Zone information in %s", _sFilename);

  fprintf_s(_oOutFile,
    "<html>\r\n"
    "<head>\r\n"
    "<title>EverQuest Zone/Race Inventory</title>\r\n"
    "<style type=\"text/css\">\r\n"
    "<!--\r\n"
    "\r\n"
    "body { font-family:Calibri,Tahoma,sans-serif; cursor:default; margin:4px; }\r\n"
    "ul { margin-top:0; margin-bottom:8px; }\r\n"
    ".GenM { color:#4444ff; }\r\n"
    ".GenF { color:#ff6666; }\r\n"
    ".GenN { color:#339933; }\r\n"
    ".Global0 { color:#cc3333; }\r\n"
    ".Global1 { color:#999999; }\r\n"
    ".Global2 { color:#336699; }\r\n"
    ".Global3 { color:#3333cc; }\r\n"
    ".Global4 { color:#339933; }\r\n"
    "\r\n"
    "-->\r\n"
    "</style>\r\n"
    "<style type=\"text/css\">\r\n"
    "<!--\r\n"
    "\r\n"
    ".ScriptHide { display:none; }\r\n"
    ".ScriptShow { display:inline; }\r\n"
    ".FrameLeft { width:17%%; height:100%%; overflow:scroll; float:left; font-size:10pt; }\r\n"
    ".FrameRight { width:82%%; height:100%%; overflow:auto; float:right; padding-right:6px; }\r\n"
    "nobr a { cursor:pointer; }\r\n"
    "nobr a:hover { color:blue; }\r\n"
    "\r\n"
    "-->\r\n"
    "</style>\r\n"
    "<noscript>\r\n"
    "<style type=\"text/css\">\r\n"
    "<!--\r\n"
    "\r\n"
    ".ScriptHide { display:inline; }\r\n"
    ".ScriptShow { display:none; }\r\n"
    ".FrameRight { width:99%%; }\r\n"
    "\r\n"
    "-->\r\n"
    "</style>\r\n"
    "</noscript>\r\n"
    "<script language=\"javascript\">\r\n"
    "<!--\r\n"
    "\r\n"
    "  var iSaveZone = 0;\r\n"
    "  var iLastZone = 0;\r\n"
    "\r\n"
    "  function hideStyle(s)\r\n"
    "  {\r\n"
    "    if (document.styleSheets[1].rules)\r\n"
    "    {\r\n"
    "      document.styleSheets[1].rules[0].style.display=s;\r\n"
    "    }\r\n"
    "    else\r\n"
    "    {\r\n"
    "      document.styleSheets[1].cssRules[0].style.display=s;\r\n"
    "    }\r\n"
    "    \r\n"
    "    document.getElementById('HideShow').innerHTML = (s == 'none') ? 'Show All' : 'Show Single';\r\n"
    "  }\r\n"
    "  \r\n"
    "  function OverZone()\r\n"
    "  {\r\n"
    "    var el;\r\n"
    "    if ((el = window.event.srcElement).tagName == 'A')\r\n"
    "    {\r\n"
    "      el.style.color = 'blue';\r\n"
    "    }\r\n"
    "  }\r\n"
    "  \r\n"
    "  function OutZone()\r\n"
    "  {\r\n"
    "    var el;\r\n"
    "    if ((el = window.event.srcElement).tagName == 'A')\r\n"
    "    {\r\n"
    "      el.style.color = '';\r\n"
    "    }\r\n"
    "  }\r\n"
    "  \r\n"
    "  function ShowZone(e)\r\n"
    "  {\r\n"
    "    if (window.event) e = window.event;\r\n"
    "    var el = (e.srcElement) ? e.srcElement : e.target;\r\n"
    "    var i = el.innerHTML.indexOf('-');\r\n"
    "    if (el.tagName == 'A')\r\n"
    "    {\r\n"
    "      if (iLastZone >= 0)\r\n"
    "      {\r\n"
    "        document.getElementById('Zone' + iLastZone).style.display = '';\r\n"
    "      }\r\n"
    "\r\n"
    "      if (iLastZone == -1)\r\n"
    "      {\r\n"
    "        hideStyle('none');\r\n"
    "      }\r\n"
    "        \r\n"
    "      if (i == -1)\r\n"
    "      {\r\n"
    "        if (iLastZone == -1)\r\n"
    "        {\r\n"
    "          iLastZone = iSaveZone;\r\n"
    "          document.getElementById('Zone' + iLastZone).style.display = 'inline';\r\n"
    "        }\r\n"
    "        else\r\n"
    "        {\r\n"
    "          hideStyle('inline');\r\n"
    "          iLastZone = -1;\r\n"
    "        }\r\n"
    "      }\r\n"
    "      else\r\n"
    "      {\r\n"
    "        var iZone = el.innerHTML.substr(0, el.innerHTML.indexOf('-') - 1);\r\n"
    "        document.getElementById('Zone' + iZone).style.display = 'inline';\r\n"
    "        iLastZone = iZone;\r\n"
    "        iSaveZone = iLastZone;\r\n"
    "      }\r\n"
    "    }\r\n"
    "  }\r\n"
    "\r\n"
    "//-->\r\n"
    "</script>\r\n"
    "</head>\r\n"
    "<body>\r\n"
    "\r\n");

  fprintf_s(_oOutFile,
    "<span class=\"FrameLeft ScriptShow\">\r\n"
    "\r\n"
    "<nobr onclick=\"ShowZone(event);\" onmouseover=\"OverZone();\" onmouseout=\"OutZone();\">\r\n"
    "<p style=\"cursor:default\"><b>Choose Zone:</b></p>\r\n"
    "<b><a id=\"HideShow\">Show All</a></b><br />\r\n");

  BYTE _iMaxExpansion = 0;

  for (_iZoneID = 0; _iZoneID <= EQ::Client.Zone_MaxID; _iZoneID++)
  {
    if (EQ::Client.Zone[_iZoneID].Expansion > _iMaxExpansion)
    {
      _iMaxExpansion = EQ::Client.Zone[_iZoneID].Expansion;
    }
  }

  for (_i = 0; _i <= _iMaxExpansion; _i++)
  {
    if (IsBlank(_sText1 = EQ::GetExpansionName((BYTE)_i)))
    {
      sprintf_s(_sBuffer, sizeof(_sBuffer), "Expansion %d", _i);
      
      _sText1 = _sBuffer;
    }

    fprintf_s(_oOutFile,
      "<br />\r\n"
      "<b class=\"Global0\">%s</b><br />\r\n", _sText1);

    for (_iZoneID = 0; _iZoneID <= EQ::Client.Zone_MaxID; _iZoneID++)
    {
      if ((_i == 0) && (_iZoneID == 0))
      {
        fprintf_s(_oOutFile,
          "<a>0 - Globals</a><br />\r\n");
      }

      if (!(_oZone = EQ::Client.GetZone(_iZoneID)))
      {
        continue;
      }

      if ((_oZone->Expansion != _i) || (IsBlank(_oZone->Nick)))
      {
        continue;
      }

      fprintf_s(_oOutFile,
        "<a>%d - %s</a><br />\r\n", _iZoneID, _oZone->Nick);
    }
  }

  fprintf_s(_oOutFile,
    "</nobr>\r\n"
    "\r\n"
    "</span>\r\n"
    "<span class=\"FrameRight\">\r\n"
    "\r\n");

  fprintf_s(_oOutFile,
    "<span id=\"Zone0\" class=\"ScriptHide\" style=\"display:inline\">\r\n"
    "\r\n");

  fprintf_s(_oOutFile, "<hr>\r\n\r\n");

  fprintf_s(_oOutFile, "<h3>Globally Available Race Models</h3>\r\n\r\n");

  fprintf_s(_oOutFile, "<ul>\r\n");

  _iMax = EQ::GetGlobalSources();
  for (_j = 0; _j < _iMax; _j++)
  {
    _oSource = EQ::GetGlobalSource((BYTE)_j);

    if ((_oSource) && (_oSource->Models > 0))
    {
      fprintf_s(_oOutFile, "<li><b>Source:</b> %s (<span class=\"Global%d\">%s</span>)\r\n", _oSource->Filename, _oSource->Global, EQGlobalName[_oSource->Global]);
      fprintf_s(_oOutFile, "<ul>\r\n");

      for (_i = 0; _i < _oSource->Models; _i++)
      {
        _oModel = _oSource->Model[_i];
        //_oModel = EQ::Source_GetModelDef(_oSource->ID, (WORD)_i);

        if ((_oModel->RaceID == EQRACE_INVALID) || (_oModel->Gender == EQRG_INVALID))
        {
          fprintf_s(_oOutFile, "<li><b>%s</b> - Unrecognized Race Model</li>\r\n", _oModel->Code);
        }
        else
        {
          switch (_oModel->Gender)
          {
            case EQRG_MALE:
              _sText2 = " Male";
              break;
            case EQRG_FEMALE:
              _sText2 = " Female";
              break;
            default:
              _sText2 = "";
              break;
          }

          fprintf_s(_oOutFile, "<li><b class=\"Gen%c\">%s</b> - %s%s (Race %d, Gender %d)", _sGenderCode[_oModel->Gender - EQRG_MALE], _oModel->Code, IfBlank(EQ::Client.Race[_oModel->RaceID].GetName(), "Unknown"), _sText2, _oModel->RaceID, _oModel->Gender - EQRG_MALE);
          ExportModelVariations(_oOutFile, _oModel);
          fprintf_s(_oOutFile, "</li>\r\n");
        }

        _bFound = true;
      }

      fprintf_s(_oOutFile, "</ul>\r\n"
                           "</li>\r\n");
    }
  }

  if (!_bFound)
  {
    fprintf_s(_oOutFile, "<li>No global race models found. Rendering limited to local models.</li>\r\n");
  }

  fprintf_s(_oOutFile, "</ul>\r\n");
  fprintf_s(_oOutFile, "<br />\r\n");

  fprintf_s(_oOutFile,
    "</span>\r\n");

  for (_iZoneID = 0; _iZoneID <= EQ::Client.Zone_MaxID; _iZoneID++)
  {
    _oZone = &EQ::Client.Zone[_iZoneID];
    
    if (IsBlank(_oZone->Nick) && IsBlank(_oZone->Name))
    {
      continue;
    }

    fprintf_s(_oOutFile,
      "<span id=\"Zone%d\" class=\"ScriptHide\">\r\n"
      "\r\n", _iZoneID);

    fprintf_s(_oOutFile, "<hr>\r\n\r\n");

    fprintf_s(_oOutFile, "<h3>Zone # %d - %s - &quot;%s&quot;</h3>\r\n", _iZoneID, IfBlank(_oZone->Nick, _sNotAvail), IfBlank(_oZone->Name, _sNotAvail));

    if (_oZone->DBStrNameID)
    {
      if (_sText1 = EQ::StringList.GetText((WORD)_oZone->DBStrNameID))
      {
        if (!StringSame(_sText1, _oZone->Name))
        {
          fprintf_s(_oOutFile, "<p><b>Zone String Table Name:</b> %s</b>\r\n", _sText1);
        }
      }
    }

    fprintf_s(_oOutFile, "<p><b>Expansion:</b> %d - %s</b>\n", _oZone->Expansion, IfBlank(EQ::DBStrings.GetText(EQDB_EXPANSION, _oZone->Expansion), _sNotAvail));

    if (_oZone->MinLevel)
    {
      fprintf_s(_oOutFile, "<p><b>Minimum Level to Enter:</b> %d</p>\r\n", _oZone->MinLevel);
    }

    if (_oZone->Unknown7)
    {
      fprintf_s(_oOutFile, "<p><b>Unknown7:</b> %d</p>\r\n", _oZone->Unknown7);
    }
    
    if (_oZone->Unknown8)
    {
      fprintf_s(_oOutFile, "<p><b>Unknown8:</b> %d</p>\r\n", _oZone->Unknown8);
    }

    fprintf_s(_oOutFile,
      "\r\n"
      "<p><pre>        3         2         1         0\r\n"
      "       10987654321098765432109876543210\r\n");

    memcpy_s(_sBuffer, sizeof(_sBuffer), "<b>Flags:</b> ", 14);
    _sBuffer[46] = '\0';

    for (_i = 0; _i < 32; _i++)
    {
      _sBuffer[14 + _i] = (_oZone->Flags & (1 << (31 - _i))) ? '1' : '0';
    }

    fprintf_s(_oOutFile, _sBuffer);
    fprintf_s(_oOutFile, "</pre></p>\r\n");

    fprintf_s(_oOutFile,"<p><b>Non-Global Race Models Available:</b></p>\r\n"
                        "\r\n");

    _bFound = false;

    fprintf_s(_oOutFile, "<ul>\r\n");

    for (_j = 0; _j < 3; _j++)
    {
      switch (_j)
      {
        case 0:
          _oSource = _oZone->ModelSource1;
          break;
        case 1:
          _oSource = _oZone->ModelSource2;
          break;
        case 2:
          _oSource = NULL;
          break;
      }

      if (_oSource)
      {
        _iSourceID = _oSource->ID;
        fprintf_s(_oOutFile, "<li>Inside: <b class=\"Global0\">%s</b>\r\n", _oSource->Filename);
        fprintf_s(_oOutFile, "<ul>\r\n");

        for (_i = 0; _i < _oSource->Models; _i++)
        {
          
          _oModel = _oSource->Model[(WORD)_i];

          if ((_oModel->RaceID == EQRACE_INVALID) || (_oModel->Gender == EQRG_INVALID))
          {
            fprintf_s(_oOutFile, "<li><b>%s</b> - Unrecognized Race Model</li>\r\n", _oModel->Code);
          }
          else
          {
            switch (_oModel->Gender)
            {
              case EQRG_MALE:
                _sText2 = " Male";
                break;
              case EQRG_FEMALE:
                _sText2 = " Female";
                break;
              default:
                _sText2 = "";
                break;
            }

            fprintf_s(_oOutFile, "<li><b class=\"Gen%c\">%s</b> - %s%s (Race %d, Gender %d)", _sGenderCode[_oModel->Gender - EQRG_MALE], _oModel->Code, IfBlank(EQ::Client.Race[_oModel->RaceID].GetName(), "Unknown"), _sText2, _oModel->RaceID, _oModel->Gender - EQRG_MALE);
            ExportModelVariations(_oOutFile, _oModel);
            fprintf_s(_oOutFile, "</li>\r\n");
          }

          _bFound = true;
        }

        fprintf_s(_oOutFile, "</ul>\r\n"
                             "</li>\r\n");
      }
      else if (_j == 2)
      {
        // Imports

        if (_oZone->ImportModels > 0)
        {
          fprintf_s(_oOutFile, "<li>Imports via: <b class=\"Global2\">%s_chr.txt</b>\r\n", _oZone->Nick);
          fprintf_s(_oOutFile, "<ul>\r\n");

          for (_i = 0; _i < _oZone->ImportModels; _i++)
          {
            _oModel = _oZone->ImportModel[(BYTE)_i];
            _oSource = _oModel->Source;

            if ((_oModel->RaceID == EQRACE_INVALID) || (_oModel->Gender == EQRG_INVALID))
            {
              fprintf_s(_oOutFile, "<li><b>%s</b> (", _oModel->Code);
              if (_oModel->RaceID == EQRACE_INVALID)
              {
                fprintf_s(_oOutFile, "Race Unknown");
              }
              else
              {
                fprintf_s(_oOutFile, "Race %d", _oModel->RaceID);
              }

              if (_oModel->Gender == EQRG_INVALID)
              {
                fprintf_s(_oOutFile, "Gender Unknown");
              }
              else
              {
                fprintf_s(_oOutFile, "Gender %d", (_oModel->Gender - EQRG_MALE));
              }

              fprintf_s(_oOutFile, "), via %s\r\n", (_oSource) ? _oSource->Filename : "UnknownSource");
            }
            else
            {
              switch (_oModel->Gender)
              {
                case EQRG_MALE:
                  _sText2 = " Male";
                  break;
                case EQRG_FEMALE:
                  _sText2 = " Female";
                  break;
                default:
                  _sText2 = "";
                  break;
              }

              fprintf_s(_oOutFile, "<li><b class=\"Gen%c\">%s</b> - %s%s (Race %d, Gender %d), via <span class=\"Global3\">%s</span>", _sGenderCode[_oModel->Gender - EQRG_MALE], _oModel->Code, IfBlank(EQ::Client.Race[_oModel->RaceID].GetName(), "Unknown"), _sText2, _oModel->RaceID, _oModel->Gender - EQRG_MALE, (_oSource) ? _oSource->Filename : "UnknownSource");
            }

            ExportModelVariations(_oOutFile, _oModel);
            fprintf_s(_oOutFile, "</li>\r\n");

            _bFound = true;
          }

          fprintf_s(_oOutFile, "</ul>\r\n"
                               "</li>\r\n");
        }
      }
    }

    if (!_bFound)
    {
      fprintf_s(_oOutFile, "<li>No in-zone or imported race models found. Rendering limited to global models.</li>\r\n");
    }

    fprintf_s(_oOutFile, "</ul>\r\n");
    fprintf_s(_oOutFile, "<br />\r\n");

    fprintf_s(_oOutFile,
      "</span>\r\n");
  }

  fprintf_s(_oOutFile,
    "\r\n"
    "</span>\r\n");

  fprintf_s(_oOutFile,
    "\r\n\r\n</body>\r\n"
    "</html>\r\n");

  fclose(_oOutFile);
  
  return S_OK;
}

HRESULT ExportZoneRaceAvailability()
{
  return S_OK;

  /*
  
  FILE* _oOutFile;
  char  _sOutFile[_MAX_PATH];

  WORD  _iZoneID;
  WORD  _iRaceModelsInZone;
  WORD  _i;
  WORD  _iRaceID;
  BYTE  _iGender;
  char*  _sModelCode;

  sprintf_s(_sOutFile, sizeof(_sOutFile), "%s_%s", EQ::Client.VersionNick, sOutFile_AvailD);

  if (fopen_s(&_oOutFile, _sOutFile, "wb"))
  {
    Log->Entry("FAILURE: Problem opening Race Availability output file: %s", _sOutFile);
    
    return E_FAIL;
  }

  fprintf_s(_oOutFile, "ZoneID^ModelCode^RaceID^GenderID^Description\r\n");
  
  for (_iZoneID = 0; _iZoneID < EQZONE_MAXID; _iZoneID++)
  {
    _iRaceModelsInZone = EQ::Zone_GetRaceModelCount(_iZoneID);
    
    for (_i = 0; _i < _iRaceModelsInZone; _i++)
    {
      _sModelCode = EQ::Zone_GetRaceModel(_iZoneID, _i);

      if (SUCCEEDED(EQ::Client.GetRaceFromModelCode(_sModelCode, &_iRaceID, &_iGender)))
      {
        fprintf_s(_oOutFile, "%d^%s^%d^%d^%s\r\n", _iZoneID, _sModelCode, _iRaceID, _iGender, "");
      }
    }
  }

  WORD _iOrphanRaces;
  char* _sOrphanRaceCode;
  char* _sOrphanRaceFile;

  _iOrphanRaces = EQ::GetOrphanSources();
  for (_i = 0; _i < _iOrphanRaces; _i++)
  {
    _sOrphanRaceCode = EQ::Model_GetOrphanCode(_i);
    _sOrphanRaceFile = EQ::Model_GetOrphanFile(_i);

    if ((_sOrphanRaceCode) && (_sOrphanRaceFile))
    {
      fprintf_s(_oOutFile, "0^%s^0^0^Orphan From %s\r\n", _sOrphanRaceCode, _sOrphanRaceFile);
    }
  }

  fclose(_oOutFile);
  
  return S_OK;
  */
}

HRESULT LogOpen()
{
  char _sLogFilename[_MAX_PATH];

  sprintf_s(_sLogFilename, sizeof(_sLogFilename), "%s_%s", EQ::Client.VersionNick, sLogFile);

  Log = new LogFile();

  Log->SetEcho(ECHO);

  if (FAILED(Log->Open(MakePath(sOutPath, _sLogFilename), "EverQuest Race Information Inventory v1.0")))
  {
    printf("FAILURE: Could not create log file: %s\n", _sLogFilename);

    return E_FAIL;
  }

  Log->Entry(QUIET, "EQGame.exe recognized as the %s version.", EQ::Client.VersionNick);

  return S_OK;
}

void ProcessedFile(DWORD FileSize)
{
  iProcessedFiles++;
  iProcessedBytes += FileSize;
}

void ProcessedFile(char* FilePath)
{
  if (FilePath)
  {
    struct _stat32 _oStats;

    if (!_stat32(FilePath, &_oStats))
    {
      ProcessedFile(_oStats.st_size);
    }
  }
}

HRESULT LoadZoneModels(EQZone* Zone)
{
  if (!Zone)
  {
    return E_INVALIDARG;
  }

  if (IsBlank(Zone->Nick))
  {
    return E_FAIL;
  }

  char  _sPath[_MAX_PATH];

  sprintf_s(_sPath, sizeof(_sPath), "%s_chr.s3d", Zone->Nick);
  if (FileExists(_sPath))
  {
    Zone->ModelSource1 = EQ::AddSource(_sPath, EQSA_LOCAL, false);

    if (Zone->ModelSource1 != EQSOURCE_INVALID)
    {
      Log->Entry(QUIET, "  Found Zone Model File: %s", _sPath);
      
      LoadModels_S3D(Zone->ModelSource1);

      ProcessedFile(_sPath);
    }
  }

  sprintf_s(_sPath, sizeof(_sPath), "%s_chr2.s3d", Zone->Nick);
  if (FileExists(_sPath))
  {
    Zone->ModelSource2 = EQ::AddSource(_sPath, EQSA_LOCAL, false);

    if (Zone->ModelSource2 != EQSOURCE_INVALID)
    {
      Log->Entry(QUIET, "  Found Zone Model File: %s", _sPath);
      
      LoadModels_S3D(Zone->ModelSource2);

      ProcessedFile(_sPath);
    }
  }

  LoadZoneModels_TXT(Zone);
  
  return S_OK;
}

HRESULT LoadModels_S3D(EQSource* Source)
{
  if (!Source)
  {
    return E_INVALIDARG;
  }

  SourceID _iSourceID = Source->ID;

  // - If Source->Loaded, return S_OK
  if (Source->Loaded)
  {
    // Source file already processed.
    
    return S_OK;
  }
  
  S3DPackage*   _oS3D;

  _oS3D = new S3DPackage();

  if (!_oS3D)
  {
    return E_OUTOFMEMORY;
  }

  // - Open S3D
  if (FAILED(_oS3D->Open(Source->Filename)))
  {
    delete _oS3D;

    return E_FAIL;
  }

  char          _sWLDFilename[EQ_MAXFILENAMELEN + 1];
  DWORD         _iFile;
  S3DFileInfo*  _oFile;
  WLDFile*      _oWLD;
  BYTE*         _aFileBytes = NULL;

  // - Get Model Definitions
  //   - Find & Open WLD
  memcpy_s(_sWLDFilename, sizeof(_sWLDFilename), Source->Filename, Source->FilenameLen + 1);
  memcpy_s(&_sWLDFilename[Source->FilenameLen - 3], sizeof(_sWLDFilename), "wld", 3);
  //sprintf_s(_sWLDFilename, sizeof(_sWLDFilename), "%s.wld", Source->Filename);

  _iFile = _oS3D->FindFile(_sWLDFilename);

  if (_iFile == S3D_INVALID)
  {
    delete _oS3D;

    return E_FAIL;
  }

  _oFile = _oS3D->GetFileInfo(_iFile);
  
  _oWLD = new WLDFile();

  if (!_oWLD)
  {
    delete _oS3D;
    
    return E_OUTOFMEMORY;
  }

  _aFileBytes = _oS3D->LoadFile(_iFile);
  if (!_aFileBytes)
  {
    delete _oWLD;
    delete _oS3D;
    
    return E_OUTOFMEMORY;
  }

  if (FAILED(_oWLD->LoadFromMemory(_aFileBytes, _oFile->Size, true, _oFile->Filename, false)))
  {
    delete _aFileBytes;
    delete _oWLD;
    delete _oS3D;
  }

  // Load model definitions and variations
  size_t        _iFragNameLen;
  size_t        _iFragNameLen2;
  char          _sFragName[64];
  _sFragName[0] = NULL;

  char          _sModelCode[64];
  _sModelCode[0] = NULL;

  BYTE          _iMeshes;
  WLDFragment*  _oMeshFrags[10];
  WLDFragment*  _oMeshFrag;
  char*         _sText1;
  WLDFragment*  _oFrag;
  WLDFragment*  _oRefFrag;
  EQModel*      _oModel;
  
  BYTE          _iVarType;
  BYTE          _iVarNum;

  char _sFragPrefix[20]; // Used for head meshes, '[ModelCode]HE', eg. GIAHE00_DMSPRITEDEF
  char _sFragSuffix[13]; // Used for both meshes, '_DMSPRITEDEF',  eg. WOLMESH_DMSPRITEDEF

  char _sFragPrefix2[32];
  char _sFragSuffix2[10];

  DWORD         _i;
  DWORD         _j;

  WLDFragment*  _oTextureRef;
  DWORD         _iTextureRef = 0;
  DWORD         _iTextureRefs = 0;
  WLDFragment*  _oTextureList;
  DWORD         _iTextureList = 0;

  WLDFragment*  _oBodyMesh;

  bool          _bTemp = false;

  memcpy_s(_sFragSuffix, sizeof(_sFragSuffix), "_DMSPRITEDEF\0", 13);

  // Load our Model Definitions
  _oFrag = NULL;
  while (_oFrag = _oWLD->FindNextFragment(0x14, _oFrag))
  {
    if (!IsBlank(_oFrag->Name))
    {
      if ((_iFragNameLen = strlen(_oFrag->Name)) > 63)
      {
        // This should never happen. It's just going to be [ModelCode]_ACTORDEF !

        BreakPoint;
        
        continue;
      }

      memcpy_s(_sFragName, sizeof(_sFragName), _oFrag->Name, _iFragNameLen + 1);
      if (!(_sText1 = (char*)memchr(_sFragName, '_', _iFragNameLen)))
      {
        // Not a validly named fragment.
        BreakPoint;
        
        continue;
      }

      _sText1[0] = NULL; // _sFragName now contains our model code.
      _iFragNameLen = (_sText1 - _sFragName);
      
      TrimString(_sFragName, &_iFragNameLen);
      if (_iFragNameLen > EQMODEL_MAXCODELEN)
      {
        // Invalid model code... too long!
        BreakPoint;

        continue;
      }

      _strupr_s(_sFragName, _iFragNameLen + 1);

      memcpy_s(_sModelCode, sizeof(_sModelCode), _sFragName, _iFragNameLen + 1);
      if (!(_oModel = Source->AddModel(_sModelCode)))
      {
        // Problem adding model code to source!
        BreakPoint;

        continue;
      }
      else
      {
        Log->Entry(QUIET, "     Found model %s in source %s", _sModelCode, Source->Filename);
      }

      sprintf_s(_sFragPrefix, sizeof(_sFragPrefix), "%sHE", _sModelCode);

      // Find head (if exists) and main mesh models
      if (!(_oRefFrag = _oFrag->Frag14_GetMeshFrag()))
      {
        // No mesh reference fragment found
        BreakPoint;
        
        continue;
      }

      switch (_oRefFrag->FragType)
      {
        case 0x07: // 2D Sprite, no body or head
          _iMeshes = 0;
          break;
        case 0x11: // Animated mesh, body and possibly a head
          _oRefFrag = _oWLD->GetFragment(*(DWORD*)&_oRefFrag->FragPointer[0]);
          if ((!_oRefFrag) || (_oRefFrag->FragType != 0x10))
          {
            _iMeshes = 0;
          }
          else
          {
            _oRefFrag->Frag10_GetMeshFrags(&_iMeshes, _oMeshFrags, sizeof(_oMeshFrags) / sizeof(WLDFragment*));
          }
          break;
        case 0x2D: // Static mesh, body but no head
          _oMeshFrags[0] = _oWLD->GetFragment(*(DWORD*)&_oRefFrag->FragPointer[0]);
          _iMeshes = (_oMeshFrags[0]) ? 1 : 0;
          break;
        default:
          _iMeshes = 0;
          break;
      }

      if (_iMeshes == 1)
      {
        _oBodyMesh = _oMeshFrags[0];
      }
      else
      {
        // Find the largest mesh, assuming that's the main body mesh.

        _oBodyMesh = NULL;

        for (_i = 0; _i < _iMeshes; _i++)
        {
          _oRefFrag = _oMeshFrags[_i];
          if ((_oRefFrag) && ((!_oBodyMesh) || (_oRefFrag->FragSize > _oBodyMesh->FragSize)))
          {
            if (!(LeftMatch(_oRefFrag->Name, _sFragPrefix) && RightMatch(_oRefFrag->Name, _sFragSuffix)))
            {
              _oBodyMesh = _oMeshFrags[_i];
            }
          }
        }
      }
      
      // Check the meshes associated with this model.
      for (_i = 0; _i < _iMeshes; _i++)
      {
        _oMeshFrag = _oMeshFrags[_i];
        
        // NOTE: I used to treat head meshes as exclusive of body meshes, but this doesn't work for models
        //       where the head mesh is used FOR the body mesh (e.g., FIS - Race 24). We have to check both.
        if (LeftMatch(_oMeshFrag->Name, _sFragPrefix) && RightMatch(_oMeshFrag->Name, _sFragSuffix))
        {
          // Head Mesh!
          if ((_oMeshFrag->Name[_oModel->CodeLen + 2] == '0') &&
              (_oMeshFrag->Name[_oModel->CodeLen + 3] == '0'))
          {
            // Skinnable! Check the file for other heads for this model
            
            _oRefFrag = NULL;
            while (_oRefFrag = _oWLD->FindNextFragment(0x36, _oRefFrag))
            {
              if (LeftMatch(_oRefFrag->Name, _sFragPrefix) && RightMatch(_oRefFrag->Name, _sFragSuffix))
              {
                // A head mesh for our model!

                // UPDATE: Make sure this head mesh isn't just a normal mesh used in our regular model
                //         and just happens to be named as if it were another head. (Fairies and Pixies
                //         are an example of this. HE00 is their head, and HE01 is their wings. Dumb!)
                
                for (_j = 0; _j < _iMeshes; _j++)
                {
                  if (_oRefFrag == _oMeshFrags[_j])
                  {
                    break;
                  }
                }

                if (_j >= _iMeshes)
                {
                  // We've verified that this is an actual new head mesh.

                  if (IsNumeric(_oRefFrag->Name[_oModel->CodeLen + 2]) &&
                      IsNumeric(_oRefFrag->Name[_oModel->CodeLen + 3]))
                  {
                    // [ModelCode]HE##_DMSPRITEDEF

                    _iVarNum = (_oRefFrag->Name[_oModel->CodeLen + 2] - '0') * 10 +
                               (_oRefFrag->Name[_oModel->CodeLen + 3] - '0');

                    // Skipping Variation 00. That's not really a variation.
                    if (_iVarNum > 0)
                    {
                      if (FAILED(_oModel->AddVariation(EQRV_HEAD, _iVarNum)))
                      {
                        // Problem adding variation. Too many?
                        BreakPoint;

                        continue;
                      }
                    }
                  }
                }
              }
            }
          }
        }
        
        if (_oMeshFrag == _oBodyMesh)
        {
          // COPOUT: I don't know how SOE codes the helm variations for playable races, so I'll just have to hardcode them in here.
          if (EQ::IsPlayableRace(_oModel->RaceID))
          {
            _oModel->AddVariations(EQRV_HEAD, 3);

            if (Source->Global == EQSA_GLOBAL_LUCLIN)
            {
              // COPOUT: I don't know how SOE codes the mesh-based hair, beard, etc. information, except that it has to do with
              //         the IT*.dds files in lgequip.s3d and lgequip2.s3d. Parsing the WLD files didn't help much.  I would
              //         need some sort of map that tells me which IT files are associated with each race/gender, and I don't
              //         know where to find that. Heck, it may be hard-coded into the client for all I know.

              switch ((_oModel->RaceID << 4) | (_oModel->Gender - EQRG_MALE + 1))
              {
                case 0x0011: // Human Male
                case 0x0021: // Barbarian Male
                case 0x0081: // Dwarf Male
                case 0x00B1: // Halfling Male
                case 0x00C1: // Gnome Male
                  _oModel->AddVariations(EQRV_HAIR, 3);
                  _oModel->AddVariations(EQRV_BEARD, 5);
                  break;
                case 0x0012: // Human Female
                case 0x0022: // Barbarian Female
                case 0x0042: // Wood Elf Female
                case 0x0052: // High Elf Female
                case 0x0062: // Dark Elf Female
                case 0x0072: // Half Elf Female
                  _oModel->AddVariations(EQRV_HAIR, 3);
                  break;
                case 0x0031: // Erudite Male
                  _oModel->AddVariations(EQRV_BEARD, 5);
                  _oModel->AddVariation(EQRV_HEAD, 4);
                  break;
                case 0x0032: // Erudite Female
                  _oModel->AddVariation(EQRV_HEAD, 4);
                  break;
                case 0x0091: // Troll Male
                case 0x00A1: // Ogre Male
                case 0x0801: // Iksar Male
                case 0x0802: // Iksar Female
                case 0x0821: // Vah Shir Male
                case 0x0822: // Vah Shir Female
                case 0x14A1: // Froglok Male
                case 0x14A2: // Froglok Female
                  break;
                case 0x0041: // Wood Elf Male
                case 0x0051: // High Elf Male
                case 0x0061: // Dark Elf Male
                case 0x0071: // Half Elf Male
                case 0x0082: // Dwarf Female
                case 0x0092: // Troll Female
                case 0x00A2: // Ogre Female
                case 0x00B2: // Halfling Female
                case 0x00C2: // Gnome Female
                  _oModel->AddVariations(EQRV_HAIR, 3);
                  break;
                case 0x2281: // Drakkin Male
                case 0x2282: // Drakkin Female
                  // Drakkin models are EQG. Everything is extrapolated from the EQG textures except heritage.
                  //
                  // We don't need to add a whole new variation thing for one race's unique property like
                  // heritage. We'll just hard-code a 00-07 fprintf_s in ExportVariations()
                  break;
              }
            }
          }
          
          // Found the model's body mesh reference, usually (but not always) [ModelCode]_DMSPRITEDEF
          // Get Textures used in the model
          if (!(_oTextureList = _oWLD->GetFragment(*(DWORD*)&_oMeshFrag->FragPointer[4])))
          {
            // Failure: Could not find the TextureList fragment being referenced!
            BreakPoint;

            continue;
          }

          _iTextureRefs = *(DWORD*)&_oTextureList->FragPointer[4];
          for (_iTextureRef = 0; _iTextureRef < _iTextureRefs; _iTextureRef++)
          {
            if (!(_oTextureRef = _oWLD->GetFragment(*(DWORD*)&_oTextureList->FragPointer[(_iTextureRef * 4) + 8])))
            {
              // Failure: Could not find texture being referenced
              BreakPoint;

              continue;
            }

            // Find alternates based on the texture reference fragment's name.
            // First, make sure the fragment follows the pattern *00##_MDF
            _iFragNameLen = strlen(_oTextureRef->Name);
            if (!RightMatch(_oTextureRef->Name, "_MDF", _iFragNameLen, 4))
            {
              // SKIPPED: Improperly named fragment
              continue;
            }

            if ((_oTextureRef->Name[_iFragNameLen - 8] != '0') ||
                (_oTextureRef->Name[_iFragNameLen - 7] != '0') ||
                !IsNumeric(_oTextureRef->Name[_iFragNameLen - 6]) ||
                !IsNumeric(_oTextureRef->Name[_iFragNameLen - 5]))
            {
              // SKIPPED: Properly named, but not skinnable (e.g., GNOMSH4_MDF, WOLCH0301_MDF)
              
              // EXCEPTION: SOE does some funky processing for the non-Luclin player-race robes
              //            CLK##xx_MDF refer to texture numbers ##+6, so CLK0401 is XXX1001, CLK1006 is XXX1606, etc.
              
              // We'll just process this once, for CLK0401, not for the rest of the CLK04xx textures
              if (!StringSame(_oTextureRef->Name, _oTextureRef->NameLen, "CLK0401_MDF", 11, false))
              {
                continue;
              }
            }
            
            // Example: If BEACH0001_MDF, look for BEACH##01_MDF
            //          Prefix = BEACH, Suffix = 01_MDF

            memcpy_s(_sFragPrefix2, sizeof(_sFragPrefix2), _oTextureRef->Name, _iFragNameLen - 8);
            _sFragPrefix2[_iFragNameLen - 8] = 0x00; // _sFragPrefix = (example) BEACH

            memcpy_s(_sFragSuffix2, sizeof(_sFragSuffix2), &_oTextureRef->Name[_iFragNameLen - 6], 6);
            _sFragSuffix2[6] = 0x00; // _sFragSuffix = (example) 01_MDF

            _oTextureRef = NULL;
            // Iterate through all of the texture fragments in the WLD file, looking for alternates.
            while (_oTextureRef = _oWLD->FindNextFragment(0x30, _oTextureRef))
            {
              _iVarType = EQRV_NONE;
              _iFragNameLen2 = strlen(_oTextureRef->Name);
              
              if (LeftMatch(_oTextureRef->Name, _sFragPrefix2, _iFragNameLen2, _iFragNameLen - 8))
              {
                if ((_iFragNameLen2 == 13) &&
                    (((*(WORD*)&_oTextureRef->Name[3]) & 0xDFDF) == 0x4548) && // "HE"
                    (EQ::IsPlayableRace(_oModel->RaceID)))
                {
                  if (IsNumeric(_oTextureRef->Name[5]) && IsNumeric(_oTextureRef->Name[6]))
                  {
                    // Playable race, extracting beard & face number from texture name (e.g., HUMHE0121_MDF, )

                    if (_oTextureRef->Frag30_GetFileCount() > 1)
                    {
                      if (IsNumeric(_oTextureRef->Name[6]))
                      {
                        switch (_oModel->RaceID)
                        {
                          case 2: // Barbarians
                            _iVarType = EQRV_TATTOO;
                            break;
                          case 3: // Erudites
                            _iVarType = EQRV_HAIR;
                            break;
                          default: // Face-based beard races
                            _iVarType = EQRV_BEARD;
                            break;
                        }
                        
                        _oModel->AddVariation(_iVarType, _oTextureRef->Name[6] - '0');
                        
                        _iVarType = EQRV_NONE;
                      }
                    }

                    if (IsNumeric(_oTextureRef->Name[7]))
                    {
                      _iVarType = EQRV_FACE;
                      _iVarNum = (_oTextureRef->Name[7] - '0');
                    }
                  }
                }
                else if (RightMatch(_oTextureRef->Name, _sFragSuffix2, _iFragNameLen2, 6))
                {
                  // Got a match! Check and extract the variation number.
                  
                  if (IsNumeric(_oTextureRef->Name[_iFragNameLen - 8]) &&
                      IsNumeric(_oTextureRef->Name[_iFragNameLen - 7]))
                  {
                    // BEACH####_MDF
                    _iVarNum = (_oTextureRef->Name[_iFragNameLen - 8] - '0') * 10 +
                               (_oTextureRef->Name[_iFragNameLen - 7] - '0');

                    // Skipping Variation 00. That's not really a variation.
                    if (_iVarNum > 0)
                    {
                      // Add in the exception for non-Luclin player-race cloaks
                      if (LeftMatch(_oTextureRef->Name, "CLK", _iFragNameLen2, 3) &&
                          RightMatch(_oTextureRef->Name, "01_MDF", _iFragNameLen2, 6))
                      {
                        _iVarNum += 6;
                      }

                      _iVarType = EQRV_TEXTURE;
                    }
                  }
                }
              }

              if (_iVarType != EQRV_NONE)
              {
                if (FAILED(_oModel->AddVariation(_iVarType, _iVarNum)))
                {
                  // Problem adding variation. Too many?
                  BreakPoint;

                  continue;
                }
              }

            }
          }
        }
      }
    }
  }

  Source->Loaded = 1;

  ProcessedFile(_oFile->Size);

  delete _aFileBytes;
  delete _oWLD;
  delete _oS3D;

  return S_OK;
}

HRESULT LoadZoneModels_TXT(EQZone* Zone)
{
  /*
  
  Grab code from ProcessTXT()

  - Open TXT
  - Load number of expected entries
  - Iterate through entries
    - XXX, SourceFile
    - If XXX != Valid ModelCode, log error
    - If SourceFile.eqg...
      - Get/Create ModelSource for SourceFile.eqg
      - LoadModels_EQG(SourceID)
      - If Model In Source, add ModelDef Index to Zone ImportDefs
      - If Model Not In Source, log error
    - If SourceFile.s3d...
      - Get/Create ModelSource for SourceFile.s3d
      - LoadModels_S3D(SourceID)
      - If Model In Source, add ModelDef Index to Zone ImportDefs
      - If Model Not In Source, log error
    - If No SourceFile, log error

  */

  if (!Zone)
  {
    return E_INVALIDARG;
  }
  
  char _sTXTFile[_MAX_PATH];

  sprintf_s(_sTXTFile, sizeof(_sTXTFile), "%s_chr.txt", Zone->Nick);
  if (!FileExists(_sTXTFile))
  {
    // No ZoneNick_chr.txt file. That's perfectly okay. Just skip it for this zone.
    
    return S_OK;
  }
  
  DWORD _iFilenameLen = strlen(_sTXTFile);

  if (_iFilenameLen > EQ_MAXFILENAMELEN)
  {
    Log->Entry("  SKIPPED: _sTXTFile '%s' is too long. Max allowed is %d characters!", _sTXTFile, (void*)EQ_MAXFILENAMELEN);

    return E_FAIL;
  }

  SourceID    _iSourceID;
  EQSource*   _oSource = NULL;
  DWORD       _iModelCount;
  DWORD       _iModelsFound;
  WORD        _iRaceID;
  EQGENDER    _iGender;
  EQModel*    _oModel;
  char        _sLine[1024];
  char        _sFullFilename[_MAX_PATH];
  bool        _bEOF = false;
  _sLine[0] = 0x00;

  char* _sModelCode;
  char* _sModelSource;

  FILE* _oFile;

  if (fopen_s(&_oFile, _sTXTFile, "rb"))
  {
    Log->Entry("  FAILURE: Problem opening TXT file %s for model import parsing.", _sTXTFile);
    
    return E_FAIL;
  }

  Log->Entry(QUIET, "  Found Zone Model Import File: %s", _sTXTFile);

  while((!_bEOF) && (_sLine[0] == 0x00))
  {
    if (!fgets(_sLine, sizeof(_sLine), _oFile))
    {
      _bEOF = true;
    }
    else
    {
      TrimString(_sLine);
    }
  }

  if (!_bEOF)
  {
    if (!(_iModelCount = atoi(_sLine)))
    {
      Log->Entry("  SKIPPED: Unable to parse %s for model imports", _sTXTFile);

      fclose(_oFile);

      return E_FAIL;
    }
    else
    {
      Log->Entry(QUIET, "   Looking for %d model code/source entries in %s", (void*)_iModelCount, _sTXTFile);
      _iModelsFound = 0;
    }
  }

  while (!_bEOF)
  {
    _sLine[0] = 0x00;
    while ((!_bEOF) && (_sLine[0] == 0x00))
    {
      if (!fgets(_sLine, sizeof(_sLine), _oFile))
      {
        _bEOF = true;
      }
      else
      {
        TrimString(_sLine);
      }
    }

    if (!_bEOF)
    {
      _sModelSource = strchr(_sLine, ',');

      if (_sModelSource)
      {
        // ModelCode,ModelSource

        _sModelSource[0] = 0x00;
        _sModelSource++;
        _sModelCode = _sLine;

        TrimString(_sModelCode);
        TrimString(_sModelSource);

        _strlwr_s(_sModelSource, strlen(_sModelSource) + 1);
        _strupr_s(_sModelCode, strlen(_sModelCode) + 1);

        if (FAILED(EQ::Client.GetRaceFromModelCode(_sModelCode, &_iRaceID, &_iGender)))
        {
          Log->Entry("   SKIPPED: No race found to match model code %s in %s", _sModelCode, _sTXTFile);

          //EQ::Model_AddOrphan(_sModelCode, _sTXTFile);
        }
        else
        {
          //if (VerifyRaceModelIn(_sModelCode, _sModelSource, _sFullFilename))
          if (strchr(_sModelSource, '.') == NULL)
          {
            sprintf_s(_sFullFilename, sizeof(_sFullFilename), "%s.eqg", _sModelSource);

            if (FileExists(_sFullFilename))
            {
              _sModelSource = _sFullFilename;
            }
            else
            {
              sprintf_s(_sFullFilename, sizeof(_sFullFilename), "%s.s3d", _sModelSource);

              if (FileExists(_sFullFilename))
              {
                _sModelSource = _sFullFilename;
              }
              else
              {
                strcpy_s(_sFullFilename, sizeof(_sFullFilename), _sModelSource);

                _sModelSource = NULL;
              }
            }
          }

          if (_sModelSource == NULL)
          {
            Log->Entry("   SKIPPED: Model %s in %s, nonexistent %s.*", _sModelCode, _sTXTFile, _sFullFilename);
          }
          else
          {
            //Log->Entry(QUIET, "   Attempting to import %s from %s...", _sModelCode, _sFullFilename);

            if ((_oSource = EQ::GetSource(_sModelSource)) == NULL)
            {
              if (!(_oSource = EQ::AddSource(_sModelSource, EQSA_LOCAL, false)))
              {
                _iSourceID = EQSOURCE_INVALID;
              }
              else
              {
                _iSourceID = _oSource->ID;
              }
              

              if (_iSourceID == EQSOURCE_INVALID)
              {
                Log->Entry("      SKIPPED: Could not add model source %s from %s", _sModelSource, _sTXTFile);
              }
              else
              {
                if (RightMatch(_sModelSource, ".eqg"))
                {
                  if (FAILED(LoadModels_EQG(_oSource)))
                  {
                    Log->Entry("      SKIPPED: Problem loading models from %s in %s", _sModelSource, _sTXTFile);
                  }
                  else
                  {
                    Log->Entry(QUIET, "      Loaded models from %s in %s", _sModelSource, _sTXTFile);

                    ProcessedFile(_sModelSource);
                  }
                }
                else if (RightMatch(_sModelSource, ".s3d"))
                {
                  if (FAILED(LoadModels_S3D(_oSource)))
                  {
                    Log->Entry("   SKIPPED: Problem loading models from %s in %s", _sModelSource, _sTXTFile);
                  }
                  else
                  {
                    Log->Entry(QUIET, "      Loaded models from %s in %s", _sModelSource, _sTXTFile);

                    ProcessedFile(_sModelSource);
                  }
                }
                else
                {
                  Log->Entry("   SKIPPED: Unknown model source type %s in %s", _sModelSource, _sTXTFile);
                }
              }
            }

            if (_iSourceID != EQSOURCE_INVALID)
            {
              if (!_oSource->HasModel(_iRaceID, _iGender, &_oModel))
              {
                Log->Entry("   SKIPPED: Model %s in %s, not in %s", _sModelCode, _sTXTFile, _sFullFilename);
              }
              else
              {
                Zone->AddImportModel(_oModel);
                //EQ::Zone_AddModel(_iRaceID, _iGender, Zone->ID, EQ::AddSource(_sFullFilename, false));
                
                Log->Entry(QUIET, "      Imported Model %s (Race %d, Gender %d) to Zone %s", _sModelCode, (void*)_oModel->RaceID, (void*)_oModel->Gender, Zone->Nick);
              }
            }
          }
        }

        _iModelsFound++;
      }
    }
  }

  if (_iModelsFound != _iModelCount)
  {
    Log->Entry("   ODDITY: Expected %d model references in %s, found %d", (void*)_iModelCount, _sTXTFile, (void*)_iModelsFound);
  }

  fclose(_oFile);

  return S_OK;
}

HRESULT LoadModels_EQG(EQSource* Source)
{
  if (!Source)
  {
    return E_INVALIDARG;
  }

  SourceID _iSourceID = Source->ID;
  
  // - If Source->Loaded, we're done
  if (Source->Loaded)
  {
    // Source file already processed.
    
    return S_OK;
  }
  
  S3DPackage*   _oS3D;

  _oS3D = new S3DPackage();

  if (!_oS3D)
  {
    return E_OUTOFMEMORY;
  }

  // - Open S3D
  if (FAILED(_oS3D->Open(Source->Filename)))
  {
    delete _oS3D;

    return E_FAIL;
  }

  DWORD         _iFiles;
  DWORD         _iModFile;
  S3DFileInfo*  _oModFile;
  char*         _sModelCode = NULL;

  BYTE          _iVarType = 0;
  BYTE          _iVarNum = 0;

  WORD          _iRaceID;
  EQGENDER      _iGender;

  EQModel*      _oModel;

  DWORD         _iVarFile;
  char*         _sVarFile;
  S3DFileInfo*  _oVarFile;

  WORD          _iVarParts;
  char          _sVarBuffer[100];
  char*         _sVarPart[10];
  size_t        _iPartLen[10];
  char*         _sFileExt;

  _iFiles = _oS3D->GetFileCount();
  
  char _sModelCodeBuf[EQMODEL_MAXCODELEN + 1];
  size_t _iModelCodeLen;

  for (_iModFile = 0; _iModFile < _iFiles; _iModFile++)
  {
    _oModFile = _oS3D->GetFileInfo(_iModFile);

    if (_oModFile->FilenameLength <= (EQMODEL_MAXCODELEN + 4))
    {
      if (RightMatch(_oModFile->Filename, ".mod") || RightMatch(_oModFile->Filename, ".mds"))
      {
        _iModelCodeLen = _oModFile->FilenameLength - 4;
        memcpy_s(_sModelCodeBuf, sizeof(_sModelCodeBuf), _oModFile->Filename, _iModelCodeLen);

        _sModelCodeBuf[_iModelCodeLen] = NULL;
        TrimString(_sModelCodeBuf, &_iModelCodeLen);
        _strupr_s(_sModelCodeBuf, _iModelCodeLen + 1);

        if (SUCCEEDED(EQ::Client.GetRaceFromModelCode(_sModelCodeBuf, &_iRaceID, &_iGender)))
        {
          if (!(_oModel = Source->AddModel(_sModelCodeBuf)))
          {
            Log->Entry("     FAILURE: Could not add model %s from source %s", _sModelCodeBuf, Source->Filename);

            continue;
          }

          Log->Entry(QUIET, "     Found model %s in source %s", _sModelCodeBuf, Source->Filename);

          // Look for variations for this EQG model

          // COPOUT: I don't know how SOE codes the helm variations for playable races, so I'll just have to hardcode them in here.
          if (EQ::IsPlayableRace(_oModel->RaceID))
          {
            _oModel->AddVariation(EQRV_HEAD, 1);
            _oModel->AddVariation(EQRV_HEAD, 2);
            _oModel->AddVariation(EQRV_HEAD, 3);
          }
          
          // Because I don't have file format specs for MOD and MDS files, I'll have to
          // extrapolate variation information from the filenames accompanying the model.
          //
          // As a result, information may be less accurate than the WLD-parsing method
          // used for S3D model variations.
          for (_iVarFile = 0; _iVarFile < _oS3D->GetFileCount(); _iVarFile++)
          {
            _iVarType = EQRV_NONE;
            _oVarFile = _oS3D->GetFileInfo(_iVarFile);
            _sVarFile = _oVarFile->Filename;

            SplitString(_sVarFile, '_', _sVarBuffer, sizeof(_sVarBuffer), &_iVarParts, _sVarPart, sizeof(_sVarPart) / sizeof(char*) - 1);

            // Extract file extension, if present
            if ((_iVarParts > 0) && (_sFileExt = strchr(_sVarPart[_iVarParts - 1], '.')))
            {
              _sFileExt[0] = 0x00;
              _sFileExt++;
            }
            else
            {
              _sFileExt = "";
            }

            if (_iVarParts > 2)
            {
              _iPartLen[0] = strlen(_sVarPart[0]);
              _iPartLen[1] = strlen(_sVarPart[1]);
              _iPartLen[2] = strlen(_sVarPart[2]);

              if (StringSame("mod", 3, _sFileExt, true))
              {
                // Model-based attachment, as opposed to texture-based?
                if (StringSame(_sModelCodeBuf, _iModelCodeLen, _sVarPart[0], _iPartLen[0], true))
                {
                  switch (_sVarPart[1][0] | 0x20)
                  {
                    case 'f':
                      if (StringSame("facialatt", 9, _sVarPart[1], _iPartLen[1], true))
                      {
                        _iVarType = EQRV_DETAIL;
                        _iVarNum = atoi(_sVarPart[2]);
                      }
                      else if (StringSame("facialhair", 10, _sVarPart[1], _iPartLen[1], true))
                      {
                        _iVarType = EQRV_BEARD;
                        _iVarNum = atoi(_sVarPart[2]);
                      }
                      break;
                    case 'h':
                      if (StringSame("hair", 4, _sVarPart[1], _iPartLen[1], true))
                      {
                        _iVarType = EQRV_HAIR;
                        _iVarNum = atoi(_sVarPart[2]);
                      }
                      break;
                  }
                }
              }
              else
              {
                // Specified as Color (c) texture?
                if (((*(WORD*)_sVarPart[_iVarParts -1]) & 0xDFDF) != 0x0043)
                {
                  continue; // Nope
                }

                // First part 'a' or 'c'?
                switch ((*(WORD*)_sVarPart[0]) & 0xDFDF)
                {
                  case 0x0041: // A
                  case 0x0043: // C
                    break;
                  default:
                    continue; // Anything else, skip
                    break;
                }

                // Valid texture for our model?
                if (!StringSame(_sModelCodeBuf, _iModelCodeLen, _sVarPart[1], _iPartLen[1], true))
                {
                  // Maybe not. Check for exceptions
                  switch (_iRaceID)
                  {
                    case 474:
                      // Re-using KOR for KRB?
                      if ((_iGender != EQRG_NEUTRAL) || (!StringSame("KOR", 3, _sVarPart[1], _iPartLen[1], true)))
                      {
                        continue; // Nope
                      }
                      break;
                    case 497:
                      // Re-using VAM for VAF?
                      if ((_iGender != EQRG_FEMALE) || (!StringSame("VAM", 3, _sVarPart[1], _iPartLen[1], true)))
                      {
                        continue; // Nope
                      }
                      break;
                    case 522:
                      // Re-using DKM for DKF?
                      if ((_iGender != EQRG_FEMALE) || (!StringSame("DKM", 3, _sVarPart[1], _iPartLen[1], true)))
                      {
                        continue; // Nope
                      }
                      break;
                    case 547:
                    case 548:
                      // Re-using GUS for GUL/GUM?
                      if ((_iGender != EQRG_NEUTRAL) || (!StringSame("GUS", 3, _sVarPart[1], _iPartLen[1], true)))
                      {
                        continue; // Nope
                      }
                      break;
                    default:
                      continue; // Not an exception.
                      break;
                  }
                }

                // Okay, we've confirmed that this file looks like:
                // ?_[ModelCode]_..._c
                
                // Part[2] is our texture type
                if (IsNumeric(_sVarPart[2][0]) && IsNumeric(_sVarPart[2][1]) && (_sVarPart[2][2] == 0x00))
                {
                  // Numeric texture type

                  _iVarType = EQRV_TEXTURE;
                  _iVarNum = atoi(_sVarPart[2]);

                  if (_iVarNum == 10)
                  {
                    // Robe type, add Part[4] for full texture variation number

                    if ((_sVarPart[4][0] | 0x20) == 's')
                    {
                      _iVarNum += atoi(&_sVarPart[4][1]);
                    }
                  }
                }
                else
                {
                  if ((_iRaceID == 497) && ((_sVarPart[2][0] | 0x20) == 'a'))
                  {
                    // EXCEPTION: Race 497 puts variations in 'ac', 'aw', and 'ab' instead of all in 'bd'

                    switch (_sVarPart[2][1] | 0x20)
                    {
                      case 'b':
                      case 'c':
                      case 'w':
                        _sVarPart[2][0] = 'b';
                        _sVarPart[2][1] = 'd';
                        break;
                    }
                  }

                  switch (_sVarPart[2][0] | 0x20)
                  {
                    case 'b':
                      if (StringSame("bd", 2, _sVarPart[2], _iPartLen[2], true))
                      {
                        if ((_sVarPart[3][0] | 0x20) == 's')
                        {
                          _iVarType = EQRV_TEXTURE;
                          _iVarNum = atoi(&_sVarPart[3][1]);

                          switch (_iRaceID)
                          {
                            case 433: // GBN - New Goblin
                              if (_iVarNum > 5)
                              {
                                // EXCEPTION: Also have helms for textures 6-8
                                _oModel->AddVariation(EQRV_HEAD, _iVarNum);
                              }
                              break;
                            case 439: // PMA - New Puma
                              // EXCEPTION: Also has helm types 4-7
                              if (_iVarNum > 3)
                              {
                                _oModel->AddVariation(EQRV_HEAD, _iVarNum);
                              }
                              break;
                            case 442: // ANS - Animated Statue
                            case 448: // ASM - Animated Statue
                            case 453: // FGT - Frost Giant
                            case 455: // KDB - New Kobold
                            case 457: // CWG - Clockwork Golem
                            case 459: // CRH - Corathus
                            case 461: // DC? - New Drachnid
                            case 467: // SHL - Shiliskin
                            case 486: // BLV - Bolvirk
                            case 487: // BSE - Banshee
                            case 489: // EE? - Elddar
                            case 490: // GFO - Forest Giant
                            case 491: // GLB - Bone Golem
                            case 495: // SRN - Scrykin
                            case 496: // TRA - Treant
                            case 497: // VA? - New Vampire
                            case 517: // UNM - Unicorn/Nightmare
                            case 519: // UNM - Unicorn/Nightmare
                            case 520: // BXI - Bixie
                            case 521: // CNT - Centaur
                            case 523: // GFR - Giant
                            case 525: // GRN - Griffin
                            case 526: // GFS - Giant Shade
                            case 527: // HRP - Harpy
                            case 528: // MTH - Mammoth
                            case 529: // SAT - Satyr
                            case 558: // AVK - New Aviak
                            case 563: // SHS - New Shissar
                            case 564: // SIN - New Siren
                            case 568: // BN? - New Brownie
                            case 570: // EXO - Exoskeleton
                            case 577: // SWC - Rotocopter
                            case 579: // WOK - Wereorc
                              // EXCEPTION: These races also have helm types to match their textures
                              _oModel->AddVariation(EQRV_HEAD, _iVarNum);
                              break;
                            case 445: // DRE - Dragon Egg
                              // EXCEPTION: Uses a different normal file for a dark Texture 5
                              if (_iVarNum == 4)
                              {
                                _oModel->AddVariation(EQRV_HEAD, _iVarNum + 1);
                              }
                              break;
                            case 456: // FNG - Fungus Man (Sporali)
                              // This race has several repeating head types, represented by numbers 1-3 (2 == 0)
                              if ((_iVarNum == 1) || (_iVarNum == 3))
                              {
                                _oModel->AddVariation(EQRV_HEAD, _iVarNum);
                              }

                              // Texture numbers are doubled for this race for some reason.
                              _iVarNum *= 2;
                              break;
                            case 458: // ORK - New Orc
                              // SOE combines textures and armor models to create 9 variations from 6 texture files
                              _oModel->AddVariation(EQRV_HEAD, _iVarNum);
                              if (_iVarNum > 3)
                              {
                                _oModel->AddVariation(EQRV_TEXTURE, _iVarNum + 3);
                                _oModel->AddVariation(EQRV_HEAD, _iVarNum + 3);
                              }
                              break;
                            case 465: // KOR - Korlach Witheran
                            case 466: // MYG - Dark Lord Miragul
                              // EXCEPTION: Has head variations
                              _oModel->AddVariation(EQRV_HEAD, _iVarNum);
                              _oModel->AddVariation(EQRV_HEAD, _iVarNum + 1);
                              break;
                            case 468: // SNK - New Snake
                              // EXCEPTION: Has a head variation for texture 4
                              if (_iVarNum == 4)
                              {
                                _oModel->AddVariation(EQRV_HEAD, _iVarNum);
                              }
                              break;
                            case 474: // KRB - Witheran
                              // EXCEPTION: Has a head variation, but no texture variation (re-using KOR Texture 1)
                              _oModel->AddVariation(EQRV_HEAD, 1);
                              if (_iVarNum == 1)
                              {
                                _iVarNum = 0;
                              }
                              break;
                            case 492: // HRS - New Horse
                            case 518: // HRS - New Horse
                              // EXCEPTION: Has head variations
                              
                              _oModel->AddVariation(EQRV_HEAD, _iVarNum * 3);
                              break;
                            case 566: // HN? - New Humans
                              // EXCEPTION: Texture files are actually used for head variations
                              if (_iVarType == EQRV_TEXTURE)
                              {
                                _iVarType = EQRV_HEAD;
                              }
                              break;
                            case 569: // DRP - Prismatic Dragon
                              // EXCEPTION: Texture variation 1 is broken. Which is a shame, because the clockwork
                              //            texture itself looks pretty sweet.
                              if (_iVarNum > 0)
                              {
                                _iVarNum = 0;
                              }
                              break;
                            case 580: // WOR - Worg
                              // EXCEPTION: Has 2 head variations, a saddle and armour
                              if (_iVarNum == 0)
                              {
                                _oModel->AddVariation(EQRV_HEAD, _iVarNum + 1);
                                _oModel->AddVariation(EQRV_HEAD, _iVarNum + 2);
                              }
                              break;
                          }
                        }
                      }
                      break;
                    case 'f':
                      if (StringSame("flag1", 5, _sVarPart[2], _iPartLen[2], true))
                      {
                        if ((_sVarPart[3][0] | 0x20) == 's')
                        {
                          _iVarType = EQRV_TEXTURE;
                          _iVarNum = atoi(&_sVarPart[3][1]);
                        }

                        switch (_iRaceID)
                        {
                          case 586:
                            if (_iVarNum == 0)
                            {
                              _iVarNum = 31;
                            }
                            break;
                        }
                      }
                      break;
                    case 't':
                      if (StringSame("tattoo", 6, _sVarPart[2], _iPartLen[2], true))
                      {
                        if ((_sVarPart[3][0] | 0x20) == 's')
                        {
                          _iVarType = EQRV_TATTOO;
                          _iVarNum = atoi(&_sVarPart[3][1]);
                        }
                      }
                      break;
                    case 'h':
                      if (StringSame("head", 4, _sVarPart[2], _iPartLen[2], true))
                      {
                        if ((_sVarPart[3][0] | 0x20) == 's')
                        {
                          _iVarType = EQRV_FACE;
                          _iVarNum = atoi(&_sVarPart[3][1]);
                        }
                      }
                      else if (StringSame("hr", 2, _sVarPart[2], _iPartLen[2], true))
                      {
                        if ((_sVarPart[3][0] | 0x20) == 's')
                        {
                          _iVarType = EQRV_HAIR;
                          _iVarNum = atoi(&_sVarPart[3][1]);
                        }
                      }
                      else if (StringSame("hd", 2, _sVarPart[2], _iPartLen[2], true))
                      {
                        if ((_sVarPart[3][0] | 0x20) == 's')
                        {
                          if (_iRaceID == 431)
                          {
                            // EXCEPTION: New-style dervishes store texture in 'HD' and don't specify their head variation 1.
                            _oModel->AddVariation(EQRV_HEAD, 1);

                            _iVarType = EQRV_TEXTURE;
                          }
                          else
                          {
                            _iVarType = EQRV_HEAD;
                          }

                          _iVarNum = atoi(&_sVarPart[3][1]);
                        }
                      }
                      break;
                  }
                }
              }
            }

            if (_iVarType != EQRV_NONE)
            {
              if (FAILED(_oModel->AddVariation(_iVarType, _iVarNum)))
              {
                Log->Entry("    SKIPPED: Problem adding variation from %s", _sVarFile);
              }
            }
          }
        }
      }
    }
  }

  Source->Loaded = 1;

  delete _oS3D;
  return S_OK;
}

HRESULT LoadSettings()
{
  char   _sINIFile[_MAX_PATH];
  FILE*  _oINIFile;
  char   _sLine[256];
  size_t _iLen;

  char   _sParts[256];
  WORD   _iParts;
  char*  _sPart[2];

  char*  _sVar;
  char*  _sVal;
  size_t _iVarLen;
  size_t _iValLen;
  
  bool   _bSetDefaults = false;

  sWorkPath[0] = 0x00;
  sOutPath[0] = 0x00;

  if (FileExists(sINIFile))
  {
    strcpy_s(_sINIFile, sizeof(_sINIFile), sINIFile);
  }
  else
  {
    _iLen = strlen(sOutPathDefault);
    if (!_iLen)
    {
      // No INI file in current directory, and no OutPathDefault to check
      
      _bSetDefaults = true;
    }
    else
    {
      if (FAILED(MakePath(sOutPathDefault, sINIFile, _sINIFile, sizeof(_sINIFile))))
      {
        // Problem with our hard-coded paths. Too long?
        
        _bSetDefaults = true;
      }
      else
      {
        if (!FileExists(_sINIFile))
        {
          // No INI file in current directory or in OutPathDefault
          
          _bSetDefaults = true;
        }
      }
    }
  }
  
  if (!_bSetDefaults)
  {
    if (!fopen_s(&_oINIFile, _sINIFile, "rt"))
    {
      while (_iLen = ReadLine(_oINIFile, _sLine, sizeof(_sLine)))
      {
        // Line of text to work with.

        SplitString(_sLine, '=', _sParts, sizeof(_sParts), &_iParts, _sPart, 2);
        
        if (_iParts == 2)
        {
          _sVar = _sPart[0];
          _sVal = _sPart[1];

          _iVarLen = strlen(_sVar);
          _iValLen = strlen(_sVal);

          TrimString(_sVar, &_iVarLen);
          TrimString(_sVal, &_iValLen);

          _strlwr_s(_sVar, _iVarLen + 1);

          switch (_sVar[0])
          {
            case 'e':
              if (StringSame(_sVar, _iVarLen, "eqpath", 6, false))
              {
                strncpy_s(sWorkPath, sizeof(sWorkPath), _sVal, _TRUNCATE);
              }
              break;
            case 'o':
              if (StringSame(_sVar, _iVarLen, "outpath", 7, false))
              {
                strncpy_s(sOutPath, sizeof(sOutPath), _sVal, _TRUNCATE);
              }
              break;
          }
        }
      }
      
      fclose(_oINIFile);
    }
  }

  if (!sWorkPath[0])
  {
    strcpy_s(sWorkPath, sWorkPathDefault);
  }

  if (!sOutPath[0])
  {
    strcpy_s(sOutPath, sOutPathDefault);
  }

  _iLen = strlen(sWorkPath);
  TrimString(sWorkPath, &_iLen);

  while (sWorkPath[_iLen - 1] == '\\')
  {
    sWorkPath[_iLen - 1] = 0x00;
    _iLen--;
  }

  _iLen = strlen(sOutPath);
  TrimString(sOutPath, &_iLen);

  while (sOutPath[_iLen - 1] == '\\')
  {
    sOutPath[_iLen - 1] = 0x00;
    _iLen--;
  }

  return S_OK;
}