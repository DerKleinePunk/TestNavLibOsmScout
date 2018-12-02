#include <set>
#include <sstream>
#include <osmscout/routing/Route.h>
#include <cassert>
#include "NavigationDescription.h"

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
