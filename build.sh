mkdir -p bin
cp lib/SDL/SDL2.dll bin
gcc -Wall -Wextra -Werror -o bin/main.exe src/*.c -Iinclude/ -Llib/SDL -lSDL2
exit $?
