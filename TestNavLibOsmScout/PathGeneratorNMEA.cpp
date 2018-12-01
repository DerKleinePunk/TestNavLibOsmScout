/**
 * Initialize the step list with way points on the route in strict
 * 1 second intervals.
 *
 * @param description
 *    Routing description
 * @param maxSpeed
 *    Max speed to use if no explicit speed limit in given on a route segment
 */
#include <chrono>
#include <osmscout/routing/Route.h>
#include <osmscout/util/Geometry.h>
#include "PathGeneratorNMEA.h"
#include <iostream>
#include "utils/easylogging++.h"
#include "NMEADecoder.h"

PathGeneratorNMEA::PathGeneratorNMEA(const std::string& filename, double maxSpeed) {
	//Todo Implement
	_filename = filename;
	_maxSpeed = maxSpeed;
}

bool PathGeneratorNMEA::GenerateSteps() {
	NMEADecoder decoder;

	std::ifstream file(_filename);
	if (!file.is_open()) {
		LOG(ERROR) << "Error Open File";
		std::cout << "Error Open File";
		return false;
	}

	auto time = std::chrono::system_clock::now();
	osmscout::GeoCoord lastPos(0, 0);

	std::string line;
	while (std::getline(file, line)) {
		if (decoder.Decode(line)) {
			//LOG(DEBUG) << "Line Read and decode " << line;
			if (decoder.IsPositionValid() && decoder.IsSpeedValid()) {
				const auto curtargetLat = decoder.GetLatitude();
				const auto curtargetLon = decoder.GetLongitude();
				const auto speed = decoder.GetSpeed();
				const osmscout::GeoCoord currentPos(curtargetLat, curtargetLon);
				if(lastPos.GetLat() == 0) {
					steps.emplace_back(time, speed, currentPos);
					lastPos.Set(curtargetLat, curtargetLon);
				} else {
					auto distance = GetEllipsoidalDistance(currentPos, lastPos);
					const auto distanceInKilometer = distance.As<osmscout::Kilometer>();
					const auto timeInHours = distanceInKilometer / speed;
					int64_t timeInSeconds = timeInHours * 60 * 60;
					time += std::chrono::seconds(timeInSeconds);
					steps.emplace_back(time, speed, currentPos);
				}
			}
		}
	}
	return false;
}
