#include <chrono>
#include <thread>

#include <osmscout/Database.h>
#include <osmscout/routing/SimpleRoutingService.h>
#include <osmscout/routing/RoutePostprocessor.h>
#include "utils/easylogging++.h"
#include "TestNavLibOsmScout.h"
#include "NMEADecoder.h"
#include "ConsoleRoutingProgress.h"
#include "PathGenerator.h"
#include "Simulator.h"
#include "PathGeneratorNMEA.h"

struct RouteDescriptionGeneratorCallback : public osmscout::RouteDescriptionGenerator::Callback
{
};

static void GetCarSpeedTable(std::map<std::string, double>& map)
{
	map["highway_motorway"] = 110.0;
	map["highway_motorway_trunk"] = 100.0;
	map["highway_motorway_primary"] = 70.0;
	map["highway_motorway_link"] = 60.0;
	map["highway_motorway_junction"] = 60.0;
	map["highway_trunk"] = 100.0;
	map["highway_trunk_link"] = 60.0;
	map["highway_primary"] = 70.0;
	map["highway_primary_link"] = 60.0;
	map["highway_secondary"] = 60.0;
	map["highway_secondary_link"] = 50.0;
	map["highway_tertiary_link"] = 55.0;
	map["highway_tertiary"] = 55.0;
	map["highway_unclassified"] = 50.0;
	map["highway_road"] = 50.0;
	map["highway_residential"] = 40.0;
	map["highway_roundabout"] = 40.0;
	map["highway_living_street"] = 10.0;
	map["highway_service"] = 30.0;
}

bool GetFirstPosInFile(const std::string& nmeaFilename, double& startLat, double& startLon) {
	NMEADecoder decoder;

	std::ifstream file(nmeaFilename);
	if (!file.is_open()) {
		LOG(ERROR) << "Error Open File";
		std::cout << "Error Open File";
		return false;
	}

	std::string line;
	while (std::getline(file, line)) {
		if (decoder.Decode(line)) {
			//LOG(DEBUG) << "Line Read and decode " << line;
			if (decoder.IsPositionValid()) {
				startLat = decoder.GetLatitude();
				startLon = decoder.GetLongitude();
				return true;
			}
		}
	}
	return false;
}

bool GetLastPosInFile(const std::string& nmeaFilename,const double& startLat, const double& startLon, double& targetLat, double& targetLon) {
	//Todo find faster way

	std::ifstream file(nmeaFilename);
	if (!file.is_open()) {
		LOG(ERROR) << "Error Open File";
		std::cout << "Error Open File";
		return false;
	}

	NMEADecoder decoder;
	std::string line;

	auto result = false;
	auto distanceInKilometerLast = 0.0;
	osmscout::GeoCoord startPos(startLat, startLon);
	while (std::getline(file, line)) {
		if (decoder.Decode(line)) {
			//LOG(DEBUG) << "Line Read and decode " << line;
			if (decoder.IsPositionValid()) {
				const auto curtargetLat = decoder.GetLatitude();
				const auto curtargetLon = decoder.GetLongitude();
				const osmscout::GeoCoord currentPos(curtargetLat, curtargetLon);
				if (startPos.GetLat() != 0) {
					auto distance = osmscout::GetEllipsoidalDistance(currentPos, startPos);
					const auto distanceInKilometer = distance.As<osmscout::Kilometer>();
					if(distanceInKilometer > distanceInKilometerLast) {
						distanceInKilometerLast = distanceInKilometer;
						targetLat = curtargetLat;
						targetLon = curtargetLon;
						result = true;
					}
				}
				
			}
		}
	}
	return result;
}

void DumpGpxFile(const std::string& fileName,
	const std::vector<osmscout::Point>& points,
	const IPathGenerator& generator)
{
	std::ofstream stream;

	std::cout << "Writing gpx file '" << fileName << "'..." << std::endl;

	stream.open(fileName, std::ofstream::trunc);

	if (!stream.is_open()) {
		std::cerr << "Cannot open gpx file!" << std::endl;
		return;
	}

	stream.precision(8);
	stream << R"(<?xml version="1.0" encoding="UTF-8" standalone="no" ?>)" << std::endl;
	stream << R"(<gpx xmlns="http://www.topografix.com/GPX/1/1" creator="bin2gpx" version="1.1" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:schemaLocation="http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd">)"
		<< std::endl;

	stream << "\t<wpt lat=\"" << generator.steps.front().coord.GetLat() << "\" lon=\"" << generator.steps.front().coord.GetLon() << "\">" << std::endl;
	stream << "\t\t<name>Start</name>" << std::endl;
	stream << "\t\t<fix>2d</fix>" << std::endl;
	stream << "\t</wpt>" << std::endl;

	stream << "\t<wpt lat=\"" << generator.steps.back().coord.GetLat() << "\" lon=\"" << generator.steps.back().coord.GetLon() << "\">" << std::endl;
	stream << "\t\t<name>Target</name>" << std::endl;
	stream << "\t\t<fix>2d</fix>" << std::endl;
	stream << "\t</wpt>" << std::endl;

	stream << "\t<rte>" << std::endl;
	stream << "\t\t<name>Route</name>" << std::endl;
	for (const auto& point : points) {
		stream << "\t\t\t<rtept lat=\"" << point.GetLat() << "\" lon=\"" << point.GetLon() << "\">" << std::endl;
		stream << "\t\t\t</rtept>" << std::endl;
	}
	stream << "\t</rte>" << std::endl;

	stream << "\t<trk>" << std::endl;
	stream << "\t\t<name>GPS</name>" << std::endl;
	stream << "\t\t<number>1</number>" << std::endl;
	stream << "\t\t<trkseg>" << std::endl;
	for (const auto& point : generator.steps) {
		stream << "\t\t\t<trkpt lat=\"" << point.coord.GetLat() << "\" lon=\"" << point.coord.GetLon() << "\">" << std::endl;
		stream << "\t\t\t\t<time>" << osmscout::TimestampToISO8601TimeString(point.time) << "</time>" << std::endl;
		stream << "\t\t\t\t<speed>" << point.speed / 3.6 << "</speed>" << std::endl;
		stream << "\t\t\t\t<fix>2d</fix>" << std::endl;
		stream << "\t\t\t</trkpt>" << std::endl;
	}
	stream << "\t\t</trkseg>" << std::endl;
	stream << "\t</trk>" << std::endl;
	stream << "</gpx>" << std::endl;

	stream.close();

	std::cout << "Writing gpx file done." << std::endl;
}

INITIALIZE_EASYLOGGINGPP
int main(int argc, char *argv[])
{
	std::string routerFilenamebase = osmscout::RoutingService::DEFAULT_FILENAME_BASE;
	//Todo put it to commandLine
	std::string gpxFile = "routeRouter.gpx";
	std::string gpxFileTour = "routeTour.gpx";

	START_EASYLOGGINGPP(argc, argv);

	std::cout << "Hello we tray test LibOsmScout Navi Class" << std::endl;
	if(argc < 3) {
		std::cout << "Missing commandline Parameters" << std::endl;
		std::cout << "Please Call TestNavLibOsmScout <map directory> <nmeafile>" << std::endl;
		return -1;
	}

	const std::string mapDirectory = argv[1];
	const std::string nmeaFile = argv[2];

	osmscout::DatabaseParameter databaseParameter;
	auto database = std::make_shared<osmscout::Database>(databaseParameter);

	if (!database->Open(mapDirectory)) {
		std::cerr << "Cannot open database" << std::endl;

		return -2;
	}

	osmscout::FastestPathRoutingProfileRef routingProfile = std::make_shared<osmscout::FastestPathRoutingProfile>(database->GetTypeConfig());
	osmscout::RouterParameter              routerParameter;

	osmscout::SimpleRoutingServiceRef router = std::make_shared<osmscout::SimpleRoutingService>(database,
		routerParameter,
		routerFilenamebase);

	if (!router->Open()) {
		std::cerr << "Cannot open routing database" << std::endl;

		return -3;
	}

	osmscout::TypeConfigRef             typeConfig = database->GetTypeConfig();
	std::map<std::string, double>        carSpeedTable;
	osmscout::RoutingParameter          parameter;

	parameter.SetProgress(std::make_shared<ConsoleRoutingProgress>());
	GetCarSpeedTable(carSpeedTable);
	routingProfile->ParametrizeForCar(*typeConfig,
		carSpeedTable,
		160.0);
	
	double startLat;
	double startLon;

	if (!GetFirstPosInFile(nmeaFile, startLat, startLon)) {
		std::cerr << "Cannot finde a start pos in file" << std::endl;
		return -4;
	}

	auto startCoord = osmscout::GeoCoord(startLat, startLon);
	std::cout << startCoord.GetDisplayText() << std::endl;

	osmscout::RoutePosition start = router->GetClosestRoutableNode(startCoord,
		*routingProfile,
		osmscout::Distance::Of<osmscout::Kilometer>(1));

	if (!start.IsValid()) {
		std::cerr << "Error while searching for routing node near start location!" << std::endl;
		return -5;
	}

	if (start.GetObjectFileRef().GetType() == osmscout::refNode) {
		std::cerr << "Cannot find start node for start location!" << std::endl;
	}

	double targetLat;
	double targetLon;

	if (!GetLastPosInFile(nmeaFile, startLat, startLon, targetLat, targetLon)) {
		std::cerr << "Cannot finde a last pos in file" << std::endl;
		return -6;
	}

	auto targetCoord = osmscout::GeoCoord(targetLat, targetLon);
	std::cout << targetCoord.GetDisplayText() << std::endl;

	osmscout::RoutePosition target = router->GetClosestRoutableNode(targetCoord,
		*routingProfile,
		osmscout::Distance::Of<osmscout::Kilometer>(1));

	if (!target.IsValid()) {
		std::cerr << "Error while searching for routing node near target location!" << std::endl;
		return -7;
	}

	if (target.GetObjectFileRef().GetType() == osmscout::refNode) {
		std::cerr << "Cannot find start node for target location!" << std::endl;
	}

	auto routingResult = router->CalculateRoute(*routingProfile,
		start,
		target,
		parameter);

	if (!routingResult.Success()) {
		std::cerr << "There was an error while calculating the route!" << std::endl;
		router->Close();
		return -8;
	}

	osmscout::RoutePointsResult routePointsResult = router->TransformRouteDataToPoints(routingResult.GetRoute());

	if (!routePointsResult.success) {
		std::cerr << "Error during route conversion" << std::endl;
		return -9;
	}

	auto routeDescriptionResult = router->TransformRouteDataToRouteDescription(routingResult.GetRoute());

	if (!routeDescriptionResult.success) {
		std::cerr << "Error during generation of route description" << std::endl;
		return -10;
	}

	std::list<osmscout::RoutePostprocessor::PostprocessorRef> postprocessors{
		std::make_shared<osmscout::RoutePostprocessor::DistanceAndTimePostprocessor>(),
		std::make_shared<osmscout::RoutePostprocessor::StartPostprocessor>("Start"),
		std::make_shared<osmscout::RoutePostprocessor::TargetPostprocessor>("Target"),
		std::make_shared<osmscout::RoutePostprocessor::WayNamePostprocessor>(),
		std::make_shared<osmscout::RoutePostprocessor::WayTypePostprocessor>(),
		std::make_shared<osmscout::RoutePostprocessor::CrossingWaysPostprocessor>(),
		std::make_shared<osmscout::RoutePostprocessor::DirectionPostprocessor>(),
		std::make_shared<osmscout::RoutePostprocessor::MotorwayJunctionPostprocessor>(),
		std::make_shared<osmscout::RoutePostprocessor::DestinationPostprocessor>(),
		std::make_shared<osmscout::RoutePostprocessor::MaxSpeedPostprocessor>(),
		std::make_shared<osmscout::RoutePostprocessor::InstructionPostprocessor>(),
		std::make_shared<osmscout::RoutePostprocessor::POIsPostprocessor>()
	};

	osmscout::RoutePostprocessor             postprocessor;
	std::set<std::string>                    motorwayTypeNames{ "highway_motorway",
															   "highway_motorway_trunk",
															   "highway_trunk",
															   "highway_motorway_primary" };
	std::set<std::string>                    motorwayLinkTypeNames{ "highway_motorway_link",
																   "highway_trunk_link" };
	std::set<std::string>                    junctionTypeNames{ "highway_motorway_junction" };

	std::vector<osmscout::RoutingProfileRef> profiles{ routingProfile };
	std::vector<osmscout::DatabaseRef>       databases{ database };

	osmscout::StopClock postprocessTimer;

	if (!postprocessor.PostprocessRouteDescription(*routeDescriptionResult.description,
		profiles,
		databases,
		postprocessors,
		motorwayTypeNames,
		motorwayLinkTypeNames,
		junctionTypeNames)) {
		std::cerr << "Error during route postprocessing" << std::endl;
		return -11;
	}

	postprocessTimer.Stop();

	std::cout << "Postprocessing time: " << postprocessTimer.ResultString() << std::endl;

	osmscout::StopClock                 generateTimer;
	osmscout::RouteDescriptionGenerator generator;
	RouteDescriptionGeneratorCallback   generatorCallback;

	generator.GenerateDescription(*routeDescriptionResult.description,
		generatorCallback);

	generateTimer.Stop();

	std::cout << "Description generation time: " << generateTimer.ResultString() << std::endl;

	PathGenerator pathGenerator(*routeDescriptionResult.description, routingProfile->GetVehicleMaxSpeed());

	PathGeneratorNMEA pathGenerator2(nmeaFile, routingProfile->GetVehicleMaxSpeed());
	pathGenerator2.GenerateSteps();

	if (!gpxFile.empty()) {
		DumpGpxFile(gpxFile,
			routePointsResult.points->points,
			pathGenerator);
	}

	if (!gpxFileTour.empty()) {
		DumpGpxFile(gpxFileTour,
			routePointsResult.points->points,
			pathGenerator2);
	}

	Simulator simulator;

	simulator.Simulate(database,
		pathGenerator,
		routePointsResult.points);

	router->Close();

	return 0;
}
