#pragma once
#ifndef NAVIGATIONDESCRIPTION_H
#define NAVIGATIONDESCRIPTION_H

#include <osmscout/navigation/Navigation.h>
#include <osmscout/GeoCoord.h>

namespace osmscout {

	struct NodeDescription
	{
		int         roundaboutExitNumber{};
		int         index{};
		std::string instructions;
		Distance    distance;
		double      time{};
		GeoCoord    location;
	};

	bool HasRelevantDescriptions(const RouteDescription::Node& node);

	std::string DumpStartDescription(const RouteDescription::StartDescriptionRef& startDescription,
		const RouteDescription::NameDescriptionRef& nameDescription);

	std::string DumpTargetDescription(const RouteDescription::TargetDescriptionRef& targetDescription);

	NodeDescription DumpTurnDescription(const RouteDescription::TurnDescriptionRef& turnDescription,
		const RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription,
		const RouteDescription::DirectionDescriptionRef& directionDescription,
		const RouteDescription::NameDescriptionRef& nameDescription);

	std::string
		DumpRoundaboutEnterDescription(const RouteDescription::RoundaboutEnterDescriptionRef& roundaboutEnterDescription,
			const RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription);

	std::string
		DumpRoundaboutLeaveDescription(const RouteDescription::RoundaboutLeaveDescriptionRef& roundaboutLeaveDescription,
			const RouteDescription::NameDescriptionRef& nameDescription,
			size_t roundaboutCrossingCounter);

	NodeDescription
		DumpMotorwayEnterDescription(const RouteDescription::MotorwayEnterDescriptionRef& motorwayEnterDescription,
			const RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription);

	NodeDescription
		DumpMotorwayChangeDescription(const RouteDescription::MotorwayChangeDescriptionRef& motorwayChangeDescription);

	NodeDescription
		DumpMotorwayLeaveDescription(const RouteDescription::MotorwayLeaveDescriptionRef& motorwayLeaveDescription,
			const RouteDescription::DirectionDescriptionRef& directionDescription,
			const RouteDescription::NameDescriptionRef& nameDescription,
			const RouteDescription::MotorwayJunctionDescriptionRef& motorwayJunction);

	NodeDescription DumpNameChangedDescription(const RouteDescription::NameChangedDescriptionRef& nameChangedDescription);

	bool advanceToNextWaypoint(std::list<RouteDescription::Node>::const_iterator& waypoint,
		std::list<RouteDescription::Node>::const_iterator end);

	template<class NodeDescription>
	class NavigationDescription : public OutputDescription<NodeDescription>
	{
	public:
		NavigationDescription()
			: _roundaboutCrossingCounter(0),
			_index(0)
		{};

		void NextDescription(const Distance &distance,
			std::list<RouteDescription::Node>::const_iterator& waypoint,
			std::list<RouteDescription::Node>::const_iterator end)
		{

			if (waypoint == end || (distance.AsMeter() >= 0 && _previousDistance > distance)) {
				return;
			}

			do {

				_description.roundaboutExitNumber = -1;
				_description.instructions = "";

				do {

					RouteDescription::DescriptionRef             desc;
					RouteDescription::NameDescriptionRef         nameDescription;
					RouteDescription::DirectionDescriptionRef    directionDescription;
					RouteDescription::NameChangedDescriptionRef  nameChangedDescription;
					RouteDescription::CrossingWaysDescriptionRef crossingWaysDescription;

					RouteDescription::StartDescriptionRef           startDescription;
					RouteDescription::TargetDescriptionRef          targetDescription;
					RouteDescription::TurnDescriptionRef            turnDescription;
					RouteDescription::RoundaboutEnterDescriptionRef roundaboutEnterDescription;
					RouteDescription::RoundaboutLeaveDescriptionRef roundaboutLeaveDescription;
					RouteDescription::MotorwayEnterDescriptionRef   motorwayEnterDescription;
					RouteDescription::MotorwayChangeDescriptionRef  motorwayChangeDescription;
					RouteDescription::MotorwayLeaveDescriptionRef   motorwayLeaveDescription;

					RouteDescription::MotorwayJunctionDescriptionRef motorwayJunctionDescription;

					desc = waypoint->GetDescription(RouteDescription::WAY_NAME_DESC);
					if (desc) {
						nameDescription = std::dynamic_pointer_cast<RouteDescription::NameDescription>(desc);
					}

					desc = waypoint->GetDescription(RouteDescription::DIRECTION_DESC);
					if (desc) {
						directionDescription = std::dynamic_pointer_cast<RouteDescription::DirectionDescription>(desc);
					}

					desc = waypoint->GetDescription(RouteDescription::WAY_NAME_CHANGED_DESC);
					if (desc) {
						nameChangedDescription = std::dynamic_pointer_cast<RouteDescription::NameChangedDescription>(desc);
					}

					desc = waypoint->GetDescription(RouteDescription::CROSSING_WAYS_DESC);
					if (desc) {
						crossingWaysDescription = std::dynamic_pointer_cast<RouteDescription::CrossingWaysDescription>(desc);
					}

					desc = waypoint->GetDescription(RouteDescription::NODE_START_DESC);
					if (desc) {
						startDescription = std::dynamic_pointer_cast<RouteDescription::StartDescription>(desc);
					}

					desc = waypoint->GetDescription(RouteDescription::NODE_TARGET_DESC);
					if (desc) {
						targetDescription = std::dynamic_pointer_cast<RouteDescription::TargetDescription>(desc);
					}

					desc = waypoint->GetDescription(RouteDescription::TURN_DESC);
					if (desc) {
						turnDescription = std::dynamic_pointer_cast<RouteDescription::TurnDescription>(desc);
					}

					desc = waypoint->GetDescription(RouteDescription::ROUNDABOUT_ENTER_DESC);
					if (desc) {
						roundaboutEnterDescription = std::dynamic_pointer_cast<RouteDescription::RoundaboutEnterDescription>(desc);
					}

					desc = waypoint->GetDescription(RouteDescription::ROUNDABOUT_LEAVE_DESC);
					if (desc) {
						roundaboutLeaveDescription = std::dynamic_pointer_cast<RouteDescription::RoundaboutLeaveDescription>(desc);
					}

					desc = waypoint->GetDescription(RouteDescription::MOTORWAY_ENTER_DESC);
					if (desc) {
						motorwayEnterDescription = std::dynamic_pointer_cast<RouteDescription::MotorwayEnterDescription>(desc);
					}

					desc = waypoint->GetDescription(RouteDescription::MOTORWAY_CHANGE_DESC);
					if (desc) {
						motorwayChangeDescription = std::dynamic_pointer_cast<RouteDescription::MotorwayChangeDescription>(desc);
					}

					desc = waypoint->GetDescription(RouteDescription::MOTORWAY_LEAVE_DESC);
					if (desc) {
						motorwayLeaveDescription = std::dynamic_pointer_cast<RouteDescription::MotorwayLeaveDescription>(desc);
					}

					desc = waypoint->GetDescription(RouteDescription::MOTORWAY_JUNCTION_DESC);
					if (desc) {
						motorwayJunctionDescription = std::dynamic_pointer_cast<RouteDescription::MotorwayJunctionDescription>(desc);
					}

					if (crossingWaysDescription &&
						_roundaboutCrossingCounter > 0 &&
						crossingWaysDescription->GetExitCount() > 1) {
						_roundaboutCrossingCounter += crossingWaysDescription->GetExitCount() - 1;
					}

					if (!HasRelevantDescriptions(*waypoint)) {
						continue;
					}

					if (startDescription) {
						_description.instructions = DumpStartDescription(startDescription,
							nameDescription);
					}
					else if (targetDescription) {
						_description.instructions = DumpTargetDescription(targetDescription);
					}
					else if (turnDescription) {
						_description = DumpTurnDescription(turnDescription,
							crossingWaysDescription,
							directionDescription,
							nameDescription);
					}
					else if (roundaboutEnterDescription) {
						_description.instructions = DumpRoundaboutEnterDescription(roundaboutEnterDescription,
							crossingWaysDescription);
						_description.roundaboutExitNumber = 1;
						_roundaboutCrossingCounter = 1;
					}
					else if (roundaboutLeaveDescription) {
						_description.instructions += DumpRoundaboutLeaveDescription(roundaboutLeaveDescription,
							nameDescription,
							_roundaboutCrossingCounter);
						_description.roundaboutExitNumber = (int)roundaboutLeaveDescription->GetExitCount();
						_roundaboutCrossingCounter = 0;
					}
					else if (motorwayEnterDescription) {
						_description = DumpMotorwayEnterDescription(motorwayEnterDescription,
							crossingWaysDescription);
					}
					else if (motorwayChangeDescription) {
						_description = DumpMotorwayChangeDescription(motorwayChangeDescription);
					}
					else if (motorwayLeaveDescription) {
						_description = DumpMotorwayLeaveDescription(motorwayLeaveDescription,
							directionDescription,
							nameDescription,
							motorwayJunctionDescription);
					}
					else if (nameChangedDescription) {
						_description = DumpNameChangedDescription(nameChangedDescription);
					}
					else {
						_description.instructions = "";
					}
				} while ((_description.instructions.empty() || _roundaboutCrossingCounter > 0) && advanceToNextWaypoint(waypoint,
					end));

				_description.index = _index++;
			} while (distance > waypoint->GetDistance() && advanceToNextWaypoint(waypoint,
				end));
			_description.distance = waypoint->GetDistance();
			_description.time = waypoint->GetTime();
			_description.location = waypoint->GetLocation();
			_previousDistance = _description.distance;
			++waypoint;
		}

		NodeDescription GetDescription()
		{
			return _description;
		}

		void Clear()
		{
			_previousDistance = Distance::Of<Meter>(0.0);
			_roundaboutCrossingCounter = 0;
			_index = 0;
		}

	private:
		size_t          _roundaboutCrossingCounter;
		size_t          _index;
		Distance        _previousDistance;
		NodeDescription _description;
	};
}

#endif //NAVIGATIONDESCRIPTION_H