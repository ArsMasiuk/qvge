/* $Id: ClpPdcoBase.cpp 1665 2011-01-04 17:55:54Z lou $ */
// Copyright (C) 2003, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifdef COIN_DO_PDCO

#include "CoinPragma.hpp"

#include <iostream>

#include "ClpPdcoBase.hpp"
#include "ClpPdco.hpp"

//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor
//-------------------------------------------------------------------
ClpPdcoBase::ClpPdcoBase () :
     d1_(0.0),
     d2_(0.0),
     type_(-1)
{

}

//-------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------
ClpPdcoBase::ClpPdcoBase (const ClpPdcoBase & source) :
     d1_(source.d1_),
     d2_(source.d2_),
     type_(source.type_)
{

}

//-------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------
ClpPdcoBase::~ClpPdcoBase ()
{

}

//----------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------
ClpPdcoBase &
ClpPdcoBase::operator=(const ClpPdcoBase& rhs)
{
     if (this != &rhs) {
          d1_ = rhs.d1_;
          d2_ = rhs.d2_;
          type_ = rhs.type_;
     }
     return *this;
}
#endif
