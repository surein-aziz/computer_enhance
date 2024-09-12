@echo off

ctime -begin haversine_rel.ctm

set CommonCompilerFlags=-O2 -nologo -fp:fast -fp:except- -GR- -EHa- -Oi -WX -W4 -wd4702 -wd4505 -wd4996 -wd4244 -wd4201 -wd4189 -wd4100 -FC -Z7 -DDEBUG

set CommonCompilerFlags= %CommonCompilerFlags% 
set CommonLinkerFlags= -incremental:no

IF NOT EXIST bin_rel mkdir bin_rel
pushd bin_rel
cl %CommonCompilerFlags% ..\source\winmain.cpp /link %CommonLinkerFlags%
set LastError=%ERRORLEVEL%
popd

ctime -end haversine_rel.ctm %LastError%