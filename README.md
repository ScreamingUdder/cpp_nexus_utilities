# cpp_nexus_utilities

Parses the OFF instrument definition format embedded in NeXus files.
The pixel offsets can be plotted using the provided script.

# Building using CMake
If necessary, the path to the HDF5 header files should be updated to the current include path.  CMake can then be configured
and generated, before building.

# Running
The resulting executable ("detector_plotter") can be run from the command line with a single argument.
The argument should describe the ABSOLUTE path to the chosen nexus file (which must contain an OFF instrument definition).
The program writes the pixel offsets for each detector to seperate files ("detector_name.txt").  The files are located in the
build directory.

# Plotting
"plotter.py" can be run from the build directory, and will plot the pixel offsets (in the X-Y and X-Z planes).
