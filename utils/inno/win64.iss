[Setup]
ArchitecturesInstallIn64BitMode=x64
AppName=AIDA-X
AppPublisher=AIDA DSP
AppPublisherURL=https://github.com/AidaDSP/aida-x/
AppSupportURL=https://github.com/AidaDSP/aida-x/issues/
AppUpdatesURL=https://github.com/AidaDSP/aida-x/releases/
AppVerName=AIDA-X @VERSION@
AppVersion=@VERSION@
DefaultDirName={commonpf64}\AIDA-X
DisableDirPage=yes
DisableProgramGroupPage=yes
; DisableReadyPage=true
; DisableWelcomePage=no
InfoBeforeFile=@UTILSDIR@\windows-installer-welcome.rtf
OutputBaseFilename=AIDA-X-@VERSION@-win64-installer
OutputDir=.
SetupIconFile=@UTILSDIR@\ax.ico
UsePreviousAppDir=no
VersionInfoCompany=AIDA DSP
VersionInfoCopyright=AIDA DSP
VersionInfoDescription=AIDA-X Installer
VersionInfoProductName=AIDA-X
VersionInfoProductVersion=@VERSION@
VersionInfoVersion=@VERSION@
; WizardImageFile=WizModernImage.bmp
; WizardImageStretch=false
; WizardSmallImageFile=WizSmall.bmp

[Types]
Name: "normal"; Description: "Full installation";
Name: "custom"; Description: "Custom installation"; Flags: iscustom;

[Components]
Name: standalone; Description: "Standalone"; Types: normal;
Name: clap; Description: "CLAP plugin"; Types: normal;
Name: lv2; Description: "LV2 plugin"; Types: normal;
Name: vst2; Description: "VST2 plugin"; Types: normal;
Name: vst3; Description: "VST3 plugin"; Types: normal;

[Files]
Source: "@BINDIR@/AIDA-X.exe"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion;
Source: "@BINDIR@/AIDA-X.clap"; DestDir: "{commoncf64}\CLAP"; Components: clap; Flags: ignoreversion;
Source: "@BINDIR@/AIDA-X.lv2/*.*"; DestDir: "{commoncf64}\LV2\AIDA-X.lv2"; Components: lv2; Flags: ignoreversion recursesubdirs;
Source: "@BINDIR@/AIDA-X-vst2.dll"; DestDir: "{code:GetVST2Dir}"; Components: vst2; Flags: ignoreversion;
Source: "@BINDIR@/AIDA-X.vst3"; DestDir: "{commoncf64}\VST3"; Components: vst3; Flags: ignoreversion recursesubdirs;

[Icons]
Name: "{commonprograms}\AIDA-X"; Filename: "{app}\AIDA-X.exe"; Components: standalone; Comment: "AIDA-X Standalone";
Name: "{commondesktop}\AIDA-X"; Filename: "{app}\AIDA-X.exe"; Components: standalone; Tasks: desktopicon;

[Tasks]
Name: desktopicon; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

; based on https://www.kvraudio.com/forum/viewtopic.php?t=501615
[Code]
var
  VST2DirPage: TInputDirWizardPage;
  TypesComboOnChangePrev: TNotifyEvent;

procedure ComponentsListCheckChanges;
begin
  WizardForm.NextButton.Enabled := (WizardSelectedComponents(False) <> '');
end;

procedure ComponentsListClickCheck(Sender: TObject);
begin
  ComponentsListCheckChanges;
end;

procedure TypesComboOnChange(Sender: TObject);
begin
  TypesComboOnChangePrev(Sender);
  ComponentsListCheckChanges;
end;

procedure InitializeWizard;
begin

  WizardForm.ComponentsList.OnClickCheck := @ComponentsListClickCheck;
  TypesComboOnChangePrev := WizardForm.TypesCombo.OnChange;
  WizardForm.TypesCombo.OnChange := @TypesComboOnChange;

  VST2DirPage := CreateInputDirPage(wpSelectComponents,
  'Confirm VST2 Plugin Directory', '',
  'Select the folder in which setup should install the VST2 Plugin, then click Next.',
  False, '');

  VST2DirPage.Add('VST2 Plugin Directory');
  VST2DirPage.Values[0] := ExpandConstant('{reg:HKLM\SOFTWARE\VST,VSTPluginsPath|{commonpf64}\VSTPlugins}');
end;

procedure CurPageChanged(CurPageID: Integer);
begin
  if CurPageID = VST2DirPage.ID then
  begin
    VST2DirPage.Buttons[0].Enabled := WizardIsComponentSelected('vst2');
    VST2DirPage.PromptLabels[0].Enabled := VST2DirPage.Buttons[0].Enabled;
    VST2DirPage.Edits[0].Enabled := VST2DirPage.Buttons[0].Enabled;
  end;

  if CurPageID = wpSelectComponents then
  begin
    ComponentsListCheckChanges;
  end;
end;

function ShouldSkipPage(PageID: Integer): Boolean;
begin
  if PageID = VST2DirPage.ID then
  begin
    If (not WizardIsComponentSelected('vst2'))then
      begin
        Result := True
      end;
  end;
end;

function GetVST2Dir(Param: string): string;
begin
    Result := VST2DirPage.Values[0];
end;
