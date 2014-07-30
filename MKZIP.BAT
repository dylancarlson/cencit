@echo off
set files=`*.c *.h *.asm *.rsp config.atm makefile mkzip.bat`
iff exist ccit.zip then
	 pkzip -f ccit
else
	 pkzip ccit %files
endiff
set files=

