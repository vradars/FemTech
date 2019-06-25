#include "FemTech.h"
#include "blas.h"

#include <assert.h>

/*Delare Functions*/
void ApplyBoundaryConditions(double Time, double dMax, double tMax);
void CustomPlot(double Time);

/* Global Variables/Parameters  - could be moved to parameters.h file?  */
double Time;
int nStep;
int nSteps;
int nPlotSteps = 100;
bool ImplicitStatic = false;
bool ImplicitDynamic = false;
bool ExplicitDynamic = true;
double ExplicitTimeStepReduction = 0.8;
double FailureTimeStep = 1e-11;

int main(int argc, char **argv) {

  // Initialize the MPI environment
  MPI_Init(NULL, NULL);
  // Get the number of processes
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);
  // Get the rank of the process
  MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

  if (ReadInputFile(argv[1])) {
    PartitionMesh();
  }

  AllocateArrays();
	ReadMaterials();

  /* Write inital, undeformed configuration*/
  Time = 0.0;
  nStep = 0;
  WriteVTU(argv[1], nStep, Time);
	CustomPlot(Time);

  if (ImplicitStatic) {
    // Static solution
    double dMax = 0.1; // max displacment in meters
    double tMax = 1.0;
    ShapeFunctions();
    ReadMaterialProperties();
    ApplyBoundaryConditions(Time, dMax, tMax);
    Assembly((char *)"stiffness");
    ApplySteadyBoundaryConditions();
    SolveSteadyImplicit();
    Time = tMax;
    nStep = 1;
    /* Write final, deformed configuration*/
    WriteVTU(argv[1], nStep, Time);
  } else if (ImplicitDynamic) {
    // Dynamic Implicit solution using Newmark's scheme for time integration
    double dt = 0.1;
    double tMax = 1.0;
    double dMax = 0.1; // max displacment in meters
    ShapeFunctions();
    ReadMaterialProperties();
    Time = 1.0;
    ApplyBoundaryConditions(Time, dMax, tMax);
    Assembly((char *)"stiffness");
    Assembly((char *)"mass");
    /* beta and gamma of Newmark's scheme */
    double beta = 0.25;
    double gamma = 0.5;
    SolveUnsteadyNewmarkImplicit(beta, gamma, dt, tMax, argv[1]);
  } else if (ExplicitDynamic) {
    // Dynamic Explcit solution using....
    double dt=2.5e-6;
    double tMax = 1.0; // max simulation time in seconds
    double dMax = 0.001; // max displacment in meters
    double Time = 0.0;
    int time_step_counter = 0;
    int plot_counter = 0;
    /** Central Difference Method - Beta and Gamma */
    // double beta = 0;
    // double gamma = 0.5;

    ShapeFunctions();
    ReadMaterialProperties();
    /*  Step-1: Calculate the mass matrix similar to that of belytschko. */
    Assembly((char *)"mass"); // Add Direct-lumped as an option
    LumpMassMatrix();
    // Include effect of elements on other processors
    updateMassMatrixNeighbour();

    /* Step-2: getforce step from Belytschko */
    GetForce(); // Calculating the force term.

    /* Obtain dt, according to Belytschko dt is calculated at end of getForce */
    dt = ExplicitTimeStepReduction * StableTimeStep();

    /* Step-3: Calculate accelerations */
    CalculateAccelerations();

    nSteps = (int)(tMax / dt);
    int nsteps_plot = (int)(nSteps / nPlotSteps);

    printf("inital dt = %3.3e, nSteps = %d, nsteps_plot = %d\n", dt, nSteps,
           nsteps_plot);

    // Save old displacements
    // memcpy(displacements_prev, displacements, ndim*nnodes*sizeof(double));

    /* Step-4: Time loop starts....*/
    time_step_counter = time_step_counter + 1;
    double t_n = 0.0;
    const int nDOF = ndim * nnodes;
    printf("------------------------------- Loop ----------------------------\n");
    printf("Time : %f, tmax : %f\n", Time, tMax);
    while (Time < tMax) {
      double t_n = Time;
      double t_np1 = Time + dt;
      Time = t_np1;          /*Update the time by adding full time step */
      printf("Time : %f, dt=%3.3e, tmax : %f\n", Time, dt, tMax);
      double dt_nphalf = dt; // equ 6.2.1
      double t_nphalf = 0.5 * (t_np1 + t_n); // equ 6.2.1

      /* Step 5 from Belytschko Box 6.1 - Update velocity */
      for (int i = 0; i < nDOF; i++) {
        velocities_half[i] =
            velocities[i] + (t_nphalf - t_n) * accelerations[i];
      }

      // Store old displacements for energy computation
      memcpy(displacements_prev, displacements, ndim * nnodes * sizeof(double));
      for (int i = 0; i < ndim * nnodes; i++) {
        displacements[i] = displacements[i] + dt_nphalf * velocities_half[i];
      }
      /* Step 6 Enforce displacement boundary Conditions */
      ApplyBoundaryConditions(Time, dMax, tMax);

      /* Step - 8 from Belytschko Box 6.1 - Calculate net nodal force*/
      GetForce(); // Calculating the force term.

      /* Step - 9 from Belytschko Box 6.1 - Calculate Accelerations */
      CalculateAccelerations(); // Calculating the new accelerations from total
                                // nodal forces.

      /** Step- 10 - Second Partial Update of Nodal Velocities */
      for (int i = 0; i < ndim * nnodes; i++) {
        velocities[i] =
            velocities_half[i] + (t_np1 - t_nphalf) * accelerations[i];
      }

      /** Step - 11 Checking* Energy Balance */
      CheckEnergy();

      if (time_step_counter % nsteps_plot == 0) {
        plot_counter = plot_counter + 1;
        printf("------Plot %d: WriteVTU\n", plot_counter);
        WriteVTU(argv[1], plot_counter, Time);
				CustomPlot(Time);

#ifdef DEBUG
        if (debug) {
          printf("DEBUG : Printing Displacement Solution\n");
          for (int i = 0; i < nnodes; ++i) {
            for (int j = 0; j < ndim; ++j) {
              printf("%15.6E", displacements[i * ndim + j]);
            }
            printf("\n");
          }
        }
#endif //DEBUG
      }
      time_step_counter = time_step_counter + 1;
      dt = ExplicitTimeStepReduction * StableTimeStep();

    } // end explcit while loop

    nStep = plot_counter;
  } // end if ExplicitDynamic
#ifdef DEBUG
  if (debug) {
    printf("DEBUG : Printing Displacement Solution\n");
    for (int i = 0; i < nnodes; ++i) {
      for (int j = 0; j < ndim; ++j) {
        printf("%15.6E", displacements[i*ndim+j]);
      }
      printf("\n");
    }
  }
#endif //DEBUG

  /* Below are things to do at end of program */
  if (world_rank == 0) {
    WritePVD(argv[1], nStep, Time);
  }
  FreeArrays();
  MPI_Finalize();
  return 0;
}

void ApplyBoundaryConditions(double Time, double dMax, double tMax) {
  double tol = 1e-5;
  int count = 0;
  double AppliedDisp;

  // Apply Ramped Displacment
  if (ExplicitDynamic || ImplicitDynamic) {
    AppliedDisp = Time * (dMax / tMax);
  } else if (ImplicitStatic) {
    AppliedDisp = dMax;
  }

  for (int i = 0; i < nnodes; i++) {
    // if x value = 0, constrain node to x plane (0-direction)
    if (fabs(coordinates[ndim * i + 0] - 0.0) < tol) {
      boundary[ndim * i + 0] = 1;
      displacements[ndim * i + 0] = 0.0;
      count = count + 1;
    }
    // if y coordinate = 0, constrain node to y plane (1-direction)
    if (fabs(coordinates[ndim * i + 1] - 0.0) < tol) {
      boundary[ndim * i + 1] = 1;
      displacements[ndim * i + 1] = 0.0;
      count = count + 1;
    }
    // if z coordinate = 0, constrain node to z plane (2-direction)
    if (fabs(coordinates[ndim * i + 2] - 0.0) < tol) {
      boundary[ndim * i + 2] = 1;
      displacements[ndim * i + 2] = 0.0;
      count = count + 1;
    }
    // if y coordinate = 1, apply disp. to node = 0.1 (1-direction)
    if (fabs(coordinates[ndim * i + 1] - 0.005) < tol) {
      boundary[ndim * i + 1] = 1;
      count = count + 1;
      // note that this may have to be divided into
      // diplacement increments for both implicit and
      // explicit solver. In the future this would be
      // equal to some time dependent function i.e.,
      // CalculateDisplacement to get current increment out
      //  displacment to be applied.
      displacements[ndim * i + 1] = AppliedDisp;
    }
  }
  printf("Time = %10.5e, Applied Disp = %10.5e\n",Time,AppliedDisp);
  return;
}

void CustomPlot(double Time){
	double tol = 1e-5;
	FILE *datFile;
  int x=0;
  int y=1;
  int z=2;

	if(fabs(Time - 0.0) < 1e-16){
		datFile=fopen("plot.dat", "w");
		fprintf(datFile,"# Results for Node ?\n");
		fprintf(datFile,"# Time  DispX    DispY   DispZ\n");
		fprintf(datFile,"%11.3e %11.3e  %11.3e  %11.3e\n",0.0,0.0,0.0,0.0);

	}else{
		datFile=fopen("plot.dat", "a");
		for (int i = 0; i < nnodes; i++) {
	    if (fabs(coordinates[ndim * i + x] - 0.005) < tol &&
	 				fabs(coordinates[ndim * i + y] - 0.005) < tol &&
					fabs(coordinates[ndim * i + z] - 0.005) < tol ) {

						fprintf(datFile,"%11.3e %11.3e  %11.3e  %11.3e\n",Time,
											displacements[ndim * i + x],
											displacements[ndim * i + y],
											displacements[ndim * i + z] );
			}
	 	}
	}

	fclose(datFile);
	return;
}
