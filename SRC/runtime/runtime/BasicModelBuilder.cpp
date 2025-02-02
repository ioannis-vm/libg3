/* ****************************************************************** **
**    OpenSees - Open System for Earthquake Engineering Simulation    **
**          Pacific Earthquake Engineering Research Center            **
** ****************************************************************** */
//
// Description: This file contains the class definition for BasicModelBuilder.
// A BasicModelBuilder adds the commands to create the model for the standard
// models that can be generated using the elements released with the g3
// framework.
//
// TODO:
// - Remove all *Map.at
//   - return from registry
//   - handle find failures consistently
//
// Written: cmp
//
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <g3_api.h>
#include <modeling/commands.h>

#include <Matrix.h>
#include <Vector.h>
#include <ID.h>

#include <Domain.h>

#include <CrdTransf.h>
#include <SectionForceDeformation.h>
#include <SectionRepres.h>
#include <UniaxialMaterial.h>
#include <NDMaterial.h>
#include <runtime/BasicModelBuilder.h>
#include <MultiSupportPattern.h>

#include <TimeSeries.h>

#include "Storage/G3_Table.h"


//
// CLASS CONSTRUCTOR & DESTRUCTOR
//
BasicModelBuilder::BasicModelBuilder(Domain &theDomain, Tcl_Interp *interp, int NDM,
                               int NDF)
    : TclBuilder(theDomain, NDM, NDF), theInterp(interp)
{
  static int ncmd = sizeof(tcl_char_cmds)/sizeof(char_cmd);

  for (int i = 0; i < ncmd; i++)
    Tcl_CreateCommand(interp, 
        tcl_char_cmds[i].name, 
        tcl_char_cmds[i].func, 
        (ClientData) this, nullptr);
 
  theTclBuilder = this;
  theTclDomain = &theDomain;
  tclEnclosingPattern = nullptr;
  // theTclMultiSupportPattern = 0;

  nodeLoadTag = 0;
  eleArgStart = 0;
  registry  = G3_NewTable();


  Tcl_SetAssocData(interp, "OPS::theTclBuilder", NULL, (ClientData)this);
  Tcl_SetAssocData(interp, "OPS::theBasicModelBuilder", NULL, (ClientData)this);
  Tcl_SetAssocData(interp, "OPS::theTclDomain", NULL, (ClientData)&theDomain);
//m_runtime = G3_getRuntime(interp);
//G3_setDomain(m_runtime, &theDomain);
}

BasicModelBuilder::~BasicModelBuilder()
{

  // OPS_clearAllTimeSeries();
  // OPS_clearAllUniaxialMaterial();
  // OPS_clearAllNDMaterial();
  // OPS_clearAllSectionForceDeformation();
  // OPS_clearAllCrdTransf();

  // OPS_clearAllHystereticBackbone();
  // OPS_clearAllFrictionModel();
  // OPS_clearAllLimitCurve();
  // OPS_clearAllDamageModel();

/*
  theSections->clearAll();
  theSectionRepresents->clearAll();
  delete theSections;
  delete theSectionRepresents;
*/

  // set the pointers to 0
  theTclDomain = nullptr;
  theTclBuilder = nullptr;
  tclEnclosingPattern = nullptr;
  
  // TODO
  // G3_DelTable(registry);

  // theTclMultiSupportPattern = 0;

  // may possibly invoke Tcl_DeleteCommand() later
  // Tcl_DeleteCommand(theInterp, "node");
  // Tcl_DeleteCommand(theInterp, "element");
  // Tcl_DeleteCommand(theInterp, "uniaxialMaterial");
  // Tcl_DeleteCommand(theInterp, "nDMaterial");
  // Tcl_DeleteCommand(theInterp, "section");
  // Tcl_DeleteCommand(theInterp, "pattern");
  // Tcl_DeleteCommand(theInterp, "timeSeries");
  // Tcl_DeleteCommand(theInterp, "load");
}

//
// CLASS METHODS
//
void
BasicModelBuilder::letClobber(bool let_clobber) {no_clobber = !let_clobber;};

bool
BasicModelBuilder::canClobber() {return !no_clobber;};

int BasicModelBuilder::incrNodalLoadTag(void){return ++nodeLoadTag;};
int BasicModelBuilder::decrNodalLoadTag(void){return --nodeLoadTag;};
int BasicModelBuilder::getNodalLoadTag(void) {return   nodeLoadTag;};

int
BasicModelBuilder::addSP_Constraint(int axisDirn, double axisValue, const ID &fixityCodes, double tol)
{
  return theTclDomain->addSP_Constraint(axisDirn, axisValue, fixityCodes, tol);
}

LoadPattern *
BasicModelBuilder::getEnclosingPattern(void) {return tclEnclosingPattern;};

int
BasicModelBuilder::setEnclosingPattern(LoadPattern* pat){
  tclEnclosingPattern = pat;
  return 1;
};

Domain *
BasicModelBuilder::getDomain(void) const {return theTclDomain;}

BasicModelBuilder *
BasicModelBuilder::getBuilder(void) const {return theTclBuilder;}

G3_TableIterator
BasicModelBuilder::iterate(const char* partition)
{
  return G3_IteratePartition(registry, partition);
}

void* 
BasicModelBuilder::getRegistryObject(const char* partition, int tag)
{
  return G3_GetTableEntry(registry, partition, tag);
}

int
BasicModelBuilder::addRegistryObject(const char* partition, int tag, void *obj)
{
  G3_AddTableEntry(registry, partition, tag, obj);
  return 1;
}

TimeSeries *
BasicModelBuilder::getTimeSeries(const std::string &name)
{
  auto iter = m_TimeSeriesMap.find(name);
//opserr << "Looking for " << name.c_str() << "\n";
  if (iter != m_TimeSeriesMap.end()) {
//  opserr << "Found.\n";
    return iter->second->getCopy();
  } else
    return nullptr;
}

TimeSeries *
BasicModelBuilder::getTimeSeries(int tag)
{
  const std::string &name = std::to_string(tag);
  return this->getTimeSeries(name);
}

int
BasicModelBuilder::addTimeSeries(const std::string &name, TimeSeries *series)
{
  m_TimeSeriesMap[name] = series;
  int tag = std::stoi(name);
  // opserr << "Adding series " << name.c_str() << "(" << tag << ")" << "\n";
  G3_AddTableEntry(registry, "TimeSeries", tag, (void*)series);
  return 1;
}

int
BasicModelBuilder::addTimeSeries(TimeSeries *series)
{
  const std::string &name = std::to_string(series->getTag());
  m_TimeSeriesMap[name] = series;
  return 1;
}

//
// SectionForceDeformation Operations
//

// Retrieve a SectionForceDeformation instance from the model runtime
SectionForceDeformation*
BasicModelBuilder::getSection(const std::string &name)
{
  // SectionForceDeformation *instance = m_SectionForceDeformationMap.at(name);
  auto iter = m_SectionForceDeformationMap.find(name);
  if (iter != m_SectionForceDeformationMap.end()) {
    return iter->second->getCopy();
  } else {
    return nullptr;
  }
}

SectionForceDeformation*
BasicModelBuilder::getSection(int tag)
{
  const std::string &name = std::to_string(tag);
  return this->getSection(name);
}

// Add a new SectionForceDeformation to the model runtime
int
BasicModelBuilder::addSection(const std::string &name, SectionForceDeformation &instance)
{
  m_SectionForceDeformationMap[name] = &instance;
  G3_AddTableEntry(registry, "CrossSection", std::stoi(name), (void*)&instance);
  return 1;
}

// Add a new SectionForceDeformation to the model runtime
int
BasicModelBuilder::addSection(SectionForceDeformation &instance)
{
  const std::string &name = std::to_string(instance.getTag());
  return addSection(name, instance);
}

//
// SectionRepres Operations
//

// Retrieve a SectionRepres instance from the model runtime
SectionRepres*
BasicModelBuilder::getSectionRepres(const std::string &name)
{
  // SectionRepres *instance = m_SectionRepresMap.at(name);
  auto iter = m_SectionRepresMap.find(name);
  if (iter != m_SectionRepresMap.end()) {
    return iter->second ;
  } else {
    return nullptr;
  }
}

SectionRepres*
BasicModelBuilder::getSectionRepres(int tag)
{
  const std::string &name = std::to_string(tag);
  return this->getSectionRepres(name);
}

// Add a new SectionRepres to the model runtime
int
BasicModelBuilder::addSectionRepres(const std::string &name, SectionRepres &instance)
{
  m_SectionRepresMap[name] = &instance;
  return 1;
}

// Add a new SectionRepres to the model runtime
int
BasicModelBuilder::addSectionRepres(SectionRepres &instance)
{
  const std::string &name = std::to_string(instance.getTag());
  m_SectionRepresMap[name] = &instance;
  return 1;
}

//
// NDMaterial Operations
//

// Retrieve a NDMaterial instance from the model runtime
NDMaterial*
BasicModelBuilder::getNDMaterial(const std::string &name)
{
  // NDMaterial *instance = m_NDMaterialMap.at(name);
  auto iter = m_NDMaterialMap.find(name);
  if (iter != m_NDMaterialMap.end()) {
    return iter->second;
  } else {
    return nullptr;
  }
}

NDMaterial*
BasicModelBuilder::getNDMaterial(int tag)
{
  const std::string &name = std::to_string(tag);
  return this->getNDMaterial(name);
}

// Add a new NDMaterial to the model runtime
int
BasicModelBuilder::addNDMaterial(const std::string &name, NDMaterial &instance)
{
  m_NDMaterialMap[name] = &instance;
  G3_AddTableEntry(registry, "NDMaterial", std::stoi(name), (void*)&instance);
  return TCL_OK;
}

// Add a new NDMaterial to the model runtime
int
BasicModelBuilder::addNDMaterial(NDMaterial &instance)
{
  const std::string &name = std::to_string(instance.getTag());
  return this->addNDMaterial(name, instance);
}

//
// UniaxialMaterial Operations
//

// Retrieve a UniaxialMaterial instance from the model runtime
UniaxialMaterial*
BasicModelBuilder::getUniaxialMaterial(const std::string &name)
{
  return (UniaxialMaterial*)getRegistryObject("UniaxialMaterial", std::stoi(name));
// // UniaxialMaterial *instance = m_UniaxialMaterialMap.at(name);
// auto iter = m_UniaxialMaterialMap.find(name);
// if (iter != m_UniaxialMaterialMap.end()) {
//   return iter->second->getCopy();
// } else {
//   return nullptr;
// }
}

UniaxialMaterial*
BasicModelBuilder::getUniaxialMaterial(int tag)
{
  const std::string &name = std::to_string(tag);
  return this->getUniaxialMaterial(name);
}

int
BasicModelBuilder::addUniaxialMaterial(UniaxialMaterial *mat)
{
  assert(mat != nullptr);
  return this->addUniaxialMaterial(*mat);
}

// Add a new UniaxialMaterial to the model runtime
int
BasicModelBuilder::addUniaxialMaterial(UniaxialMaterial &instance)
{
  const std::string &name = std::to_string(instance.getTag());
  return this->addUniaxialMaterial(name, instance);
}

// Add a new UniaxialMaterial to the model runtime
int
BasicModelBuilder::addUniaxialMaterial(const std::string &name, UniaxialMaterial &instance)
{
  if (!canClobber() && (m_UniaxialMaterialMap.find(name) != m_UniaxialMaterialMap.end())) {
    opserr // << G3_ERROR_PROMPT 
           << "Cannot add new material with tag " << name << " as one already exists.\n";
    return -1;
  }

  // m_UniaxialMaterialMap[name] = &instance;
  G3_AddTableEntry(registry, "UniaxialMaterial", std::stoi(name), (void*)&instance);
  return TCL_OK;
}


HystereticBackbone*
BasicModelBuilder::getHystereticBackbone(const std::string &name)
{
  // HystereticBackbone *instance = m_HystereticBackboneMap.at(name);
  auto iter = m_HystereticBackboneMap.find(name);
  if (iter != m_HystereticBackboneMap.end()) {
    return iter->second;
  } else {
    return nullptr;
  }
}

// Add a new HystereticBackbone to the model runtime
int
BasicModelBuilder::addHystereticBackbone(const std::string &name, HystereticBackbone &instance)
{
  m_HystereticBackboneMap[name] = &instance;
  G3_AddTableEntry(registry, "HystereticBackbone", std::stoi(name), (void*)&instance);
  return 1;
}

//
// CrdTransf Operations
//

// Retrieve a CrdTransf instance from the model
// runtime
CrdTransf*
BasicModelBuilder::getCrdTransf(const std::string &name)
{
  // CrdTransf *instance = m_CrdTransfMap.at(name);
  auto iter = m_CrdTransfMap.find(name);
  if (iter != m_CrdTransfMap.end()) {
    return iter->second;
  } else {
    return nullptr;
  }
}

CrdTransf*
BasicModelBuilder::getCrdTransf(int tag)
{
  const std::string &name = std::to_string(tag);
  return this->getCrdTransf(name);
}

// Add a new CrdTransf to the model runtime
int
BasicModelBuilder::addCrdTransf(const std::string name, CrdTransf *instance)
{
  // m_CrdTransfMap[name] = instance;
  m_CrdTransfMap.insert({name, instance});
  G3_AddTableEntry(registry, "CoordinateTransform", std::stoi(name), (void*)instance);
  return 1;
}

// Add a new CrdTransf to the model runtime
int
BasicModelBuilder::addCrdTransf(CrdTransf *instance)
{
  const key_t name = std::to_string(instance->getTag());
  return this->addCrdTransf(name, instance);
}


#if 0
int
BasicModelBuilder_addRemoHFiber(ClientData clientData, Tcl_Interp *interp,
int argc, TCL_Char **argv)
{
  return TclCommand_addHFiber(clientData, interp, argc,argv,theTclBuilder);
}
#endif

