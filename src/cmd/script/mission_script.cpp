/* 
 * Vega Strike
 * Copyright (C) 2001-2002 Daniel Horn
 * 
 * http://vegastrike.sourceforge.net/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

/*
  xml Mission Scripting written by Alexander Rawass <alexannika@users.sourceforge.net>
*/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>
#include <assert.h>
#ifndef WIN32
// this file isn't available on my system (all win32 machines?) i dun even know what it has or if we need it as I can compile without it
#include <unistd.h>
#endif

#include <expat.h>
#include "xml_support.h"

#include "vegastrike.h"

#include "mission.h"
#include "easydom.h"

#include "vs_globals.h"
#include "config_xml.h"

//#include "vegastrike.h"


void Mission::DirectorStart(missionNode *node){

  cout << "DIRECTOR START" << endl;

  debuglevel=atoi(vs_config->getVariable("interpreter","debuglevel","0").c_str());

  missionThread *main_thread=new missionThread;
  runtime.thread_nr=0;
  runtime.threads.push_back(main_thread);
  runtime.cur_thread=main_thread;

  parsemode=PARSE_DECL;

  doModule(node,SCRIPT_PARSE);

  easyDomFactory<missionNode> *importf=new easyDomFactory<missionNode>();



  while(import_stack.size()>0){
    missionNode *import=import_stack.back();
    import_stack.pop_back();

    missionNode *module=runtime.modules[import->script.name];
    if(module==NULL){
      debug(3,node,SCRIPT_PARSE,"loading module "+import->script.name);

      string filename="modules/"+import->script.name+".module";

      missionNode *import_top=importf->LoadXML(filename.c_str());

      if(import_top==NULL){
	fatalError(node,SCRIPT_PARSE,"could not load module file "+filename);
	assert(0);
      }

      import_top->Tag(&tagmap);

      doModule(import_top,SCRIPT_PARSE);

    }
    else{
      debug(3,node,SCRIPT_PARSE,"already have module "+import->script.name);
    }
  }



  parsemode=PARSE_FULL;

  doModule(node,SCRIPT_PARSE);

  map<string,missionNode *>::iterator iter;
  //=runtime.modules.begin()

  for(iter=runtime.modules.begin();iter!=runtime.modules.end();iter++){
    string mname=(*iter).first ;
    missionNode *mnode=(*iter).second;

    if(mname!="director"){
      cout << "  parsing full module " << mname << endl;
      doModule(mnode,SCRIPT_PARSE);
    }
  }


  if(director==NULL){
    return;
  }


  missionNode *initgame=director->script.scripts["initgame"];

  if(initgame==NULL){
    warning("initgame not found");
  }
  else{
    runtime.cur_thread->module_stack.push_back(director);

    varInst *vi=doScript(initgame,SCRIPT_RUN);

    runtime.cur_thread->module_stack.pop_back();
  }

  while(true){
    DirectorLoop();
#ifndef _WIN32
    sleep(1);
#endif
  }
}

void Mission::DirectorLoop(){

  if(director==NULL){
    return;
  }

  cout << "DIRECTOR LOOP" << endl;

  saveVariables(cout);

  missionNode *gameloop=director->script.scripts["gameloop"];

  if(gameloop==NULL){
    warning("no gameloop");
    return;
  }
  else{
    runtime.cur_thread->module_stack.push_back(director);

    varInst *vi=doScript(gameloop,SCRIPT_RUN);

    runtime.cur_thread->module_stack.pop_back();
  
    //    doModule(director,SCRIPT_RUN);
  }
}


/* *********************************************************** */

string Mission::modestring(int mode){
  if(mode==SCRIPT_PARSE){
    return "parse";
  }
  else{
    return "run";
  }
}
void Mission::fatalError(missionNode *node,int mode,string message){
  cout << "fatal (" << modestring(mode) << ") " << message << " : ";
  printNode(node,mode);
}

void Mission::runtimeFatal(string message){
  cout << "runtime fatalError: " << message << endl;
}

void Mission::warning(string message){
  cout << "warning: " << message << endl;
}


void Mission::debug(int level,missionNode *node,int mode,string message){
  if(level<=debuglevel){
    debug(node,mode,message);
  }
}

void Mission::debug(missionNode *node,int mode,string message){

  cout << "debug (" << modestring(mode) << ") " << message << " : " ;
  printNode(node,mode);
  //  cout << endl;
}

void Mission::printNode(missionNode *node,int mode){
  if(node){
    node->printNode(cout,0,0);
  }
}

/* *********************************************************** */

void Mission::doImport(missionNode *node,int mode){
  if(mode==SCRIPT_PARSE && parsemode==PARSE_DECL){
    string name=node->attr_value("name");
    if(name.empty()){
      fatalError(node,mode,"you have to give a name to import");
      assert(0);
    }
    node->script.name=name;
    import_stack.push_back(node);
  }

}

void Mission::doModule(missionNode *node,int mode){
  if(mode==SCRIPT_PARSE){
      string name=node->attr_value("name");
    if(parsemode==PARSE_DECL){

  
      if(name.empty()){
	fatalError(node,mode,"you have to give a module name");
	assert(0);
      }

      if(runtime.modules[name]!=NULL){
	fatalError(node,mode,"there can only be one module with name "+name);
	assert(0);
      }

      if(name=="director"){
	director=node;
      }
 
      node->script.name=name;

      runtime.modules[name]=node; // add this module to the list of known modules
    }

    scope_stack.push_back(node);

    current_module=node;

    debug(5,node,mode,"added module "+name+" to list of known modules");
  }

  if(mode==SCRIPT_RUN){
    // SCRIPT_RUN
    runtime.cur_thread->module_stack.push_back(node);
  }


  vector<easyDomNode *>::const_iterator siter;
  
  for(siter= node->subnodes.begin() ; siter!=node->subnodes.end() ; siter++){
    missionNode *snode=(missionNode *)*siter;
    if(snode->tag==DTAG_SCRIPT){
      varInst *vi=doScript(snode,mode);
    }
    else if(snode->tag==DTAG_DEFVAR){
      doDefVar(snode,mode);
    }
    else if(snode->tag==DTAG_GLOBALS){
      doGlobals(snode,mode);
    }
    else if(snode->tag==DTAG_IMPORT){
      doImport(snode,mode);
    }
    
    else{
      fatalError(node,mode,"unkown node type");
      assert(0);
    }

  }

  if(mode==SCRIPT_PARSE){
    scope_stack.pop_back();
  }
  else{
    runtime.cur_thread->module_stack.pop_back();
  }
}

/* *********************************************************** */

scriptContext *Mission::makeContext(missionNode *node){
  scriptContext *context=new scriptContext;

  context->varinsts=new varInstMap;

  return context;
}

/* *********************************************************** */

void Mission::removeContextStack(){
  runtime.cur_thread->exec_stack.pop_back();
}

void Mission::addContextStack(missionNode *node){
  contextStack *cstack=new contextStack;

  cstack->return_value=NULL;

  runtime.cur_thread->exec_stack.push_back(cstack);
}

scriptContext *Mission::addContext(missionNode *node)
{

  scriptContext *context=makeContext(node);
  contextStack *stack=runtime.cur_thread->exec_stack.back();
  stack->contexts.push_back(context);

  debug(5,node,SCRIPT_RUN,"added context for this node");
  printRuntime();

  return context;
}

/* *********************************************************** */

void Mission::removeContext()
{
  contextStack *stack=runtime.cur_thread->exec_stack.back();

  int lastelem=stack->contexts.size()-1;

  scriptContext *old=stack->contexts[lastelem];
  stack->contexts.pop_back();

 
  delete old;
}

/* *********************************************************** */

varInst * Mission::doScript(missionNode *node,int mode, varInstMap *varmap){
  if(mode==SCRIPT_PARSE){
    current_script=node;

    if(parsemode==PARSE_DECL){
      node->script.name=node->attr_value("name");

      if(node->script.name.empty()){
	fatalError(node,mode,"you have to give a script name");
      }
      current_module->script.scripts[node->script.name]=node;
      node->script.nr_arguments=0;

      string retvalue=node->attr_value("return");
      if(retvalue.empty()){
	node->script.vartype=VAR_VOID;
      }
      else{
	node->script.vartype=vartypeFromString(retvalue);
      }
    }
    scope_stack.push_back(node);

    
  }

  debug(5,node,mode,"executing script name="+node->script.name);

  if(mode==SCRIPT_RUN){
    addContextStack(node);
    addContext(node);
  }

  vector<easyDomNode *>::const_iterator siter;

  node->script.nr_arguments=0;
  node->script.argument_node=NULL;

  for(siter= node->subnodes.begin() ; siter!=node->subnodes.end() && !have_return(mode) ; siter++){
    missionNode *snode=(missionNode *)*siter;
    if(snode->tag==DTAG_ARGUMENTS){
      doArguments(snode,mode,varmap);
      if(mode==SCRIPT_PARSE && parsemode==PARSE_DECL){
	node->script.argument_node=snode;
      }
    }
    else{
      if(mode==SCRIPT_PARSE && parsemode==PARSE_DECL){
	// do nothing, break here
      }
      else{
	checkStatement(snode,mode);
      }
    }
  }

  if(mode==SCRIPT_RUN){
    removeContext();
    contextStack *cstack=runtime.cur_thread->exec_stack.back();
    varInst *vi=cstack->return_value;
    if(vi!=NULL){
      if(node->script.vartype!=vi->type){
	fatalError(node,mode,"doScript: return type not set correctly");
	assert(0);
      }
    }
    else{
      // vi==NULL
      if(node->script.vartype!=VAR_VOID){
	fatalError(node,mode,"no return set from doScript");
	assert(0);
      }
    }
    removeContextStack();

    return vi;
  }
  else{
    scope_stack.pop_back();
    return NULL;
  }
}


void Mission::doArguments(missionNode *node,int mode,varInstMap *varmap){

  int nr_arguments=0;

  if(mode==SCRIPT_PARSE){
    if(parsemode==PARSE_DECL){
      vector<easyDomNode *>::const_iterator siter;
  
      for(siter= node->subnodes.begin() ; siter!=node->subnodes.end() ; siter++){
	missionNode *snode=(missionNode *)*siter;
	if(snode->tag==DTAG_DEFVAR){
	  doDefVar(snode,mode);
	  nr_arguments++;
	}
	else{
	  fatalError(node,mode,"only defvars allowed below argument node");
	  assert(0);
	}
      }
    
      node->script.nr_arguments=nr_arguments;
    }
  }

  nr_arguments=node->script.nr_arguments;

  if(mode==SCRIPT_RUN){
  if(varmap){
    int   nr_called=varmap->size();

    if(nr_arguments!=nr_called){
      fatalError(node,mode,"wrong number of args in doScript ");
      assert(0);
    }

    for(int i=0;i<nr_arguments;i++){
      missionNode *defnode=(missionNode *)node->subnodes[i];
      
      doDefVar(defnode,mode);
      varInst *vi=doVariable(defnode,mode);
      
      varInst *call_vi=(*varmap)[defnode->script.name];

      if(call_vi==NULL){
	fatalError(node,mode,"argument var "+node->script.name+" no found in varmap");
	assert(0);
      }
      assignVariable(vi,call_vi);
    }
  }
  else{
    // no varmap == 0 args
    if(nr_arguments!=0){
      fatalError(node,mode,"doScript expected to be called with arguments");
      assert(0);
    }
  }
  }

  if(mode==SCRIPT_PARSE){
    if(parsemode==PARSE_DECL){
      missionNode *exec_scope=scope_stack.back();
      exec_scope->script.nr_arguments=nr_arguments;
      node->script.nr_arguments=nr_arguments;
    }
  }
  
}


void Mission::doReturn(missionNode *node,int mode){
  if(mode==SCRIPT_PARSE){
    missionNode *script=current_script;

    node->script.exec_node=script;
    
}
  int len=node->subnodes.size();
  varInst *vi=new varInst;

  missionNode *script=node->script.exec_node;

  if(script->script.vartype==VAR_VOID){
    if(len!=0){
      fatalError(node,mode,"script returning void, but return statement with node");
      assert(0);
    }
  }
  else{
    // return something non-void

    if(len!=1){
      fatalError(node,mode,"return statement needs only one subnode");
      assert(0);
    }

    missionNode *expr=(missionNode *)node->subnodes[0];

    if(script->script.vartype==VAR_BOOL){
      bool res=checkBoolExpr(expr,mode);
      vi->bool_val=res;
    }
    else if(script->script.vartype==VAR_FLOAT){
      float res=checkFloatExpr(expr,mode);
      vi->float_val=res;
    }
    else if(script->script.vartype==VAR_OBJECT){
      varInst *vi2=checkObjectExpr(expr,mode);
      assignVariable(vi,vi2);
    }
    else{
      fatalError(node,mode,"unkown variable type");
      assert(0);
    }
  }

  if(mode==SCRIPT_RUN){

    contextStack *cstack=runtime.cur_thread->exec_stack.back();

    vi->type=script->script.vartype;

    cstack->return_value=vi;
  }

}

/* *********************************************************** */

bool Mission::have_return(int mode){
  if(mode==SCRIPT_PARSE){
    return false;
  }

  contextStack *cstack=runtime.cur_thread->exec_stack.back();
  if(cstack->return_value==NULL){
    return false;
  }
  
  return true;
}
  
void Mission::doBlock(missionNode *node,int mode){
  if(mode==SCRIPT_PARSE){
    scope_stack.push_back(node);
  }
  if(mode==SCRIPT_RUN){
    addContext(node);
 }

    vector<easyDomNode *>::const_iterator siter;
  
  for(siter= node->subnodes.begin() ; siter!=node->subnodes.end() && !have_return(mode); siter++){
    missionNode *snode=(missionNode *)*siter;
    checkStatement(snode,mode);
  }
  if(mode==SCRIPT_RUN){
    removeContext();
  }
    if(mode==SCRIPT_PARSE){
      scope_stack.pop_back();
    }
}

/* *********************************************************** */

bool Mission::checkVarType(varInst *var,enum var_type check_type){
  if(var->type==check_type){
    return true;
  }
  return false;
}

/* *********************************************************** */

bool Mission::doBooleanVar(missionNode *node,int mode){
  varInst *var=doVariable(node,mode);

  bool ok=checkVarType(var,VAR_BOOL);

  if(!ok){
    fatalError(node,mode,"expected a bool variable - got a different type");
    assert(0);
  }

  return var->bool_val;
}
/* *********************************************************** */

float Mission::doFloatVar(missionNode *node,int mode){
  varInst *var=doVariable(node,mode);

  bool ok=checkVarType(var,VAR_FLOAT);

  if(!ok){
    fatalError(node,mode,"expected a float variable - got a different type");
    assert(0);
  }

  return var->float_val;
}

varInst * Mission::doObjectVar(missionNode *node,int mode){
  varInst *var=doVariable(node,mode);

  bool ok=checkVarType(var,VAR_OBJECT);

  if(!ok){
    fatalError(node,mode,"expected a object variable - got a different type");
    assert(0);
  }

  
  return var;
}

/* *********************************************************** */

varInst *Mission::lookupLocalVariable(missionNode *asknode){
  contextStack *cstack=runtime.cur_thread->exec_stack.back();
  varInst *defnode=NULL;

  for(unsigned int i=0;i<cstack->contexts.size() && defnode==NULL;i++){
    scriptContext *context=cstack->contexts[i];
    varInstMap *map=context->varinsts;
    defnode=(*map)[asknode->script.name];
    if(defnode!=NULL){
      debug(5,defnode->defvar_node,SCRIPT_RUN,"FOUND local variable defined in that node");
    }
  }
  if(defnode==NULL){
    return NULL;
  }

  return defnode;
}

/* *********************************************************** */

varInst *Mission::lookupModuleVariable(string mname,missionNode *asknode){
  // only when runtime
  missionNode *module_node=runtime.modules[mname];

  if(module_node==NULL){
    fatalError(asknode,SCRIPT_RUN,"no such module named "+mname);
    assert(0);
    return NULL;
  }

  vector<easyDomNode *>::const_iterator siter;
  
  for(siter= module_node->subnodes.begin() ; siter!=module_node->subnodes.end() ; siter++){
    missionNode *varnode=(missionNode *)*siter;
    if(varnode->script.name==asknode->script.name){
      char buffer[200];
      sprintf(buffer,"FOUND module variable %s in that node",varnode->script.name.c_str());
      debug(4,varnode,SCRIPT_RUN,buffer);
      printVarInst(varnode->script.varinst);

      return varnode->script.varinst;
    }
  }

  return NULL;

}

/* *********************************************************** */

varInst *Mission::lookupModuleVariable(missionNode *asknode){
  // only when runtime
  missionNode *module=runtime.cur_thread->module_stack.back();

  string mname=module->script.name;

  varInst *var=lookupModuleVariable(mname,asknode);


  return var;

}

/* *********************************************************** */

varInst *Mission::lookupGlobalVariable(missionNode *asknode){
  missionNode *varnode=runtime.global_variables[asknode->script.name];

  if(varnode==NULL){
    return NULL;
  }

  return varnode->script.varinst;
}


void Mission::doGlobals(missionNode *node,int mode){

  if(mode==SCRIPT_RUN || (mode==SCRIPT_PARSE && parsemode==PARSE_FULL)){
    // nothing to do
    return;
  }

  debug(3,node,mode,"doing global variables");

  vector<easyDomNode *>::const_iterator siter;

  for(siter= node->subnodes.begin() ; siter!=node->subnodes.end() && !have_return(mode) ; siter++){
    missionNode *snode=(missionNode *)*siter;
    if(snode->tag==DTAG_DEFVAR){
      doDefVar(snode,mode,true);
    }
    else{
      fatalError(node,mode,"only defvars allowed below globals node");
      assert(0);
    }
  }

}

/* *********************************************************** */

varInst *Mission::doVariable(missionNode *node,int mode){
  if(mode==SCRIPT_RUN){
    varInst *var=lookupLocalVariable(node);
    if(var==NULL){
      // search in module namespace
      var=lookupModuleVariable(node);
      if(var==NULL){
	// search in global namespace
	var=lookupGlobalVariable(node);
	if(var==NULL){
	  fatalError(node,mode,"did not find variable");
	  assert(0);
	}
      }
    }
    return var;
  }
  else{
    // SCRIPT_PARSE
    node->script.name=node->attr_value("name");
    if(node->script.name.empty()){
      fatalError(node,mode,"you have to give a variable name");
      assert(0);
    }

    varInst *vi=searchScopestack(node->script.name);

    if(vi==NULL){
      missionNode *global_var=runtime.global_variables[node->script.name];
      if(global_var==NULL){
	fatalError(node,mode,"no variable "+node->script.name+" found on the scopestack (dovariable)");
	assert(0);
      }
      vi=global_var->script.varinst;
    }

    return vi;


  }
}

/* *********************************************************** */

void Mission::doDefVar(missionNode *node,int mode,bool global_var){
  if(mode==SCRIPT_RUN){
    missionNode *scope=node->script.context_block_node;
    if(scope->tag==DTAG_MODULE){
      // this is a module variable - it has been initialized at parse time
      debug(4,node,mode,"defined module variable "+node->script.name);
      return;
    }

    debug(5,node,mode,"defining context variable "+node->script.name);

    contextStack *stack=runtime.cur_thread->exec_stack.back();
    scriptContext *context=stack->contexts.back();


    varInstMap *vmap=context->varinsts;

    varInst *vi=new varInst;
    vi->defvar_node=node;
    vi->block_node=scope;
    vi->type=node->script.vartype;
    vi->name=node->script.name;

    (*vmap)[node->script.name]=vi;

    printRuntime();

    return;
  }

  // SCRIPT_PARSE

  node->script.name=node->attr_value("name");
    if(node->script.name.empty()){
      fatalError(node,mode,"you have to give a variable name");
      assert(0);
    }

    string value=node->attr_value("init");

    debug(5,node,mode,"defining variable "+node->script.name);

    string type=node->attr_value("type");
    //    node->initval=node->attr_value("init");

    node->script.vartype=vartypeFromString(type);

    missionNode *scope=NULL;

    if(global_var==false){
      scope=scope_stack.back();
    }
    
  varInst *vi=new varInst;
  vi->defvar_node=node;
  vi->block_node=scope;
  vi->type=node->script.vartype;
  vi->name=node->script.name;

  if(global_var || scope->tag==DTAG_MODULE){
    if(!value.empty()){
      debug(4,node,mode,"setting init for "+node->script.name);
      if(vi->type==VAR_FLOAT){
	vi->float_val=atof(value.c_str());
      }
      else if(vi->type==VAR_BOOL){
	if(value=="true"){
	vi->bool_val=true;
	}
	else if(value=="false"){
	  vi->bool_val=false;
	}
	else{
	  fatalError(node,mode,"wrong bool value");
	  assert(0);
	}
      }
      else{
	fatalError(node,mode,"this datatype can;t be initialized");
	assert(0);
      }
      printVarInst(vi);
    }
  }

  node->script.varinst=vi;//FIXME (not for local var)

  if(global_var){
    debug(3,node,mode,"defining global variable");
    runtime.global_variables[node->script.name]=node;
    printGlobals(3);
  }
  else{
    scope->script.variables[node->script.name]=vi;
    node->script.context_block_node=scope;
    debug(5,scope,mode,"defined variable in that scope");
  }

}


void Mission::doSetVar(missionNode *node,int mode){

  if(mode==SCRIPT_PARSE){
    node->script.name=node->attr_value("name");
    if(node->script.name.empty()){
      fatalError(node,mode,"you have to give a variable name");
    }
  }

  debug(5,node,mode,"trying to set variable "+node->script.name);

  //    varInst *var_expr=checkExpression((missionNode *)node->subnodes[0],mode);

  if(node->subnodes.size()!=1){
    fatalError(node,mode,"setvar takes exactly one argument");
    assert(0);
  }

  missionNode *expr=(missionNode *)node->subnodes[0];

  if(mode==SCRIPT_PARSE){
    varInst *vi=searchScopestack(node->script.name);

    if(vi==NULL){
      missionNode *global_var=runtime.global_variables[node->script.name];
      if(global_var==NULL){

	fatalError(node,mode,"no variable "+node->script.name+" found on the scopestack (setvar)");
	assert(0);
      }
      
      vi=global_var->script.varinst;
    }

    if(vi->type==VAR_BOOL){
      bool res=checkBoolExpr(expr,mode);
    }
    else if(vi->type==VAR_FLOAT){
      float res=checkFloatExpr(expr,mode);
    }
    else if(vi->type==VAR_OBJECT){
      varInst *ovi=checkObjectExpr(expr,mode);
    }

  }

    if(mode==SCRIPT_RUN){
      varInst *var_inst=doVariable(node,mode); // lookup variable instance

      if(var_inst==NULL){
	fatalError(node,mode,"variable lookup failed for "+node->script.name);
	printRuntime();
	assert(0);
      }
    if(var_inst->type==VAR_BOOL){
      bool res=checkBoolExpr(expr,mode);
      var_inst->bool_val=res;
    }
    else if(var_inst->type==VAR_FLOAT){
      float res=checkFloatExpr(expr,mode);
      var_inst->float_val=res;
    }
    else if(var_inst->type==VAR_OBJECT){
      varInst *ovi=checkObjectExpr(expr,mode);
      assignVariable(var_inst,ovi);
    }
    else{
      fatalError(node,mode,"unsupported datatype");
      assert(0);
    }

#if 0
      if(var_expr->type != var_inst->type){
	runtimeFatal("variable "+node->script.name+" is not of the correct type\n");
	assert(0);
      }

      assignVariable(var_inst,var_expr);
      
      delete var_expr; // only temporary
#endif
    }
  
}

varInst *Mission::doConst(missionNode *node,int mode){
  if(mode==SCRIPT_PARSE){
    //string name=node->attr_value("name");
    string typestr=node->attr_value("type");
    string valuestr=node->attr_value("value");

    if(typestr.empty() || valuestr.empty()){
      fatalError(node,mode,"no valid const declaration");
      assert(0);
    }
    
    debug(5,node,mode,"parsed const value "+valuestr);

    varInst *vi=new varInst;
    if(typestr=="float"){
      node->script.vartype=VAR_FLOAT;
      vi->float_val=atof(valuestr.c_str());
    }
    else if(typestr=="bool"){
      node->script.vartype=VAR_BOOL;
      if(valuestr=="true"){
	vi->bool_val=true;
      }
      else if(valuestr=="false"){
	vi->bool_val=false;
      }
      else{
	fatalError(node,mode,"wrong bool value");
	assert(0);
      }
    }
#if 0
    else if(typestr=="string"){
      node->script.vartype=VAR_STRING;
    }
    else if(typestr=="vector"){
      node->script.vartype=VAR_VECTOR;
    }
#endif
    else if(typestr=="object"){
      fatalError(node,mode,"you cant have a const object");
      assert(0);
    }
    else{
      fatalError(node,mode,"unknown variable type");
      assert(0);
    }

    vi->type=node->script.vartype;

    node->script.varinst=vi;
    
  }

  return node->script.varinst;
    

}

varInst *Mission::doExec(missionNode *node,int mode){
  if(mode==SCRIPT_PARSE){
    string name=node->attr_value("name");
    if(name.empty()){
      fatalError(node,mode,"you have to give name to exec");
      assert(0);
    }
    node->script.name=name;

    string use_modstr=node->attr_value("module");
    missionNode *module=NULL;
    missionNode *script=NULL;
    if(!use_modstr.empty()){
      //missionNode *use_script=lookupScript(name,use_modstr);
      
      module=runtime.modules[use_modstr];
    }
    else{
      module=current_module;
    }

    if(module==NULL){
      fatalError(node,mode,"module "+use_modstr+" not found");
      assert(0);
    }
      script=module->script.scripts[name];

      if(script==NULL){
	fatalError(node,mode,"script "+name+" not found in module "+use_modstr);
	assert(0);
      }
    
    node->script.exec_node=script;
    node->script.vartype=script->script.vartype;
    node->script.module_node=module;
  }
  
  missionNode *arg_node=node->script.exec_node->script.argument_node;

  int nr_arguments;

  if(arg_node==NULL){
    nr_arguments=0;
  }
  else{
    nr_arguments=arg_node->script.nr_arguments;
  }
  int nr_exec_args=node->subnodes.size();

  if(nr_arguments!=nr_exec_args){
    fatalError(node,mode,"wrong nr of arguments in doExec");
    assert(0);
  }

  varInstMap *varmap=NULL;
 if(nr_arguments>0){
  varmap=new varInstMap;

  for(int i=0;i<nr_arguments;i++){
    missionNode *defnode=(missionNode *)arg_node->subnodes[i];
    missionNode *callnode=(missionNode *)node->subnodes[i];

    varInst *vi=new varInst;
    vi->type=defnode->script.vartype;


    if(defnode->script.vartype==VAR_FLOAT){
      debug(4,node,mode,"doExec checking floatExpr");
      float res=checkFloatExpr(callnode,mode);
      vi->float_val=res;
    }
    else if(defnode->script.vartype==VAR_BOOL){
      debug(4,node,mode,"doExec checking boolExpr");
      bool ok=checkBoolExpr(callnode,mode);
      vi->bool_val=ok;
    }
    else if(defnode->script.vartype==VAR_OBJECT){
      debug(4,node,mode,"doExec checking objectExpr");
      varInst *ovi=checkObjectExpr(callnode,mode);
      assignVariable(vi,ovi);
    }
    else{
      fatalError(node,mode,"unsupported vartype in doExec");
      assert(0);
    }

    (*varmap)[defnode->script.name]=vi;
    
  }
 }

  if(mode==SCRIPT_RUN){
    // SCRIPT_RUN

    debug(4,node,mode,"executing "+node->script.name);

    missionNode *module=node->script.module_node;

    runtime.cur_thread->module_stack.push_back(module);

    varInst *vi=doScript(node->script.exec_node,mode,varmap);

    runtime.cur_thread->module_stack.pop_back();

    delete varmap;
    return vi;
  }

  // SCRIPT_PARSE

  varInst *vi=new varInst;

  vi->type=node->script.exec_node->script.vartype;

  return vi;
}


void Mission::initTagMap(){
  tagmap["module"]=DTAG_MODULE;
  tagmap["script"]=DTAG_SCRIPT;
  tagmap["if"]=DTAG_IF;
  tagmap["block"]=DTAG_BLOCK;
  tagmap["setvar"]=DTAG_SETVAR;
  tagmap["exec"]=DTAG_EXEC;
  tagmap["call"]=DTAG_CALL;
  tagmap["while"]=DTAG_WHILE;
  tagmap["and"]=DTAG_AND_EXPR;
  tagmap["or"]=DTAG_OR_EXPR;
  tagmap["not"]=DTAG_NOT_EXPR;
  tagmap["test"]=DTAG_TEST_EXPR;
  tagmap["fmath"]=DTAG_FMATH;
  tagmap["vmath"]=DTAG_VMATH;
  tagmap["var"]=DTAG_VAR_EXPR;
  tagmap["defvar"]=DTAG_DEFVAR;
  tagmap["const"]=DTAG_CONST;
  tagmap["arguments"]=DTAG_ARGUMENTS;
  tagmap["globals"]=DTAG_GLOBALS;
  tagmap["return"]=DTAG_RETURN;
  tagmap["import"]=DTAG_IMPORT;
}

void Mission::assignVariable(varInst *v1,varInst *v2){
  if(v1->type!=v2->type){
    fatalError(NULL,SCRIPT_RUN,"wrong types in assignvariable");
    assert(0);
  }
  if(v1->type==VAR_OBJECT){
    if(v1->objectname!=v2->objectname){
      fatalError(NULL,SCRIPT_RUN,"wrong object types in assignment");
      assert(0);
    }
  }
  v1->float_val=v2->float_val;
  v1->bool_val=v2->bool_val;
  v1->objectname=v2->objectname;
  v1->object=v2->object;
}

varInst *Mission::checkExpression(missionNode *node,int mode){

  varInst *ret=NULL;
  debug(0,node,mode,"checking expression");
  printRuntime();

  switch(node->tag){
  case DTAG_AND_EXPR:
  case DTAG_OR_EXPR:
  case DTAG_NOT_EXPR:
  case DTAG_TEST_EXPR:
    checkBoolExpr(node,mode);
    break;

  case DTAG_VAR_EXPR:
    ret=doVariable(node,mode);
    break;
  case DTAG_FMATH:
    break;
  default:
    fatalError(node,mode,"no such expression");
    assert(0);
    break;
  }
  return ret;
}
void Mission::printVarInst(varInst *vi){

  return;
}
void Mission::saveVarInst(varInst *vi,ostream& my_out){

    char buffer[100];
    if(vi==NULL){
      sprintf(buffer," NULL");
    }
    else{
      if(vi->type==VAR_BOOL){
	sprintf(buffer,"type=\"bool\" value=\"%d\" ",vi->bool_val);
      }
      else if(vi->type==VAR_FLOAT){
	sprintf(buffer,"type=\"float\"  value=\"%f\" ",vi->float_val);
      }
    }
    my_out  << buffer ;
}

void Mission::printVarmap(const varInstMap & vmap){
  map<string,varInst *>::const_iterator iter;

  for(iter=vmap.begin();iter!=vmap.end();iter++){
    cout << "variable " << (*iter).first ;
    varInst *vi=(*iter).second;

    printVarInst(vi);
  }
}

void Mission::printRuntime(){
  return;
  cout << "RUNTIME" << endl;
  cout << "MODULES:" << endl;

  map<string,missionNode *>::iterator iter;
  //=runtime.modules.begin()

  for(iter=runtime.modules.begin();iter!=runtime.modules.end();iter++){
    cout << "  module " << (*iter).first ;
    printNode((*iter).second,0);
  }


  cout << "CURRENT THREAD:" << endl;

  printThread(runtime.cur_thread);
}

void Mission::printGlobals(int dbg_level){
  if(dbg_level>debuglevel){
    return;
  }

  map<string,missionNode *>::iterator iter;

  for(iter=runtime.global_variables.begin();iter!=runtime.global_variables.end();iter++){
    cout << "  global var " << (*iter).first ;
    printNode((*iter).second,0);
  }

  
}

void Mission::printThread(missionThread *thread){
  return;
      vector<contextStack *>::const_iterator siter;

    for(siter= thread->exec_stack.begin() ; siter!=thread->exec_stack.end() ; siter++){
      contextStack *stack= *siter;
      
      vector<scriptContext *>::const_iterator iter2;

      cout << "SCRIPT CONTEXTS" << endl;

      for(iter2=stack->contexts.begin(); iter2!=stack->contexts.end() ; iter2++){
	scriptContext *context= *iter2;

	cout << "VARMAP " << endl;
	printVarmap(*(context->varinsts));
      }
    }
}

varInst *Mission::searchScopestack(string name){

  int elem=scope_stack.size()-1;
  varInst *vi=NULL;

  while(vi==NULL && elem>=0){
    missionNode *scope=scope_stack[elem];

    vi=scope->script.variables[name];

    if(vi==NULL){
      debug(5,scope,0,"variable "+name+" not found in that scope");
      //printVarmap(scope->script.variables);
    }
    else{
      debug(5,scope,0,"variable "+name+" FOUND in that scope");
      //printVarmap(scope->script.variables);
    }
    elem--;
  };
  
  return vi;
}

var_type Mission::vartypeFromString(string type){
  var_type vartype;

    if(type=="float"){
      vartype=VAR_FLOAT;
    }
    else if(type=="bool"){
      vartype=VAR_BOOL;
    }
#if 0
    else if(type=="string"){
      vartype=VAR_STRING;
    }
    else if(type=="vector"){
      vartype=VAR_VECTOR;
    }
#endif
    else if(type=="object"){
      vartype=VAR_OBJECT;
    }
    else{
      fatalError(NULL,SCRIPT_PARSE,"unknown var type "+type);
      vartype=VAR_FAILURE;
    }
    return vartype;

}

missionNode *Mission::lookupScript(string scriptname,string modulename){
  missionNode *module=runtime.modules[modulename];
  if(module==NULL){
    fatalError(module,SCRIPT_PARSE,"module "+modulename+" not found - maybe you forgot to import it?");
    assert(0);
  }

  missionNode *scriptnode=module->script.scripts[scriptname];
  if(scriptnode==NULL){
    fatalError(module,SCRIPT_PARSE,"script "+scriptname+" not found in module "+modulename);
    assert(0);
  }

  return scriptnode;
}

void Mission::saveVariables(ostream& out){

  out << "<saved-variables>" << endl << endl;

  out << "    <globals>" << endl;
  map<string,missionNode *>::iterator iter;

  for(iter=runtime.global_variables.begin();iter!=runtime.global_variables.end();iter++){
    string name=(*iter).first;
    missionNode *gnode=(*iter).second;

    varInst *vi=gnode->script.varinst;

    //    out << "      <defvar name=\"" << name << "\" type=\"" << typestr << "\" value=\"" << valuestr << "\" />" << endl;

    out << "      <defvar name=\"" << name << "\" ";

    saveVarInst(vi,out);

    out << "/> " << endl;


  }

  out << "    </globals>" << endl << endl;

  {
  // modules
  map<string,missionNode *>::iterator iter;

  for(iter=runtime.modules.begin();iter!=runtime.modules.end();iter++){
    string mname=(*iter).first ;
    missionNode *module_node=(*iter).second;

    out << "    <module name=\"" << mname << "\" >" << endl;



    vector<easyDomNode *>::const_iterator siter;
  
    for(siter= module_node->subnodes.begin() ; siter!=module_node->subnodes.end() ; siter++){

    missionNode *varnode=(missionNode *)*siter;
    if(varnode->tag==DTAG_DEFVAR){
      // found a module var node
      out << "      <defvar name=\"" << varnode->script.name << "\" ";
      saveVarInst(varnode->script.varinst,out);
      out << "/> " << endl;
    }
  }

    out << "    </module>" << endl << endl;




  }
  }
  

  out << endl << "</saved-variables>" << endl;

  

}

