#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <functional>

#include <sys/time.h>

#include "Satellite.h"
#include "SatelliteCalc.h"

using namespace std;

static string SubString(const string &value, size_t start, size_t end) {
    /* This function returns a substring based on the starting
     and ending positions provided.  It is used heavily in
     the AutoUpdate function when parsing 2-line element data. */

    string str = value.substr(start, end - start + 1);
    str.erase(remove_if(str.begin(), str.end(), ptr_fun<int, int>(isspace)),
            str.end());
    return str;
}

// trim from end
static inline string RTrim(const string &s) {
    string result(s);
    result.erase(
            find_if(result.rbegin(), result.rend(),
                    not1(ptr_fun<int, int>(isspace))).base(), result.end());
    return result;
}

Satellite::Satellite(const string& name, const string& line1,
        const string& line2) :
        name_(RTrim(name)), line1_(line1), line2_(line2) {
    /* Updates data in TLE structure based on
     line1 and line2 stored in structure. */

    designator_ = SubString(line1, 9, 16);
    catnum_ = atol(SubString(line1, 2, 6).c_str());
    year_ = atoi(SubString(line1, 18, 19).c_str());
    refepoch_ = atof(SubString(line1, 20, 31).c_str());
    double tempnum = 1.0e-5 * atof(SubString(line1, 44, 49).c_str());
    nddot6_ = tempnum / pow(10.0, (line1[51] - '0'));
    tempnum = 1.0e-5 * atof(SubString(line1, 53, 58).c_str());
    bstar_ = tempnum / pow(10.0, (line1[60] - '0'));
    setnum_ = atol(SubString(line1, 64, 67).c_str());
    incl_ = atof(SubString(line2, 8, 15).c_str());
    raan_ = atof(SubString(line2, 17, 24).c_str());
    eccn_ = 1.0e-07 * atof(SubString(line2, 26, 32).c_str());
    argper_ = atof(SubString(line2, 34, 41).c_str());
    meanan_ = atof(SubString(line2, 43, 50).c_str());
    meanmo_ = atof(SubString(line2, 52, 62).c_str());
    drag_ = atof(SubString(line1, 33, 42).c_str());
    orbitnum_ = atof(SubString(line2, 63, 67).c_str());
}

double CurrentDaynum() {
    /* Read the system clock and return the number
     of days since 31Dec79 00:00:00 UTC (daynum 0) */

    int x;
    struct timeval tptr;
    double usecs, seconds;

    x = gettimeofday(&tptr, NULL);

    usecs = 0.000001 * (double) tptr.tv_usec;
    seconds = usecs + (double) tptr.tv_sec;

    return ((seconds / 86400.0) - 3651.0);
}

void Satellite::UpdatePosition() {
    SatelliteCalc calc(*this);
    calc.Calc(CurrentDaynum());
    calc.Update(*this);
}

double Satellite::GetLatitude() {
    return sat_lat;
}

double Satellite::GetLongitude() {
    return sat_lon - 360.f;
}

double Satellite::GetAltitude() {
    return sat_alt;
}
