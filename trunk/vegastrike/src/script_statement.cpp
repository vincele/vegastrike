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

#ifndef WIN32
// this file isn't available on my system (all win32 machines?) i dun even know what it has or if we need it as I can compile without it
#include <unistd.h>
#endif

#include <expat.h>
#include "xml_support.h"

#include "vegastrike.h"

#include "mission.h"
#include "easydom.h"

//#include "vs_globals.h"
//#include "vegastrike.h"

void Mission::checkStatement(missionNode *node,int mode){
    if(node->tag==DTAG_IF){
      doIf(node,mode);
    }
    else if(node->tag==DTAG_BLOCK){
      doBlock(node,mode);
    }
    else if(node->tag==DTAG_SETVAR){
      doSetVar(node,mode);
    }
    else if(node->tag==DTAG_EXEC){
      doExec(node,mode);
    }
    else if(node->tag==DTAG_CALL){
      doCall(node,mode);
    }
    else if(node->tag==DTAG_WHILE){
      doWhile(node,mode);
    }
}

void Mission::doIf(missionNode *node,int mode){
  if(mode==SCRIPT_PARSE){
    vector<easyDomNode *>::const_iterator siter;
  
    int i=0;
    for(siter= node->subnodes.begin() ; siter!=node->subnodes.end() && i<3; siter++){
      missionNode *snode=(missionNode *)*siter;
      node->script.if_block[i]=snode;
    }
    if(i<3){
      fatalError("an if-statement needs exact three subnodes");
    }
  }

  bool ok=checkBoolExpr(node->script.if_block[0],mode);

  if(mode==SCRIPT_PARSE){
    checkStatement(node->script.if_block[1],mode);
    checkStatement(node->script.if_block[2],mode);
  }
  else{
    if(ok){
      checkStatement(node->script.if_block[1],mode);
    }
    else{
      checkStatement(node->script.if_block[2],mode);
    }
  }
}


void Mission::doWhile(missionNode *node,int mode){

  if(SCRIPT_PARSE){
    int i=0;
   vector<easyDomNode *>::const_iterator siter;
    for(siter= node->subnodes.begin() ; siter!=node->subnodes.end() && i<2; siter++){
      missionNode *snode=(missionNode *)*siter;
      node->script.while_arg[i]=snode;
    }

    if(i<2){
      fatalError("a while-expr needs exact two subnodes");
      assert(0);
    }

    bool res=checkBoolExpr(node->script.while_arg[0],mode);

    checkStatement(node->script.while_arg[1],mode);
  }
  else{
    // runtime
    while(checkBoolExpr(node->script.while_arg[0],mode)){
      checkStatement(node->script.while_arg[1],mode);
    }
  }
}
