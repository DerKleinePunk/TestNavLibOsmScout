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
#include "PathGenerator.h"

PathGenerator::PathGenerator(const osmscout::RouteDescription& description,
	double maxSpeed)
{
	size_t             tickCount = 0;
	double             totalTime = 0.0;
	double             restTime = 0.0;
	auto               currentNode = description.Nodes().begin();
	auto               nextNode = currentNode;
	osmscout::GeoCoord lastPosition;

	assert(currentNode != description.Nodes().end());

	auto time = std::chrono::system_clock::now();

	lastPosition = currentNode->GetLocation();

	{
		osmscout::RouteDescription::MaxSpeedDescriptionRef maxSpeedPath = std::dynamic_pointer_cast<osmscout::RouteDescription::MaxSpeedDescription>(currentNode->GetDescription(osmscout::RouteDescription::WAY_MAXSPEED_DESC));

		if (maxSpeedPath) {
			maxSpeed = maxSpeedPath->GetMaxSpeed();
		}

		steps.emplace_back(time, maxSpeed, lastPosition);
		time += std::chrono::seconds(1);
	}


	++nextNode;

	while (nextNode != description.Nodes().end()) {
		osmscout::RouteDescription::MaxSpeedDescriptionRef maxSpeedPath = std::dynamic_pointer_cast<osmscout::RouteDescription::MaxSpeedDescription>(currentNode->GetDescription(osmscout::RouteDescription::WAY_MAXSPEED_DESC));

		if (maxSpeedPath) {
			maxSpeed = maxSpeedPath->GetMaxSpeed();
		}

		osmscout::Distance distance = osmscout::GetEllipsoidalDistance(currentNode->GetLocation(),
			nextNode->GetLocation());
		double bearing = osmscout::GetSphericalBearingInitial(currentNode->GetLocation(),
			nextNode->GetLocation());

		auto distanceInKilometer = distance.As<osmscout::Kilometer>();
		auto timeInHours = distanceInKilometer / maxSpeed;
		auto timeInSeconds = timeInHours * 60 * 60;

		totalTime += timeInHours;

		// Make sure we do not skip edges in the street
		lastPosition = currentNode->GetLocation();

		while (timeInSeconds > 1.0 - restTime) {
			timeInSeconds = timeInSeconds - (1.0 - restTime);

			double segmentDistance = maxSpeed * (1.0 - restTime) / (60 * 60);

			lastPosition = lastPosition.Add(bearing * 180 / M_PI,
				osmscout::Distance::Of<osmscout::Kilometer>(segmentDistance));

			steps.emplace_back(time, maxSpeed, lastPosition);
			time += std::chrono::seconds(1);

			restTime = 0;

			tickCount++;
		}

		restTime = timeInSeconds;

		++currentNode;
		++nextNode;
	}

	steps.emplace_back(time, maxSpeed, currentNode->GetLocation());
}
