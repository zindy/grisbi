@echo off
:: Change the next lines to choose which gtk+ version you download.
:: Choose runtime version posterior to dev version
::    Get this file name from http://ftp.gnome.org/pub/gnome/binaries/win32/gtk+/2.16/
::    Don't include the extension
SET GTK_DEV_FILE=gtk+-bundle_2.16.5-20090731_win32
::    Get this file name from http://sourceforge.net/projects/gtk-win/files
SET GTK_BIN_FILE=gtk2-runtime-2.16.5-2009-08-06-ash.exe

:: The rest of the script should do the rest
SET CURRENT_DIR=%CD%
cd ..
wget -nc -c "http://sourceforge.net/projects/gtk-win/files/GTK+ Runtime Environment/GTK+ 2.16/%GTK_BIN_FILE%/download"
./%GTK_BIN_FILE%
cd %CURRENT_DIR%
mkdir target
cd target
wget -nc -c http://ftp.gnome.org/pub/gnome/binaries/win32/gtk+/2.16/%GTK_DEV_FILE%.zip
:: Checking if already unzipped
:: If the readme file exists for this version, update the unzip
IF EXIST gtk-dev\%GTK_DEV_FILE%.README.txt (
	echo Found gtk-dev directory with the same gtk+ version.
	echo Updating the files...
	unzip -uo %GTK_DEV_FILE% -d gtk-dev
	echo Done updating the files in gtk-dev!
)
:: If the readme file for the current version does not exist, remove the dir, and unzip again
IF NOT EXIST gtk-dev\%GTK_DEV_FILE%.README.txt (
	echo Found gtk-dev directory with a different gtk+ version.
	echo Deleting gtk-dev for compatibility reasons...
	rmdir /S /Q gtk-dev
	echo Done deleting gtk-dev directory!
	echo Unzipping downloaded archive...
	unzip %GTK_DEV_FILE% -d gtk-dev
	echo Done unzipping archive!
)
SET GTK_DEV_FILE=
cd ..
call generate.cmd