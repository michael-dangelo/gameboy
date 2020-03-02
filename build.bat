if not exist bin\win\ mkdir bin\win
copy D:\Code\gameboy\lib\SDL\win32\SDL2.dll bin\win\
cl /W4 /WX /ID:\Code\gameboy\include D:\Code\gameboy\lib\SDL\win32\SDL2.lib main.c memory.c cpu.c graphics.c ops.c /link /out:bin\win\main.exe
