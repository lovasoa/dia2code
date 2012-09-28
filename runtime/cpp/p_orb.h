// p_orb.h -- Shield against CORBA system dependencies of C++ ORBs
//
// Based on the paper "Portability of C++ CORBA Applications"
// by Ciaran McHale:
// http://www.iona.com/devcenter/corba/portability_cpp_corba.pdf
//
// The name of the ORB system CORBA include file (the file where
// basic CORBA types such as CORBA::Boolean etc. are defined)
// is not standardized, and each ORB names this file differently.
// For portability, just #include "p_orb.h".
//
// Currently following ORBs are supported:
//
// * ORBIX (http://www.iona.com/)
//   In order to use this configuration, define a C preprocessor symbol
//   named P_USE_ORBIX.
//
// * ORBacus (http://www.orbacus.com/)
//   In order to use this configuration, define a C preprocessor symbol
//   named P_USE_ORBACUS.
//
// * VisiBroker (http://www.borland.com/)
//   In order to use this configuration, define a C preprocessor symbol
//   named P_USE_VISIBROKER.
//
// * omniORB4 (http://omniorb.sourceforge.net/)
//   In order to use this configuration, define a C preprocessor symbol
//   named P_USE_OMNIORB.
//
// * MICO (http://www.mico.org/)
//   In order to use this configuration, define a C preprocessor symbol
//   named P_USE_MICO.
//
// * TAO (http://www.cs.wustl.edu/~schmidt/TAO.html)
//   In order to use this configuration, define a C preprocessor symbol
//   named P_USE_TAO.
//
// * ORBit-C++ (http://orbitcpp.sourceforge.net/)
//   In order to use this configuration, define a C preprocessor
//   symbol named P_USE_ORBITCPP.
//
// [...] Add others here [...]
//
// * No CORBA ORB
//   In order to use this configuration, define a C preprocessor
//   symbol named P_USE_NONE.
//   This configuration only supports the primitive IDL data types
//   excluding sequence, bounded string, and any.
//   It does not support any ORB operations.
//

#ifndef P_ORB_H_
#define P_ORB_H_

#if defined (P_USE_ORBIX)

#include <omg/orb.hh>

#elif defined (P_USE_ORBACUS)

#include <OB/CORBA.h>

#elif defined (P_USE_VISIBROKER)

#include <corba.h>

#elif defined (P_USE_OMNIORB)

# include <omniORB4/CORBA.h>

#elif defined (P_USE_MICO)

# include <CORBA.h>

#elif defined (P_USE_TAO)

# include <tao/corbafwd.h>
namespace CORBA {
   typedef char* String;  // For some reason this is missing in corbafwd.
};

#elif defined (P_USE_ORBITCPP)

# include <orb/orbitcpp.hh>


/**********  Add other ORB configurations here.  **************/


#elif defined (P_USE_NONE)

// If P_USE_NONE is defined then we use a minimal CORBA spec.

namespace CORBA {

    typedef bool                Boolean;

    typedef unsigned char       Octet;

    typedef char                Char;

    typedef short               Short;

    typedef unsigned short      UShort;

    typedef int                 Long;

    typedef unsigned int        ULong;

    typedef long long           LongLong;

    typedef unsigned long long  ULongLong;

    typedef float               Float;

    typedef double              Double;

    typedef char *              String;

};

#else

# error "ORB system not defined"

#endif

#endif  // P_ORB_H_

