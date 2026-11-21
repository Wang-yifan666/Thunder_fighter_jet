# 构建 Naidan 的 Windows 安装包。
#
# 流程：准备载荷（exe + 说明与许可文件）-> 调 Inno Setup 的 ISCC 编译。
#
# 用法：
#   pwsh tools/build_installer.ps1
#   pwsh tools/build_installer.ps1 -Exe build/Naidan.exe -Version 1.7
#
# 前置：已安装 Inno Setup 6（winget install JRSoftware.InnoSetup），
#       或通过 -IsccPath 指定 ISCC.exe。

param(
    [string]$Exe = "build/Naidan.exe",
    [string]$Version = "1.7",
    [string]$IsccPath = ""
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot

function Resolve-Iscc {
    param([string]$Explicit)
    if ($Explicit -and (Test-Path $Explicit)) { return $Explicit }

    $candidates = @(
        "$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe",
        "$env:ProgramFiles\Inno Setup 6\ISCC.exe",
        "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe"
    )
    foreach ($c in $candidates) {
        if (Test-Path $c) { return $c }
    }
    $cmd = Get-Command iscc -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    return $null
}

$iscc = Resolve-Iscc -Explicit $IsccPath
if (-not $iscc) {
    throw "找不到 ISCC.exe：请先安装 Inno Setup 6（winget install JRSoftware.InnoSetup）"
}

$exePath = if ([System.IO.Path]::IsPathRooted($Exe)) { $Exe } else { Join-Path $root $Exe }
if (-not (Test-Path $exePath)) {
    throw "找不到可执行文件：$exePath（先执行 cmake --build build）"
}

# ---------- 1. 准备载荷 ----------
$packageDir = Join-Path $root "build/package"
if (Test-Path $packageDir) { Remove-Item -Recurse -Force $packageDir }
New-Item -ItemType Directory -Force -Path $packageDir | Out-Null

Copy-Item $exePath (Join-Path $packageDir "Naidan.exe") -Force
Copy-Item (Join-Path $root "packaging/payload/README.txt") $packageDir -Force
Copy-Item (Join-Path $root "packaging/payload/THIRD_PARTY_LICENSES.txt") $packageDir -Force
Copy-Item (Join-Path $root "LICENSE") (Join-Path $packageDir "LICENSE") -Force
Copy-Item (Join-Path $root "assets/fonts/LICENSE-Ubuntu-Font-Licence-1.0.txt") $packageDir -Force

Write-Host "载荷目录：$packageDir"
Get-ChildItem $packageDir | ForEach-Object { "  {0,-42} {1,8:N0} B" -f $_.Name, $_.Length }

# ---------- 2. 编译安装包 ----------
$iss = Join-Path $root "packaging/naidan.iss"
$sourceDir = $packageDir.Replace('\', '\\')

& $iscc "/DSourceDir=$sourceDir" "/DAppVersion=$Version" $iss
if ($LASTEXITCODE -ne 0) {
    throw "ISCC 编译失败（exit=$LASTEXITCODE）"
}

# ---------- 3. 报告产物 ----------
$outputDir = Join-Path $root "build/installer"
$setup = Get-ChildItem $outputDir -Filter "*.exe" | Sort-Object LastWriteTime -Descending | Select-Object -First 1
if ($setup) {
    $hash = (Get-FileHash $setup.FullName -Algorithm SHA256).Hash.ToLower()
    Write-Host ""
    Write-Host "安装包：$($setup.FullName)"
    Write-Host "  大小：$('{0:N0}' -f $setup.Length) B"
    Write-Host "  SHA256：$hash"
} else {
    throw "没有找到生成的安装包（检查 ISCC 的 OutputDir）"
}
