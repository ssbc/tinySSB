// loramesh/src/gesture/dollar-q.h

#define SAMPLING_RESOLUTION 32
#define MAX_INT_COORDINATES 256
// #define LUT_SIZE            64
// #define LUT_SCALE_FACTOR    (MAX_INT_COORDINATES / LUT_SIZE)

// we store a point and its stroke ID as a single 32bit integer
#define to_pt(X,Y,SID) ( (         (uint32_t) (SID)) << 24 | \
                         (0x0fff & (uint32_t)   (Y)) << 12 | \
                         (0x0fff & (uint32_t)   (X))         )
#define pt_x(P)       ((int)((P) & 0x0fff))
#define pt_y(P)       ((int)(((P) >> 12) & 0x0fff))
#define pt_sid(P)     ((int)(((P) >> 24) & 0x0ff))

struct gest_s {
  uint32_t pts[SAMPLING_RESOLUTION];
  char name;
  // short lut[LUT_SIZE][LUT_SIZE];
};

struct gest_s* qd_import(const short *x, const short *y, const short *sid,
                         short cnt);
void qd_free(gest_s *g);
char qd_classify(const struct gest_s *candidate);

// eof
