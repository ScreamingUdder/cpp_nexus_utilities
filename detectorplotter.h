#ifndef DETECTORPLOTTER_H
#define DETECTORPLOTTER_H

#include "H5Cpp.h"
#include "Eigen/Core"
#include "Eigen/Geometry"

#include <vector>

namespace Nexus {
const H5std_string NX_ENTRY = "NXentry";
const H5std_string NX_INSTRUMENT = "NXinstrument";
const H5std_string NX_DETECTOR = "NXdetector";
}

class DetectorPlotter {
public:
  // Constructor for DetectorPlotter class
  explicit DetectorPlotter(const H5std_string &FILE_NAME);
  // Destructor for DetectorPlotter class
  ~DetectorPlotter();
  // Stores all detector groups in a vector
  std::vector<H5::Group> openDetectorGroups();
  // Stores detectorGroup pixel offsets as Eigen 3xN matrix
  Eigen::Matrix<double, 3, Eigen::Dynamic>
  getPixelOffsets(H5::Group &detectorGroup);
  // Gets the transformations applied to the detector's pixelOffsets
  Eigen::Transform<double, 3, Eigen::Affine>
  getTransformations(H5::Group &detectorGroup);
  // Gets the data from a string dataset
  H5std_string get1DStringDataset(H5std_string &dataset);
  // Writes detector pixel offsets to file
  void writeToFile(Eigen::Matrix<double, 3, Eigen::Dynamic> &detectorPixels,
                   const H5std_string &name);

private:
  // Nexus file
  H5::H5File file;
  // Root group for nexus file
  H5::Group rootGroup;
  // Read dataset into vector
  template <typename valueType>
  std::vector<valueType> get1DDataset(H5std_string &dataset);
  // Opens sub groups matching CLASS_TYPE
  std::vector<H5::Group> openSubGroups(H5::Group &parentGroup,
                                       const H5std_string &CLASS_TYPE);
};

#endif // DETECTORPLOTTER_H
