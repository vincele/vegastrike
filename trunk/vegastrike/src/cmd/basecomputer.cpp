/* 
 * Vega Strike
 * Copyright (C) 2003 Mike Byron
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

#include "vegastrike.h"
#include <crtdbg.h>

#include "basecomputer.h"

#include "savegame.h"
#include "universe_util.h"
#include <algorithm>                // For std::sort.
#include <set>
#include "load_mission.h"
#include "cmd/planet_generic.h"
#include "cmd/unit_util.h"
#include "cmd/music.h"
#include "cmd/unit_const_cache.h"
#include "cmd/unit_factory.h"
#include "gui/modaldialog.h"
#include "main_loop.h"              // For QuitNow().

// FIXME mbyron -- Hack instead of reading XML.
#include "gui/newbutton.h"
#include "gui/staticdisplay.h"
#include "gui/simplepicker.h"
#include "gui/groupcontrol.h"
#include "gui/scroller.h"

using namespace std;


// The separator used between categories in a category string.
static const char CATEGORY_SEP = '/';
// Tag that says this is a category not an item.
static const char CATEGORY_TAG = (-1);


// Color of an item that there isn't enough money to buy.
// We read this out of the config file (or use a default).
static GFXColor NO_MONEY_COLOR = GFXColor(0,0,0,-1);        // Start out with bogus color.
// Color of the text of a category.
static const GFXColor CATEGORY_TEXT_COLOR = GFXColor(.5,1,.5,1);
// Space between mode buttons.
static const float MODE_BUTTON_SPACE = 0.03;
// Default color in CargoColor.
static const GFXColor DEFAULT_UPGRADE_COLOR = GFXColor(1,1,1,1);


// Some mission declarations.
// These should probably be in a header file somewhere.
static const char* const MISSION_SCRIPTS_LABEL = "mission_scripts";
static const char* const MISSION_NAMES_LABEL = "mission_names";
static const char* const MISSION_DESC_LABEL = "mission_descriptions";
extern unsigned int getSaveStringLength (int whichcp, string key);
extern unsigned int eraseSaveString (int whichcp, string key, unsigned int num);
extern std::string getSaveString (int whichcp, string key, unsigned int num);
extern void putSaveString (int whichcp, string key, unsigned int num,std::string s);

// Some new declarations.
// These should probably be in a header file somewhere.
static const char* const NEWS_NAME_LABEL = "news";

// Some upgrade declarations.
// These should probably be in a header file somewhere.
extern const Unit* makeFinalBlankUpgrade(string name, int faction);
extern int GetModeFromName(const char *);  // 1=add, 2=mult, 0=neither.
extern Cargo* GetMasterPartList(const char *input_buffer);
extern Unit& GetUnitMasterPartList();
extern void ClearDowngradeMap();
extern std::set<std::string> GetListOfDowngrades();
static const string LOAD_FAILED = "LOAD_FAILED";

// Some ship dealer declarations.
// These should probably be in a header file somewhere.
extern void SwitchUnits(Unit* ol, Unit* nw);
extern void TerminateCurrentBase(void);

// For ships stats.
extern string MakeUnitXMLPretty(std::string, Unit*);

// For Options menu.
extern void RespawnNow(Cockpit* cockpit);



// "Basic Repair" item that is added to Buy UPGRADE mode.
const string BASIC_REPAIR_NAME = "Basic Repair";
const GFXColor BASIC_REPAIR_TEXT_COLOR = GFXColor(0,1,1);
const string BASIC_REPAIR_DESC = "Hire starship mechanics to examine and assess any wear and tear on your craft. They will replace any damaged components on your vessel with the standard components of the vessel you initially purchased.  Further upgrades above and beyond the original will not be replaced free of charge.  The total assessment and repair cost applies if any components are damaged or need servicing (fuel, wear and tear on jump drive, etc...) If such components are damaged you may save money by repairing them on your own.";
// Repair price is a config variable.
static float basicRepairPrice(void) {
    static const float price = XMLSupport::parse_float(vs_config->getVariable("physics","repair_price","1000"));
    return price;
}

// Info about each mode.
static const struct{ string title; string button; string command; string groupId; } modeInfo[] = {
    { "Cargo Dealer  ", "Cargo", "CargoMode", "CargoGroup" },
    { "Ship Upgrades  ", "Upgrades", "UpgradeMode", "UpgradeGroup" },
    { "New Ships  ", "Ships", "ShipDealerMode", "ShipDealerGroup" },
    { "Missions BBS  ", "Missions", "MissionsMode", "MissionsGroup" },
    { "GNN News  ", "News", "NewsMode", "NewsGroup" },
    { "Player Info  ", "Info", "PlayerInfoMode", "PlayerInfoGroup" }
};


// Dispatch table for commands.
// Make an entry here for each command you want to handle.
// WARNING:  The order of this table is important.  There are multiple entries for
//  some commands. Basically, you can make an entry for a particular control, and then
//  later have an entry with an empty control id to cover the other cases.
const BaseComputer::WctlTableEntry BaseComputer::WctlCommandTable[] = {
    { "Picker::NewSelection", "NewsPicker", BaseComputer::newsPickerChangedSelection },
    { "Picker::NewSelection", "", BaseComputer::pickerChangedSelection },
    { modeInfo[CARGO].command, "", BaseComputer::changeToCargoMode },
    { modeInfo[UPGRADE].command, "", BaseComputer::changeToUpgradeMode },
    { modeInfo[SHIP_DEALER].command, "", BaseComputer::changeToShipDealerMode },
    { modeInfo[NEWS].command, "", BaseComputer::changeToNewsMode },
    { modeInfo[MISSIONS].command, "", BaseComputer::changeToMissionsMode },
    { modeInfo[PLAYER_INFO].command, "", BaseComputer::changeToPlayerInfoMode },
    { "BuyCargo", "", BaseComputer::buyCargo },
    { "SellCargo", "", BaseComputer::sellCargo },
    { "BuyUpgrade", "", BaseComputer::buyUpgrade },
    { "SellUpgrade", "", BaseComputer::sellUpgrade },
    { "BuyShip", "", BaseComputer::buyShip },
    { "AcceptMission", "", BaseComputer::acceptMission },
    { "ShowPlayerInfo", "", BaseComputer::showPlayerInfo },
    { "ShowShipStats", "", BaseComputer::showShipStats },
    { "ShowOptionsMenu", "", BaseComputer::showOptionsMenu },
    { "", "", NULL }
};

// Process a command from the window.
// This just dispatches to a handler.
bool BaseComputer::processWindowCommand(const EventCommandId& command, Control* control) {

    // Iterate through the dispatch table.
    for(const WctlTableEntry *p = &WctlCommandTable[0]; p->function != NULL; p++) {
        if(p->command == command) {
            if(p->controlId.size() == 0 || p->controlId == control->id()) {
                // Found a handler for the command.
                return( (this->*(p->function))(command, control) );
            }
        }
    }

    // Let the base class have a try at the command first.
    if(WindowController::processWindowCommand(command, control)) {
        return true;
    }

    // Didn't find a handler.
    return false;
};


// Take underscores out of a string and capitalize letters after spaces.
static string beautify(const string &input) {
    string result = input;
    string::iterator i = result.begin();
    for (;i!=result.end();i++) {
        if (i!=result.begin()) {
            if ((*(i-1))==' ') {
                // Capitalize words.  (First letter after a space).
                *i = toupper(*i);
            }
        }
        if (*i=='_') {
            // Turn underscores into spaces.
            *i=' ';
        }
    }
    return result;
}

// The "used" value of an item.
static double usedValue(double originalValue) {
  return .5*originalValue;
}


// CONSTRUCTOR.
BaseComputer::BaseComputer(Unit* player, Unit* base, const std::vector<DisplayMode>& modes)
    : 
    m_player(player), 
    m_base(base), 
    m_displayModes(modes),
    m_selectedList(NULL),
    m_currentDisplay(NULL_DISPLAY),
    m_playingMuzak(false)
{
    // Make sure we get this color loaded.
    if(NO_MONEY_COLOR.a < 0) {
	float color[4]={1,0,0,1};           // Default = red.
	vs_config->getColor(std::string("default"), "no_money", color, true);
	NO_MONEY_COLOR = GFXColor(color[0], color[1], color[2], color[3]);
        assert(NO_MONEY_COLOR.a >= 0.0);
    }

    // Initialize mode group controls array.
    for(int i=0; i<DISPLAY_MODE_COUNT; i++) {
        m_modeGroups[i] = NULL;
    }
}

// Destructor.
BaseComputer::~BaseComputer(void) {
    m_player.SetUnit(NULL);
    m_base.SetUnit(NULL);

    // Delete any group controls that the window doesn't "own".
    for(int i=0; i<DISPLAY_MODE_COUNT; i++) {
        if(m_modeGroups[i] != NULL) {
            delete m_modeGroups[i];
        }
    }

    // If we are playing muzak, stop it.
    if(m_playingMuzak) {
        muzak->Skip();
    }
}

// Hack that constructs controls in code.
void BaseComputer::constructControls(void) {

    // Title text display.
    StaticDisplay* title = new StaticDisplay;
    title->setRect( Rect(-.95, .7, 1.9, .08) );
    title->setText("ERROR");
    title->setTextColor(GFXColor(.4,1,.4));
    title->setColor(GUI_CLEAR);
    title->setFont( Font(.07, BLACK_STROKE) );
    title->setId("Title");
    // Put it on the window.
    window()->addControl(title);

    // Options button.
    NewButton* options = new NewButton;
    options->setRect( Rect(.74, .85, .22, .1) );
    options->setLabel("Options");
    options->setCommand("ShowOptionsMenu");
    options->setColor( GFXColor(.3,.3,1) );
    options->setHighlightColor( GFXColor(.6,.6,1) );
    options->setTextColor(GUI_OPAQUE_BLACK);
    options->setFont(Font(.08));
    // Put the button on the window.
    window()->addControl(options);

    // Done button.
    NewButton* done = new NewButton;
    done->setRect( Rect(.74, .71, .22, .1) );
    done->setLabel("Done");
    done->setCommand("Window::Close");
    done->setColor( GFXColor(.3,.3,1) );
    done->setHighlightColor( GFXColor(.6,.6,1) );
    done->setTextColor(GUI_OPAQUE_BLACK);
    done->setFont(Font(.08, BOLD_STROKE));
    window()->addControl(done);

    // Mode button.
    NewButton* mode = new NewButton;
    mode->setRect( Rect(-.96, .86, .24, .09) );
    mode->setLabel("ERROR");
    mode->setColor( GFXColor(.5,.5,.4) );
    mode->setHighlightColor( GFXColor(.6,.6,1) );
    mode->setTextColor(GUI_OPAQUE_BLACK);
    mode->setFont(Font(.07));
    mode->setId("ModeButton");
    // Put the button on the window.
    window()->addControl(mode);

    {
        // CARGO group control.
        GroupControl* cargoGroup = new GroupControl;
        cargoGroup->setId("CargoGroup");
        window()->addControl(cargoGroup);

        // Seller text display.
        StaticDisplay* sellLabel = new StaticDisplay;
        sellLabel->setRect( Rect(-.96, .6, .81, .1) );
        sellLabel->setText("Seller");
        sellLabel->setTextColor(GUI_OPAQUE_WHITE);
        sellLabel->setColor(GUI_CLEAR);
        sellLabel->setFont( Font(.09, BOLD_STROKE) );
        sellLabel->setJustification(CENTER_JUSTIFY);
        cargoGroup->addChild(sellLabel);

        // Player inventory text display.
        StaticDisplay* inv = new StaticDisplay;
        *inv = *sellLabel;
        inv->setRect( Rect(.15, .6, .81, .1) );
        inv->setText("Inventory");
        cargoGroup->addChild(inv);

        // Scroller for seller.
        Scroller* sellerScroller = new Scroller;
        sellerScroller->setRect( Rect(-.20, -.4, .05, 1) );
        sellerScroller->setColor( GFXColor(.3,.3,1) );
        sellerScroller->setTextColor(GUI_OPAQUE_WHITE);
        cargoGroup->addChild(sellerScroller);

        // Seller picker.
        SimplePicker* sellpick = new SimplePicker;
        sellpick->setRect( Rect(-.96, -.4, .76, 1) );
        sellpick->setColor( GFXColor(0,0,.4) );
        sellpick->setTextColor(GUI_OPAQUE_WHITE);
        sellpick->setFont( Font(.07) );
        sellpick->setTextMargins(Size(0.02,0.01));
        sellpick->setSelectionColor(GFXColor(.4,.6,.4));
        sellpick->setHighlightColor(GFXColor(.2,.2,.2));
        sellpick->setHighlightTextColor(GUI_OPAQUE_WHITE);
        sellpick->setId("BaseCargo");
        sellpick->setScroller(sellerScroller);
        cargoGroup->addChild(sellpick);

        // Scroller for inventory.
        Scroller* invScroller = new Scroller;
        invScroller->setRect( Rect(.91, -.4, .05, 1) );
        invScroller->setColor( GFXColor(.3,.3,1) );
        invScroller->setTextColor(GUI_OPAQUE_WHITE);
        cargoGroup->addChild(invScroller);

        // Inventory picker.
        SimplePicker* ipick = new SimplePicker;
        ipick->setRect( Rect(.15, -.4, .76, 1) );
        ipick->setColor( GFXColor(0,0,.4) );
        ipick->setTextColor(GUI_OPAQUE_WHITE);
        ipick->setFont( Font(.07) );
        ipick->setTextMargins(Size(0.02,0.01));
        ipick->setSelectionColor(GFXColor(.4,.6,.4));
        ipick->setHighlightColor(GFXColor(.2,.2,.2));
        ipick->setHighlightTextColor(GUI_OPAQUE_WHITE);
        ipick->setId("PlayerCargo");
        ipick->setScroller(invScroller);
        cargoGroup->addChild(ipick);

        // Buy button.
        NewButton* buy = new NewButton;
        buy->setRect( Rect(-.11, .2, .22, .12) );
        buy->setColor( GFXColor(.3,.3,1) );
        buy->setHighlightColor( GFXColor(.6,.6,1) );
        buy->setTextColor(GUI_OPAQUE_BLACK);
        buy->setFont(Font(.1, BLACK_STROKE));
        buy->setId("Commit");
        cargoGroup->addChild(buy);

        // Scroller for description.
        Scroller* descScroller = new Scroller;
        descScroller->setRect( Rect(.91, -.95, .05, .5) );
        descScroller->setColor( GFXColor(.3,.3,1) );
        descScroller->setTextColor(GUI_OPAQUE_WHITE);
        cargoGroup->addChild(descScroller);

        // Description box.
        StaticDisplay* ms = new StaticDisplay;
        ms->setRect( Rect(-.96, -.95, 1.87, .5) );
        ms->setColor( GFXColor(.2,.4,.8) );
        ms->setFont( Font(.06) );
        ms->setMultiLine(true);
        ms->setTextColor(GUI_OPAQUE_WHITE);
        ms->setTextMargins(Size(.02,.01));
        ms->setId("Description");
        ms->setScroller(descScroller);
        cargoGroup->addChild(ms);
    }

    {
        // UPGRADE group control.
        GroupControl* upgradeGroup = new GroupControl;
        upgradeGroup->setId("UpgradeGroup");
        window()->addControl(upgradeGroup);

        // Seller text display.
        StaticDisplay* sellLabel = new StaticDisplay;
        sellLabel->setRect( Rect(-.96, .6, .81, .1) );
        sellLabel->setText("Available Upgrades");
        sellLabel->setTextColor(GUI_OPAQUE_WHITE);
        sellLabel->setColor(GUI_CLEAR);
        sellLabel->setFont( Font(.07, BOLD_STROKE) );
        sellLabel->setJustification(CENTER_JUSTIFY);
        upgradeGroup->addChild(sellLabel);

        // Player inventory text display.
        StaticDisplay* inv = new StaticDisplay;
        *inv = *sellLabel;
        inv->setRect( Rect(.15, .6, .81, .1) );
        inv->setText("Improvements To Sell");
        upgradeGroup->addChild(inv);

        // Scroller for seller.
        Scroller* sellerScroller = new Scroller;
        sellerScroller->setRect( Rect(-.20, -.4, .05, 1) );
        sellerScroller->setColor( GFXColor(.3,.3,1) );
        sellerScroller->setTextColor(GUI_OPAQUE_WHITE);
        upgradeGroup->addChild(sellerScroller);

        // Seller picker.
        SimplePicker* sellpick = new SimplePicker;
        sellpick->setRect( Rect(-.96, -.4, .76, 1) );
        sellpick->setColor( GFXColor(0,0,.4) );
        sellpick->setTextColor(GUI_OPAQUE_WHITE);
        sellpick->setFont( Font(.07) );
        sellpick->setTextMargins(Size(0.02,0.01));
        sellpick->setSelectionColor(GFXColor(.4,.6,.4));
        sellpick->setHighlightColor(GFXColor(.2,.2,.2));
        sellpick->setHighlightTextColor(GUI_OPAQUE_WHITE);
        sellpick->setId("BaseUpgrades");
        sellpick->setScroller(sellerScroller);
        upgradeGroup->addChild(sellpick);

        // Scroller for inventory.
        Scroller* invScroller = new Scroller;
        invScroller->setRect( Rect(.91, -.4, .05, 1) );
        invScroller->setColor( GFXColor(.3,.3,1) );
        invScroller->setTextColor(GUI_OPAQUE_WHITE);
        upgradeGroup->addChild(invScroller);

        // Inventory picker.
        SimplePicker* ipick = new SimplePicker;
        ipick->setRect( Rect(.15, -.4, .76, 1) );
        ipick->setColor( GFXColor(0,0,.4) );
        ipick->setTextColor(GUI_OPAQUE_WHITE);
        ipick->setFont( Font(.07) );
        ipick->setTextMargins(Size(0.02,0.01));
        ipick->setSelectionColor(GFXColor(.4,.6,.4));
        ipick->setHighlightColor(GFXColor(.2,.2,.2));
        ipick->setHighlightTextColor(GUI_OPAQUE_WHITE);
        ipick->setId("PlayerUpgrades");
        ipick->setScroller(invScroller);
        upgradeGroup->addChild(ipick);

        // Buy button.
        NewButton* buy = new NewButton;
        buy->setRect( Rect(-.11, .2, .22, .12) );
        buy->setColor( GFXColor(.3,.3,1) );
        buy->setHighlightColor( GFXColor(.6,.6,1) );
        buy->setTextColor(GUI_OPAQUE_BLACK);
        buy->setFont(Font(.1, BLACK_STROKE));
        buy->setId("Commit");
        upgradeGroup->addChild(buy);

        // Scroller for description.
        Scroller* descScroller = new Scroller;
        descScroller->setRect( Rect(.91, -.95, .05, .5) );
        descScroller->setColor( GFXColor(.3,.3,1) );
        descScroller->setTextColor(GUI_OPAQUE_WHITE);
        upgradeGroup->addChild(descScroller);

        // Description box.
        StaticDisplay* ms = new StaticDisplay;
        ms->setRect( Rect(-.96, -.95, 1.87, .5) );
        ms->setColor( GFXColor(.2,.4,.8) );
        ms->setFont( Font(.06) );
        ms->setMultiLine(true);
        ms->setTextColor(GUI_OPAQUE_WHITE);
        ms->setTextMargins(Size(.02,.01));
        ms->setId("Description");
        ms->setScroller(descScroller);
        upgradeGroup->addChild(ms);
    }

    {
        // NEWS group control.
        GroupControl* newsGroup = new GroupControl;
        newsGroup->setId("NewsGroup");
        window()->addControl(newsGroup);

        // Scroller for picker.
        Scroller* pickScroller = new Scroller;
        pickScroller->setRect( Rect(.91, 0, .05, .65) );
        pickScroller->setColor( GFXColor(.3,.3,1) );
        pickScroller->setTextColor(GUI_OPAQUE_WHITE);
        newsGroup->addChild(pickScroller);

        // News picker.
        SimplePicker* pick = new SimplePicker;
        pick->setRect( Rect(-.96, 0, 1.87, .65) );
        pick->setColor( GFXColor(0,0,.4) );
        pick->setTextColor(GUI_OPAQUE_WHITE);
        pick->setFont( Font(.07) );
        pick->setTextMargins(Size(0.02,0.01));
        pick->setSelectionColor(GFXColor(.4,.6,.4));
        pick->setHighlightColor(GFXColor(.2,.2,.2));
        pick->setHighlightTextColor(GUI_OPAQUE_WHITE);
        pick->setId("NewsPicker");
        pick->setScroller(pickScroller);
        newsGroup->addChild(pick);

        // Scroller for description.
        Scroller* descScroller = new Scroller;
        descScroller->setRect( Rect(.91, -.95, .05, .90) );
        descScroller->setColor( GFXColor(.3,.3,1) );
        descScroller->setTextColor(GUI_OPAQUE_WHITE);
        newsGroup->addChild(descScroller);

        // Description box.
        StaticDisplay* ms = new StaticDisplay;
        ms->setRect( Rect(-.96, -.95, 1.87, .90) );
        ms->setColor( GFXColor(.2,.4,.8) );
        ms->setFont( Font(.07) );
        ms->setMultiLine(true);
        ms->setTextColor(GUI_OPAQUE_WHITE);
        ms->setTextMargins(Size(.02,.01));
        ms->setId("Description");
        ms->setScroller(descScroller);
        newsGroup->addChild(ms);
    }
    {
        // MISSIONS group control.
        GroupControl* missionsGroup = new GroupControl;
        missionsGroup->setId("MissionsGroup");
        window()->addControl(missionsGroup);

        // Scroller for picker.
        Scroller* pickScroller = new Scroller;
        pickScroller->setRect( Rect(-.20, -.8, .05, 1.45) );
        pickScroller->setColor( GFXColor(.3,.3,1) );
        pickScroller->setTextColor(GUI_OPAQUE_WHITE);
        missionsGroup->addChild(pickScroller);

        // Picker.
        SimplePicker* picker = new SimplePicker;
        picker->setRect( Rect(-.96, -.8, .76, 1.45) );
        picker->setColor( GFXColor(0,0,.4,1) );
        picker->setTextColor(GUI_OPAQUE_WHITE);
        picker->setFont( Font(.07) );
        picker->setTextMargins(Size(0.02,0.01));
        picker->setSelectionColor(GFXColor(.4,.6,.4));
        picker->setHighlightColor(GFXColor(.2,.2,.2));
        picker->setHighlightTextColor(GUI_OPAQUE_WHITE);
        picker->setId("Missions");
        picker->setScroller(pickScroller);
        missionsGroup->addChild(picker);

        // Scroller for description.
        Scroller* descScroller = new Scroller;
        descScroller->setRect( Rect(.91, -.8, .05, 1.45) );
        descScroller->setColor( GFXColor(.3,.3,1) );
        descScroller->setTextColor(GUI_OPAQUE_WHITE);
        missionsGroup->addChild(descScroller);

        // Description box.
        StaticDisplay* ms = new StaticDisplay;
        ms->setRect( Rect(-.10, -.8, 1.01, 1.45) );
        ms->setColor( GFXColor(.2,.4,.8) );
        ms->setFont( Font(.06) );
        ms->setMultiLine(true);
        ms->setTextColor(GUI_OPAQUE_WHITE);
        ms->setTextMargins(Size(.02,.01));
        ms->setId("Description");
        ms->setScroller(descScroller);
        missionsGroup->addChild(ms);

        // Accept button.
        NewButton* accept = new NewButton;
        accept->setRect( Rect(-.23, -.95, .22, .1) );
        accept->setColor( GFXColor(.3,.3,1) );
        accept->setHighlightColor( GFXColor(.6,.6,1) );
        accept->setTextColor(GUI_OPAQUE_BLACK);
        accept->setFont(Font(.08, BLACK_STROKE));
        accept->setId("Commit");
        missionsGroup->addChild(accept);
   }
    {
        // SHIP_DEALER group control.
        GroupControl* shipDealerGroup = new GroupControl;
        shipDealerGroup->setId("ShipDealerGroup");
        window()->addControl(shipDealerGroup);

        // Scroller for picker.
        Scroller* pickScroller = new Scroller;
        pickScroller->setRect( Rect(-.20, -.8, .05, 1.45) );
        pickScroller->setColor( GFXColor(.3,.3,1) );
        pickScroller->setTextColor(GUI_OPAQUE_WHITE);
        shipDealerGroup->addChild(pickScroller);

        // Picker.
        SimplePicker* picker = new SimplePicker;
        picker->setRect( Rect(-.96, -.8, .76, 1.45) );
        picker->setColor( GFXColor(0,0,.4,1) );
        picker->setTextColor(GUI_OPAQUE_WHITE);
        picker->setFont( Font(.07) );
        picker->setTextMargins(Size(0.02,0.01));
        picker->setSelectionColor(GFXColor(.4,.6,.4));
        picker->setHighlightColor(GFXColor(.2,.2,.2));
        picker->setHighlightTextColor(GUI_OPAQUE_WHITE);
        picker->setId("Ships");
        picker->setScroller(pickScroller);
        shipDealerGroup->addChild(picker);

        // Scroller for description.
        Scroller* descScroller = new Scroller;
        descScroller->setRect( Rect(.91, -.8, .05, 1.45) );
        descScroller->setColor( GFXColor(.3,.3,1) );
        descScroller->setTextColor(GUI_OPAQUE_WHITE);
        shipDealerGroup->addChild(descScroller);

        // Description box.
        StaticDisplay* ms = new StaticDisplay;
        ms->setRect( Rect(-.10, -.8, 1.01, 1.45) );
        ms->setColor( GFXColor(.2,.4,.8) );
        ms->setFont( Font(.06) );
        ms->setMultiLine(true);
        ms->setTextColor(GUI_OPAQUE_WHITE);
        ms->setTextMargins(Size(.02,.01));
        ms->setId("Description");
        ms->setScroller(descScroller);
        shipDealerGroup->addChild(ms);

        // Buy button.
        NewButton* buy = new NewButton;
        buy->setRect( Rect(-.23, -.95, .22, .1) );
        buy->setColor( GFXColor(.3,.3,1) );
        buy->setHighlightColor( GFXColor(.6,.6,1) );
        buy->setTextColor(GUI_OPAQUE_BLACK);
        buy->setFont(Font(.08, BLACK_STROKE));
        buy->setId("Commit");
        shipDealerGroup->addChild(buy);
    }

    {
        // PLAYER_INFO group control.
        GroupControl* infoGroup = new GroupControl;
        infoGroup->setId("PlayerInfoGroup");
        window()->addControl(infoGroup);

        // Player Info button.
        NewButton* playerInfo = new NewButton;
        playerInfo->setRect( Rect(-.40, .52, .27, .09) );
        playerInfo->setLabel("Player Info");
        playerInfo->setCommand("ShowPlayerInfo");
        playerInfo->setColor( GFXColor(.5,.5,1) );
        playerInfo->setHighlightColor( GFXColor(.6,.6,1) );
        playerInfo->setTextColor(GUI_OPAQUE_BLACK);
        playerInfo->setFont(Font(.07));
        infoGroup->addChild(playerInfo);

        // Ship Stats button.
        NewButton* shipStats = new NewButton;
        shipStats->setRect( Rect(-.05, .52, .27, .09) );
        shipStats->setLabel("Ship Stats");
        shipStats->setCommand("ShowShipStats");
        shipStats->setColor( GFXColor(.5,.5,1) );
        shipStats->setHighlightColor( GFXColor(.6,.6,1) );
        shipStats->setTextColor(GUI_OPAQUE_BLACK);
        shipStats->setFont(Font(.07));
        infoGroup->addChild(shipStats);

        // Scroller for description.
        Scroller* descScroller = new Scroller;
        descScroller->setRect( Rect(.91, -.95, .05, 1.4) );
        descScroller->setColor( GFXColor(.3,.3,1) );
        descScroller->setTextColor(GUI_OPAQUE_WHITE);
        infoGroup->addChild(descScroller);

        // Description box.
        StaticDisplay* ms = new StaticDisplay;
        ms->setRect( Rect(-.96, -.95, 1.87, 1.4) );
        ms->setColor( GFXColor(.2,.4,.8) );
        ms->setFont( Font(.06) );
        ms->setMultiLine(true);
        ms->setTextColor(GUI_OPAQUE_WHITE);
        ms->setTextMargins(Size(.02,.01));
        ms->setId("Description");
        ms->setScroller(descScroller);
        infoGroup->addChild(ms);
    }
}

// Create the controls that will be used for this window.
void BaseComputer::createControls(void) {
    // Set up the window.
    window()->setFullScreen();
    window()->setColor(GUI_OPAQUE_BLACK);

    // Put all the controls in the window.
    constructControls();

    // Take the mode group controls out of the window.
    for(int i=0; i<DISPLAY_MODE_COUNT; i++) {
        Control* group = window()->findControlById(modeInfo[i].groupId);
        if(group) {
            window()->removeControlFromWindow(group);
            m_modeGroups[i] = group;
        }
    }

    createModeButtons();
}

// Create the mode buttons.
void BaseComputer::createModeButtons(void) {
    NewButton* originalButton = dynamic_cast<NewButton*>(window()->findControlById("ModeButton"));
    assert(originalButton != NULL);

    if(m_displayModes.size() > 1) {
        // Create a button for each display mode, copying the original button.
        Rect rect = originalButton->rect();
        for(int i=0; i<m_displayModes.size(); i++) {
            DisplayMode mode = m_displayModes[i];
            NewButton* newButton = new NewButton(*originalButton);
            newButton->setRect(rect);
            newButton->setLabel(modeInfo[mode].button);
            newButton->setCommand(modeInfo[mode].command);
            window()->addControl(newButton);
            rect.origin.x += rect.size.width + MODE_BUTTON_SPACE;
        }
    }

    // Make sure this original doesn't show.
    originalButton->setHidden(true);
}

// Make sure the info in the transaction lists is gone.
void BaseComputer::resetTransactionLists(void) {
    m_transList1 = TransactionList();
    m_transList2 = TransactionList();
}


// Switch to the set of controls used for the specified mode.
void BaseComputer::switchToControls(DisplayMode mode) {
    if(m_currentDisplay != mode) {
        assert(m_modeGroups[mode] != NULL);         // We should have controls for this mode.

        if(m_currentDisplay != NULL_DISPLAY) {
            // Get the old controls out of the window.
            Control* oldControls = window()->findControlById(modeInfo[m_currentDisplay].groupId);
            if(oldControls) {
                window()->removeControlFromWindow(oldControls);
            }
            // We put this back in our table so that we "own" the controls.
            m_modeGroups[m_currentDisplay] = oldControls;
            // Stop playing muzak for the old mode.
            if(m_playingMuzak) {
                muzak->Skip();
                m_playingMuzak = false;
            }
        }

        m_currentDisplay = mode;

        window()->addControl(m_modeGroups[mode]);
        // Take this group out of our table because we don't own it anymore.
        m_modeGroups[mode] = NULL;
    }
}

// Change controls to CARGO mode.
bool BaseComputer::changeToCargoMode(const EventCommandId& command, Control* control) {
    if(m_currentDisplay != CARGO) {
        switchToControls(CARGO);
        loadCargoControls();
        updateTransactionControlsForSelection(NULL);
    }
    return true;
}

// Set up the window and get everything ready.
void BaseComputer::init(void) {
    // Create a new window.
    Window* w = new Window;
    setWindow(w);

    // Read in the controls for all the modes.
    createControls();
}

// Open the window, etc.
void BaseComputer::run(void) {
    // Simulate clicking the leftmost mode button.
    // We don't actually use the button because there isn't a button if there's only one mode.
    processWindowCommand(modeInfo[m_displayModes[0]].command, NULL);

    WindowController::run();
}

// Redo the title string for the display.
void BaseComputer::recalcTitle() {
    // Generic title for the display.
    string title = modeInfo[m_currentDisplay].title;

    // Base name.
    Unit* baseUnit = m_base.GetUnit();
    string baseName;
    if(baseUnit) {
        if(baseUnit->isUnit() == PLANETPTR) {
	    string temp = ((Planet*)baseUnit)->getHumanReadablePlanetType()+" Planet";
	    baseName = temp;
        } else {
	    baseName = baseUnit->name;
        }
    }
    title += baseName + " ";

    if(m_currentDisplay == MISSIONS) {
        // The first entry in the active_missions list should be invisible to the user.
        int count = active_missions.size() - 1;
        count = max(count, 0);
        title += "(Missions: " + tostring(count) + ") ";
    }

    // Credits the player has.
    char floatPrice [100];
    sprintf(floatPrice,"%.2f",_Universe->AccessCockpit()->credits);
    title += string("Credits: ") + floatPrice;

    // Set the string in the title control.
    StaticDisplay* titleDisplay = dynamic_cast<StaticDisplay*>( window()->findControlById("Title") );
    assert(titleDisplay != NULL);
    titleDisplay->setText(title);
}

// Scroll to a specific item in a picker, and optionally select it.
//  Returns true if the specified item is found.
bool BaseComputer::scrollToItem(Picker* picker, const Cargo& item, bool select, bool skipFirstCategory) {
    PickerCells* cells = picker->cells();
    if(!cells) return false;

    PickerCell* categoryCell = NULL;    // Need this if we can't find the item.

    // Walk through the category list(s).
    if(item.category.size() > 0 ) {     // Make sure we have a category.
        int categoryStart = 0;
        if(skipFirstCategory) {
            // We need to skip the first category in the string.
            // Generally need to do this when there's a category level that's not in the UI, like
            //  "upgrades" in the Upgrade UI.
            categoryStart = item.category.find(CATEGORY_SEP, 0);
            if(categoryStart != string::npos) categoryStart++;
        }
        while(true) {
            // See if we have multiple categories left.
            const int categoryEnd = item.category.find(CATEGORY_SEP, categoryStart);
            const string currentCategory = item.category.substr(categoryStart, categoryEnd-categoryStart);

            PickerCell* cell = cells->cellWithId(currentCategory);
            if(!cell || !cell->children()) {
                // The category has no children, or we have no matching category.  We are done.
                // WARNING:  We return from here!
                picker->scrollToCell(categoryCell);
                return false;
            }

            // Found the category in the right place.
            categoryCell = cell;
            categoryCell->setHideChildren(false);
            picker->setMustRecalc();
            cells = categoryCell->children();

            // Next piece of the category string.
            if(categoryEnd == string::npos) {
                break;
            }
            categoryStart = categoryEnd + 1;
        }
    }

    // We have the parent category, now we need the child itself.
    assert(cells != NULL);
    PickerCell* cell = cells->cellWithId(item.content);
    if(!cell) {
        picker->scrollToCell(categoryCell);
        picker->setMustRecalc();
        // Item is not here.
        return false;
    } else {
        if(select) {
            picker->selectCell(cell, true);
        }
        picker->setMustRecalc();
        return true;
    }

    // Shouldn't ever get here.
    assert(false);
    return false;
}

// Update the controls when the selection for a transaction changes.
void BaseComputer::updateTransactionControlsForSelection(TransactionList* list, const Cargo& item) {
    // Get the controls we need.
    NewButton* commitButton = dynamic_cast<NewButton*>( window()->findControlById("Commit") );
    assert(commitButton != NULL);
    StaticDisplay* desc = dynamic_cast<StaticDisplay*>( window()->findControlById("Description") );
    assert(desc != NULL);

    if(!list) {
        // We have no selection.  Turn off UI that commits a transaction.
        m_selectedList = NULL;
        commitButton->setHidden(true);
        desc->setText("");
        // Make sure there is no selection.
        if(m_transList1.picker) m_transList1.picker->selectCell(NULL);
        if(m_transList2.picker) m_transList2.picker->selectCell(NULL);
        return;
    }

    // We have a selection of some sort.

    // Set the button state.
    m_selectedList = list;

    // Clear selection from other list.
    TransactionList& otherList = ( (&m_transList1==m_selectedList)? m_transList2 : m_transList1 );
    if(otherList.picker) otherList.picker->selectCell(NULL);

    if(!isTransactionOK(item, list->transaction)) {
        // We can't do the transaction. so hide the transaction button.
        // This is an odd state.  We have a selection, but no transaction is possible.
        commitButton->setHidden(true);
    } else {
        // We can do the transaction.
        commitButton->setHidden(false);
        switch(list->transaction) {
            case BUY_CARGO:
                commitButton->setLabel("Buy");
                commitButton->setCommand("BuyCargo");
                break;
            case BUY_UPGRADE:
                commitButton->setLabel("Buy");
                commitButton->setCommand("BuyUpgrade");
                break;
            case BUY_SHIP:
                commitButton->setLabel("Buy");
                commitButton->setCommand("BuyShip");
                break;
            case SELL_CARGO:
                commitButton->setLabel("Sell");
                commitButton->setCommand("SellCargo");
                break;
            case SELL_UPGRADE:
                commitButton->setLabel("Sell");
                commitButton->setCommand("SellUpgrade");
                break;
             case ACCEPT_MISSION:
                commitButton->setLabel("Accept");
                commitButton->setCommand("AcceptMission");
                break;
           default:
                assert(false);      // Missed enum in transaction.
                break;
        }
    }

    // The description string.
    string descString;
    char tempString[256];
    Unit* baseUnit = m_base.GetUnit();

    if(list->transaction == ACCEPT_MISSION) {
        descString = item.description;
    } else {
        // Do the money.
        switch(list->transaction) {
            case BUY_CARGO:
                if(item.category.find("My_Fleet") != string::npos) {
                    // This ship is in my fleet -- the price is just the transport cost to get it to
                    //  the current base.  "Buying" this ship makes it my current ship.
                    sprintf(tempString, "#b4#Transport cost: %.2f#-b#n1.5#", item.price);
                } else {
                    sprintf(tempString, "Price: #b4#%.2f#-b#n#", baseUnit->PriceCargo(item.content));
                    descString += tempString;
                    sprintf(tempString, "Cargo volume: %.2f;  Mass: %.2f#n1.5#", item.volume, item.mass);
                }
                descString += tempString;
                break;
            case BUY_UPGRADE:
                if(item.content == BASIC_REPAIR_NAME) {
                    // Basic repair is implemented entirely in this module.
                    // PriceCargo() doesn't know about it.
                    sprintf(tempString, "Price: #b4#%.2f#-b#n1.5#", basicRepairPrice());
                } else {
                    sprintf(tempString, "Price: #b4#%.2f#-b#n1.5#", baseUnit->PriceCargo(item.content));
                }
                descString += tempString;
                break;
            case BUY_SHIP:
                if(item.category.find("My_Fleet") != string::npos) {
                    // This ship is in my fleet -- the price is just the transport cost to get it to
                    //  the current base.  "Buying" this ship makes it my current ship.
                    sprintf(tempString, "#b4#Transport cost: %.2f#-b#n1.5#", item.price);
                } else {
                    sprintf(tempString, "Price: #b4#%.2f#-b#n#", baseUnit->PriceCargo(item.content));
                    descString += tempString;
                    sprintf(tempString, "Cargo volume: %.2f;  Mass: %.2f#n1.5#", item.volume, item.mass);
                }
                descString += tempString;
                break;
            case SELL_CARGO:
                sprintf(tempString, "Value: #b4#%.2f#-b, purchased for %.2f#n#",
                    baseUnit->PriceCargo(item.content), item.price);
                descString += tempString;
                sprintf(tempString, "Cargo volume: %.2f;  Mass: %.2f#n1.5#", item.volume, item.mass);
                descString += tempString;
                break;
            case SELL_UPGRADE:
                sprintf(tempString, "Used value: #b4#%.2f#-b, purchased for %.2f#n1.5#",
                    usedValue(baseUnit->PriceCargo(item.content)), item.price);
                descString += tempString;
                break;
            default:
                assert(false);      // Missed transaction enum in switch statement.
                break;
        }

        // Description.
        descString += item.description;
    }

    // Change the description control.
    desc->setText(descString);
}

// Something in a Picker was selected.
bool BaseComputer::pickerChangedSelection(const EventCommandId& command, Control* control) {
    assert(control != NULL);
    Picker* picker = dynamic_cast<Picker*>(control);
    PickerCell* cell = picker->selectedCell();

    // Figure out which transaction list we are using.
    assert(picker == m_transList1.picker || picker == m_transList2.picker);
    TransactionList* list = ((picker==m_transList1.picker)? &m_transList1 : &m_transList2);

    if(m_base.GetUnit()) {
        if(!cell) {
            // The selection just got cleared.
            TransactionList& otherList = ( (&m_transList1==list)? m_transList2 : m_transList1 );
            if(otherList.picker && otherList.picker->selectedCell()) {
                // Special case.  The other picker has a selection -- we are seeing the selection
                //  cleared in this picker as result.  Do nothing.
            } else {
                updateTransactionControlsForSelection(NULL);
            }
        } else if(cell->tag() == CATEGORY_TAG) {
            // They just selected a category.  Clear the selection no matter what.
            updateTransactionControlsForSelection(NULL);
        } else {
            // They selected a cell that has a description.
            // The selected item.
            Cargo& item = list->masterList[cell->tag()].cargo;
            // Make the controls right for this item.
            updateTransactionControlsForSelection(list, item);
        }
    }

    return true;
}

// Return whether or not this 
bool BaseComputer::isTransactionOK(const Cargo& originalItem, TransactionType transType) {
    // Make sure we have somewhere to put stuff.
    Unit* playerUnit = m_player.GetUnit();
    if(!playerUnit) return false;
    Cockpit* cockpit = _Universe->isPlayerStarship(playerUnit);
    if(!cockpit) return false;

    // Need to fix item so there is only one for cost calculations.
    Cargo item = originalItem;
    item.quantity = 1;
    Unit* baseUnit = m_base.GetUnit();
    switch(transType) {
        case BUY_CARGO:
            // Enough credits and room for the item in the ship.
	    if(item.price <= cockpit->credits && playerUnit->CanAddCargo(item)) {
	        return true;
	    }
            break;
        case SELL_CARGO:
            // There is a base here, and it is willing to buy the item.
	    if(baseUnit && baseUnit->CanAddCargo(item)) {
	        return true;
	    }
            break;
        case BUY_SHIP:
            // Either you are buying this ship for your fleet, or you already own the
            //  ship and it will be transported to you.
            if(baseUnit) {
	        if(item.price <= cockpit->credits) {
	            return true;
	        }
            }
            break;
        case ACCEPT_MISSION:
            // Make sure the player doesn't take too many missions.
	    if(active_missions.size() < UniverseUtil::maxMissions()) {
	        return true;
	    }
            break;
        case SELL_UPGRADE:
        case BUY_UPGRADE:
            // cargo.mission == true means you can't do the transaction.
            if(item.price <= cockpit->credits && !item.mission) {
                return true;
            }
            break;
        default:
            assert(false);          // Missed an enum in transaction switch statement.
            break;
        }

    return false;
}

// Create whatever cells are needed to add a category to the picker.
SimplePickerCell* BaseComputer::createCategoryCell(SimplePickerCells& cells, const string& origCategory,
                                                   bool skipFirstCategory) {
    string category = origCategory;
    if(skipFirstCategory) {
        // Skip over the first category piece.
        const int sepLoc = category.find(CATEGORY_SEP);
        if(sepLoc == string::npos) {
            // No category separator.  At most one category.
            category.clear();
        } else {
            // Skip the first piece.
            category = origCategory.substr(sepLoc+1);
        }
    }

    if(category.size() == 0) {
        // No category at all.
        return NULL;
    }
    // Create or reuse a cell for the first part of the category.
    const int loc = category.find(CATEGORY_SEP);
    const string currentCategory = category.substr(0, loc);
    if(cells.count() > 0 && cells.cellAt(cells.count()-1)->id() == currentCategory) {
        // Re-use the category we have.
    } else {
        // Need to make a new cell for this.
        cells.addCell(SimplePickerCell(beautify(currentCategory), currentCategory, CATEGORY_TEXT_COLOR, CATEGORY_TAG));
    }

    SimplePickerCell* parentCell = cells.cellAt(cells.count()-1);   // Last cell in list.
    if(loc == string::npos) {
        // This is a simple category -- we are done.
        return parentCell;
    } else {
        // The category string has more stuff in it -- finish it up.
        SimplePickerCells* childCellList = dynamic_cast<SimplePickerCells*>( parentCell->children() );
        if(!childCellList) {
            // If parent doesn't have room children, we create the room manually.
            parentCell->createEmptyChildList();
            childCellList = dynamic_cast<SimplePickerCells*>( parentCell->children() );
        }
        const string newCategory = category.substr(loc+1);
        return createCategoryCell(*childCellList, newCategory, false);
    }

    assert(false);
    // Never get here.
}

// Load a picker with a list of items.
void BaseComputer::loadListPicker(TransactionList& list, SimplePicker& picker, TransactionType transType,
                              bool skipFirstCategory) {
    // Make sure the transactionList has the correct info.
    list.picker = &picker;
    list.transaction = transType;

    // Make sure there is nothing old lying around in the picker.
    picker.clear();
    
    // Iterate through the list and load the picker from it.
    string currentCategory = "--ILLEGAL CATEGORY--";    // Current category we are adding cells to.
    SimplePickerCell* parentCell = NULL;                // Place to add new items.  NULL = Add to picker.
    for(int i=0; i<list.masterList.size(); i++) {
        Cargo& item = list.masterList[i].cargo;
        if(item.category != currentCategory) {
            // Create new cell(s) for the new category.
            parentCell = createCategoryCell(*picker.cells(), item.category, skipFirstCategory);
            currentCategory = item.category;
        }

        // Construct the cell for this item.
        const bool transOK = isTransactionOK(item, transType);
        string itemName = beautify(item.content);
        if (item.quantity > 1) {
            // If there is more than one item, show the number of items.
	    itemName += " (" + tostring(item.quantity) + ")";
        }
        // Clear color means use the text color in the picker.
        SimplePickerCell cell(itemName, item.content, (transOK? GUI_CLEAR : NO_MONEY_COLOR), i);

        // Add the cell.
        if(parentCell) {
            parentCell->addChild(cell);
        } else {
            picker.addCell(cell);
        }
    }
}


// Load the controls for the CARGO display.
void BaseComputer::loadCargoControls(void) {
    // Make sure there's nothing in the transaction lists.
    resetTransactionLists();

    // Set up the base dealer's transaction list.
    loadMasterList(m_base.GetUnit(), "missions", false, true, m_transList1); // Anything but a mission.
    SimplePicker* basePicker = dynamic_cast<SimplePicker*>( window()->findControlById("BaseCargo") );
    assert(basePicker != NULL);
    loadListPicker(m_transList1, *basePicker, BUY_CARGO);

    // Set up the player's transaction list.
    loadMasterList(m_player.GetUnit(), "missions", false, true, m_transList2); // Anything but a mission.
    SimplePicker* inventoryPicker = dynamic_cast<SimplePicker*>( window()->findControlById("PlayerCargo") );
    assert(inventoryPicker != NULL);
    loadListPicker(m_transList2, *inventoryPicker, SELL_CARGO);

    // Make the title right.
    recalcTitle();
}


// Get a filtered list of items from a unit.
void BaseComputer::loadMasterList(Unit *un, const string& filterthis, bool inv, bool removezero, TransactionList& list){
    vector<CargoColor>& items = list.masterList;
    items.clear();
    for (unsigned int i=0;i<un->numCargo();i++) {
        unsigned int len = un->GetCargo(i).category.length();
        len = len<filterthis.length()?len:filterthis.length();
        if ((0==memcmp(un->GetCargo(i).category.c_str(),filterthis.c_str(),len))==inv) {//only compares up to category...so we could have starship_blue
	    if ((!removezero)||un->GetCargo(i).quantity>0) {
	        if (!un->GetCargo(i).mission) {
	            CargoColor col;
	            col.cargo=un->GetCargo(i);
	            items.push_back (col);
	        }
	    }
        }
    }
}

// Return a pointer to the selected item in the picker with the selection.
Cargo* BaseComputer::selectedItem(void) {
    Cargo* result = NULL;
    if(m_selectedList) {
        assert(m_selectedList->picker);
        PickerCell* cell = m_selectedList->picker->selectedCell();
        if(cell) {
            result = &m_selectedList->masterList[cell->tag()].cargo;
        }
    }

    return result;
}

// Update the transaction controls after a transaction.
void BaseComputer::updateTransactionControls(const Cargo& item, bool skipFirstCategory) {
    // Go reselect the item.
    assert(m_selectedList != NULL);
    const bool success = scrollToItem(m_selectedList->picker, item, true, skipFirstCategory);
    // And scroll to that item in the other picker, too.
    TransactionList& otherList = ( (&m_transList1==m_selectedList)? m_transList2 : m_transList1 );
    if(otherList.picker) {
        // Scroll to the item in the other list, but don't select it.
        scrollToItem(otherList.picker, item, false, skipFirstCategory);
    }

    if(success) {
        // We selected the item successfully.
        updateTransactionControlsForSelection(m_selectedList, item);
    } else {
        // Didn't find item.  Clear the selection.
        updateTransactionControlsForSelection(NULL);
    }
}

// Buy an item from the cargo list.
bool BaseComputer::buyCargo(const EventCommandId& command, Control* control) {
    Unit* playerUnit = m_player.GetUnit();
    Unit* baseUnit = m_base.GetUnit();
    if(!(playerUnit && baseUnit)) {
        return true;
    }

    const int quantity = 1;
    Cargo* item = selectedItem();
    if(item) {
        Cargo itemCopy = *item;     // Copy this because we reload master list before we need it.
        playerUnit->BuyCargo(item->content, quantity, baseUnit, _Universe->AccessCockpit()->credits);
        // Reload the UI -- inventory has changed.  Because we reload the UI, we need to 
        //  find, select, and scroll to the thing we bought.  The item might be gone from the
        //  list (along with some categories) after the transaction.
        loadCargoControls();        // This will reload master lists.
        updateTransactionControls(itemCopy);
    }

    return true;
}

// Sell an item from ship's cargo.
bool BaseComputer::sellCargo(const EventCommandId& command, Control* control) {
    Unit* playerUnit = m_player.GetUnit();
    Unit* baseUnit = m_base.GetUnit();
    if(!(playerUnit && baseUnit)) {
        return true;
    }

    const int quantity = 1;
    Cargo* item = selectedItem();
    if(item) {
        Cargo itemCopy = *item;     // Not sure what "sold" has in it.  Need copy of sold item.
 	Cargo sold;
        playerUnit->SellCargo(item->content, quantity, _Universe->AccessCockpit()->credits, sold, baseUnit);
        // Reload the UI -- inventory has changed.  Because we reload the UI, we need to 
        //  find, select, and scroll to the thing we bought.  The item might be gone from the
        //  list (along with some categories) after the transaction.
        loadCargoControls();
        updateTransactionControls(itemCopy);
    }

    return true;
}

// Change controls to NEWS mode.
bool BaseComputer::changeToNewsMode(const EventCommandId& command, Control* control) {
    if(m_currentDisplay != NEWS) {
        switchToControls(NEWS);
        loadNewsControls();
        // Turn on some cool music.
	static string newssong=vs_config->getVariable("audio","newssong","../music/news1.ogg");
	muzak->GotoSong(newssong);
        m_playingMuzak = true;
    }
    return true;
}

// The selection in the News picker changed.
bool BaseComputer::newsPickerChangedSelection(const EventCommandId& command, Control* control) {
    assert(control != NULL);
    Picker* picker = dynamic_cast<Picker*>(control);
    PickerCell* cell = picker->selectedCell();

    StaticDisplay* desc = dynamic_cast<StaticDisplay*>( window()->findControlById("Description") );
    assert(desc != NULL);

    if(cell == NULL) {
        // No selection.  Clear desc.  (Not sure how this can happen, but it's easy to cover.)
        desc->setText("");
    } else {
        desc->setText(cell->text());
    }

    return true;
}

// Load the controls for the News display.
void BaseComputer::loadNewsControls(void) {
    SimplePicker* picker = dynamic_cast<SimplePicker*>( window()->findControlById("NewsPicker") );
    assert(picker != NULL);
    picker->clear();

    // Load the picker.
    static const bool newsFromCargolist = XMLSupport::parse_bool(vs_config->getVariable("cargo","news_from_cargolist","false"));
    if(newsFromCargolist) {
        gameMessage last;
        int i = 0;
        vector<std::string> who;
        who.push_back(NEWS_NAME_LABEL);
        while((mission->msgcenter->last(i++, last, who))) {
            picker->addCell(SimplePickerCell(last.message));
        }
    } else {
        // Get news from save game.
        Unit* playerUnit = m_player.GetUnit();
        if(playerUnit) {
            const int playerNum=UnitUtil::isPlayerStarship(playerUnit);
            int len = getSaveStringLength(playerNum, NEWS_NAME_LABEL);
            for (int i=len-1; i>=0; i--) {
                picker->addCell(SimplePickerCell(getSaveString(playerNum, NEWS_NAME_LABEL, i)));
            }
        }
    }

    // Make sure the description is empty.
    StaticDisplay* desc = dynamic_cast<StaticDisplay*>( window()->findControlById("Description") );
    assert(desc != NULL);
    desc->setText("");

    // Make the title right.
    recalcTitle();
}


// Change display mode to MISSIONS.
bool BaseComputer::changeToMissionsMode(const EventCommandId& command, Control* control) {
    if(m_currentDisplay != MISSIONS) {
        switchToControls(MISSIONS);
        loadMissionsControls();
        updateTransactionControlsForSelection(NULL);
    }
    return true;
}

// Need this class to sort CargoColor's.
class CargoColorSort {
public:
    bool operator () (const CargoColor & a, const CargoColor&b) {
	return( a.cargo < b.cargo );
    }
};

// Load a master list with missions.
void BaseComputer::loadMissionsMasterList(TransactionList& list) {
    // Make sure the list is clear.
    list.masterList.clear();

    Unit* unit = _Universe->AccessCockpit()->GetParent();
    int playerNum = UnitUtil::isPlayerStarship(unit);
    if(playerNum < 0) {
        fprintf(stderr,"Docked ship not a player.");
        return;
    }

    // Number of strings to look at.  And make sure they match!
    const int stringCount = getSaveStringLength(playerNum, MISSION_SCRIPTS_LABEL);
    assert(stringCount == getSaveStringLength(playerNum, MISSION_NAMES_LABEL));
    assert(stringCount == getSaveStringLength(playerNum, MISSION_DESC_LABEL));

    // Make sure we have different names for all the missions.
    // This changes the savegame -- it removes ambiguity for good.
    for(int current=0; current<stringCount; current++) {
        const string currentName = getSaveString(playerNum, MISSION_NAMES_LABEL, current);
        int count = 1;
        // Check whether any after the current one have the same name.
        for(unsigned int check=current+1; check<stringCount; ++check) {
            const string checkName = getSaveString(playerNum, MISSION_NAMES_LABEL, check);
            if(check == current) {
                // Found identical names.  Add a "count" at the end. 
                putSaveString(playerNum, MISSION_NAMES_LABEL, current, checkName+"_"+tostring(count++));
            }
        }
    }

    // Create an entry for for each mission.
    for(int i=0; i<stringCount; i++) {
        CargoColor c;

        // Take any categories out of the name and put them in the cargo.category.
        const string originalName = getSaveString(playerNum, MISSION_NAMES_LABEL, i);
        const int lastCategorySep = originalName.rfind(CATEGORY_SEP);
        if(lastCategorySep != string::npos) {
            // We have a category.
            c.cargo.content = originalName.substr(lastCategorySep + 1);
            c.cargo.category = originalName.substr(0, lastCategorySep);
        } else {
            // No category in name.
            c.cargo.content = originalName;
            c.cargo.category = "";
        }

        // Description gets name at the top.
        c.cargo.description = "#b4#" + beautify(c.cargo.content) + ":#-b#n1.75#" + 
            getSaveString(playerNum, MISSION_DESC_LABEL, i);

        list.masterList.push_back(c);
    }

    // Sort the list.  Better for display, easier to compile into categories, etc.
    std::sort(list.masterList.begin(), list.masterList.end(), CargoColorSort());
}

// Load the controls for the MISSIONS display.
void BaseComputer::loadMissionsControls(void) {
    // Make sure there's nothing in the transaction lists.
    resetTransactionLists();

    // Load up the list of missions.
    loadMissionsMasterList(m_transList1);
    SimplePicker* picker = dynamic_cast<SimplePicker*>( window()->findControlById("Missions") );
    assert(picker != NULL);
    loadListPicker(m_transList1, *picker, ACCEPT_MISSION);

    // Make the title right.
    recalcTitle();
}

// Accept a mission.
bool BaseComputer::acceptMission(const EventCommandId& command, Control* control) {
    Unit* playerUnit = m_player.GetUnit();
    Unit* baseUnit = m_base.GetUnit();
    if(!(playerUnit && baseUnit)) return true;

    Cargo* item = selectedItem();

    if(!item || !isTransactionOK(*item, ACCEPT_MISSION)) {
        // Better reload the UI -- we shouldn't have gotten here.
        loadMissionsControls();
        updateTransactionControlsForSelection(NULL);
        return true;
    }

    const int playernum = UnitUtil::isPlayerStarship(playerUnit);
    const int stringCount = getSaveStringLength(playernum, MISSION_NAMES_LABEL);
    assert(stringCount == getSaveStringLength(playernum, MISSION_SCRIPTS_LABEL));
    string qualifiedName;
    if(item->category.empty()) {
        qualifiedName = item->content;
    } else {
        qualifiedName = item->category + CATEGORY_SEP + item->content;
    }
    string finalScript;
    for (unsigned int i=0; i<stringCount; i++) {
	if (getSaveString(playernum, MISSION_NAMES_LABEL, i) == qualifiedName) {
	    finalScript = getSaveString(playernum, MISSION_SCRIPTS_LABEL, i);
	    eraseSaveString(playernum, MISSION_SCRIPTS_LABEL, i);
	    eraseSaveString(playernum, MISSION_NAMES_LABEL, i);
	    eraseSaveString(playernum, MISSION_DESC_LABEL, i);
	    break;
	}
    }
    if(finalScript.empty()) {
        assert(false);       // FIXME mbyron.  Shouldn't this be an alert?
    } else {
        LoadMission("", finalScript, false);
	if(active_missions.size() > 0) {
            // Give the mission a name.
            active_missions.back()->mission_name = item->category;
	}

        // Reload the UI.
        Cargo itemCopy = *item;
        loadMissionsControls();
        updateTransactionControls(itemCopy);
    }

    // We handled the command, whether we successfully accepted the mission or not.
    return true;
}

// Load the all the controls for the UPGRADE display.
void BaseComputer::loadUpgradeControls(void) {
    // Make sure there's nothing in the transaction lists.
    resetTransactionLists();

    // Load the controls.
    loadBuyUpgradeControls();
    loadSellUpgradeControls();
  
    // Make the title right.
    recalcTitle();
}

// Load the BUY controls for the UPGRADE display.
void BaseComputer::loadBuyUpgradeControls(void) {
    Unit* playerUnit = m_player.GetUnit();
    Unit* baseUnit = m_base.GetUnit();

    TransactionList& list = m_transList1;

    // Get all the upgrades.
    assert( equalColors(CargoColor().color, DEFAULT_UPGRADE_COLOR) );
    loadMasterList(baseUnit, "upgrades", true, true, list);
    playerUnit->FilterUpgradeList(list.masterList);

    // Mark all the upgrades that we can't do.
    // cargo.mission == true means we can't upgrade this.
    vector<CargoColor>::iterator iter;
    for(iter=list.masterList.begin(); iter!=list.masterList.end(); iter++) {
        iter->cargo.mission = ( !equalColors(iter->color, DEFAULT_UPGRADE_COLOR) );
    }

    // Add Basic Repair.
    CargoColor repair;
    repair.cargo.content = BASIC_REPAIR_NAME;
    repair.cargo.price = basicRepairPrice();
    repair.cargo.description = BASIC_REPAIR_DESC;
    list.masterList.push_back(repair);

    // Load the upgrade picker from the master list.
    SimplePicker* basePicker = dynamic_cast<SimplePicker*>( window()->findControlById("BaseUpgrades") );
    assert(basePicker != NULL);
    loadListPicker(list, *basePicker, BUY_UPGRADE, true);

    // Fix the Basic Repair color.
    SimplePickerCells* baseCells = basePicker->cells();
    SimplePickerCell* repairCell = baseCells->cellAt(baseCells->count()-1);
    assert(repairCell->text() == BASIC_REPAIR_NAME);
    if(isClear(repairCell->textColor())) {
        // Have repair cell, and its color is normal.
        repairCell->setTextColor(BASIC_REPAIR_TEXT_COLOR);
    }
}

// Load the SELL controls for the UPGRADE display.
void BaseComputer::loadSellUpgradeControls(void) {
    Unit* playerUnit = m_player.GetUnit();
    Unit* baseUnit = m_base.GetUnit();
    if(!(playerUnit && baseUnit)) {
        return;
    }

    TransactionList& list = m_transList2;

    // Get a list of upgrades on our ship we could sell.
    Unit* partListUnit = &GetUnitMasterPartList();
    loadMasterList(partListUnit, "upgrades", true, false, list);
    ClearDowngradeMap();
    playerUnit->FilterDowngradeList(list.masterList);
    static const bool clearDowngrades = XMLSupport::parse_bool(vs_config->getVariable("physics","only_show_best_downgrade","true"));
    if (clearDowngrades) {
        std::set<std::string> downgradeMap = GetListOfDowngrades();
        for (unsigned int i=0;i<list.masterList.size();++i) {
            if (downgradeMap.find(list.masterList[i].cargo.content)==downgradeMap.end()) {
                list.masterList.erase(list.masterList.begin()+i);
                i--;
            }
        }
    }

    // Mark all the upgrades that we can't do.
    // cargo.mission == true means we can't upgrade this.
    vector<CargoColor>::iterator iter;
    for(iter=list.masterList.begin(); iter!=list.masterList.end(); iter++) {
        iter->cargo.mission = ( !equalColors(iter->color, DEFAULT_UPGRADE_COLOR) );
    }

    // Sort the list.  Better for display, easier to compile into categories, etc.
    std::sort(list.masterList.begin(), list.masterList.end(), CargoColorSort());

    // Load the upgrade picker form the master list.
    SimplePicker* basePicker = dynamic_cast<SimplePicker*>( window()->findControlById("PlayerUpgrades") );
    assert(basePicker != NULL);
    loadListPicker(list, *basePicker, SELL_UPGRADE, true);
}


// Change display mode to UPGRADE.
bool BaseComputer::changeToUpgradeMode(const EventCommandId& command, Control* control) {
    Unit* playerUnit = m_player.GetUnit();
    Unit* baseUnit = m_base.GetUnit();
    if(!(playerUnit && baseUnit)) return true;

    if(m_currentDisplay != UPGRADE) {
        switchToControls(UPGRADE);
        loadUpgradeControls();
        updateTransactionControlsForSelection(NULL);
    }
    return true;
}

// Ported from old code.  Not sure what it does.
const Unit* getUnitFromUpgradeName(const string& upgradeName, int myUnitFaction = 0) {
    const char* name = upgradeName.c_str();
    const Unit* partUnit = UnitConstCache::getCachedConst(StringIntKey(name, FactionUtil::GetFaction("upgrades")));
    if (!partUnit) {
        partUnit = UnitConstCache::setCachedConst(StringIntKey(name,
	    FactionUtil::GetFaction("upgrades")),
	    UnitFactory::createUnit(name, true, FactionUtil::GetFaction("upgrades")));
    }
    if (partUnit->name == LOAD_FAILED) {
	partUnit = UnitConstCache::getCachedConst(StringIntKey(name, myUnitFaction));
	if (!partUnit) {
        partUnit = UnitConstCache::setCachedConst(StringIntKey(name, myUnitFaction),
	    UnitFactory::createUnit(name, true, myUnitFaction));
	}
    }
    return partUnit;
}

// Actually do a repair operation.
static void BasicRepair(Unit* parent) {
    if (parent) {
        if(UnitUtil::getCredits(parent) < basicRepairPrice()) {
            showAlert("You don't have enough credits to repair your ship.");
        } else if(parent->RepairUpgrade()) {
            UnitUtil::addCredits(parent, -basicRepairPrice());
        } else {
            showAlert("Your ship has no damage.  No charge.");
        }
    }
}


// The "Operation" classes deal with upgrades.
// There are a number of potential questions that get asked, and a bunch of state that needs
//  to be maintained between the questions.  Rather than cluttering up the main class with this
//  stuff, it's all declared internally here.

// Base class for operation.  Support functions and common data.
// Should delete itself when done.
class BaseComputer::UpgradeOperation : public ModalDialogCallback
{
protected:
    UpgradeOperation(BaseComputer& p)
        : m_parent(p), m_newPart(NULL), m_part(), m_selectedMount(0), m_selectedTurret(0), m_selectedItem() {};
    virtual ~UpgradeOperation(void) {};

    void commonInit(void);              // Initialization.
    void finish(void);                  // Finish up -- destruct the object.  MUST CALL THIS LAST.
    void showMountPicker(void);         // Let the user pick a mount.
    void showTurretPicker(void);        // Let the user pick a turret.
    bool endInit(void);                 // Finish initialization.  Returns true if successful.
    void gotSelectedMount(int index);   // We have the mount number.
    void gotSelectedTurret(int index);  // We have the turret number.
    void updateUI(void);                // Make the UI right after we are done.

    // OVERRIDES FOR DERIVED CLASSES.
    virtual void checkTransaction(void) = 0;    // Check, and verify user wants transaction.
    virtual void concludeTransaction(void) = 0; // Finish the transaction.

    virtual void modalDialogResult( // Dispatch to correct function after some modal UI.
        const std::string& id,
        int result,
        WindowController& controller
        );

    BaseComputer& m_parent;         // Parent class that created us.
    const Unit* m_newPart;
    Cargo m_part;                   // Description of upgrade part.
    int m_selectedMount;            // Which mount to use.
    int m_selectedTurret;           // Which turret to use.
    Cargo m_selectedItem;           // Selection from original UI.
};

// Buy an upgrade for our ship.
class BaseComputer::BuyUpgradeOperation : public BaseComputer::UpgradeOperation
{
public:
    void start(void);               // Start the operation.

    BuyUpgradeOperation(BaseComputer& p) : UpgradeOperation(p),  m_theTemplate(NULL), m_addMultMode(0) {};
protected:
    virtual void checkTransaction(void);    // Check, and verify user wants transaction.
    virtual void concludeTransaction(void);     // Finish the transaction.

    virtual ~BuyUpgradeOperation(void) {};

    const Unit* m_theTemplate;
    int m_addMultMode;
};

// Sell an upgrade from our ship.
class BaseComputer::SellUpgradeOperation : public BaseComputer::UpgradeOperation
{
public:
    void start(void);               // Start the operation.

    SellUpgradeOperation(BaseComputer& p) : UpgradeOperation(p), m_downgradeLimiter(NULL) {};
protected:
    virtual void checkTransaction(void);        // Check, and verify user wants transaction.
    virtual void concludeTransaction(void);     // Finish the transaction.

    virtual ~SellUpgradeOperation(void) {};

    const Unit* m_downgradeLimiter;
};

// Id's for callbacks.
static const string GOT_MOUNT_ID = "GotMount";
static const string GOT_TURRET_ID = "GotTurret";
static const string CONFIRM_ID = "Confirm";


// Some common initialization.
void BaseComputer::UpgradeOperation::commonInit(void) {
    Cargo* selectedItem = m_parent.selectedItem();
    if(!selectedItem) {
        // We don't have enough to do the operation.  Forget it.
        finish();
    }
    m_selectedItem = *selectedItem;
}

// Update the UI controls after a transaction has been concluded successfully.
void BaseComputer::UpgradeOperation::updateUI(void) {
    m_parent.loadUpgradeControls();
    m_parent.updateTransactionControls(m_selectedItem, true);
}

// Finish this operation.
void BaseComputer::UpgradeOperation::finish(void) {
    // Destruct us now.
    delete this;
}

// Finish initialization.  Returns true if successful.
bool BaseComputer::UpgradeOperation::endInit(void) {
    if(m_parent.m_player.GetUnit()) {
        m_newPart = getUnitFromUpgradeName(m_selectedItem.content, m_parent.m_player.GetUnit()->faction);
        if(m_newPart->name != LOAD_FAILED) {
            if(m_newPart->GetNumMounts() > 0) {
                showMountPicker();
            } else {
                gotSelectedMount(0);
            }
        } else {
            return false;
        }
    }

    return true;
}

// Show the user a list of mounts to picker from.
void BaseComputer::UpgradeOperation::showMountPicker(void) {
    Unit* playerUnit = m_parent.m_player.GetUnit();
    if(!playerUnit) {
        finish();
        return;
    }

    vector<string> mounts;
    for(int i=0; i<playerUnit->GetNumMounts(); i++) {
        if (playerUnit->mounts[i].status==Mount::ACTIVE || playerUnit->mounts[i].status==Mount::INACTIVE) {
	    mounts.push_back(playerUnit->mounts[i].type->weapon_name);
        } else {
            mounts.push_back("Empty: " + lookupMountSize(playerUnit->mounts[i].size));
        }
    }

    showListQuestion("Select mount for your item:", mounts, this, GOT_MOUNT_ID);
}

// Let the user pick a turret.
void BaseComputer::UpgradeOperation::showTurretPicker(void) {
    Unit* playerUnit = m_parent.m_player.GetUnit();
    if(!playerUnit) {
        finish();
        return;
    }

    vector<string> mounts;
    for(un_iter unitIter = playerUnit->getSubUnits(); *unitIter!=NULL; unitIter++) {
        mounts.push_back((*unitIter)->name);
    }

    showListQuestion("Select turret mount for your turret:", mounts, this, GOT_TURRET_ID);
}

// Got the mount number.
void BaseComputer::UpgradeOperation::gotSelectedMount(int index) {
    if(index < 0) {
        // The user cancelled somehow.
        finish();
    } else {
        m_selectedMount = index;
        if(m_newPart->viewSubUnits().current() != NULL) {
            // Need to get selected turret.
            showTurretPicker();
        } else {
            // No turrets.  Proceed with the transaction.
            checkTransaction();
        }
    }
}

// Got the mount number.
void BaseComputer::UpgradeOperation::gotSelectedTurret(int index) {
    if(index < 0) {
        // The user cancelled somehow.
        finish();
    } else {
        m_selectedTurret = index;
        checkTransaction();
    }
}

// Dispatch to correct function after some modal UI.
void BaseComputer::UpgradeOperation::modalDialogResult(
    const std::string& id, int result, WindowController& controller) {
    if(id == GOT_MOUNT_ID) {
        // Got the selected mount from the user.
        gotSelectedMount(result);
    } else if(id == GOT_TURRET_ID) {
        // Got the selected turret from the user.
        gotSelectedTurret(result);
    } else if(id == CONFIRM_ID) {
        // User answered whether or not to conclude the transaction.
        if(result == YES_ANSWER) {
            // User wants to do this.
            concludeTransaction();
        } else {
            // User doesn't want to do it.  All done.
            finish();
        }
    }
}



// Start the Buy Upgrade Operation.
void BaseComputer::BuyUpgradeOperation::start(void) {
    commonInit();

    Unit* playerUnit = m_parent.m_player.GetUnit();
    Unit* baseUnit = m_parent.m_base.GetUnit();
    if(!(playerUnit && baseUnit)) {
        finish();
        return;
    }

    const string unitDir = GetUnitDir(playerUnit->name.c_str());
    const string templateName = unitDir + ".template";
    const int faction = playerUnit->faction;

    // Get the "limiter" for the upgrade.  Stats can't increase more than this.
    m_theTemplate = UnitConstCache::getCachedConst(StringIntKey(templateName,faction));
    if (!m_theTemplate) {
        m_theTemplate = UnitConstCache::setCachedConst(StringIntKey(templateName,faction),UnitFactory::createUnit(templateName.c_str(),true,faction));
    }
    if (m_theTemplate->name != LOAD_FAILED) {
        m_addMultMode = GetModeFromName(m_selectedItem.content.c_str());   // Whether the price is linear or geometric.
        unsigned int offset;                // Temp.  Not used.
        Cargo* part = baseUnit->GetCargo(m_selectedItem.content, offset);    // Whether the base has any of these.
        if(part && part->quantity > 0) {
            m_part = *part;
            endInit();
        } else {
            finish();
        }
    } else {
        finish();
    }

    // The object may be deleted now. Be careful here.
}

// Check, and verify user wants Buy Upgrade transaction.
void BaseComputer::BuyUpgradeOperation::checkTransaction(void) {
    Unit* playerUnit = m_parent.m_player.GetUnit();
    if(!playerUnit) {
        finish();
        return;
    }

    double percent;         // Temp.  Not used.
    if(playerUnit->canUpgrade(m_newPart, m_selectedMount, m_selectedTurret, m_addMultMode, false, percent, m_theTemplate) ) {
        // We can buy the upgrade.
        concludeTransaction();
    } else {
        showYesNoQuestion("The item cannot fit the frame of your starship.  Do you want to buy it anyway?",
            this, CONFIRM_ID);
    }
}
 
// Finish the transaction.
void BaseComputer::BuyUpgradeOperation::concludeTransaction(void) {
    Unit* playerUnit = m_parent.m_player.GetUnit();
    Unit* baseUnit = m_parent.m_base.GetUnit();
    if(!(playerUnit && baseUnit)) {
        finish();
        return;
    }

   // Get the upgrade percentage to calculate the full price.
    double percent;
    playerUnit->canUpgrade(m_newPart, m_selectedMount, m_selectedTurret, m_addMultMode, true, percent, m_theTemplate);
    const float price = m_part.price * (1-usedValue(percent));
    if (_Universe->AccessCockpit()->credits >= price) {
        // Have enough money.  Buy it.
        _Universe->AccessCockpit()->credits -= price;
        // Upgrade the ship.
        playerUnit->Upgrade(m_newPart, m_selectedMount, m_selectedTurret, m_addMultMode, true, percent, m_theTemplate);
        // Remove the item from the base, since we bought it.
        unsigned int index;
        baseUnit->GetCargo(m_part.content, index);
        baseUnit->RemoveCargo(index, 1, false);
    }

    updateUI();

    finish();
}


// Start the Sell Upgrade Operation.
void BaseComputer::SellUpgradeOperation::start(void) {
    commonInit();

    Unit* playerUnit = m_parent.m_player.GetUnit();
    if(!playerUnit) {
        finish();
        return;
    }

    const string unitDir = GetUnitDir(playerUnit->name.c_str());
    const string limiterName = unitDir + ".blank";
    const int faction = playerUnit->faction;

    // Get the "limiter" for this operation.  Stats can't decrease more than the blank ship.
    m_downgradeLimiter = makeFinalBlankUpgrade(playerUnit->name,faction);
    if(m_downgradeLimiter->name != LOAD_FAILED) {
        Cargo* part = GetMasterPartList(m_selectedItem.content.c_str());
        if(part) {
            m_part = *part;
            endInit();
        } else {
            finish();
        }
    } else {
        finish();
    }
    // The object may be deleted now. Be careful here.
}

// Check, and verify user wants Sell Upgrade transaction.
void BaseComputer::SellUpgradeOperation::checkTransaction(void) {
    Unit* playerUnit = m_parent.m_player.GetUnit();
    if(!playerUnit) {
        finish();
        return;
    }

    double percent;         // Temp.  Not used.
    if( playerUnit->canDowngrade(m_newPart, m_selectedMount, m_selectedTurret, percent, m_downgradeLimiter) ) {
        // We can sell the upgrade.
        concludeTransaction();
    } else {
        showYesNoQuestion("You don't have exactly what you wish to sell.  Continue?",
            this, CONFIRM_ID);
    }
}
 
// Finish the transaction.
void BaseComputer::SellUpgradeOperation::concludeTransaction(void) {
    Unit* playerUnit = m_parent.m_player.GetUnit();
    Unit* baseUnit = m_parent.m_base.GetUnit();
    if(!(playerUnit && baseUnit)) {
        finish();
        return;
    }

    // Get the upgrade percentage to calculate the full price.
    double percent;
    playerUnit->canDowngrade(m_newPart, m_selectedMount, m_selectedTurret, percent, m_downgradeLimiter);
    const float price = m_part.price * usedValue(percent);
    // Adjust the money.
    _Universe->AccessCockpit()->credits += price;
    // Change the ship.
    if(playerUnit->Downgrade(m_newPart, m_selectedMount, m_selectedTurret, percent, m_downgradeLimiter)) {
        // Remove the item from the ship, since we sold it, and add it to the base.
        m_part.quantity = 1;
        m_part.price = baseUnit->PriceCargo(m_part.content);
        baseUnit->AddCargo(m_part);
    }

    updateUI();

    finish();
}


// Buy a ship upgrade.
bool BaseComputer::buyUpgrade(const EventCommandId& command, Control* control) {
    // Take care of Basic Repair, which is implemented entirely in this module.
    Cargo* item = selectedItem();
    if(item && item->content == BASIC_REPAIR_NAME) {
        BasicRepair(m_player.GetUnit());
        assert(m_selectedList != NULL);
        m_selectedList->picker->selectCell(NULL);       // Turn off selection.
        return true;
    }

    // This complicated operation is done in a separate object.
    BuyUpgradeOperation* op = new BuyUpgradeOperation(*this);
    op->start();

    return true;
}

// Sell an upgrade on your ship.
bool BaseComputer::sellUpgrade(const EventCommandId& command, Control* control) {
    // This complicated operation is done in a separate object.
    SellUpgradeOperation* op = new SellUpgradeOperation(*this);
    op->start();

    return true;
}


// Change controls to SHIP_DEALER mode.
bool BaseComputer::changeToShipDealerMode(const EventCommandId& command, Control* control) {
    if(m_currentDisplay != SHIP_DEALER) {
        switchToControls(SHIP_DEALER);
        loadShipDealerControls();
        updateTransactionControlsForSelection(NULL);
    }
    return true;
}

// Create a Cargo for the specified starship.
Cargo CreateCargoForOwnerStarship(Cockpit* cockpit, int i) {
    Cargo cargo;
    cargo.quantity = 1;
    cargo.volume = 1;
    cargo.price = 0;

    bool needsTransport = true;

    if(i+1 < cockpit->unitfilename.size()) {
        if(cockpit->unitfilename[i+1] == _Universe->activeStarSystem()->getFileName()) {
            // Ship is in this system -- doesn't need transport.
            needsTransport = false;
        }
    }

    if(needsTransport) {
        static const float shipping_price = XMLSupport::parse_float (vs_config->getVariable ("physics","shipping_price","6000"));
        cargo.price = shipping_price;
    }

    cargo.content = cockpit->unitfilename[i];
    cargo.category = "starships/My_Fleet"; 

    return cargo;
}

// Create a Cargo for an owned starship from the name.
Cargo CreateCargoForOwnerStarshipName(Cockpit* cockpit, std::string name, int& index) {
    for(int i=1; i < cockpit->unitfilename.size(); i+=2) {
        if(cockpit->unitfilename[i]==name) {
            index = i;
            return CreateCargoForOwnerStarship(cockpit, i);
        }
    }

  // Didn't find it.
  return Cargo();
}


void SwapInNewShipName(Cockpit* cockpit, const std::string& newFileName, int swappingShipsIndex) {
    Unit* parent = cockpit->GetParent();
    if (parent) {
        if (swappingShipsIndex != -1) {
            while (cockpit->unitfilename.size() <= swappingShipsIndex+1) {
                cockpit->unitfilename.push_back("");
            } 
            cockpit->unitfilename[swappingShipsIndex] = parent->name;
            cockpit->unitfilename[swappingShipsIndex+1] = _Universe->activeStarSystem()->getFileName();
            for(int i=1; i < cockpit->unitfilename.size();i+=2) {
                if(cockpit->unitfilename[i] == newFileName) {
                    cockpit->unitfilename.erase(cockpit->unitfilename.begin()+i);
                    if (i<cockpit->unitfilename.size()) {
                        cockpit->unitfilename.erase(cockpit->unitfilename.begin()+i);//get rid of system
                    }
                    i -= 2;//then +=2;
                }
            }
        } else {
            cockpit->unitfilename.push_back(parent->name);
            cockpit->unitfilename.push_back(_Universe->activeStarSystem()->getFileName());
        }
    } else if (swappingShipsIndex != -1) {//if parent is dead
        if (cockpit->unitfilename.size() > swappingShipsIndex) { //erase the ship we have
            cockpit->unitfilename.erase(cockpit->unitfilename.begin()+swappingShipsIndex);
        }
        if (cockpit->unitfilename.size() > swappingShipsIndex) {
            cockpit->unitfilename.erase(cockpit->unitfilename.begin()+swappingShipsIndex);
        }
    }
    cockpit->unitfilename.front() = newFileName;
}


// Load the controls for the SHIP_DEALER display.
void BaseComputer::loadShipDealerControls(void) {
    // Make sure there's nothing in the transaction lists.
    resetTransactionLists();

    // Set up the base dealer's transaction list.
    loadMasterList(m_base.GetUnit(), "starships", true, true, m_transList1);
    // Add in the starships owned by this player.
    Cockpit* cockpit = _Universe->AccessCockpit();
    for (int i=1; i<cockpit->unitfilename.size(); i+=2) {
        CargoColor cargoColor;
        cargoColor.cargo=CreateCargoForOwnerStarship(cockpit, i);
        m_transList1.masterList.push_back(cargoColor);
    }

    // Load the picker from the master list.
    SimplePicker* basePicker = dynamic_cast<SimplePicker*>( window()->findControlById("Ships") );
    assert(basePicker != NULL);
    loadListPicker(m_transList1, *basePicker, BUY_SHIP, true);

    // Make the title right.
    recalcTitle();
}

// Buy ship from the base.
bool BaseComputer::buyShip(const EventCommandId& command, Control* control) {
    Unit* playerUnit = m_player.GetUnit();
    Unit* baseUnit = m_base.GetUnit();
    Cargo* item = selectedItem();
    if(!(playerUnit && baseUnit && item)) {
        return true;
    }

    unsigned int tempInt;           // Not used.
    Cargo* shipCargo = baseUnit->GetCargo(item->content, tempInt);
    Cargo myFleetShipCargo;
    int swappingShipsIndex = -1;
    if(item->category.find("My_Fleet") != string::npos) {
        // Player owns this starship.
        shipCargo = &myFleetShipCargo;
        myFleetShipCargo = CreateCargoForOwnerStarshipName(_Universe->AccessCockpit(), item->content, swappingShipsIndex);
        if(shipCargo->content.empty()) {
            // Something happened -- can't find ship by name.
            shipCargo = NULL;
            swappingShipsIndex = -1;
        }
    }

    if(shipCargo) {
        if (shipCargo->price < _Universe->AccessCockpit()->credits) {
            Flightgroup* flightGroup = playerUnit->getFlightgroup();
            int fgsNumber = 0;
            if (flightGroup != NULL) {
                fgsNumber = flightGroup->nr_ships;
                flightGroup->nr_ships++;
                flightGroup->nr_ships_left++;
            }
            string newModifications;
            if(swappingShipsIndex != -1) {//if we're swapping not buying load the olde one
                newModifications = _Universe->AccessCockpit()->GetUnitModifications();
            }
            Unit* newPart = UnitFactory::createUnit(item->content.c_str(), false, baseUnit->faction, newModifications,
                flightGroup,fgsNumber);
            newPart->SetFaction(playerUnit->faction);
            if (newPart->name != LOAD_FAILED) {
                if (newPart->nummesh() > 0) {
                    WriteSaveGame(_Universe->AccessCockpit(), false);//oops saved game last time at wrong place
                    _Universe->AccessCockpit()->credits -= shipCargo->price;
                    newPart->curr_physical_state = playerUnit->curr_physical_state;
                    newPart->SetPosAndCumPos(UniverseUtil::SafeEntrancePoint(playerUnit->Position(),newPart->rSize()));
                    newPart->prev_physical_state = playerUnit->prev_physical_state;
                    _Universe->activeStarSystem()->AddUnit(newPart);
                    SwapInNewShipName(_Universe->AccessCockpit(), item->content, swappingShipsIndex);
                    _Universe->AccessCockpit()->SetParent(newPart, item->content.c_str(), _Universe->AccessCockpit()->GetUnitModifications().c_str(),
                        playerUnit->curr_physical_state.position);//absolutely NO NO NO modifications...you got this baby clean off the slate

                    // We now put the player in space.
                    SwitchUnits(NULL, newPart);
                    playerUnit->UnDock(baseUnit);
                    m_player.SetUnit(newPart);
                    WriteSaveGame(_Universe->AccessCockpit(), true);
                    newPart=NULL;
                    playerUnit->Kill();
                    window()->close();
                    TerminateCurrentBase();  //BaseInterface::CurrentBase->Terminate();
                    return true;
                }
            }
            newPart->Kill();
            newPart = NULL;
        }
    }

    return true;
}



// Change controls to PLAYER_INFO mode.
bool BaseComputer::changeToPlayerInfoMode(const EventCommandId& command, Control* control) {
    if(m_currentDisplay != PLAYER_INFO) {
        switchToControls(PLAYER_INFO);
        // Initialize description with player info.
        window()->sendCommand("ShowPlayerInfo", NULL);
    }
    return true;
}

// Show the player's basic information.
bool BaseComputer::showPlayerInfo(const EventCommandId& command, Control* control) {
    // Heading.
    string text = "#b4#Factions:#-b#n1.7#";

    // Number of kills for each faction.
    vector<float>* killList = &_Universe->AccessCockpit()->savegame->getMissionData(string("kills"));

    // A line for each faction.
    const int numFactions = FactionUtil::GetNumFactions();
    int i = 0;
    for(; i<numFactions; i++) {
        float relation = FactionUtil::GetIntRelation(i, ( UniverseUtil::getPlayerX(UniverseUtil::getCurrentPlayer()) )->faction );
        relation = relation * 0.5;
        relation = relation + 0.5;
        const int percent = relation * 100.0;
        text += FactionUtil::GetFactionName(i) + "  " + XMLSupport::tostring(percent);
        if (i < killList->size()) {
            text += ", kills: " + XMLSupport::tostring((int)(*killList)[i]);
        }
        text += "#n#";
    }

    // Total Kills if we have it.
    if (i < killList->size()) {
        text += "#n##b4#Total Kills: " + XMLSupport::tostring((int)(*killList)[i]) + "#-b#";							
    }

    // Put this in the description.
    StaticDisplay* desc = dynamic_cast<StaticDisplay*>( window()->findControlById("Description") );
    assert(desc != NULL);
    desc->setText(text);

    return true;
}

// Show the stats on the player's current ship.
bool BaseComputer::showShipStats(const EventCommandId& command, Control* control) {
    Unit* playerUnit = m_player.GetUnit();
    const string rawText = MakeUnitXMLPretty(playerUnit->WriteUnitString(), playerUnit);

    // Need to translate some characters to make it even prettier.
    string text;
    for(string::const_iterator i=rawText.begin(); i!=rawText.end(); i++) {
        switch(*i) {
            case '\n':
                text.append("#n#");
                break;
            case '"':
                // Delete these, so do nothing.
                break;
            default:
                text.push_back(*i);
                break;
        }
    }

    // Put this in the description.
    StaticDisplay* desc = dynamic_cast<StaticDisplay*>( window()->findControlById("Description") );
    assert(desc != NULL);
    desc->setText(text);

    return true;
}



// Create the controls for the Options Menu window.
static void CreateOptionsMenuControls(Window* window) {

    window->setSizeAndCenter(Size(.6,1));
    window->setColor(GFXColor(.5,.8,.5));

    // Save button.
    NewButton* save = new NewButton;
    save->setRect( Rect(-.20, .3, .40, .1) );
    save->setLabel("Save");
    save->setCommand("Save");
    save->setColor( GFXColor(.2,.4,.2) );
    save->setTextColor(GUI_OPAQUE_WHITE);
    save->setFont(Font(.08, BOLD_STROKE));
    // Put the button on the window.
    window->addControl(save);

    // Load button.
    NewButton* load = new NewButton;
    load->setRect( Rect(-.20, .1, .40, .1) );
    load->setLabel("Load");
    load->setCommand("Load");
    load->setColor( GFXColor(.2,.4,.2) );
    load->setTextColor(GUI_OPAQUE_WHITE);
    load->setFont(Font(.08, BOLD_STROKE));
    // Put the button on the window.
    window->addControl(load);

    // Quit Game button.
    NewButton* quit = new NewButton;
    quit->setRect( Rect(-.20, -.1, .40, .1) );
    quit->setLabel("Quit Game");
    quit->setCommand("Quit");
    quit->setColor( GFXColor(.2,.4,.2) );
    quit->setTextColor(GUI_OPAQUE_WHITE);
    quit->setFont(Font(.08, BOLD_STROKE));
    // Put the button on the window.
    window->addControl(quit);

    // Resume Game button.
    NewButton* resume = new NewButton;
    resume->setRect( Rect(-.20, -.3, .40, .1) );
    resume->setLabel("Resume Game");
    resume->setCommand("Window::Close");
    resume->setColor( GFXColor(.2,.4,.2) );
    resume->setTextColor(GUI_OPAQUE_WHITE);
    resume->setFont(Font(.08, BOLD_STROKE));
    // Put the button on the window.
    window->addControl(resume);
}

// This class runs a simple window for doing Options in the base computer.
// It probably shouldn't be private to the BaseComputer class, but I wasn't sure
//  where to put it.
class BaseComputer::OptionsMenu : public WindowController
{
public:
    // Set up the window and get everything ready.
    virtual void init(void);

    // Process a command event from the window.
    virtual bool processWindowCommand(const EventCommandId& command, Control* control);

    // CONSTRUCTION.
    OptionsMenu(Unit* player) : m_player(player) {};
    virtual ~OptionsMenu(void) {};

protected:
    UnitContainer m_player;                 // Ship info, etc.
};

// Create the window and controls for the Options Menu.
void BaseComputer::OptionsMenu::init(void) {
    Window* window = new Window;
    setWindow(window);

    CreateOptionsMenuControls(window);

    window->setModal(true);
}

// Process a command event from the Options Menu window.
bool BaseComputer::OptionsMenu::processWindowCommand(const EventCommandId& command, Control* control) {
    if(command == "Save") {
        // Save the game.
        Unit* player = m_player.GetUnit();
        if(player) {
            Cockpit* cockpit = _Universe->isPlayerStarship(player);
            if(cockpit) {
                WriteSaveGame(cockpit, false);
                showAlert("Game saved successfully.");
            }
        }
    } else if(command == "Load") {
        // Load the game.
        Unit* player = m_player.GetUnit();
        if(player) {
            Cockpit* cockpit = _Universe->isPlayerStarship(player);
            if(cockpit) {
                player->Kill();
                RespawnNow(cockpit);
                globalWindowManager().shutDown();
                TerminateCurrentBase();  //BaseInterface::CurrentBase->Terminate();
            }
        }
    } else if(command == "Quit") {
        // Quit the game.
        CockpitKeys::QuitNow();
    } else {
        // Not a command we know about.
        return WindowController::processWindowCommand(command, control);
    }

    return true;
}

// Show options.
bool BaseComputer::showOptionsMenu(const EventCommandId& command, Control* control) {
    OptionsMenu* menu = new OptionsMenu(m_player.GetUnit());
    menu->init();
    menu->run();

    return true;
}
