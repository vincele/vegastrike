#include "base.h"
#include "gldrv/winsys.h"
#include "vs_path.h"
#include "lin_time.h"
#include "audiolib.h"
#ifdef BASE_MAKER
 #include <stdio.h>
 #ifdef _WIN32
  #include <windows.h>
 #endif
static char makingstate=0;
#endif


Base::Room::~Room () {
	int i;
	for (i=0;i<links.size();i++) {
		delete links[i];
	}
	for (i=0;i<objs.size();i++) {
		delete objs[i];
	}
}

Base::Room::Room () {
//		Do nothing...
}

void Base::Room::BaseObj::Draw (Base *base) {
//		Do nothing...
}

void Base::Room::BaseSprite::Draw (Base *base) {
	GFXBlendMode(SRCALPHA,INVSRCALPHA);
	GFXEnable(TEXTURE0);
	spr.Draw();
}

void Base::Room::BaseShip::Draw (Base *base) {
	Unit *un=base->caller.GetUnit();
	if (un) {
		GFXHudMode (GFXFALSE);
		GFXEnable (DEPTHTEST);
		GFXEnable (DEPTHWRITE);
		Vector p,q,r;
		_Universe->AccessCamera()->GetOrientation (p,q,r);
		QVector pos =  _Universe->AccessCamera ()->GetPosition();
		Matrix cam (p.i,p.j,p.k,q.i,q.j,q.k,r.i,r.j,r.k,pos);
		Matrix final;
		Matrix newmat = mat;
		newmat.p.i*=un->rSize();
		newmat.p.j*=un->rSize();
		newmat.p.k*=un->rSize();
		newmat.p+=QVector(0,0,g_game.znear);

		MultMatrix (final,cam,newmat);
		un->DrawNow(final);
		GFXDisable (DEPTHTEST);
		GFXDisable (DEPTHWRITE);
		GFXHudMode (GFXTRUE);
	}
}

void Base::Room::Draw (Base *base) {
	int i;
	for (i=0;i<objs.size();i++) {
		objs[i]->Draw(base);
	}
}

void Base::Room::BaseTalk::Draw (Base *base) {
/*	GFXColor4f(1,1,1,1);
	GFXBegin(GFXLINESTRIP);
		GFXVertex3f(caller->x,caller->y,0);
		GFXVertex3f(caller->x+caller->wid,caller->y,0);
		GFXVertex3f(caller->x+caller->wid,caller->y+caller->hei,0);
		GFXVertex3f(caller->x,caller->y+caller->hei,0);
		GFXVertex3f(caller->x,caller->y,0);
	GFXEnd();*/
	if (curchar<caller->say[sayindex].size()) {
		curtime+=GetElapsedTime();
		if (curtime>.025) {
			base->othtext.SetText(caller->say[sayindex].substr(0,++curchar));
			curtime=0;
		}
	} else {
		curtime+=GetElapsedTime();
		if (curtime>.01*caller->say[sayindex].size()) {
			curtime=0;
			caller->Click(base,0,0,WS_LEFT_BUTTON,WS_MOUSE_UP);
		}
	}
}

int Base::Room::MouseOver (Base *base,float x, float y) {
	for (int i=0;i<links.size();i++) {
		if (x>=links[i]->x&&
				x<=(links[i]->x+links[i]->wid)&&
				y>=links[i]->y&&
				y<=(links[i]->y+links[i]->hei)) {
			return i;
		}
	}
	return -1;
}

Base *Base::CurrentBase=0;
bool Base::CallComp=false;

bool RefreshGUI(void) {
	bool retval=false;
	if (_Universe->AccessCockpit()) {
		if (Base::CurrentBase) {
			if (_Universe->AccessCockpit()->GetParent()==Base::CurrentBase->caller.GetUnit()){
				if (Base::CallComp) {
					return RefreshInterface ();	
				} else {
					Base::CurrentBase->Draw();
				}
				retval=true;
			}
		}
	}
	return retval;
}

void Base::Room::Click (::Base* base,float x, float y, int button, int state) {
	if (button==WS_LEFT_BUTTON) {
		int linknum=MouseOver (base,x,y);
		if (linknum>=0) {
			Link * link=links[linknum];
			if (link) {
				link->Click(base,x,y,button,state);
			}
		}
	} else {
#ifdef BASE_MAKER
		if (state==WS_MOUSE_UP) {
			char input [201];
			char *str;
			if (button==WS_RIGHT_BUTTON)
				str="Please create a file named stdin.txt and type\nin the sprite file that you wish to use.";
			else if (button==WS_MIDDLE_BUTTON)
				str="Please create a file named stdin.txt and type\nin the type of room followed by arguments for the room followed by text in quotations:\n1 ROOM# \"TEXT\"\n2 \"TEXT\"\n3 vector<MODES>.size vector<MODES> \"TEXT\"";
			else
				return;
#ifdef _WIN32
			int ret=MessageBox(NULL,str,"Input",MB_OKCANCEL);
#else
			printf("\n%s\n",str);
			int ret=1;
#endif
			int index;
			int rmtyp;
			if (ret==1) {
				if (button==WS_RIGHT_BUTTON) {
#ifdef _WIN32
					FILE *fp=fopen("stdin.txt","rt");
#else
					FILE *fp=stdin;
#endif
					fscanf(fp,"%200s",input);
#ifdef _WIN32
					fclose(fp);
#endif
				} else if (button==WS_MIDDLE_BUTTON&&makingstate==0) {
#ifdef _WIN32
					FILE *fp=fopen("stdin.txt","rt");
#else
					FILE *fp=stdin;
#endif
	 				fscanf(fp,"%d",&rmtyp);
					switch(rmtyp) {
					case 1:
						links.push_back(new Goto());
						fscanf(fp,"%d",&((Goto*)links.back())->index);
						break;
					case 2:
						links.push_back(new Launch());
						break;
					case 3:
						links.push_back(new Comp());
						fscanf(fp,"%d",&index);
						for (int i=0;i<index&&(!feof(fp));i++) {
							fscanf(fp,"%d",&ret);
							((Comp*)links.back())->modes.push_back((UpgradingInfo::BaseMode)ret);
						}
						break;
					}
					fscanf(fp,"%200s",input);
					input[200]=input[199]='\0';
					links.back()->text=string(input);
#ifdef _WIN32
					fclose(fp);
#endif
				}
				if (button==WS_RIGHT_BUTTON) {
					input[200]=input[199]='\0';
					objs.push_back(new BaseSprite(input));
					((BaseSprite*)objs.back())->texfile=string(input);
					((BaseSprite*)objs.back())->spr.SetPosition(x,y);
				} else if (button==WS_MIDDLE_BUTTON&&makingstate==0) {
					links.back()->x=x;
					links.back()->y=y;
					links.back()->wid=0;
					links.back()->hei=0;
					makingstate=1;
				} else if (button==WS_MIDDLE_BUTTON&&makingstate==1) {
					links.back()->wid=x-links.back()->x;
					if (links.back()->wid<0)
						links.back()->wid=-links.back()->wid;
					links.back()->hei=y-links.back()->y;
					if (links.back()->hei<0)
						links.back()->hei=-links.back()->hei;
					makingstate=0;
				}
			}
		}
#else
		if (state==WS_MOUSE_UP) {
			Link *curlink=links[base->curlinkindex%links.size()];
			base->curlinkindex++;
			int x=(((curlink->x+(curlink->wid/2))+1)/2)*g_game.x_resolution;
			int y=-(((curlink->y+(curlink->hei/2))-1)/2)*g_game.y_resolution;
			winsys_warp_pointer(x,y);
			MouseOverWin(x,y);
		}
#endif
	}
}

void Base::MouseOver (float x, float y) {
	int i=rooms[curroom]->MouseOver(this,x,y);
	Room::Link *link=0;
	if (i<0) {
		link=0;
	} else {
		link=rooms[curroom]->links[i];
	}
	if (link) {
		curtext.SetText(link->text);
		drawlinkcursor=true;
	} else {
		curtext.SetText(rooms[curroom]->deftext);
		drawlinkcursor=false;
	}
}

void Base::Click (float x, float y, int button, int state) {
	rooms[curroom]->Click(this,x,y,button,state);
}

void Base::ClickWin (int button, int state, int x, int y) {
	if (CurrentBase) {
		CurrentBase->Click((((float)(x*2))/g_game.x_resolution)-1,-(((float)(y*2))/g_game.y_resolution)+1,button,state);
	}
}

void Base::MouseOverWin (int x, int y) {
	SetSoftwareMousePosition(x,y);
	if (CurrentBase)
		CurrentBase->MouseOver((((float)(x*2))/g_game.x_resolution)-1,-(((float)(y*2))/g_game.y_resolution)+1);
}

void Base::GotoLink (int linknum) {
	othtext.SetText("");
	if (rooms.size()>linknum) {
		curlinkindex=0;
		curroom=linknum;
		curtext.SetText(rooms[curroom]->deftext);
		drawlinkcursor=false;
	} else {
#ifndef BASE_MAKER
		fprintf(stderr,"\nWARNING: base room #%d tried to go to an invalid index: #%d",curroom,linknum);
		assert(0);
#else
		while(rooms.size()<=linknum) {
			rooms.push_back(new Room());
			char roomnum [50];
			sprintf(roomnum,"Room #%d",linknum);
			rooms.back()->deftext=roomnum;
		}
		GotoLink(linknum);
#endif
	}
}

Base::~Base () {
#ifdef BASE_MAKER
	vschdir("bases");
	FILE *fp=fopen("NEW_BASE.xbase","wt");
	if (fp) {
		EndXML(fp);
		fclose(fp);
	}
	vscdup();
#endif
	CurrentBase=0;
	restore_main_loop();
	for (int i=0;i<rooms.size();i++) {
		delete rooms[i];
	}
}

void Base::InitCallbacks () {
	winsys_set_mouse_func(ClickWin);
	winsys_set_motion_func(MouseOverWin);
	winsys_set_passive_motion_func(MouseOverWin);
	CurrentBase=this;
//	UpgradeCompInterface(caller,baseun);
	CallComp=false;
}

extern string getCargoUnitName (const char *name);

void Unit::UpgradeInterface(Unit * baseun) {
	if (!Base::CurrentBase) {
	  string basename = (getCargoUnitName(baseun->getFullname().c_str()));
	  if (baseun->isUnit()!=PLANETPTR) {
	    basename = baseun->name;
	  }
	  Base *base=new Base ((basename+".xbase").c_str(),baseun,this);
	  base->InitCallbacks();
	  SetSoftwareMousePosition(0,0);
	}
}

Base::Room::Talk::Talk () {
	index=-1;
#ifndef BASE_MAKER
	gameMessage * last;
	int i=0;
	vector <std::string> who;
	string newmsg;
	string newsound;
	who.push_back ("bar");
	while ((last= mission->msgcenter->last(i++,who))!=NULL) {
		newmsg=last->message;
		newsound="";
		int first=newmsg.find_first_of("[");
		int last=newmsg.find_first_of("]");
		if (first!=string::npos&&(first+1)<newmsg.size()) {
			newsound=newmsg.substr(first+1,last-first-1);
			newmsg=newmsg.substr(0,first);
		}
		this->say.push_back(newmsg);
		this->soundfiles.push_back(newsound);
	}
#endif
}

Base::Base (const char *basefile, Unit *base, Unit*un) {
	caller=un;
	curlinkindex=0;
	this->baseun=base;
	float x,y;
	curtext.GetCharSize(x,y);
	curtext.SetCharSize(x*2,y*2);
	othtext.GetCharSize(x,y);
	othtext.SetCharSize(x*2,y*2);
	othtext.SetSize(.75,-.75);
	LoadXML(basefile);
	if (!rooms.size()) {
		fprintf(stderr,"\nERROR: there are no rooms...");
		assert(0);
		rooms.push_back(new Room ());
		rooms.back()->objs.push_back(new Room::BaseShip (-1,0,0,0,0,-1,0,1,0,QVector(0,0,75)));
		rooms.back()->links.push_back(new Room::Launch ());
		rooms.back()->links.back()->x=rooms.back()->links.back()->y=-1;
		rooms.back()->links.back()->wid=rooms.back()->links.back()->hei=2;
		rooms.back()->deftext="ERROR: No rooms specified...";
		rooms.back()->links.back()->text="ERROR: No rooms specified...";
	}
	GotoLink(0);
}

void Base::Room::Link::Click (Base *base,float x, float y, int button, int state) {
	if (state==WS_MOUSE_UP) {
//		Do nothing...
	}
}

void Base::Room::Comp::Click (Base *base,float x, float y, int button, int state) {
	if (state==WS_MOUSE_UP) {
		Unit *un=base->caller.GetUnit();
		Unit *baseun=base->caller.GetUnit();
		if (un&&baseun) {
			Base::CallComp=true;
			UpgradeCompInterface(un,baseun,modes);
		}
	}
}

void Base::Room::Launch::Click (Base *base,float x, float y, int button, int state) {
	if (state==WS_MOUSE_UP) {
		Base::CurrentBase=0;
		delete base;
		restore_main_loop();
	}
}

void Base::Room::Goto::Click (Base *base,float x, float y, int button, int state) {
	if (state==WS_MOUSE_UP) {
		base->GotoLink(index);
	}
}

void Base::Room::Talk::Click (Base *base,float x, float y, int button, int state) {
	if (state==WS_MOUSE_UP) {
		if (index>=0) {
			delete base->rooms[curroom]->objs[index];
			base->rooms[curroom]->objs.erase(base->rooms[curroom]->objs.begin()+index);
			index=-1;
			base->othtext.SetText("");
		} else if (say.size()) {
			curroom=base->curroom;
			index=base->rooms[curroom]->objs.size();
			base->rooms[curroom]->objs.push_back(new Room::BaseTalk(this));
			int sayindex=rand()%say.size();
			((Room::BaseTalk*)(base->rooms[curroom]->objs.back()))->sayindex=(sayindex);
			((Room::BaseTalk*)(base->rooms[curroom]->objs.back()))->curtime=0;
			if (soundfiles[sayindex].size()>0) {
				int sound = AUDCreateSoundWAV (soundfiles[sayindex],false);
				if (sound==-1) {
					fprintf(stderr,"\nCan't find the sound file %s\n",soundfiles[sayindex].c_str());
				} else {
//					AUDAdjustSound (sound,_Universe->AccessCamera ()->GetPosition(),Vector(0,0,0));
					AUDStartPlaying (sound);
					AUDDeleteSound(sound);//won't actually toast it until it stops
				}
			}
		} else {
			fprintf(stderr,"\nThere are no things to say...\n");
			assert(0);
		}
	}
}

void Base::Draw () {
	GFXColor(0,0,0,0);
	StartGUIFrame(GFXTRUE);
	rooms[curroom]->Draw(this);
	float x,y;
	curtext.GetCharSize(x,y);
	curtext.SetPos(-1+(1.5*x),-1+(1.5*y));
	GFXColor4f(0,1,0,1);
	curtext.Draw();
	othtext.SetPos(-1,1);
	GFXColor4f(0,.5,1,1);
	othtext.Draw();
	EndGUIFrame (drawlinkcursor);
}

