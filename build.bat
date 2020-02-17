if not exist bin\win\ mkdir bin\win
cl /W4 /WX main.c memory.c cpu.c /link /out:bin\win\main.exe
