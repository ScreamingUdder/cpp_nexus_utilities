﻿#include "detectorplotter.h"

#include "Eigen/Core"
#include "Eigen/Geometry"

#include "H5Cpp.h"

#include<string>
#include<vector>

//Just for debugging
#include <iostream>

using namespace H5;

//Eigen typedefs
typedef Eigen::Matrix<double, 3, Eigen::Dynamic> Pixels;

namespace
{
     const H5G_obj_t GROUP_TYPE = static_cast<H5G_obj_t> (0);
     const H5std_string NX_CLASS = "NX_class";
     const H5std_string X_PIXEL_OFFSET = "x_pixel_offset";
     const H5std_string Y_PIXEL_OFFSET = "y_pixel_offset";
     const H5std_string Z_PIXEL_OFFSET = "z_pixel_offset";
     const H5std_string DEPENDS_ON = "depends_on";
     const H5std_string NO_DEPENDENCY = ".";
     //Transformation types
     const H5std_string TRANSFORMATION_TYPE = "transformation_type";
     const H5std_string TRANSLATION = "translation";
     const H5std_string ROTATION =  "rotation";
     const H5std_string VECTOR = "vector";
     //Radians
     const static double PI = 3.1415926535;
     const static double DEGREES_IN_CIRCLE = 360;
}

// Constructor
DetectorPlotter::DetectorPlotter(H5std_string FILE_NAME)
{
    // Open nexus file
    H5File nexusFile(FILE_NAME, H5F_ACC_RDONLY);
    DetectorPlotter::file = nexusFile;
    //Initialise root group
    DetectorPlotter::rootGroup = DetectorPlotter::file.openGroup("/");
}

// Destructor
DetectorPlotter::~DetectorPlotter()
{
    // Close file
    DetectorPlotter::file.close ();
}

// Open sub groups of choosen classType
std::vector<Group> DetectorPlotter::openSubGroups(Group &parentGroup, H5std_string CLASS_TYPE)
{
        std::vector<Group> subGroups;

        // Iterate over parentGroup's children
        for ( int i = 0; i < parentGroup.getNumObjs (); i++)
        {
            // Determine if child is a group
            if(parentGroup.getObjTypeByIdx (i) == GROUP_TYPE)
            {
                std::string childPath = parentGroup.getObjnameByIdx (i);
                // Open the sub group
                Group childGroup = parentGroup.openGroup (childPath);
                // Iterate through attributes to find NX_class
                for( int i = 0; i < childGroup.getNumAttrs (); i++)
                {
                    // Test attribute at current index for NX_class
                    Attribute attribute = childGroup.openAttribute (i);
                    if(attribute.getName() == NX_CLASS)
                    {
                        // Get attribute data type
                        DataType dataType = attribute.getDataType ();

                        // Get the NX_class type
                        H5std_string classType;
                        attribute.read(dataType, classType);

                        // If group of correct type, append to subGroup vector
                        if(classType == CLASS_TYPE)
                        {
                            subGroups.push_back (childGroup);
                        }
                    }
                }
            }
        }
        return subGroups;
}

// Open all detector groups into a vector
std::vector<Group> DetectorPlotter::openDetectorGroups ()
{
    std::vector<Group> rawDataGroupPaths = DetectorPlotter::openSubGroups (DetectorPlotter::rootGroup, Nexus::NX_ENTRY);

    //Open all instrument groups within rawDataGroups
    std::vector<Group> instrumentGroupPaths;
    for (std::vector<Group>::iterator iter = rawDataGroupPaths.begin (); iter < rawDataGroupPaths.end (); iter++)
    {
        std::vector<Group> instrumentGroups = DetectorPlotter::openSubGroups (*iter, Nexus::NX_INSTRUMENT);
        instrumentGroupPaths.insert (instrumentGroupPaths.end (), instrumentGroups.begin (), instrumentGroups.end ());
    }
    //Open all detector groups within instrumentGroups
    std::vector<Group> detectorGroupPaths;
    for(std::vector<Group>::iterator iter = instrumentGroupPaths.begin(); iter < instrumentGroupPaths.end (); iter++)
    {
        //Open sub detector groups
        std::vector<Group> detectorGroups = DetectorPlotter::openSubGroups (*iter, Nexus::NX_DETECTOR);
        //Append to detectorGroups vector
        detectorGroupPaths.insert (detectorGroupPaths.end (), detectorGroups.begin (), detectorGroups.end ());
    }
    //Return the detector groups
    return detectorGroupPaths;
}

//Function to return the (x,y,z) offsets of pixels in the chosen detector
Pixels DetectorPlotter::getPixelOffsets (Group &detectorGroup)
{
    H5std_string detectorName = detectorGroup.getObjName();

    //Initialise matrix
    Pixels offsetData;
    std::vector<double> xValues, yValues, zValues;
    for (int i = 0; i < detectorGroup.getNumObjs (); i++)
    {
        H5std_string objName = detectorGroup.getObjnameByIdx (i);
        H5std_string objPath = detectorName + "/" + objName;
        if(objName == X_PIXEL_OFFSET)
        {
            xValues = DetectorPlotter::get1DDataset<double>(objPath);
        }
        if(objName == Y_PIXEL_OFFSET)
        {
            yValues = DetectorPlotter::get1DDataset<double>(objPath);
        }
        if(objName == Z_PIXEL_OFFSET)
        {  
            zValues = DetectorPlotter::get1DDataset<double> (objPath);
        }
    }

    // Determine size of dataset
    int rowLength = 0;
    bool xEmpty = xValues.empty();
    bool yEmpty = yValues.empty();
    bool zEmpty = zValues.empty();

    if(!xEmpty)
        rowLength = static_cast<int>(xValues.size ());
    else if(!yEmpty)
        rowLength = static_cast<int>(yValues.size());
    // Need at least 2 dimensions to define points
    else
        return offsetData;

    // Default x,y,z to zero if no data provided
    offsetData.resize (3,rowLength);
    offsetData.setZero(3, rowLength);

    if(!xEmpty)
    {
        for(int i = 0; i < rowLength; i++)
            offsetData(0,i) = xValues[i];
    }
    if(!yEmpty)
    {
        for(int i = 0; i < rowLength; i++)
            offsetData(1,i) = yValues[i];
    }
    if(!zEmpty)
    {
        for(int i = 0; i < rowLength; i++)
            offsetData(2,i) = zValues[i];
    }
    // Return the coordinate matrix
    return offsetData;
}



//Function to read in a dataset into a vector
template<typename valueType>
std::vector<valueType> DetectorPlotter::get1DDataset(H5std_string &dataset)
{

    //Open data set
    DataSet data = DetectorPlotter::file.openDataSet (dataset);

    //Get data type
    DataType dataType = data.getDataType ();

    //Get data size
    DataSpace dataSpace = data.getSpace ();

    //Create vector to hold data
    std::vector<valueType> values;
    values.resize (dataSpace.getSelectNpoints ());

    //Read data into vector
    data.read (values.data (), dataType, dataSpace);

    //Return the data vector
    return values;
}

H5std_string DetectorPlotter::get1DStringDataset(H5std_string &dataset)
{
    //Open data set
    DataSet data = DetectorPlotter::file.openDataSet (dataset);
    //Get size
    DataSpace dataspace = data.getSpace ();
    //Initialize string
    H5std_string value;

    //Read into string
    data.read(value, data.getDataType (), dataspace);

    return value;
}


//Function to get the transformations from the nexus file, and create the Eigen transform object
Eigen::Transform<double, 3, Eigen::Affine> DetectorPlotter::getTransformations(Group &detectorGroup)
{
    //Get path to depends_on file
    H5std_string detectorPath = detectorGroup.getObjName ();
    H5std_string depends_on = detectorPath + "/" + DEPENDS_ON;

    //Get absolute dependency path
    H5std_string dependency = DetectorPlotter::get1DStringDataset(depends_on);

    //Initialise transformation holder as zero-degree rotation
    Eigen::Transform<double, 3, Eigen::Affine> transforms;
    Eigen::Vector3d axis(1.0,0.0,0.0);
    transforms = Eigen::AngleAxisd(0.0, axis);

    //Breaks when no more dependencies (dependency = ".")
    //Transformations must be applied in opposite order to direction of discovery
    while(dependency != NO_DEPENDENCY)
    {
        //Open the transformation data set
        DataSet transformation = DetectorPlotter::file.openDataSet (dependency);

        //Get magnitude of current transformation
        double magnitude = DetectorPlotter::get1DDataset<double>(dependency)[0];
        //Container for unit vector of transformation
        Eigen::Vector3d transformVector;
        //Container for transformation type
        H5std_string transformType;

        for(int i = 0; i < transformation.getNumAttrs (); i++)
        {
            //Open attribute at current index
            Attribute attribute = transformation.openAttribute (i);

            //Get next dependency
            if(attribute.getName () == DEPENDS_ON)
            {
                DataType dataType = attribute.getDataType ();
                attribute.read(dataType, dependency);
            }
            //Get transform type
            if(attribute.getName () == TRANSFORMATION_TYPE)
            {
                DataType dataType = attribute.getDataType ();
                attribute.read(dataType, transformType);
            }
            //Get unit vector for transformation
            if(attribute.getName () == VECTOR)
            {
                std::vector<double> unitVector;
                DataType dataType = attribute.getDataType ();

                //Get data size
                DataSpace dataSpace = attribute.getSpace ();

                //Resize vector to hold data
                unitVector.resize (dataSpace.getSelectNpoints ());

                //Read the data into Eigen vector
                attribute.read(dataType, unitVector.data());
                transformVector(0) = unitVector[0];
                transformVector(1) = unitVector[1];
                transformVector(2) = unitVector[2];
            }
        }
        //Transform_type = translation
        if(transformType == TRANSLATION)
        {
            //Translation = magnitude*unitVector
            transformVector *= magnitude;
            Eigen::Translation3d translation(transformVector);
            transforms *= translation;
        }
        else if(transformType == ROTATION)
        {
            //Convert angle from degrees to radians
            double angle = magnitude * 2*PI/DEGREES_IN_CIRCLE;

            Eigen::AngleAxisd rotation(angle, transformVector);
            transforms *= rotation;
        }

    }
    return transforms;
}
