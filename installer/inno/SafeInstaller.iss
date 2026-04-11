#ifndef AppVersion
  #define AppVersion "1.0.0"
#endif

#ifndef SourceDir
  #define SourceDir "..\..\debug-build\Release"
#endif

#ifndef OutputDir
  #define OutputDir "..\..\debug-build\installer"
#endif

[Setup]
AppId=DF57A8E1-E9F7-4F49-9F4A-922293A953A1
AppName=Safe
AppVersion={#AppVersion}
AppPublisher=Safe Project
DefaultDirName={localappdata}\Programs\Safe
DefaultGroupName=Safe
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=none
UninstallDisplayName=Safe
UninstallDisplayIcon={app}\Safe.exe
OutputDir={#OutputDir}
OutputBaseFilename=safe
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UsedUserAreasWarning=no
ChangesEnvironment=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "{#SourceDir}\Safe.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\assets\*"; DestDir: "{app}\assets"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{userprograms}\Safe"; Filename: "{app}\Safe.exe"

[Run]
Filename: "{app}\Safe.exe"; Description: "Launch Safe"; Flags: nowait postinstall skipifsilent

[Code]
function ContainsPathToken(PathValue: string; PathToken: string): Boolean;
begin
  Result := Pos(';' + Uppercase(PathToken) + ';', ';' + Uppercase(PathValue) + ';') > 0;
end;

function EndsWithSafeSuffix(NameValue: string): Boolean;
begin
  Result := (Length(NameValue) >= 5) and (Uppercase(Copy(NameValue, Length(NameValue) - 4, 5)) = '.SAFE');
end;

function IsDirectoryEmpty(DirPath: string): Boolean;
var
  FindRec: TFindRec;
begin
  Result := True;
  if not FindFirst(AddBackslash(DirPath) + '*', FindRec) then
    exit;

  try
    repeat
      if (FindRec.Name <> '.') and (FindRec.Name <> '..') then
      begin
        Result := False;
        exit;
      end;
    until not FindNext(FindRec);
  finally
    FindClose(FindRec);
  end;
end;

procedure RemoveInstalledContentPreservingSafe(DirPath: string);
var
  FindRec: TFindRec;
  EntryPath: string;
  IsDir: Boolean;
begin
  if not DirExists(DirPath) then
    exit;

  if not FindFirst(AddBackslash(DirPath) + '*', FindRec) then
    exit;

  try
    repeat
      if (FindRec.Name <> '.') and (FindRec.Name <> '..') then
      begin
        EntryPath := AddBackslash(DirPath) + FindRec.Name;
        IsDir := (FindRec.Attributes and FILE_ATTRIBUTE_DIRECTORY) <> 0;

        if IsDir then
        begin
          if not EndsWithSafeSuffix(FindRec.Name) then
          begin
            RemoveInstalledContentPreservingSafe(EntryPath);
            if IsDirectoryEmpty(EntryPath) then
              RemoveDir(EntryPath);
          end;
        end
        else
        begin
          if not EndsWithSafeSuffix(FindRec.Name) then
            DeleteFile(EntryPath);
        end;
      end;
    until not FindNext(FindRec);
  finally
    FindClose(FindRec);
  end;
end;

procedure AddToUserPath(PathToken: string);
var
  OrigPath: string;
  NewPath: string;
begin
  if not RegQueryStringValue(HKCU, 'Environment', 'Path', OrigPath) then
    OrigPath := '';

  if ContainsPathToken(OrigPath, PathToken) then
    exit;

  if OrigPath = '' then
    NewPath := PathToken
  else
    NewPath := OrigPath + ';' + PathToken;

  RegWriteExpandStringValue(HKCU, 'Environment', 'Path', NewPath);
end;

procedure RemoveFromUserPath(PathToken: string);
var
  OrigPath: string;
  UpdatedPath: string;
begin
  if not RegQueryStringValue(HKCU, 'Environment', 'Path', OrigPath) then
    exit;

  UpdatedPath := ';' + OrigPath + ';';
  StringChangeEx(UpdatedPath, ';' + PathToken + ';', ';', True);

  while Pos(';;', UpdatedPath) > 0 do
    StringChangeEx(UpdatedPath, ';;', ';', True);

  if (Length(UpdatedPath) > 0) and (UpdatedPath[1] = ';') then
    Delete(UpdatedPath, 1, 1);

  if (Length(UpdatedPath) > 0) and (UpdatedPath[Length(UpdatedPath)] = ';') then
    Delete(UpdatedPath, Length(UpdatedPath), 1);

  RegWriteExpandStringValue(HKCU, 'Environment', 'Path', UpdatedPath);
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
    AddToUserPath(ExpandConstant('{app}'));
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  AppPath: string;
begin
  AppPath := ExpandConstant('{app}');

  if CurUninstallStep = usUninstall then
    RemoveFromUserPath(AppPath)
  else if CurUninstallStep = usPostUninstall then
  begin
    RemoveInstalledContentPreservingSafe(AppPath);
    if DirExists(AppPath) and IsDirectoryEmpty(AppPath) then
      RemoveDir(AppPath);
  end;
end;
