#pragma once

#include <string>
#include <vector>

#include "IFileReader.h"

class Satellite {
    friend class SatelliteCalc;

    // Original unmodified data
    std::string name_;
    std::string line1_;
    std::string line2_;
    long catnum_;
    long setnum_;
    std::string designator_;
    int year_;
    double refepoch_;
    double incl_;
    double raan_;
    double eccn_;
    double argper_;
    double meanan_;
    double meanmo_;
    double drag_;
    double nddot6_;
    double bstar_;
    long orbitnum_;

    // Calculated values
    double sat_lat, sat_lon, sat_alt;
public:
    Satellite(const std::string& name, const std::string& line1,
            const std::string& line2);

    void UpdatePosition();
    double GetLatitude();
    double GetLongitude();
    double GetAltitude();
};
