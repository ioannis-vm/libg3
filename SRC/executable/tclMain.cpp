#include <string.h>
#ifndef _WIN32
#  include <unistd.h>
#endif

#ifdef USE_TCL_STUBS
#undef USE_TCL_STUBS
#endif

extern "C" {
 #include <tcl.h>
 #include <tclDecls.h>
 #ifdef _TCL85
 #define TclFormatInt(buf, n)   sprintf((buf),"%ld", (long)(n))
 #else
 EXTERN int  TclFormatInt _ANSI_ARGS_((char *buffer, long n));
 #endif
 EXTERN int  TclObjCommandComplete _ANSI_ARGS_((Tcl_Obj *cmdPtr));
}


int Tcl_AppInit _ANSI_ARGS_((Tcl_Interp *interp));

# undef TCL_STORAGE_CLASS
# define TCL_STORAGE_CLASS DLLEXPORT

/*
 * The following code ensures that tclLink.c is linked whenever
 * Tcl is linked.  Without this code there's no reference to the
 * code in that file from anywhere in Tcl, so it may not be
 * linked into the application.
 */
// #ifdef _TCL85
// int (*tclDummyLinkVarPtr)(Tcl_Interp *interp, const char *a,
//                                 char *b, int c) = Tcl_LinkVar;
// #elif _TCL84
// int (*tclDummyLinkVarPtr)(Tcl_Interp *interp, const char *a,
//                                 char *b, int c) = Tcl_LinkVar;
// #else
// int (*tclDummyLinkVarPtr)(Tcl_Interp *interp, char *a,
//                                 char *b, int c) = Tcl_LinkVar;
// #endif

/*
 * Declarations for various library procedures and variables (don't want
 * to include tclPort.h here, because people might copy this file out of
 * the Tcl source directory to make their own modified versions).
 * Note:  "exit" should really be declared here, but there's no way to
 * declare it without causing conflicts with other definitions elsewher
 * on some systems, so it's better just to leave it out.
 */

#ifdef _WIN32
extern "C" int          isatty _ANSI_ARGS_((int fd));
//extern "C" char * strcpy _ANSI_ARGS_((char *dst, CONST char *src)) throw();
#endif
static char *tclStartupScriptFileName = NULL;

/*
 *----------------------------------------------------------------------
 *
 * TclSetStartupScriptFileName --
 *
 *          Primes the startup script file name, used to override the
 *      command line processing.
 *
 * Results:
 *          None. 
 *
 * Side effects:
 *          This procedure initializes the file name of the Tcl script to
 *      run at startup.
 *
 *----------------------------------------------------------------------
 */
void TclSetStartupScriptFileName(char *fileName)
{
    tclStartupScriptFileName = fileName;
}


/*
 *----------------------------------------------------------------------
 *
 * TclGetStartupScriptFileName --
 *
 *          Gets the startup script file name, used to override the
 *      command line processing.
 *
 * Results:
 *          The startup script file name, NULL if none has been set.
 *
 * Side effects:
 *          None.
 *
 *----------------------------------------------------------------------
 */
char *TclGetStartupScriptFileName()
{
    return tclStartupScriptFileName;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_Main --
 *
 *          Main program for tclsh and most other Tcl-based applications.
 *
 * Results:
 *          None. This procedure never returns (it exits the process when
 *          it's done.
 *
 * Side effects:
 *          This procedure initializes the Tcl world and then starts
 *          interpreting commands;  almost anything could happen, depending
 *          on the script being interpreted.
 *
 *----------------------------------------------------------------------
 */

static bool OPS_suppressOpenSeesOutput = false;
static bool OPS_showHeader = true;

int
main(int argc, char **argv)
{ 
    Tcl_Obj *resultPtr;
    Tcl_Obj *commandPtr = NULL;
    char buffer[1000], *args;
    int code, gotPartial, tty, length;
    int exitCode = 0;
    Tcl_Channel inChannel, outChannel, errChannel;
    Tcl_Interp *interp;
    Tcl_DString argString;

    interp = Tcl_CreateInterp();
    Tcl_InitStubs(interp, "8.6", 0);


    /* fmk - beginning of modifications for OpenSees */
    if (OPS_showHeader) {
        fprintf(stderr,"\n\n");
        fprintf(stderr,"         OpenSees -- Open System For Earthquake Engineering Simulation\n");
        fprintf(stderr,"                 Pacific Earthquake Engineering Research Center\n");
        // fprintf(stderr,"                        Version %s %s\n\n", OPS_VERSION, WIN_ARCH);
        
        fprintf(stderr,"      (c) Copyright 1999-2023 The Regents of the University of California\n");
        fprintf(stderr,"                              All Rights Reserved\n");
        fprintf(stderr,"  (Copyright and Disclaimer @ http://www.berkeley.edu/OpenSees/copyright.html)\n\n\n");
    }


    // Tcl_FindExecutable(argv[0]);

    Tcl_Eval(interp, "rename load import;");
    Tcl_Eval(interp, "interp alias {} load {} import;");


#ifdef TCL_MEM_DEBUG
    Tcl_InitMemory(interp);
#endif

    /*
     * Make command-line arguments available in the Tcl variables "argc"
     * and "argv".  If the first argument doesn't start with a "-" then
     * strip it off and use it as the name of a script file to process.
     */
    tclStartupScriptFileName = argv[1];
    if (tclStartupScriptFileName == NULL) {
          if ((argc > 1) && (argv[1][0] != '-')) {
              tclStartupScriptFileName = argv[1];
              argc--;
              argv++;
          }
    }

    args = Tcl_Merge(argc-1, argv+1);
    Tcl_ExternalToUtfDString(NULL, args, -1, &argString);
    Tcl_SetVar(interp, "argv", Tcl_DStringValue(&argString), TCL_GLOBAL_ONLY);
    Tcl_DStringFree(&argString);
    ckfree(args);


    if (tclStartupScriptFileName == NULL) {
          Tcl_ExternalToUtfDString(NULL, argv[0], -1, &argString);
    } else {
          tclStartupScriptFileName = Tcl_ExternalToUtfDString(NULL,
                    tclStartupScriptFileName, -1, &argString);
    }

    // TclFormatInt(buffer, argc-1);
    // Tcl_SetVar(interp, "argc", buffer, TCL_GLOBAL_ONLY);
    // Tcl_SetVar(interp, "argv0", Tcl_DStringValue(&argString), TCL_GLOBAL_ONLY);

    /*
     * Set the "tcl_interactive" variable.
     */

    tty = isatty(0);
    char one[2] = "1";
    char zero[2] = "0";

    Tcl_SetVar(interp, "tcl_interactive",
              ((tclStartupScriptFileName == NULL) && tty) ? one : zero,
              TCL_GLOBAL_ONLY);
    
    /*
     * Invoke application-specific initialization.
     */
    // TODO
    Tcl_Eval(interp, "import /home/claudio/opensees/OpenSeesRT/build/temp.linux-x86_64-cpython-39_stack/src/libg3/SRC/runtime/libOpenSeesRT.so");

//
//  if ((*appInitProc)(interp) != TCL_OK) {
//      errChannel = Tcl_GetStdChannel(TCL_STDERR);
//      if (errChannel) {
//          Tcl_WriteChars(errChannel,
//                    "application-specific initialization failed: ", -1);
//          Tcl_WriteObj(errChannel, Tcl_GetObjResult(interp));
//          Tcl_WriteChars(errChannel, "\n", 1);
//      }
//  }
//

    /*
     * If a script file was specified then just source that file
     * and quit.
     */

    if (tclStartupScriptFileName != NULL) {
      
//      if (numParam == 0)
          code = Tcl_EvalFile(interp, tclStartupScriptFileName);
//      else
//          code = EvalFileWithParameters(interp, tclStartupScriptFileName, 0, 0, 0, 1);
      
      if (code != TCL_OK) {
          errChannel = Tcl_GetStdChannel(TCL_STDERR);
          if (errChannel) {
            /*
             * The following statement guarantees that the errorInfo
             * variable is set properly.
             */
            
            Tcl_AddErrorInfo(interp, "");
            Tcl_WriteObj(errChannel, Tcl_GetVar2Ex(interp, "errorInfo",
                                                             NULL, TCL_GLOBAL_ONLY));
            Tcl_WriteChars(errChannel, "\n", 1);
          }
          exitCode = 1;
      }
      goto done;
    }
      /*
       * Process commands from stdin until there's an end-of-file.  Note
       * that we need to fetch the standard channels again after every
       * eval, since they may have been changed.
       */

      commandPtr = Tcl_NewObj();
      Tcl_IncrRefCount(commandPtr);
      
      inChannel = Tcl_GetStdChannel(TCL_STDIN);
      outChannel = Tcl_GetStdChannel(TCL_STDOUT);
      gotPartial = 0;

      while (1) {
          if (tty) {
            Tcl_Obj *promptCmdPtr;
            
            char one[12] = "tcl_prompt1";
            char two[12] = "tcl_prompt2";
            promptCmdPtr = Tcl_GetVar2Ex(interp,
                                               (gotPartial ? one : two),
                                               NULL, TCL_GLOBAL_ONLY);
            if (promptCmdPtr == NULL) {
            defaultPrompt:
              if (!gotPartial && outChannel) {
                Tcl_WriteChars(outChannel, "OpenSees > ", 11);
              }
            } else {
              
              code = Tcl_EvalObjEx(interp, promptCmdPtr, 0);
              
              inChannel = Tcl_GetStdChannel(TCL_STDIN);
              outChannel = Tcl_GetStdChannel(TCL_STDOUT);
              errChannel = Tcl_GetStdChannel(TCL_STDERR);
              if (code != TCL_OK) {
                if (errChannel) {
                    Tcl_WriteObj(errChannel, Tcl_GetObjResult(interp));
                    Tcl_WriteChars(errChannel, "\n", 1);
                }
                Tcl_AddErrorInfo(interp,
                                     "\n    (script that generates prompt)");
                goto defaultPrompt;
              }
            }
            if (outChannel) {
              Tcl_Flush(outChannel);
              }
          }
          if (!inChannel) {
            goto done;
          }
          length = Tcl_GetsObj(inChannel, commandPtr);
          if (length < 0) {
            goto done;
          }
          if ((length == 0) && Tcl_Eof(inChannel) && (!gotPartial)) {
            goto done;
          }
                
          /*
           * Add the newline removed by Tcl_GetsObj back to the string.
           */
          
          Tcl_AppendToObj(commandPtr, "\n", 1);
          // if (!TclObjCommandComplete(commandPtr)) {
          //   gotPartial = 1;
          //   continue;
          // }

          gotPartial = 0;
          code = Tcl_RecordAndEvalObj(interp, commandPtr, 0);
          inChannel = Tcl_GetStdChannel(TCL_STDIN);
          outChannel = Tcl_GetStdChannel(TCL_STDOUT);
          errChannel = Tcl_GetStdChannel(TCL_STDERR);
          Tcl_DecrRefCount(commandPtr);
          commandPtr = Tcl_NewObj();
          Tcl_IncrRefCount(commandPtr);
          if (code != TCL_OK) {
            if (errChannel) {
              Tcl_WriteObj(errChannel, Tcl_GetObjResult(interp));
              Tcl_WriteChars(errChannel, "\n", 1);
            }
          } else if (tty) {
            resultPtr = Tcl_GetObjResult(interp);
            Tcl_GetStringFromObj(resultPtr, &length);
            if ((length > 0) && outChannel) {
              Tcl_WriteObj(outChannel, resultPtr);
              Tcl_WriteChars(outChannel, "\n", 1);
            }
          }
#ifdef TCL_MEM_DEBUG
          if (tclMemDumpFileName != NULL) {
            Tcl_DecrRefCount(commandPtr);
            Tcl_DeleteInterp(interp);
            Tcl_Exit(0);
          }
#endif
      }

 done:

    if (commandPtr != NULL) {
      Tcl_DecrRefCount(commandPtr);
    }

#if defined(_PARALLEL_PROCESSING) || defined( _PARALLEL_INTERPRETERS)
    return;
#endif

    /*
     * Rather than calling exit, invoke the "exit" command so that
     * users can replace "exit" with some other command to do additional
     * cleanup on exit.  The Tcl_Eval call should never return.
     */
    Tcl_Eval(interp, buffer);

    Tcl_Eval(interp, "quit"); 

    return 0;
}
