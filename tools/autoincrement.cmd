@echo off
REM auto-increments the game's build number using some crazy hacks on Windows
REM Can't pipe to same file

REM NOTE THIS HAS TO BE HARDCODED TO WHERE VISUAL STUDIO RUNS IT DUE TO VISUAL SHITUDIO!!!!
REM SO CHANGE IT IF YOU MOVE WHERE THIS GETS RUN 

..\tools\win64\awk "/VERSION_BUILD/{$3=$3+1\"\"}1;" common/version.h > common\/version2.h
del common\version.h
ren common\version2.h version.h