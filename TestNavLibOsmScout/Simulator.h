#pragma once
#include <osmscout/navigation/Agents.h>
#include "NavigationDescription.h"
#include <fstream>
class IPathGenerator;
class PathGenerator;

class Simulator
{
	osmscout::RouteStateChangedMessage::State routeState{};
	std::string                               lastBearingString;
	osmscout::Navigation<osmscout::NodeDescription> _navigation;
	std::string _lastInstructions;
	bool _onRoute;
	std::ofstream _streamGpxFile;
	int _errorCount;
	osmscout::GeoCoord _lastGeopos;
	void ProcessMessages(const std::list<osmscout::NavigationMessageRef>& messages);

public:
	Simulator();
	~Simulator();
	void Simulate(const osmscout::DatabaseRef& database,
		const IPathGenerator& generator,
		const osmscout::RoutePointsRef& routePoints,
		const osmscout::RouteDescriptionRef& description);
};
