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







#ifndef _CMD_ORDER_H

#define _CMD_ORDER_H

#include "gfx/vec.h"
#include "cmd/collection.h"
#include <list>

#include <cmd/script/mission.h>

/**
 * Order is the base class for all orders.  All orders inherit from Order
 * Each fighter may have an order they are currently executing. Orders naturally
 * can queue other orders, and decide which orders may be executed in parallel by those orders' bit codes.
 * Orders affect their "parent" and may call any function within that parent
 * Limitation (perhaps resolvable) Orders do not "know" what their parent
 * is until Execute() time.  Perhaps "SetParent" Should be made virtual so that
 * various variables may be set earlier than at every execution time
 */
//#define ORDERDEBUG

class Order {
 private:

 protected:
  virtual ~Order ();
  ///The unit this order is attached to
  Unit * parent;
  ///The bit code (from ORDERTYPES) that this order is (for parallel execution)
  unsigned int type; 
  
  unsigned int subtype;
  ///Whether or not this order is done
  bool done;
  ///If this order applies to a group of units (as in form up with this group)
  UnitContainer group;
  ///If this order applies to a physical location in world space
  QVector targetlocation;
  ///The queue of suborders that will be executed in parallel according to bit code
  std::vector<Order*> suborders;
  ///a bunch of communications that have not been answered CommunicationMessages are actually containing reference to a nice Finite State Machine that can allow a player to have a reasonable conversation with an AI
  std::list<class CommunicationMessage *>messagequeue;
  ///changes the local relation of this unit to another...may inform superiors about "good" or bad! behavior depending on the AI
  virtual void AdjustRelationTo (Unit * un, float factor);
  virtual void Destructor();
 protected:
  /// this function calls the destructor (needs to be overridden for python;

 public:

  virtual float GetEffectiveRelationship (const Unit * target) const;
  ///clears the messasges of this order
  void ClearMessages();
  ///The varieties of order types  MOVEMENT,FACING, and WEAPON orders may not be mutually executed (lest one engine goes left, the other right)
  enum ORDERTYPES { MOVEMENT =1, FACING = 2, WEAPON = 4, CLOAKING=8, ALLTYPES=(1|2|4|8)};
  enum SUBORDERTYPES {SLOCATION=1, STARGET=2, SSELF=4};
  ///The default constructor setting everything to NULL and no dependency on order
  Order (): targetlocation(0,0,0){parent = NULL;type=0;subtype=0,done=false; actionstring=""; VSCONSTRUCT1('O')}
  ///The constructor that specifies what order dependencies this order has
  Order(int ttype,int subtype): targetlocation(0,0,0){parent = NULL;type = ttype;done=false; actionstring=""; VSCONSTRUCT1('O')}
  ///The virutal function that unrefs all memory then calls Destruct () which takes care of unreffing this or calling delete on this
  virtual void Destroy();

  ///The function that gets called and executes all queued suborders 
  virtual void Execute();
  ///returns a pointer to the first order that may be bitwised ored with that type
  Order* queryType (unsigned int type);
  ///Erases all orders that bitwise OR with that type
  void eraseType (unsigned int type);
  ///Attaches a group of targets to this order (used for strategery-type games)
  bool AttachOrder (Unit *targets);
  ///Attaches a navigation point to this order
  bool AttachOrder (QVector target);
  ///Attaches a group (form up) to this order
  bool AttachSelfOrder (Unit *targets);
  ///Enqueues another order that will be executed (in parallel perhaps) when next void Execute() is called
  Order* EnqueueOrder (Order * ord);
  ///Replaces the first order of that type in the order queue
  Order* ReplaceOrder (Order * ord);
  bool Done() {return done;}
  int getType() {return type;}
  int getSubType () {return subtype;}
  ///Sets the parent of this Unit.  Any virtual functions must call this one
  virtual void SetParent(Unit *parent1) {parent = parent1;};
  Unit * GetParent() {return parent;}
  ///Sends a communication message from the Unit (encapulated in c) to this unit
  virtual void Communicate (const class CommunicationMessage &c);
  ///processes a single message...generally called by the Messages() func
  virtual void ProcessCommMessage(class CommunicationMessage &c);
  ///responds (or does not) to certain messages in the message queue
  virtual void ProcessCommunicationMessages(float CommRepsonseTime, bool RemoveMessageProcessed);
  /// return pointer to order or NULL if not found
  Order *findOrder(Order *ord);
  /// erase that order from the list
  void eraseOrder(Order *ord);
  /// enqueue order as first order
  Order* EnqueueOrderFirst (Order *ord);
  /// returns the orderlist (NULL for orders that haven't got any)
  virtual olist_t* getOrderList(){ return NULL;};

  virtual string getOrderDescription() { return "empty"; };

  ///searches the suborders recursively for the first order that has an orderlist
  Order *findOrderList();
  string createFullOrderDescription(int level=0);
  void setActionString(string astring) { actionstring=astring; };
  string getActionString() { return actionstring; };
  Animation * getCommFace(float mood, unsigned char & sex);
 protected:
  virtual std::vector <Animation *>* getCommFaces(unsigned char &sex);
  string  actionstring;
};
///Convenience order factory for "clicking to create an order"
class OrderFactory {
public:
  virtual int type () {return 0;}
  OrderFactory(){}
  virtual Order * newOrder() {return new Order;}
};
///Executes another order for a number of seconds
class ExecuteFor:  public Order {

 private:

  ///The child order to execute
  Order * child;
  ///the time it has executed the child order for
  float time;
  ///the total time it can execute child order
  float maxtime;
 protected:
  virtual ~ExecuteFor () {}
 public:

  ExecuteFor (Order * chld, float seconds): Order(chld->getType(),chld->getSubType()),child(chld),time(0),maxtime(seconds) {}
  ///Executes child order and then any suborders that may be pertinant
  void Execute ();
  ///Removes this order
  virtual void Destroy () {
    child->Destroy();
    Order::Destroy();
  }

};

#endif
