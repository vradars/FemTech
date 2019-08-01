#include "FemTech.h"

double CalculateCentroidAndVolume_C3D8(int e, double *cm) {
  double volume = 0.0, volumeTet;
  for (int k = 0; k < ndim; ++k) {
    cm[k] = 0.0;
  }
  // Split hexahedron to 6 tetrahedrons and compute centroid and volume
  // Each set of 4 indices forms a tetrahedron from hexahedron coords
  const int index[24] = {2, 3, 4, 0, 2, 1, 4, 0, 2, 7, 3, 4, 2, 7, 6, 4, \
                      2, 5, 1, 4, 2, 5, 6, 4};
  double coord[24], tetCoord[12], centroidTet[3];
  for (int i = eptr[e], j = 0; i < eptr[e+1]; ++i, ++j) {
    int index = ndim*connectivity[i];
    for (int k = 0; k < ndim; ++k) {
      coord[j*ndim+k] = coordinates[index+k]+displacements[index+k];
    }
  }
  for (int i = 0; i < 6; ++i) {
    for (int k = 0; k < ndim; ++k) {
      centroidTet[k] = 0.0;
    }
    for (int j = 0; j < 4; ++j) {
      int tetIndex = index[i*4+j];
      for (int k = 0; k < ndim; ++k) {
        tetCoord[j*ndim+k] = coord[tetIndex+k];
        centroidTet[k] += tetCoord[j*ndim+k];
      }
    }
    volumeTet = volumeTetrahedron(tetCoord); 
    volume += volumeTet;
    // // Centroid tetrahedron
    // for (int j = 0; j < 4; ++j) {
    //   for (int k = 0; k < ndim; ++k) {
    //     centroidTet[k] += tetCoord[j*ndim+k];
    //   }
    // }
    for (int k = 0; k < ndim; ++k) {
      cm[k] += volumeTet*centroidTet[k]/4.0;
    }
  }
  for (int k = 0; k < ndim; ++k) {
    cm[k] /= volume;
  }
  return volume;
}