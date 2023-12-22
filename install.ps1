#Requires -Version 7.2
#Requires -RunAsAdministrator

<#
.SYNOPSIS
	Installs hyperenable
.DESCRIPTION
	Copies hyperenable.exe to %ProgramFiles%, and edits relevant registry entries.
.PARAMETER Action
	One of "install" or "uninstall"
.PARAMETER Exe
	Path to hyperenable.exe file which will be installed
.PARAMETER Destination
	Path to where hyperenable.exe will be installed to. Optional
.EXAMPLE
	PS> ./install.ps1 -action install -exe "hyperenable.exe"
	PS> ./install.ps1 -action uninstall
.LINK
	https://github.com/midrare/hyperenable
.NOTES
	Author: midrare | License: MIT
#>



param(
    [parameter(Mandatory, HelpMessage="Either ""install"" or ""uninstall"".")]
    [validateset("install", "uninstall")]
    [string]$Action = $null,
    [parameter(HelpMessage='Path to hyperenable.exe file to install')]
    [string]$Exe = $null,
    [parameter(HelpMessage='Path to where hyperenable.exe will be installed')]
    [string]$Destination = "$Env:ProgramFiles\hyperenable\hyperenable.exe"
)



function Get-ArrayItemIndex([string[]]$Array, [string]$Pattern) {
    For ($i = 0; $i -lt $Array.Length; $i++) {
        If ($Array[$i] -match "$Pattern") {
            Return $i
        }
    }

    Return -1
}


function Remove-ArrayItem([ref][string[]]$Array, [string]$Pattern) {
    $i = 0
    While ($i -lt $Array.Value.Length) {
        If ($Array.Value[$i] -match "$Pattern") {
            If ($i -le 0) {
                $Array.Value = @($Array.Value[($i + 1)..$Array.Value.Length])
            } Else {
                $Array.Value = @($Array.Value[0..($i - 1)]) + @($Array.Value[($i + 1)..$Array.Value.Length])
            }
        } Else {
            $i += 1
        }
    }
}


function Add-ArrayItem([ref][string[]]$Array, [int]$Idx, [string]$Value) {
    If ($idx -le 0) {
        $Array.Value = @($Value) + @($Array.Value[$Idx..$Array.Value.Length])
    } Else {
        $Array.Value = @($Array.Value[0..($Idx - 1)]) + @($Value) + @($Array.Value[$Idx..$Array.Value.Length])
    }
}


# Get-ScriptPath() is from https://stackoverflow.com/a/58768926
# authored by Randy https://stackoverflow.com/users/1561650/randy
# used here under CC BY-SA 4.0
function Get-ScriptPath() {
    # If using PowerShell ISE
    if ($psISE)
    {
        $ScriptPath = Split-Path -Parent -Path $psISE.CurrentFile.FullPath
    }
    # If using PowerShell 3.0 or greater
    elseif($PSVersionTable.PSVersion.Major -gt 3)
    {
        $ScriptPath = $PSScriptRoot
    }
    # If using PowerShell 2.0 or lower
    else
    {
        $ScriptPath = split-path -parent $MyInvocation.MyCommand.Path
    }

    # If still not found
    # I found this can happen if running an exe created using PS2EXE module
    if(-not $ScriptPath) {
        $ScriptPath = [System.AppDomain]::CurrentDomain.BaseDirectory.TrimEnd('\')
    }

    # Return result
    return $ScriptPath
}


function Write-Userinit([string]$Exe = $null) {
    $Userinit = (Get-ItemProperty -Path $USERINIT_REG_KEY -Name $USERINIT_REG_VALUE).$USERINIT_REG_VALUE
    $UserinitArr = $Userinit -Split ',' | ForEach-Object { $_.Trim() } | Where-Object { $_ }
    $UserinitArr = @($UserinitArr) + "hyperenable.exe"
    Remove-ArrayItem ([ref]$UserinitArr) "hyperenable\.exe"

    If ($Exe) {
        $UserinitIdx = Get-ArrayItemIndex $UserinitArr "userinit\.exe"
        If ($UserinitIdx -lt 0) {
            Add-ArrayItem ([ref]$UserinitArr) 0 """$Exe"" start"
        } Else {
            Add-ArrayItem ([ref]$UserinitArr) $UserinitIdx """$Exe"" start"
        }
    }

    $Userinit = (($UserinitArr) -join ', ') + ','
    New-Item -Path $USERINIT_REG_KEY -ErrorAction Ignore | Out-Null
    Set-ItemProperty -Path $USERINIT_REG_KEY -Name $USERINIT_REG_VALUE -Value $Userinit -Type String
}


function Write-Shortcut([string]$Path, [string]$Target, [string]$TargetArgs) {
    Remove-Item -Path $Path -Force -ErrorAction Ignore
    $WshShell = New-Object -comObject WScript.Shell
    $WshSc = $WshShell.CreateShortcut($Path)
    $WshSc.TargetPath = $Target
    $WshSc.Arguments = $TargetArgs
    $WshSc.Save()
}




# TODO Do not use Userinit registry key https://superuser.com/a/1047629

$MSOFFICE_REG_KEY = 'HKLM:\SOFTWARE\Classes\ms-officeapp\Shell\Open\Command'
$MSOFFICE_REG_VALUE = 'rundll32'

$USERINIT_REG_KEY = 'HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon'
$USERINIT_REG_VALUE = 'Userinit'

$STARTUP_SHORTCUT = "$Env:ProgramData\Microsoft\Windows\Start Menu\Programs\Startup\hyperenable.lnk"


If ($Action -eq "install") {
    If (!$Exe) {
        Write-Error "Path to hyperenable.exe not specified."
        Exit 1
    }

    If (!(Test-Path $Exe -PathType Leaf)) {
        Write-Error "hyperenable.exe file at ""$Exe"" not found."
        Exit 1
    }

    If (!$Destination) {
        Write-Error "Destination path not specified."
        Exit 1
    }

    If (!($Destination -match '\.[eE][xX][eE]$') -or (Test-Path -Path $Destination -PathType Container)) {
        $Destination = Join-Path -Path $Destination -ChildPath '\hyperenable.exe'
    }


    Write-Output "Writing $Exe to $Destination"
    New-Item -Path (Split-Path -Parent $Destination) -Type Directory -ErrorAction Ignore | Out-Null
    Copy-Item -Path "hyperenable.exe" -Destination $Destination -Force -ErrorAction Stop


    Write-Output "Writing ${USERINIT_REG_KEY}\${USERINIT_REG_VALUE}"
    Write-Userinit -exe $Destination


    Write-Output "Writing ${MSOFFICE_REG_KEY}\${MSOFFICE_REG_VALUE}"
    New-Item -Path $MSOFFICE_REG_KEY -ErrorAction Ignore | Out-Null
    Set-Item -Path $MSOFFICE_REG_KEY -Value $MSOFFICE_REG_VALUE -Type String


    Write-Output "Writing $STARTUP_SHORTCUT"
    Write-Shortcut $STARTUP_SHORTCUT $Destination 'stop'
} ElseIf ($Action -eq "uninstall") {
    Write-Output "Deleting $Destination"
    Remove-Item -Path $Destination -Force -ErrorAction Ignore
    $DestDir = Split-Path -Parent $Destination
    If ((Get-ChildItem -Path $DestDir -ErrorAction Ignore | Measure-Object).Count -le 0) {
        Remove-Item -Path $DestDir -Force -ErrorAction Ignore -Recurse
    }


    Write-Output "Deleting entry from ${USERINIT_REG_KEY}\${USERINIT_REG_VALUE}"
    Write-Userinit -exe $null


    Write-Output "Deleting ${MSOFFICE_REG_KEY}\${MSOFFICE_REG_VALUE}"
    Remove-Item -Path $MSOFFICE_REG_KEY -Force -ErrorAction Ignore


    Write-Output "Deleting $STARTUP_SHORTCUT"
    Remove-Item -Path $STARTUP_SHORTCUT -Force -ErrorAction Ignore
}
