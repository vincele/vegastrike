#ifndef _CMD_KEYBOARD_AI_H_
#define _CMD_KEYBOARD_AI_H_
#include "in.h"
#include "order.h"
#include "event_xml.h"
#include "communication.h"
//all unified AI's should inherit from FireAt, so they can choose targets together.


class FireKeyboard: public Order {
  unsigned char sex;
  bool itts;
  bool cloaktoggle;
  bool refresh_target;
  float gunspeed;
  float gunrange;
  float rxntime;
  float delay;
  bool ShouldFire(Unit * targ);
  std::list <CommunicationMessage> resp;
 public:
  static void PressComm1Key (int,KBSTATE);
  static void PressComm2Key (int,KBSTATE);
  static void PressComm3Key (int,KBSTATE);
  static void PressComm4Key (int,KBSTATE);
  static void PressComm5Key (int,KBSTATE);
  static void PressComm6Key (int,KBSTATE);
  static void PressComm7Key (int,KBSTATE);
  static void PressComm8Key (int,KBSTATE);
  static void PressComm9Key (int,KBSTATE);
  static void PressComm10Key (int,KBSTATE);
  static void RequestClearenceKey(int, KBSTATE);
  static void UnDockKey(int, KBSTATE);
  static void EjectKey(int, KBSTATE);
  static void EjectCargoKey(int, KBSTATE);
  static void DockKey(int, KBSTATE);
  static void FireKey(int, KBSTATE);
  static void MissileKey(int, KBSTATE);
  static void NearestTargetKey (int, KBSTATE);
  static void ThreatTargetKey (int, KBSTATE);
  static void TargetKey(int, KBSTATE);
  static void ReverseTargetKey(int, KBSTATE);
  static void PickTargetKey(int, KBSTATE);
  static void NearestTargetTurretKey (int, KBSTATE);
  static void ThreatTargetTurretKey (int, KBSTATE);
  static void TargetTurretKey(int, KBSTATE);
  static void PickTargetTurretKey(int, KBSTATE);
  static void JFireKey(KBSTATE,float,float,int);  
  static void JMissileKey(KBSTATE,float,float,int);  
  static void JTargetKey(KBSTATE,float,float,int);  
  static void WeapSelKey (int,KBSTATE);
  static void MisSelKey (int, KBSTATE);
  static void CloakKey (int, KBSTATE);
  static void LockKey (int, KBSTATE);
  static void ECMKey (int,KBSTATE);
  static void HelpMeOut (int,KBSTATE);
  static void HelpMeOutFaction (int,KBSTATE);

  static void HelpMeOutCrit (int,KBSTATE);
  static void JoinFg (int,KBSTATE);
  static void BreakFormation (int,KBSTATE);
  static void FormUp (int,KBSTATE);
  static void AttackTarget (int,KBSTATE);
  static void TurretAI (int,KBSTATE);
protected:
  float distance;

  void ChooseTargets(bool targetturrets);//chooses n targets and puts the best to attack in unit's target container
  void ChooseRTargets(bool targetturrets);//chooses n targets and puts the best to attack in unit's target container
  void ChooseNearTargets(bool targetturrets);//chooses n targets and puts the best to attack in unit's target container
  void ChooseThreatTargets(bool targetturrets);//chooses n targets and puts the best to attack in unit's target container
  void PickTargets(bool targetturrets); // chooses the target which is nearest to the center of the screen
  unsigned int whichplayer;
  unsigned int whichjoystick;
  struct FIREKEYBOARDTYPE &f();
  struct FIREKEYBOARDTYPE &j();
public:
  virtual void ProcessCommMessage (class CommunicationMessage&c);
  FireKeyboard (unsigned int whichjoystick, unsigned int whichplayer);//weapon prefs?
  virtual void Execute();
  virtual ~FireKeyboard();
};




#endif


