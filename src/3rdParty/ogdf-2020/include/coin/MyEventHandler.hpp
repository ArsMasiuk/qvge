/* $Id: MyEventHandler.hpp 1665 2011-01-04 17:55:54Z lou $ */
// Copyright (C) 2004, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef MyEventHandler_H
#define MyEventHandler_H

#include "ClpEventHandler.hpp"

/** This is so user can trap events and do useful stuff.
    This is used in Clp/Test/unitTest.cpp

    ClpSimplex model_ is available as well as anything else you care
    to pass in
*/

class MyEventHandler : public ClpEventHandler {

public:
     /**@name Overrides */
     //@{
     virtual int event(Event whichEvent) override;
     //@}

     /**@name Constructors, destructor etc*/
     //@{
     /** Default constructor. */
     MyEventHandler();
     /// Constructor with pointer to model (redundant as setEventHandler does)
     MyEventHandler(ClpSimplex * model);
     /** Destructor */
     virtual ~MyEventHandler();
     /** The copy constructor. */
     MyEventHandler(const MyEventHandler & rhs);
     /// Assignment
     MyEventHandler& operator=(const MyEventHandler & rhs);
     /// Clone
     virtual ClpEventHandler * clone() const override ;
     //@}


protected:
     // data goes here
};

#endif
