﻿#include <osmscout/Database.h>
#include <osmscout/routing/SimpleRoutingService.h>
#include <osmscout/routing/RoutePostprocessor.h>
#include "utils/easylogging++.h"
#include "TestNavLibOsmScout.h"
#include "NMEADecoder.h"
#include <chrono>
#include <thread>

namespace osmscout {

	static std::string MoveToTurnCommand(osmscout::RouteDescription::DirectionDescription::Move move)
	{
		switch (move) {
		case osmscout::RouteDescription::DirectionDescription::sharpLeft:
			return "Turn sharp left";
		case osmscout::RouteDescription::DirectionDescription::left:
			return "Turn left";
		case osmscout::RouteDescription::DirectionDescription::slightlyLeft:
			return "Turn slightly left";
		case osmscout::RouteDescription::DirectionDescription::straightOn:
			return "Straight on";
		case osmscout::RouteDescription::DirectionDescription::slightlyRight:
			return "Turn slightly right";
		case osmscout::RouteDescription::DirectionDescription::right:
			return "Turn right";
		case osmscout::RouteDescription::DirectionDescription::sharpRight:
			return "Turn sharp right";
		}

		assert(false);

		return "???";
	}


	static std::string CrossingWaysDescriptionToString(const osmscout::RouteDescription::CrossingWaysDescription& crossingWaysDescription)
	{
		std::set<std::string> names;
		osmscout::RouteDescription::NameDescriptionRef originDescription = crossingWaysDescription.GetOriginDesccription();
		osmscout::RouteDescription::NameDescriptionRef targetDescription = crossingWaysDescription.GetTargetDesccription();

		if (originDescription) {
			std::string nameString = originDescription->GetDescription();

			if (!nameString.empty() && nameString.compare("unnamed road")) {
				names.insert(nameString);
			}
		}

		if (targetDescription) {
			std::string nameString = targetDescription->GetDescription();

			if (!nameString.empty() && nameString.compare("unnamed road")) {
				names.insert(nameString);
			}
		}

		for (const auto& name : crossingWaysDescription.GetDescriptions()) {
			std::string nameString = name->GetDescription();

			if (!nameString.empty() && nameString.compare("unnamed road")) {
				names.insert(nameString);
			}
		}

		if (names.size() > 1) {
			std::ostringstream stream;

			for (auto name = names.begin();
				name != names.end();
				++name) {
				if (name != names.begin()) {
					stream << ", ";
				}
				stream << "'" << *name << "'";
			}

			return stream.str();
		}
		else {
			return "";
		}
	}

	bool HasRelevantDescriptions(const osmscout::RouteDescription::Node& node)
	{
		if (node.HasDescription(osmscout::RouteDescription::NODE_START_DESC)) {
			return true;
		}

		if (node.HasDescription(osmscout::RouteDescription::NODE_TARGET_DESC)) {
			return true;
		}

		if (node.HasDescription(RouteDescription::WAY_NAME_CHANGED_DESC)) {
			return true;
		}

		if (node.HasDescription(osmscout::RouteDescription::ROUNDABOUT_ENTER_DESC)) {
			return true;
		}

		if (node.HasDescription(osmscout::RouteDescription::ROUNDABOUT_LEAVE_DESC)) {
			return true;
		}

		if (node.HasDescription(osmscout::RouteDescription::TURN_DESC)) {
			return true;
		}

		if (node.HasDescription(osmscout::RouteDescription::MOTORWAY_ENTER_DESC)) {
			return true;
		}

		if (node.HasDescription(osmscout::RouteDescription::MOTORWAY_CHANGE_DESC)) {
			return true;
		}

		if (node.HasDescription(osmscout::RouteDescription::MOTORWAY_LEAVE_DESC)) {
			return true;
		}

		return false;
	}

	std::string DumpStartDescription(const osmscout::RouteDescription::StartDescriptionRef& startDescription,
		const osmscout::RouteDescription::NameDescriptionRef& nameDescription)
	{
		std::ostringstream stream;
		stream << startDescription->GetDescription();

		if (nameDescription &&
			nameDescription->HasName()) {
			stream << ", drive along '" << nameDescription->GetDescription() << "'";
		}
		return stream.str();
	}

	std::string DumpTargetDescription(const osmscout::RouteDescription::TargetDescriptionRef& /*targetDescription*/)
	{
		std::ostringstream stream;
		stream << "Target reached";
		return stream.str();
	}

	NodeDescription DumpTurnDescription(const osmscout::RouteDescription::TurnDescriptionRef& /*turnDescription*/,
		const osmscout::RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription,
		const osmscout::RouteDescription::DirectionDescriptionRef& directionDescription,
		const osmscout::RouteDescription::NameDescriptionRef& nameDescription)
	{
		NodeDescription description;
		description.roundaboutExitNumber = -1;
		std::ostringstream stream;
		std::string crossingWaysString;

		if (crossingWaysDescription) {
			crossingWaysString = CrossingWaysDescriptionToString(*crossingWaysDescription);
		}

		if (!crossingWaysString.empty()) {
			stream << "At crossing " << crossingWaysString << std::endl;
		}

		if (directionDescription) {
			osmscout::RouteDescription::DirectionDescription::Move move = directionDescription->GetCurve();
			if (!crossingWaysString.empty()) {
				stream << " " << MoveToTurnCommand(move);
			}
			else {
				stream << MoveToTurnCommand(move);
			}
		}
		else {
			if (!crossingWaysString.empty()) {
				stream << " turn";
			}
			else {
				stream << "Turn";
			}
		}

		if (nameDescription &&
			nameDescription->HasName()) {
			stream << " to '" << nameDescription->GetDescription() << "'";
		}
		description.instructions = stream.str();
		return description;
	}

	std::string DumpRoundaboutEnterDescription(const osmscout::RouteDescription::RoundaboutEnterDescriptionRef& /*roundaboutEnterDescription*/,
		const osmscout::RouteDescription::CrossingWaysDescriptionRef& /*crossingWaysDescription*/)
	{
		std::ostringstream stream;
		stream << "Enter in the roundabout, then ";
		return stream.str();
	}

	static std::string digitToOrdinal(size_t digit) {
		switch (digit) {
		case 1:
			return "first";
		case 2:
			return "second";
		case 3:
			return "third";
		case 4:
			return "fourth";
		default: {
			char str[32];
			std::snprintf(str, sizeof(str), "number %zu", digit);
			return str;
		}
		}
	}

	std::string DumpRoundaboutLeaveDescription(const osmscout::RouteDescription::RoundaboutLeaveDescriptionRef& roundaboutLeaveDescription,
		const osmscout::RouteDescription::NameDescriptionRef& nameDescription, size_t /*roundaboutCrossingCounter*/)
	{
		std::ostringstream stream;
		size_t exitCount = roundaboutLeaveDescription->GetExitCount();
		if (exitCount > 0 && exitCount < 4) {
			stream << "take the " << digitToOrdinal(exitCount) << " exit";
		}
		else {
			stream << "take the exit " << exitCount;
		}

		if (nameDescription &&
			nameDescription->HasName()) {
			stream << ", to '" << nameDescription->GetDescription() << "'";
		}

		return stream.str();
	}

	NodeDescription DumpMotorwayEnterDescription(const osmscout::RouteDescription::MotorwayEnterDescriptionRef& motorwayEnterDescription,
		const osmscout::RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription)
	{
		NodeDescription desc;
		std::ostringstream stream;
		std::string crossingWaysString;

		if (crossingWaysDescription) {
			crossingWaysString = CrossingWaysDescriptionToString(*crossingWaysDescription);
		}

		if (!crossingWaysString.empty()) {
			stream << "At the crossing " << crossingWaysString << std::endl;
		}

		if (motorwayEnterDescription->GetToDescription() &&
			motorwayEnterDescription->GetToDescription()->HasName()) {
			if (!crossingWaysString.empty()) {
				stream << " enter the motorway";
			}
			else {
				stream << "Enter the motorway";
			}
			stream << " '" << motorwayEnterDescription->GetToDescription()->GetDescription() << "'";
		}
		desc.instructions = stream.str();
		return desc;
	}

	NodeDescription DumpMotorwayChangeDescription(const osmscout::RouteDescription::MotorwayChangeDescriptionRef& motorwayChangeDescription)
	{
		NodeDescription desc;
		std::ostringstream stream;

		if (motorwayChangeDescription->GetFromDescription() &&
			motorwayChangeDescription->GetFromDescription()->HasName()) {
			stream << "Change motorway";
			stream << " from '" << motorwayChangeDescription->GetFromDescription()->GetDescription() << "'";
		}

		if (motorwayChangeDescription->GetToDescription() &&
			motorwayChangeDescription->GetToDescription()->HasName()) {
			stream << " to '" << motorwayChangeDescription->GetToDescription()->GetDescription() << "'";
		}
		desc.instructions = stream.str();
		return desc;
	}

	NodeDescription DumpMotorwayLeaveDescription(const osmscout::RouteDescription::MotorwayLeaveDescriptionRef& motorwayLeaveDescription,
		const osmscout::RouteDescription::DirectionDescriptionRef& /*directionDescription*/,
		const osmscout::RouteDescription::NameDescriptionRef& nameDescription,
		const RouteDescription::MotorwayJunctionDescriptionRef& motorwayJunction)
	{
		std::ostringstream stream;
		NodeDescription desc;
		desc.roundaboutExitNumber = 0;

		if (motorwayLeaveDescription->GetFromDescription() &&
			motorwayLeaveDescription->GetFromDescription()->HasName()) {
			stream << "Leave the motorway";
			stream << " '" << motorwayLeaveDescription->GetFromDescription()->GetDescription() << "'";
		}

		if (nameDescription &&
			nameDescription->HasName()) {
			stream << " to '" << nameDescription->GetDescription() << "'";
		}
		if (motorwayJunction) {
			if (!motorwayJunction->GetJunctionDescription()->GetName().empty()) {
				stream << " exit '" << motorwayJunction->GetJunctionDescription()->GetName();
				if (!motorwayJunction->GetJunctionDescription()->GetRef().empty()) {
					stream << " (" << motorwayJunction->GetJunctionDescription()->GetRef() << ")";
				}
				stream << "'";
			}
			else {
				stream << " exit " << motorwayJunction->GetJunctionDescription()->GetRef();
			}
		}
		desc.instructions = stream.str();
		return desc;
	}

	NodeDescription DumpNameChangedDescription(const osmscout::RouteDescription::NameChangedDescriptionRef& nameChangedDescription)
	{
		NodeDescription desc;
		std::ostringstream stream;
		if (nameChangedDescription->GetOriginDescription() && nameChangedDescription->GetTargetDescription()) {
			std::string originNameString = nameChangedDescription->GetOriginDescription()->GetDescription();
			std::string targetNameString = nameChangedDescription->GetTargetDescription()->GetDescription();

			if (!originNameString.empty() && originNameString.compare("unnamed road") &&
				!targetNameString.empty() && targetNameString.compare("unnamed road")) {
				stream << "Way changes name";
				stream << " from '" << nameChangedDescription->GetOriginDescription()->GetDescription() << "'";
				stream << " to '" << nameChangedDescription->GetTargetDescription()->GetDescription() << "'";

			}
		}
		desc.instructions = stream.str();
		return desc;
	}

	bool advanceToNextWaypoint(std::list<RouteDescription::Node>::const_iterator &waypoint,
		std::list<RouteDescription::Node>::const_iterator end) {
		if (waypoint == end) return false;
		std::list<RouteDescription::Node>::const_iterator next = waypoint;
		std::advance(next, 1);
		if (next == end) {
			return false;
		}
		else {
			waypoint = next;
			return true;
		}
	}
}

static std::string TimeToString(double time)
{
	std::ostringstream stream;
	stream << std::setfill(' ') << std::setw(2) << (int)std::floor(time) << ":";
	time -= std::floor(time);
	stream << std::setfill('0') << std::setw(2) << (int)floor(60 * time + 0.5);
	return stream.str();
}

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

bool GetLastPosInFile(const std::string& nmeaFilename, double& targetLat, double& targetLon) {
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
	while (std::getline(file, line)) {
		if (decoder.Decode(line)) {
			//LOG(DEBUG) << "Line Read and decode " << line;
			if (decoder.IsPositionValid()) {
				targetLat = decoder.GetLatitude();
				targetLon = decoder.GetLongitude();
				result = true;
			}
		}
	}
	return result;
}

INITIALIZE_EASYLOGGINGPP
int main(int argc, char *argv[])
{
	std::string routerFilenamebase = osmscout::RoutingService::DEFAULT_FILENAME_BASE;

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

	auto routingProfile = std::make_shared<osmscout::FastestPathRoutingProfile>(database->GetTypeConfig());
	osmscout::RouterParameter              routerParameter;

	auto router = std::make_shared<osmscout::SimpleRoutingService>(database, routerParameter, routerFilenamebase);

	if (!router->Open()) {
		std::cerr << "Cannot open routing database" << std::endl;
		return -3;
	}

	double startLat;
	double startLon;
	

	const auto typeConfig = database->GetTypeConfig();
	osmscout::RouteDescription          description;
	std::map<std::string, double>        carSpeedTable;
	osmscout::RoutingParameter          parameter;

	GetCarSpeedTable(carSpeedTable);
	routingProfile->ParametrizeForCar(*typeConfig, carSpeedTable, 160.0);

	if(!GetFirstPosInFile(nmeaFile, startLat, startLon)) {
		std::cerr << "Cannot finde a start pos in file" << std::endl;
		return -4;
	}

	auto startCoord = osmscout::GeoCoord(startLat, startLon);
	std::cout << startCoord.GetDisplayText() << std::endl;

	auto start = router->GetClosestRoutableNode(startCoord, *routingProfile, osmscout::Distance::Of<osmscout::Kilometer>(1));

	if (!start.IsValid()) {
		std::cerr << "Error while searching for routing node near start location!" << std::endl;
		return -5;
	}

	if (start.GetObjectFileRef().GetType() == osmscout::refNode) {
		std::cerr << "Cannot find start node for start location!" << std::endl;
	}

	double targetLat;
	double targetLon;

	if (!GetLastPosInFile(nmeaFile, targetLat, targetLon)) {
		std::cerr << "Cannot finde a last pos in file" << std::endl;
		return -6;
	}

	auto targetCoord = osmscout::GeoCoord(targetLat, targetLon);
	std::cout << targetCoord.GetDisplayText() << std::endl;
	auto target = router->GetClosestRoutableNode(targetCoord, *routingProfile, osmscout::Distance::Of<osmscout::Kilometer>(1));

	if (!target.IsValid()) {
		std::cerr << "Error while searching for routing node near target location!" << std::endl;
		return -7;
	}

	if (target.GetObjectFileRef().GetType() == osmscout::refNode) {
		std::cerr << "Cannot find start node for target location!" << std::endl;
	}

	auto result = router->CalculateRoute(*routingProfile, start, target, parameter);

	if (!result.Success()) {
		std::cerr << "There was an error while calculating the route!" << std::endl;
		router->Close();
		return -8;
	}

	const auto routeDescriptionResult = router->TransformRouteDataToRouteDescription(result.GetRoute());

	std::list<osmscout::RoutePostprocessor::PostprocessorRef> postprocessors;

	postprocessors.push_back(std::make_shared<osmscout::RoutePostprocessor::DistanceAndTimePostprocessor>());
	postprocessors.push_back(std::make_shared<osmscout::RoutePostprocessor::StartPostprocessor>("Start"));
	postprocessors.push_back(std::make_shared<osmscout::RoutePostprocessor::TargetPostprocessor>("Target"));
	postprocessors.push_back(std::make_shared<osmscout::RoutePostprocessor::WayNamePostprocessor>());
	postprocessors.push_back(std::make_shared<osmscout::RoutePostprocessor::WayTypePostprocessor>());
	postprocessors.push_back(std::make_shared<osmscout::RoutePostprocessor::CrossingWaysPostprocessor>());
	postprocessors.push_back(std::make_shared<osmscout::RoutePostprocessor::DirectionPostprocessor>());
	postprocessors.push_back(std::make_shared<osmscout::RoutePostprocessor::MotorwayJunctionPostprocessor>());
	postprocessors.push_back(std::make_shared<osmscout::RoutePostprocessor::DestinationPostprocessor>());

	auto*instructionProcessor = new osmscout::RoutePostprocessor::InstructionPostprocessor();
	const std::set<std::string> motorwayTypeNames = { "highway_motorway", "highway_motorway_trunk", "highway_motorway_primary" };
	const std::set<std::string> motorwayLinkTypeNames = { "highway_motorway_link", "highway_trunk_link" };
	const std::set<std::string> junctionTypeNames = { "highway_motorway_junction" };
	postprocessors.push_back(
		std::static_pointer_cast<osmscout::RoutePostprocessor::Postprocessor>(
			std::make_shared<osmscout::RoutePostprocessor::MaxSpeedPostprocessor>()));
	
	postprocessors.push_back(osmscout::RoutePostprocessor::PostprocessorRef(instructionProcessor));

	const std::vector<osmscout::RoutingProfileRef> profiles = { routingProfile };
	const std::vector<osmscout::DatabaseRef> databases = { database };
	osmscout::RoutePostprocessor postprocessor;
	if (!postprocessor.PostprocessRouteDescription(*routeDescriptionResult.description,
		profiles,
		databases,
		postprocessors,
		motorwayTypeNames,
		motorwayLinkTypeNames,
		junctionTypeNames)) {
		std::cerr << "Error during route postprocessing" << std::endl;
		return -9;
	}

	osmscout::Navigation<osmscout::NodeDescription> navigation(new osmscout::NavigationDescription<osmscout::NodeDescription>);

	//
    // Navigation
    //

    // Snap to route distance set to 100m
	navigation.SetSnapDistance(osmscout::Distance::Of<osmscout::Meter>(100.0));
	navigation.SetRoute(&description);

	//Todo can be set start pos from commandline
	auto latitude = startLat;
	auto longitude = startLon;

	osmscout::GeoCoord location(latitude, longitude);
	double minDistance = 0.0;

	navigation.UpdateCurrentLocation(location, minDistance);
	osmscout::ClosestRoutableObjectResult routableResult = router->GetClosestRoutableObject(location,
		routingProfile->GetVehicle(),
		osmscout::Distance::Of<osmscout::Meter>(100));

	std::cout << "Distance to route: " << minDistance << " °" << std::endl;

	std::cout << "Distance from start: " << navigation.GetDistanceFromStart().AsMeter() << std::endl;
	std::cout << "Time from start: " << TimeToString(navigation.GetDurationFromStart()) << std::endl;

	std::cout << "Distance to destination: " << navigation.GetDistance().AsMeter() << std::endl;
	std::cout << "Time to destination: " << TimeToString(navigation.GetDuration()) << std::endl;

	osmscout::NodeDescription nextWaypointDescription = navigation.nextWaypointDescription();
	std::cout << "Next routing instructions: " << nextWaypointDescription.instructions << std::endl;

	std::cout << "Closest routable object: " << routableResult.GetObject().GetName() << " '" << routableResult.GetName() << "' " <<
		routableResult.GetDistance().AsMeter() << "m" << std::endl;
	return 0;
}
