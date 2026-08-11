# MINECRAFT

### Windows Run

Run `gcc -o minecraft.exe .\minecraft.c -lraylib -lgdi32 -lwinmm` to compile

and `.\minecraft.exe` to execute.

### Linux Run

```bash
gcc -o minecraft minecraft.c -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
./minecraft
```

### MacOS Run

```bash
gcc -o minecraft minecraft.c -lraylib -framework OpenGL -framework Cocoa -framework IOKit
./minecraft
```