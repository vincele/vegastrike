#include <stdlib.h>
#include <vector>
#include "collection.h"
#ifndef LIST_TESTING
#include "unit.h"
#endif

UnitCollection::UnitListNode::UnitListNode (Unit *unit):unit(unit), next(NULL){
  if (unit) {
    unit->Ref();
  }
}
UnitCollection::UnitListNode::UnitListNode(Unit *unit, UnitListNode *next) : unit(unit), next(next) { 
  if (unit) {
    unit->Ref();
  }
}

UnitCollection::UnitListNode::~UnitListNode() { 
  if(NULL!=unit) {
    unit->UnRef(); 	
  }
}
void UnitCollection::destr() {
  UnitListNode *tmp;
  while (u.next) {
    tmp = u.next;
    u.next = u.next->next;
    delete tmp;
  }  
}

void * UnitCollection::PushUnusedNode (UnitListNode * node) {
  static UnitListNode cat(NULL,NULL);
  static UnitListNode dog(NULL,&cat);
  static bool cachunk=true;
  if (cachunk) {
    cachunk=false;
    fprintf (stderr,"%x %x",&dog,&cat);
  }
  static std::vector <UnitCollection::UnitListNode * >dogpile;
  if (node==NULL) {
    return &dogpile;
  }else {
    node->next=&dog;
    dogpile.push_back (node);
  }
  return NULL;
}
void UnitCollection::FreeUnusedNodes () {
  std::vector<UnitCollection::UnitListNode *> *dogpile = (std::vector <UnitCollection::UnitListNode *> *)PushUnusedNode (NULL);
  while (!dogpile->empty()) {
    delete dogpile->back();
    dogpile->pop_back ();
  }
}
void UnitCollection::prepend(UnitIterator *iter) {
  UnitListNode *n = &u;
  Unit * tmp;
  while((tmp=iter->current())) {//iter->current checks for killed()
    n->next = new UnitListNode(tmp, n->next);
    iter->advance();
  }
}
void UnitCollection::append(UnitIterator *iter) {
  UnitListNode *n = &u;
  while(n->next->unit!=NULL) n = n->next;
  Unit * tmp;
  while((tmp=iter->current())) {
    n->next = new UnitListNode(tmp,n->next);
    n = n->next;
    iter->advance();
  }
}
void UnitCollection::append(Unit *unit) { 
  UnitListNode *n = &u;
  while(n->next->unit!=NULL) n = n->next;
  n->next = new UnitListNode(unit, n->next);
}	
void UnitCollection::UnitListNode::PostInsert (Unit * unit) {
  if(next->unit!=NULL)
    next->next = new UnitListNode(unit, next->next);
  else
    next = new UnitListNode(unit, next);
}
void UnitCollection::UnitIterator::postinsert(Unit *unit) {
  pos->PostInsert (unit);
}
void UnitCollection::FastIterator::postinsert(Unit *unit) {
  pos->PostInsert (unit);
}
void UnitCollection::UnitListNode::Remove () {
  if (next->unit) {
    UnitListNode *tmp = next->next;
    //    delete next; //takes care of unref! And causes a shitload of bugs
    //concurrent lists, man
    PushUnusedNode(next);
    next = tmp;
  }else {
    assert (0);
  }
}
void UnitCollection::UnitIterator::remove() {
  pos->Remove ();
}
void UnitCollection::FastIterator::remove() {
  pos->Remove ();
}


void UnitCollection::UnitIterator::GetNextValidUnit () {
  while (pos->next->unit?pos->next->unit->Killed():false) {
    remove();
  }
}


void UnitCollection::ConstIterator::GetNextValidUnit () {
  while (pos->next->unit?pos->next->unit->Killed():false) {
    pos = pos->next;
  }
}

const UnitCollection &UnitCollection::operator = (const UnitCollection & uc){
  printf ("warning could cause problems with concurrent lists. Make sure no one is traversing gotten list");
  destr();
  u=NULL;
  init();
  un_iter ui = createIterator();
  const UnitListNode * n = &uc.u;
  while (n) {
    if (n->unit) {
      ui.postinsert (n->unit);
      ++ui;
    }
    n = n->next;
  }
  return uc;
}
UnitCollection::UnitCollection (const UnitCollection& uc):u(NULL) {
  u=NULL;
  init();
  un_iter ui = createIterator();
  const UnitListNode * n = &uc.u;
  while (n) {
    if (n->unit) {
      ui.postinsert (n->unit);
      ++ui;
    }
    n = n->next;
  }
}
