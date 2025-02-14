# Visualization of Dollar-Q ($Q) Normalized Pointe Clouds

and generation of C code for the $Q gesture recognizer.

Relevant files:
```
loramesh/src/gesture/dollar-q.h
loramesh/src/gesture/dollar-q.cpp
loramesh/src/gesture/dollar-gest.h
```

## Input

After captuting the draw moveements, the tinySSB app for the T-Watch
puts the normalized $Q point cloud to the serial port in a
Python-compatible format e.g.,

```
[(131,326,0),(131,311,0),(131,291,0),...,(249,70,0)]
```

It's a list of 32 3-tuples each carrying an x, y and a stroke-id (SID) value.

Consecutive tuples with the same SID value means that these are the
points for the same independent stroke. Several strokes can be encoded in
one list with 32 3-tuples, each stroke having a different SID.

The above Python list must be manually added to the ```gallery.py``` program
(list ```gest```) and a corresponding letter be defined in the string
```char_vect``` at the same index position.

It is possible to have several normalized sequences for the same letter.

## Output

The ```gallery.py``` program renders each 32-element list in its own
box and outputs a PDF file with the name ```gallery.pdf```.

At the same time it outputs to stdout the C definition for inclusion
in the tinySSB code for the T-Watch (```dollar-gest.h```)

----

