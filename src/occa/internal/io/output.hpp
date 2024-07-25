#ifndef OCCA_INTERNAL_IO_OUTPUT_HEADER
#define OCCA_INTERNAL_IO_OUTPUT_HEADER

#include <iostream>
#include <sstream>

#if (OCCA_OS == OCCA_WINDOWS_OS)
#ifdef stdout
#undef stdout
#endif
#ifdef stderr
#undef stderr
#endif
#endif


namespace occa {
  namespace io {
    typedef void (*outputFunction_t)(const char *str);

    class output {
    private:
      std::ostream &out;
      std::stringstream ss;
      outputFunction_t overrideOut;

    public:
      output(std::ostream &out_);

      void setOverride(outputFunction_t overrideOut_);

      template <class TM>
      output& operator << (const TM &t);
    };

    template <>
    output& output::operator << (const std::string &str);
    template <>
    output& output::operator << (char * const &c);
    template <>
    output& output::operator << (const char &c);

    extern output stdout;
    extern output stderr;
    inline auto endl = "\n";
  }
}

#include "output.tpp"

#endif
