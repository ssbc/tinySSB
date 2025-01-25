# Directory loramesh/lib

This is where all dependencies (external libraries) are stored, per
board.

We copy the full libraries into these per-board subdirectory so that
users do not have to search and download them separately.

The ```shared``` subdirectory contains those libaries that are
common among several boards. Inside the board-specific subdirectories
we refer to them via symbolic links.

```
loramesh
+- build
|  ...
+- lib
|  +- shared
|  |  ...
|  +- Heltec
|  |  ...
|  +- TBeam
|  |  ... 
|  +- TWatch
|     ...
|  ...
+- src
|  ...
```
