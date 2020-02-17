if not exist bin\win\ mkdir bin\win
copy D:\Projects\gameboy\lib\SDL\win32\SDL2.dll bin\win\
cl /W4 /WX /ID:\Projects\gameboy\include D:\Projects\gameboy\lib\SDL\win32\SDL2.lib main.c memory.c cpu.c graphics.c /link /out:bin\win\main.exe
