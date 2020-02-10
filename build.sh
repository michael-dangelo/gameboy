gcc -Wall -Wextra -Werror main.c cpu.c memory.c -o build/main && build/main | awk '/unknown/ { unknown += 1 } { total += 1} END { printf "%.1f%% known\n", ((total - unknown) / total) * 100; }'
