// rendertext.cpp: based on Don's gl_text.cpp
// Based on Aardarples rendertext 
#include "config.h"
#include "command.h"

#include "vegastrike.h"
#include "cg_global.h"
#ifdef HAVE_SDL
#include "SDL/SDL.h"
#endif
#include "gfx/hud.h"
#include "gldrv/winsys.h"
#include <sstream>
#include <string>
#include <vector>
using namespace std;
// ****************
// Console Rendering System by Rogue
// 2005-08-a-few-days
//
// ****************

#ifdef HAVE_SDL
static SDL_mutex * _rtextSDLMutex()
{
	static SDL_mutex * rv = SDL_CreateMutex();
	return rv;
}
#endif


//Render Text (Console) Constructor {{{
RText::RText() {
#ifdef HAVE_SDL
	//Initialize shared mutex
	// (creation is always single-threaded, since no aliases are possible yet)
	_rtextSDLMutex();
#endif

	ndraw = 15;
	WORDWRAP = 85;
	conskip = 0;
//	histpos = 0;
	saycommand("");
}
// }}}
// Render Text (Console) Destructor {{{
RText::~RText() {

}
// }}}
// Set the text width, not used  .. yet{{{
int RText::text_width(char *str)
{
return 0;
};
// }}}
// Should be unused defines {{{
#define VIRTW 2400
#define VIRTH 1800
#define PIXELTAB (VIRTW/12)
#define FONTH 64
// }}}
// Draw text, used by the console, should be private, use conoutf to print to the console {{{
void RText::draw_text(std::string &str, float left, float top, int gl_num)
{
    int x = float_to_int(left);
    int y = float_to_int(top);
	
    std::string::iterator iter = str.begin();
    GFXColor foreground(1, 1, 1, 1);
    GFXColor background(0.05f, 0.05f, 0.2f, 0.5f);
    TextPlane newTextPlane(foreground,background);
    newTextPlane.SetPos(x, y);
    newTextPlane.SetCharSize(.8, .12);
    newTextPlane.Draw(str);
};
// }}}
// render the console, only call if bool console == true {{{
void RText::renderconsole()// render buffer
{

    int nd = 0;
    vector<string> refs;
	bool breaker = false;
//	int i = 0;
//	int lastmillis = 0;
//	int length = 0;
//	for(vector<cline>::iterator iter = conlines.begin();
//	iter < conlines.end(); iter++) length++;
	{for(vector<cline>::iterator iter = conlines.begin(); iter < conlines.end(); iter++)  {
	if(nd < ndraw) 
		refs.push_back((*(iter)).cref);
	else iter = conlines.end();
	nd++;
	}}
    size_t j = 0;
    float x = -1;
    float y = -0.5;
    string workIt;
    workIt.append("\n");
    bool breakout = true;
	vector<string>::iterator iter = refs.end();
	if(iter == refs.begin()) breakout = false;
    for(; breakout;) {
		iter--;
		if(iter == refs.begin()) breakout = false;
		workIt.append((*(iter)));
		workIt.append("\n");
    };
    y = 1;
    ostringstream drawCommand;
	string shorter;
	shorter.append(getcurcommand() );
	while (shorter.size() > 80) { 
		shorter.erase(shorter.begin()); 
	} //erase the front of the current command while it's larger than 80
	// charactors, as to not draw off the screen
    drawCommand << workIt << "#FF1100> " << "#FF1100" << shorter << "#00000";
    string Acdraw; //passing .str() straight to draw_text produces an 
		//error with gcc 4, because it's constant I believe
    Acdraw.append(drawCommand.str());
    draw_text(Acdraw, x, y, 2);

};
// }}}
//append a line to the console, optional "highlight" method , untested {{{
void RText::conline(string &sf, bool highlight)        // add a line to the console buffer
{
	{
		size_t search =0;
		size_t lastsearch = 0;
		for(; (search = sf.find("/r"))!=string::npos ; ) {
			sf.replace(lastsearch, search-lastsearch, "");
			lastsearch = search;
		}
	}
    cline cl;
    int lastmillis = 0;
    cl.outtime = lastmillis;                        // for how long to keep line on screen
    if(highlight)        // show line in a different colour, for chat etc.
    {
        cl.cref.append("\f");
        cl.cref.append(sf);
    }
    else
    {
        cl.cref.append(sf);
    };
    conlines.insert(conlines.begin(), cl);
//    puts(cl.cref.c_str());

};
// }}}
// print a line to the console, broken at \n's {{{
void RText::conoutf(char *in) {
	string foobar(in);
	conoutf(foobar);
	return;
}

void RText::conoutf(string &s, int a, int b, int c)
{
#ifdef HAVE_SDL
	// NOTE: first call must be single-threaded!
	SDL_mutex * mymutex = _rtextSDLMutex();
	SDL_LockMutex(mymutex);
#endif
	cout << s << endl;
// Old {{{
//	{
//		for(int x = WORDWRAP; x < s.size(); x = x+WORDWRAP) {
//			s.insert(x, "\n");
//		}
//	}

//	size_t x = s.find("\n");
//	if(x < string::npos) {
//		size_t xlast = 0;
//		for(; x < string::npos; x = s.find("\n", x+1)) {
//			string newone;
//			newone.append(s.substr(xlast, x-xlast));
//			conline(newone, 1);
//			xlast = x+1;
//		}
//		
//	} else {
//		conline(s, 1);
//	}
// }}}
	string::size_type fries = s.size();
	string customer;
	for(string::size_type burger = 0; burger < fries; burger++) {
		if(s[burger] == '\n' || burger == fries-1) {
			if(burger == fries-1) 
				if(s[fries-1] != '\n' && s[fries-1] != '\r')
					customer += s[burger];
			conline(customer, 1);
			customer.erase();
		} else if( customer.size() >= WORDWRAP) {
			customer += s[burger];
			string fliptheburger;
			while( customer[customer.size()-1] != ' ') {
				fliptheburger += customer[customer.size()-1];
				string::iterator oldfloormeat = customer.end();
				oldfloormeat--; 
				customer.erase(oldfloormeat);
			}
			conline(customer, 1);
			customer.erase();
			{
				string spatchula;
				for(int salt = fliptheburger.size()-1; salt >= 0; salt--) {
					spatchula += fliptheburger[salt];
				}
				fliptheburger.erase();
				fliptheburger.append(spatchula);
			}
			customer.append(fliptheburger);
		} else if( s[burger] != '\r') { 
			customer += s[burger]; // get fat
		}
	}
#ifdef HAVE_SDL
	SDL_UnlockMutex(mymutex);
#endif
};
// }}}
//same as above, but I think it works better {{{
void RText::conoutn(string &s, int a, int b, int c) {
	size_t x = s.find("\n");
	size_t xlast = 0;
	if(x >= string::npos) {
		conoutf(s);
	}
	string::iterator iter = s.end();
	if(iter != s.begin() ) { 
		iter--;
		if(strcmp(&(*(iter)),"\n") != 0) { s.append("\n"); };
	}
	while(x < string::npos) {
		string part;
		part.append(s.substr(xlast, x-xlast));
		xlast = x+1;
		x = s.find("\n", x+1);
		conoutf(part, a, b, c);
	}
}
// }}}
// saycommand(char *), should "say" something, will be useful only with network enabled {{{
//does nothing now
void RText::saycommand(const char *init)/// 
{ //actually, it appends "init" to commandbuf
//Unused.
//  SDL_EnableUNICODE((init!=NULL));
//    if(!editmode) keyrepeat(saycommandon);
//  if(!init) init = "";
//  commandbuf.append(init);

};
// }}}
// Console Keyboard Input {{{
void RText::ConsoleKeyboardI(int code, bool isdown)
{
	if(isdown) {
		switch(code){
//pop teh back of commandbuf
	                case WSK_BACKSPACE:
				{
				string::iterator iter = commandbuf.begin();
				if(iter < commandbuf.end()) {
					iter = commandbuf.end();
					iter--;
					commandbuf.erase(iter);
				}
				break;
				}
	                case WSK_LEFT:
//this should move a put pointer for commandbuf
//right should move it the other way.
//		                for(int i = 0; commandbuf[i]; i++) if(!commandbuf[i+1]) commandbuf[i] = 0;
		        break;

			case WSK_RETURN:
				if(commandbuf[0])
				{
					
					vector<string>::iterator iter = vhistory.end();
					bool noSize = false;
					if(iter <=vhistory.begin() && iter >= vhistory.end()) noSize = true;
					if(!noSize) {
						iter--;
						if(commandbuf.compare((*(iter))) != 0 && !noSize)
						{
//store what was typed into a vector for a command history 
//to scroll up and down through what was typed
//This "feature" isn't finished
							vhistory.push_back(commandbuf);  // cap this?
						}
					} else if(noSize)vhistory.push_back(commandbuf);
					
//					histpos = vhistory.end();
//					histpos--;
				//commands beginning with / are executed
				//in localPlayer.cpp just before this is called
				};
				if(commandbuf.size() > 0) {
//print what was typed - Now done in the command processor
//clear the buffer
					commandbuf.erase();
				}
				break;
			default:
//add it to the command buffer
				if (code>0&&code<256) {
					unsigned char k = (unsigned char)code;
					commandbuf+=k;
				};
				break;
		}
	}

};
// }}}
// get the current command buffer, to execute at enter {{{
string RText::getcurcommand()
{
    return commandbuf;
};
// }}}




//footer, leave at bottom
/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */

