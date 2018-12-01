#pragma once

#include <iomanip>
#include <chrono>
#include <vector>

class NMEADecoder
{
    std::vector <std::string> _data;
    double _geoLat;
    double _geoLon;
    double _speed;
    double _compass;
    bool _posValid;
    bool _timestampValid;
    bool _speedValid;
    bool _compassValid;
    time_t _lastTime;
    bool _satelliteOnline;
    
    void DecodeGPGGA();
    void DecodeGPGSA();
    void DecodeGPGSV();
    void DecodeGPGLL();
    void DecodeUtcTime(const std::string& cs);
    void DecodeLat(const std::string& richtung, const std::string& value);
    void DecodeLon(const std::string& richtung, const std::string& value);
    void DecodeGPRMC();
    bool CheckCRC();
    
public:
    NMEADecoder();
    ~NMEADecoder();

    bool Decode(const std::string& line);
	bool IsPositionValid() const;
	bool IsSpeedValid() const;
	bool IsCompassValid() const;
	double GetLatitude() const;
	double GetLongitude() const;
	double GetSpeed() const;
	double GetCompass() const;
};

