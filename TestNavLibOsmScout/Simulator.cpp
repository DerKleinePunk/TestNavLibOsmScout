#include <iostream>
#include <osmscout/routing/SimpleRoutingService.h>
#include <osmscout/routing/RoutePostprocessor.h>
#include <osmscout/navigation/Engine.h>
#include <osmscout/navigation/Agents.h>

#include "Simulator.h"
#include "PathGenerator.h"
#include <iomanip>

static std::string TimeToString(double time)
{
	std::ostringstream stream;
	stream << std::setfill(' ') << std::setw(2) << (int)std::floor(time) << ":";
	time -= std::floor(time);
	stream << std::setfill('0') << std::setw(2) << (int)floor(60 * time + 0.5);
	return stream.str();
}

Simulator::Simulator()
	: routeState(osmscout::RouteStateChangedMessage::State::noRoute), _navigation(nullptr), _onRoute(false),
	  _errorCount(0) {
}

Simulator::~Simulator() {
	if(_streamGpxFile.is_open()) {
		_streamGpxFile << "</gpx>" << std::endl;
		_streamGpxFile.close();
	}
}

void Simulator::ProcessMessages(const std::list<osmscout::NavigationMessageRef>& messages)
{
	for (const auto& message : messages) {
		if (dynamic_cast<osmscout::PositionChangedMessage*>(message.get()) != nullptr) {
			const auto positionChangedMessage=dynamic_cast<osmscout::PositionChangedMessage*>(message.get());
			//std::cout << positionChangedMessage->currentPosition.GetDisplayText() <<  " Speed " << positionChangedMessage->currentSpeed << std::endl;
			/*osmscout::ClosestRoutableObjectResult routableResult = router->GetClosestRoutableObject(location,
				routingProfile->GetVehicle(),
				osmscout::Distance::Of<osmscout::Meter>(100));*/
			auto minDistance = 0.0;
			auto result = _navigation.UpdateCurrentLocation(positionChangedMessage->currentPosition, minDistance);
			const auto desc = _navigation.nextWaypointDescription();
			//std::cout << desc.distance.AsMeter() << std::endl;
			auto distance = osmscout::GetEllipsoidalDistance(positionChangedMessage->currentPosition, desc.location);
			const auto distanceInMeter = distance.As<osmscout::Meter>();
			if (distanceInMeter <= 100 && _lastInstructions != desc.instructions) {
				std::cout << "Distance to route: " << minDistance << " ?" << std::endl;
				std::cout << "Distance to destination: " << _navigation.GetDistance().AsMeter() << std::endl;
				std::cout << "Time to destination: " << TimeToString(_navigation.GetDuration()) << std::endl;
				std::cout << "Next routing instructions: " << desc.instructions << std::endl;
				_lastInstructions = desc.instructions;
			}

			if(result != _onRoute) {
				if(result) {
					std::cout << "route" << std::endl;
					_streamGpxFile << "\t<wpt lat=\"" << desc.location.GetLat() << "\" lon=\"" << desc.location.GetLon() << "\">" << std::endl;
					_streamGpxFile << "\t\t<name>Route found "<< std::to_string(_errorCount) << "</name>" << std::endl;
					_streamGpxFile << "\t\t<fix>2d</fix>" << std::endl;
					_streamGpxFile << "\t</wpt>" << std::endl;
					_streamGpxFile << "\t<wpt lat=\"" << positionChangedMessage->currentPosition.GetLat() << "\" lon=\"" << positionChangedMessage->currentPosition.GetLon() << "\">" << std::endl;
					_streamGpxFile << "\t\t<name>Car Point" << std::to_string(_errorCount) << "</name>" << std::endl;
					_streamGpxFile << "\t\t<fix>2d</fix>" << std::endl;
					_streamGpxFile << "\t</wpt>" << std::endl;
				} else {
					std::cout << "route verlassen" << std::endl;
					_streamGpxFile << "\t<wpt lat=\"" << desc.location.GetLat() << "\" lon=\"" << desc.location.GetLon() << "\">" << std::endl;
					_streamGpxFile << "\t\t<name>Route lost " << std::to_string(_errorCount) << "</name>" << std::endl;
					_streamGpxFile << "\t\t<fix>2d</fix>" << std::endl;
					_streamGpxFile << "\t</wpt>" << std::endl;
					_streamGpxFile << "\t<wpt lat=\"" << positionChangedMessage->currentPosition.GetLat() << "\" lon=\"" << positionChangedMessage->currentPosition.GetLon() << "\">" << std::endl;
					_streamGpxFile << "\t\t<name>Car Point" << std::to_string(_errorCount) << "</name>" << std::endl;
					_streamGpxFile << "\t\t<fix>2d</fix>" << std::endl;
					_streamGpxFile << "\t</wpt>" << std::endl;
				}
				_onRoute = result;
				_errorCount++;
			}
		}
		if (dynamic_cast<osmscout::BearingChangedMessage*>(message.get()) != nullptr) {
			const auto bearingChangedMessage = dynamic_cast<osmscout::BearingChangedMessage*>(message.get());

			auto bearingString = bearingChangedMessage->hasBearing ? osmscout::BearingDisplayString(bearingChangedMessage->bearing) : "";
			if (lastBearingString != bearingString) {
				std::cout << osmscout::TimestampToISO8601TimeString(bearingChangedMessage->timestamp)
					<< " Bearing: " << bearingString << std::endl;

				lastBearingString = bearingString;
			}
		}
		else if (dynamic_cast<osmscout::StreetChangedMessage*>(message.get()) != nullptr) {
			auto streetChangedMessage = dynamic_cast<osmscout::StreetChangedMessage*>(message.get());

			std::cout << osmscout::TimestampToISO8601TimeString(streetChangedMessage->timestamp)
				<< " Street name: " << streetChangedMessage->name << std::endl;
		}
		else if (dynamic_cast<osmscout::RouteStateChangedMessage*>(message.get()) != nullptr) {
			auto routeStateChangedMessage = dynamic_cast<osmscout::RouteStateChangedMessage*>(message.get());

			if (routeStateChangedMessage->state != routeState) {

				routeState = routeStateChangedMessage->state;


				std::cout << osmscout::TimestampToISO8601TimeString(routeStateChangedMessage->timestamp)
					<< " RouteState: ";

				switch (routeState) {
				case osmscout::RouteStateChangedMessage::State::noRoute:
					std::cout << "no route";
					break;
				case osmscout::RouteStateChangedMessage::State::onRoute:
					std::cout << "on route";
					break;
				case osmscout::RouteStateChangedMessage::State::offRoute:
					std::cout << "off route";
					break;
				}

				std::cout << std::endl;
			}
		}
	}
}

void Simulator::Simulate(const osmscout::DatabaseRef& database,
	const IPathGenerator& generator,
	const osmscout::RoutePointsRef& routePoints,
	const osmscout::RouteDescriptionRef& description)
{
	auto locationDescriptionService = std::make_shared<osmscout::LocationDescriptionService>(database);

	routeState = osmscout::RouteStateChangedMessage::State::noRoute;

	osmscout::NavigationEngine engine{
	  std::make_shared<osmscout::PositionAgent>(),
	  std::make_shared<osmscout::CurrentStreetAgent>(locationDescriptionService),
	  std::make_shared<osmscout::RouteStateAgent>(),
	};

	const auto initializeMessage = std::make_shared<osmscout::InitializeMessage>(generator.steps.front().time);

	ProcessMessages(engine.Process(initializeMessage));
	_navigation = new osmscout::NavigationDescription<osmscout::NodeDescription>;
	_navigation.SetSnapDistance(osmscout::Distance::Of<osmscout::Meter>(100.0));
	_navigation.SetRoute(description.get());

	_streamGpxFile.open("routeLife.gpx", std::ofstream::trunc);
	_streamGpxFile.precision(8);
	_streamGpxFile << R"(<?xml version="1.0" encoding="UTF-8" standalone="no" ?>)" << std::endl;
	_streamGpxFile << R"(<gpx xmlns="http://www.topografix.com/GPX/1/1" creator="TestNavLibOsmScout" version="0.1" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:schemaLocation="http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd">)"
		<< std::endl;

	_streamGpxFile << "\t<wpt lat=\"" << generator.steps.front().coord.GetLat() << "\" lon=\"" << generator.steps.front().coord.GetLon() << "\">" << std::endl;
	_streamGpxFile << "\t\t<name>Start</name>" << std::endl;
	_streamGpxFile << "\t\t<fix>2d</fix>" << std::endl;
	_streamGpxFile << "\t</wpt>" << std::endl;

	_streamGpxFile << "\t<wpt lat=\"" << generator.steps.back().coord.GetLat() << "\" lon=\"" << generator.steps.back().coord.GetLon() << "\">" << std::endl;
	_streamGpxFile << "\t\t<name>Target</name>" << std::endl;
	_streamGpxFile << "\t\t<fix>2d</fix>" << std::endl;
	_streamGpxFile << "\t</wpt>" << std::endl;

	// TODO: Simulator possibly should not send this message on start but later on to simulate driver starting before
	// getting route
	auto routeUpdateMessage = std::make_shared<osmscout::RouteUpdateMessage>(generator.steps.front().time, routePoints);

	ProcessMessages(engine.Process(routeUpdateMessage));

	for (const auto& point : generator.steps) {
		auto gpsUpdateMessage = std::make_shared<osmscout::GPSUpdateMessage>(point.time, point.coord, point.speed);

		ProcessMessages(engine.Process(gpsUpdateMessage));

		auto timeTickMessage = std::make_shared<osmscout::TimeTickMessage>(point.time);

		ProcessMessages(engine.Process(timeTickMessage));
	}
}