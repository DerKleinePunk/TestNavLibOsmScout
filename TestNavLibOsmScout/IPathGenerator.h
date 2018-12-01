#pragma once
#include <list>
#include <osmscout/GeoCoord.h>
#include <osmscout/AreaAreaIndex.h>

namespace osmscout {
	class RouteDescription;
}

class IPathGenerator
{
public:
	virtual ~IPathGenerator() = default;

	struct Step
	{
		osmscout::Timestamp time;
		double              speed;
		osmscout::GeoCoord  coord;

		Step(const osmscout::Timestamp& time,
			double speed,
			const osmscout::GeoCoord& coord)
			: time(time),
			speed(speed),
			coord(coord)
		{
			// no code
		}
	};

public:
	std::list<Step> steps;

};
