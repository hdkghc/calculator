/** @file /inc/cas/constdef.hpp
 *  @brief Mathematical & scientific constant definitions for the computer algebra system module of the calculator project
 *  @author hdkghc
 *  @version 0.1
 *  Copyright (C) 2026 hdkghc (peitongxin@outlook.com)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef _CAS_CONSTDEF_HPP_
#define _CAS_CONSTDEF_HPP_

namespace CAS {
    namespace Constants {
        long double constants[] = {
            0.0L,                                                   //  0: NULL
            3.14159265358979323846264338327950288419716939937510L,  //  1: pi
            2.71828182845904523536028747135266249775724709369996L,  //  2: e
            1.61803398874989484820458683436563811772030917980576L,  //  3: phi (golden ratio)
            0.0L,                                                   //  4: i (imaginary unit, no real value)
            6.62607015e-34L,            //  5: Planck constant              CODATA 2019
            1.0545718176461564e-34L,    //  6: Reduced Planck constant      CODATA 2019
            299792458.0L,               //  7: speed of light in vacuum     CODATA 2019
            8.8541878128e-12L,          //  8: vacuum permittivity          CODATA 2019
            1.2566370614359173e-6L,     //  9: vacuum permeability          CODATA 2019
            376.73031346177L,           // 10: vacuum impedance             CODATA 2019
            6.67430e-11L,               // 11: gravitational constant       CODATA 2019
            1.616255e-35L,              // 12: Planck length                CODATA 2019
            5.391247e-44L,              // 13: Planck time                  CODATA 2019
            2.176434e-8L,               // 14: Planck mass                  CODATA 2019
            5.0507837461e-27L,          // 15: Nuclear magneton             CODATA 2019
            9.274009994e-24L,           // 16: Bohr magneton                CODATA 2019
            1.602176634e-19L,           // 17: Elementary charge            CODATA 2019
            2.067833848461312e-15L,     // 18: Magnetic flux quantum        CODATA 2019
            7.748091729e-5L,            // 19: Conductance quantum          CODATA 2019
            25812.8070L,                // 20: von Klitzing constant        CODATA 2019
            483597.8484e9L,             // 21: Josephson constant           CODATA 2019
            1.67262192369e-27L,         // 22: mass of proton               CODATA 2019
            1.67492749804e-27L,         // 23: mass of neutron              CODATA 2019
            9.1093837015e-31L,          // 24: mass of electron             CODATA 2019
            1.883531627e-28L,           // 25: mass of muon                 CODATA 2019
            0.529177210903e-10L,        // 26: Bohr radius                  CODATA 2019
            7.2973525693e-3L,           // 27: Fine-structure constant      CODATA 2019
            2.8179403262e-15L,          // 28: Classical electron radius    CODATA 2019
            2.6752314908e8L,            // 29: Nuclear gyromagnetic ratio   CODATA 2019
            2.6752314908e8L,            // 30: Proton gyromagnetic ratio    CODATA 2019
            -1.832471792e8L,            // 31: Neutron gyromagnetic ratio   CODATA 2019
            -1.76085963023e11L,         // 32: Electron gyromagnetic ratio  CODATA 2019
            1.353424602e11L,            // 33: Muon gyromagnetic ratio      CODATA 2019
            1.32140985539e-15L,         // 34: Proton Compton wavelength    CODATA 2019
            1.31959090581e-15L,         // 35: Neutron Compton wavelength   CODATA 2019
            2.42631023867e-12L,         // 36: Electron Compton wavelength  CODATA 2019
            1.173444110e-14L,           // 37: Muon Compton wavelength      CODATA 2019
            10973731.568160L,           // 38: Rydberg constant             CODATA 2019
            1.41060679736e-26L,         // 39: Proton magnetic moment       CODATA 2019
            -9.66236470e-27L,           // 40: Neutron magnetic moment      CODATA 2019
            -9.2847647043e-24L,         // 41: Electron magnetic moment     CODATA 2019
            -4.49044830e-26L,           // 42: Muon magnetic moment         CODATA 2019
            879.4L,                     // 43: Neutron mean lifetime        CODATA 2019
            3.16754e-27L,               // 44: mass of tau lepton           CODATA 2019
            1.66053906660e-27L,         // 45: atomic mass constant         CODATA 2019
            6.02214076e23L,             // 46: Avogadro constant            CODATA 2019
            1.380649e-23L,              // 47: Boltzmann constant           CODATA 2019
            8.314462618L,               // 48: molar gas constant           CODATA 2019
            96485.33212L,               // 49: Faraday constant             CODATA 2019
            5.670374419e-8L,            // 50: Stefan-Boltzmann constant    CODATA 2019
            0.02241396954L,             // 51: molar volume of ideal gas    CODATA 2019
            3.741771852e-16L,           // 52: first radiation constant     CODATA 2019
            0.01438775036L,             // 53: second radiation constant    CODATA 2019
            9.80665L,                   // 54: standard accel of gravity    ISO standard
            101325.0L,                  // 55: standard atmosphere          ISO standard
            25812.807L,                 // 56: von Klitzing constant        CODATA 1990
            483597.9e9L,                // 57: Josephson constant           CODATA 1990
            273.15L                     // 58: standard temperature         ITS-90
        }; // long double constants[]
    } // namespace Constants
}

#endif // _CAS_CONSTDEF_HPP_