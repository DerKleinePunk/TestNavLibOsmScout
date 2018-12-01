#pragma once
#include "IPathGenerator.h"

namespace osmscout {
	class RouteDescription;
}

class PathGenerator : public IPathGenerator
{

public:
	PathGenerator(const osmscout::RouteDescription& description, double maxSpeed);
};
