// Some helper with stolen math routines
// These might have come from https://github.com/intel/external-mesa/blob/master/src/mesa/math/m_matrix.c ?
//FIXME: Verify that these are conforming to GL spec

#include <assert.h>

#ifndef NXDK
#define debugPrint(...) printf(__VA_ARGS__)
#define debugPrintFloat(f) printf("%.4f", f)
#else
#include <hal/debug.h>
void debugPrintFloat(float);
#endif

#define PRINT_MATRIX(_m) \
  if (1) { \
    const float* __m = _m; \
    for(int i = 0; i < 4; i++) { \
      for(int j = 0; j < 4; j++) { \
        debugPrint(" "); \
        debugPrintFloat(__m[i*4+j]); \
      } \
      debugPrint("\n"); \
    } \
    CHECK_MATRIX(__m); \
  }

#define CHECK_MATRIX(_mc) \
  if (1) { \
    /*debugPrint(":%d (%s)\n", __LINE__, __FUNCTION__); */ \
    const float* __mc = _mc; \
    for(int i = 0; i < 4; i++) { \
      for(int j = 0; j < 4; j++) { \
        assert(!isinf(__mc[i*4+j])); \
      } \
    } \
  }

#define _USE_MATH_DEFINES
#include <math.h>

typedef double GLdouble;

static void _math_matrix_translate(GLfloat *m, GLfloat x, GLfloat y, GLfloat z ) {
   m[12] = m[0] * x + m[4] * y + m[8]  * z + m[12];
   m[13] = m[1] * x + m[5] * y + m[9]  * z + m[13];
   m[14] = m[2] * x + m[6] * y + m[10] * z + m[14];
   m[15] = m[3] * x + m[7] * y + m[11] * z + m[15];
}

static void matrix_identity(float* out) {
  memset(out, 0x00, 4 * 4 * sizeof(float));
  for(int i = 0; i < 4; i++) {
    out[i*4+i] = 1.0f;
  }
}

static void _math_matrix_scale(GLfloat *m, GLfloat x, GLfloat y, GLfloat z ) {
   m[0] *= x;   m[4] *= y;   m[8]  *= z;
   m[1] *= x;   m[5] *= y;   m[9]  *= z;
   m[2] *= x;   m[6] *= y;   m[10] *= z;
   m[3] *= x;   m[7] *= y;   m[11] *= z;
}


static bool invert(float invOut[16], const float m[16])
{
  float inv[16], det;
  int i;

  inv[0] = m[5]  * m[10] * m[15] - 
           m[5]  * m[11] * m[14] - 
           m[9]  * m[6]  * m[15] + 
           m[9]  * m[7]  * m[14] +
           m[13] * m[6]  * m[11] - 
           m[13] * m[7]  * m[10];

  inv[4] = -m[4]  * m[10] * m[15] + 
            m[4]  * m[11] * m[14] + 
            m[8]  * m[6]  * m[15] - 
            m[8]  * m[7]  * m[14] - 
            m[12] * m[6]  * m[11] + 
            m[12] * m[7]  * m[10];

  inv[8] = m[4]  * m[9] * m[15] - 
           m[4]  * m[11] * m[13] - 
           m[8]  * m[5] * m[15] + 
           m[8]  * m[7] * m[13] + 
           m[12] * m[5] * m[11] - 
           m[12] * m[7] * m[9];

  inv[12] = -m[4]  * m[9] * m[14] + 
             m[4]  * m[10] * m[13] +
             m[8]  * m[5] * m[14] - 
             m[8]  * m[6] * m[13] - 
             m[12] * m[5] * m[10] + 
             m[12] * m[6] * m[9];

  inv[1] = -m[1]  * m[10] * m[15] + 
            m[1]  * m[11] * m[14] + 
            m[9]  * m[2] * m[15] - 
            m[9]  * m[3] * m[14] - 
            m[13] * m[2] * m[11] + 
            m[13] * m[3] * m[10];

  inv[5] = m[0]  * m[10] * m[15] - 
           m[0]  * m[11] * m[14] - 
           m[8]  * m[2] * m[15] + 
           m[8]  * m[3] * m[14] + 
           m[12] * m[2] * m[11] - 
           m[12] * m[3] * m[10];

  inv[9] = -m[0]  * m[9] * m[15] + 
            m[0]  * m[11] * m[13] + 
            m[8]  * m[1] * m[15] - 
            m[8]  * m[3] * m[13] - 
            m[12] * m[1] * m[11] + 
            m[12] * m[3] * m[9];

  inv[13] = m[0]  * m[9] * m[14] - 
            m[0]  * m[10] * m[13] - 
            m[8]  * m[1] * m[14] + 
            m[8]  * m[2] * m[13] + 
            m[12] * m[1] * m[10] - 
            m[12] * m[2] * m[9];

  inv[2] = m[1]  * m[6] * m[15] - 
           m[1]  * m[7] * m[14] - 
           m[5]  * m[2] * m[15] + 
           m[5]  * m[3] * m[14] + 
           m[13] * m[2] * m[7] - 
           m[13] * m[3] * m[6];

  inv[6] = -m[0]  * m[6] * m[15] + 
            m[0]  * m[7] * m[14] + 
            m[4]  * m[2] * m[15] - 
            m[4]  * m[3] * m[14] - 
            m[12] * m[2] * m[7] + 
            m[12] * m[3] * m[6];

  inv[10] = m[0]  * m[5] * m[15] - 
            m[0]  * m[7] * m[13] - 
            m[4]  * m[1] * m[15] + 
            m[4]  * m[3] * m[13] + 
            m[12] * m[1] * m[7] - 
            m[12] * m[3] * m[5];

  inv[14] = -m[0]  * m[5] * m[14] + 
             m[0]  * m[6] * m[13] + 
             m[4]  * m[1] * m[14] - 
             m[4]  * m[2] * m[13] - 
             m[12] * m[1] * m[6] + 
             m[12] * m[2] * m[5];

  inv[3] = -m[1] * m[6] * m[11] + 
            m[1] * m[7] * m[10] + 
            m[5] * m[2] * m[11] - 
            m[5] * m[3] * m[10] - 
            m[9] * m[2] * m[7] + 
            m[9] * m[3] * m[6];

  inv[7] = m[0] * m[6] * m[11] - 
           m[0] * m[7] * m[10] - 
           m[4] * m[2] * m[11] + 
           m[4] * m[3] * m[10] + 
           m[8] * m[2] * m[7] - 
           m[8] * m[3] * m[6];

  inv[11] = -m[0] * m[5] * m[11] + 
             m[0] * m[7] * m[9] + 
             m[4] * m[1] * m[11] - 
             m[4] * m[3] * m[9] - 
             m[8] * m[1] * m[7] + 
             m[8] * m[3] * m[5];

  inv[15] = m[0] * m[5] * m[10] - 
            m[0] * m[6] * m[9] - 
            m[4] * m[1] * m[10] + 
            m[4] * m[2] * m[9] + 
            m[8] * m[1] * m[6] - 
            m[8] * m[2] * m[5];

  det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

  if (det == 0)
      return false;

  det = 1.0 / det;

  for (i = 0; i < 16; i++)
      invOut[i] = inv[i] * det;

  return true;
}

#define A(row,col)  a[(col<<2)+row]
#define B(row,col)  b[(col<<2)+row]
#define P(row,col) product[(col<<2)+row]
static void matmul4( GLfloat *product, const GLfloat *a, const GLfloat *b )
{
  GLint i;
  for (i = 0; i < 4; i++) {
    const GLfloat ai0=A(i,0),  ai1=A(i,1),  ai2=A(i,2),  ai3=A(i,3);
    P(i,0) = ai0 * B(0,0) + ai1 * B(1,0) + ai2 * B(2,0) + ai3 * B(3,0);
    P(i,1) = ai0 * B(0,1) + ai1 * B(1,1) + ai2 * B(2,1) + ai3 * B(3,1);
    P(i,2) = ai0 * B(0,2) + ai1 * B(1,2) + ai2 * B(2,2) + ai3 * B(3,2);
    P(i,3) = ai0 * B(0,3) + ai1 * B(1,3) + ai2 * B(2,3) + ai3 * B(3,3);
  }
}
#undef A
#undef B
#undef P

static void ortho(float* r, GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble nearval, GLdouble farval) {
  //FIXME: Multiply this onto the existing stack - don't overwrite

  GLfloat m[16];

#define M(row,col)  m[col*4+row]
  M(0,0) = 2.0F / (right-left);
  M(0,1) = 0.0F;
  M(0,2) = 0.0F;
  M(0,3) = -(right+left) / (right-left);

  M(1,0) = 0.0F;
  M(1,1) = 2.0F / (top-bottom);
  M(1,2) = 0.0F;
  M(1,3) = -(top+bottom) / (top-bottom);

  M(2,0) = 0.0F;
  M(2,1) = 0.0F;
  M(2,2) = -2.0F / (farval-nearval);
  M(2,3) = -(farval+nearval) / (farval-nearval);

  M(3,0) = 0.0F;
  M(3,1) = 0.0F;
  M(3,2) = 0.0F;
  M(3,3) = 1.0F;
#undef M

  memcpy(r, m, sizeof(m));
}

static void _math_matrix_rotate( GLfloat *m, GLfloat angle, GLfloat x, GLfloat y, GLfloat z )
{
    GLfloat xx, yy, zz, xy, yz, zx, xs, ys, zs, one_c, s, c;

    s = sinf( angle * M_PI / 180.0 );
    c = cosf( angle * M_PI / 180.0 );

#define M(row,col)  m[col*4+row]

    const GLfloat mag = sqrtf(x * x + y * y + z * z);

    if (mag <= 1.0e-4F) {
       /* no rotation, leave mat as-is */
       return;
    }

    x /= mag;
    y /= mag;
    z /= mag;


    /*
     *     Arbitrary axis rotation matrix.
     *
     *  This is composed of 5 matrices, Rz, Ry, T, Ry', Rz', multiplied
     *  like so:  Rz * Ry * T * Ry' * Rz'.  T is the final rotation
     *  (which is about the X-axis), and the two composite transforms
     *  Ry' * Rz' and Rz * Ry are (respectively) the rotations necessary
     *  from the arbitrary axis to the X-axis then back.  They are
     *  all elementary rotations.
     *
     *  Rz' is a rotation about the Z-axis, to bring the axis vector
     *  into the x-z plane.  Then Ry' is applied, rotating about the
     *  Y-axis to bring the axis vector parallel with the X-axis.  The
     *  rotation about the X-axis is then performed.  Ry and Rz are
     *  simply the respective inverse transforms to bring the arbitrary
     *  axis back to its original orientation.  The first transforms
     *  Rz' and Ry' are considered inverses, since the data from the
     *  arbitrary axis gives you info on how to get to it, not how
     *  to get away from it, and an inverse must be applied.
     *
     *  The basic calculation used is to recognize that the arbitrary
     *  axis vector (x, y, z), since it is of unit length, actually
     *  represents the sines and cosines of the angles to rotate the
     *  X-axis to the same orientation, with theta being the angle about
     *  Z and phi the angle about Y (in the order described above)
     *  as follows:
     *
     *  cos ( theta ) = x / sqrt ( 1 - z^2 )
     *  sin ( theta ) = y / sqrt ( 1 - z^2 )
     *
     *  cos ( phi ) = sqrt ( 1 - z^2 )
     *  sin ( phi ) = z
     *
     *  Note that cos ( phi ) can further be inserted to the above
     *  formulas:
     *
     *  cos ( theta ) = x / cos ( phi )
     *  sin ( theta ) = y / sin ( phi )
     *
     *  ...etc.  Because of those relations and the standard trigonometric
     *  relations, it is pssible to reduce the transforms down to what
     *  is used below.  It may be that any primary axis chosen will give the
     *  same results (modulo a sign convention) using thie method.
     *
     *  Particularly nice is to notice that all divisions that might
     *  have caused trouble when parallel to certain planes or
     *  axis go away with care paid to reducing the expressions.
     *  After checking, it does perform correctly under all cases, since
     *  in all the cases of division where the denominator would have
     *  been zero, the numerator would have been zero as well, giving
     *  the expected result.
     */

    xx = x * x;
    yy = y * y;
    zz = z * z;
    xy = x * y;
    yz = y * z;
    zx = z * x;
    xs = x * s;
    ys = y * s;
    zs = z * s;
    one_c = 1.0F - c;

    /* We already hold the identity-matrix so we can skip some statements */
    M(0,0) = (one_c * xx) + c;
    M(0,1) = (one_c * xy) - zs;
    M(0,2) = (one_c * zx) + ys;
/*    M(0,3) = 0.0F; */

    M(1,0) = (one_c * xy) + zs;
    M(1,1) = (one_c * yy) + c;
    M(1,2) = (one_c * yz) - xs;
/*    M(1,3) = 0.0F; */

    M(2,0) = (one_c * zx) - ys;
    M(2,1) = (one_c * yz) + xs;
    M(2,2) = (one_c * zz) + c;
/*    M(2,3) = 0.0F; */

/*
    M(3,0) = 0.0F;
    M(3,1) = 0.0F;
    M(3,2) = 0.0F;
    M(3,3) = 1.0F;
*/
#undef M
}
