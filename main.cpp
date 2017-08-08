#include "detectorplotter.h"

#include "H5Cpp.h"
#include "Eigen/Core"

#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace H5;
typedef Eigen::Matrix<double, 3, Eigen::Dynamic> Pixels;

int main(int argc, char *argv[])
{
    H5std_string FILE_NAME;
    if (argc > 1)
    {
        FILE_NAME = argv[1];
    }

    DetectorPlotter nexusFile(FILE_NAME);
    // Get path to all detector groups
    std::vector<Group> detectorGroups = nexusFile.openDetectorGroups ();

    for(std::vector<Group>::iterator iter = detectorGroups.begin (); iter < detectorGroups.end (); iter++)
    {
        // Get the pixel offsets
        Pixels pixelOffsets = nexusFile.getPixelOffsets (*iter);

        // Get the transformations
        Eigen::Transform<double, 3,2> transforms = nexusFile.getTransformations (*iter);

        // Detector Pixels
        Pixels detectorPixels = transforms * pixelOffsets;
        H5std_string name = iter->getObjName();

        //Call write function
        nexusFile.writeToFile (detectorPixels, name);


    }
}
