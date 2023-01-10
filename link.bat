@echo off

set bin_dir=T:\cmake\LuaVis\bin

set asset_dir=T:\projekte\LuaVis\assets
set gfx_dir_1=T:\temp\adrian\Dataset1png
set gfx_dir_2=T:\temp\adrian\NewDataset

set releases=Debug MinSizeRel Release RelWithDebInfo
set dir_links=assets\gfx_1 assets\gfx_2 assets
set file_links=config.json run.bat

cd /D %bin_dir%

echo Removing old symlinks

(for %%d in (%releases%) do (
    (for %%l in (%dir_links%) do (
        if exist %%d\%%l (
            if exist %%d\%%l (rmdir %%d\%%l)
        )
    ))
    (for %%l in (%file_links%) do (
        if exist %%d\%%l (
            if exist %%d\%%l (del %%d\%%l)
        )
    ))
))

echo Creating symlinks

(for %%d in (%releases%) do (
    if exist %%d (
        echo   for %%d
        cd %%d
        call :link
        cd ..
    )
))

pause
exit /b

:link
mklink /D assets %asset_dir%

cd assets
if not exist gfx_1 (
    mklink /D gfx_1 %gfx_dir_1%
    mklink /D gfx_2 %gfx_dir_2%
)
cd ..

mklink config.json ..\config.json
mklink run.bat ..\run.bat
goto :eof
