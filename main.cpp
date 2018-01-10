#include "detectorplotter.h"

#include <iostream>
#include <map>

using namespace H5;
typedef Eigen::Matrix<double, 3, Eigen::Dynamic> Pixels;

int main(int argc, char *argv[]) {
  H5std_string FILE_NAME;
  if (argc > 1) {
    FILE_NAME = argv[1];
  }

  DetectorPlotter nexusFile(FILE_NAME);
  // Get path to all detector groups
  std::vector<Group> detectorGroups = nexusFile.openDetectorGroups();

  for (auto group : detectorGroups) {
    // Get the pixel offsets
    Pixels pixelOffsets = nexusFile.getPixelOffsets(group);

    // Get the transformations
    Eigen::Transform<double, 3, 2> transforms =
        nexusFile.getTransformations(group);

    // Detector Pixels
    Pixels detectorPixels = transforms * pixelOffsets;
    H5std_string name = group.getObjName();

    // Call write function
    nexusFile.writeToFile(detectorPixels, name);
  }
}
