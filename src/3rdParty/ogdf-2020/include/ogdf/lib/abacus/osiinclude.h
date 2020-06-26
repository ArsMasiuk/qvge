
#pragma once

#ifdef COIN_OSI_CBC
#include <coin/OsiCbcSolverInterface.hpp>
#endif
#ifdef COIN_OSI_CLP
#include <coin/OsiClpSolverInterface.hpp>
#endif
#ifdef COIN_OSI_CPX
#include <coin/OsiCpxSolverInterface.hpp>
#endif
#ifdef COIN_OSI_DYLP
#include <coin/OsiDylpSolverInterface.hpp>
#endif
#ifdef COIN_OSI_FORTMP
#include <coin/OsiFmpSolverInterface.hpp>
#endif
#ifdef COIN_OSI_GLPK
#include <coin/OsiGlpkSolverInterface.hpp>
#endif
#ifdef COIN_OSI_MOSEK
#include <coin/OsiMskSolverInterface.hpp>
#endif
#ifdef COIN_OSI_OSL
#include <coin/OsiOslSolverInterface.hpp>
#endif
#ifdef COIN_OSI_SOPLEX
#include <coin/OsiSpxSolverInterface.hpp>
#endif
#ifdef COIN_OSI_SYM
#include <coin/OsiSymSolverInterface.hpp>
#endif
#ifdef COIN_OSI_VOL
#include <coin/OsiVolSolverInterface.hpp>
#endif
#ifdef COIN_OSI_XPRESS
#include <coin/OsiXprSolverInterface.hpp>
#endif
#ifdef COIN_OSI_GRB
#include <coin/OsiGrbSolverInterface.hpp>
#endif
#ifdef COIN_OSI_CSDP
#include <coin/OsiCsdpSolverInterface.hpp>
#endif

#include <coin/OsiSolverInterface.hpp>
