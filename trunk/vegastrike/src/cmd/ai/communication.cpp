#include "communication.h"
#include <assert.h>
FSM::FSM (const char * filename) {
    //loads a conversation finite state machine with deltaRelation weight transition from an XML?
  if (strlen(filename)==0) {
    nodes.push_back (Node("welcome to cachunkcachunk.com",0));
    nodes.push_back (Node("I love you!",.1));
    nodes.push_back (Node("J00 0wnz m3",.08));
    nodes.push_back (Node("You are cool!",.06));
    nodes.push_back (Node("You are nice!",.05));
    nodes.push_back (Node("Ya you're naled! NALED PAL!",-.02));
    nodes.push_back (Node("i 0wnz j00",-.08));
    nodes.push_back (Node("I hate you!",-.1));

    nodes.push_back (Node("Prepare To Be Searched. Maintain Speed and Course.",0));
    nodes.push_back (Node("No contraband detected: You may proceed.",0));
    nodes.push_back (Node("Contraband detected! All units close and engage!",0));
    nodes.push_back (Node("Your Course is deviating! Maintain Course!",0));
    nodes.push_back (Node("Request Clearence To Land.",0));
    nodes.push_back (Node("*hit*",-.2));
    vector <unsigned int> edges;
    unsigned int i;
    for (i=0;i<nodes.size()-6;i++) {
      edges.push_back (i);
    }
    for (i=0;i<nodes.size();i++) {
      nodes[i].edges = edges;
    }
  } else {
    LoadXML(filename);
  }
}
int FSM::GetContrabandInitiateNode() {
  return nodes.size()-6;
}
int FSM::GetContrabandUnDetectedNode() {
  return nodes.size()-5;
}
int FSM::GetContrabandDetectedNode() {
  return nodes.size()-4;
}
int FSM::GetContrabandWobblyNode() {
  return nodes.size()-3;
}


int FSM::GetRequestLandNode () {
  return nodes.size()-2;
}
int FSM::GetHitNode () {
  return nodes.size()-1;
}
static float sq (float i) {return i*i;}
bool nonneg (float i) {return i>=0;}
int FSM::getCommMessageMood (int curstate, float mood, float randomresponse) const{
  const FSM::Node *n = &nodes[curstate];
    mood+=-randomresponse+2*randomresponse*((float)rand())/RAND_MAX;
  
  int choice=0;
  float bestchoice=4;
  bool fitmood=false;
  for (unsigned i=0;i<n->edges.size();i++) {
    float md = nodes[n->edges[i]].messagedelta;
    bool newfitmood=nonneg(mood)==nonneg(md);
    if ((!fitmood)||newfitmood) {
      float newbestchoice=sq(md-mood);
      if ((newbestchoice<=bestchoice)||(fitmood==false&&newfitmood==true)) {
	if ((newbestchoice==bestchoice&&rand()%2)||newbestchoice<bestchoice) {
	  //to make sure some variety happens
	  fitmood=newfitmood;
	  choice =i;
	  bestchoice = newbestchoice;
	}
      }
    }
  }
  return choice;

}
int FSM::getDefaultState (float relationship) const{
  return nodes[0].edges[getCommMessageMood (0,relationship,.01)];
}
std::string FSM::GetEdgesString (int curstate) {
  std::string retval="\n";
  for (unsigned int i=0;i<nodes[curstate].edges.size();i++) {
    retval+= tostring ((int)((i+1)%10))+"."+nodes[nodes[curstate].edges[i]].message+"\n";
  }
  retval+= "0 Request Docking Clearence";
  return retval;
}
float FSM::getDeltaRelation (int prevstate, int current_state) const{
  return nodes[current_state].messagedelta;
}

void CommunicationMessage::Init (Unit * send, Unit * recv) {
  fsm = _Universe->GetConversation (send->faction,recv->faction);
  sender.SetUnit (send);
  this->prevstate=this->curstate = fsm->getDefaultState(_Universe->GetRelation(send->faction,recv->faction));
}
void CommunicationMessage::SetAnimation (std::vector <Animation *>*ani) {
  if (ani){ 
    if (ani->size()>0) {
	float mood= fsm->getDeltaRelation(this->prevstate,this->curstate);
	mood+=1;
	mood*=ani->size()/2.;
	unsigned int index=(unsigned int)mood;
	if (index>=ani->size()) {
	  index=ani->size()-1;
	}
	this->ani=(*ani)[index];
    } else {
      this->ani=NULL;
    }
  }else {
    this->ani=NULL;
  }
}

void CommunicationMessage::SetCurrentState (int msg,std::vector <Animation *>*ani) {
  curstate = msg;
  SetAnimation(ani);
  assert (this->curstate>=0);
}

CommunicationMessage::CommunicationMessage (Unit * send, Unit * recv, int messagechoice, std::vector <Animation *>* ani) {
  Init (send,recv);
  prevstate=fsm->getDefaultState (_Universe->GetRelation (send->faction,recv->faction));
  if (fsm->nodes[prevstate].edges.size()) {
    curstate = fsm->nodes[prevstate].edges[messagechoice%fsm->nodes[prevstate].edges.size()];
  }
  SetAnimation(ani);
  assert (this->curstate>=0);

}
CommunicationMessage::CommunicationMessage (Unit * send, Unit * recv, int laststate, int thisstate, std::vector <Animation *>* ani) {
  Init (send,recv);
  prevstate=laststate;
  curstate = thisstate;
  SetAnimation(ani);
    assert (this->curstate>=0);

}
CommunicationMessage::CommunicationMessage (Unit * send, Unit * recv,std::vector<Animation *>* ani) {
  Init (send,recv);
  SetAnimation(ani);
  assert (this->curstate>=0);

}
CommunicationMessage::CommunicationMessage (Unit * send, Unit * recv, const CommunicationMessage &prevstate, int curstate, std::vector<Animation *>* ani) {
  Init (send,recv);
  this->prevstate = prevstate.curstate;
  if (fsm->nodes[this->prevstate].edges.size()) {
    this->curstate = fsm->nodes[this->prevstate].edges[curstate%fsm->nodes[this->prevstate].edges.size()];
  }
  SetAnimation(ani);
  assert (this->curstate>=0);

}
