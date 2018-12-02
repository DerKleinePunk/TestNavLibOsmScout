#ifndef ELPP_DEFAULT_LOGGER
#   define ELPP_DEFAULT_LOGGER "NMEADecoder"
#endif
#ifndef ELPP_CURR_FILE_PERFORMANCE_LOGGER_ID
#   define ELPP_CURR_FILE_PERFORMANCE_LOGGER_ID ELPP_DEFAULT_LOGGER
#endif

#include "utils/easylogging++.h"
#include "NMEADecoder.h"
#include "Tokenizer.h"

//http://www.kowoma.de/gps/zusatzerklaerungen/NMEA.htm
//http://aprs.gids.nl/nmea/
//D:\Mine\CarPC - Selbstbau\Fremder Code\roadmap-1.2.1\src\roadmap_nmea.c
//http://nmea.sourceforge.net/ -> D:\Mine\OpenSource\nmealib
//D:\Mine\CarPC - Selbstbau\Fremder Code\roadmap-1.2.1\src\roadmap_nmea.c
//Linux http://www.rjsystems.nl/en/2100-ntpd-garmin-gps-18-lvc-gpsd.php !gpsd!
//http://www.it-adviser.net/raspberry-pi-gps-empfaenger-einrichten-und-mit-java-auswerten/
//http://catb.org/gpsd/
//D:\Mine\OpenSource\qtgpsc
//https://github.com/redhog/agpsd
//http://catb.org/gpsd/installation.html
//http://catb.org/gpsd/client-howto.html
//https://github.com/adafruit/Adafruit_GPS/blob/master/Adafruit_GPS.h


#define KMPH    1.852       // kilometers-per-hour in one knot
#define MPH     1.1507794   // miles-per-hour in one knot

NMEADecoder::NMEADecoder():
	_geoLat(0),
	_geoLon(0),
	_speed(0),
	_compass(-1),
	_posValid(false),
	_timestampValid(false),
	_speedValid(false),
	_compassValid(false),
	_lastTimestamp(0), 
	_lastTime(),
	_satelliteOnline(false) {
	el::Loggers::getLogger(ELPP_DEFAULT_LOGGER);
}

NMEADecoder::~NMEADecoder() {
}

void NMEADecoder::DecodeGPGGA() {
    DecodeUtcTime(_data[1]);
    //Global positioning system fixed data
    //$GPGGA,191410,4735.5634,N,00739.3538,E,1,04,4.4,351.5,M,48.0,M,,*45
    if (std::stoi(_data[6]) > 0) {
        DecodeLat(_data[3], _data[2]);
        DecodeLon(_data[5], _data[4]);
        _posValid = true;
        _satelliteOnline = true;
    } else {
        _satelliteOnline = false;
    }
}

void NMEADecoder::DecodeGPGSA() {
    //GPS DOP and active satellites
}

void NMEADecoder::DecodeGPGSV() {
    //Satellites in view
}

void NMEADecoder::DecodeGPGLL() {
    //Geographic Position - Latitude/Longitude
    //$GPGLL,5024.6102,N,00921.8833,E,183242.000,A,A*5B
    if (_data[6] == "A") {
        DecodeLat(_data[2], _data[1]);
        DecodeLon(_data[4], _data[3]);
        DecodeUtcTime(_data[5]);
        _posValid = true;
        _satelliteOnline = true;
    } else {
        _satelliteOnline = false;
    }
}

void NMEADecoder::DecodeUtcTime(const std::string& time) {
	//without seperator vs2015 can't decode the Time MS Bug ?
	//Todo Corrent this we need two thinks an TimeStamp in local time and the di form last position update on linux and windows with micro sekonds
	static const std::string dateTimeFormat{ "%H:%M:%S" };
	auto timeLocal = time;
	const auto j = timeLocal.find_first_of('.');
	if (std::string::npos != j)
	{
		timeLocal = timeLocal.substr(0,j);
	}
	timeLocal = timeLocal.substr(0, 2) + ":" + timeLocal.substr(2);
	timeLocal = timeLocal.substr(0, 5) + ":" + timeLocal.substr(5);
	std::istringstream input(timeLocal);
	input.imbue(std::locale(setlocale(LC_ALL, nullptr)));
	input >> std::get_time(&_lastTime, dateTimeFormat.c_str());
	if(input.fail()) {
		LOG(WARNING) << "Date convert Failed";
		return;
	}
}

void NMEADecoder::DecodeLat(const std::string& richtung, const std::string& value) {
	_geoLat = std::stoi(value.substr(0, 2));
	double nMinuten = std::stof(value.substr(2));
	nMinuten = nMinuten / 60;
	_geoLat += nMinuten;
	if(richtung == "S") {
		//Andere Seite WeltKugel
		_geoLat = -_geoLat;
	}
}

void NMEADecoder::DecodeLon(const std::string& richtung, const std::string& value) {
	_geoLon = std::stoi(value.substr(0, 3));
	double nMinuten = std::stof(value.substr(3));
	nMinuten = nMinuten / 60;
	_geoLon += nMinuten;
	if (richtung == "W") {
		//Andere Seite WeltKugel
		_geoLon = -_geoLon;
	}
}

void NMEADecoder::DecodeDate(const std::string& dateString) {
	static const std::string dateTimeFormat{ "%d.%m.%Y" };
	auto dateLocal = dateString;
	dateLocal = dateLocal.substr(0, 2) + "." + dateLocal.substr(2);
	dateLocal = dateLocal.substr(0, 5) + ".20" + dateLocal.substr(5);

	std::tm dt{};
	std::istringstream input(dateLocal);
	input.imbue(std::locale(setlocale(LC_ALL, nullptr)));
	input >> std::get_time(&dt, dateTimeFormat.c_str());
	if (input.fail()) {
		LOG(WARNING) << "Date convert Failed";
		return;
	}

	dt.tm_hour = _lastTime.tm_hour;
	dt.tm_sec = _lastTime.tm_sec;
	dt.tm_min = _lastTime.tm_min;

	_lastTimestamp = std::mktime(&dt);
	_timestampValid = true;
}

void NMEADecoder::DecodeGPRMC() {
	//Recommended minimum specific GNSS data
	DecodeUtcTime(_data[1]);
	if(_data[2] == "A") {
		//V Ungültig
		DecodeLat(_data[4], _data[3]);
		DecodeLon(_data[6], _data[5]);
		_posValid = true;
		if(_data.size() >= 8) {
			if (_data[7].length() > 0) {
				_speedValid = true;
				_speed = std::stof(_data[7]) * KMPH; //convert Knoten to Km/h
			}
			if (_data[8].length() > 0) {
				_compassValid = true;
				_compass = std::stof(_data[8]); //Bewegungsrichtung in Grad
			}
            if(_data.size() >= 9){
                if(!_data[9].empty()) {
                    //Not all GPS send this
					DecodeDate(_data[9]);
                }
            }
		}
        _satelliteOnline = true;
	} else {
        _satelliteOnline = false;
    }
}

bool NMEADecoder::CheckCRC() {
	auto value = _data[_data.size() - 1];
	return true;
}

bool NMEADecoder::Decode(const std::string& line) {
	_timestampValid = false;
	_data.clear();
	Tokenizer tokenizer(line, ",*");
	
	while(tokenizer.NextToken()) {
		_data.push_back(tokenizer.GetToken());
	}

	if (_data.empty()) return false;
	if (!CheckCRC()) return false;

	//Todo Many more

	if(_data[0] == "$GPGGA") {
		DecodeGPGGA();
	} else if (_data[0] == "$GPGSA") {
		DecodeGPGSA();
	} else if (_data[0] == "$GPGSV") {
		DecodeGPGSV();
	} else if (_data[0] == "$GPRMC") {
		DecodeGPRMC();
    } else if (_data[0] == "$GPGLL") {
		DecodeGPGLL();
	} else {
		return false;
	}
    
    if(!_satelliteOnline) {
        _posValid = false;
        _timestampValid = false;
        _speedValid = false;
    }
    
	return true;
}

bool NMEADecoder::IsPositionValid() const {
	return _posValid;
}

bool NMEADecoder::IsSpeedValid() const {
	return _speedValid;
}

bool NMEADecoder::IsCompassValid() const {
	return _compassValid;
}

bool NMEADecoder::IsTimestampValid() const {
	return _timestampValid;
}

double NMEADecoder::GetLatitude() const {
	return _geoLat;
}

double NMEADecoder::GetLongitude() const {
	return _geoLon;
}

double NMEADecoder::GetSpeed() const {
	return _speed;
}

double NMEADecoder::GetCompass() const {
	return _compass;
}

std::time_t NMEADecoder::GetTimestamp() const {
	return _lastTimestamp;
}
