# 构建 Naidan 的 Windows 安装包。
#
# 流程：准备载荷（exe + 说明与许可文件）->（可选）签名 -> 调 Inno Setup 的 ISCC 编译
#       ->（可选）给安装包再签一次。
#
# 关于"Windows 提示未知发布者 / 杀软报毒"
# ----------------------------------------
# 这不是打包方式的问题，而是**没有代码签名证书**：
#   * SmartScreen 只认 Authenticode 签名 + 下载量积累的信誉，新文件一律"未知发布者"；
#   * 杀软对"未签名的 Inno Setup 安装包"本身就有启发式名额。
# 拿到证书后，把证书指纹传给本脚本即可签 exe 与安装包：
#   pwsh tools/build_installer.ps1 -Thumbprint <证书指纹>
# 需要 signtool.exe（Windows SDK）或 -SignToolPath 指定路径。
#
# 用法：
#   pwsh tools/build_installer.ps1
#   pwsh tools/build_installer.ps1 -Exe build/Naidan.exe -Version 1.8
#   pwsh tools/build_installer.ps1 -Thumbprint 0123ABCD... -TimestampUrl http://timestamp.digicert.com
#
# 前置：已安装 Inno Setup 6（winget install JRSoftware.InnoSetup），
#       或通过 -IsccPath 指定 ISCC.exe。

param(
    [string]$Exe = "build/Naidan.exe",
    [string]$Version = "1.8",
    [string]$IsccPath = "",
    # 代码签名：留空则跳过（产物会是"未知发布者"）
    [string]$Thumbprint = "",
    [string]$SignToolPath = "",
    [string]$TimestampUrl = "http://timestamp.digicert.com"
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot

function Resolve-SignTool {
    param([string]$Explicit)
    if ($Explicit -and (Test-Path $Explicit)) { return $Explicit }

    $cmd = Get-Command signtool -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }

    # Windows SDK 的默认安装位置（按版本号倒序找最新的）
    $roots = @(
        "${env:ProgramFiles(x86)}\Windows Kits\10\bin",
        "$env:ProgramFiles\Windows Kits\10\bin"
    )
    foreach ($dir in $roots) {
        if (-not (Test-Path $dir)) { continue }
        $candidate = Get-ChildItem $dir -Recurse -Filter signtool.exe -ErrorAction SilentlyContinue |
            Where-Object { $_.FullName -match 'x64' } |
            Sort-Object FullName -Descending |
            Select-Object -First 1
        if ($candidate) { return $candidate.FullName }
    }
    return $null
}

function Sign-File {
    param([string]$Path)
    if (-not $Thumbprint) { return }

    $signtool = Resolve-SignTool -Explicit $SignToolPath
    if (-not $signtool) {
        throw "给了 -Thumbprint 但找不到 signtool.exe：装 Windows SDK，或用 -SignToolPath 指定"
    }

    & $signtool sign /sha1 $Thumbprint /fd SHA256 /tr $TimestampUrl /td SHA256 $Path
    if ($LASTEXITCODE -ne 0) { throw "签名失败：$Path" }
    Write-Host "已签名：$Path"
}

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

# 内层 exe 也签上：安装包里的可执行文件未签名，同样会拉低整体信誉
Sign-File (Join-Path $packageDir "Naidan.exe")

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
    Sign-File $setup.FullName

    $hash = (Get-FileHash $setup.FullName -Algorithm SHA256).Hash.ToLower()
    Write-Host ""
    Write-Host "安装包：$($setup.FullName)"
    Write-Host "  大小：$('{0:N0}' -f $setup.Length) B"
    Write-Host "  SHA256：$hash"

    $sig = Get-AuthenticodeSignature $setup.FullName
    Write-Host "  签名：$($sig.Status)"
    if ($sig.Status -ne 'Valid') {
        Write-Host "  提示：未签名 —— 用户会看到 SmartScreen 的'未知发布者'，杀软也可能误报。"
        Write-Host "        拿到代码签名证书后加 -Thumbprint <指纹> 重新打包即可。"
    }
} else {
    throw "没有找到生成的安装包（检查 ISCC 的 OutputDir）"
}
