# SAPPHIRE

![Screenshot](img/Screenshot.png)

## Commands
- **Move:** `W`, `A`, `S`, `D`, to run or flight faster press `ctrl`
- **Visual:** `mouse`
- **Flight mode:** `F`
- **Flight** Press `space` to increase, `Shift` to decrease
- **Third person:** `V`
- **Debug information:** `F3`
- **Body visualization:** `F1`

## How to compile

### Windows
```
gcc -o sapphire.exe sapphire.c -lraylib -lgdi32 -lwinmm
```
```
.\sapphire.exe
```

### Linux

```bash
gcc -o sapphire sapphire.c -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
```
```
./sapphire
```

### MacOS

```bash
gcc -o sapphire  sapphire.c -lraylib -framework OpenGL -framework Cocoa -framework IOKit
```
```
./sapphire
```
