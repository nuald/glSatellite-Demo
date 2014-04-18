#include "SatelliteCalc.h"

// ----------------------------------------------------------------------------
// Constants used by SGP4/SDP4
// ----------------------------------------------------------------------------

const double DEG2RAD = 1.745329251994330E-2; // Degrees to radians
const double PIO2 = M_PI / 2; // Pi/2
const double X3PIO2 = 3 * M_PI / 2; // 3*Pi/2
const double TWOPI = 2 * M_PI; // 2*Pi
const double E6A = 1.0E-6;
const double TOTHRD = 2.0 / 3; // 2/3
const double XJ3 = -2.53881E-6; // J3 Harmonic (WGS '72)
const double XKE = 7.43669161E-2;
const double XKMPER = 6.378137E3; // WGS 84 Earth radius km
const double SECDAY = 8.6400E4; // Seconds per day
const double MINDAY = 1.44E3; // Minutes per day
const double AE = 1.0;
const double CK2 = 5.413079E-4;
const double CK4 = 6.209887E-7;
const double FF = 3.35281066474748E-3; // Flattening factor
const double S4_INIT = 1.012229;
const double QOMS2T = 1.880279E-09;
const double OMEGA_E = 1.00273790934; // Earth rotations/siderial day
const double ZNS = 1.19459E-5;
const double C1SS = 2.9864797E-6;
const double ZES = 1.675E-2;
const double ZNL = 1.5835218E-4;
const double C1L = 4.7968065E-7;
const double ZEL = 5.490E-2;
const double ZCOSIS = 9.1744867E-1;
const double ZSINIS = 3.9785416E-1;
const double ZSINGS = -9.8088458E-1;
const double ZCOSGS = 1.945905E-1;
const double Q22 = 1.7891679E-6;
const double Q31 = 2.1460748E-6;
const double Q33 = 2.2123015E-7;
const double G22 = 5.7686396;
const double G32 = 9.5240898E-1;
const double G44 = 1.8014998;
const double G52 = 1.0508330;
const double G54 = 4.4108898;
const double ROOT22 = 1.7891679E-6;
const double ROOT32 = 3.7393792E-7;
const double ROOT44 = 7.3636953E-9;
const double ROOT52 = 1.1428639E-7;
const double ROOT54 = 2.1765803E-9;
const double THDT = 4.3752691E-3;

// Stages of deep-space calculations
enum {
    DPINIT, // Deep-space initialization code
    DPSEC, // Deep-space secular code
    DPPER // Deep-space periodic code
};

// Geodetic position structure
typedef struct {
    double lat, lon, alt, theta;
} geodetic_t;

// ----------------------------------------------------------------------------
// Extended math functions
// ----------------------------------------------------------------------------

static double Sqr(double arg) {
    // Returns square of a double
    return arg * arg;
}

static double AcTan(double sinx, double cosx) {
    // Four-quadrant arctan function

    if (cosx == 0.0) {
        return sinx > 0.0 ? PIO2 : X3PIO2;
    } else {
        if (cosx > 0.0) {
            return sinx > 0.0 ? atan(sinx / cosx) : TWOPI + atan(sinx / cosx);
        } else {
            return M_PI + atan(sinx / cosx);
        }
    }
}

static double FMod2p(double x) {
    // Returns mod 2PI of argument
    double ret_val = x;
    int i = ret_val / TWOPI;
    ret_val -= i * TWOPI;

    if (ret_val < 0.0) {
        ret_val += TWOPI;
    }

    return ret_val;
}

static double Modulus(double arg1, double arg2) {
    // Returns arg1 mod arg2
    double ret_val = arg1;
    int i = ret_val / arg2;
    ret_val -= i * arg2;

    if (ret_val < 0.0) {
        ret_val += arg2;
    }

    return ret_val;
}

static double Frac(double arg) {
    // Returns fractional part of double argument
    return arg - floor(arg);
}

// ----------------------------------------------------------------------------
// Auxiliary functions
// ----------------------------------------------------------------------------

static double JulianDateofYear(double year) {
    // The function JulianDateofYear calculates the Julian Date
    // of Day 0.0 of {year}. This function is used to calculate the
    // Julian Date of any date by using JulianDateofYear.

    // Astronomical Formulae for Calculators, Jean Meeus,
    // pages 23-25. Calculate Julian Date of 0.0 Jan year
    year = year - 1;
    long i = year / 100;
    long a = i;
    i = a / 4;
    long b = 2 - a + i;
    i = 365.25 * year;
    i += 30.6001 * 14;
    double jdoy = i + 1720994.5 + b;

    return jdoy;
}

static double JulianDateofEpoch(double epoch) {
    // The function JulianDateofEpoch returns the Julian Date of
    // an epoch specified in the format used in the NORAD two-line
    // element sets. It has been modified to support dates beyond
    // the year 1999 assuming that two-digit years in the range 00-56
    // correspond to 2000-2056. Until the two-line element set format
    // is changed, it is only valid for dates through 2056 December 31.

    double year;

    // Modification to support Y2K
    // Valid 1957 through 2056

    double day = modf(epoch * 1E-3, &year) * 1E3;
    year += year < 57 ? 2000 : 1900;
    return JulianDateofYear(year) + day;
}

static double ThetaG_JD(double jd) {
    // Reference:  The 1992 Astronomical Almanac, page B6.
    double ut = Frac(jd + 0.5);
    jd = jd - ut;
    double tu = (jd - 2451545.0) / 36525;
    double gmst = 24110.54841
            + tu * (8640184.812866 + tu * (0.093104 - tu * 6.2E-6));
    gmst = Modulus(gmst + SECDAY * OMEGA_E * ut, SECDAY);

    return TWOPI * gmst / SECDAY;
}

static void CalculateLatLonAlt(double time, vector_t &pos,
    geodetic_t &geodetic) {
    // Procedure CalculateLatLonAlt will calculate the geodetic
    // position of an object given its ECI position pos and time.
    // It is intended to be used to determine the ground track of
    // a satellite.  The calculations  assume the earth to be an
    // oblate spheroid as defined in WGS '72.
    // Reference:  The 1992 Astronomical Almanac, page K12.

    double r, e2, phi, c;

    geodetic.theta = AcTan(pos.y, pos.x); /* radians */
    geodetic.lon = FMod2p(geodetic.theta - ThetaG_JD(time)); /* radians */
    r = sqrt(Sqr(pos.x) + Sqr(pos.y));
    e2 = FF * (2 - FF);
    geodetic.lat = AcTan(pos.z, r); /* radians */

    do {
        phi = geodetic.lat;
        c = 1 / sqrt(1 - e2 * Sqr(sin(phi)));
        geodetic.lat = AcTan(pos.z + XKMPER * c * e2 * sin(phi), r);
    } while (fabs(geodetic.lat - phi) >= 1E-10);

    geodetic.alt = r / cos(geodetic.lat) - XKMPER * c; /* kilometers */

    if (geodetic.lat > PIO2) {
        geodetic.lat -= TWOPI;
    }
}

// ----------------------------------------------------------------------------
// Class methods
// ----------------------------------------------------------------------------

SatelliteCalc::SatelliteCalc(const Satellite& satellite) {
    tle_sat_name = satellite.name_;
    tle_idesg = satellite.designator_;
    tle_catnr = satellite.catnum_;
    tle_epoch = (1000.0 * (double)satellite.year_) + satellite.refepoch_;
    tle_xndt2o = satellite.drag_;
    tle_xndd6o = satellite.nddot6_;
    tle_bstar = satellite.bstar_;
    tle_xincl = satellite.incl_;
    tle_xnodeo = satellite.raan_;
    tle_eo = satellite.eccn_;
    tle_omegao = satellite.argper_;
    tle_xmo = satellite.meanan_;
    tle_xno = satellite.meanmo_;
    tle_revnum = satellite.orbitnum_;

    /* Clear all flags */
    is_sdp4_ = epoch_restart = synchronous = resonance = do_loop =
        lunar_terms_done = simple = sdp4_initialized = sgp4_initialized = false;
    SelectEphemeris();
}

void SatelliteCalc::Calc(double daynum) {
    // Zero vector for initializations
    vector_t zero_vector = {0, 0, 0, 0};

    // Satellite position and velocity vectors
    vector_t vel = zero_vector;
    vector_t pos = zero_vector;

    // Satellite's predicted geodetic position
    geodetic_t sat_geodetic;

    double jul_utc = daynum + 2444238.5;

    // Convert satellite's epoch time to Julian
    // and calculate time since epoch in minutes
    double jul_epoch = JulianDateofEpoch(tle_epoch);
    double tsince = (jul_utc - jul_epoch) * MINDAY;
    age = jul_utc - jul_epoch;

    // Call NORAD routines according to deep-space flag.
    if (is_sdp4_) {
        SDP4(tsince, pos, vel);
    } else {
        SGP4(tsince, pos, vel);
    }

    // Scale position and velocity vectors to km and km/sec
    pos.Scale(XKMPER);
    vel.Scale(XKMPER * MINDAY / SECDAY);

    // Calculate velocity of satellite
    vel.Magnitude();
    sat_vel = vel.w;

    // All angles in rads. Distance in km. Velocity in km/s
    CalculateLatLonAlt(jul_utc, pos, sat_geodetic);

    // Convert satellite data
    sat_lat = sat_geodetic.lat / DEG2RAD;
    sat_lon = sat_geodetic.lon / DEG2RAD;
    sat_alt = sat_geodetic.alt;
}

double SatelliteCalc::ThetaG() {
    // The function ThetaG calculates the Greenwich Mean Sidereal Time
    // for an epoch specified in the format used in the NORAD two-line
    // element sets. It has now been adapted for dates beyond the year
    // 1999, as described above. The function ThetaG_JD provides the
    // same calculation except that it is based on an input in the
    // form of a Julian Date.
    // Reference:  The 1992 Astronomical Almanac, page B6.

    double year;

    /* Modification to support Y2K */
    /* Valid 1957 through 2056     */

    double day = modf(tle_epoch * 1E-3, &year) * 1E3;
    year += year < 57 ? 2000 : 1900;

    double ut = modf(day, &day);
    double jd = JulianDateofYear(year) + day;
    deep_arg.ds50 = jd - 2433281.5 + ut;
    return FMod2p(6.3003880987 * deep_arg.ds50 + 1.72944494);
}

void SatelliteCalc::SelectEphemeris() {
    // Selects the apropriate ephemeris type to be used
    // for predictions according to the data in the TLE
    // It also processes values in the TLE set so that
    // they are apropriate for the SGP4/SDP4 routines

    // Preprocess tle set
    tle_xnodeo *= DEG2RAD;
    tle_omegao *= DEG2RAD;
    tle_xmo *= DEG2RAD;
    tle_xincl *= DEG2RAD;
    const double TEMP_C = TWOPI / MINDAY / MINDAY;
    tle_xno = tle_xno * TEMP_C * MINDAY;
    tle_xndt2o *= TEMP_C;
    tle_xndd6o = tle_xndd6o * TEMP_C / MINDAY;
    tle_bstar /= AE;

    // Period > 225 minutes is deep space
    double dd1 = (XKE / tle_xno);
    double dd2 = TOTHRD;
    double a1 = pow(dd1, dd2);
    double r1 = cos(tle_xincl);
    dd1 = (1.0 - tle_eo * tle_eo);
    double temp = CK2 * 1.5f * (r1 * r1 * 3.0 - 1.0) / pow(dd1, 1.5);
    double del1 = temp / (a1 * a1);
    double ao = a1
            * (1.0
                    - del1
                            * (TOTHRD * .5
                                    + del1 * (del1 * 1.654320987654321 + 1.0)));
    double delo = temp / (ao * ao);
    double xnodp = tle_xno / (delo + 1.0);

    // Select a deep-space/near-earth ephemeris
    is_sdp4_ = TWOPI / xnodp / MINDAY >= 0.15625;
}

void SatelliteCalc::SGP4(double tsince, vector_t &pos, vector_t &vel) {
    // This function is used to calculate the position and velocity
    // of near-earth (period < 225 minutes) satellites. tsince is
    // time since epoch in minutes, pos and vel are vector_t structures
    // returning ECI satellite position and velocity.
    double cosuk, sinuk, rfdotk, vx, vy, vz, ux, uy, uz, xmy, xmx, cosnok,
            sinnok, cosik, sinik, rdotk, xinck, xnodek, uk, rk, cos2u, sin2u, u,
            sinu, cosu, betal, rfdot, rdot, r, pl, elsq, esine, ecose, epw,
            cosepw, x1m5th, xhdot1, tfour, sinepw, capu, ayn, xlt, aynl, xll,
            axn, xn, beta, xl, e, a, tcube, delm, delomg, templ, tempe, tempa,
            xnode, tsq, xmp, omega, xnoddf, omgadf, xmdf, a1, a3ovk2, ao, betao,
            betao2, c1sq, c2, c3, coef, coef1, del1, delo, eeta, eosq, etasq,
            perigee, pinvsq, psisq, qoms24, s4, temp, temp1, temp2, temp3,
            temp4, temp5, temp6, theta2, theta4, tsi;

    int i;

    // Initialization
    if (!sgp4_initialized) {
        sgp4_initialized = true;

        // Recover original mean motion (xnodp) and
        // semimajor axis (aodp) from input elements.
        a1 = pow(XKE / tle_xno, TOTHRD);
        cosio = cos(tle_xincl);
        theta2 = cosio * cosio;
        x3thm1 = 3 * theta2 - 1.0;
        eosq = tle_eo * tle_eo;
        betao2 = 1.0 - eosq;
        betao = sqrt(betao2);
        del1 = 1.5 * CK2 * x3thm1 / (a1 * a1 * betao * betao2);
        ao = a1
                * (1.0
                        - del1
                                * (0.5 * TOTHRD
                                        + del1 * (1.0 + 134.0 / 81.0 * del1)));
        delo = 1.5 * CK2 * x3thm1 / (ao * ao * betao * betao2);
        xnodp = tle_xno / (1.0 + delo);
        aodp = ao / (1.0 - delo);

        // For perigee less than 220 kilometers, the "simple"
        // flag is set and the equations are truncated to linear
        // variation in sqrt a and quadratic variation in mean
        // anomaly.  Also, the c3 term, the delta omega term, and
        // the delta m term are dropped.

        simple = (aodp * (1 - tle_eo) / AE) < (220 / XKMPER + AE);

        // For perigees below 156 km, the values of s and QOMS2T are altered.
        s4 = S4_INIT;
        qoms24 = QOMS2T;
        perigee = (aodp * (1 - tle_eo) - AE) * XKMPER;

        if (perigee < 156.0) {
            s4 = perigee <= 98.0 ? 20 : perigee - 78.0;
            qoms24 = pow((120 - s4) * AE / XKMPER, 4);
            s4 = s4 / XKMPER + AE;
        }

        pinvsq = 1 / (aodp * aodp * betao2 * betao2);
        tsi = 1 / (aodp - s4);
        eta = aodp * tle_eo * tsi;
        etasq = eta * eta;
        eeta = tle_eo * eta;
        psisq = fabs(1 - etasq);
        coef = qoms24 * pow(tsi, 4);
        coef1 = coef / pow(psisq, 3.5);
        c2 = coef1 * xnodp
                * (aodp * (1 + 1.5 * etasq + eeta * (4 + etasq))
                        + 0.75 * CK2 * tsi / psisq * x3thm1
                                * (8 + 3 * etasq * (8 + etasq)));
        c1 = tle_bstar * c2;
        sinio = sin(tle_xincl);
        a3ovk2 = -XJ3 / CK2 * pow(AE, 3);
        c3 = coef * tsi * a3ovk2 * xnodp * AE * sinio / tle_eo;
        x1mth2 = 1 - theta2;

        c4 = 2 * xnodp * coef1 * aodp * betao2
                * (eta * (2 + 0.5 * etasq) + tle_eo * (0.5 + 2 * etasq)
                        - 2 * CK2 * tsi / (aodp * psisq)
                                * (-3 * x3thm1
                                        * (1 - 2 * eeta
                                                + etasq * (1.5 - 0.5 * eeta))
                                        + 0.75 * x1mth2
                                                * (2 * etasq
                                                        - eeta * (1 + etasq))
                                                * cos(2 * tle_omegao)));
        c5 = 2 * coef1 * aodp * betao2
                * (1 + 2.75 * (etasq + eeta) + eeta * etasq);

        theta4 = theta2 * theta2;
        temp1 = 3 * CK2 * pinvsq * xnodp;
        temp2 = temp1 * CK2 * pinvsq;
        temp3 = 1.25 * CK4 * pinvsq * pinvsq * xnodp;
        xmdot = xnodp + 0.5 * temp1 * betao * x3thm1
                + 0.0625 * temp2 * betao * (13 - 78 * theta2 + 137 * theta4);
        x1m5th = 1 - 5 * theta2;
        omgdot = -0.5 * temp1 * x1m5th
                + 0.0625 * temp2 * (7 - 114 * theta2 + 395 * theta4)
                + temp3 * (3 - 36 * theta2 + 49 * theta4);
        xhdot1 = -temp1 * cosio;
        xnodot = xhdot1
                + (0.5 * temp2 * (4 - 19 * theta2)
                        + 2 * temp3 * (3 - 7 * theta2)) * cosio;
        omgcof = tle_bstar * c3 * cos(tle_omegao);
        xmcof = -TOTHRD * coef * tle_bstar * AE / eeta;
        xnodcf = 3.5 * betao2 * xhdot1 * c1;
        t2cof = 1.5 * c1;
        xlcof = 0.125 * a3ovk2 * sinio * (3 + 5 * cosio) / (1 + cosio);
        aycof = 0.25 * a3ovk2 * sinio;
        delmo = pow(1 + eta * cos(tle_xmo), 3);
        sinmo = sin(tle_xmo);
        x7thm1 = 7 * theta2 - 1;

        if (!simple) {
            c1sq = c1 * c1;
            d2 = 4 * aodp * tsi * c1sq;
            temp = d2 * tsi * c1 / 3;
            d3 = (17 * aodp + s4) * temp;
            d4 = 0.5 * temp * aodp * tsi * (221 * aodp + 31 * s4) * c1;
            t3cof = d2 + 2 * c1sq;
            t4cof = 0.25 * (3 * d3 + c1 * (12 * d2 + 10 * c1sq));
            t5cof = 0.2
                    * (3 * d4 + 12 * c1 * d3 + 6 * d2 * d2
                            + 15 * c1sq * (2 * d2 + c1sq));
        }
    }

    // Update for secular gravity and atmospheric drag.
    xmdf = tle_xmo + xmdot * tsince;
    omgadf = tle_omegao + omgdot * tsince;
    xnoddf = tle_xnodeo + xnodot * tsince;
    omega = omgadf;
    xmp = xmdf;
    tsq = tsince * tsince;
    xnode = xnoddf + xnodcf * tsq;
    tempa = 1 - c1 * tsince;
    tempe = tle_bstar * c4 * tsince;
    templ = t2cof * tsq;

    if (!simple) {
        delomg = omgcof * tsince;
        delm = xmcof * (pow(1 + eta * cos(xmdf), 3) - delmo);
        temp = delomg + delm;
        xmp = xmdf + temp;
        omega = omgadf - temp;
        tcube = tsq * tsince;
        tfour = tsince * tcube;
        tempa = tempa - d2 * tsq - d3 * tcube - d4 * tfour;
        tempe = tempe + tle_bstar * c5 * (sin(xmp) - sinmo);
        templ = templ + t3cof * tcube + tfour * (t4cof + tsince * t5cof);
    }

    a = aodp * pow(tempa, 2);
    e = tle_eo - tempe;
    xl = xmp + omega + xnode + xnodp * templ;
    beta = sqrt(1 - e * e);
    xn = XKE / pow(a, 1.5);

    // Long period periodics
    axn = e * cos(omega);
    temp = 1 / (a * beta * beta);
    xll = temp * xlcof * axn;
    aynl = temp * aycof;
    xlt = xl + xll;
    ayn = e * sin(omega) + aynl;

    // Solve Kepler's Equation
    capu = FMod2p(xlt - xnode);
    temp2 = capu;
    i = 0;

    do {
        sinepw = sin(temp2);
        cosepw = cos(temp2);
        temp3 = axn * sinepw;
        temp4 = ayn * cosepw;
        temp5 = axn * cosepw;
        temp6 = ayn * sinepw;
        epw = (capu - temp4 + temp3 - temp2) / (1 - temp5 - temp6) + temp2;

        if (fabs(epw - temp2) <= E6A) {
            break;
        }

        temp2 = epw;

    } while (i++ < 10);

    // Short period preliminary quantities
    ecose = temp5 + temp6;
    esine = temp3 - temp4;
    elsq = axn * axn + ayn * ayn;
    temp = 1 - elsq;
    pl = a * temp;
    r = a * (1 - ecose);
    temp1 = 1 / r;
    rdot = XKE * sqrt(a) * esine * temp1;
    rfdot = XKE * sqrt(pl) * temp1;
    temp2 = a * temp1;
    betal = sqrt(temp);
    temp3 = 1 / (1 + betal);
    cosu = temp2 * (cosepw - axn + ayn * esine * temp3);
    sinu = temp2 * (sinepw - ayn - axn * esine * temp3);
    u = AcTan(sinu, cosu);
    sin2u = 2 * sinu * cosu;
    cos2u = 2 * cosu * cosu - 1;
    temp = 1 / pl;
    temp1 = CK2 * temp;
    temp2 = temp1 * temp;

    // Update for short periodics
    rk = r * (1 - 1.5 * temp2 * betal * x3thm1) + 0.5 * temp1 * x1mth2 * cos2u;
    uk = u - 0.25 * temp2 * x7thm1 * sin2u;
    xnodek = xnode + 1.5 * temp2 * cosio * sin2u;
    xinck = tle_xincl + 1.5 * temp2 * cosio * sinio * cos2u;
    rdotk = rdot - xn * temp1 * x1mth2 * sin2u;
    rfdotk = rfdot + xn * temp1 * (x1mth2 * cos2u + 1.5 * x3thm1);

    // Orientation vectors
    sinuk = sin(uk);
    cosuk = cos(uk);
    sinik = sin(xinck);
    cosik = cos(xinck);
    sinnok = sin(xnodek);
    cosnok = cos(xnodek);
    xmx = -sinnok * cosik;
    xmy = cosnok * cosik;
    ux = xmx * sinuk + cosnok * cosuk;
    uy = xmy * sinuk + sinnok * cosuk;
    uz = sinik * sinuk;
    vx = xmx * cosuk - cosnok * sinuk;
    vy = xmy * cosuk - sinnok * sinuk;
    vz = sinik * cosuk;

    // Position and velocity
    pos.x = rk * ux;
    pos.y = rk * uy;
    pos.z = rk * uz;
    vel.x = rdotk * ux + rfdotk * vx;
    vel.y = rdotk * uy + rfdotk * vy;
    vel.z = rdotk * uz + rfdotk * vz;
}

void SatelliteCalc::Deep(int ientry) {
    // This function is used by SDP4 to add lunar and solar
    // perturbation effects to deep-space orbit objects.
    double a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, ainv2, alfdp, aqnv, sgh,
            sini2, sinis, sinok, sh, si, sil, day, betdp, dalf, bfact, c, cc,
            cosis, cosok, cosq, ctem, f322, zx, zy, dbet, dls, eoc, eq, f2,
            f220, f221, f3, f311, f321, xnoh, f330, f441, f442, f522, f523,
            f542, f543, g200, g201, g211, pgh, ph, s1, s2, s3, s4, s5, s6, s7,
            se, sel, ses, xls, g300, g310, g322, g410, g422, g520, g521, g532,
            g533, gam, sinq, sinzf, sis, sl, sll, sls, stem, temp, temp1, x1,
            x2, x2li, x2omi, x3, x4, x5, x6, x7, x8, xl, xldot, xmao, xnddt,
            xndot, xno2, xnodce, xnoi, xomi, xpidot, z1, z11, z12, z13, z2, z21,
            z22, z23, z3, z31, z32, z33, ze, zf, zm, zmo, zn, zsing, zsinh,
            zsini, zcosg, zcosh, zcosi, delt = 0, ft = 0;

    switch (ientry) {
    case DPINIT: // Entrance for deep space initialization
        thgr = ThetaG();
        eq = tle_eo;
        xnq = deep_arg.xnodp;
        aqnv = 1 / deep_arg.aodp;
        xqncl = tle_xincl;
        xmao = tle_xmo;
        xpidot = deep_arg.omgdot + deep_arg.xnodot;
        sinq = sin(tle_xnodeo);
        cosq = cos(tle_xnodeo);
        omegaq = tle_omegao;

        // Initialize lunar solar terms
        day = deep_arg.ds50 + 18261.5; // Days since 1900 Jan 0.5

        if (day != preep) {
            preep = day;
            xnodce = 4.5236020 - 9.2422029E-4 * day;
            stem = sin(xnodce);
            ctem = cos(xnodce);
            zcosil = 0.91375164 - 0.03568096 * ctem;
            zsinil = sqrt(1 - zcosil * zcosil);
            zsinhl = 0.089683511 * stem / zsinil;
            zcoshl = sqrt(1 - zsinhl * zsinhl);
            c = 4.7199672 + 0.22997150 * day;
            gam = 5.8351514 + 0.0019443680 * day;
            zmol = FMod2p(c - gam);
            zx = 0.39785416 * stem / zsinil;
            zy = zcoshl * ctem + 0.91744867 * zsinhl * stem;
            zx = AcTan(zx, zy);
            zx = gam + zx - xnodce;
            zcosgl = cos(zx);
            zsingl = sin(zx);
            zmos = 6.2565837 + 0.017201977 * day;
            zmos = FMod2p(zmos);
        }

        // Do solar terms
        savtsn = 1E20;
        zcosg = ZCOSGS;
        zsing = ZSINGS;
        zcosi = ZCOSIS;
        zsini = ZSINIS;
        zcosh = cosq;
        zsinh = sinq;
        cc = C1SS;
        zn = ZNS;
        ze = ZES;
        zmo = zmos;
        xnoi = 1 / xnq;

        // Loop breaks when Solar terms are done a second
        // time, after Lunar terms are initialized

        for (;;) {
            // Solar terms done again after Lunar terms are done
            a1 = zcosg * zcosh + zsing * zcosi * zsinh;
            a3 = -zsing * zcosh + zcosg * zcosi * zsinh;
            a7 = -zcosg * zsinh + zsing * zcosi * zcosh;
            a8 = zsing * zsini;
            a9 = zsing * zsinh + zcosg * zcosi * zcosh;
            a10 = zcosg * zsini;
            a2 = deep_arg.cosio * a7 + deep_arg.sinio * a8;
            a4 = deep_arg.cosio * a9 + deep_arg.sinio * a10;
            a5 = -deep_arg.sinio * a7 + deep_arg.cosio * a8;
            a6 = -deep_arg.sinio * a9 + deep_arg.cosio * a10;
            x1 = a1 * deep_arg.cosg + a2 * deep_arg.sing;
            x2 = a3 * deep_arg.cosg + a4 * deep_arg.sing;
            x3 = -a1 * deep_arg.sing + a2 * deep_arg.cosg;
            x4 = -a3 * deep_arg.sing + a4 * deep_arg.cosg;
            x5 = a5 * deep_arg.sing;
            x6 = a6 * deep_arg.sing;
            x7 = a5 * deep_arg.cosg;
            x8 = a6 * deep_arg.cosg;
            z31 = 12 * x1 * x1 - 3 * x3 * x3;
            z32 = 24 * x1 * x2 - 6 * x3 * x4;
            z33 = 12 * x2 * x2 - 3 * x4 * x4;
            z1 = 3 * (a1 * a1 + a2 * a2) + z31 * deep_arg.eosq;
            z2 = 6 * (a1 * a3 + a2 * a4) + z32 * deep_arg.eosq;
            z3 = 3 * (a3 * a3 + a4 * a4) + z33 * deep_arg.eosq;
            z11 = -6 * a1 * a5 + deep_arg.eosq * (-24 * x1 * x7 - 6 * x3 * x5);
            z12 = -6 * (a1 * a6 + a3 * a5)
                    + deep_arg.eosq
                            * (-24 * (x2 * x7 + x1 * x8)
                                    - 6 * (x3 * x6 + x4 * x5));
            z13 = -6 * a3 * a6 + deep_arg.eosq * (-24 * x2 * x8 - 6 * x4 * x6);
            z21 = 6 * a2 * a5 + deep_arg.eosq * (24 * x1 * x5 - 6 * x3 * x7);
            z22 = 6 * (a4 * a5 + a2 * a6)
                    + deep_arg.eosq
                            * (24 * (x2 * x5 + x1 * x6)
                                    - 6 * (x4 * x7 + x3 * x8));
            z23 = 6 * a4 * a6 + deep_arg.eosq * (24 * x2 * x6 - 6 * x4 * x8);
            z1 = z1 + z1 + deep_arg.betao2 * z31;
            z2 = z2 + z2 + deep_arg.betao2 * z32;
            z3 = z3 + z3 + deep_arg.betao2 * z33;
            s3 = cc * xnoi;
            s2 = -0.5 * s3 / deep_arg.betao;
            s4 = s3 * deep_arg.betao;
            s1 = -15 * eq * s4;
            s5 = x1 * x3 + x2 * x4;
            s6 = x2 * x3 + x1 * x4;
            s7 = x2 * x4 - x1 * x3;
            se = s1 * zn * s5;
            si = s2 * zn * (z11 + z13);
            sl = -zn * s3 * (z1 + z3 - 14 - 6 * deep_arg.eosq);
            sgh = s4 * zn * (z31 + z33 - 6);
            sh = -zn * s2 * (z21 + z23);

            if (xqncl < 5.2359877E-2) {
                sh = 0;
            }

            ee2 = 2 * s1 * s6;
            e3 = 2 * s1 * s7;
            xi2 = 2 * s2 * z12;
            xi3 = 2 * s2 * (z13 - z11);
            xl2 = -2 * s3 * z2;
            xl3 = -2 * s3 * (z3 - z1);
            xl4 = -2 * s3 * (-21 - 9 * deep_arg.eosq) * ze;
            xgh2 = 2 * s4 * z32;
            xgh3 = 2 * s4 * (z33 - z31);
            xgh4 = -18 * s4 * ze;
            xh2 = -2 * s2 * z22;
            xh3 = -2 * s2 * (z23 - z21);

            if (lunar_terms_done) {
                break;
            }

            // Do lunar terms
            sse = se;
            ssi = si;
            ssl = sl;
            ssh = sh / deep_arg.sinio;
            ssg = sgh - deep_arg.cosio * ssh;
            se2 = ee2;
            si2 = xi2;
            sl2 = xl2;
            sgh2 = xgh2;
            sh2 = xh2;
            se3 = e3;
            si3 = xi3;
            sl3 = xl3;
            sgh3 = xgh3;
            sh3 = xh3;
            sl4 = xl4;
            sgh4 = xgh4;
            zcosg = zcosgl;
            zsing = zsingl;
            zcosi = zcosil;
            zsini = zsinil;
            zcosh = zcoshl * cosq + zsinhl * sinq;
            zsinh = sinq * zcoshl - cosq * zsinhl;
            zn = ZNL;
            cc = C1L;
            ze = ZEL;
            zmo = zmol;
            lunar_terms_done = true;
        }

        sse = sse + se;
        ssi = ssi + si;
        ssl = ssl + sl;
        ssg = ssg + sgh - deep_arg.cosio / deep_arg.sinio * sh;
        ssh = ssh + sh / deep_arg.sinio;

        // Geopotential resonance initialization for 12 hour orbits
        resonance = false;
        synchronous = false;

        if (!((xnq < 0.0052359877) && (xnq > 0.0034906585))) {
            if ((xnq < 0.00826) || (xnq > 0.00924)) {
                return;
            }

            if (eq < 0.5) {
                return;
            }

            resonance = true;
            eoc = eq * deep_arg.eosq;
            g201 = -0.306 - (eq - 0.64) * 0.440;

            if (eq <= 0.65) {
                g211 = 3.616 - 13.247 * eq + 16.290 * deep_arg.eosq;
                g310 = -19.302 + 117.390 * eq - 228.419 * deep_arg.eosq
                        + 156.591 * eoc;
                g322 = -18.9068 + 109.7927 * eq - 214.6334 * deep_arg.eosq
                        + 146.5816 * eoc;
                g410 = -41.122 + 242.694 * eq - 471.094 * deep_arg.eosq
                        + 313.953 * eoc;
                g422 = -146.407 + 841.880 * eq - 1629.014 * deep_arg.eosq
                        + 1083.435 * eoc;
                g520 = -532.114 + 3017.977 * eq - 5740 * deep_arg.eosq
                        + 3708.276 * eoc;
            } else {
                g211 = -72.099 + 331.819 * eq - 508.738 * deep_arg.eosq
                        + 266.724 * eoc;
                g310 = -346.844 + 1582.851 * eq - 2415.925 * deep_arg.eosq
                        + 1246.113 * eoc;
                g322 = -342.585 + 1554.908 * eq - 2366.899 * deep_arg.eosq
                        + 1215.972 * eoc;
                g410 = -1052.797 + 4758.686 * eq - 7193.992 * deep_arg.eosq
                        + 3651.957 * eoc;
                g422 = -3581.69 + 16178.11 * eq - 24462.77 * deep_arg.eosq
                        + 12422.52 * eoc;

                if (eq <= 0.715) {
                    g520 = 1464.74 - 4664.75 * eq + 3763.64 * deep_arg.eosq;
                } else {
                    g520 = -5149.66 + 29936.92 * eq - 54087.36 * deep_arg.eosq
                            + 31324.56 * eoc;
                }
            }

            if (eq < 0.7) {
                g533 = -919.2277 + 4988.61 * eq - 9064.77 * deep_arg.eosq
                        + 5542.21 * eoc;
                g521 = -822.71072 + 4568.6173 * eq - 8491.4146 * deep_arg.eosq
                        + 5337.524 * eoc;
                g532 = -853.666 + 4690.25 * eq - 8624.77 * deep_arg.eosq
                        + 5341.4 * eoc;
            } else {
                g533 = -37995.78 + 161616.52 * eq - 229838.2 * deep_arg.eosq
                        + 109377.94 * eoc;
                g521 = -51752.104 + 218913.95 * eq - 309468.16 * deep_arg.eosq
                        + 146349.42 * eoc;
                g532 = -40023.88 + 170470.89 * eq - 242699.48 * deep_arg.eosq
                        + 115605.82 * eoc;
            }

            sini2 = deep_arg.sinio * deep_arg.sinio;
            f220 = 0.75 * (1 + 2 * deep_arg.cosio + deep_arg.theta2);
            f221 = 1.5 * sini2;
            f321 = 1.875 * deep_arg.sinio
                    * (1 - 2 * deep_arg.cosio - 3 * deep_arg.theta2);
            f322 = -1.875 * deep_arg.sinio
                    * (1 + 2 * deep_arg.cosio - 3 * deep_arg.theta2);
            f441 = 35 * sini2 * f220;
            f442 = 39.3750 * sini2 * sini2;
            f522 = 9.84375 * deep_arg.sinio
                    * (sini2 * (1 - 2 * deep_arg.cosio - 5 * deep_arg.theta2)
                            + 0.33333333
                                    * (-2 + 4 * deep_arg.cosio
                                            + 6 * deep_arg.theta2));
            f523 = deep_arg.sinio
                    * (4.92187512 * sini2
                            * (-2 - 4 * deep_arg.cosio + 10 * deep_arg.theta2)
                            + 6.56250012
                                    * (1 + 2 * deep_arg.cosio
                                            - 3 * deep_arg.theta2));
            f542 = 29.53125 * deep_arg.sinio
                    * (2 - 8 * deep_arg.cosio
                            + deep_arg.theta2
                                    * (-12 + 8 * deep_arg.cosio
                                            + 10 * deep_arg.theta2));
            f543 = 29.53125 * deep_arg.sinio
                    * (-2 - 8 * deep_arg.cosio
                            + deep_arg.theta2
                                    * (12 + 8 * deep_arg.cosio
                                            - 10 * deep_arg.theta2));
            xno2 = xnq * xnq;
            ainv2 = aqnv * aqnv;
            temp1 = 3 * xno2 * ainv2;
            temp = temp1 * ROOT22;
            d2201 = temp * f220 * g201;
            d2211 = temp * f221 * g211;
            temp1 = temp1 * aqnv;
            temp = temp1 * ROOT32;
            d3210 = temp * f321 * g310;
            d3222 = temp * f322 * g322;
            temp1 = temp1 * aqnv;
            temp = 2 * temp1 * ROOT44;
            d4410 = temp * f441 * g410;
            d4422 = temp * f442 * g422;
            temp1 = temp1 * aqnv;
            temp = temp1 * ROOT52;
            d5220 = temp * f522 * g520;
            d5232 = temp * f523 * g532;
            temp = 2 * temp1 * ROOT54;
            d5421 = temp * f542 * g521;
            d5433 = temp * f543 * g533;
            xlamo = xmao + tle_xnodeo + tle_xnodeo - thgr - thgr;
            bfact = deep_arg.xmdot + deep_arg.xnodot + deep_arg.xnodot - THDT
                    - THDT;
            bfact = bfact + ssl + ssh + ssh;
        } else {
            resonance = true;
            synchronous = true;

            // Synchronous resonance terms initialization
            g200 = 1 + deep_arg.eosq * (-2.5 + 0.8125 * deep_arg.eosq);
            g310 = 1 + 2 * deep_arg.eosq;
            g300 = 1 + deep_arg.eosq * (-6 + 6.60937 * deep_arg.eosq);
            f220 = 0.75 * (1 + deep_arg.cosio) * (1 + deep_arg.cosio);
            f311 = 0.9375 * deep_arg.sinio * deep_arg.sinio
                    * (1 + 3 * deep_arg.cosio) - 0.75 * (1 + deep_arg.cosio);
            f330 = 1 + deep_arg.cosio;
            f330 = 1.875 * f330 * f330 * f330;
            del1 = 3 * xnq * xnq * aqnv * aqnv;
            del2 = 2 * del1 * f220 * g200 * Q22;
            del3 = 3 * del1 * f330 * g300 * Q33 * aqnv;
            del1 = del1 * f311 * g310 * Q31 * aqnv;
            fasx2 = 0.13130908;
            fasx4 = 2.8843198;
            fasx6 = 0.37448087;
            xlamo = xmao + tle_xnodeo + tle_omegao - thgr;
            bfact = deep_arg.xmdot + xpidot - THDT;
            bfact = bfact + ssl + ssg + ssh;
        }

        xfact = bfact - xnq;

        // Initialize integrator
        xli = xlamo;
        xni = xnq;
        atime = 0;
        stepp = 720;
        stepn = -720;
        step2 = 259200;

        return;

    case DPSEC: // Entrance for deep space secular effects
        deep_arg.xll = deep_arg.xll + ssl * deep_arg.t;
        deep_arg.omgadf = deep_arg.omgadf + ssg * deep_arg.t;
        deep_arg.xnode = deep_arg.xnode + ssh * deep_arg.t;
        deep_arg.em = tle_eo + sse * deep_arg.t;
        deep_arg.xinc = tle_xincl + ssi * deep_arg.t;

        if (deep_arg.xinc < 0) {
            deep_arg.xinc = -deep_arg.xinc;
            deep_arg.xnode = deep_arg.xnode + M_PI;
            deep_arg.omgadf = deep_arg.omgadf - M_PI;
        }

        if (!resonance) {
            return;
        }

        do {
            if ((atime == 0) || ((deep_arg.t >= 0) && (atime < 0))
                    || ((deep_arg.t < 0) && (atime >= 0))) {
                /* Epoch restart */
                delt = deep_arg.t >= 0 ? stepp : stepn;
                atime = 0;
                xni = xnq;
                xli = xlamo;
            } else {
                if (fabs(deep_arg.t) >= fabs(atime)) {
                    delt = deep_arg.t > 0 ? stepp : stepn;
                }
            }

            do {
                if (fabs(deep_arg.t - atime) >= stepp) {
                    do_loop = true;
                    epoch_restart = false;
                } else {
                    ft = deep_arg.t - atime;
                    do_loop = false;
                }

                if (fabs(deep_arg.t) < fabs(atime)) {
                    delt = deep_arg.t >= 0 ? stepn : stepp;
                    do_loop = epoch_restart = true;
                }

                // Dot terms calculated
                if (synchronous) {
                    xndot = del1 * sin(xli - fasx2)
                            + del2 * sin(2 * (xli - fasx4))
                            + del3 * sin(3 * (xli - fasx6));
                    xnddt = del1 * cos(xli - fasx2)
                            + 2 * del2 * cos(2 * (xli - fasx4))
                            + 3 * del3 * cos(3 * (xli - fasx6));
                } else {
                    xomi = omegaq + deep_arg.omgdot * atime;
                    x2omi = xomi + xomi;
                    x2li = xli + xli;
                    xndot = d2201 * sin(x2omi + xli - G22)
                            + d2211 * sin(xli - G22)
                            + d3210 * sin(xomi + xli - G32)
                            + d3222 * sin(-xomi + xli - G32)
                            + d4410 * sin(x2omi + x2li - G44)
                            + d4422 * sin(x2li - G44)
                            + d5220 * sin(xomi + xli - G52)
                            + d5232 * sin(-xomi + xli - G52)
                            + d5421 * sin(xomi + x2li - G54)
                            + d5433 * sin(-xomi + x2li - G54);
                    xnddt = d2201 * cos(x2omi + xli - G22)
                            + d2211 * cos(xli - G22)
                            + d3210 * cos(xomi + xli - G32)
                            + d3222 * cos(-xomi + xli - G32)
                            + d5220 * cos(xomi + xli - G52)
                            + d5232 * cos(-xomi + xli - G52)
                            + 2
                                    * (d4410 * cos(x2omi + x2li - G44)
                                            + d4422 * cos(x2li - G44)
                                            + d5421 * cos(xomi + x2li - G54)
                                            + d5433 * cos(-xomi + x2li - G54));
                }

                xldot = xni + xfact;
                xnddt = xnddt * xldot;

                if (do_loop) {
                    xli = xli + xldot * delt + xndot * step2;
                    xni = xni + xndot * delt + xnddt * step2;
                    atime = atime + delt;
                }
            } while (do_loop && !epoch_restart);
        } while (do_loop && epoch_restart);

        deep_arg.xn = xni + xndot * ft + xnddt * ft * ft * 0.5;
        xl = xli + xldot * ft + xndot * ft * ft * 0.5;
        temp = -deep_arg.xnode + thgr + deep_arg.t * THDT;

        if (!synchronous) {
            deep_arg.xll = xl + temp + temp;
        } else {
            deep_arg.xll = xl - deep_arg.omgadf + temp;
        }

        return;

    case DPPER: // Entrance for lunar-solar periodics
        sinis = sin(deep_arg.xinc);
        cosis = cos(deep_arg.xinc);

        if (fabs(savtsn - deep_arg.t) >= 30) {
            savtsn = deep_arg.t;
            zm = zmos + ZNS * deep_arg.t;
            zf = zm + 2 * ZES * sin(zm);
            sinzf = sin(zf);
            f2 = 0.5 * sinzf * sinzf - 0.25;
            f3 = -0.5 * sinzf * cos(zf);
            ses = se2 * f2 + se3 * f3;
            sis = si2 * f2 + si3 * f3;
            sls = sl2 * f2 + sl3 * f3 + sl4 * sinzf;
            sghs = sgh2 * f2 + sgh3 * f3 + sgh4 * sinzf;
            shs = sh2 * f2 + sh3 * f3;
            zm = zmol + ZNL * deep_arg.t;
            zf = zm + 2 * ZEL * sin(zm);
            sinzf = sin(zf);
            f2 = 0.5 * sinzf * sinzf - 0.25;
            f3 = -0.5 * sinzf * cos(zf);
            sel = ee2 * f2 + e3 * f3;
            sil = xi2 * f2 + xi3 * f3;
            sll = xl2 * f2 + xl3 * f3 + xl4 * sinzf;
            sghl = xgh2 * f2 + xgh3 * f3 + xgh4 * sinzf;
            sh1 = xh2 * f2 + xh3 * f3;
            pe = ses + sel;
            pinc = sis + sil;
            pl = sls + sll;
        }

        pgh = sghs + sghl;
        ph = shs + sh1;
        deep_arg.xinc = deep_arg.xinc + pinc;
        deep_arg.em = deep_arg.em + pe;

        if (xqncl >= 0.2) {
            // Apply periodics directly
            ph = ph / deep_arg.sinio;
            pgh = pgh - deep_arg.cosio * ph;
            deep_arg.omgadf = deep_arg.omgadf + pgh;
            deep_arg.xnode = deep_arg.xnode + ph;
            deep_arg.xll = deep_arg.xll + pl;
        } else {
            // Apply periodics with Lyddane modification
            sinok = sin(deep_arg.xnode);
            cosok = cos(deep_arg.xnode);
            alfdp = sinis * sinok;
            betdp = sinis * cosok;
            dalf = ph * cosok + pinc * cosis * sinok;
            dbet = -ph * sinok + pinc * cosis * cosok;
            alfdp = alfdp + dalf;
            betdp = betdp + dbet;
            deep_arg.xnode = FMod2p(deep_arg.xnode);
            xls = deep_arg.xll + deep_arg.omgadf + cosis * deep_arg.xnode;
            dls = pl + pgh - pinc * deep_arg.xnode * sinis;
            xls = xls + dls;
            xnoh = deep_arg.xnode;
            deep_arg.xnode = AcTan(alfdp, betdp);
            if (fabs(xnoh - deep_arg.xnode) > M_PI) {
                if (deep_arg.xnode < xnoh) {
                    deep_arg.xnode += TWOPI;
                } else {
                    deep_arg.xnode -= TWOPI;
                }
            }
            deep_arg.xll = deep_arg.xll + pl;
            deep_arg.omgadf = xls - deep_arg.xll
                    - cos(deep_arg.xinc) * deep_arg.xnode;
        }
        return;
    }
}

void SatelliteCalc::SDP4(double tsince, vector_t &pos, vector_t &vel) {
    // This function is used to calculate the position and velocity
    // of deep-space (period > 225 minutes) satellites. tsince is
    // time since epoch in minutes, pos and vel are vector_t structures
    // returning ECI satellite position and velocity.

    int i;
    double a, axn, ayn, aynl, beta, betal, capu, cos2u, cosepw, cosik, cosnok,
            cosu, cosuk, ecose, elsq, epw, esine, pl, theta4, rdot, rdotk,
            rfdot, rfdotk, rk, sin2u, sinepw, sinik, sinnok, sinu, sinuk, tempe,
            templ, tsq, u, uk, ux, uy, uz, vx, vy, vz, xinck, xl, xlt, xmam,
            xmdf, xmx, xmy, xnoddf, xnodek, xll, a1, a3ovk2, ao, c2, coef,
            coef1, x1m5th, xhdot1, del1, r, delo, eeta, eta, etasq, perigee,
            psisq, tsi, qoms24, s4, pinvsq, temp, tempa, temp1, temp2, temp3,
            temp4, temp5, temp6;

    // Initialization
    if (!sdp4_initialized) {
        sdp4_initialized = true;

        // Recover original mean motion (xnodp) and
        // semimajor axis (aodp) from input elements.
        a1 = pow(XKE / tle_xno, TOTHRD);
        deep_arg.cosio = cos(tle_xincl);
        deep_arg.theta2 = deep_arg.cosio * deep_arg.cosio;
        x3thm1 = 3 * deep_arg.theta2 - 1;
        deep_arg.eosq = tle_eo * tle_eo;
        deep_arg.betao2 = 1 - deep_arg.eosq;
        deep_arg.betao = sqrt(deep_arg.betao2);
        del1 = 1.5 * CK2 * x3thm1
                / (a1 * a1 * deep_arg.betao * deep_arg.betao2);
        ao = a1 * (1 - del1 * (0.5 * TOTHRD + del1 * (1 + 134 / 81 * del1)));
        delo = 1.5 * CK2 * x3thm1
                / (ao * ao * deep_arg.betao * deep_arg.betao2);
        deep_arg.xnodp = tle_xno / (1 + delo);
        deep_arg.aodp = ao / (1 - delo);

        // For perigee below 156 km, the values of s and QOMS2T are altered.
        s4 = S4_INIT;
        qoms24 = QOMS2T;
        perigee = (deep_arg.aodp * (1 - tle_eo) - AE) * XKMPER;

        if (perigee < 156.0) {
            s4 = perigee <= 98.0 ? 20.0 : perigee - 78.0;
            qoms24 = pow((120 - s4) * AE / XKMPER, 4);
            s4 = s4 / XKMPER + AE;
        }

        pinvsq = 1
                / (deep_arg.aodp * deep_arg.aodp * deep_arg.betao2
                        * deep_arg.betao2);
        deep_arg.sing = sin(tle_omegao);
        deep_arg.cosg = cos(tle_omegao);
        tsi = 1 / (deep_arg.aodp - s4);
        eta = deep_arg.aodp * tle_eo * tsi;
        etasq = eta * eta;
        eeta = tle_eo * eta;
        psisq = fabs(1 - etasq);
        coef = qoms24 * pow(tsi, 4);
        coef1 = coef / pow(psisq, 3.5);
        c2 = coef1 * deep_arg.xnodp
                * (deep_arg.aodp * (1 + 1.5 * etasq + eeta * (4 + etasq))
                        + 0.75 * CK2 * tsi / psisq * x3thm1
                                * (8 + 3 * etasq * (8 + etasq)));
        c1 = tle_bstar * c2;
        deep_arg.sinio = sin(tle_xincl);
        a3ovk2 = -XJ3 / CK2 * pow(AE, 3);
        x1mth2 = 1 - deep_arg.theta2;
        c4 = 2 * deep_arg.xnodp * coef1 * deep_arg.aodp * deep_arg.betao2
                * (eta * (2 + 0.5 * etasq) + tle_eo * (0.5 + 2 * etasq)
                        - 2 * CK2 * tsi / (deep_arg.aodp * psisq)
                                * (-3 * x3thm1
                                        * (1 - 2 * eeta
                                                + etasq * (1.5 - 0.5 * eeta))
                                        + 0.75 * x1mth2
                                                * (2 * etasq
                                                        - eeta * (1 + etasq))
                                                * cos(2 * tle_omegao)));
        theta4 = deep_arg.theta2 * deep_arg.theta2;
        temp1 = 3 * CK2 * pinvsq * deep_arg.xnodp;
        temp2 = temp1 * CK2 * pinvsq;
        temp3 = 1.25 * CK4 * pinvsq * pinvsq * deep_arg.xnodp;
        deep_arg.xmdot = deep_arg.xnodp + 0.5 * temp1 * deep_arg.betao * x3thm1
                + 0.0625 * temp2 * deep_arg.betao
                        * (13 - 78 * deep_arg.theta2 + 137 * theta4);
        x1m5th = 1 - 5 * deep_arg.theta2;
        deep_arg.omgdot = -0.5 * temp1 * x1m5th
                + 0.0625 * temp2 * (7 - 114 * deep_arg.theta2 + 395 * theta4)
                + temp3 * (3 - 36 * deep_arg.theta2 + 49 * theta4);
        xhdot1 = -temp1 * deep_arg.cosio;
        deep_arg.xnodot = xhdot1
                + (0.5 * temp2 * (4 - 19 * deep_arg.theta2)
                        + 2 * temp3 * (3 - 7 * deep_arg.theta2))
                        * deep_arg.cosio;
        xnodcf = 3.5 * deep_arg.betao2 * xhdot1 * c1;
        t2cof = 1.5 * c1;
        xlcof = 0.125 * a3ovk2 * deep_arg.sinio * (3 + 5 * deep_arg.cosio)
                / (1 + deep_arg.cosio);
        aycof = 0.25 * a3ovk2 * deep_arg.sinio;
        x7thm1 = 7 * deep_arg.theta2 - 1;

        // initialize Deep()
        Deep(DPINIT);
    }

    // Update for secular gravity and atmospheric drag
    xmdf = tle_xmo + deep_arg.xmdot * tsince;
    deep_arg.omgadf = tle_omegao + deep_arg.omgdot * tsince;
    xnoddf = tle_xnodeo + deep_arg.xnodot * tsince;
    tsq = tsince * tsince;
    deep_arg.xnode = xnoddf + xnodcf * tsq;
    tempa = 1 - c1 * tsince;
    tempe = tle_bstar * c4 * tsince;
    templ = t2cof * tsq;
    deep_arg.xn = deep_arg.xnodp;

    // Update for deep-space secular effects
    deep_arg.xll = xmdf;
    deep_arg.t = tsince;

    Deep(DPSEC);

    xmdf = deep_arg.xll;
    a = pow(XKE / deep_arg.xn, TOTHRD) * tempa * tempa;
    deep_arg.em = deep_arg.em - tempe;
    xmam = xmdf + deep_arg.xnodp * templ;

    // Update for deep-space periodic effects
    deep_arg.xll = xmam;

    Deep(DPPER);

    xmam = deep_arg.xll;
    xl = xmam + deep_arg.omgadf + deep_arg.xnode;
    beta = sqrt(1 - deep_arg.em * deep_arg.em);
    deep_arg.xn = XKE / pow(a, 1.5);

    // Long period periodics
    axn = deep_arg.em * cos(deep_arg.omgadf);
    temp = 1 / (a * beta * beta);
    xll = temp * xlcof * axn;
    aynl = temp * aycof;
    xlt = xl + xll;
    ayn = deep_arg.em * sin(deep_arg.omgadf) + aynl;

    // Solve Kepler's Equation
    capu = FMod2p(xlt - deep_arg.xnode);
    temp2 = capu;
    i = 0;

    do {
        sinepw = sin(temp2);
        cosepw = cos(temp2);
        temp3 = axn * sinepw;
        temp4 = ayn * cosepw;
        temp5 = axn * cosepw;
        temp6 = ayn * sinepw;
        epw = (capu - temp4 + temp3 - temp2) / (1 - temp5 - temp6) + temp2;

        if (fabs(epw - temp2) <= E6A) {
            break;
        }

        temp2 = epw;

    } while (i++ < 10);

    // Short period preliminary quantities
    ecose = temp5 + temp6;
    esine = temp3 - temp4;
    elsq = axn * axn + ayn * ayn;
    temp = 1 - elsq;
    pl = a * temp;
    r = a * (1 - ecose);
    temp1 = 1 / r;
    rdot = XKE * sqrt(a) * esine * temp1;
    rfdot = XKE * sqrt(pl) * temp1;
    temp2 = a * temp1;
    betal = sqrt(temp);
    temp3 = 1 / (1 + betal);
    cosu = temp2 * (cosepw - axn + ayn * esine * temp3);
    sinu = temp2 * (sinepw - ayn - axn * esine * temp3);
    u = AcTan(sinu, cosu);
    sin2u = 2 * sinu * cosu;
    cos2u = 2 * cosu * cosu - 1;
    temp = 1 / pl;
    temp1 = CK2 * temp;
    temp2 = temp1 * temp;

    // Update for short periodics
    rk = r * (1 - 1.5 * temp2 * betal * x3thm1) + 0.5 * temp1 * x1mth2 * cos2u;
    uk = u - 0.25 * temp2 * x7thm1 * sin2u;
    xnodek = deep_arg.xnode + 1.5 * temp2 * deep_arg.cosio * sin2u;
    xinck = deep_arg.xinc
            + 1.5 * temp2 * deep_arg.cosio * deep_arg.sinio * cos2u;
    rdotk = rdot - deep_arg.xn * temp1 * x1mth2 * sin2u;
    rfdotk = rfdot + deep_arg.xn * temp1 * (x1mth2 * cos2u + 1.5 * x3thm1);

    // Orientation vectors
    sinuk = sin(uk);
    cosuk = cos(uk);
    sinik = sin(xinck);
    cosik = cos(xinck);
    sinnok = sin(xnodek);
    cosnok = cos(xnodek);
    xmx = -sinnok * cosik;
    xmy = cosnok * cosik;
    ux = xmx * sinuk + cosnok * cosuk;
    uy = xmy * sinuk + sinnok * cosuk;
    uz = sinik * sinuk;
    vx = xmx * cosuk - cosnok * sinuk;
    vy = xmy * cosuk - sinnok * sinuk;
    vz = sinik * cosuk;

    // Position and velocity
    pos.x = rk * ux;
    pos.y = rk * uy;
    pos.z = rk * uz;
    vel.x = rdotk * ux + rfdotk * vx;
    vel.y = rdotk * uy + rfdotk * vy;
    vel.z = rdotk * uz + rfdotk * vz;
}

void SatelliteCalc::Update(Satellite& satellite) {
    satellite.sat_lat = sat_lat;
    satellite.sat_lon = sat_lon;
    satellite.sat_alt = sat_alt;
}
