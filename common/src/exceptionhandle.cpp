#ifndef _EXCEPTIONHANDLE_CPP
#define _EXCEPTIONHANDLE_CPP

#include "common/exceptionhandle.h"

SignalTranslator<FloatingPointException> g_objFloatingPointExceptionTranslator;
SignalTranslator<SegmentationFault> g_objSegmentationFaultTranslator;

#endif //_EXCEPTIONHANDLE_CPP
