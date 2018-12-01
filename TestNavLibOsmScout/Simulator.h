#pragma once
#include <osmscout/navigation/Agents.h>
class IPathGenerator;
class PathGenerator;

class Simulator
{
private:
	osmscout::RouteStateChangedMessage::State routeState{};
	std::string                               lastBearingString;

private:
	void ProcessMessages(const std::list<osmscout::NavigationMessageRef>& messages);

public:
	Simulator();
	void Simulate(const osmscout::DatabaseRef& database,
		const IPathGenerator& generator,
		const osmscout::RoutePointsRef& routePoints);
};
