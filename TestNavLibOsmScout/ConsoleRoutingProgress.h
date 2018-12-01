#pragma once
#include <osmscout/Database.h>

#include <osmscout/routing/SimpleRoutingService.h>
#include <osmscout/routing/RoutePostprocessor.h>
#include <iostream>

class ConsoleRoutingProgress : public osmscout::RoutingProgress
{
private:
	std::chrono::system_clock::time_point lastDump;
	double                                maxPercent;

public:
	ConsoleRoutingProgress()
		: lastDump(std::chrono::system_clock::now()),
		maxPercent(0.0)
	{
		// no code
	}

	void Reset() override
	{
		lastDump = std::chrono::system_clock::now();
		maxPercent = 0.0;
	}

	void Progress(const osmscout::Distance &currentMaxDistance,
		const osmscout::Distance &overallDistance) override
	{
		double currentPercent = (currentMaxDistance.AsMeter()*100.0) / overallDistance.AsMeter();

		std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

		maxPercent = std::max(maxPercent, currentPercent);

		if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastDump).count() > 500) {
			std::cout << (size_t)maxPercent << "%" << std::endl;

			lastDump = now;
		}
	}
};
