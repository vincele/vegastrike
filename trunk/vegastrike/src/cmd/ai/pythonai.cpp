#include <stdio.h>
#ifdef USE_BOOST_129
#include <boost/python/class.hpp>
#else
#include <boost/python/class_builder.hpp>
#include <boost/python/detail/extension_class.hpp>
#endif
#include <Python.h>
#include <compile.h>
#include <eval.h>
#include "python/python_compile.h"
#include "config_xml.h"
#include "vs_globals.h"
#include "vs_path.h"
#include "pythonai.h"
using namespace Orders;
PythonAI * PythonAI::last_ai=NULL;
PythonAI::PythonAI(PyObject * self_, float reaction_time, float aggressivity):FireAt (reaction_time,aggressivity) {
  self = self_;
  // boost::python:
  Py_XINCREF(self);//by passing this to whoami, we are counting on them to Destruct us
  last_ai=this;
}
void PythonAI::Destruct() {
  Py_XDECREF(self);//this should destroy SELF
}
void PythonAI::default_Execute (FireAt &self_) {
  (self_).FireAt::Execute();
}
PythonAI * PythonAI::LastAI() {
  PythonAI * myai=last_ai;
  last_ai=NULL;
  return myai;
}
PythonAI * PythonAI::Factory (const std::string &filename) {
  CompileRunPython (filename);  
  return LastAI();
}
void PythonAI::Execute() {
  boost::python::callback <void>::call_method (self,"Execute");
}
void PythonAI::InitModuleAI () {
  boost::python::module_builder ai_builder("AI");
  boost::python::class_builder <FireAt,PythonAI> BaseClass (ai_builder,"FireAt");

  BaseClass.def (boost::python::constructor<float,float>());
  BaseClass.def (&FireAt::Execute,"PythonAI",PythonAI::default_Execute);
  
}
PythonAI::~PythonAI () {
  fprintf (stderr,"Destruct called. If called from C++ this is death %d (%x)",this,this);
}
