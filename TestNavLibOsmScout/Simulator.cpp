#include <iostream>
#include <osmscout/routing/SimpleRoutingService.h>
#include <osmscout/routing/RoutePostprocessor.h>
#include <osmscout/navigation/Engine.h>
#include <osmscout/navigation/Agents.h>

#include "Simulator.h"
#include "PathGenerator.h"

Simulator::Simulator()
	: routeState(osmscout::RouteStateChangedMessage::State::noRoute)
{
}

void Simulator::ProcessMessages(const std::list<osmscout::NavigationMessageRef>& messages)
{
	for (const auto& message : messages) {
		if (dynamic_cast<osmscout::PositionChangedMessage*>(message.get()) != nullptr) {
			//auto positionChangedMessage=dynamic_cast<osmscout::PositionChangedMessage*>(message.get());
		}
		if (dynamic_cast<osmscout::BearingChangedMessage*>(message.get()) != nullptr) {
			auto bearingChangedMessage = dynamic_cast<osmscout::BearingChangedMessage*>(message.get());

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
	const osmscout::RoutePointsRef& routePoints)
{
	auto locationDescriptionService = std::make_shared<osmscout::LocationDescriptionService>(database);

	routeState = osmscout::RouteStateChangedMessage::State::noRoute;

	osmscout::NavigationEngine engine{
	  std::make_shared<osmscout::PositionAgent>(),
	  std::make_shared<osmscout::CurrentStreetAgent>(locationDescriptionService),
	  std::make_shared<osmscout::RouteStateAgent>(),
	};

	auto initializeMessage = std::make_shared<osmscout::InitializeMessage>(generator.steps.front().time);

	ProcessMessages(engine.Process(initializeMessage));

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