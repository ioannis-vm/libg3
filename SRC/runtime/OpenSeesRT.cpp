/* ****************************************************************** **
**    OpenSees - Open System for Earthquake Engineering Simulation    **
**          Pacific Earthquake Engineering Research Center            **
** ****************************************************************** */
//
//
#ifndef OPENSEESRT_VERSION
#  define OPENSEESRT_VERSION "0.0.0"
#endif
//
#include <g3_api.h>
#undef G3_Runtime
#include "G3_Runtime.h"
#include "Logging/G3_Logging.h"
#include <handler/OPS_Stream.h>
#include <StandardStream.h>      
#include "commands/strings.cpp"
#include <stdio.h>
#ifdef _WIN32
#  include <io.h>
#  define isatty _isatty
#  define STDERR_FILENO _fileno(stderr)
#else
#  include <unistd.h>               
#endif
//
extern int OpenSeesAppInit(Tcl_Interp *interp);
extern void G3_InitTclSequentialAPI(Tcl_Interp* interp);
extern int init_g3_tcl_utils(Tcl_Interp*);

//
// Return the current OpenSees version
//
static int
version(ClientData clientData, Tcl_Interp *interp, int argc, TCL_Char **argv)
{
  char buffer[20];

  sprintf(buffer, "%s", OPENSEESRT_VERSION);
  Tcl_SetResult(interp, buffer, TCL_VOLATILE);

  return TCL_OK;
}

//
extern "C" {
//
// Called when the library is loaded as a Tcl extension.
//
int DLLEXPORT
Openseesrt_Init(Tcl_Interp *interp)
{
  if (Tcl_InitStubs(interp, TCL_VERSION, 0) == NULL) {
    return TCL_ERROR;
  }

  if (Tcl_PkgProvide(interp, "OpenSeesRT", OPENSEESRT_VERSION) == TCL_ERROR) {
    return TCL_ERROR;
  }

  G3_Runtime *rt = new G3_Runtime{interp};
  Tcl_SetAssocData(interp, "G3_Runtime", NULL, (ClientData)rt);

  OpenSeesAppInit(interp);
  G3_InitTclSequentialAPI(interp);
  init_g3_tcl_utils(interp);

  if (isatty(STDERR_FILENO))
    G3_setStreamColor(nullptr, G3_Warn, 1);


  // Set some variables
  Tcl_SetVar(interp, "opensees::copyright", copyright,      TCL_LEAVE_ERR_MSG);
  Tcl_SetVar(interp, "opensees::license",   license,        TCL_LEAVE_ERR_MSG);
  Tcl_SetVar(interp, "opensees::banner",    unicode_banner, TCL_LEAVE_ERR_MSG);
  Tcl_CreateCommand(interp, "version",      version,      nullptr, nullptr);
  return TCL_OK;
}

} // extern "C"

