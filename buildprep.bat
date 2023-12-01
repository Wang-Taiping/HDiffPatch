@echo off

if not exist builds\vc\HDiffPatch.sln (
	echo Folder error.
	exit 1
)

rmdir /s /q ..\libmd5 >nul 2>nul
git clone --branch master --single-branch --depth=1 https://github.com/sisong/libmd5.git ..\libmd5

rmdir /s /q ..\lzma >nul 2>nul
git clone --branch fix-make-build --single-branch --depth=1 https://github.com/sisong/lzma.git ..\lzma

rmdir /s /q ..\zstd >nul 2>nul
git clone --branch deltaUpdateDict --single-branch --depth=1 https://github.com/sisong/zstd.git ..\zstd

rmdir /s /q ..\zlib >nul 2>nul
git clone --branch master --single-branch --depth=1 https://github.com/sisong/zlib.git ..\zlib

rmdir /s /q ..\bzip2 >nul 2>nul
git clone --branch master --single-branch --depth=1 https://github.com/sisong/bzip2.git ..\bzip2

exit 0