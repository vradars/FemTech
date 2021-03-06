#include "FemTech.h"
#include "blas.h"

void InternalForceUpdate(int e, int gp, double *force) {
  int cSize = 6;
  int nNodes = nShapeFunctions[e];
  double *fintGQ = (double*)calloc(nNodes*ndim, sizeof(double));
  int bColSize = nNodes*ndim;
  int Bsize = bColSize*cSize;
  double *B = (double*)calloc(Bsize, sizeof(double));
  // Calculate B matrix for each shape function
  for (int k = 0; k < nNodes; ++k) {
    StrainDisplacementMatrix(e, gp, k, &(B[6*ndim*k]));
  }
  double* sigma = &(pk2[pk2ptr[e]+6*gp]);
  // Compute B^T*sigma^n
  dgemv_(chy, &cSize, &bColSize, &one, B, &cSize, sigma, \
      &oneI, &zero, fintGQ, &oneI);
  // Add to fint
  int wIndex = gpPtr[e]+gp;
  // Following Belytschko equation 4.9.22
  const double preFactor = gaussWeights[wIndex]*detJacobian[wIndex];
  for (int k = 0; k < bColSize; ++k) {
    force[k] += preFactor*fintGQ[k];
  }
  free(fintGQ);
  free(B);
	return;
}
