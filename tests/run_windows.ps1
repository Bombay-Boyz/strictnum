$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$build = Join-Path $env:TEMP ("strictnum-win-" + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $build | Out-Null
try {
  Push-Location $root
  & cl.exe /nologo /std:c11 /W4 /WX /I. tests\test_strictnum.c "/Fe:$build\strictnum.exe"
  if ($LASTEXITCODE -ne 0) { throw 'MSVC strict suite compilation failed' }
  & "$build\strictnum.exe"
  if ($LASTEXITCODE -ne 0) { throw 'MSVC strict suite failed' }

  & cl.exe /nologo /std:c11 /W4 /WX /I. tests\test_portable.c "/Fe:$build\portable-c.exe"
  if ($LASTEXITCODE -ne 0) { throw 'MSVC portable C suite compilation failed' }
  & "$build\portable-c.exe"
  if ($LASTEXITCODE -ne 0) { throw 'MSVC portable C suite failed' }

  & cl.exe /nologo /std:c11 /W4 /WX /I. tests\test_header_namespace.c "/Fe:$build\namespace.exe"
  if ($LASTEXITCODE -ne 0) { throw 'MSVC namespace/SN_API compatibility compilation failed' }
  & "$build\namespace.exe"
  if ($LASTEXITCODE -ne 0) { throw 'MSVC namespace/SN_API compatibility test failed' }

  & cl.exe /nologo /std:c11 /W4 /WX /I. tests\multi_tu\first.c tests\multi_tu\second.c tests\multi_tu\main.c "/Fe:$build\multi-tu.exe"
  if ($LASTEXITCODE -ne 0) { throw 'MSVC multi-translation-unit compilation failed' }
  & "$build\multi-tu.exe"
  if ($LASTEXITCODE -ne 0) { throw 'MSVC multi-translation-unit test failed' }

  & cl.exe /nologo /std:c11 /W4 /WX /I. examples\port.c "/Fe:$build\port.exe"
  if ($LASTEXITCODE -ne 0) { throw 'MSVC port example compilation failed' }
  $portOutput = & "$build\port.exe" 8080
  if ($LASTEXITCODE -ne 0 -or $portOutput -notmatch 'port = 8080') { throw 'MSVC port example failed' }
  # Capture the expected failure through .NET rather than PowerShell's native
  # stderr stream, which can become a terminating NativeCommandError when
  # ErrorActionPreference is Stop.
  $portProcess = New-Object System.Diagnostics.Process
  $portProcess.StartInfo = New-Object System.Diagnostics.ProcessStartInfo
  $portProcess.StartInfo.FileName = Join-Path $build 'port.exe'
  $portProcess.StartInfo.Arguments = '0'
  $portProcess.StartInfo.UseShellExecute = $false
  $portProcess.StartInfo.RedirectStandardError = $true
  if (-not $portProcess.Start()) { throw 'Could not start MSVC port example negative test' }
  $portError = $portProcess.StandardError.ReadToEnd()
  $portProcess.WaitForExit()
  $portExitCode = $portProcess.ExitCode
  $portProcess.Dispose()
  if ($portExitCode -eq 0) { throw 'MSVC port example accepted port zero' }
  if ($portError -notmatch 'invalid port "0": must be 1-65535') {
    throw 'MSVC port example did not emit the expected port-zero diagnostic'
  }

  foreach ($standard in @('c++14', 'c++17', 'c++20')) {
    $exe = Join-Path $build ("portable-" + $standard + '.exe')
    & cl.exe /nologo "/std:$standard" /TP /W4 /WX /I. tests\test_portable.c "/Fe:$exe"
    if ($LASTEXITCODE -ne 0) { throw "MSVC $standard portable C++ compilation failed" }
    & $exe
    if ($LASTEXITCODE -ne 0) { throw "MSVC $standard portable C++ suite failed" }
  }
} finally {
  Pop-Location
  Remove-Item -Recurse -Force $build -ErrorAction SilentlyContinue
}
