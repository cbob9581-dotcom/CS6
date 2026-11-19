$ErrorActionPreference='Stop'
$sourceRoot='D:/CS6/Plugins/CS6BombRuntime/Source'
$targetRoot='D:/CS6/Game/Ultimate Multiplayer FPS Kit UE5.4/UltimateMultiplayerFPSKit/Plugins/CS6BombRuntime/Source'
$changed=0
foreach($file in Get-ChildItem -LiteralPath $sourceRoot -File -Recurse){
    # Path.GetRelativePath is .NET Core only; strip the shared prefix so Windows PowerShell 5.1 works too.
    $relative=$file.FullName.Substring($sourceRoot.Length).TrimStart('\','/')
    $target=Join-Path $targetRoot $relative
    if(-not (Test-Path -LiteralPath $target) -or (Get-FileHash -LiteralPath $file.FullName).Hash -ne (Get-FileHash -LiteralPath $target).Hash){
        [System.IO.Directory]::CreateDirectory((Split-Path $target -Parent)) | Out-Null
        Copy-Item -LiteralPath $file.FullName -Destination $target -Force
        (Get-Item -LiteralPath $target).LastWriteTime=Get-Date
        $changed++
    }
}
Write-Output "Runtime source sync: $changed files updated."
