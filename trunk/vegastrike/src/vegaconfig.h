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
  xml Configuration written by Alexander Rawass <alexannika@users.sourceforge.net>
*/

#ifndef _VEGACONFIG_H_
#define _VEGACONFIG_H_

#include <expat.h>
#include <string>
#include "xml_support.h"
#include "easydom.h"

using std::string;

using XMLSupport::AttributeList;

class VegaConfig {
 public:
  VegaConfig(char *configfile);

  void getColor(char *name, float color[4]);
  string getVariable(string section,string name,string defaultvalue);

 private:
  string getVariable(easyDomNode *section,string name,string defaultval);

  easyDomNode *variables;

  bool checkConfig(easyDomNode *node);
  void doVariables(easyDomNode *node);
  void doBindings(easyDomNode *node);
  void checkSection(easyDomNode *node);
  void checkVar(easyDomNode *node);
  void doColors(easyDomNode *node);

};

#endif // _VEGACONFIG_H_
