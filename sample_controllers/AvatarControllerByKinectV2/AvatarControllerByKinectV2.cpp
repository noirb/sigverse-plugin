/*
 * AvatarControllerByKinectV2
 *
 *  Created on: 2015/03/12
 *      Author: Nozaki
 */

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <sigverse/plugin/common/sensor/SensorData.h>
#include <cmath>
#include "AvatarControllerByKinectV2.h"

///@brief Initialize this controller.
void AvatarControllerByKinectV2::onInit(InitEvent &evt)
{
	this->readIniFileAndInitialize();

	SimObj *myself = getObj(myname());

	this->kinectV2DeviceManager.initPositionAndRotation(myself);
}


///@brief Movement of the robot.
double AvatarControllerByKinectV2::onAction(ActionEvent &evt)
{
	bool kinectV2Available = checkService(this->kinectV2DeviceManager.serviceName);

	if (kinectV2Available && this->kinectV2DeviceManager.service == NULL)
	{
		this->kinectV2DeviceManager.service = connectToService(this->kinectV2DeviceManager.serviceName);
	}
	else if (!kinectV2Available && this->kinectV2DeviceManager.service != NULL)
	{
		this->kinectV2DeviceManager.service = NULL;
	}

	return 1.0;
}

void AvatarControllerByKinectV2::onRecvMsg(RecvMsgEvent &evt)
{
	try
	{
		std::string allMsg = evt.getMsg();

//		std::cout << "msg:" << allMsg << std::endl;

		// Decode message to sensor data of kinect v2.
		std::map<std::string, std::vector<std::string> > sensorDataMap = KinectV2SensorData::decodeSensorData(allMsg);

		if (sensorDataMap.find(MSG_KEY_DEV_TYPE) == sensorDataMap.end()){ return; }
		if(sensorDataMap[MSG_KEY_DEV_TYPE][0]     !=this->kinectV2DeviceManager.deviceType    ){ return; }
		if(sensorDataMap[MSG_KEY_DEV_UNIQUE_ID][0]!=this->kinectV2DeviceManager.deviceUniqueID){ return; }

		KinectV2SensorData sensorData;
		sensorData.setSensorData(sensorDataMap);

		ManNiiPosture posture = KinectV2DeviceManager::convertSensorData2ManNiiPosture(sensorData);

		// Set SIGVerse quaternions and positions
		SimObj *obj = getObj(myname());
		this->kinectV2DeviceManager.setRootPosition(obj, sensorData.rootPosition);
		KinectV2DeviceManager::setJointQuaternions2ManNii(obj, posture, sensorData);
	}
	catch(SimObj::Exception &err)
	{
		LOG_MSG(("Exception: %s", err.msg()));
	}
	catch(...)
	{
		std::cout << "some error occurred." << std::endl;
	}
}


///@brief Read parameter file.
void AvatarControllerByKinectV2::readIniFileAndInitialize()
{
	try
	{
		std::ifstream ifs(parameterFileName.c_str());

		// Parameter file is "not" exists.
		if (ifs.fail())
		{
			std::cout << "Not exist : " << parameterFileName << std::endl;
			exit(-1);
		}

		// Parameter file is exists.
		std::cout << "Read " << parameterFileName << std::endl;
		boost::property_tree::ptree pt;
		boost::property_tree::read_ini(parameterFileName, pt);

		std::string serviceName    = pt.get<std::string>(PARAMETER_FILE_KEY_GENERAL_SERVICE_NAME);
		std::string deviceType     = pt.get<std::string>(PARAMETER_FILE_KEY_GENERAL_DEVICE_TYPE);
		std::string deviceUniqueID = pt.get<std::string>(PARAMETER_FILE_KEY_GENERAL_DEVICE_UNIQUE_ID);

		double      scaleRatio        = pt.get<double>(paramFileKeyKinectV2ScaleRatio);
		std::string sensorDataModeStr = pt.get<std::string>(paramFileKeyKinectV2SensorDataMode);

		std::cout << PARAMETER_FILE_KEY_GENERAL_SERVICE_NAME     << ":" << serviceName    << std::endl;
		std::cout << PARAMETER_FILE_KEY_GENERAL_DEVICE_TYPE      << ":" << deviceType     << std::endl;
		std::cout << PARAMETER_FILE_KEY_GENERAL_DEVICE_UNIQUE_ID << ":" << deviceUniqueID << std::endl;
		std::cout << paramFileKeyKinectV2SensorDataMode << ":" << sensorDataModeStr << std::endl;
		std::cout << paramFileKeyKinectV2ScaleRatio     << ":" << scaleRatio << std::endl;

		this->kinectV2DeviceManager = KinectV2DeviceManager(serviceName, deviceType, deviceUniqueID, scaleRatio);

		// Set sensor data mode.
		KinectV2SensorData::setSensorDataMode(sensorDataModeStr);
	}
	catch (boost::exception &ex)
	{
		std::cout << parameterFileName << " ERR :" << *boost::diagnostic_information_what(ex) << std::endl;
		exit(-1);
	}
	catch (...)
	{
		std::cout << "Some exception occurred in readIniFileAndInitialize()." << std::endl;
		exit(-1);
	}
}


extern "C" Controller * createController()
{
	return new AvatarControllerByKinectV2;
}


