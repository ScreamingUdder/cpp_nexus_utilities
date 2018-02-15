#include "detectorplotter.h"

#include <iostream>
#include <fstream>

using namespace H5;

using Pixels = Eigen::Matrix<double, 3, Eigen::Dynamic>;

namespace {
const H5G_obj_t GROUP_TYPE = static_cast<H5G_obj_t>(0);
const H5std_string NX_CLASS = "NX_class";
const H5std_string X_PIXEL_OFFSET = "x_pixel_offset";
const H5std_string Y_PIXEL_OFFSET = "y_pixel_offset";
const H5std_string Z_PIXEL_OFFSET = "z_pixel_offset";
const H5std_string DEPENDS_ON = "depends_on";
const H5std_string NO_DEPENDENCY = ".";
// Transformation types
const H5std_string TRANSFORMATION_TYPE = "transformation_type";
const H5std_string TRANSLATION = "translation";
const H5std_string ROTATION = "rotation";
const H5std_string VECTOR = "vector";
// Radians
const double PI = 3.1415926535;
const double DEGREES_IN_CIRCLE = 360;
}

DetectorPlotter::DetectorPlotter(const H5std_string &FILE_NAME) {
  H5File nexusFile(FILE_NAME, H5F_ACC_RDONLY);
  DetectorPlotter::file = nexusFile;
  DetectorPlotter::rootGroup = DetectorPlotter::file.openGroup("/");
}

DetectorPlotter::~DetectorPlotter() {
  DetectorPlotter::file.close();
}

std::vector<Group>
DetectorPlotter::openSubGroups(Group &parentGroup,
                               const H5std_string &CLASS_TYPE) {
  std::vector<Group> subGroups;

  // Iterate over parentGroup's children
  for (hsize_t i = 0; i < parentGroup.getNumObjs(); i++) {
    // Determine if child is a group
    if (parentGroup.getObjTypeByIdx(i) == GROUP_TYPE) {
      std::string childPath = parentGroup.getObjnameByIdx(i);
      // Open the sub group
      Group childGroup = parentGroup.openGroup(childPath);
      // Iterate through attributes to find NX_class
      for (uint32_t j = 0; j < childGroup.getNumAttrs(); j++) {
        // Test attribute at current index for NX_class
        Attribute attribute = childGroup.openAttribute(j);
        if (attribute.getName() == NX_CLASS) {
          // Get attribute data type
          DataType dataType = attribute.getDataType();

          // Get the NX_class type
          H5std_string classType;
          attribute.read(dataType, classType);

          // If group of correct type, append to subGroup vector
          if (classType == CLASS_TYPE) {
            subGroups.push_back(childGroup);
          }
        }
      }
    }
  }
  return subGroups;
}

std::vector<Group> DetectorPlotter::openDetectorGroups() {
  std::vector<Group> rawDataGroupPaths = DetectorPlotter::openSubGroups(
      DetectorPlotter::rootGroup, Nexus::NX_ENTRY);

  // Open all instrument groups within rawDataGroups
  std::vector<Group> instrumentGroupPaths;
  for (auto group : rawDataGroupPaths) {
    std::vector<Group> instrumentGroups =
        DetectorPlotter::openSubGroups(group, Nexus::NX_INSTRUMENT);
    instrumentGroupPaths.insert(instrumentGroupPaths.end(),
                                instrumentGroups.begin(),
                                instrumentGroups.end());
  }
  // Open all detector groups within instrumentGroups
  std::vector<Group> detectorGroupPaths;
  for (auto group : instrumentGroupPaths) {
    // Open sub detector groups
    std::vector<Group> detectorGroups =
        DetectorPlotter::openSubGroups(group, Nexus::NX_DETECTOR);
    // Append to detectorGroups vector
    detectorGroupPaths.insert(detectorGroupPaths.end(), detectorGroups.begin(),
                              detectorGroups.end());
  }
  // Return the detector groups
  return detectorGroupPaths;
}

Pixels DetectorPlotter::getPixelOffsets(Group &detectorGroup) {
  H5std_string detectorName = detectorGroup.getObjName();

  Pixels offsetData;
  std::vector<double> xValues, yValues, zValues;
  for (hsize_t i = 0; i < detectorGroup.getNumObjs(); i++) {
    H5std_string objName = detectorGroup.getObjnameByIdx(i);
    H5std_string objPath = detectorName;
    objPath += "/";
    objPath += objName;
    if (objName == X_PIXEL_OFFSET) {
      xValues = DetectorPlotter::get1DDataset<double>(objPath);
    }
    if (objName == Y_PIXEL_OFFSET) {
      yValues = DetectorPlotter::get1DDataset<double>(objPath);
    }
    if (objName == Z_PIXEL_OFFSET) {
      zValues = DetectorPlotter::get1DDataset<double>(objPath);
    }
  }

  // Determine size of dataset
  int rowLength = 0;
  bool xEmpty = xValues.empty();
  bool yEmpty = yValues.empty();
  bool zEmpty = zValues.empty();

  if (!xEmpty)
    rowLength = static_cast<int>(xValues.size());
  else if (!yEmpty)
    rowLength = static_cast<int>(yValues.size());
  // Need at least 2 dimensions to define points
  else
    return offsetData;

  // Default x,y,z to zero if no data provided
  offsetData.resize(3, rowLength);
  offsetData.setZero(3, rowLength);

  if (!xEmpty) {
    for (int i = 0; i < rowLength; i++)
      offsetData(0, i) = xValues[i];
  }
  if (!yEmpty) {
    for (int i = 0; i < rowLength; i++)
      offsetData(1, i) = yValues[i];
  }
  if (!zEmpty) {
    for (int i = 0; i < rowLength; i++)
      offsetData(2, i) = zValues[i];
  }
  return offsetData;
}

template <typename valueType>
std::vector<valueType> DetectorPlotter::get1DDataset(H5std_string &dataset) {

  DataSet data = DetectorPlotter::file.openDataSet(dataset);
  DataType dataType = data.getDataType();
  DataSpace dataSpace = data.getSpace();

  std::vector<valueType> values;
  values.resize(static_cast<uint64_t>(dataSpace.getSelectNpoints()));
  data.read(values.data(), dataType, dataSpace);

  return values;
}

H5std_string DetectorPlotter::get1DStringDataset(H5std_string &dataset) {
  DataSet data = DetectorPlotter::file.openDataSet(dataset);
  DataSpace dataspace = data.getSpace();
  H5std_string value;

  data.read(value, data.getDataType(), dataspace);

  return value;
}

Eigen::Transform<double, 3, Eigen::Affine>
DetectorPlotter::getTransformations(Group &detectorGroup) {
  H5std_string detectorPath = detectorGroup.getObjName();
  H5std_string depends_on = detectorPath + "/" + DEPENDS_ON;
  H5std_string dependencyPath = DetectorPlotter::get1DStringDataset(depends_on);

  // Initialise transformation holder as zero-degree rotation
  Eigen::Transform<double, 3, Eigen::Affine> transforms;
  Eigen::Vector3d axis(1.0, 0.0, 0.0);
  transforms = Eigen::AngleAxisd(0.0, axis);

  // Breaks when no more dependencies (dependency = ".")
  // Transformations must be applied in opposite order to direction of discovery
  while (dependencyPath != NO_DEPENDENCY) {
    DataSet transformation = DetectorPlotter::file.openDataSet(dependencyPath);

    double magnitude = DetectorPlotter::get1DDataset<double>(dependencyPath)[0];
    Eigen::Vector3d transformVector;
    H5std_string transformType;

    for (uint32_t i = 0; i < transformation.getNumAttrs(); i++) {
      Attribute attribute = transformation.openAttribute(i);

      // Get next dependency
      if (attribute.getName() == DEPENDS_ON) {
        DataType dataType = attribute.getDataType();
        attribute.read(dataType, dependencyPath);
      }
      // Get transform type
      if (attribute.getName() == TRANSFORMATION_TYPE) {
        DataType dataType = attribute.getDataType();
        attribute.read(dataType, transformType);
      }
      getTransformationVector(transformVector, attribute);
    }
    if (transformType == TRANSLATION) {
      // Translation = magnitude*unitVector
      transformVector *= magnitude;
      Eigen::Translation3d translation(transformVector);
      transforms = translation * transforms;
    } else if (transformType == ROTATION) {
      double angle = degrees_to_radians(magnitude);
      Eigen::AngleAxisd rotation(angle, transformVector);
      transforms = rotation * transforms;
    }
  }
  return transforms;
}

double DetectorPlotter::degrees_to_radians(double angle_in_degrees) const {
  return angle_in_degrees * 2 * PI / DEGREES_IN_CIRCLE;
}

void DetectorPlotter::getTransformationVector(
    Eigen::Vector3d &transformVector, const Attribute &attribute) const {
  if (attribute.getName() == VECTOR) {
    std::vector<double> unitVector;
    DataType dataType = attribute.getDataType();

    DataSpace dataSpace = attribute.getSpace();
    unitVector.resize(static_cast<uint64_t>(dataSpace.getSelectNpoints()));

    attribute.read(dataType, unitVector.data());
    transformVector(0) = unitVector[0];
    transformVector(1) = unitVector[1];
    transformVector(2) = unitVector[2];
  }
}

void DetectorPlotter::writeToFile(Pixels &detectorPixels,
                                  const H5std_string &name) {
  std::ofstream dataFile;
  H5std_string filename = name.substr(name.find_last_of('/') + 1) + ".txt";
  dataFile.open(filename, std::ofstream::out);

  dataFile << detectorPixels << std::endl;
  dataFile.close();
}
