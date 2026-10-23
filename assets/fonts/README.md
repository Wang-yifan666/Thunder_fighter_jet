# 字体资源

- `UbuntuMono-Regular.ttf`：Ubuntu Mono Regular（weight 400），用于 SDL3 版 HUD 与菜单文字。
- `LICENSE-Ubuntu-Font-Licence-1.0.txt`：Ubuntu Font Licence 1.0 全文。

## 授权

字体版权归 Canonical Ltd.（2010–2022），按 Ubuntu Font Licence 1.0 分发。
该许可证允许字体随软件打包与嵌入；再分发时需保留本授权文件。

## 来源

来自 Google Fonts 的 Ubuntu Mono（400 / Regular）：
通过 `https://fonts.googleapis.com/css?family=Ubuntu+Mono` 获取 CSS，
使用不支持 woff 的旧浏览器 UA 请求，CSS 会给出 `fonts.gstatic.com` 的 `.ttf` 直链。

重新获取的简单做法：

```powershell
$ua = 'Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.7.13) Gecko/20060417'
$css = (Invoke-WebRequest 'https://fonts.googleapis.com/css?family=Ubuntu+Mono' -Headers @{ 'User-Agent' = $ua }).Content
$url = [regex]::Match($css, 'url\((https://[^)]+\.ttf)\)').Groups[1].Value
Invoke-WebRequest $url -OutFile UbuntuMono-Regular.ttf
```

## 回退

未编译 `SDL3_ttf`、或运行时找不到该字体文件时，SDL 前端会自动回退到内置的
5×7 点阵字体（`src/sdl/bitmap_font.cpp`），不影响游戏运行。
