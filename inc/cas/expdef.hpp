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
        const char *const defint    = "\001in"; ///< definite integral
        const char *const diff      = "\001df";
        const char *const indefint  = "\001id"; ///< indefinite integral

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
        const char *const norm      = "\001nm";
        const char *const det       = "\001dt";
        const char *const matrix    = "\001mt";
        const char *const transpose = "\001tr";
        const char *const eigenval  = "\001Ex";
        const char *const eigenvec  = "\001Ev";
        const char *const adjoint   = "\001aj";
        const char *const rank      = "\001rk";

        // Statistical functions
        const char *const estx1     = "\001s1"; ///< estimate x1
        const char *const estx2     = "\001s2"; ///< estimate x2
        const char *const esty      = "\001sy"; ///< estimate y

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
        const char *const pi        = "\002\001";
        const char *const e         = "\002\002";
        const char *const phi       = "\002\003"; ///< golden ratio
        const char *const i         = "\002\004"; ///< imaginary

        // scientific constants
        // Commonly used physical constants
        const char *const h         = "\002\005"; ///< Planck constant
        const char *const hbar      = "\002\006"; ///< Reduced Planck constant
        const char *const c         = "\002\007"; ///< speed of light in vacuum
        const char *const eps0      = "\002\010"; ///< vacuum permittivity
        const char *const mu0       = "\002\011"; ///< vacuum permeability
        const char *const Z0        = "\002\012"; ///< vacuum impedance
        const char *const G         = "\002\013"; ///< gravitational constant
        const char *const lP        = "\002\014"; ///< Planck length
        const char *const tP        = "\002\015"; ///< Planck time
        const char *const mP        = "\002\016"; ///< Planck mass

        // Electromagnetic constants
        const char *const muN       = "\002\017"; ///< Nuclear magneton
        const char *const muB       = "\002\020"; ///< Bohr magneton
        const char *const e0        = "\002\021"; ///< Elementary charge
        const char *const Phi0      = "\002\022"; ///< Magnetic flux quantum
        const char *const G0        = "\002\023"; ///< Conductance quantum
        const char *const Rk        = "\002\024"; ///< von Klitzing constant
        const char *const Kj        = "\002\025"; ///< Josephson constant
        
        // Atomic & nuclear constants
        const char *const mp        = "\002\026"; ///< mass of proton
        const char *const mn        = "\002\027"; ///< mass of neutron
        const char *const me        = "\002\030"; ///< mass of electron
        const char *const mu        = "\002\031"; ///< mass of muon
        const char *const a0        = "\002\032"; ///< Bohr radius
        const char *const alpha     = "\002\033"; ///< Fine-structure constant
        const char *const re        = "\002\034"; ///< Classical electron radius
        const char *const gammaN    = "\002\035"; ///< Nuclear gyromagnetic ratio
        const char *const gammaP    = "\002\036"; ///< Proton gyromagnetic ratio
        const char *const gammaNeu  = "\002\037"; ///< Neutron gyromagnetic ratio
        const char *const gammaE    = "\002\040"; ///< Electron gyromagnetic ratio
        const char *const gammaMu   = "\002\041"; ///< Muon gyromagnetic ratio
        const char *const lambdaCp  = "\002\042"; ///< Proton Compton wavelength
        const char *const lambdaCn  = "\002\043"; ///< Neutron Compton wavelength
        const char *const lambdaCe  = "\002\044"; ///< Electron Compton wavelength
        const char *const lambdaCMu = "\002\045"; ///< Muon Compton wavelength
        const char *const Rinf      = "\002\046"; ///< Rydberg constant
        const char *const muP       = "\002\047"; ///< Proton magnetic moment
        const char *const muNeu     = "\002\050"; ///< Neutron magnetic moment
        const char *const muE       = "\002\051"; ///< Electron magnetic moment
        const char *const muMu      = "\002\052"; ///< Muon magnetic moment
        const char *const tauN      = "\002\053"; ///< Neutron mean lifetime
        const char *const mtau      = "\002\054"; ///< mass of tau lepton

        // Physical & chemical constants
        const char *const u         = "\002\055"; ///< atomic mass constant
        const char *const NA        = "\002\056"; ///< Avogadro constant
        const char *const kB        = "\002\057"; ///< Boltzmann constant
        const char *const R         = "\002\060"; ///< molar gas constant
        const char *const F         = "\002\061"; ///< Faraday constant
        const char *const sigma     = "\002\062"; ///< Stefan-Boltzmann constant
        const char *const Vm        = "\002\063"; ///< molar volume of ideal gas
        const char *const c1        = "\002\064"; ///< first radiation constant
        const char *const c2        = "\002\065"; ///< second radiation constant

        // Other physical constants
        const char *const g         = "\002\066"; ///< standard acceleration of gravity
        const char *const atm       = "\002\067"; ///< standard atmosphere
        const char *const Rk90      = "\002\070"; ///< von Klitzing constant (1990)
        const char *const Kj90      = "\002\071"; ///< Josephson constant
        const char *const t         = "\002\072"; ///< standard temperature
    } // namespace ConstName
    namespace KeywordName {
        const char *const If        = "if";
        const char *const While     = "while";
        const char *const For       = "for";
        const char *const Do        = "do";
        const char *const Return    = "return";
        const char *const Func      = "func";
        const char *const Call      = "call";
    }
} // namespace CAS

#endif // _CAS_EXPDEF_HPP_