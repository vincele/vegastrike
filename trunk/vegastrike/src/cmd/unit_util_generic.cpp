#include <string>
#include "cmd/unit_generic.h"
#include "cmd/unit_util.h"
#include "configxml.h"
#include "vs_globals.h"
#include "gfx/cockpit_generic.h"
#include "universe_util.h"
using std::string;

namespace UnitUtil {
	string getFactionName (Unit *my_unit) {
		if (!my_unit)return "";
		return FactionUtil::GetFaction(my_unit->faction);
    }
	int getFactionIndex (Unit *my_unit) {
		if (!my_unit)return 0;
		return my_unit->faction;
    }
	void setFactionIndex (Unit *my_unit,int factionname) {
		if (!my_unit)return;
		my_unit->SetFaction(factionname);
    }
	void setFactionName (Unit *my_unit,string factionname) {
		if (!my_unit)return;
		my_unit->SetFaction(FactionUtil::GetFaction(factionname.c_str()));
    }
	string getName(Unit *my_unit){
		if (!my_unit)return "";
		return my_unit->name;
	}
	float getCredits(Unit *my_unit) {
		if (!my_unit)return 0;
		Cockpit * tmp;
		float viret=0;
		if ((tmp=_Universe->isPlayerStarship (my_unit))) {
			viret=tmp->credits;
		}
		return viret;
	}
	void addCredits(Unit *my_unit,float credits) {
		if (!my_unit)return;
		Cockpit * tmp;
		if ((tmp=_Universe->isPlayerStarship (my_unit))) {
			tmp->credits+=credits;
		}
	}
    string getFlightgroupName(Unit *my_unit){
		if (!my_unit)return "";
		string fgname;
		Flightgroup *fg=my_unit->getFlightgroup();
		if(fg){
			fgname=fg->name;
		}
		return fgname;
	}
	Unit *getFlightgroupLeader (Unit *my_unit) {
		if (!my_unit)return 0;
		class Flightgroup * fg=my_unit->getFlightgroup();
		Unit *ret_unit = fg?fg->leader.GetUnit():my_unit;
		if (!ret_unit)
			ret_unit=0;
		return ret_unit;
	}
	bool setFlightgroupLeader (Unit *my_unit, Unit *un) {
		if (!my_unit||!un)return false;		 
		if (my_unit->getFlightgroup()) {
			my_unit->getFlightgroup()->leader.SetUnit(un);
			return true;
		} else {
			return false;
		}
	}
	string getFgDirective(Unit *my_unit){
		if (!my_unit)return "";
		string fgdir ("b");
		if (my_unit->getFlightgroup())
			fgdir = my_unit->getFlightgroup()->directive;
		return fgdir;
	}
	bool setFgDirective(Unit *my_unit,string inp){
		if (!my_unit)return false;
		if (my_unit->getFlightgroup()!=NULL) {
			my_unit->getFlightgroup()->directive = inp;
			return true;
		}
		return false;
	}
	int getFgSubnumber(Unit *my_unit){
		if (!my_unit)return -1;
		return my_unit->getFgSubnumber();
	}
	int removeCargo(Unit *my_unit,string s, int quantity, bool erasezero){
		if (!my_unit)return 0;
		unsigned int index;
		if (my_unit->GetCargo(s,index)) {
			quantity = my_unit->RemoveCargo (index,quantity,erasezero);
		}else {
			quantity=0;
		}
		return quantity;
	}
	float upgrade(Unit *my_unit, string file,int mountoffset,int subunitoffset, bool force,bool loop_through_mounts) {
		if (!my_unit)return 0;
		double percentage=0;
		printf ("upgrading %s %s %d %d %s\n",my_unit->name.c_str(),file.c_str(),mountoffset, subunitoffset,loop_through_mounts?"true":"false");
		fflush (stdout);
		percentage=my_unit->Upgrade (file,mountoffset,subunitoffset,force,loop_through_mounts);
		my_unit->SetTurretAI();
		return percentage;
	}
	int addCargo (Unit *my_unit,Cargo carg) {
	  if (!my_unit)return 0;
	  int i;
	  for (i=carg.quantity;i>0&&!my_unit->CanAddCargo(carg);i--) {
	    carg.quantity=i;
	  }
	  if (i>0) {
	    carg.quantity=i;
	    my_unit->AddCargo(carg);
	  }else {
	    carg.quantity=0;
	  }
	  return carg.quantity; 
	}
	int hasCargo (Unit * my_unit, string mycarg) {
	  if (!my_unit) return 0;
		unsigned int i;
		Cargo * c = my_unit->GetCargo (mycarg,i);
		if (c==NULL)
			return 0;
		return c->quantity;
	}
	bool JumpTo (Unit * unit, string system) {
		if (unit!=NULL)
			return unit->getStarSystem()->JumpTo(unit,NULL,system);
		else
			return false;
	}
        string getUnitSystemFile (Unit * un) {
	  if (!un) {
	    return _Universe->activeStarSystem()->getFileName();
	  }
	  StarSystem * ss = un->getStarSystem();
	  return ss->getFileName();
        }
	bool incrementCargo(Unit *my_unit,float percentagechange,int quantity){
		if (!my_unit)return false;
		if (my_unit->numCargo()>0) {
			unsigned int index;
			index = rand()%my_unit->numCargo();
			Cargo c(my_unit->GetCargo(index));	  
			c.quantity=quantity;
			if (my_unit->CanAddCargo(c)) {
				my_unit->AddCargo(c);
				my_unit->GetCargo(index).price*=percentagechange;
				return true;
			}
		}
		return false;
	}
	bool decrementCargo(Unit *my_unit,float percentagechange){
		if (!my_unit)return false;
		if (my_unit->numCargo()>0) {
			unsigned int index;
			index = rand()%my_unit->numCargo();
			if (my_unit->RemoveCargo(index,1,false)) {
				my_unit->GetCargo(index).price*=percentagechange;
			}
			return true;
		}
		return false;
	}


  
	Cargo GetCargoIndex (Unit *my_unit, int index) {
	  if (my_unit) {
	    if (index>=0&&(unsigned int)index<my_unit->numCargo()) {
	      return my_unit->GetCargo(index);
	    }
	  }
	  Cargo ret;
	  ret.quantity=0;
	  return ret;
	}

	Cargo GetCargo (Unit *my_unit, std::string cargname) {
		if (my_unit) {
			unsigned int indx=0;
			Cargo *cargptr=my_unit->GetCargo(cargname,indx);
			if (cargptr&&indx>=0) {
				return *cargptr;
			}
		}
		Cargo ret;
		ret.quantity=0;
		return ret;
	}
	float getDistance(Unit *my_unit,Unit *un){
	  if (my_unit==NULL||un==NULL)
	    return FLT_MAX;
	  return (my_unit->Position()-un->Position()).Magnitude()-my_unit->rSize()-un->rSize();
	}
	float getSignificantDistance (Unit *un, Unit *sig) {
	  if (un==NULL||sig==NULL)
	    return FLT_MAX;
	  
		float dist = getDistance (un,sig);
		
		float planetpct=UniverseUtil::getPlanetRadiusPercent();
		if (sig->isPlanet ())
			dist = dist - (sig->rSize()*planetpct);
		if (un->isPlanet ())
			dist = dist - (un->rSize()*planetpct);
		return dist;
	}
        int isPlayerStarship (Unit * un) {
                Cockpit * cp = _Universe->isPlayerStarship (un);
                if (cp==NULL) {
                        return -1;
                }
                return (cp - _Universe->AccessCockpit(0));
        }
  void setSpeed (Unit * my_unit, float speed) {
    if (my_unit) {
      my_unit->GetComputerData().set_speed = speed;
    }
  }
  void setECM (Unit * my_unit, short NewECM) {
    if (!my_unit)
      return;
    my_unit->GetImageInformation().ecm = NewECM;
  }
  short getECM (Unit * my_unit) {
    if (!my_unit)
      return 0;
    return my_unit->GetImageInformation().ecm;
  }
	static bool ishere (Unit *par,Unit *look) {
		for (un_iter uniter=par->getSubUnits();uniter.current();uniter.advance()) {
			if (uniter.current()==look) {
				return true;
			}
			if (uniter.current()!=par&&ishere(uniter.current(),look)) {
				return true;
			}
		}
		return false;
	}
	Unit *owner (Unit *un) {
		Unit *found=NULL;
		for (un_iter uniter=UniverseUtil::getUnitList();uniter.current();uniter.advance()) {
			if (uniter.current()==un||ishere(uniter.current(),un)) {
				found=uniter.current();
				break;
			}
		}
		return found;
	}
}
