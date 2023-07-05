// Project Base Randomizer

#ifndef RANDO_H
#define RANDO_H

// For compilers that don't support precompilation, include "wx/wx.h"
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
	#include "wx/wx.h"
#endif

#include <algorithm>
#include <random>
#include <bitset>
#include <stack>
#include <unordered_set>
#include <functional>

#include <wx/notebook.h>
#include "wx/colordlg.h"
#include "wx/fontdlg.h"
#include "wx/numdlg.h"
#include "wx/aboutdlg.h"

#include "wx/grid.h"
#include "wx/headerctrl.h"
#include "wx/generic/gridctrl.h"
#include "wx/generic/grideditors.h"
#include <wx/tglbtn.h>
#include <wx/toolbar.h>
#include <wx/filedlg.h>
#include <wx/wfstream.h>
#include <wx/artprov.h>
#include <wx/vector.h>
#include <wx/checkbox.h>
#include <wx/log.h>
#include <wx/textfile.h>
#include <wx/tokenzr.h>
#include <wx/srchctrl.h>
#include <wx/statbox.h>
#include <wx/dialog.h>
#include <wx/textctrl.h>
#include <wx/scrolwin.h>
#include <wx/clrpicker.h>
#include <wx/windowptr.h>
#include <wx/stc/stc.h>
#include "wx/numformatter.h"
#include "wx/renderer.h"
#include "wx/uilocale.h"
#include <wx/spinctrl.h>
#include <wx/statline.h>

#include "rom.h"

struct Node;
struct Location;

enum StorageType {
	kNormal,
	kChozo,
	kHidden
};

enum Major {
	kMinor,
	kMajor
};

struct Item {
	wxString name;
	uint16_t value;
	int weight;
	int number;
	Major major = kMajor;
	Item() {}
	Item(wxString n, uint16_t v, int w) {
		name = n;
		value = v;
		weight = w;
		number = 1;
	}
};

struct Player {
	uint64_t collected 	 = 0;
	bool majorMinor  = false;
	int energyLow 	 = 0;
	int energyMed 	 = 0;
	int energyHigh 	 = 0;
	int missilesLow  = 0;
	int missilesMed  = 0;
	int missilesHigh = 0;
};

// We're going to need a dictionary for the items
WX_DECLARE_HASH_MAP(uint64_t, Item, wxIntegerHash, wxIntegerEqual, ItemDict);

/* wxWidgets related enums and structs
 */
enum CommonValues {
	kMacMargins = 19,
};

enum Address {
	kAddrItems    = 0xF0AC,
	kAddrOptions1 = 0xF0E2,
	kAddrOptions2 = 0xF0E9,
	kAddrPause    = 0xF0F0,
	kAddrHud      = 0xF0F7,
	kAddrBombTime = 0xF0FE,
	kAddrButtons  = 0x0000,
	kAddrGfxPal   = 0x7E6A8,
	kAddrEnemyPal = 0x104EC1,
	kAddrBeamPal  = 0x8763A,
	kAddrSuits	  = 0x20717,
	kAddrSpore    = 0x78642,
	kAddrHeat	  = 0x6E37D
};

enum ID {
	// You can not add a menu ID of 0 on macOS (it's reserved for 'help' I think?)
	ID_Default,

	// Main components
	ID_ToolBar,
	ID_MainPanel,
	ID_Statbar,
	
	// MenuBar
	ID_MenuOpen,
	ID_MenuPreferences,
	ID_MenuCredits,
	ID_MenuContact,

	// Toolbar	
	ID_ToolOpen
};

/* Rando Application
 * This class controls the entire program
 */
class Rando : public wxApp {
public:
	// Preferences needs to be part of the app, not the frame
	wxTextFile _prefFile;

	virtual bool OnInit();
};

/* Rando Main Frame (ha)
 * This class is the frame within which all
 * panels and controls get placed
 */
class RandoFrame : public wxFrame {
public:
	RandoFrame(wxSize s);

protected:
	// The rom is a member so that it can be read/written from anywhere
	Rom *_rom = nullptr;

	// The log also needs to be written to from anywhere
	wxString _log = "";

	// This object represents the player. It needs to be outside of the logic so that tricks can be applied to it
	Player _player;

	wxString _basePath = "";
	wxString _outputPath = "";
	long _seed = 0;

	// The 'options' are bitflags in project base that are controlled by a set of event bits in the code base
	// They default to the default's in project base
	uint16_t _options1 = 0xFDF9;
	uint16_t _options2 = 0x0314;

	uint64_t _tricks1 = 0x89000000;

	// And now the names and descriptions, which won't change, so we can just write them out here
	wxString _optionNames[26] = {"Auto-morph", "Beam Flicker", "Beam Trails", "Morph Flash", "Spark Exit", "Low Health Alert",
								 "Screen Shake", "Keep Speed", "Quick Booster", "Auto Run", "Sound FX", "BackFlip", "Respin",
								 "Quick-morph", "Music", "Cannon Tint", "Time Attack", "Moon Walk", "Up-Spin", "Disable Map",
								 "Speed Echoes", "Flip Echoes", "Auto-Save", "Clear Message box", "Screw Attack Glow", "Spin Complex"};

	wxString _optionDesc[26] = {"(Automatically morph into holes)", "(Beam sprite draws half as often)", "(Particle effects behind beams)",
								"", "(Press jump to exit shinespark)", "", "", "(Preserve speed upon landing)", "(Gain speed echoes twice as fast)",
								"", "(All sound effects)", "(Crouch while holding run and press jump)", "(Press jump while falling to spin)",
								"(Hold item cancel and press down to morph)", "(All game music)", "(Samus arm cannon matches her beam)",
								"(Map displays extra info)", "", "(Hold run and jump to enter spin)", "(Disable map to save processing)",
								"", "(Add echoes to the backflip)", "(Skip save dialog)", "(Message boxes are transparent)", "",
								"(Spinjump animation follows speed)"};

	wxString _trickNames[7] = {"Walljumping", "Infinite Bomb Jump", "Heat Run", "Moch Ball", "Super Missile Jumping", "Underwater Walljumping", "Backflip Through Crumble Blocks"};

	// Now the main panel and sizer
	wxBoxSizer *_mainSizer;
	   wxPanel *_mainPanel;

	// And all the components we need to be able to reference within logic
	wxCheckBox *_romType;

	  wxChoice *_hud;
	  wxChoice *_pauseDefault;
	  wxChoice *_bomb;
	wxCheckBox *_logCheck;
	wxCheckBox *_skipCeres;
	wxCheckBox *_shuffleTilePal;
	wxCheckBox *_shuffleEnemyPal;
	wxCheckBox *_shuffleBeamPal;
	wxCheckBox *_shuffleSuitPal;
	wxCheckBox *_includePBExpanded;
	wxCheckBox *_includeHacks;
	wxCheckBox *_includeVanilla;
	wxCheckBox *_shuffleBeamGfx;
	wxCheckBox *_shuffleMorphGfx;
	wxCheckBox *_shuffleFontNGfx;
	wxCheckBox *_shuffleFontTGfx;
	wxCheckBox *_mysteryItemGfx;
	wxCheckBox *_gravityHeat;
	wxCheckBox *_majorMinor;
	wxSpinCtrl *_energyLow;
	wxSpinCtrl *_energyMed;
	wxSpinCtrl *_energyHigh;
	wxSpinCtrl *_missilesLow;
	wxSpinCtrl *_missilesMed;
	wxSpinCtrl *_missilesHigh;
		wxGrid *_tricksGrid;
	wxTextCtrl *_seedCtrl;
	wxTextCtrl *_outputCtrl;
	wxTextCtrl *_logCtrl;

	// Lastly we have the patches and palettes
	// These are the palettes for the suits that get randomly applied
	int _paletteSizes[8] = {0x20, 0x20 * 8, 0x20 * 4, 0x20 * 4, 0x20 * 4, 0x20 * 4, 0x20, 0x220};
	int _paletteAddrPower[8]   = {0xD9400, 0xD9820, 0xD9B20, 0xD9BA0, 0xD9C20, 0xD9CA0, 0x6DC09, 0x6E466};
	int _paletteAddrVaria[8]   = {0xD9520, 0xD9920, 0xD9D20, 0xD9DA0, 0xD9E20, 0xD9EA0, 0x6DD6F, 0x6E692};
	int _paletteAddrGravity[8] = {0xD9800, 0xD9A20, 0xD9F20, 0xD9FA0, 0xDA020, 0xDA0A0, 0x6DED5, 0x6E8BE};

	// Vanilla palettes
	wxByte **_palettePower;
	wxByte **_paletteVaria;
	wxByte **_paletteGravity;

	// PB Extended palettes
	wxByte **_paletteSlate;
	wxByte **_paletteWhite;
	wxByte **_paletteGreen;
	wxByte **_paletteGrime;
	wxByte **_palettePhazon;
	wxByte **_paletteFusion;

	// Hack palettes
	wxByte **_palettePhazonP;
	wxByte **_palettePhazonV;
	wxByte **_palettePhazonG;
	wxByte **_paletteHallowEve;
	wxByte **_paletteFinalStandP;
	wxByte **_paletteFinalStandV;
	wxByte **_paletteFinalStandG;
	wxByte **_paletteHangTimeP;
	wxByte **_paletteHangTimeG;
	wxByte **_paletteLostWorldP;
	wxByte **_paletteLostWorldG;
	wxByte **_paletteVanillaPlus;
	wxByte **_paletteHyperP;
	wxByte **_paletteHyperV;
	wxByte **_paletteHyperG;
	wxByte **_paletteIceMetalP;
	wxByte **_paletteIceMetalV;
	wxByte **_paletteIceMetalG;

	// These are the patches needed for the rom, in the form of byte buffers
	wxByte *_patchEvents;
	wxByte *_patchSkipCeres;

	// These are patches needed for fixing things in the vanilla map version of PB
	wxByte *_patchDashBall;

	// These are also patches, but specifically for gfx data
	wxByte *_gfxFontBrail;
	wxByte *_gfxFontUpsideDown;
	wxByte *_gfxFontGalactic;
	wxByte *_gfxFontWingDings;
	wxByte *_gfxFontRuneScape;
	wxByte *_gfxFontStarWars;
	wxByte *_gfxFontBadKerning;
	wxByte *_gfxFontChozo;
	wxByte *_gfxFontGreek;
	wxByte *_gfxFontHylian;
	wxByte *_gfxFontKlingon;
	wxByte *_gfxFontLeetSpeak;
	wxByte *_gfxFontLotR;
	wxByte *_gfxFontMinecraft;
	wxByte *_gfxFontDots;
	wxByte *_gfxFontRoman;
	wxByte *_gfxFontBars;
	wxByte *_gfxFontAnalogue;
	wxByte *_gfxFontDice;
	wxByte *_gfxFontBinary;
	wxByte *_gfxFontAscii;
	wxByte *_gfxFontTallies;
	wxByte *_gfxFontElements;
	wxByte *_gfxMysteryItem;

private:
	// Debug
	void debug(wxString s);
	void debug(int i);

	// Menu functions
	void onOpen(wxCommandEvent& event);
	void onRandomize(wxCommandEvent& event);
	void onPreferences(wxCommandEvent& event);
	void onContact(wxCommandEvent& event);
	void onCredits(wxCommandEvent& event);
	void onExit(wxCommandEvent& event);
	void onAbout(wxCommandEvent& event);

	// Init functions
	void initializePatches();
	void populateMenuBar(wxMenuBar *menuBar);
	void populateToolBar(wxToolBar *toolBar);
	void populateMainPanel();
	void makeGrid(wxGrid *grid, int numRows);
	void onGravityHeatCheck(wxCommandEvent &event);
	void onDifficultyChoice(wxCommandEvent &event);
	void onOptionsGridLeftClick(wxGridEvent& event);
	void onTechniqueGridLeftClick(wxGridEvent& event);
	void onBrowse(wxCommandEvent& event);

	// Other feature functions
	void shuffleTilesetPalettes();
	void shuffleEnemyPalettes();
	void shuffleBeamPalettes();
	void shuffleSuitPalettes();
	void shuffleFontText();
	void shuffleFontNumbers();

	// Seed functions
	void getSeed();

	// Logic functions
	void logic();
	bool checkRequirements(Node *node, ItemDict itemPool);
	void setItem(wxVector<Location> &locations, int pos, uint64_t item, ItemDict &itemPool, int &allWeights);
	void resetLocationsAndItems(wxVector<Location> &locations, wxVector<Location> &locationsMinor, ItemDict &itemPool, int &allWeights);
};

DECLARE_APP(Rando)

#endif

