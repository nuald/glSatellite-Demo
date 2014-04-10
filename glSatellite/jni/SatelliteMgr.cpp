#include <string>

#include "SatelliteMgr.h"

using namespace std;

static unsigned char val[256];

static char KepCheck(const string &line1, const string &line2) {
    /* This function scans line 1 and line 2 of a NASA 2-Line element
     set and returns a 1 if the element set appears to be valid or
     a 0 if it does not.  If the data survives this torture test,
     it's a pretty safe bet we're looking at a valid 2-line
     element set and not just some random text that might pass
     as orbital data based on a simple checksum calculation alone. */

    int x;
    unsigned sum1, sum2;

    /* Compute checksum for each line */

    for (x = 0, sum1 = 0, sum2 = 0; x <= 67;
            sum1 += val[(int) line1[x]], sum2 += val[(int) line2[x]], ++x)
        ;

    /* Perform a "torture test" on the data */

    x = (val[(int) line1[68]] ^ (sum1 % 10))
            | (val[(int) line2[68]] ^ (sum2 % 10)) | (line1[0] ^ '1')
            | (line1[1] ^ ' ') | (line1[7] ^ 'U') | (line1[8] ^ ' ')
            | (line1[17] ^ ' ') | (line1[23] ^ '.') | (line1[32] ^ ' ')
            | (line1[34] ^ '.') | (line1[43] ^ ' ') | (line1[52] ^ ' ')
            | (line1[61] ^ ' ') | (line1[62] ^ '0') | (line1[63] ^ ' ')
            | (line2[0] ^ '2') | (line2[1] ^ ' ') | (line2[7] ^ ' ')
            | (line2[11] ^ '.') | (line2[16] ^ ' ') | (line2[20] ^ '.')
            | (line2[25] ^ ' ') | (line2[33] ^ ' ') | (line2[37] ^ '.')
            | (line2[42] ^ ' ') | (line2[46] ^ '.') | (line2[51] ^ ' ')
            | (line2[54] ^ '.') | (line1[2] ^ line2[2]) | (line1[3] ^ line2[3])
            | (line1[4] ^ line2[4]) | (line1[5] ^ line2[5])
            | (line1[6] ^ line2[6]) | (isdigit(line1[68]) ? 0 : 1)
            | (isdigit(line2[68]) ? 0 : 1) | (isdigit(line1[18]) ? 0 : 1)
            | (isdigit(line1[19]) ? 0 : 1) | (isdigit(line2[31]) ? 0 : 1)
            | (isdigit(line2[32]) ? 0 : 1);

    return (x ? 0 : 1);
}

void SatelliteMgr::Init(IFileReader& fd) {
    sat_.clear();
    // Use temporaty vector to set all values at once below
    vector<Satellite> sat_list;
    if (fd.is_open()) {
        while (!fd.eof()) {
            /* Read element set */
            string name, line1, line2;

            name = fd.getline();
            line1 = fd.getline();
            line2 = fd.getline();

            if (KepCheck(line1, line2) && !fd.eof()) {
                /* We found a valid TLE! */
                Satellite sat(name, line1, line2);
                sat_list.push_back(sat);
            }
        }
    }
    sat_ = sat_list;
}

void SatelliteMgr::UpdateAll() {
    size_t len = sat_.size();
    min_alt_ = max_alt_ = 0;
    for (size_t i = 0; i < len; ++i) {
        Satellite &sat = sat_[i];
        // need to update before getting values
        sat.UpdatePosition();
        double alt = sat.GetAltitude();
        min_alt_ = min(min_alt_, alt);
        max_alt_ = max(max_alt_, alt);
    }
}
