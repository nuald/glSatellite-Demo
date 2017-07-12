#pragma once

#include "Satellite.h"

class SatelliteMgr {
    std::vector<Satellite> sat_;
    double min_alt_;
    double max_alt_;
public:
    SatelliteMgr() {
    }

    void Init(IFileReader& reader);

    size_t GetNumber() const {
        return sat_.size();
    }

    Satellite& GetSatellite(size_t index) {
        return sat_[index];
    }

    void UpdateAll();

    double GetMinAltitude() {
        return min_alt_;
    }

    double GetMaxAltitude() {
        return max_alt_;
    }
};
