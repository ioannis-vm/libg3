/* ****************************************************************** **
**    OpenSees - Open System for Earthquake Engineering Simulation    **
**          Pacific Earthquake Engineering Research Center            **
** ****************************************************************** */
//
// Description: This file contains the function invoked when the user invokes
// the section command in the interpreter.
//
// Written: rms, MHS, cmp
// Created: 07/99
//
#include <assert.h>
#include <tcl.h>
#include <g3_api.h>
#include <G3_Logging.h>
#include <elementAPI.h>
#include <TclBasicBuilder.h>
#include <runtime/BasicModelBuilder.h>

extern "C" int OPS_ResetInputNoBuilder(ClientData clientData,
                                       Tcl_Interp *interp, int cArg, int mArg,
                                       TCL_Char ** const argv, Domain *domain);

#include <ElasticMaterial.h>
#include <ElasticSection2d.h>
#include <ElasticSection3d.h>
#include <ElasticShearSection2d.h>
#include <ElasticShearSection3d.h>
#include <ElasticWarpingShearSection2d.h>
#include <ElasticTubeSection3d.h>
#include <SectionAggregator.h>
#include <ParallelSection.h>
//#include <FiberSection.h>
#include <FiberSection2d.h>
#include <NDFiberSection2d.h>
#include <NDFiberSection3d.h>
#include <NDFiberSectionWarping2d.h>
#include <FiberSection2dInt.h>
#include <FiberSection3d.h>
#include <FiberSectionAsym3d.h>
//#include <FiberSectionGJ.h>
#include <FiberSectionRepr.h>

#include <LayeredShellFiberSection.h> // Yuli Huang & Xinzheng Lu

#include <ElasticPlateSection.h>
#include <ElasticMembranePlateSection.h>
#include <MembranePlateFiberSection.h>

#include <QuadPatch.h>
#include <CircPatch.h>
#include <QuadCell.h>
#include <StraightReinfLayer.h>
#include <CircReinfLayer.h>
#include <ReinfBar.h>

#include <UniaxialFiber2d.h>
#include <UniaxialFiber3d.h>
#include <NDFiber2d.h>
#include <NDFiber3d.h>

#include <Bidirectional.h>
#include <Elliptical2.h>
#include <Isolator2spring.h>

//#include <WSection2d.h>
#include <WideFlangeSectionIntegration.h>
#include <RCSectionIntegration.h>
#include <RCTBeamSectionIntegration.h>
#include <RCCircularSectionIntegration.h>
#include <RCTunnelSectionIntegration.h>
// #include <RCTBeamSectionIntegrationUniMat.h>
// #include <TubeSectionIntegration.h>

//#include <McftSection2dfiber.h>

#include <string.h>
#include <fstream>
using std::ifstream;

#include <iostream>
using std::ios;

#include <packages.h>

extern OPS_Routine OPS_ElasticSection;
extern OPS_Routine OPS_ElasticWarpingShearSection2d;
extern OPS_Routine OPS_ElasticTubeSection3d;
extern OPS_Routine OPS_UniaxialSection;
extern OPS_Routine OPS_ParallelSection;
extern OPS_Routine OPS_Bidirectional;
extern OPS_Routine OPS_Elliptical2;
// extern OPS_Routine OPS_WFSection2d;
// extern OPS_Routine OPS_RCCircularSection;
// extern OPS_Routine OPS_RCSection2d;
// extern OPS_Routine OPS_RCTBeamSection2d;
// extern OPS_Routine OPS_RCTunnelSection;
// extern OPS_Routine OPS_TubeSection;

int TclCommand_addFiberSection(ClientData clientData, Tcl_Interp *interp,
                               int argc, TCL_Char ** const argv,
                               TclBasicBuilder *theBuilder);

int TclCommand_addFiberSectionAsym(ClientData clientData, Tcl_Interp *interp,
                                   int argc, TCL_Char ** const argv,
                                   TclBasicBuilder *theBuilder);

int TclCommand_addFiberIntSection(ClientData clientData, Tcl_Interp *interp,
                                  int argc, TCL_Char ** const argv,
                                  TclBasicBuilder *theBuilder);

//--- Adding Thermo-mechanical Sections:[BEGIN]   by UoE OpenSees Group ---//
#include <FiberSection2dThermal.h>
#include <FiberSection3dThermal.h> //Added by L.Jiang [SIF] 2017
//#include <FiberSectionGJThermal.h> //Added by Liming, [SIF] 2017
#include <MembranePlateFiberSectionThermal.h> //Added by Liming, [SIF] 2017
#include <LayeredShellFiberSectionThermal.h>  //Added by Liming, [SIF] 2017

int TclCommand_addFiberSectionThermal(ClientData clientData, Tcl_Interp *interp, int argc, TCL_Char ** const argv, TclBasicBuilder *theBuilder);
static int buildSectionThermal(ClientData clientData, Tcl_Interp *interp, TclBasicBuilder *theTclBasicBuilder, int secTag, UniaxialMaterial &theTorsion);
//--- Adding Thermo-mechanical Sections: [END]   by UoE OpenSees Group ---//

int TclCommand_addUCFiberSection(ClientData clientData, Tcl_Interp *interp, int argc, TCL_Char ** const argv, TclBasicBuilder *theBuilder);

SectionForceDeformation *
TclBasicBuilderYS_SectionCommand(ClientData clientData, Tcl_Interp *interp, int argc, TCL_Char ** const argv, TclBasicBuilder *theTclBuilder);

int
TclCommand_addSection(ClientData clientData, Tcl_Interp *interp,
                              int argc, TCL_Char ** const argv)
{
  assert(clientData != nullptr);
  G3_Runtime *rt = G3_getRuntime(interp);
  TclBasicBuilder *theTclBuilder = (TclBasicBuilder*)clientData;
  BasicModelBuilder *builder = (BasicModelBuilder*)clientData;
  Domain *theDomain = builder->getDomain();

  // Make sure there is a minimum number of arguments
  if (argc < 3) {
    opserr << G3_ERROR_PROMPT << "insufficient number of section arguments\n";
    opserr << "Want: section type? tag? <specific material args>" << endln;
    return TCL_ERROR;
  }

  OPS_ResetInputNoBuilder(clientData, interp, 2, argc, argv, theDomain);

  // Pointer to a section that will be added to the model builder
  SectionForceDeformation *theSection = nullptr;

  // Check argv[1] to dispatch section type

  if (strcmp(argv[1], "Fiber") == 0 || 
      strcmp(argv[1], "fiberSec") == 0 ||
      strcmp(argv[1], "FiberSection") == 0 ||
      strcmp(argv[1], "NDFiberWarping") == 0 ||
      strcmp(argv[1], "NDFiber") == 0)

    return TclCommand_addFiberSection(clientData, interp, argc, argv, theTclBuilder);

  else if (strcmp(argv[1], "FiberAsym") == 0 ||
           strcmp(argv[1], "fiberSecAsym") == 0)

    return TclCommand_addFiberSectionAsym(clientData, interp, argc, argv, theTclBuilder);

  else if (strcmp(argv[1], "FiberThermal") == 0 ||
           strcmp(argv[1], "fiberSecThermal") == 0)

    return TclCommand_addFiberSectionThermal(clientData, interp, argc, argv, theTclBuilder);

  else if (strcmp(argv[1], "FiberInt") == 0)

    return TclCommand_addFiberIntSection(clientData, interp, argc, argv, theTclBuilder);

  else if (strcmp(argv[1], "UCFiber") == 0)

    return TclCommand_addUCFiberSection(clientData, interp, argc, argv, theTclBuilder);

  else if (strcmp(argv[1], "Parallel") == 0) {
    void *theMat = OPS_ParallelSection(rt, argc, argv);
    if (theMat != nullptr)
      theSection = (SectionForceDeformation *)theMat;
    else
      return TCL_ERROR;
  }

  else if (strcmp(argv[1], "Elastic") == 0) {
    void *theMat = OPS_ElasticSection(rt, argc, argv);
    if (theMat != nullptr)
      theSection = (SectionForceDeformation *)theMat;
    else
      return TCL_ERROR;
  }

  else if (strcmp(argv[1], "ElasticWarpingShear") == 0) {
    void *theMat = OPS_ElasticWarpingShearSection2d(rt, argc, argv);
    if (theMat != 0)
      theSection = (SectionForceDeformation *)theMat;
    else
      return TCL_ERROR;
  }

  else if (strcmp(argv[1], "Generic1D") == 0 ||
           strcmp(argv[1], "Generic1d") == 0 ||
           strcmp(argv[1], "Uniaxial") == 0) {
    void *theMat = OPS_UniaxialSection(rt, argc, argv);
    if (theMat != 0)
      theSection = (SectionForceDeformation *)theMat;
    else
      return TCL_ERROR;
  }


  // created by Yuli Huang & Xinzheng Lu ----
  else if (strcmp(argv[1], "Bidirectional") == 0) {
    void *theMat = OPS_Bidirectional(rt, argc, argv);
    if (theMat != 0)
      theSection = (SectionForceDeformation *)theMat;
    else
      return TCL_ERROR;
  }

  else if (strcmp(argv[1], "Elliptical") == 0 ||
           strcmp(argv[1], "Elliptical2") == 0) {
    void *theMat = OPS_Elliptical2(rt, argc, argv);
    if (theMat != 0)
      theSection = (SectionForceDeformation *)theMat;
    else
      return TCL_ERROR;
  }

#if 0 // TODO[cmp]: Keep names, but add removal message

  else if (strcmp(argv[1], "ElasticTube") == 0) {
    void *theMat = OPS_ElasticTubeSection3d(rt, argc, argv);
    if (theMat != 0)
      theSection = (SectionForceDeformation *)theMat;
    else
      return TCL_ERROR;
  }
  else if (strcmp(argv[1], "WFSection2d") == 0 ||
           strcmp(argv[1], "WSection2d") == 0) {
    void *theMat = OPS_WFSection2d(rt, argc, argv);
    if (theMat != 0)
      theSection = (SectionForceDeformation *)theMat;
    else
      return TCL_ERROR;
  }


  else if (strcmp(argv[1], "RCSection2d") == 0) {
    void *theMat = OPS_RCSection2d(rt, argc, argv);
    if (theMat != 0)
      theSection = (SectionForceDeformation *)theMat;
    else
      return TCL_ERROR;
  }

  else if (strcmp(argv[1], "RCCircularSection") == 0) {
    void *theMat = OPS_RCCircularSection(rt, argc, argv);
    if (theMat != 0)
      theSection = (SectionForceDeformation *)theMat;
    else
      return TCL_ERROR;
  }

  else if (strcmp(argv[1], "RCTunnelSection") == 0) {
    void *theMat = OPS_RCTunnelSection(rt, argc, argv);
    if (theMat != 0)
      theSection = (SectionForceDeformation *)theMat;
    else
      return TCL_ERROR;
  }

  else if (strcmp(argv[1], "RCTBeamSection2d") == 0 ||
           strcmp(argv[1], "RCTBeamSectionUniMat2d") == 0) {
    void *theMat = OPS_RCTBeamSection2d(rt, argc, argv);
    if (theMat != 0)
      theSection = (SectionForceDeformation *)theMat;
    else
      return TCL_ERROR;
  }
#endif


  else if (strcmp(argv[1], "AddDeformation") == 0 ||
           strcmp(argv[1], "Aggregator") == 0  ||
           strcmp(argv[1], "Aggregate") == 0) {
    if (argc < 5) {
      opserr << G3_ERROR_PROMPT << "insufficient arguments\n";
      opserr << "Want: section Aggregator tag? uniTag1? code1? ... <-section "
                "secTag?>"
             << endln;
      return TCL_ERROR;
    }

    int tag;
    int secTag;
    SectionForceDeformation *theSec = nullptr;

    if (Tcl_GetInt(interp, argv[2], &tag) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid Aggregator tag" << endln;
      return TCL_ERROR;
    }

    int nArgs = argc - 3;

    for (int ii = 5; ii < argc; ii++) {
      if (strcmp(argv[ii], "-section") == 0 && ++ii < argc) {
        if (Tcl_GetInt(interp, argv[ii], &secTag) != TCL_OK) {
          opserr << G3_ERROR_PROMPT << "invalid Aggregator tag" << endln;
          return TCL_ERROR;
        }

        theSec = builder->getSection(secTag);

        if (theSec == 0) {
          opserr << G3_ERROR_PROMPT << "section does not exist\n";
          opserr << "section: " << secTag;
          opserr << "\nsection Aggregator: " << tag << endln;
          return TCL_ERROR;
        }

        nArgs -= 2;
      }
    }

    int nMats = nArgs / 2;

    if (nArgs % 2 != 0) {
      opserr << G3_ERROR_PROMPT << "improper number of arguments for Aggregator" << endln;
      return TCL_ERROR;
    }

    UniaxialMaterial **theMats = 0;
    ID codes(nMats);

    theMats = new UniaxialMaterial *[nMats];

    if (theMats == 0) {
      opserr << "TclBasicBuilderSection (Aggregator) -- unable to create "
                "uniaxial array"
             << endln;
      return TCL_ERROR;
    }

    int tagI;
    int i, j;

    for (i = 3, j = 0; j < nMats; i++, j++) {
      if (Tcl_GetInt(interp, argv[i], &tagI) != TCL_OK) {
        opserr << G3_ERROR_PROMPT << "invalid Aggregator matTag" << endln;
        return TCL_ERROR;
      }

      theMats[j] = builder->getUniaxialMaterial(tagI);

      if (theMats[j] == 0) {
        opserr << G3_ERROR_PROMPT << "uniaxial material does not exist\n";
        opserr << "uniaxial material: " << tagI;
        opserr << "\nsection Aggregator: " << tag << endln;
        return TCL_ERROR;
      }

      i++;

      if (strcmp(argv[i], "Mz") == 0)
        codes(j) = SECTION_RESPONSE_MZ;
      else if (strcmp(argv[i], "P") == 0)
        codes(j) = SECTION_RESPONSE_P;
      else if (strcmp(argv[i], "Vy") == 0)
        codes(j) = SECTION_RESPONSE_VY;
      else if (strcmp(argv[i], "My") == 0)
        codes(j) = SECTION_RESPONSE_MY;
      else if (strcmp(argv[i], "Vz") == 0)
        codes(j) = SECTION_RESPONSE_VZ;
      else if (strcmp(argv[i], "T") == 0)
        codes(j) = SECTION_RESPONSE_T;
      else {
        opserr << G3_ERROR_PROMPT << "invalid code" << endln;
        opserr << "\nsection Aggregator: " << tag << endln;
        return TCL_ERROR;
      }
    }

    if (theSec)
      theSection = new SectionAggregator(tag, *theSec, nMats, theMats, codes);
    else
      theSection = new SectionAggregator(tag, nMats, theMats, codes);

    delete[] theMats;
  }


  else if (strcmp(argv[1], "ElasticPlateSection") == 0) {

    if (argc < 5) {
      opserr << G3_ERROR_PROMPT << "insufficient arguments\n";
      opserr << "Want: section ElasticPlateSection tag? E? nu? h? " << endln;
      return TCL_ERROR;
    }

    int tag;
    double E, nu, h;

    if (Tcl_GetInt(interp, argv[2], &tag) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid section ElasticPlateSection tag" << endln;
      return TCL_ERROR;
    }

    if (Tcl_GetDouble(interp, argv[3], &E) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid E" << endln;
      opserr << "ElasticPlateSection section: " << tag << endln;
      return TCL_ERROR;
    }

    if (Tcl_GetDouble(interp, argv[4], &nu) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid nu" << endln;
      opserr << "ElasticPlateSection section: " << tag << endln;
      return TCL_ERROR;
    }

    if (Tcl_GetDouble(interp, argv[5], &h) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid h" << endln;
      opserr << "ElasticPlateSection section: " << tag << endln;
      return TCL_ERROR;
    }

    theSection = new ElasticPlateSection(tag, E, nu, h);
  }

  else if (strcmp(argv[1], "ElasticMembranePlateSection") == 0) {
    if (argc < 5) {
      opserr << G3_ERROR_PROMPT << "insufficient arguments\n";
      opserr << "Want: section ElasticMembranePlateSection tag? E? nu? h? "
                "<rho?> <Ep_mod?>"
             << endln;
      return TCL_ERROR;
    }

    int tag;
    double E, nu, h;
    double rho = 0.0;
    double Ep_mod = 1.0;

    if (Tcl_GetInt(interp, argv[2], &tag) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid section ElasticMembranePlateSection tag"
             << endln;
      return TCL_ERROR;
    }

    if (Tcl_GetDouble(interp, argv[3], &E) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid E" << endln;
      opserr << "ElasticMembranePlateSection section: " << tag << endln;
      return TCL_ERROR;
    }

    if (Tcl_GetDouble(interp, argv[4], &nu) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid nu" << endln;
      opserr << "ElasticMembranePlateSection section: " << tag << endln;
      return TCL_ERROR;
    }

    if (Tcl_GetDouble(interp, argv[5], &h) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid h" << endln;
      opserr << "ElasticMembranePlateSection section: " << tag << endln;
      return TCL_ERROR;
    }

    if (argc > 6 && Tcl_GetDouble(interp, argv[6], &rho) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid rho" << endln;
      opserr << "ElasticMembranePlateSection section: " << tag << endln;
      return TCL_ERROR;
    }

    if (argc > 7 && Tcl_GetDouble(interp, argv[7], &Ep_mod) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid Ep_mod" << endln;
      opserr << "ElasticMembranePlateSection section: " << tag << endln;
      return TCL_ERROR;
    }

    theSection = new ElasticMembranePlateSection(tag, E, nu, h, rho, Ep_mod);
  }

  else if (strcmp(argv[1], "PlateFiber") == 0) {
    if (argc < 5) {
      opserr << G3_ERROR_PROMPT << "insufficient arguments\n";
      opserr << "Want: section PlateFiber tag? matTag? h? " << endln;
      return TCL_ERROR;
    }

    int tag, matTag;
    double h;

    if (Tcl_GetInt(interp, argv[2], &tag) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid section PlateFiber tag" << endln;
      return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[3], &matTag) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid matTag" << endln;
      opserr << "PlateFiber section: " << matTag << endln;
      return TCL_ERROR;
    }

    if (Tcl_GetDouble(interp, argv[4], &h) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid h" << endln;
      opserr << "PlateFiber section: " << tag << endln;
      return TCL_ERROR;
    }

    NDMaterial *theMaterial = builder->getNDMaterial(matTag);
    if (theMaterial == 0) {
      opserr << G3_ERROR_PROMPT << "nD material does not exist\n";
      opserr << "nD material: " << matTag;
      opserr << "\nPlateFiber section: " << tag << endln;
      return TCL_ERROR;
    }

    theSection = new MembranePlateFiberSection(tag, h, *theMaterial);
  }

  // start Yuli Huang & Xinzheng Lu LayeredShellFiberSection
  else if (strcmp(argv[1], "LayeredShell") == 0) {
    if (argc < 6) {
      opserr << G3_ERROR_PROMPT << "insufficient arguments" << endln;
      opserr << "Want: section LayeredShell tag? nLayers? matTag1? h1? ... "
                "matTagn? hn? "
             << endln;
      return TCL_ERROR;
    }

    int tag, nLayers, matTag;
    double h, *thickness;
    NDMaterial **theMats;

    if (Tcl_GetInt(interp, argv[2], &tag) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid section LayeredShell tag" << endln;
      return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[3], &nLayers) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid nLayers" << endln;
      opserr << "LayeredShell section: " << tag << endln;
      return TCL_ERROR;
    }

    if (nLayers < 3) {
      opserr << "ERROR number of layers must be larger than 2" << endln;
      opserr << "LayeredShell section: " << tag << endln;
      return TCL_ERROR;
    }

    theMats = new NDMaterial *[nLayers];
    thickness = new double[nLayers];

    if (argc < 3+2*nLayers) {
      opserr << G3_ERROR_PROMPT << "Must provide " << 2*nLayers << " layers\n";
      return TCL_ERROR;
    }

    for (int iLayer = 0; iLayer < nLayers; iLayer++) {

      if (Tcl_GetInt(interp, argv[4 + 2 * iLayer], &matTag) != TCL_OK) {
        opserr << G3_ERROR_PROMPT << "invalid matTag" << endln;
        opserr << "LayeredShell section: " << tag << endln;
        return TCL_ERROR;
      }

      theMats[iLayer] = builder->getNDMaterial(matTag);
      if (theMats[iLayer] == 0) {
        opserr << G3_ERROR_PROMPT << "nD material does not exist" << endln;
        opserr << "nD material: " << matTag;
        return TCL_ERROR;
      }

      if (Tcl_GetDouble(interp, argv[5 + 2 * iLayer], &h) != TCL_OK) {
        opserr << G3_ERROR_PROMPT << "invalid h" << endln;
        opserr << "LayeredShell section: " << tag << endln;
        return TCL_ERROR;
      }

      if (h < 0) {
        opserr << G3_ERROR_PROMPT << "invalid h" << endln;
        opserr << "PlateFiber section: " << tag << endln;
        return TCL_ERROR;
      }

      thickness[iLayer] = h;
    }

    theSection = new LayeredShellFiberSection(tag, nLayers, thickness, theMats);
    if (thickness != 0)
      delete[] thickness;
    if (theMats != 0)
      delete[] theMats;
  }
  // end Yuli Huang & Xinzheng Lu LayeredShellFiberSection

  //-----Thermo-mechanical shell sections added by L.Jiang [SIF]
  else if (strcmp(argv[1], "PlateFiberThermal") == 0) {
    if (argc < 5) {
      opserr << G3_ERROR_PROMPT << "insufficient arguments\n";
      opserr << "Want: section PlateFiberThermal tag? matTag? h? " << endln;
      return TCL_ERROR;
    }

    int tag, matTag;
    double h;

    if (Tcl_GetInt(interp, argv[2], &tag) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid section PlateFiberThermal tag" << endln;
      return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[3], &matTag) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid matTag" << endln;
      opserr << "PlateFiberThermal section: " << matTag << endln;
      return TCL_ERROR;
    }

    if (Tcl_GetDouble(interp, argv[4], &h) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid h" << endln;
      opserr << "PlateFiberThermal section: " << tag << endln;
      return TCL_ERROR;
    }

    NDMaterial *theMaterial = builder->getNDMaterial(matTag);
    if (theMaterial == nullptr) {
      opserr << G3_ERROR_PROMPT << "nD material does not exist\n";
      opserr << "nD material: " << matTag;
      opserr << "\nPlateFiberThermal section: " << tag << endln;
      return TCL_ERROR;
    }

    theSection = new MembranePlateFiberSectionThermal(tag, h, *theMaterial);
  }

  // LayeredShellFiberSectionThermal based on the
  // LayeredShellFiberSectionThermal by Yuli Huang & Xinzheng Lu
  else if (strcmp(argv[1], "LayeredShellThermal") == 0) {
    if (argc < 6) {
      opserr << G3_ERROR_PROMPT << "insufficient arguments" << endln;
      opserr << "Want: section LayeredShellThermal tag? nLayers? matTag1? h1? "
                "... matTagn? hn? "
             << endln;
      return TCL_ERROR;
    }

    int tag, nLayers, matTag;
    double h, *thickness;
    NDMaterial **theMats;

    if (Tcl_GetInt(interp, argv[2], &tag) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid section LayeredShellThermal tag" << endln;
      return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[3], &nLayers) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid nLayers" << endln;
      opserr << "LayeredShellThermal section: " << tag << endln;
      return TCL_ERROR;
    }

    if (nLayers < 3) {
      opserr << "ERROR number of layers must be larger than 2" << endln;
      opserr << "LayeredShellThermal section: " << tag << endln;
      return TCL_ERROR;
    }

    theMats = new NDMaterial *[nLayers];
    thickness = new double[nLayers];

    for (int iLayer = 0; iLayer < nLayers; iLayer++) {
      if (Tcl_GetInt(interp, argv[4 + 2 * iLayer], &matTag) != TCL_OK) {
        opserr << G3_ERROR_PROMPT << "invalid matTag" << endln;
        opserr << "LayeredShellThermal section: " << tag << endln;
        return TCL_ERROR;
      }

      theMats[iLayer] = builder->getNDMaterial(matTag);
      if (theMats[iLayer] == 0) {
        opserr << G3_ERROR_PROMPT << "nD material does not exist" << endln;
        ;
        opserr << "nD material: " << matTag;
        opserr << "LayeredShellThermal section: " << tag << endln;
        return TCL_ERROR;
      }

      if (Tcl_GetDouble(interp, argv[5 + 2 * iLayer], &h) != TCL_OK) {
        opserr << G3_ERROR_PROMPT << "invalid h" << endln;
        opserr << "LayeredShellThermal section: " << tag << endln;
        return TCL_ERROR;
      }

      if (h < 0) {
        opserr << G3_ERROR_PROMPT << "invalid h" << endln;
        opserr << "LayeredShellThermal section: " << tag << endln;
        return TCL_ERROR;
      }

      thickness[iLayer] = h;
    }

    theSection =
        new LayeredShellFiberSectionThermal(tag, nLayers, thickness, theMats);
    if (thickness != 0)
      delete[] thickness;
    if (theMats != 0)
      delete[] theMats;
  }
  // end L.Jiang [SIF] added based on LayeredShellFiberSectionThermal section
  //
  else if (strcmp(argv[1], "Iso2spring") == 0) {
    if (argc < 10) {
      opserr << G3_ERROR_PROMPT << "insufficient arguments\n";
      opserr
          << "Want: section Iso2spring tag? tol? k1? Fy? k2? kv? hb? Pe? <Po?>"
          << endln;
      return TCL_ERROR;
    }

    int tag;
    double tol, k1, Fy, kb, kvo, hb, Pe;
    double Po = 0.0;

    if (Tcl_GetInt(interp, argv[2], &tag) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid Iso2spring tag" << endln;
      return TCL_ERROR;
    }

    if (Tcl_GetDouble(interp, argv[3], &tol) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid tol\n";
      opserr << "section Iso2spring: " << tag << endln;
      return TCL_ERROR;
    }

    if (Tcl_GetDouble(interp, argv[4], &k1) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid k1\n";
      opserr << "section Iso2spring: " << tag << endln;
      return TCL_ERROR;
    }

    if (Tcl_GetDouble(interp, argv[5], &Fy) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid Fy\n";
      opserr << "section Iso2spring: " << tag << endln;
      return TCL_ERROR;
    }

    if (Tcl_GetDouble(interp, argv[6], &kb) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid k2\n";
      opserr << "section Iso2spring: " << tag << endln;
      return TCL_ERROR;
    }

    if (Tcl_GetDouble(interp, argv[7], &kvo) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid kv\n";
      opserr << "section Iso2spring: " << tag << endln;
      return TCL_ERROR;
    }
    if (Tcl_GetDouble(interp, argv[8], &hb) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid hb\n";
      opserr << "section Iso2spring: " << tag << endln;
      return TCL_ERROR;
    }

    if (Tcl_GetDouble(interp, argv[9], &Pe) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid Pe\n";
      opserr << "section Iso2spring: " << tag << endln;
      return TCL_ERROR;
    }
    if (argc > 10) {
      if (Tcl_GetDouble(interp, argv[10], &Po) != TCL_OK) {
        opserr << G3_ERROR_PROMPT << "invalid Po\n";
        opserr << "section Iso2spring: " << tag << endln;
        return TCL_ERROR;
      }
    }

    theSection = new Isolator2spring(tag, tol, k1, Fy, kb, kvo, hb, Pe, Po);
  }

  else {
    theSection = TclBasicBuilderYS_SectionCommand(clientData, interp, argc,
                                                  argv, theTclBuilder);
  }

  // Ensure we have created the Material, out of memory if got here and no
  // section
  if (theSection == nullptr) {
    opserr << G3_ERROR_PROMPT << "could not create section " << argv[1] << endln;
    return TCL_ERROR;
  }

  // Now add the material to the modelBuilder
  if (builder->addSection(*theSection) < 0) {
  // if (OPS_addSectionForceDeformation(theSection) != true) {
    opserr << G3_ERROR_PROMPT << "could not add section to the domain\n";
    opserr << *theSection << endln;
    delete theSection; // invoke the material objects destructor, otherwise mem
                       // leak
    return TCL_ERROR;
  }

  return TCL_OK;
}

// static int currentSectionTag = 0;
static bool currentSectionIsND = false;
static bool currentSectionIsWarping = false;
static bool currentSectionComputeCentroid = true;

static int buildSection(ClientData clientData, Tcl_Interp *interp, TclBasicBuilder *theTclBasicBuilder,
                 int secTag, UniaxialMaterial &theTorsion);

static int buildSectionAsym(ClientData clientData, Tcl_Interp *interp, TclBasicBuilder *theTclBasicBuilder,
                     int secTag, bool isTorsion, double GJ, double Ys,
                     double Zs); // Xinlong

int buildSectionInt(ClientData clientData, Tcl_Interp *interp, TclBasicBuilder *theTclBasicBuilder,
                    int secTag, UniaxialMaterial &theTorsion, int NStrip1,
                    double t1, int NStrip2, double t2, int NStrip3, double t3);

int
TclCommand_addFiberSection(ClientData clientData, Tcl_Interp *interp, int argc,
                           TCL_Char ** const argv, TclBasicBuilder *theTclBasicBuilder)
{
  // G3_Runtime *rt = G3_getRuntime(interp);
  BasicModelBuilder* builder = (BasicModelBuilder*)clientData;

  int secTag;
  int maxNumPatches = 30;
  int maxNumReinfLayers = 30;
  int NDM = builder->getNDM();

  if (argc < 4)
    return TCL_ERROR;

  if (Tcl_GetInt(interp, argv[2], &secTag) != TCL_OK) {
    opserr << G3_ERROR_PROMPT << "bad command - want: \nsection fiberSec secTag { "
              "\n\tpatch <patch arguments> \n\tlayer <layer arguments> \n}\n";
    return TCL_ERROR;
  }

  builder->currentSectionTag = secTag;
  currentSectionIsND = false;
  currentSectionIsWarping = false;
  currentSectionComputeCentroid = true;
  if (strcmp(argv[1], "NDFiber") == 0)
    currentSectionIsND = true;
  if (strcmp(argv[1], "NDFiberWarping") == 0) {
    currentSectionIsND = true;
    currentSectionIsWarping = true;
  }

  // create the fiber section representation (with the geometric information)

  SectionRepres *fiberSectionRepr =
      new FiberSectionRepr(secTag, maxNumPatches, maxNumReinfLayers);

  if (builder->addSectionRepres(*fiberSectionRepr) < 0) {
    opserr << G3_ERROR_PROMPT << "- cannot add section representation\n";
    return TCL_ERROR;
  }

  int brace = 3; // Start of recursive parse

  double GJ;
  UniaxialMaterial *torsion = 0;
  bool deleteTorsion = false;
  currentSectionComputeCentroid = true;
  int iarg = brace;
  while (iarg < argc) {
    if (strcmp(argv[iarg], "-noCentroid") == 0) {
      currentSectionComputeCentroid = false;
      brace += 1;
    }

    if (strcmp(argv[iarg], "-GJ") == 0 && iarg + 1 < argc) {
      if (Tcl_GetDouble(interp, argv[brace + 1], &GJ) != TCL_OK) {
        opserr << G3_ERROR_PROMPT << "invalid GJ";
        return TCL_ERROR;
      }
      deleteTorsion = true;
      torsion = new ElasticMaterial(0, GJ);

      brace += 2;
    }

    if (strcmp(argv[iarg], "-torsion") == 0 && iarg + 1 < argc) {
      int torsionTag = 0;
      if (Tcl_GetInt(interp, argv[brace + 1], &torsionTag) != TCL_OK) {
        opserr << G3_ERROR_PROMPT << "invalid torsionTag";
        return TCL_ERROR;
      }

      torsion = builder->getUniaxialMaterial(torsionTag);
      if (torsion == 0) {
        opserr << G3_ERROR_PROMPT << "uniaxial material does not exist\n";
        opserr << "uniaxial material: " << torsionTag;
        opserr << "\nFiberSection3d: " << secTag << endln;
        return TCL_ERROR;
      }

      brace += 2;
    }

    iarg += 1;
  }

  if (torsion == 0 && NDM == 3) {
    opserr << G3_ERROR_PROMPT << "- no torsion specified for 3D fiber section, use -GJ or "
              "-torsion\n";
    opserr << "\nFiberSection3d: " << secTag << endln;
    return TCL_ERROR;
  }


  // Tcl_CreateCommand(interp, "patch", &TclCommand_addPatch, (ClientData)fiberSectionRepr, nullptr);

  // parse the information inside the braces (patches and reinforcing layers)
  if (Tcl_Eval(interp, argv[brace]) != TCL_OK) {
    opserr << G3_ERROR_PROMPT << "- error reading information in { } \n";
    return TCL_ERROR;
  }

  // build the fiber section (for analysis)
  if (buildSection(clientData, interp, theTclBasicBuilder, secTag, *torsion) != TCL_OK) {
    opserr << G3_ERROR_PROMPT << "- error constructing the section\n";
    return TCL_ERROR;
  }

  //    currentSectionTag = 0;

  if (deleteTorsion)
    delete torsion;

  return TCL_OK;
}

int
TclCommand_addFiberIntSection(ClientData clientData, Tcl_Interp *interp,
                              int argc, TCL_Char ** const argv,
                              TclBasicBuilder *theTclBasicBuilder)
{
  G3_Runtime *rt = G3_getRuntime(interp);
  int secTag;
  int maxNumPatches = 30;
  int maxNumReinfLayers = 30;
  int NDM = G3_getNDM(rt);

  if (argc < 4)
    // TODO(cmp): Print error message here
    return TCL_ERROR;

  if (Tcl_GetInt(interp, argv[2], &secTag) != TCL_OK) {
    opserr << G3_ERROR_PROMPT << "bad command - want: \nsection fiberSec secTag -GJ <GJ> { "
              "\n\tpatch <patch arguments> \n\tlayer <layer arguments> \n}\n";
    return TCL_ERROR;
  }

  theTclBasicBuilder->currentSectionTag = secTag;

  // create the fiber section representation (with the geometric information)

  SectionRepres *fiberSectionRepr =
      new FiberSectionRepr(secTag, maxNumPatches, maxNumReinfLayers);

  if (theTclBasicBuilder->addSectionRepres(*fiberSectionRepr) < 0) {
    opserr << G3_ERROR_PROMPT << "- cannot add section representation\n";
    return TCL_ERROR;
  }

  int brace = 3; // Start of recursive parse
  double GJ = 1.0;
  bool deleteTorsion = false;
  UniaxialMaterial *torsion = 0;
  if (strcmp(argv[3], "-GJ") == 0) {
    if (Tcl_GetDouble(interp, argv[4], &GJ) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid GJ";
      return TCL_ERROR;
    }
    torsion = new ElasticMaterial(0, GJ); // Is this gonna be a memory leak? MHS

    brace = 5;
  }
  int torsionTag = 0;
  if (strcmp(argv[3], "-torsion") == 0) {
    if (Tcl_GetInt(interp, argv[4], &torsionTag) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid torsionTag";
      return TCL_ERROR;
    }

    torsion = G3_getUniaxialMaterialInstance(rt,torsionTag);
    if (torsion == 0) {
      opserr << G3_ERROR_PROMPT << "uniaxial material does not exist\n";
      opserr << "uniaxial material: " << torsionTag;
      opserr << "\nFiberSection3d: " << secTag << endln;
      return TCL_ERROR;
    }

    brace = 5;
  }

  int NStrip1, NStrip2, NStrip3;
  double t1, t2, t3;

  if (strcmp(argv[3], "-NStrip") == 0) {

    if (Tcl_GetInt(interp, argv[4], &NStrip1) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid NStrip1";
      return TCL_ERROR;
    }

    if (Tcl_GetDouble(interp, argv[5], &t1) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid t1";
      return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[6], &NStrip2) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid NStrip2";
      return TCL_ERROR;
    }

    if (Tcl_GetDouble(interp, argv[7], &t2) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid t2";
      return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[8], &NStrip3) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid NStrip3";
      return TCL_ERROR;
    }

    if (Tcl_GetDouble(interp, argv[9], &t3) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid t3";
      return TCL_ERROR;
    }

    brace = 10; // may be 5
  }

  // parse the information inside the braces (patches and reinforcing layers)
  if (Tcl_Eval(interp, argv[brace]) != TCL_OK) {
    opserr << G3_ERROR_PROMPT << "- error reading information in { } \n";
    return TCL_ERROR;
  }

  if (NDM == 3 && torsion == 0) {
    opserr << G3_ERROR_PROMPT << "- no torsion specified for 3D fiber section, use -GJ or "
              "-torsion\n";
    opserr << "\nFiberSectionInt3d: " << secTag << endln;
    return TCL_ERROR;
  }

  // build the fiber section (for analysis)
  if (buildSectionInt(clientData, interp, theTclBasicBuilder, secTag, *torsion, NStrip1, t1,
                      NStrip2, t2, NStrip3, t3) != TCL_OK) {
    opserr << G3_ERROR_PROMPT << "- error constructing the section\n";
    return TCL_ERROR;
  }

  //    currentSectionTag = 0;

  if (deleteTorsion)
    delete torsion;

  return TCL_OK;
}

//
// add patch to fiber section
//
int
TclCommand_addPatch(ClientData clientData, 
                    Tcl_Interp *interp, 
                    int argc,
                    TCL_Char ** const argv)
{
  G3_Runtime *rt = G3_getRuntime(interp);
  TclBuilder *theTclBasicBuilder = G3_getModelBuilder(rt);

  // check if a section is being processed
  if ((theTclBasicBuilder->currentSectionTag == -1) && (clientData==NULL)) {
    opserr << G3_ERROR_PROMPT << "subcommand 'patch' is only valid inside a 'section' "
              "command\n";
    return TCL_ERROR;
  }

  // make sure at least one other argument to contain patch type
  if (argc < 2) {
    opserr << G3_ERROR_PROMPT << "need to specify a patch type \n";
    return TCL_ERROR;
  }

  // check argv[1] for type of patch  and create the object
  if (strcmp(argv[1], "quad") == 0 || strcmp(argv[1], "quadr") == 0) {
    int numSubdivIJ, numSubdivJK, matTag, secTag;
    double vertexCoordY, vertexCoordZ;
    static Matrix vertexCoords(4, 2);
    int j, argi;

    if (argc < 13) {
      opserr << G3_ERROR_PROMPT << "invalid number of parameters: patch quad matTag "
                "numSubdivIJ numSubdivJK yVertI zVertI yVertJ zVertJ yVertK "
                "zVertK yVertL zVertL\n";
      return TCL_ERROR;
    }

    argi = 2;

    if (Tcl_GetInt(interp, argv[argi++], &matTag) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid matTag: patch quad matTag numSubdivIJ "
                "numSubdivJK yVertI zVertI yVertJ zVertJ yVertK zVertK yVertL "
                "zVertL\n";
      return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[argi++], &numSubdivIJ) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid numSubdivIJ: patch quad matTag numSubdivIJ "
                "numSubdivJK yVertI zVertI yVertJ zVertJ yVertK zVertK yVertL "
                "zVertL\n";
      return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[argi++], &numSubdivJK) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid numSubdivJK: patch quad matTag numSubdivIJ "
                "numSubdivJK yVertI zVertI yVertJ zVertJ yVertK zVertK yVertL "
                "zVertL\n";
      return TCL_ERROR;
    }

    for (j = 0; j < 4; j++) {
      if (Tcl_GetDouble(interp, argv[argi++], &vertexCoordY) != TCL_OK) {
        opserr << G3_ERROR_PROMPT << "invalid Coordinate y: ...yVertI zVertI yVertJ "
                  "zVertJ yVertK zVertK yVertL zVertL\n";
        return TCL_ERROR;
      }

      if (Tcl_GetDouble(interp, argv[argi++], &vertexCoordZ) != TCL_OK) {
        opserr << G3_ERROR_PROMPT << "invalid Coordinate z: ...yVertI zVertI yVertJ "
                  "zVertJ yVertK zVertK yVertL zVertL\n";
        return TCL_ERROR;
      }

      vertexCoords(j, 0) = vertexCoordY;
      vertexCoords(j, 1) = vertexCoordZ;
    }

    // get section representation
    secTag = theTclBasicBuilder->currentSectionTag;

    SectionRepres *sectionRepres = theTclBasicBuilder->getSectionRepres(secTag);
    if (sectionRepres == 0) {
      opserr << G3_ERROR_PROMPT << "cannot retrieve section\n";
      return TCL_ERROR;
    }

    if (sectionRepres->getType() != SEC_TAG_FiberSection) {
      opserr << G3_ERROR_PROMPT << "section invalid: patch can only be added to fiber "
                "sections\n";
      return TCL_ERROR;
    }

    FiberSectionRepr *fiberSectionRepr = (FiberSectionRepr *)sectionRepres;

    // create patch

    QuadPatch *patch =
        new QuadPatch(matTag, numSubdivIJ, numSubdivJK, vertexCoords);
    if (!patch) {
      opserr << G3_ERROR_PROMPT << "cannot allocate patch\n";
      return TCL_ERROR;
    }

    // add patch to section representation

    int error = fiberSectionRepr->addPatch(*patch);
    delete patch;

    if (error) {
      opserr << G3_ERROR_PROMPT << "cannot add patch to section\n";
      return TCL_ERROR;
    }
  }

  // check argv[1] for type of patch  and create the object
  else if (strcmp(argv[1], "rect") == 0 ||
           strcmp(argv[1], "rectangular") == 0) {

    int numSubdivIJ, numSubdivJK, matTag, secTag;
    double vertexCoordY, vertexCoordZ;
    static Matrix vertexCoords(4, 2);
    int j, argi;

    if (argc < 9) {
      opserr << G3_ERROR_PROMPT << "invalid number of parameters: patch quad matTag "
                "numSubdivIJ numSubdivJK yVertI zVertI yVertK zVertK\n";
      return TCL_ERROR;
    }

    argi = 2;

    if (Tcl_GetInt(interp, argv[argi++], &matTag) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid matTag: patch quad matTag numSubdivIJ "
                "numSubdivJK yVertI zVertI yVertJ zVertJ yVertK zVertK yVertL "
                "zVertL\n";
      return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[argi++], &numSubdivIJ) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid numSubdivIJ: patch quad matTag numSubdivIJ "
                "numSubdivJK yVertI zVertI yVertJ zVertJ yVertK zVertK yVertL "
                "zVertL\n";
      return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[argi++], &numSubdivJK) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid numSubdivJK: patch quad matTag numSubdivIJ "
                "numSubdivJK yVertI zVertI yVertJ zVertJ yVertK zVertK yVertL "
                "zVertL\n";
      return TCL_ERROR;
    }

    for (j = 0; j < 2; j++) {
      if (Tcl_GetDouble(interp, argv[argi++], &vertexCoordY) != TCL_OK) {
        opserr << G3_ERROR_PROMPT << "invalid Coordinate y: ...yVertI zVertI yVertJ "
                  "zVertJ yVertK zVertK yVertL zVertL\n";
        return TCL_ERROR;
      }

      if (Tcl_GetDouble(interp, argv[argi++], &vertexCoordZ) != TCL_OK) {
        opserr << G3_ERROR_PROMPT << "invalid Coordinate z: ...yVertI zVertI yVertJ "
                  "zVertJ yVertK zVertK yVertL zVertL\n";
        return TCL_ERROR;
      }

      vertexCoords(j * 2, 0) = vertexCoordY;
      vertexCoords(j * 2, 1) = vertexCoordZ;
    }

    vertexCoords(1, 0) = vertexCoords(2, 0);
    vertexCoords(1, 1) = vertexCoords(0, 1);
    vertexCoords(3, 0) = vertexCoords(0, 0);
    vertexCoords(3, 1) = vertexCoords(2, 1);

    // get section representation
    secTag = theTclBasicBuilder->currentSectionTag;

    SectionRepres *sectionRepres = theTclBasicBuilder->getSectionRepres(secTag);
    if (sectionRepres == 0) {
      opserr << G3_ERROR_PROMPT << "cannot retrieve section\n";
      return TCL_ERROR;
    }

    if (sectionRepres->getType() != SEC_TAG_FiberSection) {
      opserr << G3_ERROR_PROMPT << "section invalid: patch can only be added to fiber "
                "sections\n";
      return TCL_ERROR;
    }

    FiberSectionRepr *fiberSectionRepr = (FiberSectionRepr *)sectionRepres;

    // create patch

    QuadPatch *patch =
        new QuadPatch(matTag, numSubdivIJ, numSubdivJK, vertexCoords);
    if (!patch) {
      opserr << G3_ERROR_PROMPT << "cannot allocate patch\n";
      return TCL_ERROR;
    }


    // add patch to section representation

    int error = fiberSectionRepr->addPatch(*patch);
    delete patch;

    if (error) {
      opserr << G3_ERROR_PROMPT << "cannot add patch to section\n";
      return TCL_ERROR;
    }
  }

  else if (strcmp(argv[1], "circ") == 0) {
    int numSubdivRad, numSubdivCirc, matTag, secTag;
    double yCenter, zCenter;
    static Vector centerPosition(2);
    double intRad, extRad;
    double startAng, endAng;

    int argi;

    argi = 2;
    if (argc < 11) {
      opserr << G3_ERROR_PROMPT << "invalid number of parameters: patch circ matTag "
                "numSubdivCirc numSubdivRad yCenter zCenter intRad extRad "
                "startAng endAng\n";
      return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[argi++], &matTag) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid matTag: patch circ matTag numSubdivCirc "
                "numSubdivRad yCenter zCenter intRad extRad startAng endAng\n";
      return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[argi++], &numSubdivCirc) != TCL_OK) {
      opserr
          << G3_ERROR_PROMPT << "invalid numSubdivCirc: patch circ matTag numSubdivCirc "
             "numSubdivRad yCenter zCenter intRad extRad startAng endAng\n";
      return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[argi++], &numSubdivRad) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid numSubdivRad: patch circ matTag numSubdivCirc "
                "numSubdivRad yCenter zCenter intRad extRad startAng endAng\n";
      return TCL_ERROR;
    }

    if (Tcl_GetDouble(interp, argv[argi++], &yCenter) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid yCenter: patch circ matTag numSubdivCirc "
                "numSubdivRad yCenter zCenter intRad extRad startAng endAng\n";
      return TCL_ERROR;
    }

    if (Tcl_GetDouble(interp, argv[argi++], &zCenter) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid zCenter: patch circ matTag numSubdivCirc "
                "numSubdivRad yCenter zCenter intRad extRad startAng endAng\n";
      return TCL_ERROR;
    }

    if (Tcl_GetDouble(interp, argv[argi++], &intRad) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid intRad: patch circ matTag numSubdivCirc "
                "numSubdivRad yCenter zCenter intRad extRad startAng endAng\n";
      return TCL_ERROR;
    }

    if (Tcl_GetDouble(interp, argv[argi++], &extRad) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid extRad: patch circ matTag numSubdivCirc "
                "numSubdivRad yCenter zCenter intRad extRad startAng endAng\n";
      return TCL_ERROR;
    }

    if (Tcl_GetDouble(interp, argv[argi++], &startAng) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid startAng: patch circ matTag numSubdivCirc "
                "numSubdivRad yCenter zCenter intRad extRad startAng endAng\n";
      return TCL_ERROR;
    }

    if (Tcl_GetDouble(interp, argv[argi++], &endAng) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid endAng: patch circ matTag numSubdivCirc "
                "numSubdivRad yCenter zCenter intRad extRad startAng endAng\n";
      return TCL_ERROR;
    }

    // get section
    secTag = theTclBasicBuilder->currentSectionTag;

    SectionRepres *sectionRepres = theTclBasicBuilder->getSectionRepres(secTag);
    if (sectionRepres == 0) {
      opserr << G3_ERROR_PROMPT << "cannot retrieve section\n";
      return TCL_ERROR;
    }

    if (sectionRepres->getType() != SEC_TAG_FiberSection) {
      opserr << G3_ERROR_PROMPT << "section invalid: patch can only be added to fiber "
                "sections\n";
      return TCL_ERROR;
    }

    FiberSectionRepr *fiberSectionRepr = (FiberSectionRepr *)sectionRepres;

    centerPosition(0) = yCenter;
    centerPosition(1) = zCenter;

    // create patch

    CircPatch *patch =
        new CircPatch(matTag, numSubdivCirc, numSubdivRad, centerPosition,
                      intRad, extRad, startAng, endAng);
    if (!patch) {
      opserr << G3_ERROR_PROMPT << "cannot allocate patch\n";
      return TCL_ERROR;
    }


    // add patch to section

    int error = fiberSectionRepr->addPatch(*patch);
    delete patch;

    if (error) {
      opserr << G3_ERROR_PROMPT << "cannot add patch to section\n";
      return TCL_ERROR;
    }
  }

  else {
    opserr << G3_ERROR_PROMPT << "patch type is not available\n";
    return TCL_ERROR;
  }

  return TCL_OK;
}

// add patch to fiber section
int
TclCommand_addFiber(ClientData clientData, Tcl_Interp *interp, int argc,
                    TCL_Char ** const argv)
{
  G3_Runtime *rt = G3_getRuntime(interp);
  TclBuilder *theTclBasicBuilder = G3_getModelBuilder(rt);
  assert(clientData != nullptr);
  BasicModelBuilder* builder = (BasicModelBuilder*)clientData;

  // check if a section is being processed
  if (theTclBasicBuilder->currentSectionTag == 0) {
    opserr << G3_ERROR_PROMPT << "subcommand 'fiber' is only valid inside a 'section' "
              "command\n";
    return TCL_ERROR;
  }

  // make sure at least one other argument to contain patch type
  if (argc < 5) {
    opserr << G3_ERROR_PROMPT << "invalid num args: fiber yLoc zLoc area matTag\n";
    return TCL_ERROR;
  }

  SectionRepres *sectionRepres =
      theTclBasicBuilder->getSectionRepres(theTclBasicBuilder->currentSectionTag);

  if (sectionRepres == nullptr) {
    opserr << G3_ERROR_PROMPT << "cannot retrieve section\n";
    return TCL_ERROR;
  }

  if (sectionRepres->getType() != SEC_TAG_FiberSection) {
    opserr << G3_ERROR_PROMPT << "section invalid: fiber can only be added to fiber "
              "sections\n";
    return TCL_ERROR;
  }

  FiberSectionRepr *fiberSectionRepr = (FiberSectionRepr *)sectionRepres;
  int numFibers = fiberSectionRepr->getNumFibers();

  Fiber *theFiber = 0;
  int matTag;
  double yLoc, zLoc, area;
  int NDM = builder->getNDM();

  if (Tcl_GetDouble(interp, argv[1], &yLoc) != TCL_OK) {
    opserr << G3_ERROR_PROMPT << "invalid yLoc: fiber yLoc zLoc area matTag\n";
    return TCL_ERROR;
  }
  if (Tcl_GetDouble(interp, argv[2], &zLoc) != TCL_OK) {
    opserr << G3_ERROR_PROMPT << "invalid zLoc: fiber yLoc zLoc area matTag\n";
    return TCL_ERROR;
  }
  if (Tcl_GetDouble(interp, argv[3], &area) != TCL_OK) {
    opserr << G3_ERROR_PROMPT << "invalid area: fiber yLoc zLoc area matTag\n";
    return TCL_ERROR;
  }
  if (Tcl_GetInt(interp, argv[4], &matTag) != TCL_OK) {
    opserr << G3_ERROR_PROMPT << "invalid matTag: fiber yLoc zLoc area matTag\n";
    return TCL_ERROR;
  }

  // creates 2d section
  if (NDM == 2) {
    if (currentSectionIsND) {
      NDMaterial *material = builder->getNDMaterial(matTag);
      if (material == 0) {
        opserr << G3_ERROR_PROMPT << "invalid NDMaterial ID for patch\n";
        return TCL_ERROR;
      }
      theFiber = new NDFiber2d(numFibers, *material, area, yLoc);
    } else {
      UniaxialMaterial *material = G3_getUniaxialMaterialInstance(rt, matTag);
      if (material == 0) {
        opserr << G3_ERROR_PROMPT << "invalid UniaxialMaterial ID for patch\n";
        return TCL_ERROR;
      }
      theFiber = new UniaxialFiber2d(numFibers, *material, area, yLoc);
    }

    if (theFiber == 0) {
      opserr << G3_ERROR_PROMPT << "unable to allocate fiber \n";
      return TCL_ERROR;
    }
  }

  else if (NDM == 3) {
    static Vector fiberPosition(2);
    fiberPosition(0) = yLoc;
    fiberPosition(1) = zLoc;

    if (currentSectionIsND) {
      NDMaterial *material = builder->getNDMaterial(matTag);
      if (material == 0) {
        opserr << G3_ERROR_PROMPT << "invalid NDMaterial ID for patch\n";
        return TCL_ERROR;
      }
      theFiber = new NDFiber3d(numFibers, *material, area, yLoc, zLoc);
    } else {
      UniaxialMaterial *material = G3_getUniaxialMaterialInstance(rt,matTag);
      if (material == 0) {
        opserr << G3_ERROR_PROMPT << "invalid UniaxialMaterial ID for patch\n";
        return TCL_ERROR;
      }
      theFiber = new UniaxialFiber3d(numFibers, *material, area, fiberPosition);
    }

    if (theFiber == 0) {
      opserr << G3_ERROR_PROMPT << "unable to allocate fiber \n";
      return TCL_ERROR;
    }
  }

  else {
    opserr << G3_ERROR_PROMPT << "fiber command for FiberSection only for 2 or 3d \n";
    return TCL_ERROR;
  }

  // add patch to section representation
  int error = fiberSectionRepr->addFiber(*theFiber);

  if (error) {
    opserr << G3_ERROR_PROMPT << "cannot add patch to section\n";
    return TCL_ERROR;
  }

  return TCL_OK;
}

// add Hfiber to fiber section
int
TclCommand_addHFiber(ClientData clientData, Tcl_Interp *interp, int argc,
                     TCL_Char ** const argv, TclBasicBuilder *theTclBasicBuilder)
{
  assert(clientData != nullptr);
  BasicModelBuilder* builder = (BasicModelBuilder*)clientData;

  // check if a section is being processed
  if (theTclBasicBuilder->currentSectionTag == 0) {
    opserr << G3_ERROR_PROMPT << "subcommand 'Hfiber' is only valid inside a 'section' "
              "command\n";
    return TCL_ERROR;
  }

  // make sure at least one other argument to contain patch type
  if (argc < 5) {
    opserr << G3_ERROR_PROMPT << "invalid num args: Hfiber yLoc zLoc area matTag\n";
    return TCL_ERROR;
  }

  SectionRepres *sectionHRepres =
      theTclBasicBuilder->getSectionRepres(theTclBasicBuilder->currentSectionTag);

  if (sectionHRepres == 0) {
    opserr << G3_ERROR_PROMPT << "cannot retrieve section\n";
    return TCL_ERROR;
  }

  if (sectionHRepres->getType() != SEC_TAG_FiberSection) {
    opserr << G3_ERROR_PROMPT << "section invalid: patch can only be added to fiber "
              "sections\n";
    return TCL_ERROR;
  }

  FiberSectionRepr *fiberSectionHRepr = (FiberSectionRepr *)sectionHRepres;
  int numHFibers = fiberSectionHRepr->getNumHFibers();

  Fiber *theHFiber = 0;
  int matHTag;
  double yHLoc, zHLoc, Harea;
  int HNDM = builder->getNDM();

  if (Tcl_GetDouble(interp, argv[1], &yHLoc) != TCL_OK) {
    opserr << G3_ERROR_PROMPT << "invalid yLoc: Hfiber yLoc zLoc area matTag\n";
    return TCL_ERROR;
  }
  if (Tcl_GetDouble(interp, argv[2], &zHLoc) != TCL_OK) {
    opserr << G3_ERROR_PROMPT << "invalid zLoc: Hfiber yLoc zLoc area matTag\n";
    return TCL_ERROR;
  }
  if (Tcl_GetDouble(interp, argv[3], &Harea) != TCL_OK) {
    opserr << G3_ERROR_PROMPT << "invalid area: Hfiber yLoc zLoc area matTag\n";
    return TCL_ERROR;
  }

  if (Tcl_GetInt(interp, argv[4], &matHTag) != TCL_OK) {
    opserr << G3_ERROR_PROMPT << "invalid matTag: Hfiber yLoc zLoc area matTag\n";
    return TCL_ERROR;
  }

  UniaxialMaterial *Hmaterial = builder->getUniaxialMaterial(matHTag);

  // creates 2d section
  if (HNDM == 2) {

    if (Hmaterial == 0) {
      opserr << G3_ERROR_PROMPT << "invalid Hmaterial ID for patch\n";
      return TCL_ERROR;
    }

    theHFiber = new UniaxialFiber2d(numHFibers, *Hmaterial, Harea, yHLoc);
    if (theHFiber == 0) {
      opserr << G3_ERROR_PROMPT << "unable to allocate Hfiber \n";
      return TCL_ERROR;
    }
  }

  else if (HNDM == 3) {

    static Vector fiberHPosition(2);
    fiberHPosition(0) = yHLoc;
    fiberHPosition(1) = zHLoc;

    theHFiber =
        new UniaxialFiber3d(numHFibers, *Hmaterial, Harea, fiberHPosition);
    if (theHFiber == 0) {
      opserr << G3_ERROR_PROMPT << "unable to allocate Hfiber \n";
      return TCL_ERROR;
    }
  }

  else {
    opserr << G3_ERROR_PROMPT << "Hfiber command for FiberSection only fo 2 or 3d \n";
    return TCL_ERROR;
  }

  // add patch to section representation
  int error = fiberSectionHRepr->addHFiber(*theHFiber);

  if (error) {
    opserr << G3_ERROR_PROMPT << "cannot add patch to section\n";
    return TCL_ERROR;
  }

  return TCL_OK;
}

// add layers of reinforcing bars to fiber section

int
TclCommand_addReinfLayer(ClientData clientData, Tcl_Interp *interp, int argc,
                         TCL_Char ** const argv) //, TclBasicBuilder *theTclBasicBuilder)
{
  assert(clientData != nullptr);

  G3_Runtime *rt = G3_getRuntime(interp);
  TclBasicBuilder *theTclBasicBuilder = (TclBasicBuilder*)G3_getModelBuilder(rt);

  // check if a section is being processed
  if (theTclBasicBuilder->currentSectionTag == 0) {
    opserr << G3_ERROR_PROMPT << "subcommand 'patch' is only valid inside a 'section' "
              "command\n";
    return TCL_ERROR;
  }

  // make sure at least one other argument to contain layer type
  if (argc < 2) {
    opserr << G3_ERROR_PROMPT << "need to specify a layer type \n";
    return TCL_ERROR;
  }

  // check argv[1] for type of layer and create the object
  if (strcmp(argv[1], "straight") == 0 ||
      strcmp(argv[1], "line")     == 0) {
    if (argc < 9) {
      opserr << G3_ERROR_PROMPT << "invalid number of parameters: layer straight matTag "
                "numReinfBars reinfBarArea yStartPt zStartPt yEndPt zEndPt\n";
      return TCL_ERROR;
    }

    int secTag, matTag, numReinfBars;
    double reinfBarArea;
    double yStartPt, zStartPt, yEndPt, zEndPt;

    int argi;

    argi = 2;

    if (Tcl_GetInt(interp, argv[argi++], &matTag) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid matTag: layer straight matTag numReinfBars "
                "reinfBarArea  yStartPt zStartPt yEndPt zEndPt\n";
      return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[argi++], &numReinfBars) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid numReinfBars: layer straight matTag "
                "numReinfBars reinfBarArea  yStartPt zStartPt yEndPt zEndPt\n";
      return TCL_ERROR;
    }

    if (Tcl_GetDouble(interp, argv[argi++], &reinfBarArea) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid reinfBarArea: layer straight matTag "
                "numReinfBars reinfBarArea  yStartPt zStartPt yEndPt zEndPt\n";
      return TCL_ERROR;
    }

    if (Tcl_GetDouble(interp, argv[argi++], &yStartPt) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid yStartPt: layer straight matTag numReinfBars "
                "reinfBarArea  yStartPt zStartPt yEndPt zEndPt\n";
      return TCL_ERROR;
    }

    if (Tcl_GetDouble(interp, argv[argi++], &zStartPt) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid zStartPt: layer straight matTag numReinfBars "
                "reinfBarArea  yStartPt zStartPt yEndPt zEndPt\n";
      return TCL_ERROR;
    }

    if (Tcl_GetDouble(interp, argv[argi++], &yEndPt) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid yEndPt: layer straight matTag numReinfBars "
                "reinfBarArea  yStartPt zStartPt yEndPt zEndPt\n";
      return TCL_ERROR;
    }

    if (Tcl_GetDouble(interp, argv[argi++], &zEndPt) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid zEndPt: layer straight matTag numReinfBars "
                "reinfBarArea  yStartPt zStartPt yEndPt zEndPt\n";
      return TCL_ERROR;
    }


    // get section
    secTag = theTclBasicBuilder->currentSectionTag;

    SectionRepres *sectionRepres = theTclBasicBuilder->getSectionRepres(secTag);
    if (sectionRepres == 0) {
      opserr << G3_ERROR_PROMPT << "cannot retrieve section\n";
      return TCL_ERROR;
    }

    if (sectionRepres->getType() != SEC_TAG_FiberSection) {
      opserr << G3_ERROR_PROMPT << "section invalid: patch can only be added to fiber "
                "sections\n";
      return TCL_ERROR;
    }

    FiberSectionRepr *fiberSectionRepr = (FiberSectionRepr *)sectionRepres;

    // create the reinforcing layer

    static Vector startPt(2);
    static Vector endPt(2);

    startPt(0) = yStartPt;
    startPt(1) = zStartPt;
    endPt(0) = yEndPt;
    endPt(1) = zEndPt;

    StraightReinfLayer *reinfLayer = new StraightReinfLayer(
        matTag, numReinfBars, reinfBarArea, startPt, endPt);
    if (!reinfLayer) {
      opserr << G3_ERROR_PROMPT << "cannot allocate reinfLayer\n";
      return TCL_ERROR;
    }

    // add reinfLayer to section
    int error = fiberSectionRepr->addReinfLayer(*reinfLayer);
    delete reinfLayer;

    if (error) {
      opserr << G3_ERROR_PROMPT << "cannot add reinforcing layer to section\n";
      return TCL_ERROR;
    }

  } else if (strcmp(argv[1], "circ") == 0) {
    if (argc < 8) {
      opserr << G3_ERROR_PROMPT << "invalid number of parameters: layer circ matTag "
                "numReinfBars reinfBarArea yCenter zCenter arcRadius <startAng "
                "endAng>\n";
      return TCL_ERROR;
    }

    int secTag, matTag, numReinfBars;
    double reinfBarArea;
    double yCenter, zCenter, radius, startAng, endAng;

    int argi;

    argi = 2;

    if (Tcl_GetInt(interp, argv[argi++], &matTag) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid matTag: layer circ matTag numReinfBars "
                "reinfBarArea yCenter zCenter radius startAng endAng\n";
      return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[argi++], &numReinfBars) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid numReinfBars: layer circ matTag numReinfBars "
                "reinfBarArea yCenter zCenter radius startAng endAng\n";
      return TCL_ERROR;
    }

    if (Tcl_GetDouble(interp, argv[argi++], &reinfBarArea) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid reinfBarArea: layer circ matTag numReinfBars "
                "reinfBarArea yCenter zCenter radius startAng endAng\n";
      return TCL_ERROR;
    }

    if (Tcl_GetDouble(interp, argv[argi++], &yCenter) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid yCenter: layer circ matTag numReinfBars "
                "reinfBarArea yCenter zCenter radius startAng endAng\n";
      return TCL_ERROR;
    }

    if (Tcl_GetDouble(interp, argv[argi++], &zCenter) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid zCenter: layer circ matTag numReinfBars "
                "reinfBarArea yCenter zCenter radius startAng endAng\n";
      return TCL_ERROR;
    }

    if (Tcl_GetDouble(interp, argv[argi++], &radius) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid radius: layer circ matTag numReinfBars "
                "reinfBarArea yCenter zCenter radius startAng endAng\n";
      return TCL_ERROR;
    }

    bool anglesSpecified = false;

    if (argc > 9) {
      if (Tcl_GetDouble(interp, argv[argi++], &startAng) != TCL_OK) {
        opserr << G3_ERROR_PROMPT << "invalid startAng: layer circ matTag numReinfBars "
                  "reinfBarArea yCenter zCenter radius startAng endAng\n";
        return TCL_ERROR;
      }

      if (Tcl_GetDouble(interp, argv[argi++], &endAng) != TCL_OK) {
        opserr << G3_ERROR_PROMPT << "invalid endAng: layer circ matTag numReinfBars "
                  "reinfBarArea yCenter zCenter radius startAng endAng\n";
        return TCL_ERROR;
      }

      anglesSpecified = true;
    }

    // get section
    secTag = theTclBasicBuilder->currentSectionTag;

    SectionRepres *sectionRepres = theTclBasicBuilder->getSectionRepres(secTag);
    if (sectionRepres == 0) {
      opserr << G3_ERROR_PROMPT << "cannot retrieve section\n";
      return TCL_ERROR;
    }

    if (sectionRepres->getType() != SEC_TAG_FiberSection) {
      opserr << G3_ERROR_PROMPT << "section invalid: patch can only be added to fiber "
                "sections\n";
      return TCL_ERROR;
    }

    FiberSectionRepr *fiberSectionRepr = (FiberSectionRepr *)sectionRepres;

    // create the reinforcing layer

    static Vector center(2);

    center(0) = yCenter;
    center(1) = zCenter;

    CircReinfLayer *reinfLayer = 0;
    if (anglesSpecified)
      // Construct arc
      reinfLayer = new CircReinfLayer(matTag, numReinfBars, reinfBarArea,
                                      center, radius, startAng, endAng);
    else
      // Construct circle
      reinfLayer = new CircReinfLayer(matTag, numReinfBars, reinfBarArea,
                                      center, radius);

    if (!reinfLayer) {
      opserr << G3_ERROR_PROMPT << "cannot allocate reinfLayer\n";
      return TCL_ERROR;
    }

    // add reinfLayer to section
    int error = fiberSectionRepr->addReinfLayer(*reinfLayer);
    delete reinfLayer;

    if (error) {
      opserr << G3_ERROR_PROMPT << "cannot add reinforcing layer to section\n";
      return TCL_ERROR;
    }

  } else {
    opserr << G3_ERROR_PROMPT << "reinforcing layer type is not available\n";
    return TCL_ERROR;
  }

  return TCL_OK;
}

// build the section
static int
buildSection(ClientData clientData, Tcl_Interp *interp, TclBasicBuilder *theTclBasicBuilder,
             int secTag, UniaxialMaterial &theTorsion)
{
  assert(clientData != nullptr);
  BasicModelBuilder *builder = (BasicModelBuilder*)clientData;
//G3_Runtime *rt = G3_getRuntime(interp);

  SectionRepres *sectionRepres = theTclBasicBuilder->getSectionRepres(secTag);

  if (sectionRepres == nullptr) {
    opserr << G3_ERROR_PROMPT << "cannot retrieve section\n";
    return TCL_ERROR;
  }

  if (sectionRepres->getType() == SEC_TAG_FiberSection) {
    // build the section
    FiberSectionRepr *fiberSectionRepr = (FiberSectionRepr *)sectionRepres;


    int numPatches = fiberSectionRepr->getNumPatches();
    Patch **patch  = fiberSectionRepr->getPatches();

    int numReinfLayers      = fiberSectionRepr->getNumReinfLayers();
    ReinfLayer **reinfLayer = fiberSectionRepr->getReinfLayers();

    int numSectionRepresFibers  = fiberSectionRepr->getNumFibers();
    Fiber **sectionRepresFibers = fiberSectionRepr->getFibers();


    int i, j, k;
    int numFibers;
    numFibers = numSectionRepresFibers;
    for (i = 0; i < numPatches; i++)
      numFibers += patch[i]->getNumCells();

    for (i = 0; i < numReinfLayers; i++)
      numFibers += reinfLayer[i]->getNumReinfBars();


    static Vector fiberPosition(2);
    int matTag;

    ID fibersMaterial(numFibers - numSectionRepresFibers);
    Matrix fibersPosition(2, numFibers - numSectionRepresFibers);
    Vector fibersArea(numFibers - numSectionRepresFibers);

    int numCells;
    Cell **cell;

    k = 0;
    for (int i = 0; i < numPatches; i++) {

      numCells = patch[i]->getNumCells();
      matTag = patch[i]->getMaterialID();


      cell = patch[i]->getCells();

      for (int j = 0; j < numCells; j++) {
        fibersMaterial(k) = matTag;
        fibersArea(k) = cell[j]->getArea();
        fiberPosition = cell[j]->getCentroidPosition();

        fibersPosition(0, k) = fiberPosition(0);
        fibersPosition(1, k) = fiberPosition(1);

        k++;
      }

      for (int j = 0; j < numCells; j++)
        delete cell[j];

      delete[] cell;
    }

    ReinfBar *reinfBar;
    int numReinfBars;

    for (i = 0; i < numReinfLayers; i++) {
      numReinfBars = reinfLayer[i]->getNumReinfBars();
      reinfBar = reinfLayer[i]->getReinfBars();
      matTag = reinfLayer[i]->getMaterialID();

      for (j = 0; j < numReinfBars; j++) {
        fibersMaterial(k) = matTag;
        fibersArea(k) = reinfBar[j].getArea();
        fiberPosition = reinfBar[j].getPosition();

        fibersPosition(0, k) = fiberPosition(0);
        fibersPosition(1, k) = fiberPosition(1);

        k++;
      }
      delete[] reinfBar;
    }

    UniaxialMaterial *material;
    NDMaterial *ndmaterial;

    // dimension of the structure (1d, 2d, or 3d)
    int NDM = builder->getNDM();

    Fiber **fiber = new Fiber *[numFibers];

    // copy the section repres fibers
    for (i = 0; i < numSectionRepresFibers; i++)
      fiber[i] = sectionRepresFibers[i];

    // creates 2d section

    if (NDM == 2) {
      k = 0;
      for (i = numSectionRepresFibers; i < numFibers; i++) {
        if (currentSectionIsND) {
          ndmaterial = builder->getNDMaterial(fibersMaterial(k));
          if (ndmaterial == nullptr) {
            opserr << G3_ERROR_PROMPT << "invalid NDmaterial ID for patch\n";
            return TCL_ERROR;
          }
          fiber[i] = new NDFiber2d(k, *ndmaterial, fibersArea(k),
                                   fibersPosition(0, k));
        } else {
          material = builder->getUniaxialMaterial(fibersMaterial(k));
          if (material == 0) {
            opserr << G3_ERROR_PROMPT << "invalid UniaxialMaterial ID for patch\n";
            return TCL_ERROR;
          }
          fiber[i] = new UniaxialFiber2d(k, *material, fibersArea(k), fibersPosition(0, k));
        }

        k++;
      }

      SectionForceDeformation *section = 0;
      if (currentSectionIsND) {
        if (currentSectionIsWarping)
          section = new NDFiberSectionWarping2d(secTag, numFibers, fiber);
        else
          section = new NDFiberSection2d(secTag, numFibers, fiber,
                                         currentSectionComputeCentroid);
      } else
        section = new FiberSection2d(secTag, numFibers, fiber,
                                     currentSectionComputeCentroid);

      // Delete fibers
      for (i = 0; i < numFibers; i++)
        delete fiber[i];

      if (section == nullptr) {
        opserr << G3_ERROR_PROMPT << "cannot construct section\n";
        return TCL_ERROR;
      }

      if (theTclBasicBuilder->addSection(*section) < 0) {
      // if (OPS_addSectionForceDeformation(section) != true) {
        opserr << G3_ERROR_PROMPT << "cannot add section\n";
        return TCL_ERROR;
      }


    } else if (NDM == 3) {

      static Vector fiberPosition(2);
      k = 0;
      for (int i = numSectionRepresFibers; i < numFibers; i++) {
        fiberPosition(0) = fibersPosition(0, k);
        fiberPosition(1) = fibersPosition(1, k);
        if (currentSectionIsND) {
          ndmaterial = builder->getNDMaterial(fibersMaterial(k));
          if (ndmaterial == nullptr) {
            opserr << G3_ERROR_PROMPT << "invalid NDmaterial ID for patch\n";
            return TCL_ERROR;
          }
          fiber[i] = new NDFiber3d(k, *ndmaterial, fibersArea(k),
                                   fiberPosition(0), fiberPosition(1));
        } else {
          material = builder->getUniaxialMaterial(fibersMaterial(k));
          if (material == nullptr) {
            opserr << G3_ERROR_PROMPT << "invalid UniaxialMaterial ID for patch\n";
            return TCL_ERROR;
          }
          fiber[i] =
              new UniaxialFiber3d(k, *material, fibersArea(k), fiberPosition);
        }
        k++;
      }

      // SectionForceDeformation *section = new FiberSection(secTag, numFibers,
      // fiber);
      SectionForceDeformation *section = nullptr;
      if (currentSectionIsND)
        section = new NDFiberSection3d(secTag, numFibers, fiber,
                                       currentSectionComputeCentroid);
      else
        section = new FiberSection3d(secTag, numFibers, fiber, theTorsion,
                                     currentSectionComputeCentroid);

      // Delete fibers
      for (int i = 0; i < numFibers; i++)
        delete fiber[i];

      if (section == nullptr) {
        opserr << G3_ERROR_PROMPT << "- cannot construct section\n";
        return TCL_ERROR;
      }

      if (builder->addSection(*section) < 0) {
      // if (OPS_addSectionForceDeformation(section) != true) {
        opserr << G3_ERROR_PROMPT << "- cannot add section\n";
        return TCL_ERROR;
      }


    } else {
      opserr << G3_ERROR_PROMPT << "NDM = " << NDM
             << " is imcompatible with available frame elements\n";
      return TCL_ERROR;
    }

    // Delete fiber array
    delete[] fiber;

  } else {
    opserr << G3_ERROR_PROMPT << "section invalid: can only build fiber sections\n";
    return TCL_ERROR;
  }

  return TCL_OK;
}

// build the section Interaction
int
buildSectionInt(ClientData clientData, Tcl_Interp *interp, TclBasicBuilder *theTclBasicBuilder,
                int secTag, UniaxialMaterial &theTorsion, int NStrip1,
                double t1, int NStrip2, double t2, int NStrip3, double t3)
{
  assert(clientData != nullptr);
  BasicModelBuilder* builder = (BasicModelBuilder*)clientData;
  SectionRepres *sectionRepres = theTclBasicBuilder->getSectionRepres(secTag);

  if (sectionRepres == nullptr) {
    opserr << G3_ERROR_PROMPT << "cannot retrieve section\n";
    return TCL_ERROR;
  }

  if (sectionRepres->getType() == SEC_TAG_FiberSection) {
    // build the section

    FiberSectionRepr *fiberSectionRepr = (FiberSectionRepr *)sectionRepres;

    int i, j, k;
    int numFibers;
    int numHFibers;

    int numPatches;
    Patch **patch;

    int numReinfLayers;
    ReinfLayer **reinfLayer;

    numPatches = fiberSectionRepr->getNumPatches();
    patch = fiberSectionRepr->getPatches();
    numReinfLayers = fiberSectionRepr->getNumReinfLayers();
    reinfLayer = fiberSectionRepr->getReinfLayers();

    int numSectionRepresFibers = fiberSectionRepr->getNumFibers();
    Fiber **sectionRepresFibers = fiberSectionRepr->getFibers();

    int numSectionRepresHFibers = fiberSectionRepr->getNumHFibers();
    Fiber **sectionRepresHFibers = fiberSectionRepr->getHFibers();

    numFibers = numSectionRepresFibers;
    for (i = 0; i < numPatches; i++)
      numFibers += patch[i]->getNumCells();

    for (i = 0; i < numReinfLayers; i++)
      numFibers += reinfLayer[i]->getNumReinfBars();

    numHFibers = numSectionRepresHFibers;

    static Vector fiberPosition(2);
    int matTag;

    ID fibersMaterial(numFibers - numSectionRepresFibers);
    Matrix fibersPosition(2, numFibers - numSectionRepresFibers);
    Vector fibersArea(numFibers - numSectionRepresFibers);

    int numCells;
    Cell **cell;

    k = 0;
    for (int i = 0; i < numPatches; i++) {
      numCells = patch[i]->getNumCells();
      matTag = patch[i]->getMaterialID();

      cell = patch[i]->getCells();

      for (int j = 0; j < numCells; j++) {
        fibersMaterial(k) = matTag;
        fibersArea(k) = cell[j]->getArea();
        fiberPosition = cell[j]->getCentroidPosition();
        fibersPosition(0, k) = fiberPosition(0);
        fibersPosition(1, k) = fiberPosition(1);
        k++;
      }

      for (int j = 0; j < numCells; j++)
        delete cell[j];

      delete[] cell;
    }

    ReinfBar *reinfBar;
    int numReinfBars;

    for (i = 0; i < numReinfLayers; i++) {
      numReinfBars = reinfLayer[i]->getNumReinfBars();
      reinfBar = reinfLayer[i]->getReinfBars();
      matTag = reinfLayer[i]->getMaterialID();

      for (j = 0; j < numReinfBars; j++) {
        fibersMaterial(k) = matTag;
        fibersArea(k) = reinfBar[j].getArea();
        fiberPosition = reinfBar[j].getPosition();

        fibersPosition(0, k) = fiberPosition(0);
        fibersPosition(1, k) = fiberPosition(1);

        k++;
      }
      delete[] reinfBar;
    }

    UniaxialMaterial *material;


    Fiber **fiber = new Fiber *[numFibers];
    if (fiber == nullptr) {
      opserr << G3_ERROR_PROMPT << "unable to allocate fibers \n";
      return TCL_ERROR;
    }

    // copy the section repres fibers
    for (i = 0; i < numSectionRepresFibers; i++)
      fiber[i] = sectionRepresFibers[i];

    Fiber **Hfiber = new Fiber *[numHFibers];
    if (Hfiber == nullptr) {
      opserr << G3_ERROR_PROMPT << "unable to allocate Hfibers \n";
      return TCL_ERROR;
    }

    // copy the section repres fibers
    for (int i = 0; i < numSectionRepresHFibers; i++)
      Hfiber[i] = sectionRepresHFibers[i];

    // creates 2d section
    int NDM = builder->getNDM();
    if (NDM == 2) {
      k = 0;
      for (i = numSectionRepresFibers; i < numFibers; i++) {
        material = builder->getUniaxialMaterial(fibersMaterial(k));
        if (material == nullptr) {
          opserr << G3_ERROR_PROMPT << "invalid material ID for patch\n";
          return TCL_ERROR;
        }

        fiber[i] = new UniaxialFiber2d(k, *material, fibersArea(k),
                                       fibersPosition(0, k));

        k++;
      }

      SectionForceDeformation *section =
          new FiberSection2dInt(secTag, numFibers, fiber, numHFibers, Hfiber,
                                NStrip1, t1, NStrip2, t2, NStrip3, t3);

      // Delete fibers
      for (i = 0; i < numFibers; i++)
        delete fiber[i];

      for (i = 0; i < numHFibers; i++)
        delete Hfiber[i];

      if (section == nullptr) {
        opserr << G3_ERROR_PROMPT << "cannot construct section\n";
        return TCL_ERROR;
      }

      if (theTclBasicBuilder->addSection (*section) < 0) {
      // if (OPS_addSectionForceDeformation(section) != true) {
        opserr << G3_ERROR_PROMPT << "- cannot add section\n";
        return TCL_ERROR;
      }

    } else if (NDM == 3) {

      static Vector fiberPosition(2);
      k = 0;
      for (i = numSectionRepresFibers; i < numFibers; i++) {
        material = builder->getUniaxialMaterial(fibersMaterial(k));
        if (material == nullptr) {
          opserr << G3_ERROR_PROMPT << "invalid material ID for patch\n";
          return TCL_ERROR;
        }

        fiberPosition(0) = fibersPosition(0, k);
        fiberPosition(1) = fibersPosition(1, k);

        fiber[i] =
            new UniaxialFiber3d(k, *material, fibersArea(k), fiberPosition);
        if (fibersArea(k) < 0)
          opserr << "ERROR: " << fiberPosition(0) << " " << fiberPosition(1)
                 << endln;
        if (fiber[k] == 0) {
          opserr << G3_ERROR_PROMPT << "unable to allocate fiber \n";
          return TCL_ERROR;
        }
        k++;
      }

      SectionForceDeformation *section = 0;
      section = new FiberSection3d(secTag, numFibers, fiber, theTorsion,
                                   currentSectionComputeCentroid);

      // Delete fibers
      for (i = 0; i < numFibers; i++)
        delete fiber[i];

      if (section == 0) {
        opserr << G3_ERROR_PROMPT << "- cannot construct section\n";
        return TCL_ERROR;
      }

      if (theTclBasicBuilder->addSection (*section) < 0) {
      // if (OPS_addSectionForceDeformation(section) != true) {
        opserr << G3_ERROR_PROMPT << "- cannot add section\n";
        return TCL_ERROR;
      }

    } else {
      opserr << G3_ERROR_PROMPT << "NDM = " << NDM
             << " is imcompatible with available frame elements\n";
      return TCL_ERROR;
    }

    // Delete fiber array
    delete[] fiber;
    //   delete [] Hfiber;

  } else {
    opserr << G3_ERROR_PROMPT << "section invalid: can only build fiber sections\n";
    return TCL_ERROR;
  }

  return TCL_OK;
}

int
TclCommand_addUCFiberSection(ClientData clientData, Tcl_Interp *interp,
                             int argc, TCL_Char ** const argv,
                             TclBasicBuilder *theTclBasicBuilder)
{
  assert(clientData != nullptr);
  BasicModelBuilder* builder = (BasicModelBuilder*)clientData;
  G3_Runtime *rt = G3_getRuntime(interp);
  int secTag;

  if (argc < 4)
    return TCL_ERROR;

  if (Tcl_GetInt(interp, argv[2], &secTag) != TCL_OK) {
    opserr << "could not read section tag\n";
    return TCL_ERROR;
  }

  theTclBasicBuilder->currentSectionTag = secTag;

  // first create an empty FiberSection

  SectionForceDeformation *section = nullptr;
  FiberSection2d *section2d = nullptr;
  FiberSection3d *section3d = nullptr;

  int NDM = builder->getNDM();
  if (NDM == 2) {
    section2d = new FiberSection2d(secTag, 0, 0, currentSectionComputeCentroid);
    section = section2d;
    // SectionForceDeformation *section = new FiberSection(secTag, 0, 0);
  } else if (NDM == 3) {
    UniaxialMaterial *theGJ = new ElasticMaterial(0, 1e10);
    section3d =
        new FiberSection3d(secTag, 0, 0, *theGJ, currentSectionComputeCentroid);
    section = section3d;
    delete theGJ;
  }

  if (section == 0) {
    return TCL_ERROR;
  }

  //
  // now parse the output file containing the fiber data,
  // create fibers and add them to the section
  //

  // open the file
  TCL_Char *fileName = argv[3];
  ifstream theFile;
  theFile.open(fileName, ios::in);
  if (!theFile) {
    opserr << "section UCFiber - could not open file named " << fileName;
    return TCL_ERROR;
  } else {
    int foundStart = 0;
    static char garbage[100];

    // parse through until find start of fiber data
    while (foundStart == 0 && theFile >> garbage)
      if (strcmp(garbage, "#FIBERS") == 0)
        foundStart = 1;

    if (foundStart == 0) {
      theFile.close();
      return TCL_ERROR;
    }

    // parse the fiber data until eof, creating a fiber and adding to section as
    // go
    double ycoord, zcoord, area, prestrain;
    int matTag;
    int fiberCount = 0;

    while (theFile >> ycoord >> zcoord >> area >> prestrain >> garbage >>
           matTag) {

      UniaxialMaterial *theMaterial = G3_getUniaxialMaterialInstance(rt,matTag);
      if (theMaterial == 0) {
        opserr << "section UCFiber - no material exists with tag << " << matTag
               << endln;
        return TCL_ERROR;
      }

      Fiber *theFiber = 0;
      if (NDM == 2) {
        theFiber =
            new UniaxialFiber2d(fiberCount++, *theMaterial, area, zcoord);
        if (theFiber != 0) {
          section2d->addFiber(*theFiber);
          delete theFiber;
        }
      } else {
        static Vector pos(2);
        pos(0) = ycoord;
        pos(1) = zcoord;
        theFiber = new UniaxialFiber3d(fiberCount++, *theMaterial, area, pos);
        if (theFiber != 0) {
          section3d->addFiber(*theFiber);
          delete theFiber;
        }
      }
    }

    // close the file
    theFile.close();
  }

  // finally add the section to our modelbuilder
  if (theTclBasicBuilder->addSection(*section) < 0) {
  // if (OPS_addSectionForceDeformation(section) != true) {
    opserr << G3_ERROR_PROMPT << "- cannot add section\n";
    return TCL_ERROR;
  }

  return TCL_OK;
}

////Changes made by L.Jiang [SIF] 2017
///--Adding Tclcommand for FiberSectionThermal:[BEGIN] by UoE OpenSees Group
///--///
int
TclCommand_addFiberSectionThermal(ClientData clientData, Tcl_Interp *interp,
                                  int argc, TCL_Char ** const argv,
                                  TclBasicBuilder *theTclBasicBuilder)
{
  assert(clientData != nullptr);
  BasicModelBuilder* builder = (BasicModelBuilder*)clientData;

  int secTag;
  int maxNumPatches = 30;
  int maxNumReinfLayers = 30;

  if (argc < 4)
    return TCL_ERROR;

  if (Tcl_GetInt(interp, argv[2], &secTag) != TCL_OK) {
    opserr << G3_ERROR_PROMPT << "bad command - want: \nsection fiberSec secTag { "
              "\n\tpatch <patch arguments> \n\tlayer <layer arguments> \n}\n";
    return TCL_ERROR;
  }
  theTclBasicBuilder->currentSectionTag = secTag;
  // create the fiber section representation (with the geometric information)
  SectionRepres *fiberSectionRepr =
      new FiberSectionRepr(secTag, maxNumPatches, maxNumReinfLayers);


  if (theTclBasicBuilder->addSectionRepres(*fiberSectionRepr) < 0) {
    opserr << G3_ERROR_PROMPT << "- cannot add section representation\n";
    return TCL_ERROR;
  }

  int brace = 3; // Start of recursive parse
  double GJ = 1.0;
  bool deleteTorsion = false;
  UniaxialMaterial *torsion = 0;
  if (strcmp(argv[3], "-GJ") == 0) {
    if (Tcl_GetDouble(interp, argv[4], &GJ) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid GJ";
      return TCL_ERROR;
    }
    torsion = new ElasticMaterial(0, GJ);

    brace = 5;
  }
  int torsionTag = 0;
  if (strcmp(argv[3], "-torsion") == 0) {
    if (Tcl_GetInt(interp, argv[4], &torsionTag) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid torsionTag";
      return TCL_ERROR;
    }

    torsion = builder->getUniaxialMaterial(torsionTag);
    if (torsion == 0) {
      opserr << G3_ERROR_PROMPT << "uniaxial material does not exist\n";
      opserr << "uniaxial material: " << torsionTag;
      opserr << "\nFiberSection3d: " << secTag << endln;
      return TCL_ERROR;
    }

    brace = 5;
  }

  // parse the information inside the braces (patches and reinforcing layers)
  if (Tcl_Eval(interp, argv[brace]) != TCL_OK) {
    opserr << G3_ERROR_PROMPT << "- error reading information in { } \n";
    return TCL_ERROR;
  }

  int NDM = builder->getNDM();
  if (NDM == 3 && torsion == 0) {
    opserr << G3_ERROR_PROMPT << "- no torsion specified for 3D fiber section, use -GJ or "
              "-torsion\n";
    opserr << "\nFiberSectionThermal3d: " << secTag << endln;
    return TCL_ERROR;
  }

  // build the fiber section (for analysis)
  if (buildSectionThermal(clientData, interp, theTclBasicBuilder, secTag, *torsion) !=
      TCL_OK) {
    opserr << G3_ERROR_PROMPT << "- error constructing the section\n";
    return TCL_ERROR;
  }

  if (deleteTorsion)
    delete torsion;

  return TCL_OK;
}

//
// function for building FiberSectionThermal 
// by UoE OpenSees Group
//
static int
buildSectionThermal(ClientData clientData, Tcl_Interp *interp, TclBasicBuilder *theTclBasicBuilder,
                    int secTag, UniaxialMaterial &theTorsion)
{
  assert(clientData != nullptr);
  BasicModelBuilder *builder = (BasicModelBuilder*)clientData;
  G3_Runtime *rt = G3_getRuntime(interp);

  SectionRepres *sectionRepres = theTclBasicBuilder->getSectionRepres(secTag);
  if (sectionRepres == nullptr) {
    opserr << G3_ERROR_PROMPT << "cannot retrieve section\n";
    return TCL_ERROR;
  }

  if (sectionRepres->getType() == SEC_TAG_FiberSection) {
    // build the section
    FiberSectionRepr *fiberSectionRepr = (FiberSectionRepr *)sectionRepres;
    int i, j, k;
    int numFibers;
    int numPatches;
    Patch **patch;

    int numReinfLayers;
    ReinfLayer **reinfLayer;

    numPatches = fiberSectionRepr->getNumPatches();
    patch = fiberSectionRepr->getPatches();
    numReinfLayers = fiberSectionRepr->getNumReinfLayers();
    reinfLayer = fiberSectionRepr->getReinfLayers();

    int numSectionRepresFibers = fiberSectionRepr->getNumFibers();
    Fiber **sectionRepresFibers = fiberSectionRepr->getFibers();

    numFibers = numSectionRepresFibers;
    for (i = 0; i < numPatches; i++)
      numFibers += patch[i]->getNumCells();

    for (i = 0; i < numReinfLayers; i++)
      numFibers += reinfLayer[i]->getNumReinfBars();

    static Vector fiberPosition(2);
    int matTag;

    ID fibersMaterial(numFibers - numSectionRepresFibers);
    Matrix fibersPosition(2, numFibers - numSectionRepresFibers);
    Vector fibersArea(numFibers - numSectionRepresFibers);

    int numCells;
    Cell **cell;

    k = 0;
    for (i = 0; i < numPatches; i++) {
      numCells = patch[i]->getNumCells();
      matTag = patch[i]->getMaterialID();
      cell = patch[i]->getCells();
      if (cell == 0) {
        opserr << G3_ERROR_PROMPT << "out of run to create fibers\n";
        return TCL_ERROR;
      }
      for (j = 0; j < numCells; j++) {
        fibersMaterial(k) = matTag;
        fibersArea(k) = cell[j]->getArea();
        fiberPosition = cell[j]->getCentroidPosition();

        fibersPosition(0, k) = fiberPosition(0);
        fibersPosition(1, k) = fiberPosition(1);
        k++;
      }
      for (j = 0; j < numCells; j++)
        delete cell[j];
      delete[] cell;
    }
    ReinfBar *reinfBar;
    int numReinfBars;
    for (i = 0; i < numReinfLayers; i++) {
      numReinfBars = reinfLayer[i]->getNumReinfBars();
      reinfBar = reinfLayer[i]->getReinfBars();
      matTag = reinfLayer[i]->getMaterialID();

      for (j = 0; j < numReinfBars; j++) {
        fibersMaterial(k) = matTag;
        fibersArea(k) = reinfBar[j].getArea();
        fiberPosition = reinfBar[j].getPosition();
        fibersPosition(0, k) = fiberPosition(0);
        fibersPosition(1, k) = fiberPosition(1);
        k++;
      }
      delete[] reinfBar;
    }
    UniaxialMaterial *material;

    int NDM = builder->getNDM(); 
    Fiber **fiber = new Fiber *[numFibers];
    if (fiber == nullptr) {
      opserr << G3_ERROR_PROMPT << "unable to allocate fibers \n";
      return TCL_ERROR;
    }
    // copy the section repres fibers
    for (i = 0; i < numSectionRepresFibers; i++)
      fiber[i] = sectionRepresFibers[i];

    // creates 2d section
    if (NDM == 2) {
      k = 0;
      for (i = numSectionRepresFibers; i < numFibers; i++) {
        material = G3_getUniaxialMaterialInstance(rt,fibersMaterial(k));
        if (material == 0) {
          opserr << G3_ERROR_PROMPT << "invalid material ID for patch\n";
          return TCL_ERROR;
        }

        fiber[i] = new UniaxialFiber2d(k, *material, fibersArea(k),
                                       fibersPosition(0, k));
        if (!fiber[i]) {
          opserr << G3_ERROR_PROMPT << "unable to allocate fiber \n";
          return TCL_ERROR;
        }
        k++;
      }

      SectionForceDeformation *section = new FiberSection2dThermal(
          secTag, numFibers, fiber, currentSectionComputeCentroid);

      // Delete fibers
      for (i = 0; i < numFibers; i++)
        delete fiber[i];

      if (section == 0) {
        opserr << G3_ERROR_PROMPT << "- cannot construct section\n";
        return TCL_ERROR;
      }

      if (builder->addSection(*section) < 0) {
        opserr << G3_ERROR_PROMPT << "- cannot add section\n";
        return TCL_ERROR;
      }
    } else if (NDM == 3) {
      static Vector fiberPosition(2);
      k = 0;
      for (i = numSectionRepresFibers; i < numFibers; i++) {
        material = builder->getUniaxialMaterial(fibersMaterial(k));
        if (material == 0) {
          opserr << G3_ERROR_PROMPT << "invalid material ID for patch\n";
          return TCL_ERROR;
        }
        fiberPosition(0) = fibersPosition(0, k);
        fiberPosition(1) = fibersPosition(1, k);

        fiber[i] =
            new UniaxialFiber3d(k, *material, fibersArea(k), fiberPosition);
        if (fibersArea(k) < 0)
          opserr << "ERROR: " << fiberPosition(0) << " " << fiberPosition(1)
                 << endln;
        if (!fiber[k]) {
          opserr << G3_ERROR_PROMPT << "unable to allocate fiber \n";
          return TCL_ERROR;
        }
        k++;
      }
      // SectionForceDeformation *section = new FiberSection(secTag, numFibers,
      // fiber);

      SectionForceDeformation *section = 0;
      section = new FiberSection3dThermal(secTag, numFibers, fiber,
                                          currentSectionComputeCentroid);

      // Delete fibers
      for (i = 0; i < numFibers; i++)
        delete fiber[i];
      if (section == 0) {
        opserr << G3_ERROR_PROMPT << "- cannot construct section\n";
        return TCL_ERROR;
      }

      if (builder->addSection(*section) < 0) {
        opserr << G3_ERROR_PROMPT << "- cannot add section\n";
        return TCL_ERROR;
      }
    } else {
      opserr << G3_ERROR_PROMPT << "NDM = " << NDM
             << " is imcompatible with available frame elements\n";
      return TCL_ERROR;
    }
    // Delete fiber array
    delete[] fiber;

  } else {
    opserr << G3_ERROR_PROMPT << "section invalid: can only build fiber sections\n";
    return TCL_ERROR;

  }
  return TCL_OK;

}

// Changes made by L.Jiang [SIF]

int
TclCommand_addFiberSectionAsym(ClientData clientData, Tcl_Interp *interp,
                               int argc, TCL_Char ** const argv,
                               TclBasicBuilder *theTclBasicBuilder)
{
  int secTag;
  int maxNumPatches = 30;
  int maxNumReinfLayers = 30;

  if (argc < 4)
    return TCL_ERROR;

  if (Tcl_GetInt(interp, argv[2], &secTag) != TCL_OK) {
    opserr << G3_ERROR_PROMPT << "bad command - want: \nsection fiberSec secTag { "
              "\n\tpatch <patch arguments> \n\tlayer <layer arguments> \n}\n";
    return TCL_ERROR;
  }

  theTclBasicBuilder->currentSectionTag = secTag;
  currentSectionIsND = false;
  currentSectionIsWarping = false;
  if (strcmp(argv[1], "NDFiber") == 0)
    currentSectionIsND = true;
  if (strcmp(argv[1], "NDFiberWarping") == 0) {
    currentSectionIsND = true;
    currentSectionIsWarping = true;
  }

  // create the fiber section representation (with the geometric information)

  SectionRepres *fiberSectionRepr =
      new FiberSectionRepr(secTag, maxNumPatches, maxNumReinfLayers);

  if (theTclBasicBuilder->addSectionRepres(*fiberSectionRepr) < 0) {
    opserr << G3_ERROR_PROMPT << "- cannot add section representation\n";
    return TCL_ERROR;
  }
  // Xinlong
  double Ys, Zs; // Xinlong: input of coords of shear center relative to
                 // centroid
  if (Tcl_GetDouble(interp, argv[3], &Ys) != TCL_OK) {
    opserr << G3_ERROR_PROMPT << "invalid Ys";
    return TCL_ERROR;
  }
  if (Tcl_GetDouble(interp, argv[4], &Zs) != TCL_OK) {
    opserr << G3_ERROR_PROMPT << "invalid Zs";
    return TCL_ERROR;
  }
  // Xinlong

  int brace = 5; // Start of recursive parse //Xinlong
  double GJ = 1.0;
  bool isTorsion = false;
  if (strcmp(argv[5], "-GJ") == 0) {                     // Xinlong
    if (Tcl_GetDouble(interp, argv[6], &GJ) != TCL_OK) { // Xinlong
      opserr << G3_ERROR_PROMPT << "invalid GJ";
      return TCL_ERROR;
    }
    isTorsion = true;
    brace = 7; // Xinlong
  }

  // parse the information inside the braces (patches and reinforcing layers)
  if (Tcl_Eval(interp, argv[brace]) != TCL_OK) {
    opserr << G3_ERROR_PROMPT << "- error reading information in { } \n";
    return TCL_ERROR;
  }

  // build the fiber section (for analysis)
  if (buildSectionAsym(clientData, interp, theTclBasicBuilder, secTag, isTorsion, GJ, Ys, Zs) != TCL_OK) {
    opserr << G3_ERROR_PROMPT << "- error constructing the section\n";
    return TCL_ERROR;
  }

  //    currentSectionTag = 0;

  return TCL_OK;
}

static int
buildSectionAsym(ClientData clientData, Tcl_Interp *interp, TclBasicBuilder *theTclBasicBuilder,
                 int secTag, bool isTorsion, double GJ, double Ys,
                 double Zs) // Xinlong
{
  assert(clientData != nullptr);
  BasicModelBuilder *builder = (BasicModelBuilder*)clientData;
  G3_Runtime *rt = G3_getRuntime(interp);
  SectionRepres *sectionRepres = theTclBasicBuilder->getSectionRepres(secTag);

  if (sectionRepres == 0) {
    opserr << G3_ERROR_PROMPT << "cannot retrieve section\n";
    return TCL_ERROR;
  }

  if (sectionRepres->getType() == SEC_TAG_FiberSection) {
    // build the section

    FiberSectionRepr *fiberSectionRepr = (FiberSectionRepr *)sectionRepres;

    int i, j, k;
    int numFibers;

    int numPatches;
    Patch **patch;

    int numReinfLayers;
    ReinfLayer **reinfLayer;

    numPatches = fiberSectionRepr->getNumPatches();
    patch = fiberSectionRepr->getPatches();
    numReinfLayers = fiberSectionRepr->getNumReinfLayers();
    reinfLayer = fiberSectionRepr->getReinfLayers();

    int numSectionRepresFibers = fiberSectionRepr->getNumFibers();
    Fiber **sectionRepresFibers = fiberSectionRepr->getFibers();

    numFibers = numSectionRepresFibers;
    for (i = 0; i < numPatches; i++)
      numFibers += patch[i]->getNumCells();

    for (i = 0; i < numReinfLayers; i++)
      numFibers += reinfLayer[i]->getNumReinfBars();


    static Vector fiberPosition(2);
    int matTag;

    ID fibersMaterial(numFibers - numSectionRepresFibers);
    Matrix fibersPosition(2, numFibers - numSectionRepresFibers);
    Vector fibersArea(numFibers - numSectionRepresFibers);

    int numCells;
    Cell **cell;

    k = 0;
    for (i = 0; i < numPatches; i++) {

      numCells = patch[i]->getNumCells();
      matTag = patch[i]->getMaterialID();


      cell = patch[i]->getCells();

      if (cell == 0) {
        opserr << G3_ERROR_PROMPT << "out of run to create fibers\n";
        return TCL_ERROR;
      }

      for (j = 0; j < numCells; j++) {
        fibersMaterial(k) = matTag;
        fibersArea(k) = cell[j]->getArea();
        fiberPosition = cell[j]->getCentroidPosition();

        fibersPosition(0, k) = fiberPosition(0);
        fibersPosition(1, k) = fiberPosition(1);

        k++;
      }

      for (j = 0; j < numCells; j++)
        delete cell[j];

      delete[] cell;
    }

    ReinfBar *reinfBar;
    int numReinfBars;

    for (i = 0; i < numReinfLayers; i++) {
      numReinfBars = reinfLayer[i]->getNumReinfBars();
      reinfBar = reinfLayer[i]->getReinfBars();
      matTag = reinfLayer[i]->getMaterialID();

      for (j = 0; j < numReinfBars; j++) {
        fibersMaterial(k) = matTag;
        fibersArea(k) = reinfBar[j].getArea();
        fiberPosition = reinfBar[j].getPosition();

        fibersPosition(0, k) = fiberPosition(0);
        fibersPosition(1, k) = fiberPosition(1);

        k++;
      }
      delete[] reinfBar;
    }

    UniaxialMaterial *material;
    NDMaterial *ndmaterial;

    // dimension of the structure (1d, 2d, or 3d)
    int NDM = builder->getNDM();

    Fiber **fiber = new Fiber *[numFibers];
    if (fiber == 0) {
      opserr << G3_ERROR_PROMPT << "unable to allocate fibers \n";
      return TCL_ERROR;
    }

    // copy the section repres fibers
    for (i = 0; i < numSectionRepresFibers; i++)
      fiber[i] = sectionRepresFibers[i];

    // creates 2d section

    if (NDM == 2) {
      k = 0;
      for (i = numSectionRepresFibers; i < numFibers; i++) {
        if (currentSectionIsND) {
          ndmaterial = builder->getNDMaterial(fibersMaterial(k));
          if (ndmaterial == nullptr) {
            opserr << G3_ERROR_PROMPT << "invalid NDmaterial ID for patch\n";
            return TCL_ERROR;
          }
          fiber[i] = new NDFiber2d(k, *ndmaterial, fibersArea(k),
                                   fibersPosition(0, k));
        } else {
          material = builder->getUniaxialMaterial(fibersMaterial(k));
          if (material == nullptr) {
            opserr << G3_ERROR_PROMPT << "invalid UniaxialMaterial ID for patch\n";
            return TCL_ERROR;
          }
          fiber[i] = new UniaxialFiber2d(k, *material, fibersArea(k),
                                         fibersPosition(0, k));
        }
        if (fiber[i] == 0) {
          opserr << G3_ERROR_PROMPT << "unable to allocate fiber \n";
          return TCL_ERROR;
        }
        k++;
      }

      SectionForceDeformation *section = 0;
      if (currentSectionIsND) {
        if (currentSectionIsWarping)
          section = new NDFiberSectionWarping2d(secTag, numFibers, fiber);
        else
          section = new NDFiberSection2d(secTag, numFibers, fiber);
      } else
        section = new FiberSection2d(secTag, numFibers, fiber);

      // Delete fibers
      for (i = 0; i < numFibers; i++)
        delete fiber[i];

      if (section == 0) {
        opserr << G3_ERROR_PROMPT << "- cannot construct section\n";
        return TCL_ERROR;
      }

      // if (OPS_addSectionForceDeformation(section) != true) {
      if (builder->addSection(*section) < 0) {
        opserr << G3_ERROR_PROMPT << "- cannot add section\n";
        return TCL_ERROR;
      }


    } else if (NDM == 3) {

      static Vector fiberPosition(2);
      k = 0;
      for (i = numSectionRepresFibers; i < numFibers; i++) {
        fiberPosition(0) = fibersPosition(0, k);
        fiberPosition(1) = fibersPosition(1, k);
        if (currentSectionIsND) {
          ndmaterial = builder->getNDMaterial(fibersMaterial(k));
          if (ndmaterial == 0) {
            opserr << G3_ERROR_PROMPT << "invalid NDmaterial ID for patch\n";
            return TCL_ERROR;
          }
          fiber[i] = new NDFiber3d(k, *ndmaterial, fibersArea(k),
                                   fiberPosition(0), fiberPosition(1));
        } else {
          material = G3_getUniaxialMaterialInstance(rt,fibersMaterial(k));
          if (material == nullptr) {
            opserr << G3_ERROR_PROMPT << "invalid UniaxialMaterial ID for patch\n";
            return TCL_ERROR;
          }
          fiber[i] =
              new UniaxialFiber3d(k, *material, fibersArea(k), fiberPosition);
        }
        if (fiber[k] == nullptr) {
          opserr << G3_ERROR_PROMPT << "unable to allocate fiber \n";
          return TCL_ERROR;
        }
        k++;
      }

      SectionForceDeformation *section = nullptr;
      if (currentSectionIsND)
        section = new NDFiberSection3d(secTag, numFibers, fiber);
      else if (isTorsion) {
        ElasticMaterial theGJ(0, GJ);
        section = new FiberSectionAsym3d(secTag, numFibers, fiber, &theGJ, Ys, Zs);
      } else
        section = new FiberSectionAsym3d(secTag, numFibers, fiber, 0, Ys, Zs);

      // Delete fibers
      for (i = 0; i < numFibers; i++)
        delete fiber[i];

      if (section == 0) {
        opserr << G3_ERROR_PROMPT << "- failed to construct section\n";
        return TCL_ERROR;
      }

      // if (theTclBasicBuilder->addSection (*section) < 0) {
      if (OPS_addSectionForceDeformation(section) != true) {
        opserr << G3_ERROR_PROMPT << "- cannot add section\n";
        return TCL_ERROR;
      }

    } else {
      opserr << G3_ERROR_PROMPT << "NDM = " << NDM
             << " is imcompatible with available frame elements\n";
      return TCL_ERROR;
    }

    // Delete fiber array
    delete[] fiber;

  } else {
    opserr << G3_ERROR_PROMPT << "section invalid: can only build fiber sections\n";
    return TCL_ERROR;
  }

  return TCL_OK;
}

#if 0
SectionForceDeformation*
G3Parse_newTubeSection(G3_Runtime* rt, int argc, G3_Char ** const argv)
{
  SectionForceDeformation *theSection = nullptr;
  if (strcmp(argv[1], "Tube") == 0) {
    if (argc < 8) {
      opserr << G3_ERROR_PROMPT << "insufficient arguments\n";
      opserr << "Want: section Tube tag? matTag? D? t? nfw? nfr?" << endln;
      return nullptr;
    }

    int tag, matTag;
    double D, t;
    int nfw, nfr;

    if (Tcl_GetInt(interp, argv[2], &tag) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid section Tube tag" << endln;
      return nullptr;
    }

    if (Tcl_GetInt(interp, argv[3], &matTag) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid section Tube matTag" << endln;
      return nullptr;
    }

    if (Tcl_GetDouble(interp, argv[4], &D) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid D" << endln;
      opserr << "Tube section: " << tag << endln;
      return nullptr;
    }

    if (Tcl_GetDouble(interp, argv[5], &t) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid t" << endln;
      opserr << "Tube section: " << tag << endln;
      return nullptr;
    }

    if (Tcl_GetInt(interp, argv[6], &nfw) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid nfw" << endln;
      opserr << "Tube section: " << tag << endln;
      return nullptr;
    }

    if (Tcl_GetInt(interp, argv[7], &nfr) != TCL_OK) {
      opserr << G3_ERROR_PROMPT << "invalid nfr" << endln;
      opserr << "Tube  section: " << tag << endln;
      return nullptr;
    }

    TubeSectionIntegration tubesect(D, t, nfw, nfr);

    int numFibers = tubesect.getNumFibers();

    if (argc > 8) {

      double shape = 1.0;
      if (argc > 9) {
        if (Tcl_GetDouble(interp, argv[9], &shape) != TCL_OK) {
          opserr << G3_ERROR_PROMPT << "invalid shape" << endln;
          opserr << "WFSection2d section: " << tag << endln;
          return nullptr;
        }
      }

      NDMaterial *theSteel = builder->getNDMaterial(matTag);

      if (theSteel == 0) {
        opserr << G3_ERROR_PROMPT << "ND material does not exist\n";
        opserr << "material: " << matTag;
        opserr << "\nTube section: " << tag << endln;
        return nullptr;
      }

      NDMaterial **theMats = new NDMaterial *[numFibers];

      tubesect.arrangeFibers(theMats, theSteel);

      // Parsing was successful, allocate the section
      theSection = 0;
      if (strcmp(argv[8], "-nd") == 0)
        theSection =
            new NDFiberSection3d(tag, numFibers, theMats, tubesect, shape);
      if (strcmp(argv[8], "-ndWarping") == 0)
        theSection = new NDFiberSectionWarping2d(tag, numFibers, theMats,
                                                 tubesect, shape);

      delete[] theMats;
    } else {
      UniaxialMaterial *theSteel = builder->getUniaxialMaterial(matTag);

      if (theSteel == 0) {
        opserr << G3_ERROR_PROMPT << "uniaxial material does not exist\n";
        opserr << "material: " << matTag;
        opserr << "\nTube section: " << tag << endln;
        return nullptr;
      }

      UniaxialMaterial **theMats = new UniaxialMaterial *[numFibers];

      tubesect.arrangeFibers(theMats, theSteel);

      // Parsing was successful, allocate the section
      theSection = new FiberSection2d(tag, numFibers, theMats, tubesect);

      delete[] theMats;
    }
  }
}
#endif

