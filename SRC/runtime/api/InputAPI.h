#ifndef G3PARSE_H
#define G3PARSE_H
#ifndef G3_RUNTIME_H
#include <G3_Runtime.h>
#define G3_RUNTIME_H
#endif

#include <tcl.h>

#define G3_Char TCL_Char

// #define OPS_GetIntInput(ndat, dat)  (rt->interp->OPS_GetIntInput((ndat), (dat)))
// #define OPS_GetDoubleInput (rt->getDoubleInput)

#define G3Parse_getInt(bld, arg, adr)    Tcl_GetInt((bld)->m_interp, (arg), (adr))
#define G3Parse_getDouble(bld, arg, adr) Tcl_GetDouble((bld)->m_interp, (arg), (adr))
// #define G3Parse_AppendResult(interp, 

enum SuccessFlag {
  G3_OK    = TCL_OK, 
  G3_ERROR = TCL_ERROR
};

typedef enum SuccessFlag SuccessFlag;
/*
static void printCommand(int argc, TCL_Char **argv) {
  opserr << "Input command: ";
  for (int i = 0; i < argc; i++)
    opserr << argv[i] << " ";
  opserr << endln;
}
*/
#endif // G3PARSE_H
