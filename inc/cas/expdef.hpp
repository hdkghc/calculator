/** @file /inc/cas/expdef.hpp
 *  @brief Expression tree node definition for the computer algebra system module of the calculator project
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
#ifndef _CAS_EXPDEF_HPP_
#define _CAS_EXPDEF_HPP_

namespace CAS {
    namespace FuncName {
        // \00x represents that they're built-in functions.

        // Trigonometric functions
        const char *const sin       = "\001si";
        const char *const cos       = "\001co";
        const char *const tan       = "\001ta";
        const char *const asin      = "\001Si";
        const char *const acos      = "\001Co";
        const char *const atan      = "\001At";
        const char *const sinh      = "\001sh";
        const char *const cosh      = "\001ch";
        const char *const tanh      = "\001th";
        const char *const asinh     = "\001Sh";
        const char *const acosh     = "\001Ch";
        const char *const atanh     = "\001Th";

        // Logarithmic and exponential functions
        const char *const ln        = "\001ln";
        const char *const log       = "\001lg";
        const char *const log10     = "\001lX";
        const char *const exp       = "\001ex";
        const char *const sqrt      = "\001sq";
        const char *const root      = "\001rt";

        // Numeric functions
        const char *const abs       = "\001ab";
        const char *const floor     = "\001fl";
        const char *const ceil      = "\001ce";
        const char *const round     = "\001ro";
        const char *const sign      = "\001sg";
        const char *const max       = "\001mx";
        const char *const min       = "\001mn";
        const char *const frac      = "\001fr";
        const char *const fact      = "\001f!";
        const char *const gcd       = "\001gc";
        const char *const lcm       = "\001lc";
        const char *const mod       = "\001md";
        const char *const permut    = "\001pm";
        const char *const combin    = "\001cb";

        // Summation and product functions
        const char *const sum       = "\001sm";
        const char *const prod      = "\001pd";
        const char *const defint    = "\001in"; // definite integral
        const char *const diff      = "\001df";
        const char *const indefint  = "\001id"; // indefinite integral

        // Coordinate transformation functions
        const char *const polar     = "\001pl";
        const char *const rect      = "\001rc";

        // Angle conversion functions
        const char *const deg       = "\001dg";
        const char *const rad       = "\001rd";
        
        // Random number generation functions
        const char *const randrat   = "\001rr";
        const char *const randint   = "\001ri";

        // Complex number functions
        const char *const realpart  = "\001re";
        const char *const imagpart  = "\001im";
        const char *const conjg     = "\001cj";
        const char *const arg       = "\001ar";

        // Vector and matrix functions
        const char *const vector    = "\001vc";
        const char *const dot       = "\001v.";
        const char *const angle     = "\001va";
        const char *const det       = "\001dt";
        const char *const matrix    = "\001mt";
        const char *const transpose = "\001tr";
        const char *const eigenval  = "\001Ex";
        const char *const eigenvec  = "\001Ev";
        const char *const adjoint   = "\001aj";
        const char *const rank      = "\001rk";

        // Statistical functions
        const char *const estx1     = "\001s1"; // estimate x1
        const char *const estx2     = "\001s2"; // estimate x2
        const char *const esty      = "\001sy"; // estimate y

        // Bitwise computation functions
        const char *const band      = "\001b&";
        const char *const bor       = "\001b|";
        const char *const bxor      = "\001b^";
        const char *const bnot      = "\001b~";
        const char *const blshift   = "\001b<";
        const char *const brshift   = "\001b>";
        const char *const bxnor     = "\001!^";
        const char *const bnand     = "\001!&";
        const char *const bnor      = "\001!|";

        // Plotting functions
        const char *const plot2d    = "\001p2";
        const char *const plot3d    = "\001p3";
    } // namespace FuncName
    namespace ConstName {
        const char *const pi        = "\002$p";
        const char *const e         = "\002$e";
        const char *const phi       = "\002$f"; // golden ratio
        const char *const i         = "\002$i"; // imaginary unit

        // scientific constants
        // Commonly used physical constants
        const char *const h         = "\002_h";  // Planck constant
        const char *const hbar      = "\002hb"; // Reduced Planck constant
        const char *const c         = "\002cc"; // speed of light in vacuum
        const char *const eps0      = "\002ep"; // vacuum permittivity
        const char *const mu0       = "\002m0"; // vacuum permeability
        const char *const Z0        = "\002Z0"; // vacuum impedance
        const char *const G         = "\002GG"; // gravitational constant
        const char *const lP        = "\002lP"; // Planck length
        const char *const tP        = "\002tP"; // Planck time
        const char *const mP        = "\002mP"; // Planck mass

        // Electromagnetic constants
        const char *const muN       = "\002uN"; // Nuclear magneton
        const char *const muB       = "\002uB"; // Bohr magneton
        const char *const e0        = "\002e0"; // Elementary charge
        const char *const Phi0      = "\002Ph"; // Magnetic flux quantum
        const char *const G0        = "\002G0"; // Conductance quantum
        const char *const Rk        = "\002Rk"; // von Klitzing constant
        const char *const Kj        = "\002Kj"; // Josephson constant
        
        // Atomic & nuclear constants
        const char *const mp        = "\002mp"; // mass of proton
        const char *const mn        = "\002mn"; // mass of neutron
        const char *const me        = "\002me"; // mass of electron
        const char *const mu        = "\002mu"; // mass of muon
        const char *const a0        = "\002a0"; // Bohr radius
        const char *const alpha     = "\002al"; // Fine-structure constant
        const char *const re        = "\002re"; // Classical electron radius
        const char *const gammaN    = "\002gN"; // Nuclear gyromagnetic ratio
        const char *const gammaP    = "\002gP"; // Proton gyromagnetic ratio
        const char *const gammaNeu  = "\002gn"; // Neutron gyromagnetic ratio
        const char *const gammaE    = "\002gE"; // Electron gyromagnetic ratio
        const char *const gammaMu   = "\002gM"; // Muon gyromagnetic ratio
        const char *const lambdaCp  = "\002lp"; // Proton Compton wavelength
        const char *const lambdaCn  = "\002lN"; // Neutron Compton wavelength
        const char *const lambdaCe  = "\002lE"; // Electron Compton wavelength
        const char *const lambdaCMu = "\002lM"; // Muon Compton wavelength
        const char *const Rinf      = "\002Ri"; // Rydberg constant
        const char *const muP       = "\002uP"; // Proton magnetic moment
        const char *const muNeu     = "\002un"; // Neutron magnetic moment
        const char *const muE       = "\002uE"; // Electron magnetic moment
        const char *const muMu      = "\002uM"; // Muon magnetic moment
        const char *const tauN      = "\002tN"; // Neutron mean lifetime
        const char *const mtau      = "\002tM"; // mass of tau lepton

        // Physical & chemical constants
        const char *const u         = "\002uu"; // atomic mass constant
        const char *const NA        = "\002NA"; // Avogadro constant
        const char *const kB        = "\002kB"; // Boltzmann constant
        const char *const R         = "\002RR"; // molar gas constant
        const char *const F         = "\002FF"; // Faraday constant
        const char *const sigma     = "\002ss"; // Stefan-Boltzmann constant
        const char *const Vm        = "\002Vm"; // molar volume of ideal gas
        const char *const c1        = "\002c1"; // first radiation constant
        const char *const c2        = "\002c2"; // second radiation constant

        // Other physical constants
        const char *const g         = "\002gg"; // standard acceleration of gravity
        const char *const atm       = "\002at"; // standard atmosphere
        const char *const Rk90      = "\002R9"; // von Klitzing constant (1990)
        const char *const Kj90      = "\002K9"; // Josephson constant
        const char *const t         = "\002tt"; // standard temperature
    } // namespace ConstName
} // namespace CAS

#endif // _CAS_EXPDEF_HPP_