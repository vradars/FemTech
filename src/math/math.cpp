#include "FemTech.h"
#include "blas.h"

void inverse3x3Matrix(double* mat, double* invMat, double* det) {
  // Mat and invMat are 1d arrays with colum major format for storing matrix
  // Compute matrix determinant
  double detLocal = mat[0] * (mat[4] * mat[8] - mat[5] * mat[7]) -
              mat[3] * (mat[1] * mat[8] - mat[7] * mat[2]) +
              mat[6] * (mat[1] * mat[5] - mat[4] * mat[2]);

  double invdet = 1 / detLocal;

  invMat[0] = (mat[4] * mat[8] - mat[5] * mat[7]) * invdet;
  invMat[3] = (mat[6] * mat[5] - mat[3] * mat[8]) * invdet;
  invMat[6] = (mat[3] * mat[7] - mat[6] * mat[4]) * invdet;
  invMat[1] = (mat[7] * mat[2] - mat[1] * mat[8]) * invdet;
  invMat[4] = (mat[0] * mat[8] - mat[6] * mat[2]) * invdet;
  invMat[7] = (mat[1] * mat[6] - mat[0] * mat[7]) * invdet;
  invMat[2] = (mat[1] * mat[5] - mat[2] * mat[4]) * invdet;
  invMat[5] = (mat[2] * mat[3] - mat[0] * mat[5]) * invdet;
  invMat[8] = (mat[0] * mat[4] - mat[1] * mat[3]) * invdet;

  (*det) = detLocal;
}

double normOfCrossProduct(double *a, double *b) {
  double z = a[0]*b[1]-a[1]*b[0];
  if (ndim == 3) {
    double x = a[1]*b[2]-a[2]*b[1];
    double y = -a[0]*b[2]+a[2]*b[0];
    return sqrt(x*x + y*y + z*z);
  } else {
    if (ndim == 2) {
      return z;
    }
  }
  return 0.0;
}

double tripleProduct(double *s, double *a, double *b) {
  return s[2]*(a[0]*b[1]-a[1]*b[0])+
         s[0]*(a[1]*b[2]-a[2]*b[1])-
         s[1]*(a[0]*b[2]-a[2]*b[0]);
}

void crossProduct(double* a, double* b, double* result) {
  result[0] = a[1]*b[2]-a[2]*b[1];
  result[1] = -a[0]*b[2]+a[2]*b[0];
  result[2] = a[0]*b[1]-a[1]*b[0];
}
double norm3D(double *a) {
  return sqrt(a[0]*a[0]+a[1]*a[1]+a[2]*a[2]);
}
double dotProduct3D(double *a, double *b) {
  return (a[0]*b[0]+a[1]*b[1]+a[2]*b[2]);
}
// n and x are assumed to be three dimensional
// Source: http://scipp.ucsc.edu/~haber/ph216/rotation_12.pdf
// After rotation the results are stored in the input matrix
// Theta is assumed to be in radians
void rotate3d(double *n, double theta, double *xin) {
  double x, y, z;
  const double cth = cos(theta);
  const double sth = sin(theta);
  const double mcth_m1 = 1.0-cth;
  x = (cth+n[0]*n[0]*mcth_m1)*xin[0] + (n[0]*n[1]*mcth_m1-n[2]*sth)*xin[1] + \
      (n[0]*n[2]*mcth_m1+n[1]*sth)*xin[2];
  y = (cth+n[1]*n[1]*mcth_m1)*xin[1] + (n[1]*n[2]*mcth_m1-n[0]*sth)*xin[2] + \
      (n[0]*n[1]*mcth_m1+n[2]*sth)*xin[0];
  z = (cth+n[2]*n[2]*mcth_m1)*xin[2] + (n[0]*n[2]*mcth_m1-n[1]*sth)*xin[0] + \
      (n[2]*n[1]*mcth_m1+n[0]*sth)*xin[1];
  xin[0] = x; xin[1] = y; xin[2] = z;
}
// Create the rotation matrix in row major format for faster multiplication with
// position vector
void get3dRotationMatrix(double *n, double theta, double mat[3][3]) {
  const double cth = cos(theta);
  const double sth = sin(theta);
  const double mcth_m1 = 1.0-cth;

  mat[0][0] = cth+n[0]*n[0]*mcth_m1;
  mat[0][1] = n[0]*n[1]*mcth_m1-n[2]*sth;
  mat[0][2] = n[0]*n[2]*mcth_m1+n[1]*sth;

  mat[1][0] = n[0]*n[1]*mcth_m1+n[2]*sth;
  mat[1][1] = cth+n[1]*n[1]*mcth_m1;
  mat[1][2] = n[1]*n[2]*mcth_m1-n[0]*sth;

  mat[2][0] = n[0]*n[2]*mcth_m1-n[1]*sth;
  mat[2][1] = n[2]*n[1]*mcth_m1+n[0]*sth;
  mat[2][2] = cth+n[2]*n[2]*mcth_m1;
}
