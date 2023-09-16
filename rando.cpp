// Project Base Randomizer
#include "rando.h"
#include "rom.h"

IMPLEMENT_APP(Rando)

// ------------------------------------------------------------------
/* ****          ****
 * **** App Init ****
 * ****          ****
 */
bool Rando::OnInit()
{
	// We will be using .png images for the program, so we need a handler for them
	wxImage::AddHandler(new wxPNGHandler);

	// Default size is 530x580
	int sizeX = 530;
	int sizeY = 580;

	// Create the frame that will contain the program
	RandoFrame *frame = new RandoFrame(wxSize(sizeX, sizeY));
	frame->Show(true);

	// Rando needs to be resizable, but we don't want it going smaller than the default
	frame->SetMinSize(wxSize(sizeX, sizeY));

	// Finally, we're finished, run the program
	return true;
}
// ------------------------------------------------------------------

// ------------------------------------------------------------------
/* ****               ****
 * **** The App Frame ****
 * ****               ****
 */
void RandoFrame::debug(wxString s) {
	std::cout << s << std::endl;
}

void RandoFrame::debug(int i) {
	std::cout << i << std::endl;
}

RandoFrame::RandoFrame(wxSize s) : wxFrame(NULL, wxID_ANY, "Project Base Randomizer", wxDefaultPosition, s) {
	// We start by initializing the patches and palettes that we'll be using
	initializePatches();

	/* This program is structured as:
	 * /--- Menu ---\
	 * |-- Toolbar -|
	 * |--- Panel --|
	 * \-- StatBar -/
	 */

	// So first we give it a menu
	wxMenuBar *menuBar = new wxMenuBar;				// Create a menu bar for our menus
	populateMenuBar(menuBar);
	SetMenuBar(menuBar);

	// Next we give it a toolbar (might remove this on windows, it looks much less cool there...)
	// *** THIS IS MAC SPECIFIC, IT DOES NOT LOOK GOOD ON WINDOWS ***
	wxToolBar *toolBar = new wxToolBar(this, ID_ToolBar);
	populateToolBar(toolBar);
	SetToolBar(toolBar);
	toolBar->Realize();

	// After the toolbar is the main program area, which needs a general sizer to correctly size the panel when resizing the window
	_mainSizer = new wxBoxSizer(wxVERTICAL);
	
	// We start by making a new panel
	_mainPanel = new wxPanel(this, ID_MainPanel, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL, "Main Panel");
	
	// And popoulating it with all the widgets needed
	populateMainPanel();
	_mainSizer->Add(_mainPanel, 0, wxGROW);

	// And now we need to apply the sizer to the frame
	_mainSizer->Layout();
	SetSizer(_mainSizer);

	// Finally, we also want a status bar for showing the last action performed (ie. 'Rom saved successfull')
	//CreateStatusBar();

	// By default, the status bar just tells the user they should load a rom
	//SetStatusText("To get started, load a rom");
}
// ------------------------------------------------------------------

// ------------------------------------------------------------------
/* ****              	   ****
 * **** The Main Functions ****
 * ****                    ****
 */

#include <chrono>

void RandoFrame::onRandomize(wxCommandEvent& event) {
	// If the rom doesn't exist yet, then we haven't loaded a PB base rom yet
	if (_rom == nullptr) {
		wxMessageBox("Please load a Super Metroid Project Base 0.8+ rom into the program.","Can't randomize!", wxOK | wxICON_INFORMATION);
		return;
	}

	auto startTime = std::chrono::high_resolution_clock::now();

	/* --- Randomization Seeding ---
	 */
	// If the seed box has a value, we try to convert it to a number
	if (_seedCtrl->GetValue() != "") {

		//If we can't, we won't use that for the seed generation
		if (_seedCtrl->GetValue().ToLong(&_seed, 10) != true) {
			_seed = 0;
			wxMessageBox("Please input a valid seed or leave blank.","Can't randomize!", wxOK | wxICON_INFORMATION);
			return;
		}

	} else {
		// If this is called twice in one second, we want to use the last random number to ensure it's still random
		_seed = (rand() ^ time(0));
	}

	// Now either way, seed the random generator with the seed value
	srand(_seed);

	// And begin the log file with the seed value
	_log += "Super Metroid Project Base 0.8+ Randomizer Spoiler log\n\n";
	_log += wxString::Format("Seed: %ld\n\n", _seed);

	/* --- Fixes ---
	 */
	// This prevents the suit aquired animation from playing, so you can't get stuck from it
	_rom->setWord(kAddrSuits, 0xEAEA);
	_rom->setWord(kAddrSuits + 2, 0xEAEA);

	// In vanilla PB, the spore spawn door is green. We want it to be a mini boss door instead, so that you can get out without supers
	_rom->setWord(kAddrSpore, 0xC84E); // Green door -> locked door
	_rom->setWord(kAddrSpore + 4, 0x042E); // Index high 00 -> 04 (mini boss of area is dead)

	// This fixes an issue with the original game. In vanilla, chozo orb/hidden morph gives the equipment bit of springball
	// This just sets it to give morph like it's supposed to
	_rom->setByte(0x26E02, 0x04);
	_rom->setByte(0x268CE, 0x04);

	// This one adjusts the event state so that zebes awakens after any (?) item is acquired
	_rom->applyPatch(_patchEvents);

	// The vanilla patch doesn't include the dash ball gfx, so we need to include it as a patch
	/*if (_romType->GetValue() == true) {
		_rom->applyPatch(_patchDashBall);
	}*/

	/* --- Options ---
	 */
	// This will skip the entire intro and ceres station
	if (_skipCeres->GetValue() == true) {
		_rom->applyPatch(_patchSkipCeres);
	}

	if (_mysteryItemGfx->GetValue() == true) {
		_rom->applyPatch(_gfxMysteryItem);
	}

	if (_gravityHeat->GetValue() == false) {
		// If we don't want it to stop heat, we need to change the game code and leave the player object unchanged
		// Just changes the bit check in samus heat pre-instruction from 21 (varia + gravity) to 01 (varia)
		// *** Mother brain does a huge amount of damage now?!? But only in vanilla mode? ***
		_rom->setByte(kAddrHeat, 0x01);
	}

	if (_partyRando->GetValue() == true) {
		_rom->applyPatch(_patchEffects);
	}

	// Set the options to their respective words
	_rom->setWord(kAddrOptions1, _options1);
	_rom->setWord(kAddrOptions2, _options2);
	_rom->setWord(kAddrPause,    _pauseDefault->GetSelection());
	_rom->setWord(kAddrHud,      _hud->GetSelection());
	_rom->setWord(kAddrBombTime, _bomb->GetSelection());

	// Now all of the palette and gfx shuffling functions
	if (_shuffleTilePal->GetValue() == true) { shuffleTilesetPalettes(); }
	if (_shuffleBeamPal->GetValue() == true) { shuffleBeamPalettes(); }
	if (_shuffleSuitPal->GetValue() == true) { shuffleSuitPalettes(); }
	if (_shuffleFontNGfx->GetValue() == true) { shuffleFontNumbers(); }
	if (_shuffleFontTGfx->GetValue() == true) { shuffleFontText(); }

	/* --- Logic ---
	 */ 
	// Now the actual rando logic itself, which calls from a separate file to ensure logic is separate from the rest of the program
	_player.collected = 0;
	_player.collected |= _tricks1;

	if (_majorMinor->GetValue() == true) {
		_player.majorMinor = true;
	}

	_player.energyLow = _energyLow->GetValue();
	_player.energyMed = _energyMed->GetValue();
	_player.energyHigh = _energyHigh->GetValue();
	_player.missilesLow = _missilesLow->GetValue();
	_player.missilesMed = _missilesMed->GetValue();
	_player.missilesHigh = _missilesHigh->GetValue();

	logic();

	/* --- Output ---
	 */
	// If the name contains <seed>, we want to replace all of those occurances with the seed value as a string
	wxString fileName = _outputCtrl->GetValue();
	fileName.Replace("<seed>", wxString::Format("%ld", _seed), true);

	// Make a new file that will be the output rom using the current data buffer
	_rom->makeNewRom(fileName);

	// Set the text control for the log to the value of the log and reset the log value
	_logCtrl->SetValue(_log);
	_log = "";

	// And if the user wants a spoiler log file output, save the contents to a text file of the same name
	if (_logCheck->GetValue() == true) {
		_logCtrl->SaveFile(wxString::Format("%s.txt", fileName.BeforeLast('.')));
	}

	// Since we potentially apply patches, we want to re-make the data buffer before making a new seed
	delete _rom;
	_rom = new Rom(_basePath);

	auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
	std::cout << "Execution time: " << duration << " milliseconds" << std::endl;

}

void RandoFrame::shuffleTilesetPalettes() {
	int t1[] = {3, 7, 8, 10, 11, 12, 22, 23, 24, 25, 0};
	int t2[] = {7, 8, 10, 11, 12, 13, 22, 23, 24, 25, 0};
	int t3[] = {7, 8, 12, 22, 23, 24, 25, 0};
	int t4[] = {5, 8, 10, 11, 12, 13, 0};
	int t5[] = {1, 3, 7, 8, 10, 11, 12, 13, 22, 23, 24, 25, 0};
	int t6[] = {1, 3, 7, 8, 10, 11, 12, 13, 22, 23, 24, 25, 0};
	int t7[] = {8, 10, 11, 23, 24, 25, 0};
	int t8[] = {10, 11, 12, 13, 0};
	int t9[] = {9, 0};
	int t10[] = {7, 8, 12, 23, 24, 25, 0};
	int t11[] = {7, 8, 12, 13, 23, 24, 25, 0};
	int t12[] = {7, 8, 10, 11, 22, 23, 24, 25, 0};
	int t13[] = {7, 8, 10, 11, 22, 23, 24, 25, 0};
	int *tilesets[13] = {t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13};

	wxVector<wxVector<int>> palettes;
	for (int i = 0; i < 25; i++) {
		wxVector<int> palette;
		palette.push_back(_rom->getByte(kAddrGfxPal + (i * 9)));
		palette.push_back(_rom->getByte(kAddrGfxPal + (i * 9 + 1)));
		palette.push_back(_rom->getByte(kAddrGfxPal + (i * 9 + 2)));
		palettes.push_back(palette);
	}

	std::unordered_set<int> used;
	for (int i = 0; i < 13; i++) {
		if (i != 8) {
			wxVector<int> tileset;
			int n = 0;
			while (tilesets[i][n] != 0) {
				tileset.push_back(tilesets[i][n] - 1);
				n++;
			}
			int t = 0;
			int tryT = 0;
			do {
				t = rand() % tileset.size();
				t = tileset[t];
				tryT++;
			} while ((used.find(t) != used.end()) && (tryT < tileset.size()));
			used.insert(t);

			_rom->setByte(kAddrGfxPal + (i * 9), palettes[t][0]);
			_rom->setByte(kAddrGfxPal + (i * 9) + 1, palettes[t][1]);
			_rom->setByte(kAddrGfxPal + (i * 9) + 2, palettes[t][2]);
		}
	}
}

void RandoFrame::shuffleBeamPalettes() {
	wxVector<wxVector<int>> palettes;
	for (int i = 0; i < 12; i++) {
		wxVector<int> palette;
		palette.push_back(_rom->getByte(kAddrBeamPal + (i * 2)));
		palette.push_back(_rom->getByte(kAddrBeamPal + (i * 2)+ 1));
		// This should also do beam_type and beam_type_charge in bank 91
		palettes.push_back(palette);
	}

	std::shuffle(palettes.begin(), palettes.end(), std::mt19937{std::random_device{}()});
	for (int i = 0; i < 12; i++) {
		_rom->setByte(kAddrBeamPal + (i * 2), palettes[i][0]);
		_rom->setByte(kAddrBeamPal + (i * 2) + 1, palettes[i][1]);
	}
}

void RandoFrame::shuffleSuitPalettes() {
	wxVector<wxByte **> palettes;

	// Selected by default, we include the vanilla palettes
	if (_includeVanilla->GetValue() == true) {
		palettes.push_back(_palettePower);
		palettes.push_back(_paletteVaria);
		palettes.push_back(_paletteGravity);
	}

	// But if expanded is checked, we include the PB expanded suit palettes
	if (_includePBExpanded->GetValue() == true) {
		palettes.push_back(_paletteSlate);
		palettes.push_back(_palettePhazon);
		palettes.push_back(_paletteWhite);
		palettes.push_back(_paletteGrime);
		palettes.push_back(_paletteGreen);
		palettes.push_back(_paletteFusion);
	}

	// And if hacks is checked, we also include the hack palettes
	if (_includeHacks->GetValue() == true) {
		palettes.push_back(_palettePhazonP);
		palettes.push_back(_palettePhazonV);
		palettes.push_back(_palettePhazonG);
		palettes.push_back(_paletteHallowEve);
		palettes.push_back(_paletteFinalStandP);
		palettes.push_back(_paletteFinalStandV);
		palettes.push_back(_paletteFinalStandG);
		palettes.push_back(_paletteLostWorldP);
		palettes.push_back(_paletteLostWorldG);
		palettes.push_back(_paletteVanillaPlus);
		palettes.push_back(_paletteHyperP);
		palettes.push_back(_paletteHyperV);
		palettes.push_back(_paletteHyperG);
		palettes.push_back(_paletteIceMetalP);
		palettes.push_back(_paletteIceMetalV);
		palettes.push_back(_paletteIceMetalG);
	}

	// If nothing is selected but shuffle is still applied, just do nothing
	if (palettes.size() > 0) {
		// Otherwise, we choose a random suit for each one

		// Power
		int pal = rand() % palettes.size();
		for (int i = 0; i < 8; i++) {
			for (int j = 0; j < _paletteSizes[i]; j++) {
				_rom->setByte(_paletteAddrPower[i] + j, palettes[pal][i][j]);
			}
		}
		palettes.erase(palettes.begin() + pal);

		// Varia
		pal = rand() % palettes.size();
		for (int i = 0; i < 8; i++) {
			for (int j = 0; j < _paletteSizes[i]; j++) {
				_rom->setByte(_paletteAddrVaria[i] + j, palettes[pal][i][j]);
			}
		}
		palettes.erase(palettes.begin() + pal);
		
		// Gravity
		pal = rand() % palettes.size();
		for (int i = 0; i < 8; i++) {
			for (int j = 0; j < _paletteSizes[i]; j++) {
				_rom->setByte(_paletteAddrGravity[i] + j, palettes[pal][i][j]);
			}
		}		
	}
}

void RandoFrame::shuffleEnemyPalettes() {
}

void RandoFrame::shuffleFontText() {
	// First we want a vector of the possible patches to apply
	wxVector<wxByte *> fontText;
	fontText.push_back(_gfxFontBrail);
	fontText.push_back(_gfxFontUpsideDown);
	fontText.push_back(_gfxFontGalactic);
	fontText.push_back(_gfxFontWingDings);
	fontText.push_back(_gfxFontRuneScape);
	fontText.push_back(_gfxFontStarWars);
	fontText.push_back(_gfxFontBadKerning);
	fontText.push_back(_gfxFontChozo);
	fontText.push_back(_gfxFontGreek);
	fontText.push_back(_gfxFontHylian);
	fontText.push_back(_gfxFontKlingon);
	fontText.push_back(_gfxFontLeetSpeak);
	fontText.push_back(_gfxFontLotR);
	fontText.push_back(_gfxFontMinecraft);

	int randNum = rand() % fontText.size();

	// Then we pick one at random to apply
	_rom->applyPatch(fontText[randNum]);
}

void RandoFrame::shuffleFontNumbers() {
	// First we want a vector of the possible patches to apply
	wxVector<wxByte *> fontNumbers;
	//fontNumbers.push_back(_gfxFontDots);
	fontNumbers.push_back(_gfxFontRoman);
	//fontNumbers.push_back(_gfxFontBars);
	fontNumbers.push_back(_gfxFontAnalogue);
	fontNumbers.push_back(_gfxFontDice);
	fontNumbers.push_back(_gfxFontBinary);
	//fontNumbers.push_back(_gfxFontAscii);
	fontNumbers.push_back(_gfxFontTallies);
	//fontNumbers.push_back(_gfxFontElements);

	int randNum = rand() % fontNumbers.size();

	// Then we pick one at random to apply
	_rom->applyPatch(fontNumbers[randNum]);
}

// ------------------------------------------------------------------

// ------------------------------------------------------------------
/* ****              ****
 * **** The Menu Bar ****
 * ****              ****
 */
void RandoFrame::populateMenuBar(wxMenuBar *menuBar) {

	/* ---- File ----
	 * -Load Rom
	 */
	wxMenu *menuFile = new wxMenu;
	menuFile->Append(ID_MenuOpen, 		"&Open...\tCtrl-O", "Load a new base Rom");
	/* -------------- */

	/* ---- Help ----
	 * -Credits
	 * -Contact
	 */
	wxMenu *menuHelp = new wxMenu;
	menuHelp->AppendSeparator();
	menuHelp->Append(ID_MenuContact, "&Contact", "Show contact information");
	menuHelp->Append(ID_MenuCredits, "&Credits", "Show credits");
	/* -------------- */

	/* ---- Rando ----
	 * (special cases)
	 * Quit
	 * About
	 * Preferences
	 */
	menuFile->Append(wxID_EXIT);
	menuHelp->Append(wxID_ABOUT);
	menuFile->Append(wxID_PREFERENCES);
	/* --------------- */
 
	// Append the menu items to the menu bar
	menuBar->Append(menuFile, "&File");
	menuBar->Append(menuHelp, "&Help");

	// And finally, we bind all the events to their respective functions
	Bind(wxEVT_MENU, &RandoFrame::onOpen,    	 this, ID_MenuOpen);
	Bind(wxEVT_MENU, &RandoFrame::onContact,	 this, ID_MenuContact);
	Bind(wxEVT_MENU, &RandoFrame::onCredits, 	 this, ID_MenuCredits);
	Bind(wxEVT_MENU, &RandoFrame::onAbout,   	 this, wxID_ABOUT);
	Bind(wxEVT_MENU, &RandoFrame::onExit,    	 this, wxID_EXIT);
	Bind(wxEVT_MENU, &RandoFrame::onPreferences, this, wxID_PREFERENCES);
}
// ------------------------------------------------------------------

// ------------------------------------------------------------------
/* ****              ****
 * **** The Tool Bar ****
 * ****              ****
 */
void RandoFrame::populateToolBar(wxToolBar *toolBar) {
}
// ------------------------------------------------------------------

// ------------------------------------------------------------------
/* ****                ****
 * **** The Main Panel ****
 * ****                ****
 */
void RandoFrame::populateMainPanel() {
	// We need sizers for each main component
	wxBoxSizer *panelSizer  = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer *topSizer    = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer *seedSizer   = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer *outputSizer = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer *bookSizer   = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer *randomizeSizer = new wxBoxSizer(wxHORIZONTAL);

	/* Top Sizer
	 */
		wxString  diffChoices[5] = {"Casual", "Normal", "Speedrunner", "Hard", "Very Hard"};
	wxStaticText *diffText = new wxStaticText(_mainPanel, wxID_ANY, "Difficulty", wxDefaultPosition, wxDefaultSize, 0, wxEmptyString);
		wxChoice *difficulty = new wxChoice(_mainPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 5, diffChoices, 0, wxDefaultValidator, wxEmptyString);
				  difficulty->Bind(wxEVT_CHOICE, &RandoFrame::onDifficultyChoice, this);
				  difficulty->SetSelection(0);

	//_romType = new wxCheckBox(_mainPanel, wxID_ANY, "Vanilla Map", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, wxEmptyString);

	wxButton *buttonOpen = new wxButton(_mainPanel, wxID_ANY, "Load Rom", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, "Open");
	buttonOpen->SetBitmap(wxArtProvider::GetIcon(wxART_FOLDER, wxART_FRAME_ICON));
	buttonOpen->Bind(wxEVT_BUTTON, &RandoFrame::onOpen, this);

	topSizer->Add(diffText, 0, wxALIGN_CENTER_VERTICAL);
	topSizer->Add(difficulty, 0, wxGROW | wxLEFT, 5);
	topSizer->AddStretchSpacer();
	//topSizer->Add(_romType, 0, wxGROW | wxLEFT, 5);
	topSizer->AddStretchSpacer();
	topSizer->Add(buttonOpen, 0, wxGROW | wxLEFT);

	/* Seed Sizer
	 */
	wxStaticText *seedText = new wxStaticText(_mainPanel, wxID_ANY, "Seed", wxDefaultPosition, wxDefaultSize, 0, wxEmptyString);
				 _seedCtrl = new wxTextCtrl(_mainPanel, wxID_ANY, wxEmptyString , wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, wxEmptyString);

	seedSizer->Add(seedText, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 23);
	seedSizer->Add(_seedCtrl, 1, wxGROW | wxLEFT, 5);

	/* Output Sizer
	 */
	wxStaticText *outputText = new wxStaticText(_mainPanel, wxID_ANY, "Output", wxDefaultPosition, wxDefaultSize, 0, wxEmptyString);
				 _outputCtrl = new wxTextCtrl(_mainPanel, wxID_ANY, "SMPB Random.smc" , wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, wxEmptyString);
	wxStaticText *outputDesc = new wxStaticText(_mainPanel, wxID_ANY, "Save to", wxDefaultPosition, wxDefaultSize, 0, wxEmptyString);
	  wxButton *outputButton = new wxButton(_mainPanel, wxID_ANY, "Browse...", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, "Browse");
	  			outputButton->Bind(wxEVT_BUTTON, &RandoFrame::onBrowse, this);

	outputSizer->Add(outputText, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 11);
	outputSizer->Add(_outputCtrl, 1, wxGROW | wxLEFT, 5);
	outputSizer->Add(outputDesc, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
	outputSizer->Add(outputButton, 0, wxGROW | wxLEFT, 5);

	// And a sizer for the options page
	wxBoxSizer *optionsSizer = new wxBoxSizer(wxVERTICAL);

	/* Randomize Button
	 */
	// Randomize button is just a regular button
	wxButton *randomizeButton = new wxButton(_mainPanel, wxID_ANY, "Randomize");
			  randomizeButton->SetBitmap(wxArtProvider::GetIcon(wxART_GO_FORWARD, wxART_FRAME_ICON));
			  randomizeButton->Bind(wxEVT_BUTTON, &RandoFrame::onRandomize, this);

	// Log checkbox is just a simple checkbox
	_logCheck = new wxCheckBox(_mainPanel, wxID_ANY, "Output Spoiler Log", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, wxEmptyString);

	randomizeSizer->AddStretchSpacer();
	randomizeSizer->Add(_logCheck, 0, wxGROW);
	randomizeSizer->Add(randomizeButton, 0, wxGROW | wxLEFT, kMacMargins);
	randomizeSizer->AddStretchSpacer();

	/* Options Book
	 */
	wxNotebook  *optionsBook = new wxNotebook(_mainPanel, wxID_ANY);
	
	// First we need a panel for the page
	wxPanel *optionsPage = new wxPanel(optionsBook);

	/* Options widgets
	 */
	wxStaticBoxSizer *defaultBox = new wxStaticBoxSizer(wxVERTICAL, optionsPage, "Default Settings");
	wxStaticBoxSizer *gameplayBox = new wxStaticBoxSizer(wxVERTICAL, optionsPage, "Gameplay Settings");

	// The first thing we have on the options page are choice boxes for the HUD,
	  wxBoxSizer *hudSizer = new wxBoxSizer(wxHORIZONTAL);
	wxStaticText *hudText = new wxStaticText(defaultBox->GetStaticBox(), wxID_ANY, "Hud Default", wxDefaultPosition, wxDefaultSize, 0, wxEmptyString);
		wxString  hudChoices[8] = {"Normal", "Pro", "Rando", "Speed", "Prime", "Practice", "Classic", "Retro"};
				 _hud = new wxChoice(defaultBox->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 8, hudChoices, 0, wxDefaultValidator, wxEmptyString);
				 _hud->SetSelection(2);

	// The bomb type
	wxStaticText *bombText = new wxStaticText(defaultBox->GetStaticBox(), wxID_ANY, "Bomb Timer", wxDefaultPosition, wxDefaultSize, 0, wxEmptyString);
		wxString  bombChoices[3] = {"Slow", "Normal", "Fast"};
				 _bomb = new wxChoice(defaultBox->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 3, bombChoices, 0, wxDefaultValidator, wxEmptyString);
				 _bomb->SetSelection(2);

	// The controls
	wxButton *controlsButton = new wxButton(defaultBox->GetStaticBox(), wxID_ANY, "Controls");

	// The default screen of the pause screen,
	  wxBoxSizer *pauseSizer = new wxBoxSizer(wxHORIZONTAL);
	wxStaticText *pauseText = new wxStaticText(defaultBox->GetStaticBox(), wxID_ANY, "Pause Default", wxDefaultPosition, wxDefaultSize, 0, wxEmptyString);
		wxString  pauseChoices[3] = {"Map", "Equip", "Options"};
				 _pauseDefault = new wxChoice(defaultBox->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 3, pauseChoices, 0, wxDefaultValidator, wxEmptyString);
				 _pauseDefault->SetSelection(0);

	_skipCeres = new wxCheckBox(defaultBox->GetStaticBox(), wxID_ANY, "Skip Ceres", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, wxEmptyString);
	_skipCeres->SetValue(true);

	// Now add all the choice boxes to their sizers
	hudSizer->Add(hudText, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 16);
	hudSizer->Add(_hud, 0, wxGROW | wxLEFT, 5);
	hudSizer->Add(bombText, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, kMacMargins);
	hudSizer->Add(_bomb, 0, wxGROW | wxLEFT, 5);
	hudSizer->Add(controlsButton, 0, wxGROW | wxLEFT, kMacMargins);
	defaultBox->Add(hudSizer, 0, wxGROW);

	pauseSizer->Add(pauseText, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
	pauseSizer->Add(_pauseDefault, 0, wxGROW | wxLEFT, 5);
	pauseSizer->Add(_skipCeres, 0, wxGROW | wxLEFT, (kMacMargins * 10) + 6);
	defaultBox->Add(pauseSizer, 0, wxGROW | wxTOP, 5);

	// Next we make the grid
	wxGrid *grid = new wxGrid(gameplayBox->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL, wxEmptyString);
	makeGrid(grid, 26);
	
	// For the descriptions, we want the text to be lighter
	wxFont fontDesc = grid->GetDefaultCellFont();
	fontDesc.SetWeight(wxFONTWEIGHT_LIGHT);

	// And now we start by filling in the grid with the default values
	for (int i = 0; i < 26; i++) {
		if (i >= 16) {
			grid->SetCellValue(i, 0, wxString::Format("%d", (_options2 >> (i - 16)) & 1));

		} else {
			grid->SetCellValue(i, 0, wxString::Format("%d", (_options1 >> i) & 1));			
		}
		grid->SetCellValue(i, 1, _optionNames[i]);
		grid->SetCellValue(i, 2, _optionDesc[i]);
		grid->SetCellFont(i, 2, fontDesc);
		grid->SetRowSize(i, grid->GetTextExtent("000").GetWidth());
	}

	// We also need to bind the events of left clicking so we can have the checkboxes
	grid->Bind(wxEVT_GRID_CELL_LEFT_CLICK,  &RandoFrame::onOptionsGridLeftClick, this);
	grid->Bind(wxEVT_GRID_CELL_LEFT_DCLICK, &RandoFrame::onOptionsGridLeftClick, this);

	gameplayBox->Add(grid, 0, wxGROW);

	// With the grid created, we can add all the different components into the final options page sizer
	optionsSizer->Add(defaultBox, 0, wxGROW | wxLEFT | wxRIGHT, 5);
	optionsSizer->Add(gameplayBox, 1, wxGROW | wxLEFT | wxRIGHT | wxTOP | wxBOTTOM, 5);

	// And set the sizer to the options page
	optionsPage->SetSizer(optionsSizer);

	// We also need a visuals page
	wxPanel *visualsPage = new wxPanel(optionsBook);
	wxBoxSizer *visualsSizer = new wxBoxSizer(wxVERTICAL);

	wxStaticBoxSizer *paletteBox = new wxStaticBoxSizer(wxVERTICAL, visualsPage, "Palette Settings");
	wxStaticBoxSizer *gfxBox = new wxStaticBoxSizer(wxVERTICAL, visualsPage, "Graphics Settings");

	wxBoxSizer *suitSizer = new wxBoxSizer(wxHORIZONTAL);

	// Now the palette box
	_shuffleTilePal = new wxCheckBox(paletteBox->GetStaticBox(), wxID_ANY, "Shuffle Tileset Palettes", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, wxEmptyString);
	//_shuffleEnemyPal = new wxCheckBox(paletteBox->GetStaticBox(), wxID_ANY, "Shuffle Enemy Palettes", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, wxEmptyString);
	_shuffleBeamPal = new wxCheckBox(paletteBox->GetStaticBox(), wxID_ANY, "Shuffle Beam Palettes", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, wxEmptyString);

	_shuffleSuitPal = new wxCheckBox(paletteBox->GetStaticBox(), wxID_ANY, "Shuffle Suit Palettes", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, wxEmptyString);
	wxStaticText *shuffleSuitText = new wxStaticText(paletteBox->GetStaticBox(), wxID_ANY, " - Include:");
	_includePBExpanded = new wxCheckBox(paletteBox->GetStaticBox(), wxID_ANY, "PB Expanded", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, wxEmptyString);
	_includeHacks = new wxCheckBox(paletteBox->GetStaticBox(), wxID_ANY, "Hacks", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, wxEmptyString);
	_includeVanilla = new wxCheckBox(paletteBox->GetStaticBox(), wxID_ANY, "Vanilla", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, wxEmptyString);
	_includeVanilla->SetValue(true);

	suitSizer->Add(_shuffleSuitPal);
	suitSizer->Add(shuffleSuitText, 0);
	suitSizer->Add(_includePBExpanded, 0, wxLEFT, 5);
	suitSizer->Add(_includeHacks, 0, wxLEFT, 5);
	suitSizer->Add(_includeVanilla, 0, wxLEFT, 5);

	paletteBox->Add(_shuffleTilePal, 0, wxGROW | wxLEFT, 5);
	//paletteBox->Add(_shuffleEnemyPal, 0, wxGROW | wxLEFT | wxTOP, 5);
	paletteBox->Add(_shuffleBeamPal, 0, wxGROW | wxLEFT | wxTOP, 5);
	paletteBox->Add(suitSizer, 0, wxGROW | wxLEFT | wxTOP, 5);

	// Now the graphics box
	//_shuffleBeamGfx = new wxCheckBox(gfxBox->GetStaticBox(), wxID_ANY, "Shuffle Beam Graphics", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, wxEmptyString);
	//_shuffleMorphGfx = new wxCheckBox(gfxBox->GetStaticBox(), wxID_ANY, "Shuffle Morph Graphics", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, wxEmptyString);
	_shuffleFontNGfx = new wxCheckBox(gfxBox->GetStaticBox(), wxID_ANY, "Shuffle Font Graphics (numbers)", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, wxEmptyString);
	_shuffleFontTGfx = new wxCheckBox(gfxBox->GetStaticBox(), wxID_ANY, "Shuffle Font Graphics (text)", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, wxEmptyString);
	_mysteryItemGfx = new wxCheckBox(gfxBox->GetStaticBox(), wxID_ANY, "Mystery Item Graphics", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, wxEmptyString);

	//gfxBox->Add(_shuffleBeamGfx, 0, wxGROW | wxLEFT, 5);
	//gfxBox->Add(_shuffleMorphGfx, 0, wxGROW | wxLEFT | wxTOP, 5);
	gfxBox->Add(_shuffleFontNGfx, 0, wxGROW | wxLEFT | wxTOP, 5);
	gfxBox->Add(_shuffleFontTGfx, 0, wxGROW | wxLEFT | wxTOP, 5);
	gfxBox->Add(_mysteryItemGfx, 0, wxGROW | wxLEFT | wxTOP, 5);


	visualsSizer->Add(paletteBox, 0, wxGROW | wxLEFT | wxRIGHT, 5);
	visualsSizer->Add(gfxBox, 0, wxGROW | wxLEFT | wxRIGHT | wxTOP, 5);

	// And now set the sizer to the visuals page
	visualsPage->SetSizer(visualsSizer);

	// We also have a logic page
	wxPanel *logicPage = new wxPanel(optionsBook);
	wxBoxSizer *logicSizer = new wxBoxSizer(wxVERTICAL);
	
	wxStaticBoxSizer *logicOptionsBox = new wxStaticBoxSizer(wxVERTICAL, logicPage, "Optional Settings");
	wxStaticBoxSizer *energyReqBox = new wxStaticBoxSizer(wxVERTICAL, logicPage, "Ammo Requirements");
	wxStaticBoxSizer *tricksBox = new wxStaticBoxSizer(wxVERTICAL, logicPage, "Technique Settings");

	// First the options box
	_gravityHeat = new wxCheckBox(logicOptionsBox->GetStaticBox(), wxID_ANY, "Gravity protects against heat damage", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, wxEmptyString);
	_gravityHeat->Bind(wxEVT_CHECKBOX, &RandoFrame::onGravityHeatCheck, this);
	_gravityHeat->SetValue(true);

	_majorMinor = new wxCheckBox(logicOptionsBox->GetStaticBox(), wxID_ANY, "Major / Minor item split", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, wxEmptyString);

	_partyRando = new wxCheckBox(logicOptionsBox->GetStaticBox(), wxID_ANY, "Party Rando", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, wxEmptyString);

	logicOptionsBox->Add(_gravityHeat, 0, wxGROW | wxLEFT, 9);
	logicOptionsBox->Add(_majorMinor, 0, wxGROW | wxLEFT | wxTOP, 9);
	logicOptionsBox->Add(_partyRando, 0, wxGROW | wxLEFT | wxTOP, 9);

	// Next the energy requirement box
	wxBoxSizer *energySizer = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer *missileSizer = new wxBoxSizer(wxHORIZONTAL);

	wxStaticText *energyLowText = new wxStaticText(energyReqBox->GetStaticBox(), wxID_ANY, "Energy Low");
	_energyLow = new wxSpinCtrl(energyReqBox->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, 0, 14, 1, wxEmptyString);

	wxStaticText *energyMedText = new wxStaticText(energyReqBox->GetStaticBox(), wxID_ANY, "Energy Medium");
	_energyMed = new wxSpinCtrl(energyReqBox->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, 0, 14, 3, wxEmptyString);

	wxStaticText *energyHighText = new wxStaticText(energyReqBox->GetStaticBox(), wxID_ANY, "Energy High");
	_energyHigh = new wxSpinCtrl(energyReqBox->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, 0, 14, 5, wxEmptyString);

	wxStaticText *missilesLowText = new wxStaticText(energyReqBox->GetStaticBox(), wxID_ANY, "Missiles Low");
	_missilesLow = new wxSpinCtrl(energyReqBox->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, 0, 50, 2, wxEmptyString);

	wxStaticText *missilesMedText = new wxStaticText(energyReqBox->GetStaticBox(), wxID_ANY, "Missiles Medium");
	_missilesMed = new wxSpinCtrl(energyReqBox->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, 0, 50, 8, wxEmptyString);

	wxStaticText *missilesHighText = new wxStaticText(energyReqBox->GetStaticBox(), wxID_ANY, "Missiles High");
	_missilesHigh = new wxSpinCtrl(energyReqBox->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, 0, 50, 15, wxEmptyString);

	energySizer->Add(energyLowText, 0, wxALIGN_CENTER_VERTICAL);
	energySizer->Add(_energyLow, 0, wxLEFT, 5);
	energySizer->Add(energyMedText, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 15);
	energySizer->Add(_energyMed, 0, wxLEFT, 5);
	energySizer->Add(energyHighText, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 15);
	energySizer->Add(_energyHigh, 0, wxLEFT, 5);

	missileSizer->Add(missilesLowText, 0, wxALIGN_CENTER_VERTICAL);
	missileSizer->Add(_missilesLow, 0, wxLEFT, 5);
	missileSizer->Add(missilesMedText, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 7);
	missileSizer->Add(_missilesMed, 0, wxLEFT, 5);
	missileSizer->Add(missilesHighText, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 7);
	missileSizer->Add(_missilesHigh, 0, wxLEFT, 5);

	energyReqBox->Add(energySizer, 0, wxGROW | wxLEFT, 16);
	energyReqBox->Add(missileSizer, 0, wxGROW | wxLEFT, 9);

	// And lastly the tricks box
	_tricksGrid = new wxGrid(tricksBox->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL, wxEmptyString);
	makeGrid(_tricksGrid, 7);

	// And now we start by filling in the grid with the default values
	for (int i = 0; i < 7; i++) {
		_tricksGrid->SetCellValue(i, 0, wxString::Format("%ld", (_tricks1 >> (i + 24)) & 1));			

		_tricksGrid->SetCellValue(i, 1, _trickNames[i]);
		_tricksGrid->SetRowSize(i, _tricksGrid->GetTextExtent("000").GetWidth());
	}
	tricksBox->Add(_tricksGrid, 0, wxGROW);

	// We also need to bind the events of left clicking so we can have the checkboxes
	_tricksGrid->Bind(wxEVT_GRID_CELL_LEFT_CLICK,  &RandoFrame::onTechniqueGridLeftClick, this);
	_tricksGrid->Bind(wxEVT_GRID_CELL_LEFT_DCLICK, &RandoFrame::onTechniqueGridLeftClick, this);

	logicSizer->Add(logicOptionsBox, 0, wxGROW | wxLEFT | wxRIGHT | wxTOP | wxBOTTOM, 5);
	logicSizer->Add(energyReqBox, 0, wxGROW | wxLEFT | wxRIGHT | wxTOP | wxBOTTOM, 5);
	logicSizer->Add(tricksBox, 1, wxGROW | wxLEFT | wxRIGHT | wxTOP | wxBOTTOM, 5);

	logicPage->SetSizer(logicSizer);

	// And a log page
	   wxPanel *logPage = new wxPanel(optionsBook);
	wxBoxSizer *logSizer = new wxBoxSizer(wxHORIZONTAL);
			   _logCtrl = new wxTextCtrl(logPage, wxID_ANY, wxEmptyString , wxDefaultPosition, wxDefaultSize, wxTE_READONLY | wxTE_MULTILINE, wxDefaultValidator, wxEmptyString);

	logSizer->Add(_logCtrl, 1, wxGROW | wxLEFT | wxRIGHT | wxTOP | wxBOTTOM, 5);
	logPage->SetSizer(logSizer);

	// We can add the pages to the notebook
	optionsBook->AddPage(optionsPage, "Options");
	optionsBook->AddPage(visualsPage, "Visuals");
	optionsBook->AddPage(logicPage, "Logic");
	optionsBook->AddPage(logPage, "Log");

	// And the book to the book sizer
	bookSizer->Add(optionsBook, 1, wxGROW);

	// Now we add all the sizers to the panel sizer
	panelSizer->Add(topSizer, 0, wxGROW | wxTOP | wxLEFT | wxRIGHT, kMacMargins);
	panelSizer->Add(seedSizer, 0, wxGROW | wxTOP | wxLEFT | wxRIGHT, kMacMargins);
	panelSizer->Add(outputSizer, 0, wxGROW | wxTOP |wxLEFT | wxRIGHT, kMacMargins);
	panelSizer->Add(randomizeSizer, 0, wxGROW | wxTOP | wxLEFT | wxRIGHT, kMacMargins);
	panelSizer->Add(bookSizer, 1, wxGROW | wxTOP | wxLEFT | wxRIGHT | wxBOTTOM, kMacMargins);

	// And we layout the panel sizer, setting it to the main panel
	panelSizer->Layout();
	_mainPanel->SetSizer(panelSizer);
}

void RandoFrame::makeGrid(wxGrid *grid, int numRows) {
	/* Before creating the grid, we need to set the properties
	 * so that it looks like it isn't a grid (but we still get the
	 * performance benefit of the virtual gridtable backend)
	 */
	grid->EnableGridLines(false);

	// The headers are obviously not relevant in this one
	grid->SetRowLabelSize(0);
	grid->SetColLabelSize(0);

	// We don't want the user changing any of the row/col sizes
	grid->DisableDragColMove();
	grid->DisableDragRowMove();
	grid->DisableDragColSize();
	grid->DisableDragGridSize();
	grid->DisableDragRowSize();
	grid->DisableCellEditControl();
	grid->EnableEditing(false);

	// We don't want a highlight box around the entry
	grid->SetCellHighlightPenWidth(0);
	grid->SetCellHighlightROPenWidth(0);

	// We also want the alignment of everything to be centered
	grid->SetDefaultCellAlignment(wxALIGN_LEFT, wxALIGN_CENTER);

	// And lastly, we want the entries to be drawn directly on the panel, so we use a transparent background
	// *** THIS IS MAC SPECIFIC, AND DOES NOT WORK IN WINDOWS ***
	grid->SetDefaultCellBackgroundColour(wxColour(0,0,0,0));

	// Now we can actually create the grid
	grid->CreateGrid(numRows, 3, wxGrid::wxGridSelectionModes::wxGridSelectNone);

	// This is where we make the checkboxes visible, by changing the attribute of the column to a bool renderer
	wxGridCellAttr *checkBoxAttr = new wxGridCellAttr();
	checkBoxAttr->SetReadOnly(false);
	checkBoxAttr->SetEditor(new wxGridCellBoolEditor());
	checkBoxAttr->SetRenderer(new wxGridCellBoolRenderer());
	checkBoxAttr->SetAlignment(wxALIGN_CENTER, wxALIGN_CENTER);
	grid->SetColAttr(0, checkBoxAttr);

	// We also want the size of the columns to match the contents
	grid->SetColSize(0, grid->GetTextExtent("0000").GetWidth());
	grid->SetColSize(1, grid->GetTextExtent("Screw Attack Glow0").GetWidth());
	grid->SetColSize(2, grid->GetTextExtent("(Hold item cancel and press down to morph)0").GetWidth());
}

void RandoFrame::onGravityHeatCheck(wxCommandEvent &event) {
	if (event.IsChecked() == true) {
		_tricks1 |= 0x80000000;
	} else {
		_tricks1 &= 0x7FFFFFFF;
	}
}

void RandoFrame::onDifficultyChoice(wxCommandEvent &event) {
	switch (event.GetSelection()) {
		default:
		// Casual
		case 0:
			_gravityHeat->SetValue(true);
			_energyLow->SetValue(1);
			_energyMed->SetValue(3);
			_energyHigh->SetValue(5);
			_tricks1 = 0x80000000;
			break;
		// Normal
		case 1:
			_gravityHeat->SetValue(false);
			_energyLow->SetValue(1);
			_energyMed->SetValue(2);
			_energyHigh->SetValue(3);
			_tricks1 = 0xB000000;
			break;
		// Speedrunner
		case 2:
			_gravityHeat->SetValue(false);
			_energyLow->SetValue(0);
			_energyMed->SetValue(1);
			_energyHigh->SetValue(2);
			_tricks1 = 0x5B000000;
			break;
		// Hard
		case 3:
			_gravityHeat->SetValue(false);
			_energyLow->SetValue(0);
			_energyMed->SetValue(0);
			_energyHigh->SetValue(0);
			_tricks1 = 0x7B000000;
			break;
		// Hard with heat runs
		case 4:
			_gravityHeat->SetValue(false);
			_energyLow->SetValue(0);
			_energyMed->SetValue(0);
			_energyHigh->SetValue(0);
			_tricks1 = 0x7F000000;
			break;
	}

	for (int i = 0; i < 7; i++) {
		_tricksGrid->SetCellValue(i, 0, wxString::Format("%ld", (_tricks1 >> (i + 24)) & 1));			
	}
	_tricksGrid->ForceRefresh();
}

void RandoFrame::onOptionsGridLeftClick(wxGridEvent &event) {
	int currentRow = event.GetRow();
	if ((event.GetCol() == 0) || (event.GetCol() == 1)) {
		wxGrid *grid = (wxGrid *) event.GetEventObject();
		wxString value = grid->GetCellValue(currentRow, 0);

		if (value == "0") {
			grid->SetCellValue(currentRow, 0, "1");

			if (currentRow >= 16) {
				_options2 |= (1 << (currentRow - 16));

			} else {
				_options1 |= (1 << currentRow);
			}

		} else {
			grid->SetCellValue(currentRow, 0, "0");
			
			if (currentRow >= 16) {
				_options2 &= ~(1 << (currentRow - 16));

			} else {
				_options1 &= ~(1 << currentRow);
			}
		}

	grid->Refresh();
	}
}

void RandoFrame::onTechniqueGridLeftClick(wxGridEvent &event) {
	int currentRow = event.GetRow();
	if ((event.GetCol() == 0) || (event.GetCol() == 1)) {
		wxGrid *grid = (wxGrid *) event.GetEventObject();
		wxString value = grid->GetCellValue(currentRow, 0);

		if (value == "0") {
			grid->SetCellValue(currentRow, 0, "1");
			_tricks1 |= (1 << (currentRow + 24));

		} else {
			grid->SetCellValue(currentRow, 0, "0");
			_tricks1 &= ~(1 << (currentRow + 24));
		}

	grid->Refresh();
	}
}

void RandoFrame::onOpen(wxCommandEvent& event) {
	// Prompt the user to open a rom file
	wxFileDialog open(this, _("Open base ROM file"), "", "", "SNES Rom files (*.smc)|*.smc", wxFD_OPEN | wxFD_FILE_MUST_EXIST);

	// If they decide to cancel, just return
	if (open.ShowModal() == wxID_CANCEL) {
		return;
	}

	// Base path will be used later
	_basePath = open.GetPath();

	if (!wxFile::Exists(_basePath)) {
		wxLogError("Cannot open file");
		return;
	}

	// Get the rom loaded in
	_rom = new Rom(_basePath);
}

void RandoFrame::onBrowse(wxCommandEvent& event) {
	wxFileDialog open(this, _("Choose file to save as"), "", "", "SNES Rom files (*.smc)|*.smc", wxFD_SAVE);

	if (open.ShowModal() != wxID_CANCEL) {
		_outputPath = open.GetPath();
		_outputCtrl->SetValue(_outputPath);
	}
}

void RandoFrame::onPreferences(wxCommandEvent& event) {
}

void RandoFrame::onContact(wxCommandEvent& event) {
	wxMessageBox("Discord: Quote58#6249\nTwitter: @Quote_58", "Please report issues to:", wxOK | wxICON_INFORMATION);
}
 
void RandoFrame::onCredits(wxCommandEvent& event) {
	wxMessageBox("MetroidConstruction.com","Special thanks to: Dessy, Gala, Begrimed", wxOK | wxICON_INFORMATION);
}

void RandoFrame::onAbout(wxCommandEvent& event) {
	wxMessageBox("Project Base Rando is a program for creating randomized roms of Super Metroid Project Base", "About Project Base Rando", wxOK | wxICON_INFORMATION);
}

void RandoFrame::onExit(wxCommandEvent& event) {
	// This true ensures that this close button has vito over all windows
	Close(true);
}
// ------------------------------------------------------------------





























