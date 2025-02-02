//
// Written: fmk
// Created: 07/99
//
// Description: This file contains the class definition for TclBasicBuilder.
// A TclBasicBuilder adds the commands to create the model for the standard
// models that can be generated using the elements released with the g3
// framework.
//
#include <stdlib.h>
#include <string.h>

#include <Matrix.h>
#include <Vector.h>
#include <ID.h>
#include <ArrayOfTaggedObjects.h>

#include <Domain.h>
#include <Node.h>
#include <NodeIter.h>
#include <SP_Constraint.h>
#include <SP_ConstraintIter.h>
#include <MP_Constraint.h>

// #include <CrdTransf.h>

// #include <NodalLoad.h>
#include <Beam2dPointLoad.h>
#include <Beam2dUniformLoad.h>
#include <Beam2dPartialUniformLoad.h>
#include <Beam2dTempLoad.h>
#include <Beam2dThermalAction.h>  //L.Jiang [SIF]
#include <Beam3dThermalAction.h>  //L.Jiang [SIF]
#include <ShellThermalAction.h>   //L.Jiang [SIF]
#include <ThermalActionWrapper.h> //L.Jiang [SIF]
#include <NodalThermalAction.h>   //L.Jiang [SIF]

#include <Beam3dPointLoad.h>
#include <Beam3dUniformLoad.h>
#include <Beam3dPartialUniformLoad.h>
#include <BrickSelfWeight.h>
#include <SurfaceLoader.h>
#include <SelfWeight.h>
#include <LoadPattern.h>

#include <SectionForceDeformation.h>
#include <SectionRepres.h>

#include <NDMaterial.h>
#include <TclBasicBuilder.h>
#include <ImposedMotionSP.h>
#include <ImposedMotionSP1.h>
#include <MultiSupportPattern.h>

#include <TimeSeries.h>
#include <PathTimeSeriesThermal.h>                   //L.Jiang [SIF]

// Added by Scott J. Brandenberg (sjbrandenberg@ucdavis.edu)
#include <PySimple1Gen.h>
#include <TzSimple1Gen.h>
// End added by SJB

// Added by Prishati Raychowdhury  (PRC)
#include <ShallowFoundationGen.h>
// end PRC

#include <YieldSurface_BC.h>
#include <YS_Evolution.h>
#include <PlasticHardeningMaterial.h>

#ifdef OPSDEF_DAMAGE
#  include <DamageModel.h> //!!
#endif

#include <FrictionModel.h>

#include <StiffnessDegradation.h>
#include <UnloadingRule.h>
#include <StrengthDegradation.h>
// #include <HystereticBackbone.h>

#include <Element.h>

extern int OPS_ResetInput(ClientData clientData, Tcl_Interp *interp, int cArg,
                          int mArg, TCL_Char ** const argv, Domain *domain,
                          TclBuilder *builder);
#include <packages.h>

//
// SOME STATIC POINTERS USED IN THE FUNCTIONS INVOKED BY THE INTERPRETER
//
static Domain *theTclDomain = nullptr;
static TclBasicBuilder *theTclBuilder = nullptr;
extern LoadPattern *theTclLoadPattern;
// static int nodeLoadTag = 0;

//
// THE PROTOTYPES OF THE FUNCTIONS INVOKED BY THE INTERPRETER
//
// REMO
extern Tcl_CmdProc TclCommand_addPatch;
extern Tcl_CmdProc TclCommand_addFiber;
extern Tcl_CmdProc TclCommand_addReinfLayer;
static Tcl_CmdProc TclCommand_addParameter;


#if 0 // mesh commands
static Tcl_CmdProc TclCommand_mesh;
static Tcl_CmdProc TclCommand_remesh;
#endif


#ifdef OPSDEF_DAMAGE
int
TclCommand_addDamageModel(ClientData, Tcl_Interp*, int argc,
                          TCL_Char ** const);
#endif // OPSDEF_DAMAGE

static Tcl_CmdProc TclCommand_addEqualDOF_MP;
static Tcl_CmdProc TclCommand_addEqualDOF_MP_Mixed;
static Tcl_CmdProc TclCommand_addImposedMotionSP;

// Added by Scott J. Brandenberg
static Tcl_CmdProc TclCommand_doPySimple1Gen;
static Tcl_CmdProc TclCommand_doTzSimple1Gen;

static Tcl_CmdProc TclBasicBuilder_doShallowFoundationGen;
static Tcl_CmdProc TclBasicBuilder_addRemoHFiber;

static Tcl_CmdProc TclCommand_addStiffnessDegradation;
static Tcl_CmdProc TclCommand_addUnloadingRule;
static Tcl_CmdProc TclCommand_addStrengthDegradation;
extern Tcl_CmdProc TclCommand_addGroundMotion;
// static int TclCommand_UpdateMaterialStage(ClientData, Tcl_Interp*, int argc, TCL_Char ** const);
extern Tcl_CmdProc TclBasicBuilderUpdateParameterCommand;
static Tcl_CmdProc TclCommand_Package;
extern Tcl_CmdProc TclCommand_GenerateInterfacePoints;


extern int TclCommand_addHFiber(ClientData, Tcl_Interp*,
                                int argc, TCL_Char **const,
                                TclBasicBuilder *theTclBuilder);



//
//
// CLASS CONSTRUCTOR & DESTRUCTOR

// constructor: the constructor will add certain commands to the interpreter
TclBasicBuilder::TclBasicBuilder(Domain &theDomain, Tcl_Interp *interp, int NDM,
                                 int NDF)
    : TclBuilder(theDomain, NDM, NDF), theInterp(interp)
{
  theSections = new ArrayOfTaggedObjects(32);
  theSectionRepresents = new ArrayOfTaggedObjects(32);

  // call Tcl_CreateCommand for class specific commands
  Tcl_CreateCommand(interp, "parameter", TclCommand_addParameter, NULL, NULL);
  Tcl_CreateCommand(interp, "addToParameter", TclCommand_addParameter, NULL, NULL);
  Tcl_CreateCommand(interp, "updateParameter", TclCommand_addParameter, NULL, NULL);

#if 0
  Tcl_CreateCommand(interp, "mesh", TclCommand_mesh, NULL, NULL);
  Tcl_CreateCommand(interp, "remesh", TclCommand_remesh, NULL, NULL);
  Tcl_CreateCommand(interp, "load", TclCommand_addNodalLoad, NULL, NULL);
#endif


  Tcl_CreateCommand(interp, "imposedMotion", TclCommand_addImposedMotionSP, NULL, NULL);

  Tcl_CreateCommand(interp, "imposedSupportMotion",
                    TclCommand_addImposedMotionSP, NULL, NULL);

  Tcl_CreateCommand(interp, "groundMotion", TclCommand_addGroundMotion, NULL, NULL);

  Tcl_CreateCommand(interp, "equalDOF",       TclCommand_addEqualDOF_MP, NULL, NULL);
  Tcl_CreateCommand(interp, "equalDOF_Mixed", TclCommand_addEqualDOF_MP_Mixed, NULL, NULL);


  Tcl_CreateCommand(interp, "PySimple1Gen", TclCommand_doPySimple1Gen, NULL, NULL);
  Tcl_CreateCommand(interp, "TzSimple1Gen", TclCommand_doTzSimple1Gen, NULL, NULL);

  Tcl_CreateCommand(interp, "ShallowFoundationGen",
                    TclBasicBuilder_doShallowFoundationGen, NULL, NULL);

  // Added by LEO
  Tcl_CreateCommand(interp, "Hfiber",               TclBasicBuilder_addRemoHFiber, NULL, NULL);

#if 0
  Tcl_CreateCommand(interp, "frictionModel",        TclCommand_addFrictionModel, NULL, NULL);
  Tcl_CreateCommand(interp, "yieldSurface_BC", TclCommand_addYieldSurface_BC, NULL, NULL);
  Tcl_CreateCommand(interp, "ysEvolutionModel", TclCommand_addYS_EvolutionModel, NULL, NULL);
  Tcl_CreateCommand(interp, "plasticMaterial", TclCommand_addYS_PlasticMaterial, NULL, NULL);
  Tcl_CreateCommand(interp, "cyclicModel", TclCommand_addCyclicModel, NULL, NULL);
#endif
  Tcl_CreateCommand(interp, "stiffnessDegradation", TclCommand_addStiffnessDegradation, NULL, NULL);
  Tcl_CreateCommand(interp, "unloadingRule",        TclCommand_addUnloadingRule, NULL, NULL);
  Tcl_CreateCommand(interp, "strengthDegradation",  TclCommand_addStrengthDegradation, NULL, NULL);
  // Tcl_CreateCommand(interp, "updateMaterialStage",  TclCommand_UpdateMaterialStage, NULL, NULL);
  // Tcl_CreateCommand(interp, "updateMaterials",      TclCommand_UpdateMaterials, NULL, NULL);
  Tcl_CreateCommand(interp, "loadPackage",          TclCommand_Package, NULL, NULL);


  // set the static pointers in this file
  theTclBuilder = this;
  theTclDomain = &theDomain;
  theTclLoadPattern = 0;
  // theTclMultiSupportPattern = 0;

  Tcl_SetAssocData(interp, "OPS::theTclBuilder", NULL, (ClientData)this);
  Tcl_SetAssocData(interp, "OPS::theTclDomain", NULL, (ClientData)&theDomain);
}

TclBasicBuilder::~TclBasicBuilder()
{
  theSections->clearAll();
  theSectionRepresents->clearAll();
  delete theSections;
  delete theSectionRepresents;

  // set the pointers to 0
  theTclDomain = nullptr;
  theTclBuilder = nullptr;
  theTclLoadPattern = nullptr;
  // theTclMultiSupportPattern = 0;

  // may possibly invoke Tcl_DeleteCommand() later
  Tcl_DeleteCommand(theInterp, "parameter");
  Tcl_DeleteCommand(theInterp, "addToParameter");
  Tcl_DeleteCommand(theInterp, "updateParameter");
#if 0 // mesh commands
  Tcl_DeleteCommand(theInterp, "mesh");
  Tcl_DeleteCommand(theInterp, "remesh");
#endif
  Tcl_DeleteCommand(theInterp, "background");
  Tcl_DeleteCommand(theInterp, "uniaxialMaterial");
  Tcl_DeleteCommand(theInterp, "imposedSupportMotion");
  Tcl_DeleteCommand(theInterp, "groundMotion");
  Tcl_DeleteCommand(theInterp, "equalDOF");
  Tcl_DeleteCommand(theInterp, "PySimple1Gen"); // Added by Scott J. Brandenberg
  Tcl_DeleteCommand(theInterp, "TzSimple1Gen"); // Added by Scott J. Brandenberg

  Tcl_DeleteCommand(theInterp, "Hfiber"); // LEO
  Tcl_DeleteCommand(theInterp, "updateMaterialStage");
  Tcl_DeleteCommand(theInterp, "updateMaterials");

#if 0
  Tcl_DeleteCommand(theInterp, "frictionModel");
#endif
  Tcl_DeleteCommand(theInterp, "unloadingRule");
  Tcl_DeleteCommand(theInterp, "stiffnessDegradation");
  Tcl_DeleteCommand(theInterp, "strengthDegradation");
  Tcl_DeleteCommand(theInterp, "hystereticBackbone");

  Tcl_DeleteCommand(theInterp, "damageModel");

  Tcl_DeleteCommand(theInterp, "loadPackage");
  Tcl_DeleteCommand(
      theInterp,
      "generateInterfacePoints"); // Added by Alborz Ghofrani - U.Washington
}

//
// CLASS METHODS
//

int
TclBasicBuilder::addSection(SectionForceDeformation &theSection)
{
  //  bool result = theSections->addComponent(&theSection);
  bool result = OPS_addSectionForceDeformation(&theSection);
  if (result == true)
    return 0;
  else {
    opserr << "TclBasicBuilder::addSection() - failed to add section: "
           << theSection;
    return -1;
  }
}

#undef OPS_getSectionForceDeformation
extern SectionForceDeformation* OPS_getSectionForceDeformation(int);
SectionForceDeformation *
TclBasicBuilder::getSection(int tag)
{
  return OPS_getSectionForceDeformation(tag);
}


int
TclBasicBuilder::addSectionRepres(SectionRepres &theSectionRepres)
{
  bool result = theSectionRepresents->addComponent(&theSectionRepres);

  if (result == true)
    return 0;

  else {
    opserr << "TclBasicBuilder::addSectionRepres() - failed to add "
              "SectionRepres\n";
    return -1;
  }
}

SectionRepres *
TclBasicBuilder::getSectionRepres(int tag)
{
  TaggedObject *mc = theSectionRepresents->getComponentPtr(tag);
  if (mc == 0)
    return 0;
  SectionRepres *result = (SectionRepres *)mc;
  return result;
}

//
// THE FUNCTIONS INVOKED BY THE INTERPRETER
//

/////////////////////////////   gnp adding element damping

// the function for creating ne material objects and patterns is in a seperate
// file. this allows new material and patternobjects to be added without
// touching this file. does so at the expense of an extra procedure call.

extern int TclBasicBuilderParameterCommand(ClientData clientData,
                                           Tcl_Interp *interp, int argc,
                                           TCL_Char ** const argv, Domain *theDomain,
                                           TclBasicBuilder *theTclBuilder);
int
TclCommand_addParameter(ClientData clientData, Tcl_Interp *interp, int argc,
                        TCL_Char ** const argv)

{
  return TclBasicBuilderParameterCommand(clientData, interp, argc, argv,
                                         theTclDomain, theTclBuilder);
}


// extern int Tcl_AddLimitCurveCommand(ClientData clienData, Tcl_Interp *interp,
//                                     int argc, TCL_Char ** const argv,
//                                     Domain *theDomain);
// 
// int
// TclCommand_addLimitCurve(ClientData clientData, Tcl_Interp *interp, int argc,
//                          TCL_Char ** const argv)
// {
//   return Tcl_AddLimitCurveCommand(clientData, interp, argc, argv, theTclDomain);
// }
//
extern int
TclBasicBuilderYieldSurface_BCCommand(ClientData clienData, Tcl_Interp *interp,
                                      int argc, TCL_Char ** const argv);

int
TclCommand_addYieldSurface_BC(ClientData clientData, Tcl_Interp *interp,
                              int argc, TCL_Char ** const argv)

{
  return TclBasicBuilderYieldSurface_BCCommand(clientData, interp, argc, argv);
}

extern int TclBasicBuilderYS_EvolutionModelCommand(
    ClientData clienData, Tcl_Interp *interp, int argc, TCL_Char ** const argv,
    TclBasicBuilder *theTclBuilder);

int
TclCommand_addYS_EvolutionModel(ClientData clientData, Tcl_Interp *interp,
                                int argc, TCL_Char ** const argv)

{
  return TclBasicBuilderYS_EvolutionModelCommand(clientData, interp, argc, argv,
                                                 theTclBuilder);
}

extern int
TclBasicBuilderPlasticMaterialCommand(ClientData clienData, Tcl_Interp *interp,
                                      int argc, TCL_Char ** const argv,
                                      TclBasicBuilder *theTclBuilder);

int
TclCommand_addYS_PlasticMaterial(ClientData clientData, Tcl_Interp *interp,
                                 int argc, TCL_Char ** const argv)

{
  return TclBasicBuilderPlasticMaterialCommand(clientData, interp, argc, argv,
                                               theTclBuilder);
}

//!!
extern int TclBasicBuilderCyclicModelCommand(ClientData clienData,
                                             Tcl_Interp *interp, int argc,
                                             TCL_Char ** const argv,
                                             TclBasicBuilder *theTclBuilder);
int
TclCommand_addCyclicModel(ClientData clientData, Tcl_Interp *interp, int argc,
                          TCL_Char ** const argv)

{
  return TclBasicBuilderCyclicModelCommand(clientData, interp, argc, argv,
                                           theTclBuilder);
}

extern int TclBasicBuilderDamageModelCommand(ClientData clienData,
                                             Tcl_Interp *interp, int argc,
                                             TCL_Char ** const argv);

#ifdef OPSDEF_DAMAGE
int
TclCommand_addDamageModel(ClientData clientData, Tcl_Interp *interp, int argc,
                          TCL_Char ** const argv)

{
  return TclBasicBuilderDamageModelCommand(clientData, interp, argc, argv);
}
#endif // OPSDEF_DAMAGE


#if 0
int
TclCommand_addNodalLoad(ClientData clientData, Tcl_Interp *interp, int argc,
                        TCL_Char ** const argv)
{
  // ensure the destructor has not been called -
  if (theTclBuilder == 0) {
    opserr << "WARNING builder has been destroyed - load \n";
    return TCL_ERROR;
  }

  int ndf = argc - 2;
  NodalLoad *theLoad = 0;

  bool isLoadConst = false;
  bool userSpecifiedPattern = false;
  int loadPatternTag = 0;
  // The above definition are moved forward for the use in both cases

  //-------------Adding Proc for NodalThermalAction, By Liming Jiang, [SIF] 2017
  if ((strcmp(argv[2], "-NodalThermal") == 0) ||
      (strcmp(argv[2], "-nodalThermal") == 0)) {

#if 0
    int nodeId;
    if (Tcl_GetInt(interp, argv[1], &nodeId) != TCL_OK) {
      opserr << "WARNING invalid nodeId: " << argv[1] << endln;
      return TCL_ERROR;
    }

    Vector *thecrds = new Vector();
    Node *theNode = theTclDomain->getNode(nodeId);
    if (theNode == 0) {
      opserr << "WARNING invalid nodeID: " << argv[1] << endln;
      return TCL_ERROR;
    }
    (*thecrds) = theNode->getCrds();

    int count = 3;
    if (strcmp(argv[count], "-source") == 0) {
      count++;
      const char *pwd = getInterpPWD(interp);
      simulationInfo.addInputFile(argv[count], pwd);
      TimeSeries *theSeries;

      int dataLen =
          9; // default num of temperature input for nodal ThermalAction;

      if (argc - count == 5) {
        // which indicates the nodal thermal action is applied to 3D I section
        // Beam;
        dataLen = 15;
        theSeries = new PathTimeSeriesThermal(nodeId, argv[count], dataLen);
        count++;
        double RcvLoc1, RcvLoc2, RcvLoc3, RcvLoc4;
        if (Tcl_GetDouble(interp, argv[count], &RcvLoc1) != TCL_OK) {
          opserr << "WARNING NodalLoad - invalid loc1  " << argv[count]
                 << " for NodalThermalAction\n";
          return TCL_ERROR;
        }
        if (Tcl_GetDouble(interp, argv[count + 1], &RcvLoc2) != TCL_OK) {
          opserr << "WARNING NodalLoad - invalid loc2  " << argv[count + 1]
                 << " for NodalThermalAction\n";
          return TCL_ERROR;
        }
        if (Tcl_GetDouble(interp, argv[count + 2], &RcvLoc3) != TCL_OK) {
          opserr << "WARNING NodalLoad - invalid loc3  " << argv[count + 2]
                 << " for NodalThermalAction\n";
          return TCL_ERROR;
        }
        if (Tcl_GetDouble(interp, argv[count + 3], &RcvLoc4) != TCL_OK) {
          opserr << "WARNING NodalLoad - invalid loc4  " << argv[count + 3]
                 << " for NodalThermalAction\n";
          return TCL_ERROR;
        }
        // end of recieving data;
        theLoad = new NodalThermalAction(nodeLoadTag, nodeId, RcvLoc1, RcvLoc2,
                                         RcvLoc3, RcvLoc4, theSeries, thecrds);
      }
      // end of for 15 data input;
      else if (argc - count == 3 || argc - count == 10) {

        theSeries = new PathTimeSeriesThermal(nodeId, argv[count]);
        count++;
        Vector locy;
        if (argc - count == 2) {
          double RcvLoc1, RcvLoc2;
          if (Tcl_GetDouble(interp, argv[count], &RcvLoc1) != TCL_OK) {
            opserr << "WARNING NodalLoad - invalid loc1  " << argv[count]
                   << " for NodalThermalAction\n";
            return TCL_ERROR;
          }
          if (Tcl_GetDouble(interp, argv[count + 1], &RcvLoc2) != TCL_OK) {
            opserr << "WARNING NodalLoad - invalid loc2  " << argv[count + 1]
                   << " for NodalThermalAction\n";
            return TCL_ERROR;
          }
          locy = Vector(9);
          locy(0) = RcvLoc1;
          locy(1) = (7 * RcvLoc1 + 1 * RcvLoc2) / 8;
          locy(2) = (6 * RcvLoc1 + 2 * RcvLoc2) / 8;
          locy(3) = (5 * RcvLoc1 + 3 * RcvLoc2) / 8;
          locy(4) = (4 * RcvLoc1 + 4 * RcvLoc2) / 8;
          locy(5) = (3 * RcvLoc1 + 5 * RcvLoc2) / 8;
          locy(6) = (2 * RcvLoc1 + 6 * RcvLoc2) / 8;
          locy(7) = (1 * RcvLoc1 + 7 * RcvLoc2) / 8;
          locy(8) = RcvLoc2;

        } // end of if only recieving one loc data;
        else if (argc - count == 9) {
          double indata[9];
          double BufferData;

          for (int i = 0; i < 9; i++) {
            if (Tcl_GetDouble(interp, argv[count], &BufferData) != TCL_OK) {
              opserr << "WARNING eleLoad - invalid data " << argv[count]
                     << " for -beamThermal 3D\n";
              return TCL_ERROR;
            }
            indata[i] = BufferData;
            count++;
          }
          locy = Vector(indata, 9);
          // temp1,loc1,temp2,loc2...temp9,loc9
        } // end of if only recieving 9 loc data;

        theLoad = new NodalThermalAction(nodeLoadTag, nodeId, locy, theSeries,
                                         thecrds);
        delete thecrds;
      }
      // end of recieving 9 temp data in external file;
      else {
        opserr << "WARNING NodalThermalAction - invalid dataLen\n";
      }
      // end of definition for different data input length(9 or15)

    }
    // end for detecting source
    else {
      if (argc - count == 4) {
        double t1, t2, locY1, locY2;
        if (Tcl_GetDouble(interp, argv[count], &t1) != TCL_OK) {
          opserr << "WARNING eleLoad - invalid T1 " << argv[count]
                 << " for NodalThermalAction\n";
          return TCL_ERROR;
        }
        if (Tcl_GetDouble(interp, argv[count + 1], &locY1) != TCL_OK) {
          opserr << "WARNING eleLoad - invalid LocY1 " << argv[count + 1]
                 << " for NodalThermalAction\n";
          return TCL_ERROR;
        }
        if (Tcl_GetDouble(interp, argv[count + 2], &t2) != TCL_OK) {
          opserr << "WARNING eleLoad - invalid T1 " << argv[count]
                 << " for NodalThermalAction\n";
          return TCL_ERROR;
        }
        if (Tcl_GetDouble(interp, argv[count + 3], &locY2) != TCL_OK) {
          opserr << "WARNING eleLoad - invalid LocY1 " << argv[count + 1]
                 << " for NodalThermalAction\n";
          return TCL_ERROR;
        }

        theLoad = new NodalThermalAction(nodeLoadTag, nodeId, t1, locY1, t2,
                                         locY2, thecrds);
      }
      // for defining a uniform gradient thermal action
    }
    // end for source or no source

    // get the current pattern tag if no tag given in i/p
    if (userSpecifiedPattern == false) {
      if (theTclLoadPattern == 0) {
        opserr << "WARNING no current load pattern - NodalThermalAction "
               << nodeId;
        return TCL_ERROR;
      } else
        loadPatternTag = theTclLoadPattern->getTag();
    }
#endif
  }
  // end of adding NodalThermalAction -------------end---------Liming,[SIF] 2017

  // start of else block, Liming [SIF]
  else {

    // make sure at least one other argument to contain type of system
    if (argc < (2 + ndf)) {
      opserr << "WARNING bad command - want: load nodeId " << ndf
             << " forces\n";
      return TCL_ERROR;
    }

    // get the id of the node
    int nodeId;
    if (Tcl_GetInt(interp, argv[1], &nodeId) != TCL_OK) {
      opserr << "WARNING invalid nodeId: " << argv[1];
      opserr << " - load nodeId " << ndf << " forces\n";
      return TCL_ERROR;
    }

    // get the load vector
    Vector forces(ndf);
    for (int i = 0; i < ndf; i++) {
      double theForce;
      if (Tcl_GetDouble(interp, argv[2 + i], &theForce) != TCL_OK) {
        opserr << "WARNING invalid force " << i + 1 << " - load " << nodeId;
        opserr << " " << ndf << " forces\n";
        return TCL_ERROR;
      } else
        forces(i) = theForce;
    }

    // allow some additional options at end of command
    int endMarker = 2 + ndf;
    while (endMarker != argc) {
      if (strcmp(argv[endMarker], "-const") == 0) {
        // allow user to specify const load
        isLoadConst = true;
      } else if (strcmp(argv[endMarker], "-pattern") == 0) {
        // allow user to specify load pattern other than current
        endMarker++;
        userSpecifiedPattern = true;
        if (endMarker == argc ||
            Tcl_GetInt(interp, argv[endMarker], &loadPatternTag) != TCL_OK) {

          opserr << "WARNING invalid patternTag - load " << nodeId << " ";
          opserr << ndf << " forces pattern patterntag\n";
          return TCL_ERROR;
        }
      }
      endMarker++;
    }

    // get the current pattern tag if no tag given in i/p
    if (userSpecifiedPattern == false) {
      if (theTclLoadPattern == 0) {
        opserr << "WARNING no current load pattern - load " << nodeId;
        opserr << " " << ndf << " forces\n";
        return TCL_ERROR;
      } else
        loadPatternTag = theTclLoadPattern->getTag();
    }

    // create the load
    theLoad = new NodalLoad(nodeLoadTag, nodeId, forces, isLoadConst);

  } // end of Liming change for nodal thermal action , putting the above block
    // into else{ }

  // add the load to the domain
  if (theTclDomain->addNodalLoad(theLoad, loadPatternTag) == false) {
    opserr << "WARNING TclBasicBuilder - could not add load to domain\n";
    delete theLoad;
    return TCL_ERROR;
  }
  nodeLoadTag++;

  // if get here we have sucessfully created the load and added it to the domain
  return TCL_OK;
}
#endif



int
TclCommand_addImposedMotionSP(ClientData clientData, Tcl_Interp *interp,
                              int argc, TCL_Char ** const argv)
{
  // ensure the destructor has not been called -
  if (theTclBuilder == 0) {
    opserr << "WARNING builder has been destroyed - sp \n";
    return TCL_ERROR;
  }


  // check number of arguments
  if (argc < 4) {
    opserr
        << "WARNING bad command - want: imposedMotion nodeId dofID gMotionID\n";
    return TCL_ERROR;
  }

  // get the nodeID, dofId and value of the constraint
  int nodeId, dofId, gMotionID;

  if (Tcl_GetInt(interp, argv[1], &nodeId) != TCL_OK) {
    opserr << "WARNING invalid nodeId: " << argv[1];
    opserr << " - imposedMotion nodeId dofID gMotionID\n";
    return TCL_ERROR;
  }

  if (Tcl_GetInt(interp, argv[2], &dofId) != TCL_OK) {
    opserr << "WARNING invalid dofId: " << argv[2] << " -  imposedMotion ";
    opserr << nodeId << " dofID gMotionID\n";
    return TCL_ERROR;
  }
  dofId--; // DECREMENT THE DOF VALUE BY 1 TO GO TO OUR C++ INDEXING

  if (Tcl_GetInt(interp, argv[3], &gMotionID) != TCL_OK) {
    opserr << "WARNING invalid gMotionID: " << argv[3] << " -  imposedMotion ";
    opserr << nodeId << " dofID gMotionID\n";
    return TCL_ERROR;
  }

  bool alt = false;
  if (argc == 5) {
    if (strcmp(argv[4], "-other") == 0)
      alt = true;
  }

  //
  // check valid node & dof
  //

  Node *theNode = theTclDomain->getNode(nodeId);
  if (theNode == 0) {
    opserr << "WARNING invalid node " << argv[2] << " node not found\n ";
    return -1;
  }
  int nDof = theNode->getNumberDOF();
  if (dofId < 0 || dofId >= nDof) {
    opserr << "WARNING invalid dofId: " << argv[2]
           << " dof specified cannot be <= 0 or greater than num dof at nod\n ";
    return -2;
  }

  MultiSupportPattern *thePattern = (MultiSupportPattern *)Tcl_GetAssocData(interp, "theTclMultiSupportPattern", NULL);
  int loadPatternTag = thePattern->getTag();

  // create a new ImposedMotionSP
  SP_Constraint *theSP;
  if (alt == true) {
    theSP = new ImposedMotionSP1(nodeId, dofId, loadPatternTag, gMotionID);
  } else {
    theSP = new ImposedMotionSP(nodeId, dofId, loadPatternTag, gMotionID);
  }

  if (thePattern->addSP_Constraint(theSP) == false) {
    opserr << "WARNING could not add SP_Constraint to pattern ";
    delete theSP;
    return TCL_ERROR;
  }

  // if get here we have sucessfully created the node and added it to the domain
  return TCL_OK;
}

int
TclCommand_addEqualDOF_MP(ClientData clientData, Tcl_Interp *interp, int argc,
                          TCL_Char ** const argv)
{
  // Ensure the destructor has not been called
  if (theTclBuilder == 0) {
    opserr << "WARNING builder has been destroyed - equalDOF \n";
    return TCL_ERROR;
  }

  // Check number of arguments
  if (argc < 4) {
    opserr << "WARNING bad command - want: equalDOF RnodeID? CnodeID? DOF1? "
              "DOF2? ...";
    return TCL_ERROR;
  }

  // Read in the node IDs and the DOF
  int RnodeID, CnodeID, dofID;

  if (Tcl_GetInt(interp, argv[1], &RnodeID) != TCL_OK) {
    opserr << "WARNING invalid RnodeID: " << argv[1]
           << " equalDOF RnodeID? CnodeID? DOF1? DOF2? ...";
    return TCL_ERROR;
  }
  if (Tcl_GetInt(interp, argv[2], &CnodeID) != TCL_OK) {
    opserr << "WARNING invalid CnodeID: " << argv[2]
           << " equalDOF RnodeID? CnodeID? DOF1? DOF2? ...";
    return TCL_ERROR;
  }

  // The number of DOF to be coupled
  int numDOF = argc - 3;

  // The constraint matrix ... U_c = C_cr * U_r
  Matrix Ccr(numDOF, numDOF);
  Ccr.Zero();

  // The vector containing the retained and constrained DOFs
  ID rcDOF(numDOF);

  int i, j;
  // Read the degrees of freedom which are to be coupled
  for (i = 3, j = 0; i < argc; i++, j++) {
    if (Tcl_GetInt(interp, argv[i], &dofID) != TCL_OK) {
      opserr << "WARNING invalid dofID: " << argv[3]
             << " equalDOF RnodeID? CnodeID? DOF1? DOF2? ...";
      return TCL_ERROR;
    }

    dofID -= 1; // Decrement for C++ indexing
    if (dofID < 0) {
      opserr << "WARNING invalid dofID: " << argv[i] << " must be >= 1";
      return TCL_ERROR;
    }
    rcDOF(j) = dofID;
    Ccr(j, j) = 1.0;
  }

  // Create the multi-point constraint
  MP_Constraint *theMP = new MP_Constraint(RnodeID, CnodeID, Ccr, rcDOF, rcDOF);

  // Add the multi-point constraint to the domain
  if (theTclDomain->addMP_Constraint(theMP) == false) {
    opserr << "WARNING could not add equalDOF MP_Constraint to domain ";
    delete theMP;
    return TCL_ERROR;
  }

  char buffer[80];
  sprintf(buffer, "%d", theMP->getTag());
  Tcl_SetResult(interp, buffer, TCL_VOLATILE);

  return TCL_OK;
}

int
TclCommand_addEqualDOF_MP_Mixed(ClientData clientData, Tcl_Interp *interp,
                                int argc, TCL_Char ** const argv)
{
  // Ensure the destructor has not been called
  if (theTclBuilder == 0) {
    opserr << "WARNING builder has been destroyed - equalDOF \n";
    return TCL_ERROR;
  }

  // Check number of arguments
  if (argc < 4) {
    opserr << "WARNING bad command - want: equalDOFmixed RnodeID? CnodeID? "
              "numDOF? RDOF1? CDOF1? ... ...";
    return TCL_ERROR;
  }

  // Read in the node IDs and the DOF
  int RnodeID, CnodeID, dofIDR, dofIDC, numDOF;

  if (Tcl_GetInt(interp, argv[1], &RnodeID) != TCL_OK) {
    opserr << "WARNING invalid RnodeID: " << argv[1]
           << " equalDOF RnodeID? CnodeID? numDOF? RDOF1? CDOF1? ...";
    return TCL_ERROR;
  }
  if (Tcl_GetInt(interp, argv[2], &CnodeID) != TCL_OK) {
    opserr << "WARNING invalid CnodeID: " << argv[2]
           << " equalDOF RnodeID? CnodeID? numDOF? RDOF1? CDOF1? ...";
    return TCL_ERROR;
  }

  if (Tcl_GetInt(interp, argv[3], &numDOF) != TCL_OK) {
    opserr << "WARNING invalid numDOF: " << argv[2]
           << " equalDOF RnodeID? CnodeID? numDOF? RDOF1? CDOF1? ...";
    return TCL_ERROR;
  }

  // The number of DOF to be coupled
  //        int numDOF = argc - 3;

  // The constraint matrix ... U_c = C_cr * U_r
  Matrix Ccr(numDOF, numDOF);
  Ccr.Zero();

  // The vector containing the retained and constrained DOFs
  ID rDOF(numDOF);
  ID cDOF(numDOF);

  int i, j, k;
  // Read the degrees of freedom which are to be coupled
  for (i = 4, j = 5, k = 0; k < numDOF; i += 2, j += 2, k++) {
    if (Tcl_GetInt(interp, argv[i], &dofIDR) != TCL_OK) {
      opserr << "WARNING invalid dofID: " << argv[3]
             << " equalDOF RnodeID? CnodeID? DOF1? DOF2? ...";
      return TCL_ERROR;
    }
    if (Tcl_GetInt(interp, argv[j], &dofIDC) != TCL_OK) {
      opserr << "WARNING invalid dofID: " << argv[3]
             << " equalDOF RnodeID? CnodeID? DOF1? DOF2? ...";
      return TCL_ERROR;
    }

    dofIDR -= 1; // Decrement for C++ indexing
    dofIDC -= 1;
    if (dofIDC < 0 || dofIDR < 0) {
      opserr << "WARNING invalid dofID: " << argv[i] << " must be >= 1";
      return TCL_ERROR;
    }
    rDOF(k) = dofIDR;
    cDOF(k) = dofIDC;
    Ccr(k, k) = 1.0;
  }

  // Create the multi-point constraint
  MP_Constraint *theMP = new MP_Constraint(RnodeID, CnodeID, Ccr, cDOF, rDOF);

  // Add the multi-point constraint to the domain
  if (theTclDomain->addMP_Constraint(theMP) == false) {
    opserr << "WARNING could not add equalDOF MP_Constraint to domain ";
    delete theMP;
    return TCL_ERROR;
  }

  char buffer[80];
  sprintf(buffer, "%d", theMP->getTag());
  Tcl_SetResult(interp, buffer, TCL_VOLATILE);

  return TCL_OK;
}


////////////////////////////////////////////////////////////////////////////////////////////
// Added by Scott J. Brandenberg, UC Davis, sjbrandenberg@ucdavis.edu
int
TclCommand_doPySimple1Gen(ClientData clientData, Tcl_Interp *interp, int argc,
                          TCL_Char ** const argv)
{
  if (argc < 6 || argc > 7) {
    opserr
        << "WARNING PySimple1Gen file1? file2? file3? file4? file5? <file6?>";
    opserr << "Must have either 5 or 6 arguments." << endln;
  }

  PySimple1Gen *thePySimple1Gen;
  thePySimple1Gen = new PySimple1Gen;

  if (argc == 6)
    thePySimple1Gen->WritePySimple1(argv[1], argv[2], argv[3], argv[4],
                                    argv[5]);
  if (argc == 7)
    thePySimple1Gen->WritePySimple1(argv[1], argv[2], argv[3], argv[4], argv[5],
                                    argv[6]);

  delete thePySimple1Gen;

  return TCL_OK;
}

int
TclCommand_doTzSimple1Gen(ClientData clientData, Tcl_Interp *interp, int argc,
                          TCL_Char ** const argv)
{
  if (argc < 6 || argc > 7) {
    opserr
        << "WARNING TzSimple1Gen file1? file2? file3? file4? file5? <file6?>";
    opserr << "Must have either 5 or 6 arguments." << endln;
  }

  TzSimple1Gen *theTzSimple1Gen;
  theTzSimple1Gen = new TzSimple1Gen;

  if (argc == 6)
    theTzSimple1Gen->WriteTzSimple1(argv[1], argv[2], argv[3], argv[4],
                                    argv[5]);
  if (argc == 7)
    theTzSimple1Gen->WriteTzSimple1(argv[1], argv[2], argv[3], argv[4], argv[5],
                                    argv[6]);

  delete theTzSimple1Gen;

  return TCL_OK;
}
// End Added by Scott J. Brandenberg
///////////////////////////////////////////////////////////////////////////////////////////////////

// Added by Prishati Raychowdhury (UCSD)
int
TclBasicBuilder_doShallowFoundationGen(ClientData clientData,
                                       Tcl_Interp *interp, int argc,
                                       TCL_Char ** const argv)
{
  if (argc != 5) {
    opserr << "WARNING ShallowFoundationGen FoundationID? ConnectingNode? "
              "InputDataFile? FoundationMatType?";
    opserr << "Must have 4 arguments." << endln;
  }

  ShallowFoundationGen *theShallowFoundationGen;
  theShallowFoundationGen = new ShallowFoundationGen;

  // Checking for error
  int FoundationID;
  int ConnectingNode;
  int FoundationMatType;

  if (Tcl_GetInt(interp, argv[1], &FoundationID) != TCL_OK) {
    opserr << "WARNING invalid FoundationID: " << argv[1]
           << ". ShallowFoundationGen FoundationID? ConnectingNode? "
              "InputDataFile? FoundationMatType? ";
    return TCL_ERROR;
  }
  if (Tcl_GetInt(interp, argv[2], &ConnectingNode) != TCL_OK) {
    opserr << "WARNING invalid ConnectingNode: " << argv[2]
           << ". ShallowFoundationGen FoundationID? ConnectingNode? "
              "InputDataFile? FoundationMatType? ";
    return TCL_ERROR;
  }
  if (Tcl_GetInt(interp, argv[4], &FoundationMatType) != TCL_OK) {
    opserr << "WARNING invalid FoundationMatType: " << argv[4]
           << ". ShallowFoundationGen FoundationID? ConnectingNode? "
              "InputDataFile? FoundationMatType? ";
    return TCL_ERROR;
  }

  theShallowFoundationGen->GetShallowFoundation(argv[1], argv[2], argv[3],
                                                argv[4]);
  delete theShallowFoundationGen;

  return TCL_OK;
}
// End PRC

int
TclBasicBuilder_addRemoHFiber(ClientData clientData, Tcl_Interp *interp,
                              int argc, TCL_Char ** const argv)
{
  return TclCommand_addHFiber(clientData, interp, argc, argv, theTclBuilder);
}

extern int TclBasicBuilderStiffnessDegradationCommand(ClientData clientData,
                                                      Tcl_Interp *interp,
                                                      int argc, TCL_Char ** const argv,
                                                      Domain *theDomain);

int
TclCommand_addStiffnessDegradation(ClientData clientData, Tcl_Interp *interp,
                                   int argc, TCL_Char ** const argv)
{
  return TclBasicBuilderStiffnessDegradationCommand(clientData, interp, argc,
                                                    argv, theTclDomain);
}

extern int TclBasicBuilderUnloadingRuleCommand(ClientData clientData,
                                               Tcl_Interp *interp, int argc,
                                               TCL_Char ** const argv,
                                               Domain *theDomain);

int
TclCommand_addUnloadingRule(ClientData clientData, Tcl_Interp *interp, int argc,
                            TCL_Char ** const argv)
{
  return TclBasicBuilderUnloadingRuleCommand(clientData, interp, argc, argv,
                                             theTclDomain);
}

extern int TclBasicBuilderStrengthDegradationCommand(ClientData clientData,
                                                     Tcl_Interp *interp,
                                                     int argc, TCL_Char ** const argv,
                                                     Domain *theDomain);

int
TclCommand_addStrengthDegradation(ClientData clientData, Tcl_Interp *interp,
                                  int argc, TCL_Char ** const argv)
{
  return TclBasicBuilderStrengthDegradationCommand(clientData, interp, argc,
                                                   argv, theTclDomain);
}

#if 0
/// added by ZHY
extern int TclCommand_UpdateMaterialsCommand(ClientData clientData,
                                             Tcl_Interp *interp, int argc,
                                             TCL_Char ** const argv,
                                             TclBasicBuilder *theTclBuilder,
                                             Domain *theDomain);
int
TclCommand_UpdateMaterials(ClientData clientData, Tcl_Interp *interp, int argc,
                           TCL_Char ** const argv)
{
  return TclCommand_UpdateMaterialsCommand(clientData, interp, argc, argv,
                                           theTclBuilder, theTclDomain);
}
#endif


extern int TclBasicBuilderFrictionModelCommand(ClientData clienData,
                                               Tcl_Interp *interp, int argc,
                                               TCL_Char ** const argv,
                                               Domain *theDomain);


int
TclCommand_Package(ClientData clientData, Tcl_Interp *interp, int argc,
                   TCL_Char ** const argv)
{

  void *libHandle;
  int (*funcPtr)(ClientData clientData, Tcl_Interp *interp, int argc,
                 TCL_Char ** const argv, Domain *, TclBasicBuilder *);

  int res = -1;

  if (argc == 2) {
    res = getLibraryFunction(argv[1], argv[1], &libHandle, (void **)&funcPtr);
  } else if (argc == 3) {
    res = getLibraryFunction(argv[1], argv[2], &libHandle, (void **)&funcPtr);
  }

  if (res == 0) {
    res =
        (*funcPtr)(clientData, interp, argc, argv, theTclDomain, theTclBuilder);
  } else {
    opserr << "Error: Could not find function: " << argv[1] << endln;
    return -1;
  }

  return res;
}

