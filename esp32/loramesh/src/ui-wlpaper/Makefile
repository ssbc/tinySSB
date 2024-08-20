# Makefile


all:
	for i in ./img/*.png; do \
	  FNAME=$${i##./img/}; \
	  STEM=$${FNAME%%.png}; \
	  ./imgDeflate.py -i $${i} -n $${STEM}_z -o epd-$${STEM}_z.h; \
        done
	ls -l *_z.h
