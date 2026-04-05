#include <8x8Matrix.h>

const bool flag1[64] = {
  1, 0, 0, 0, 0, 0, 0, 0,
  1, 1, 1, 0, 0, 1, 0, 0,
  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,
  1, 0, 0, 1, 1, 0, 0, 1,
  1, 0, 0, 0, 0, 0, 0, 0,
  1, 0, 0, 0, 0, 0, 0, 0
};

const bool flag2[64] = {
  1, 0, 0, 0, 0, 0, 0, 1,
  1, 1, 0, 1, 1, 0, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,
  1, 0, 1, 0, 0, 1, 1, 0,
  1, 0, 0, 0, 0, 0, 0, 0,
  1, 0, 0, 0, 0, 0, 0, 0
};

Matrix matrix(6, 7, 8, 9, 10, 11, 12, 13, A0, A1, A2, A3, A4, A5, 4, 5);

/**
 * Arduino example setup hook; this sketch needs no extra initialization.
 */
void setup() {}

/**
 * Continuously refresh the matrix while showing the boolean-array flag pattern.
 */
void loop() {
  matrix.setBoolArray(flag1);
  matrix.scanForMillis(200);

  matrix.setBoolArray(flag2);
  matrix.scanForMillis(200);
}
