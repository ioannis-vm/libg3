/* ****************************************************************** **
**    OpenSees - Open System for Earthquake Engineering Simulation    **
**          Pacific Earthquake Engineering Research Center            **
**                                                                    **
**                                                                    **
** (C) Copyright 1999, The Regents of the University of California    **
** All Rights Reserved.                                               **
**                                                                    **
** Commercial use of this program without express permission of the   **
** University of California, Berkeley, is strictly prohibited.  See   **
** file 'COPYRIGHT'  in main directory for information on usage and   **
** redistribution,  and for a DISCLAIMER OF ALL WARRANTIES.           **
**                                                                    **
** Developed by:                                                      **
**   Frank McKenna (fmckenna@ce.berkeley.edu)                         **
**   Gregory L. Fenves (fenves@ce.berkeley.edu)                       **
**   Filip C. Filippou (filippou@ce.berkeley.edu)                     **
**                                                                    **
** ****************************************************************** */
//
// Purpose: This file contains the implementation of class SP_Constraint.
//
// Written: fmk 
// Created: 11/96
// Revision: A
//
#include <string.h>
#include <SP_Constraint.h>
#include <classTags.h>
#include <Vector.h>
#include <Channel.h>
#include <FEM_ObjectBroker.h>
#include <string>
#include <Domain.h>
#include <Node.h>
#include <ID.h>

static int numSPs = 0;
static int nextTag = 0;

// 2 little procedures needed for parallel processing all due to fact that SP's need 
// to keep unique tags among processes in parallel

int SP_Constraint_SetNextTag(int next) {
  nextTag = next;
  return nextTag;
}

int SP_Constraint_GetNextTag(void) {
  return nextTag;
}

// constructor for FEM_ObjectBroker
SP_Constraint::SP_Constraint(int clasTag)
:DomainComponent(0,clasTag),
 nodeTag(0), dofNumber(0), valueR(0.0), valueC(0.0), isConstant(true), 
 loadPatternTag(-1)
{
  numSPs++;
}

// constructor for a subclass to use
SP_Constraint::SP_Constraint(int node, int ndof, int clasTag)
:DomainComponent(nextTag++, clasTag),
 nodeTag(node), dofNumber(ndof), valueR(0.0), valueC(0.0), isConstant(true), 
 loadPatternTag(-1)
 // valueC is set to 1.0 so that homo will be false when recvSelf() invoked
 // should be ok as valueC cannot be used by subclasses and subclasses should
 // not be used if it is a homogeneous constraint.
{
  numSPs++;
}

// constructor for object of type SP_Constraint
SP_Constraint::SP_Constraint(int node, int ndof, double value, bool ISconstant)
:DomainComponent(nextTag++, CNSTRNT_TAG_SP_Constraint),
 nodeTag(node), dofNumber(ndof), valueR(value), valueC(value), isConstant(ISconstant),
 loadPatternTag(-1)
{
  numSPs++;
}

SP_Constraint::~SP_Constraint()
{
  numSPs--;
  if (numSPs == 0)
    nextTag = 0;
}

int
SP_Constraint::getNodeTag(void) const
{
    // return id of constrained node
    return nodeTag;
}

int
SP_Constraint::getDOF_Number(void) const
{
    //  return the number of the constrained DOF    
    return dofNumber;
}


double
SP_Constraint::getValue(void)
{
    // return the value of the constraint
    return valueC;
}

int
SP_Constraint::applyConstraint(double loadFactor)
{
    // as SP_Constraint objects are time invariant nothing is done
    if (isConstant == false)
	valueC = loadFactor*valueR;

    return 0;
}


bool
SP_Constraint::isHomogeneous(void) const
{
    if (valueR == 0.0)
	return true;
    else
	return false;
}

void
SP_Constraint::setLoadPatternTag(int tag)
{
  loadPatternTag = tag;
}

int
SP_Constraint::getLoadPatternTag(void) const
{
  return loadPatternTag;
}

int 
SP_Constraint::sendSelf(int cTag, Channel &theChannel)
{
    static Vector data(8);  // we send as double to avoid having 
                     // to send two messages.
    data(0) = this->getTag(); 
    data(1) = nodeTag;
    data(2) = dofNumber;
    data(3) = valueC;
    if (isConstant == true)
	data(4) = 1.0;
    else
	data(4) = 0.0;
    data(5) = valueR;
    data(6) = this->getLoadPatternTag();

    data(7) = nextTag;

    int result = theChannel.sendVector(this->getDbTag(), cTag, data);
    if (result != 0) {
      opserr << "WARNING SP_Constraint::sendSelf - error sending Vector data\n";
      return result;
    }

    return 0;
}

int 
SP_Constraint::recvSelf(int cTag, Channel &theChannel, 
			FEM_ObjectBroker &theBroker)
{
    static Vector data(8);  // we sent the data as double to avoid having to send
                     // two messages
    int result = theChannel.recvVector(this->getDbTag(), cTag, data);
    if (result < 0) {
	opserr << "WARNING SP_Constraint::recvSelf - error receiving Vector data\n";
	return result;
    }
    
    // if o.k. set the data
    this->setTag((int)data(0));
    nodeTag = (int)data(1);
    dofNumber = (int)data(2);
    valueC = data(3);

    if (data(4) == 1.0)
	isConstant = true;
    else
	isConstant = false;
    valueR = data(5);
    valueC = valueR;
    this->setLoadPatternTag((int)data(6));

    nextTag = (int)data(7);

    return 0;
}


void
SP_Constraint::Print(OPS_Stream &s, int flag) 
{
    s << "SP_Constraint: " << this->getTag();
    s << "\t Node: " << nodeTag << " DOF: " << dofNumber+1;
    s << " ref value: " << valueR << " current value: " << valueC << endln;
}








