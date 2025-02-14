// loramesh/src/q-dollar.cpp

#include "Arduino.h"

// (c) 2025 <christian.tschudin@unibas.ch>

/* an implementation of

 * The academic publication for the $Q recognizer, and what should be
 * used to cite it, is:
 *
 *    Vatavu, R.-D., Anthony, L. and Wobbrock, J.O. (2018). $Q: A super-quick,
 *    articulation-invariant stroke-gesture recognizer for low-resource devices.
 *    Proceedings of the ACM Conference on Human-Computer Interaction with Mobile
 *    Devices and Services (MobileHCI '18). Barcelona, Spain (September 3-6, 2018).
 *    New York: ACM Press. Article No. 23.
 *    https://dl.acm.org/citation.cfm?id=3229434.3229465

 */

#include <math.h>   // float max, sqrt

// ---------------------------------------------------------------------------

#include "dollar-q.h"
#include "dollar-gest.h"

static float dist(int x1, int y1, int x2, int y2)
{
  int dx = x2 - x1;
  int dy = y2 - y1;

  return sqrtf(dx*dx + dy*dy);
}

static float path_len(const short *x, const short *y, const short *sid,
                      short cnt)
{
  float len = 0;

  for (int i = 1; i < cnt; i++)
    if (sid[i] == sid[i-1]) {
      float d = dist(x[i-1], y[i-1], x[i], y[i]);
      len += d;
    }

  return len;
}

static float cloudDistance(const uint32_t *pts1, const uint32_t *pts2,
                    int startIndex, float minSoFar)
{
  // stores point indexes for points from the 2nd cloud
  // that haven't been matched yet
  int indexesNotMatched[SAMPLING_RESOLUTION];
  for (int j = 0; j < SAMPLING_RESOLUTION; j++)
    indexesNotMatched[j] = j;

  // sum of distances between matched points (i.e., the distance
  // between the two clouds)
  float sum = 0;
  // start matching with point startIndex from the 1st cloud
  int i = startIndex;
   // implements weights, decreasing from n to 1
  int weight = SAMPLING_RESOLUTION;
  // indexes the indexesNotMatched[..] array of points from the 2nd cloud
  // that are not matched yet
  int indexNotMatched = 0;

  do {
    int index = -1;
    float minDistance = MAXFLOAT;
    for (int j = indexNotMatched; j < SAMPLING_RESOLUTION; j++) {
      uint32_t p1 = pts1[i], p2 = pts2[indexesNotMatched[j]];
      float d = dist(pt_x(p1), pt_y(p1), pt_x(p2), pt_y(p2));
      if (d < minDistance) {
        minDistance = d;
        index = j;
      }
    }
    indexesNotMatched[index] = indexesNotMatched[indexNotMatched];
    // point indexesNotMatched[index] of the 2nd cloud is now matched
    // to point i of the 1st cloud
    sum += (weight--) * minDistance;
    // weight each distance with a confidence coefficient that
    // decreases from n to 1
    /*
    if (UseEarlyAbandoning) {
      if (sum >= minSoFar) 
        return sum;       // implement early abandoning
    }
    */

    // advance to the next point in the 1st cloud
    i = (i + 1) % SAMPLING_RESOLUTION;
    // update number of points from the 2nd cloud that haven't been matched yet
    indexNotMatched++;
  } while (i != startIndex);
  return sum;
}

static float greedyCloudMatch(const struct gest_s *g1, const struct gest_s *g2,
                              float minSoFar)
{
  // eps controls the number of greedy search trials (eps is in [0..1])
  float eps = 0.5f; 
  int step = floorf(powf(SAMPLING_RESOLUTION, 1.0f - eps));

  for (int i = 0; i < SAMPLING_RESOLUTION; i += step) {
    // direction of matching: gesture1 --> gesture2 starting with index point i
    float d = cloudDistance(g1->pts, g2->pts, i, minSoFar);
    if (d < minSoFar)
      minSoFar = d;
    // direction of matching: gesture2 --> gesture1 starting with index point i
    d = cloudDistance(g2->pts, g1->pts, i, minSoFar);
    if (d < minSoFar)
      minSoFar = d;
  }
  return minSoFar;
}

// ---------------------------------------------------------------------------

struct gest_s* qd_import(const short *x, const short *y, const short *sid,
                         short cnt)
{
  // resample given point series
  /*                        ref
                              | ------ |
                              v s=0..1 v
       x--------x--x---x------x--------x--------x----
       *            *            *            *
   */
  struct gest_s *g = (struct gest_s*) malloc(sizeof(*g));
  g->pts[0] = to_pt(x[0], y[0], sid[0]);
  int numPoints = 1;

  float I = path_len(x, y, sid, cnt);
  I = I / (SAMPLING_RESOLUTION - 1); // distance(interval) between sampling pts
  float D = 0;
  for (int i = 1; i < cnt; i++) {
    if (sid[i] == sid[i-1]) {
      float d = dist(x[i], y[i], x[i-1], y[i-1]);
      if ((D + d) >= I)	{
        uint32_t ref = to_pt(x[i-1], y[i-1], 0);
        while ((D + d) >= I) {
          float s = (I - D) / d;
          if (s > 1) s = 1; else if (s < 0) s = 0;
          float _x = pt_x(ref) + s * (x[i] - x[i-1]);
          float _y = pt_y(ref) + s * (y[i] - y[i-1]);
          ref = to_pt(_x, _y, sid[i]);
          if (numPoints < SAMPLING_RESOLUTION)
            g->pts[numPoints++] = ref;
          else {
            Serial.println("too many sampled points");
            break;
          }
          d = D + d - I;
          D = 0;
        }
        D = d;
      } else
        D += d;
    }
  }
  if (numPoints == (SAMPLING_RESOLUTION - 1)) {
    // sometimes we fall a rounding-error short of adding the last point,
    // so add it if so
    g->pts[numPoints] = to_pt(x[cnt-1], y[cnt-1], sid[cnt-1]);
  }

  // scale
  int minx, miny, maxx, maxy;
  minx = miny = +32000;
  maxx = maxy = -32000;
  for (int i = 0; i < SAMPLING_RESOLUTION; i++) {
    uint32_t pt = g->pts[i];
    if (minx > pt_x(pt)) minx = pt_x(pt);
    if (miny > pt_y(pt)) miny = pt_y(pt);
    if (maxx < pt_x(pt)) maxx = pt_x(pt);
    if (maxy < pt_y(pt)) maxy = pt_y(pt);
  }
  int dx = maxx - minx, dy = maxy - miny;
  int size = dx > dy ? dx : dy;
  for (int i = 0; i < SAMPLING_RESOLUTION; i++) {
    uint32_t pt = g->pts[i];
    g->pts[i] = to_pt(MAX_INT_COORDINATES * (pt_x(pt) - minx) / size,
                      MAX_INT_COORDINATES * (pt_y(pt) - miny) / size,
                      pt_sid(pt));
  }

  // centroid
  int cx = 0, cy = 0;
  for (int i = 0; i < SAMPLING_RESOLUTION; i++) {
    uint32_t pt = g->pts[i];
    cx += pt_x(pt);
    cy += pt_y(pt);
  }
  cx /= SAMPLING_RESOLUTION;
  cy /= SAMPLING_RESOLUTION;

  // translateTo
  for (int i = 0; i < SAMPLING_RESOLUTION; i++) {
    uint32_t pt = g->pts[i];
    int _x = 5 * MAX_INT_COORDINATES / 8 + pt_x(pt) - cx;
    int _y = 5 * MAX_INT_COORDINATES / 8 + pt_y(pt) - cy;
    if (_x < 0) _x = 0;
    if (_y < 0) _y = 0;

    g->pts[i] = to_pt(_x, _y, pt_sid(pt));
  }

  Serial.print("resampled\r\n[");
  for (int i = 0; i < SAMPLING_RESOLUTION; i++) {
    uint32_t pt = g->pts[i];
    if (i != 0) Serial.print(",");
    Serial.printf("(%d,%d,%d)", pt_x(pt), pt_y(pt), pt_sid(pt));
  }
  Serial.println("]");

  return g;
}

void qd_free(struct gest_s *g) {
  free(g);
}

char qd_classify(const struct gest_s *candidate)
{
  // Q dollar version
  float minDistance = MAXFLOAT;
  char name = 0;
  for (int i = 0; i < PROTOTYPES_CNT; i++) {
    float dist = greedyCloudMatch(candidate, prototypes + i, minDistance);
    if (dist < minDistance) {
      minDistance = dist;
      name = prototypes[i].name;
    }
  }
  return name;
}

// eof
