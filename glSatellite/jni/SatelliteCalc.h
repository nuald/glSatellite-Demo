#pragma once

#include <cmath>
#include "Satellite.h"

/* General three-dimensional vector structure used by SGP4/SDP4 code. */
struct  vector_t {
    double x, y, z, w;

    void Magnitude() {
        /* Calculates scalar magnitude */
        w = sqrt(x * x + y * y + z * z);
    }

    void Scale(double k) {
        /* Multiplies the vector by the scalar k */
        x *= k;
        y *= k;
        z *= k;
        Magnitude();
    }
};

/* Common arguments between deep-space functions used by SGP4/SDP4 code. */
typedef struct {
    /* Used by dpinit part of Deep() */
    double eosq, sinio, cosio, betao, aodp, theta2, sing, cosg, betao2, xmdot,
            omgdot, xnodot, xnodp;

    /* Used by dpsec and dpper parts of Deep() */
    double xll, omgadf, xnode, em, xinc, xn, t;

    /* Used by thetg and Deep() */
    double ds50;
} deep_arg_t;

class SatelliteCalc {
    // Two-line-element satellite orbital data used for calculations
    double tle_epoch, tle_xndt2o, tle_xndd6o, tle_bstar, tle_xincl, tle_xnodeo;
    double tle_xmo, tle_xno, tle_eo, tle_omegao;
    int tle_catnr, tle_revnum;
    std::string tle_sat_name, tle_idesg;

    // Flags
    bool is_sdp4_, epoch_restart, synchronous, resonance,
        do_loop, lunar_terms_done, simple, sdp4_initialized,
        sgp4_initialized;

    // SGP4 temp values
    double aodp, aycof, c1, c4, c5, cosio, d2, d3, d4, delmo, omgcof,
        eta, omgdot, sinio, xnodp, sinmo, t2cof, t3cof, t4cof, t5cof,
        x1mth2, x3thm1, x7thm1, xmcof, xmdot, xnodcf, xnodot, xlcof;

    // SDP4 temp values
    double thgr, xnq, xqncl, omegaq, zmol, zmos, savtsn, ee2, e3, xi2,
        xl2, xl3, xl4, xgh2, xgh3, xgh4, xh2, xh3, sse, ssi, ssg, xi3, se2,
        si2, sl2, sgh2, sh2, se3, si3, sl3, sgh3, sh3, sl4, sgh4, ssl, ssh,
        d3210, d3222, d4410, d4422, d5220, d5232, d5421, d5433, del1, del2,
        del3, fasx2, fasx4, fasx6, xlamo, xfact, xni, atime, stepp, stepn,
        step2, preep, pl, sghs, xli, d2201, d2211, sghl, sh1, pinc, pe, shs,
        zsingl, zcosgl, zsinhl, zcoshl, zsinil, zcosil;
    deep_arg_t deep_arg;

    // Calculated values
    double sat_lat, sat_lon, sat_alt, sat_vel, age;

    void SelectEphemeris();
    void SGP4(double tsince, vector_t &pos, vector_t &vel);
    void SDP4(double tsince, vector_t &pos, vector_t &vel);
    void Deep(int ientry);
    double ThetaG();
public:
    SatelliteCalc(const Satellite& satellite);
    void Calc(double daynum);
    void Update(Satellite& satellite);
};
