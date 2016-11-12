////////////////////////////////////////////////////////////
//
// Volume Classifier
//
////////////////////////////////////////////////////////////
#ifndef VOLUMECLASSIFIER_H
#define VOLUMECLASSIFIER_H

#include <vector>
#include <string>
#include "ConnectedComponents.h"
#include "navigator_msgs/PerceptionObject.h"

void VolumeClassifier(objectMessage &object)
{
	auto h = object.maxHeightFromLidar;
	auto x = object.scale.x;
	auto y = object.scale.y;
	auto z = object.scale.z;
	//If we classify something as shooter/scanthecode, leave it alone
	if (object.locked || object.name == navigator_msgs::PerceptionObject::START_GATE_BUOY || !object.real) {
		return;
	}

	std::vector<std::string> names = {"dock","shooter","scan_the_code","totem","buoy"};
	double volumes[5][8] = { 	{0.7,	1.25,	6.0,	7.0,	6.0,	7.0,	2.5,	3.25}, //dock (NOT TESTED!)
								{0.7,	1.25,	2.75,	4.75,	2.75,	4.75,	2.5,	3.25}, //shooter
								{-0.15,	0.5,	1.4,	2.75,	1.4,	2.75,	1.5,	1.75}, //scan_the_code (needs testing!)
								{-0.6,	0.1,	1.4,	2.75,	1.4,	2.75,	1.0,	1.5}, //totems
								{-1.5,-0.75,	0.5,	1.25,	0.5,	1.25,	0.0,	1.0} }; //buoy
	auto match = false;
	for (auto ii = 0; ii < names.size(); ++ii) {
		if ( object.name == names[ii] || ( h >= volumes[ii][0] 	&& h <= volumes[ii][1]	&& x >= volumes[ii][2] && x <= volumes[ii][3] && y >= volumes[ii][4] && y <= volumes[ii][5] && z >= volumes[ii][6]	&& z <= volumes[ii][7]) ) {
			object.name = names[ii];
			match = true;
			break;
		}
	}
	if (!match) { object.name = "unknown"; }
}
#endif