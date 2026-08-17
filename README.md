# SAPPHIRE

### Windows run/compile
```
gcc -o zapphire.exe zapphire.c -lraylib -lgdi32 -lwinmm
```
```
.\zapphire.exe
```

### Linux run/compile

```bash
gcc -o zapphire zapphire.c -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
```
```
./zapphire
```

### MacOS run/compile

```bash
gcc -o zapphire  zapphire.c -lraylib -framework OpenGL -framework Cocoa -framework IOKit
```
```
./zapphire
```
