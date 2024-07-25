#include <stdarg.h>

#include <occa/internal/c/types.hpp>
#include <occa/c/io.h>
#include <occa/internal/io/output.hpp>
#if (OCCA_OS == OCCA_WINDOWS_OS)
#ifdef stdout
#undef stdout
#endif
#ifdef stderr
#undef stderr
#endif
#endif
OCCA_START_EXTERN_C

void occaOverrideStdout(occaIoOutputFunction_t out) {
  occa::io::stdout.setOverride(out);
}

void occaOverrideStderr(occaIoOutputFunction_t out) {
  occa::io::stderr.setOverride(out);
}

OCCA_END_EXTERN_C
