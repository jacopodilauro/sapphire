# MINECRAFT

### Windows run/compile
```
gcc -o minecraft.exe minecraft.c -lraylib -lgdi32 -lwinmm

.\minecraft.exe.
```

### Linux run/compile

```bash
gcc -o minecraft minecraft.c -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
./minecraft
```

### MacOS run/compile

```bash
gcc -o minecraft minecraft.c -lraylib -framework OpenGL -framework Cocoa -framework IOKit
./minecraft
```