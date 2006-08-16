#include "faction_generic.h"
#include "unit_generic.h"
#include "pilot.h"
#include <vector>

Pilot::Pilot(int faction) {
  static float reaction = XMLSupport::parse_float (vs_config->getVariable ("AI","Firing","ReactionTime",".2"));
  static float ran = XMLSupport::parse_float(vs_config->getVariable ("AI","DefaultRank",".01"));
  this->rank = ran;

  this->reaction_time=reaction;
  this->faction=faction;
  comm_face=NULL;
  gender=0;
}
void Pilot::SetComm(Unit * parent) {
  this->faction=parent->faction;
  //GET BETTER REACTION TIME AND RANK HERE
  comm_face=FactionUtil::GetRandCommAnimation(faction,parent,gender);
}
float Pilot::getAnger(const Unit * target)const {
    relationmap::const_iterator iter=effective_relationship.find(target);
    float rel=0;
    if (iter!=effective_relationship.end())
      rel= iter->second;
    if (_Universe->isPlayerStarship(target)){
      if (FactionUtil::GetFactionName(faction).find("pirates")!=std::string::npos) {
        static unsigned int cachedCargoNum=0;
        static bool good=true;
        if (cachedCargoNum!=target->numCargo()) {
          cachedCargoNum=target->numCargo();
          good=true;
          for (unsigned int i=0;i<cachedCargoNum;++i) {
            Cargo * c=&target->image->cargo[i];
            if (c->quantity!=0&&c->category.find("upgrades")==string::npos){
              good=false;
              break;
            }
          }
        }
        if (good) {
          static float goodness_for_nocargo=XMLSupport::parse_float(vs_config->getVariable("AI","pirate_bonus_for_empty_hold",".75"));
          rel+=goodness_for_nocargo;
        }    
      }
      {
        int fac=faction;
        MapStringFloat::iterator mapiter=factions[fac]->ship_relation_modifier.find(target->name);
        if (mapiter!=factions[fac]->ship_relation_modifier.end()) {
          rel+=(*mapiter).second;
        }
      }
    }
    
    return rel;
}

float Pilot::GetEffectiveRelationship(const Unit * target)  const {
    return getAnger(target)+FactionUtil::GetIntRelation(faction,target->faction);    
}

extern float myroundclamp(float i);
Animation * Pilot::getCommFace(Unit * parent, float mood, unsigned char & sex) {
  vector <Animation *> *ani = getCommFaces (sex);
  if (ani==NULL) {
    ani = FactionUtil::GetRandCommAnimation(parent->faction,parent,sex);
    if (ani==NULL) {
      return NULL;
    }
  }
  if (ani->size()==0) {
    return NULL;
  }
  mood+=.1;
  mood*=(ani->size())/.2;
  unsigned int index=(unsigned int)myroundclamp(floor(mood));
  if (index>=ani->size()) {
    index=ani->size()-1;
  }
  return (*ani)[index];
}
