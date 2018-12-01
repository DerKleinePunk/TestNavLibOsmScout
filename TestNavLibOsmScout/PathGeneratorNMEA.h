#pragma once
#include "IPathGenerator.h"

namespace osmscout {
	class RouteDescription;
}

class PathGeneratorNMEA : public IPathGenerator
{
	std::string _filename;
	double _maxSpeed;
public:
	PathGeneratorNMEA(const std::string& filename, double maxSpeed);
	bool GenerateSteps();
};
