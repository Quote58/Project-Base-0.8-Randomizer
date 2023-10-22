// Project Base Randomizer
#include "rando.h"
#include "rom.h"

#include "okcolor.h"

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

	// Haze is removed in PB, this just puts it back in vanilla
	if (_romType->GetValue() == true) {
		_rom->setByte(0x059C98, 0x88);	// Call to hdma during title screen, in PB this is 93 to deactivate hdma
		_rom->setByte(0x00924C, 0x88);  // Call to hdma during game over screen, in PB this is 93 to deactivate hdma
		_rom->setByte(0x045DC7, 0xA9);	// The actual call to the hdma routine for starting haze, in PB this is an RTL
	}

	// This is to undo all the alternate palettes for tilesets in PB. The palette rando is not taking those into account
	if (_romType->GetValue() == false) {
		_rom->setByte(0x1bcff, 0x00);
		_rom->setByte(0x1bd0f, 0x00);
		_rom->setByte(0x1849f, 0x00);
		_rom->setByte(0x1c33f, 0x00);
		_rom->setByte(0x1c27f, 0x00);
		_rom->setByte(0x18323, 0x00);
		_rom->setByte(0x19DA3, 0x00);
		_rom->setByte(0x186AF, 0x00);
		_rom->setByte(0x1809F, 0x00);
		_rom->setByte(0x1808F, 0x00);
		_rom->setByte(0x1806F, 0x00);
		_rom->setByte(0x19D73, 0x00);
		_rom->setByte(0x19D83, 0x00);
		_rom->setByte(0x19D03, 0x00);
		_rom->setByte(0x19CC3, 0x00);
		_rom->setByte(0x19CB3, 0x00);
		_rom->setByte(0x19B11, 0x00);
		_rom->setByte(0x19B31, 0x00);
		_rom->setByte(0x19B41, 0x00);
		_rom->setByte(0x197FF, 0x00);
		_rom->setByte(0x19C33, 0x00);
		_rom->setByte(0x19B73, 0x00);
		_rom->setByte(0x1B2BF, 0x00);
		_rom->setByte(0x1B2CF, 0x00);
		_rom->setByte(0x19F83, 0x00);
		_rom->setByte(0x1BCFF, 0x00);
		_rom->setByte(0x1BD0F, 0x00);
	}

	// This one adjusts the event state so that zebes awakens after any (?) item is acquired
	_rom->applyPatch(_patchEvents);

	// The vanilla patch doesn't include the dash ball gfx, so we need to include it as a patch
	if (_romType->GetValue() == true) {
		_rom->applyPatch(_patchDashBall);
	}

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
	if (_shuffleTilePal->GetValue() == true) { makeNewTilesetPalettes(); }
	if (_shuffleBeamPal->GetValue() == true) { shuffleBeamPalettes(); }
	if (_shuffleSuitPal->GetValue() == true) { shuffleSuitPalettes(); }
	if (_shuffleFontNGfx->GetValue() == true) { shuffleFontNumbers(); }
	if (_shuffleFontTGfx->GetValue() == true) { shuffleFontText(); }

	// Shuffle FX1 needs to happen before flood mode for obvious reasons
	if (_shuffleFX1->GetValue() == true) { shuffleFX1(); }
	if (_floodMode->GetValue() == true) { floodFX1(); }

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

void RandoFrame::floodFX1() {
	for (int i = 0x018000; i < 0x0188FC; i += 16) {
		if (_rom->getByte(i) != 0xFF) {
			wxByte type = _rom->getByte(i + 9);
			if (type < 0x0D) {
				if (rand() % 2 == 0) {
					_rom->setByte(i + 9, 0x06);
					_rom->setByte(i + 15, 0x48);
					_rom->setByte(i + 7, 0x50);
					_rom->setByte(i + 8, 0x0);
					_rom->setWord(i + 2, rand() % 0x00FF);
					_rom->setWord(i + 4, 0xFFFF);
				}
			}

		} else {
			// If it's an FFFF, we want to move back by a full row minus one word to realign
			i -= 14;
		}
	}
}

void RandoFrame::shuffleFX1() {
	for (int i = 0x018000; i < 0x0188FC; i += 16) {
		if (_rom->getByte(i) != 0xFF) {
			wxByte type = _rom->getByte(i + 9);
			if (type < 0x0D) {
				if ((type > 0) && (type < 8)) {
					// Type is a liquid, so choose only liquids to replace it with
					wxByte randLiquid = type;

					// We want it to always pick a new type
					while (randLiquid == type) {
						randLiquid = ((rand() % 3) * 2) + 2;
					}

					_rom->setByte(i + 9, randLiquid);
					
					switch (randLiquid) {
						case 0x02:
							// For lava, we want to make the blend 0x02
							_rom->setByte(i + 15, 0x02);
							break;
						case 0x04:
							// Acid doesn't really need a blend because it uses the yellow in the tileset
							_rom->setByte(i + 15, 0x48);
							break;
						case 0x06:
							// But water does need a colour, so we'll use 0x48 for now
							_rom->setByte(i + 15, 0x48);
						default:
							break;
					}
				
				} else {
					wxByte randOption = type;

					// We want it to always pick a new type
					while (randOption == type) {
						randOption = ((rand() % 3) * 2) + 8;
					}

					_rom->setByte(i + 9, randOption);

					switch (randOption) {
						case 0x08:
							// For spores, we want a blend and also movement
							_rom->setByte(i + 15, 0x48);
							_rom->setWord(i + 2, 0xFFC0);
							break;
						case 0x0A:
							// Rain is pretty simple
							_rom->setByte(i + 15, 0x22);
							break;
						case 0x0C:
							// Fog has a lot of good blend options, might need to tweak this
							_rom->setByte(i + 15, 0x62);
							_rom->setWord(i + 2, 0xFFF0);
							_rom->setWord(i + 4, 0x0060);
							break;
						default:
							break;
					}
				}		
			}

			

		} else {
			// If it's an FFFF, we want to move back by a full row minus one word to realign
			i -= 14;
		}
	}
}

void RandoFrame::makeNewTilesetPalettes() {
	uint64_t palettes[29] = {0x212D7C, 0x212F7C, 0x21317C, 0x21337C, 0x21357C, 0x21377C, 0x21397C, 0x213B7C, 0x213D7C, 0x213F7C,
							 0x1C0000, 0x1C0200, 0x1C0400, 0x1C0600, 0x1C0800, 0x1C0A00, 0x1C0C00, 0x1C0A00, 0x1C0C00, 0x1C0A00,
							 0x1C0C00, 0x1C0E00, 0x1C1000, 0x1C1200, 0x1C1400, 0x1C1600, 0x1C1800, 0x1C1A00, 0x1C1C00};

	uint64_t palettesSnes[29] = {0x42AD7C, 0x42AF7C, 0x42B17C, 0x42B37C, 0x42B57C, 0x42B77C, 0x42B97C, 0x42BB7C, 0x42BD7C, 0x42BF7C,
								 0x388000, 0x388200, 0x388400, 0x388600, 0x388800, 0x388A00, 0x388C00, 0x388A00, 0x388C00, 0x388A00,
								 0x388C00, 0x388E00, 0x389000, 0x389200, 0x389400, 0x389600, 0x389800, 0x389A00, 0x389C00};

	// These are for getting the original palettes out of a vanilla rom (should probably just use the table instead, that would work for both games)		 
	uint64_t palettesVanilla[13] = {0x212D7C, 0x212E5D, 0x212F43, 0x213015, 0x2130E7, 0x2131A6, 0x213264, 0x21335F, 0x213447, 0x2135E4, 0x2136BB, 0x21383C, 0x21392E};


	// Even in PB the palette locations are a little small, so we're just going to repoint all of them
	for (int i = 0; i < 29; i++) {
		_rom->setByte(kAddrGfxPal + (i * 9), palettesSnes[i] & 0x0000FF);
		_rom->setByte(kAddrGfxPal + (i * 9 + 1), (palettesSnes[i] & 0x00FF00) >> 8);
		_rom->setByte(kAddrGfxPal + (i * 9 + 2), (palettesSnes[i] & 0xFF0000) >> 16);
	}

	// Now we set up the tileset palettes themselves
	wxColour t1[128]; wxColour t2[128]; wxColour t3[128]; wxColour t4[128]; wxColour t5[128]; wxColour t6[128]; wxColour t7[128]; wxColour t8[128]; wxColour t9[128]; wxColour t10[128];
	wxColour t11[128]; wxColour t12[128]; wxColour t13[128]; wxColour t14[128]; wxColour t15[128]; wxColour t16[128]; wxColour t17[128]; wxColour t18[128]; wxColour t19[128]; wxColour t20[128];
	wxColour t21[128]; wxColour t22[128]; wxColour t23[128]; wxColour t24[128]; wxColour t25[128];

	wxColour *tilesets[29] = {t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15, t16, t17, nullptr, nullptr, nullptr, nullptr, t18, t19, t20, t21, t22, t23, t24, t25};

	// Next we define each template as a collection of colour objects
	// PB
	wxVector<PalColour *> template1; template1.push_back(new PalColour(0x04, kObjNone, kRelNone, -1, 0.4f, 4, 80, 5, 10.0)); template1.push_back(new PalColour(0x41, kObjNone, kRelNone, -1, 1, 3, 80, 30)); template1.push_back(new PalColour(0x44, kObjNone, kRelTriadic, 0x41, 0.7f, 3, 110, 30, 10.0)); template1.push_back(new PalColour(0x47, kObjNone, kRelNone, 0x46, 1, 0, 20, -1)); template1.push_back(new PalColour(0x48, kObjNone, kRelAnalogous, 0x44, 0.7f, 4, 100, 20)); template1.push_back(new PalColour(0x4C, 0x41, 120)); template1.push_back(new PalColour(0x51, kObjMetal, kRelNone, -1, 1, 3, 80, 30)); template1.push_back(new PalColour(0x58, kObjNone, kRelCompliment, 0x48, 1, 4, 100, 30)); template1.push_back(new PalColour(0x5C, 0x53, 60)); template1.push_back(new PalColour(0x61, 0x41, 3, 50, 1)); template1.push_back(new PalColour(0x64, kObjNone, kRelTriadic, 0x44, 0.5f, 4, 40, 1)); template1.push_back(new PalColour(0x68, 0x48, 4, 50, 1)); template1.push_back(new PalColour(0x6C, 0x4C, 60)); template1.push_back(new PalColour(0x74, kObjStone, kRelNone, -1, 1, 4, 100, 30)); template1.push_back(new PalColour(0x78, 0x48, 4, 100, 30));
	wxVector<PalColour *> template2; template2.push_back(new PalColour(0x04, kObjNone, kRelNone, -1, 0.4f, 4, 80, 5, 10.0)); template2.push_back(new PalColour(0x34, kObjMetal, kRelNone, -1, 0.7f, 4, 50, 5)); template2.push_back(new PalColour(0x34, kObjNone, kRelNone, -1, 0.7f, 4, 30, 5)); template2.push_back(new PalColour(0x3E, 0x34, 110)); template2.push_back(new PalColour(0x41, kObjNone, kRelNone, -1, 1, 3, 80, 30)); template2.push_back(new PalColour(0x44, kObjNone, kRelTriadic, 0x41, 0.7f, 3, 110, 30, 10.0)); template2.push_back(new PalColour(0x47, kObjNone, kRelNone, 0x46, 1, 0, 20, -1)); template2.push_back(new PalColour(0x48, kObjNone, kRelAnalogous, 0x44, 0.7f, 4, 100, 20)); template2.push_back(new PalColour(0x4C, 0x41, 120)); template2.push_back(new PalColour(0x4D, 0x4C, 60)); template2.push_back(new PalColour(0x4E, 0x4C, 100)); template2.push_back(new PalColour(0x51, kObjMetal, kRelNone, -1, 0.4f, 3, 80, 40)); template2.push_back(new PalColour(0x57, wxBLACK)); template2.push_back(new PalColour(0x58, kObjNone, kRelCompliment, 0x48, 0.8f, 4, 80, 20, 10.0)); template2.push_back(new PalColour(0x5C, 0x53, 70)); template2.push_back(new PalColour(0x61, 0x41, 3, 60, 5)); template2.push_back(new PalColour(0x64, 0x44, 4, 60, 5));	template2.push_back(new PalColour(0x68, 0x48, 4, 60, 5)); template2.push_back(new PalColour(0x6C, 0x4C, 50)); template2.push_back(new PalColour(0x6E, 0x4E, 50)); template2.push_back(new PalColour(0x71, 0x41, 3, 80, 30)); template2.push_back(new PalColour(0x74, kObjMetal, kRelNone, 0x74, 1, 4, 100, 30, 0, 0)); template2.push_back(new PalColour(0x78, 0x48, 4, 100, 20)); template2.push_back(new PalColour(0x7C, kObjNatural, kRelNone, -1, 1, 0, 80, -1)); template2.push_back(new PalColour(0x7D, 0x7C, 120));
	wxVector<PalColour *> template3; template3.push_back(new PalColour(0x04, kObjNone, kRelNone, -1, 0.4f, 4, 80, 5, 10.0)); template3.push_back(new PalColour(0x41, kObjNone, kRelNone, -1, 0.7f, 3, 80, 30)); template3.push_back(new PalColour(0x44, kObjNone, kRelCompliment, 0x41, 0.7f, 4, 80, 20, 10.0)); template3.push_back(new PalColour(0x48, kObjNatural, kRelNone, -1, 1, 2, 100, 70)); template3.push_back(new PalColour(0x4A, kObjNone, kRelAnalogous, 0x48, 1, 2, 70, 40)); template3.push_back(new PalColour(0x4E, 0x41, 110)); template3.push_back(new PalColour(0x51, 0x41, 3, 60, 5)); template3.push_back(new PalColour(0x54, kObjMetal, kRelNone, -1, 1, 4, 40, 5, 10.0)); template3.push_back(new PalColour(0x58, kObjNatural, kRelNone, -1, 1, 2, 100, 80)); template3.push_back(new PalColour(0x5A, 0x4A, 2, 100, 40)); template3.push_back(new PalColour(0x5C, 0x53, 30)); template3.push_back(new PalColour(0x5D, 0x58, 120)); template3.push_back(new PalColour(0x5E, 0x51, 120)); template3.push_back(new PalColour(0x61, kObjMetal, kRelNone, -1, 0.7f, 3, 80, 30)); template3.push_back(new PalColour(0x64, 0x54, 4, 100, 30)); template3.push_back(new PalColour(0x68, kObjMetal, kRelNone, -1, 1, 4, 60, 5)); template3.push_back(new PalColour(0x71, 0x51, 3, 80, 20)); template3.push_back(new PalColour(0x74, 0x54, 3, 120, 80)); template3.push_back(new PalColour(0x78, kObjMetal, kRelNone, -1, 1, 4, 100, 20)); template3.push_back(new PalColour(0x7E, 0x71, 120));
	wxVector<PalColour *> template4; template4.push_back(new PalColour(0x04, kObjNone, kRelNone, -1, 0.4f, 4, 80, 5, 10.0)); template4.push_back(new PalColour(0x41, kObjNone, kRelNone, -1, 0.7f, 3, 80, 30)); template4.push_back(new PalColour(0x44, kObjNone, kRelNone, 0x44, 0.7f, 4, 80, 20, 10.0, 0)); template4.push_back(new PalColour(0x48, kObjNatural, kRelNone, -1, 1, 2, 100, 70)); template4.push_back(new PalColour(0x4B, kObjNone, kRelAnalogous, 0x48, 1, -2, 70, 40)); template4.push_back(new PalColour(0x4E, 0x41, 110)); template4.push_back(new PalColour(0x51, 0x41, 3, 60, 5)); template4.push_back(new PalColour(0x54, kObjMetal, kRelNone, -1, 1, 4, 40, 5, 10.0)); template4.push_back(new PalColour(0x58, kObjNatural, kRelNone, -1, 1, 2, 100, 80)); template4.push_back(new PalColour(0x5A, 0x4A, 2, 100, 40)); template4.push_back(new PalColour(0x5C, 0x53, 30)); template4.push_back(new PalColour(0x5D, 0x58, 120)); template4.push_back(new PalColour(0x5E, 0x51, 120)); template4.push_back(new PalColour(0x61, kObjMetal, kRelNone, -1, 0.7f, 3, 80, 30)); template4.push_back(new PalColour(0x64, 0x54, 4, 100, 30)); template4.push_back(new PalColour(0x68, kObjMetal, kRelNone, -1, 1, 4, 60, 5)); template4.push_back(new PalColour(0x71, 0x51, 3, 80, 20)); template4.push_back(new PalColour(0x74, 0x54, 3, 120, 80)); template4.push_back(new PalColour(0x78, kObjMetal, kRelNone, -1, 1, 4, 100, 20)); template4.push_back(new PalColour(0x7E, 0x71, 120));
	wxVector<PalColour *> template5; template5.push_back(new PalColour(0x04, kObjNone, kRelNone, -1, 0.4f, 4, 80, 5, 10.0)); template5.push_back(new PalColour(0x41, kObjNatural, kRelNone, 0x41, 1, 3, 70, 30, 0, 0)); template5.push_back(new PalColour(0x44, kObjMetal, kRelNone, 0x74, 1, 4, 70, 10, 0, 0)); template5.push_back(new PalColour(0x48, kObjNone, kRelCompliment, 0x44, 1, 4, 80, 10)); template5.push_back(new PalColour(0x4C, kObjNone, kRelNone, 0x79, 1, 0, 100, -1, 0, 0)); template5.push_back(new PalColour(0x4D, kObjNone, kRelNone, 0x78, 1, 0, 100, -1, 0, 0)); template5.push_back(new PalColour(0x4E, 0x44, 120)); template5.push_back(new PalColour(0x51, 0x41, 3, 40, 5)); template5.push_back(new PalColour(0x54, 0x44, 4, 40, 5)); template5.push_back(new PalColour(0x58, kObjNone, kRelCompliment, 0x44, 1, 4, 120, 40)); template5.push_back(new PalColour(0x71, kObjMetal, kRelNone, -1, 1, 3, 20, 5)); template5.push_back(new PalColour(0x74, kObjMetal, kRelNone, -1, 1, 4, 20, 5)); template5.push_back(new PalColour(0x78, kObjMetal, kRelNone, -1, 1, 4, 20, 5)); template5.push_back(new PalColour(0x7D, kObjNatural, kRelNone, -1, 1, -2, 80, 60));
	wxVector<PalColour *> template6; template6.push_back(new PalColour(0x04, kObjNone, kRelNone, -1, 0.4f, 4, 80, 5, 10.0)); template6.push_back(new PalColour(0x41, kObjNatural, kRelNone, -1, 1, 3, 70, 30, 0)); template6.push_back(new PalColour(0x44, kObjMetal, kRelNone, 0x74, 1, 4, 70, 10, 0, 0)); template6.push_back(new PalColour(0x48, kObjNone, kRelCompliment, 0x44, 1, 4, 80, 10)); template6.push_back(new PalColour(0x4C, kObjNone, kRelNone, 0x79, 1, 0, 100, -1, 0, 0)); template6.push_back(new PalColour(0x4D, kObjNone, kRelNone, 0x78, 1, 0, 100, -1, 0, 0)); template6.push_back(new PalColour(0x4E, 0x44, 120)); template6.push_back(new PalColour(0x51, 0x41, 3, 60, 5)); template6.push_back(new PalColour(0x54, 0x44, 4, 60, 5)); template6.push_back(new PalColour(0x58, kObjNone, kRelCompliment, 0x44, 1, 4, 120, 40)); template6.push_back(new PalColour(0x71, kObjMetal, kRelNone, -1, 1, 3, 40, 5)); template6.push_back(new PalColour(0x74, kObjMetal, kRelNone, -1, 1, 4, 40, 5)); template6.push_back(new PalColour(0x78, kObjMetal, kRelNone, -1, 1, 4, 40, 5)); template6.push_back(new PalColour(0x7D, kObjNatural, kRelNone, -1, 1, -2, 80, 60));
	wxVector<PalColour *> template7; template7.push_back(new PalColour(0x04, kObjNone, kRelNone, -1, 0.4f, 4, 80, 5, 10.0)); template7.push_back(new PalColour(0x28, kObjNatural, kRelNone, -1, 1, 4, 50, 5)); template7.push_back(new PalColour(0x2C, new wxColour(227,194,212))); template7.push_back(new PalColour(0x31, kObjNatural, kRelNone, -1, 1, 7, 60, 10)); template7.push_back(new PalColour(0x48, kObjNatural, kRelNone, -1, 0.8f, 3, 100, 60)); template7.push_back(new PalColour(0x44, kObjNone, kRelNone, 0x49, 1, 4, 80, 10, 10.0)); template7.push_back(new PalColour(0x43, 0x46, 80)); template7.push_back(new PalColour(0x4C, 0x48, 100)); template7.push_back(new PalColour(0x4E, 0x4C, 110)); template7.push_back(new PalColour(0x51, kObjNatural, kRelNone, -1, 1, 3, 100, 60, 10.0)); template7.push_back(new PalColour(0x54, kObjNone, kRelNone, 0x4C, 0.7f, 4, 100, 60, 10.0)); template7.push_back(new PalColour(0x58, 0x54, 40)); template7.push_back(new PalColour(0x24, 0x54, 4, 100, 20)); template7.push_back(new PalColour(0x61, kObjNatural, kRelNone, -1, 0.6f, 7, 90, 5)); template7.push_back(new PalColour(0x68, kObjNone, kRelCompliment, 0x61, 1, 4, 100, 5)); template7.push_back(new PalColour(0x71, kObjNone, kRelCompliment, 0x61, 1, 3, 100, 5)); template7.push_back(new PalColour(0x74, 0x72, 4, 60, 5));
	wxVector<PalColour *> template8; template8.push_back(new PalColour(0x04, kObjNone, kRelNone, -1, 0.4f, 4, 80, 5, 10.0)); template8.push_back(new PalColour(0x31, kObjNatural, kRelNone, -1, 1, 7, 60, 10)); template8.push_back(new PalColour(0x3C, 0x31, 110)); template8.push_back(new PalColour(0x3D, 0x33, 140)); template8.push_back(new PalColour(0x3E, kObjNatural, kRelNone, -1, 1, 0, 100, -1)); template8.push_back(new PalColour(0x41, kObjNone, kRelNone, -1, 1, 3, 100, 50, 20.0)); template8.push_back(new PalColour(0x44, kObjNone, kRelNone, 0x41, 0.8f, 8, 90, 1)); template8.push_back(new PalColour(0x4C, 0x41, 100)); template8.push_back(new PalColour(0x53, kObjMetal, kRelNone, -1, 0.4f, 5, 100, 5)); template8.push_back(new PalColour(0x58, kObjNone, kRelNone, -1, 1, 4, 100, 20, 20.0)); template8.push_back(new PalColour(0x5C, 0x53, 30)); template8.push_back(new PalColour(0x61, kObjNatural, kRelNone, -1, 1, 0, 100, -1)); template8.push_back(new PalColour(0x64, 0x61, 8, 100, 5)); template8.push_back(new PalColour(0x6C, 0x61, 120)); template8.push_back(new PalColour(0x71, kObjNone, kRelNone, -1, 1, 4, 100, 40)); template8.push_back(new PalColour(0x75, 0x73, 3, 100, 5)); template8.push_back(new PalColour(0x78, kObjNone, kRelCompliment, 0x71, 1, 3, 100, 50, 20.0)); template8.push_back(new PalColour(0x7B, 0x77, 90)); template8.push_back(new PalColour(0x7C, 0x71, 120)); template8.push_back(new PalColour(0x7D, 0x73, 100)); template8.push_back(new PalColour(0x7E, 0x71, 110));
	wxVector<PalColour *> template10; template10.push_back(new PalColour(0x04, kObjNone, kRelNone, -1, 0.4f, 4, 80, 5, 10.0)); template10.push_back(new PalColour(0x34, kObjNatural, kRelNone, -1, 1, 4, 100, 50, 10.0)); template10.push_back(new PalColour(0x38, kObjMetal, kRelNone, -1, 1, 4, 100, 5)); template10.push_back(new PalColour(0x3C, 0x35, 100)); template10.push_back(new PalColour(0x3D, 0x39, 100)); template10.push_back(new PalColour(0x41, kObjNatural, kRelNone, -1, 1, 3, 100, 70, 10.0)); template10.push_back(new PalColour(0x44, kObjNone, kRelNone, -1, 1, 4, 100, 20)); template10.push_back(new PalColour(0x48, kObjNone, kRelNone, -1, 0.7f, 3, 100, 40)); template10.push_back(new PalColour(0x4B, 0x4A, 10)); template10.push_back(new PalColour(0x4C, 0x45, 100)); template10.push_back(new PalColour(0x4D, kObjNone, kRelAnalogous, 0x48, 1, 0, 120, -1)); template10.push_back(new PalColour(0x51, 0x41, 3, 100, 70, 10.0)); template10.push_back(new PalColour(0x54, kObjStone, kRelNone, -1, 0.7f, 4, 80, 5)); template10.push_back(new PalColour(0x58, kObjNone, kRelNone, -1, 1, 4, 100, 5)); template10.push_back(new PalColour(0x5C, 0x55, 100)); template10.push_back(new PalColour(0x5D, 0x59, 100)); template10.push_back(new PalColour(0x5E, 0x4B, 100)); template10.push_back(new PalColour(0x61, 0x41, 3, 50, 20, 10.0)); template10.push_back(new PalColour(0x64, 0x54, 4, 50, 20)); template10.push_back(new PalColour(0x68, 0x58, 4, 50, 20)); template10.push_back(new PalColour(0x6C, 0x5C, 40)); template10.push_back(new PalColour(0x6D, 0x5D, 40)); template10.push_back(new PalColour(0x71, 0x41, 3, 70, 20, 10.0)); template10.push_back(new PalColour(0x74, 0x54, 4, 60, 5)); template10.push_back(new PalColour(0x78, 0x58, 4, 60, 5)); template10.push_back(new PalColour(0x7C, 0x68, 90)); template10.push_back(new PalColour(0x7D, 0x68, 110));
	wxVector<PalColour *> template11; template11.push_back(new PalColour(0x04, kObjNone, kRelNone, -1, 0.4f, 4, 80, 5, 10.0)); template11.push_back(new PalColour(0x24, kObjStone, kRelNone, -1, 0.8, 4, 100, 20, 20.0)); template11.push_back(new PalColour(0x28, kObjNatural, kRelNone, -1, 1, 4, 100, 40, 10.0)); template11.push_back(new PalColour(0x34, 0x28, 4, 100, 40, 10.0)); template11.push_back(new PalColour(0x38, kObjMetal, kRelNone, -1, 1, 4, 100, 5, 10.0)); template11.push_back(new PalColour(0x3C, 0x35, 100)); template11.push_back(new PalColour(0x3D, 0x39, 100)); template11.push_back(new PalColour(0x41, 0x29, 3, 100, 60, 10.0)); template11.push_back(new PalColour(0x44, kObjNatural, kRelNone, -1, 1, 4, 100, 20)); template11.push_back(new PalColour(0x48, kObjNone, kRelTriadic, 0x44, 0.7f, 3, 100, 40)); template11.push_back(new PalColour(0x4B, 0x4A, 10)); template11.push_back(new PalColour(0x4C, 0x45, 100)); template11.push_back(new PalColour(0x4D, kObjNone, kRelAnalogous, 0x48, 1, 0, 120, -1)); template11.push_back(new PalColour(0x54, kObjNone, kRelAnalogous, 0x24, 0.7f, 4, 80, 5, 20.0)); template11.push_back(new PalColour(0x51, 0x56, 3, 90, 50, 10.0)); template11.push_back(new PalColour(0x58, kObjNone, kRelTriadic, 0x48, 1, 4, 100, 5)); template11.push_back(new PalColour(0x5C, 0x55, 100)); template11.push_back(new PalColour(0x5D, 0x59, 100)); template11.push_back(new PalColour(0x5E, 0x4B, 100)); template11.push_back(new PalColour(0x61, 0x29, 3, 60, 20, 10.0)); template11.push_back(new PalColour(0x64, 0x54, 4, 60, 20)); template11.push_back(new PalColour(0x68, 0x58, 4, 60, 20)); template11.push_back(new PalColour(0x6C, 0x5C, 50)); template11.push_back(new PalColour(0x6D, 0x5D, 50)); template11.push_back(new PalColour(0x71, 0x29, 3, 60, 20, 10.0)); template11.push_back(new PalColour(0x74, kObjNone, kRelAnalogous, 0x54, 1, 4, 40, 5, 20.0)); template11.push_back(new PalColour(0x78, 0x58, 4, 40, 5)); template11.push_back(new PalColour(0x7C, 0x68, 90)); template11.push_back(new PalColour(0x7D, 0x68, 110));
	wxVector<PalColour *> template12; template12.push_back(new PalColour(0x04, kObjNone, kRelNone, -1, 0.4f, 4, 80, 5, 10.0)); template12.push_back(new PalColour(0x24, kObjStone, kRelNone, -1, 0.6f, 8, 100, 30, 5.0)); template12.push_back(new PalColour(0x2C, 0x04, 100)); template12.push_back(new PalColour(0x2D, 0x05, 100)); template12.push_back(new PalColour(0x34, kObjNatural, kRelNone, -1, 0.7f, 4, 60, 5, 5.0)); template12.push_back(new PalColour(0x38, kObjNone, kRelAnalogous, 0x34, 4, 60, 20, 5.0)); template12.push_back(new PalColour(0x3C, 0x38, 130)); template12.push_back(new PalColour(0x3E, 0x3C, 150)); template12.push_back(new PalColour(0x41, kObjNone, kRelNone, -1, 0.8f, 3, 70, 30, 5.0)); template12.push_back(new PalColour(0x44, 0x28, 4, 100, 30)); template12.push_back(new PalColour(0x48, kObjNone, kRelTriadic, 0x41, 1, 4, 140, 20, 15.0)); template12.push_back(new PalColour(0x4D, 0x41, 150)); template12.push_back(new PalColour(0x51, kObjNone, kRelNone, -1, 1, 3, 100, 20)); template12.push_back(new PalColour(0x54, kObjNone, kRelTriadic, 0x48, 1, 4, 100, 5, 5.0)); template12.push_back(new PalColour(0x5B, kObjNone, kRelCompliment, 0x54, 1, -4, 100, 10, 10.0)); template12.push_back(new PalColour(0x5D, 0x58, 50)); template12.push_back(new PalColour(0x68, 0x58, 4, 100, 5)); template12.push_back(new PalColour(0x6C, 0x69, 100)); template12.push_back(new PalColour(0x74, 0x54, 4, 100, 20)); template12.push_back(new PalColour(0x78, kObjGlass, kRelNone, -1, 1, 6, 100, 20, 10.0)); template12.push_back(new PalColour(0x7E, 0x77, 60));
	wxVector<PalColour *> template13; template13.push_back(new PalColour(0x04, kObjNone, kRelNone, -1, 0.4f, 4, 80, 5, 10.0)); template13.push_back(new PalColour(0x24, kObjStone, kRelNone, -1, 0.6f, 8, 100, 30, 5.0)); template13.push_back(new PalColour(0x2C, 0x04, 100)); template13.push_back(new PalColour(0x2D, 0x05, 100)); template13.push_back(new PalColour(0x37, kObjNatural, kRelNone, -1, 0.7f, -4, 60, 5, 5.0)); template13.push_back(new PalColour(0x38, kObjNone, kRelAnalogous, 0x34, 4, 60, 20, 5.0)); template13.push_back(new PalColour(0x3C, 0x38, 130)); template13.push_back(new PalColour(0x3E, 0x3C, 150)); template13.push_back(new PalColour(0x41, kObjNone, kRelNone, -1, 0.8f, 3, 70, 30, 5.0)); template13.push_back(new PalColour(0x44, 0x28, 4, 100, 30)); template13.push_back(new PalColour(0x48, kObjNone, kRelTriadic, 0x41, 1, 4, 140, 20, 15.0)); template13.push_back(new PalColour(0x4D, 0x41, 150)); template13.push_back(new PalColour(0x51, kObjNone, kRelNone, -1, 1, 3, 100, 20)); template13.push_back(new PalColour(0x54, kObjNone, kRelTriadic, 0x48, 1, 4, 100, 5, 5.0)); template13.push_back(new PalColour(0x5B, kObjNone, kRelCompliment, 0x54, 1, -4, 100, 10, 10.0)); template13.push_back(new PalColour(0x5D, 0x58, 50)); template13.push_back(new PalColour(0x68, 0x58, 4, 100, 5)); template13.push_back(new PalColour(0x6C, 0x69, 100)); template13.push_back(new PalColour(0x74, kObjNone, kRelNone, -1, 1, 4, 100, 20)); template13.push_back(new PalColour(0x78, kObjGlass, kRelNone, -1, 1, 6, 100, 20, 10.0)); template13.push_back(new PalColour(0x7E, 0x77, 60));
	wxVector<PalColour *> template14; template14.push_back(new PalColour(0x04, kObjNone, kRelNone, -1, 0.4f, 4, 80, 5, 10.0)); template14.push_back(new PalColour(0x24, true, kObjNone, kRelNone, -1, 1, 4, 100, 30, 10.0)); template14.push_back(new PalColour(0x28, true, kObjNone, kRelNone, -1, 1, 4, 50, 1)); template14.push_back(new PalColour(0x34, true, kObjNone, kRelNone, -1, 1, 4, 100, 40, 5.0)); template14.push_back(new PalColour(0x38, true, kObjMetal, kRelNone, -1, 1, 4, 70, 10)); template14.push_back(new PalColour(0x3C, wxBLACK)); template14.push_back(new PalColour(0x41, kObjNone, kRelNone, -1, 1, 4, 100, 10, 10.0)); template14.push_back(new PalColour(0x45, 0x41, 4, 90, 20, 10.0)); template14.push_back(new PalColour(0x49, kObjNone, kRelAnalogous, 0x45, 1, 4, 100, 20, 20.0)); template14.push_back(new PalColour(0x53, wxBLACK)); template14.push_back(new PalColour(0x54, kObjMetal, kRelNone, -1, 0.5f, 4, 80, 5)); template14.push_back(new PalColour(0x58, kObjNatural, kRelNone, -1, 1, 2, 100, 70)); template14.push_back(new PalColour(0x5A, kObjNatural, kRelNone, -1, 1, 2, 100, 70)); template14.push_back(new PalColour(0x5C, 0x57, 50)); template14.push_back(new PalColour(0x5D, 0x58, 120)); template14.push_back(new PalColour(0x61, 0x41, 4, 120, 30, 10.0)); template14.push_back(new PalColour(0x65, 0x45, 4, 110, 30, 10.0)); template14.push_back(new PalColour(0x73, wxBLACK)); template14.push_back(new PalColour(0x74, 0x54, 4, 100, 25)); template14.push_back(new PalColour(0x78, 0x58, 2, 100, 70)); template14.push_back(new PalColour(0x7A, 0x5A, 2, 100, 70)); template14.push_back(new PalColour(0x7C, 0x77, 50)); template14.push_back(new PalColour(0x7D, 0x78, 120));
	wxVector<PalColour *> template15; template15.push_back(new PalColour(0x04, kObjNone, kRelNone, -1, 0.4f, 4, 80, 5, 10.0)); template15.push_back(new PalColour(0x24, true, kObjNone, kRelNone, -1, 1, 4, 100, 30, 10.0)); template15.push_back(new PalColour(0x28, true, kObjNone, kRelNone, -1, 1, 4, 50, 1)); template15.push_back(new PalColour(0x34, true, kObjNone, kRelNone, -1, 1, 4, 100, 40, 5.0)); template15.push_back(new PalColour(0x38, true, kObjMetal, kRelNone, -1, 1, 4, 70, 10)); template15.push_back(new PalColour(0x3C, wxBLACK)); template15.push_back(new PalColour(0x41, kObjNone, kRelNone, -1, 1, 4, 100, 10, 10.0)); template15.push_back(new PalColour(0x45, 0x41, 4, 90, 20, 10.0)); template15.push_back(new PalColour(0x49, kObjNone, kRelAnalogous, 0x45, 1, 4, 100, 20, 20.0)); template15.push_back(new PalColour(0x53, wxBLACK)); template15.push_back(new PalColour(0x54, kObjMetal, kRelNone, -1, 0.5f, 4, 80, 5)); template15.push_back(new PalColour(0x58, kObjNatural, kRelNone, -1, 1, 2, 100, 70)); template15.push_back(new PalColour(0x5A, kObjNatural, kRelNone, -1, 1, 2, 100, 70)); template15.push_back(new PalColour(0x5C, 0x57, 50)); template15.push_back(new PalColour(0x5D, 0x58, 120)); template15.push_back(new PalColour(0x61, 0x41, 3, 100, 30, 10.0)); template15.push_back(new PalColour(0x64, kObjGlass, kRelNone, -1, 1, 8, 100, 1, 10.0)); template15.push_back(new PalColour(0x6C, 0x63, 50)); template15.push_back(new PalColour(0x6D, 0x61, 120)); template15.push_back(new PalColour(0x73, wxBLACK)); template15.push_back(new PalColour(0x74, 0x54, 4, 100, 25)); template15.push_back(new PalColour(0x78, 0x58, 2, 100, 70)); template15.push_back(new PalColour(0x7A, 0x5A, 2, 100, 70)); template15.push_back(new PalColour(0x7C, 0x77, 50)); template15.push_back(new PalColour(0x7D, 0x78, 120));
	wxVector<PalColour *> template16; template16.push_back(new PalColour(0x14, kObjMetal, kRelNone, -1, 0.4f, 4, 100, 5)); template16.push_back(new PalColour(0x21, kObjNone, kRelNone, -1, 1, 4, 100, 5)); template16.push_back(new PalColour(0x25, 0x21, 4, 90, 30)); template16.push_back(new PalColour(0x29, wxWHITE)); template16.push_back(new PalColour(0x41, 0x21, 4, 80, 5)); template16.push_back(new PalColour(0x45, 0x25, 4, 80, 20)); template16.push_back(new PalColour(0x51, true, kObjNone, kRelCompliment, 0x21, 1, 4, 60, 5)); template16.push_back(new PalColour(0x55, true, kObjNone, kRelNone, 0x51, 1, 4, 90, 30)); template16.push_back(new PalColour(0x61, kObjNone, kRelNone, -1, 1, 4, 100, 5)); template16.push_back(new PalColour(0x65, kObjNatural, kRelNone, -1, 1, 4, 100, 20)); template16.push_back(new PalColour(0x69, kObjStone, kRelNone, -1, 1, 3, 100, 40)); template16.push_back(new PalColour(0x6C, kObjNone, kRelCompliment, 0x61, 1, 3, 100, 40)); template16.push_back(new PalColour(0x71, 0x21, 100)); template16.push_back(new PalColour(0x72, kObjNone, kRelNone, -1, 1, 0, 100, -1)); template16.push_back(new PalColour(0x73, 0x26, 100)); template16.push_back(new PalColour(0x74, kObjNone, kRelAnalogous, 0x72, 1, 0, 100, -1)); template16.push_back(new PalColour(0x75, wxWHITE)); template16.push_back(new PalColour(0x76, 0x45, 100)); template16.push_back(new PalColour(0x77, 0x46, 100)); template16.push_back(new PalColour(0x78, 0x25, 100));
	wxVector<PalColour *> template18; template18.push_back(new PalColour(0x04, kObjNone, kRelNone, -1, 0.4f, 4, 80, 5, 10.0)); template18.push_back(new PalColour(0x24, kObjNone, kRelNone, -1, 1, 4, 100, 40, 10.0)); template18.push_back(new PalColour(0x28, 0x24, 4, 80, 5)); template18.push_back(new PalColour(0x2C, 0x2A, 70)); template18.push_back(new PalColour(0x2D, 0x29, 120)); template18.push_back(new PalColour(0x34, kObjNone, kRelCompliment, 0x24, 1, 7, 70, 30)); template18.push_back(new PalColour(0x41, kObjMetal, kRelNone, -1, 1, 7, 100, 20)); template18.push_back(new PalColour(0x48, kObjGlass, kRelNone, -1, 1, 4, 100, 30, 10.0)); template18.push_back(new PalColour(0x51, kObjNatural, kRelNone, -1, 1, 3, 100, 30, 15.0)); template18.push_back(new PalColour(0x54, kObjMetal, kRelNone, -1, 1, 4, 100, 5)); template18.push_back(new PalColour(0x58, 0x54, 4, 120, 40)); template18.push_back(new PalColour(0x61, kObjNatural, kRelNone, -1, 1, 8, 100, 20, 10.0)); template18.push_back(new PalColour(0x69, kObjNatural, kRelNone, -1, 1, 3, 100, 30)); template18.push_back(new PalColour(0x71, 0x61, 8, 100, 20, 10.0)); template18.push_back(new PalColour(0x7C, kObjNatural, kRelNone, -1, 1, 4, 100, 5));
	wxVector<PalColour *> template23; template23.push_back(new PalColour(0x04, kObjNone, kRelNone, -1, 0.4f, 4, 80, 5, 10.0)); template23.push_back(new PalColour(0x24, kObjNone, kRelNone, 0x24, 1, 4, 100, 20, 0, 6)); template23.push_back(new PalColour(0x28, kObjNone, kRelNone, 0x28, 1, 4, 100, 5, 0, 6)); template23.push_back(new PalColour(0x2C, kObjNone, kRelNone, 0x2C, 0, 100, -1, 0, 6)); template23.push_back(new PalColour(0x34, 0x24, 4, 100, 5)); template23.push_back(new PalColour(0x44, kObjNatural, kRelNone, -1, 1, 4, 100, 5, 10.0)); template23.push_back(new PalColour(0x48, wxWHITE)); template23.push_back(new PalColour(0x49, 0x44, 3, 80, 5)); template23.push_back(new PalColour(0x4C, 0x44, 130)); template23.push_back(new PalColour(0x51, kObjNatural, kRelNone, -1, 1, 3, 100, 30, 15.0)); template23.push_back(new PalColour(0x54, kObjNatural, kRelNone, -1, 1, 4, 100, 20, 10.0)); template23.push_back(new PalColour(0x58, 0x54, 4, 60, 5)); template23.push_back(new PalColour(0x71, kObjNatural, kRelNone, -1, 1, 3, 100, 30, 15.0)); template23.push_back(new PalColour(0x74, kObjNatural, kRelNone, -1, 0.7f, 9, 100, 1, 10.0)); template23.push_back(new PalColour(0x7D, wxLIGHT_GREY));
	wxVector<PalColour *> template24; template24.push_back(new PalColour(0x04, kObjNone, kRelNone, -1, 0.4f, 4, 80, 5, 10.0)); template24.push_back(new PalColour(0x61, wxWHITE)); template24.push_back(new PalColour(0x62, kObjNone, kRelNone, -1, 1, 5, 100, 5, 10.0)); template24.push_back(new PalColour(0x67, kObjNone, kRelNone, -1, 0.7, 5, 100, 10, 5.0)); template24.push_back(new PalColour(0x6C, kObjNatural, kRelNone, -1, 1, 3, 100, 50, 10.0)); template24.push_back(new PalColour(0x71, wxWHITE)); template24.push_back(new PalColour(0x72, kObjNone, kRelNone, -1, 1, 5, 100, 10)); template24.push_back(new PalColour(0x77, 0x67, 5, 120, 20, 5.0)); template24.push_back(new PalColour(0x7C, 0x6C, 3, 100, 50));
	wxVector<PalColour *> template25; template25.push_back(new PalColour(0x04, kObjNone, kRelNone, -1, 0.4f, 4, 80, 5, 10.0)); template25.push_back(new PalColour(0x41, kObjMetal, kRelNone, -1, 1, 4, 100, 20)); template25.push_back(new PalColour(0x44, 0x41, 4, 120, 40)); template25.push_back(new PalColour(0x51, kObjNatural, kRelNone, -1, 1, 4, 100, 5, 10.0)); template25.push_back(new PalColour(0x55, 0x51, 4, 90, 40)); template25.push_back(new PalColour(0x59, kObjNatural, kRelNone, -1, 1, 4, 100, 20)); template25.push_back(new PalColour(0x5D, kObjNatural, kRelCompliment, 0x59, 1, 2, 100, 60)); template25.push_back(new PalColour(0x61, kObjNone, kRelNone, -1, 1, 4, 100, 20, 10.0)); template25.push_back(new PalColour(0x65, 0x61, 4, 90, 40)); template25.push_back(new PalColour(0x69, kObjNatural, kRelNone, -1, 1, 4, 100, 5, 15.0)); template25.push_back(new PalColour(0x6D, 0x6A, 90)); template25.push_back(new PalColour(0x6E, 0x6A, 70)); template25.push_back(new PalColour(0x71, kObjNone, kRelNone, -1, 1, 4, 100, 5, 10.0)); template25.push_back(new PalColour(0x75, 0x71, 6, 90, 5)); template25.push_back(new PalColour(0x7F, kObjNatural, kRelNone, -1, 1, 0, 100, -1));

	// Vanilla specific ones
	wxVector<PalColour *> template1V; template1V.push_back(new PalColour(0x04, kObjNone, kRelNone, -1, 0.4f, 4, 80, 5, 10.0)); template1V.push_back(new PalColour(0x41, kObjNone, kRelNone, -1, 0.8f, 3, 80, 30, 5.0)); template1V.push_back(new PalColour(0x44, kObjNone, kRelTriadic, 0x41, 0.9f, 4, 100, 1, 20.0)); template1V.push_back(new PalColour(0x48, kObjNone, kRelAnalogous, 0x44, 1, 4, 100, 5, 5.0)); template1V.push_back(new PalColour(0x4C, 0x41, 120)); template1V.push_back(new PalColour(0x51, kObjMetal, kRelNone, -1, 0.4f, 3, 100, 30)); template1V.push_back(new PalColour(0x54, kObjNone, kRelNone, -1, 0.5f, 5, 50, 20)); template1V.push_back(new PalColour(0x59, 0x58, 100)); template1V.push_back(new PalColour(0x5A, 0x59, 90)); template1V.push_back(new PalColour(0x5B, 0x5A, 90)); template1V.push_back(new PalColour(0x5C, kObjMetal, kRelNone, -1, 1, 2, 30, 20, 10.0)); template1V.push_back(new PalColour(0x4D, 0x53, 100)); template1V.push_back(new PalColour(0x24, kObjNone, kRelTriadic, 0x44, 0.9f, 4, 40, 1, 20.0)); template1V.push_back(new PalColour(0x64, kObjWater, kRelNone, -1, 1, 4, 70, 20, 10.0)); template1V.push_back(new PalColour(0x68, kObjNone, kRelCompliment, 0x48, 1, 4, 100, 5)); template1V.push_back(new PalColour(0x6C, kObjWater, kRelNone, -1, 1, 2, 100, 60)); template1V.push_back(new PalColour(0x74, kObjMetal, kRelNone, -1, 1, 4, 100, 30, 10.0)); template1V.push_back(new PalColour(0x78, 0x48, 4, 100, 5, 5.0));
	wxVector<PalColour *> template2V; template2V.push_back(new PalColour(0x04, kObjNone, kRelNone, -1, 0.4f, 4, 80, 5, 10.0)); template2V.push_back(new PalColour(0x41, kObjNone, kRelNone, -1, 0.8f, 3, 80, 30, 5.0)); template2V.push_back(new PalColour(0x44, kObjNone, kRelTriadic, 0x41, 0.9f, 4, 100, 1, 20.0)); template2V.push_back(new PalColour(0x48, kObjNone, kRelAnalogous, 0x44, 1, 4, 100, 5, 5.0)); template2V.push_back(new PalColour(0x4C, 0x41, 120)); template2V.push_back(new PalColour(0x51, kObjMetal, kRelNone, -1, 0.4f, 3, 100, 30)); template2V.push_back(new PalColour(0x54, kObjNone, kRelNone, -1, 0.5f, 5, 50, 20)); template2V.push_back(new PalColour(0x59, 0x58, 100)); template2V.push_back(new PalColour(0x5A, 0x59, 90)); template2V.push_back(new PalColour(0x5B, 0x5A, 90)); template2V.push_back(new PalColour(0x5C, kObjMetal, kRelNone, -1, 1, 2, 30, 20, 10.0)); template2V.push_back(new PalColour(0x4D, 0x53, 100)); template2V.push_back(new PalColour(0x24, kObjNone, kRelTriadic, 0x44, 0.9f, 4, 40, 1, 20.0)); template2V.push_back(new PalColour(0x64, kObjWater, kRelNone, -1, 1, 4, 70, 20, 10.0)); template2V.push_back(new PalColour(0x68, kObjNone, kRelCompliment, 0x48, 1, 4, 100, 5)); template2V.push_back(new PalColour(0x6C, kObjWater, kRelNone, -1, 1, 2, 100, 60)); template2V.push_back(new PalColour(0x74, kObjMetal, kRelNone, -1, 1, 4, 100, 30, 10.0)); template2V.push_back(new PalColour(0x78, 0x48, 4, 100, 5, 5.0));
	wxVector<PalColour *> template3V; template3V.push_back(new PalColour(0x04, kObjNone, kRelNone, -1, 0.4f, 4, 80, 5, 10.0)); template3V.push_back(new PalColour(0x41, kObjNone, kRelNone, -1, 1, 3, 80, 30)); template3V.push_back(new PalColour(0x44, kObjNone, kRelCompliment, 0x41, 0.5f, 4, 80, 1, 15.0)); template3V.push_back(new PalColour(0x48, kObjNatural, kRelNone, -1, 1, 2, 100, 70)); template3V.push_back(new PalColour(0x4B, kObjNone, kRelAnalogous, 0x48, 1, -2, 80, 30)); template3V.push_back(new PalColour(0x4C, 0x4A, 50)); template3V.push_back(new PalColour(0x4D, 0x4C, 50)); template3V.push_back(new PalColour(0x4E, 0x41, 110)); template3V.push_back(new PalColour(0x51, kObjMetal, kRelNone, -1, 0.5f, 3, 80, 30)); template3V.push_back(new PalColour(0x54, 0x44, 4, 80, 5)); template3V.push_back(new PalColour(0x58, kObjNatural, kRelNone, -1, 1, 2, 100, 80)); template3V.push_back(new PalColour(0x5A, 0x4A, 2, 100, 40)); template3V.push_back(new PalColour(0x5C, 0x53, 30)); template3V.push_back(new PalColour(0x5D, 0x58, 120)); template3V.push_back(new PalColour(0x5E, 0x51, 120)); template3V.push_back(new PalColour(0x61, kObjNone, kRelNone, -1, 1, 3, 80, 30)); template3V.push_back(new PalColour(0x64, 0x54, 4, 100, 30)); template3V.push_back(new PalColour(0x68, kObjMetal, kRelNone, -1, 1, 4, 100, 5)); template3V.push_back(new PalColour(0x71, 0x51, 3, 80, 20)); template3V.push_back(new PalColour(0x74, 0x54, 3, 120, 80)); template3V.push_back(new PalColour(0x78, kObjMetal, kRelNone, -1, 1, 4, 100, 20));
	wxVector<PalColour *> template4V; template4V.push_back(new PalColour(0x04, kObjNone, kRelNone, -1, 0.4f, 4, 80, 5, 10.0)); template4V.push_back(new PalColour(0x41, kObjNone, kRelNone, -1, 0.7f, 3, 80, 30)); template4V.push_back(new PalColour(0x44, kObjNone, kRelCompliment, 0x41, 0.7f, 4, 80, 20, 10.0)); template4V.push_back(new PalColour(0x48, kObjNatural, kRelNone, -1, 1, 2, 100, 70)); template4V.push_back(new PalColour(0x4A, kObjNone, kRelAnalogous, 0x48, 1, 2, 70, 40)); template4V.push_back(new PalColour(0x4E, 0x41, 110)); template4V.push_back(new PalColour(0x51, 0x41, 3, 60, 5)); template4V.push_back(new PalColour(0x54, kObjMetal, kRelNone, -1, 1, 4, 40, 5, 10.0)); template4V.push_back(new PalColour(0x58, kObjNatural, kRelNone, -1, 1, 2, 100, 80)); template4V.push_back(new PalColour(0x5A, 0x4A, 2, 100, 20)); template4V.push_back(new PalColour(0x5C, 0x53, 30)); template4V.push_back(new PalColour(0x5D, 0x58, 120)); template4V.push_back(new PalColour(0x5E, 0x51, 120)); template4V.push_back(new PalColour(0x61, kObjMetal, kRelNone, -1, 0.7f, 3, 80, 30)); template4V.push_back(new PalColour(0x64, 0x54, 4, 100, 30)); template4V.push_back(new PalColour(0x68, kObjMetal, kRelNone, -1, 1, 4, 100, 5)); template4V.push_back(new PalColour(0x71, 0x51, 3, 80, 20)); template4V.push_back(new PalColour(0x74, 0x54, 3, 120, 80)); template4V.push_back(new PalColour(0x78, kObjMetal, kRelNone, -1, 1, 4, 100, 20)); template4V.push_back(new PalColour(0x7E, 0x71, 120));
	wxVector<PalColour *> template7V; template7V.push_back(new PalColour(0x04, kObjNone, kRelNone, -1, 0.4f, 4, 80, 5, 10.0)); template7V.push_back(new PalColour(0x28, kObjNatural, kRelNone, -1, 1, 4, 50, 5)); template7V.push_back(new PalColour(0x2C, new wxColour(227,194,212))); template7V.push_back(new PalColour(0x31, kObjNatural, kRelNone, -1, 1, 7, 100, 5, 10.0)); template7V.push_back(new PalColour(0x48, kObjNatural, kRelNone, -1, 0.8f, 3, 100, 60)); template7V.push_back(new PalColour(0x44, kObjNone, kRelNone, 0x49, 1, 4, 80, 10, 10.0)); template7V.push_back(new PalColour(0x43, 0x46, 80)); template7V.push_back(new PalColour(0x4C, 0x48, 100)); template7V.push_back(new PalColour(0x4E, 0x4C, 110)); template7V.push_back(new PalColour(0x51, kObjNatural, kRelNone, -1, 1, 3, 100, 40, 15.0)); template7V.push_back(new PalColour(0x54, kObjNone, kRelNone, 0x4C, 0.7f, 4, 100, 60, 10.0)); template7V.push_back(new PalColour(0x58, kObjNone, kRelAnalogous, 0x54, 1, 4, 40, 1)); template7V.push_back(new PalColour(0x24, 0x54, 4, 100, 20)); template7V.push_back(new PalColour(0x61, kObjNatural, kRelNone, -1, 0.6f, 7, 90, 1, 7.5)); template7V.push_back(new PalColour(0x68, kObjNone, kRelCompliment, 0x61, 1, 4, 100, 5)); template7V.push_back(new PalColour(0x71, kObjNone, kRelCompliment, 0x61, 1, 3, 100, 5)); template7V.push_back(new PalColour(0x74, kObjNone, kRelNone, -1, 0.7f, 4, 60, 5));	
	wxVector<PalColour *> template12V; template12V.push_back(new PalColour(0x04, kObjNone, kRelNone, -1, 0.4f, 4, 80, 5, 10.0)); template12V.push_back(new PalColour(0x24, kObjStone, kRelNone, -1, 0.6f, 4, 100, 30, 5.0)); template12V.push_back(new PalColour(0x24, kObjNone, kRelNone, -1, 0.6f, 4, 60, 5, 5.0)); template12V.push_back(new PalColour(0x2C, 0x04, 100)); template12V.push_back(new PalColour(0x2D, 0x05, 100)); template12V.push_back(new PalColour(0x34, kObjNatural, kRelNone, -1, 0.7f, 4, 60, 5, 5.0)); template12V.push_back(new PalColour(0x38, kObjNone, kRelAnalogous, 0x34, 1, 4, 100, 20, 5.0)); template12V.push_back(new PalColour(0x3C, 0x38, 130)); template12V.push_back(new PalColour(0x3E, 0x3C, 150)); template12V.push_back(new PalColour(0x41, kObjNone, kRelNone, -1, 0.8f, 3, 70, 30, 5.0)); template12V.push_back(new PalColour(0x44, 0x28, 4, 100, 30)); template12V.push_back(new PalColour(0x48, kObjNone, kRelTriadic, 0x41, 1, 4, 140, 20, 15.0)); template12V.push_back(new PalColour(0x4D, 0x41, 150)); template12V.push_back(new PalColour(0x51, kObjNone, kRelNone, -1, 1, 3, 100, 20)); template12V.push_back(new PalColour(0x54, kObjNone, kRelTriadic, 0x48, 1, 4, 100, 5, 5.0)); template12V.push_back(new PalColour(0x5B, kObjNone, kRelCompliment, 0x54, 1, -4, 100, 10, 10.0)); template12V.push_back(new PalColour(0x5D, 0x58, 50)); template12V.push_back(new PalColour(0x68, 0x58, 4, 100, 5)); template12V.push_back(new PalColour(0x6C, 0x69, 100)); template12V.push_back(new PalColour(0x74, 0x54, 4, 100, 20)); template12V.push_back(new PalColour(0x78, kObjGlass, kRelNone, -1, 1, 6, 100, 20, 10.0)); template12V.push_back(new PalColour(0x7E, 0x77, 60));
	wxVector<PalColour *> template13V; template13V.push_back(new PalColour(0x04, kObjNone, kRelNone, -1, 0.4f, 4, 80, 5, 10.0)); template13V.push_back(new PalColour(0x24, kObjStone, kRelNone, -1, 0.6f, 8, 100, 30, 5.0)); template13V.push_back(new PalColour(0x2C, 0x04, 100)); template13V.push_back(new PalColour(0x2D, 0x05, 100)); template13V.push_back(new PalColour(0x3B, kObjNatural, kRelNone, -1, 0.7f, -8, 100, 5, 5.0)); template13V.push_back(new PalColour(0x3C, 0x38, 130)); template13V.push_back(new PalColour(0x3E, 0x3C, 150));template13V.push_back(new PalColour(0x41, kObjNone, kRelNone, -1, 0.8f, 3, 70, 30, 5.0)); template13V.push_back(new PalColour(0x44, 0x28, 4, 100, 30)); template13V.push_back(new PalColour(0x48, kObjNone, kRelTriadic, 0x41, 1, 4, 140, 20, 15.0)); template13V.push_back(new PalColour(0x4D, 0x41, 150)); template13V.push_back(new PalColour(0x51, kObjNone, kRelNone, -1, 1, 3, 100, 20)); template13V.push_back(new PalColour(0x54, kObjNone, kRelTriadic, 0x48, 1, 4, 100, 5, 5.0)); template13V.push_back(new PalColour(0x5B, kObjNone, kRelCompliment, 0x54, 1, -4, 100, 10, 10.0)); template13V.push_back(new PalColour(0x5D, 0x58, 50)); template13V.push_back(new PalColour(0x6B, 0x58, -8, 100, 5)); template13V.push_back(new PalColour(0x6C, 0x69, 100)); template13V.push_back(new PalColour(0x74, kObjNone, kRelNone, -1, 1, 4, 100, 20)); template13V.push_back(new PalColour(0x78, kObjGlass, kRelNone, -1, 1, 6, 100, 20, 10.0)); template13V.push_back(new PalColour(0x7E, 0x77, 60));

	wxVector<PalColour *> templatesV[29] = {template1V, template2V, template3V, template4V, template5, template6, template7V, template8, template8, template10, template11, template12V, template13V, template14, template15, template16, template16, template1, template1, template1, template1, template18, template18, template18, template18, template18, template23, template24, template25};
	wxVector<PalColour *> templates[29] = {template1, template2, template3, template4, template5, template6, template7, template8, template8, template10, template11, template12, template13, template14, template15, template16, template16, template1, template1, template1, template1, template18, template18, template18, template18, template18, template23, template24, template25};

	// If the rom is vanilla, we want to use slightly different templates for the tilesets
	if (_romType->GetValue() == true) {
		for (int i = 0; i < 29; i++) {
			templates[i] = templatesV[i];
		}
	}

	// Now the actual palette creation algorithm
	for (int t = 0; t < 29; t++) {
		if (tilesets[t] != nullptr) {
			// Start by blacking out the entire tileset
			for (int i = 0; i < 128; i++) {
				// Except for the 0x0E colour for each line after the first two, which is usually white
				if (((i % 16) == 0x0E) && (i > 0x20)) {
					tilesets[t][i] = *wxWHITE;
		
				} else {
					tilesets[t][i] = wxColour(0,0,0);
				}
			}
			processTilesetTemplate(tilesets, templates, t);
		}
	}

	// Now we input the colours that stay the same in every tileset
	writeLockedPaletteColours(tilesets);

	// And we fix the heat palettes to reflect the new norfair colours
	fixHeatPalettes(tilesets);

	// Also the glows
	fixGlowPalettes(tilesets);

	// And temporarily fill out the example palette that gets shown in the program
	for (int i = 0; i < 128; i++) {
		_examplePalette[i] = tilesets[6][i];
	}

	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 16; j++) {
			_paletteGrid->SetCellValue(i, j, wxString::Format("%02X,%02X,%02X", _examplePalette[(i * 16) + j].GetRed(), _examplePalette[(i * 16) + j].GetGreen(), _examplePalette[(i * 16) + j].GetBlue()));
		}
	}

	// And finally we compress this new palette into the rom
	for (int i = 0; i < 29; i++) {
		if (tilesets[i] != nullptr) {
			if (_rom->compressPalette(tilesets[i], palettes[i]) == false) {
				std::cout << "uh oh, palette too big!!!" << std::endl;
			}
		}
	}

	// The initial haze colour is now decided by the palette colour
	if (_romType->GetValue() == true) {
		wxByte num = 0;

		// Get the main colour for ceres tiles
		wxColour clr = tilesets[14][0x25];

		// Then depending on the intensity of each colour, set the colour bit of the haze byte
		if (clr.Red() > 128) {
			num |= 0x80;
		}
		if (clr.Green() > 128) {
			num |= 0x40;
		}
		if (clr.Blue() > 128) {
			num |= 0x20;
		}
		_rom->setByte(0x45E11, num);
	}
}

void RandoFrame::fixGlowPalettes(wxColour *tilesets[29]) {
	// Wrecked ship green glow
	_rom->storeColour(0x6EAE8, tilesets[4][0x4C]);
	_rom->storeColour(0x6EAEA, tilesets[4][0x4D]);
	_rom->storeColour(0x6EAF0, tilesets[4][0x4C].ChangeLightness(90));
	_rom->storeColour(0x6EAF2, tilesets[4][0x4D].ChangeLightness(90));
	_rom->storeColour(0x6EAF8, tilesets[4][0x4C].ChangeLightness(80));
	_rom->storeColour(0x6EAFA, tilesets[4][0x4D].ChangeLightness(80));
	_rom->storeColour(0x6EB00, tilesets[4][0x4C].ChangeLightness(70));
	_rom->storeColour(0x6EB02, tilesets[4][0x4D].ChangeLightness(70));
	_rom->storeColour(0x6EB08, tilesets[4][0x4C].ChangeLightness(60));
	_rom->storeColour(0x6EB0A, tilesets[4][0x4D].ChangeLightness(60));
	_rom->storeColour(0x6EB10, tilesets[4][0x4C].ChangeLightness(70));
	_rom->storeColour(0x6EB12, tilesets[4][0x4D].ChangeLightness(70));
	_rom->storeColour(0x6EB18, tilesets[4][0x4C].ChangeLightness(80));
	_rom->storeColour(0x6EB1A, tilesets[4][0x4D].ChangeLightness(80));
	_rom->storeColour(0x6EB20, tilesets[4][0x4C].ChangeLightness(90));
	_rom->storeColour(0x6EB22, tilesets[4][0x4D].ChangeLightness(90));

	// Red brinstar purple glow
	uint64_t brinstarGlow = 0x6EEDD;
	uint64_t gBrinstarGlow = 0x6ED9F;
	wxColour glow[8];
	wxColour glow2[3];

	int j = 0;
	for (int i = 0; i < 16; i++) {
		j = i;
		if (i > 6) { 
			j = 13 - (i - 7);
		}

		if ((i == 0) || (i == 7)) {
			for (int g = 0; g < 8; g++) {
				glow[g] = tilesets[7][0x64 + g];
			}

			for (int g = 0; g < 3; g++) {
				glow2[g] = tilesets[6][0x71 + g];
			}
		}

		for (int r = 0; r < 8; r++) {
			_rom->storeColour(brinstarGlow + ((j) * 0x14) + (r * 2), glow[r]);
		}
		
		for (int r = 0; r < 3; r++) {
			_rom->storeColour(gBrinstarGlow + ((j) * 0x0A) + (r * 2), glow2[r]);
		}

		for (int g = 0; g < 8; g++) {
			glow[g] = glow[g].ChangeLightness(85);
		}
		
		for (int g = 0; g < 3; g++) {
			glow2[g] = glow2[g].ChangeLightness(90);
		}
	}
}

void RandoFrame::fixHeatPalettes(wxColour *tilesets[29]) {
	uint64_t mainHeat = 0x6F1D7;
	uint64_t bubbleHeat = 0x6F1DF;
	uint64_t pipesHeat = 0x6F097;
	uint64_t blocksHeat = 0x6F09F;
	uint64_t otherHeat = 0x6F2DF;
	uint64_t stoneHeat = 0x6F2E7;
	uint64_t darkHeat = 0x6F3E7;
	uint64_t darkStoneHeat = 0x6F3EF;

	wxColour baseClr1, baseClr2, baseClr3;
	wxColour baseBubble;
	wxColour baseTile;
	wxColour baseStone;
	wxColour baseRock;
	wxColour baseClr4, baseClr5, baseClr6;
	wxColour baseStoneDark;
	wxColour baseRockDark;

	int j = 0;
	for (int i = 0; i < 16; i++) {
		j = i;
		if (i > 7) { 
			j = 15 - (i - 8);
		}

		if ((i == 0) || (i == 8)) {
			baseClr1 = tilesets[9][0x41];
			baseClr2 = tilesets[9][0x42];
			baseClr3 = tilesets[9][0x43];
			baseBubble = tilesets[9][0x4C];
			baseTile = tilesets[9][0x4D];
			baseStone = tilesets[9][0x5C];
			baseRock = tilesets[9][0x5D];
			baseClr4 = tilesets[9][0x61];
			baseClr5 = tilesets[9][0x62];
			baseClr6 = tilesets[9][0x63];
			baseStoneDark = tilesets[9][0x6C];
			baseRockDark = tilesets[9][0x6D];
		}

		_rom->storeColour(mainHeat + ((j) * 0x10), baseClr1);
		_rom->storeColour(mainHeat + ((j) * 0x10) + 2, baseClr2);
		_rom->storeColour(mainHeat + ((j) * 0x10) + 4, baseClr3);
		_rom->storeColour(bubbleHeat + ((j) * 0x10), baseBubble);
		_rom->storeColour(bubbleHeat + ((j) * 0x10) + 2, baseTile);

		_rom->storeColour(pipesHeat + ((j) * 0x13), baseClr1);
		_rom->storeColour(pipesHeat + ((j) * 0x13) + 2, baseClr2);
		_rom->storeColour(pipesHeat + ((j) * 0x13) + 4, baseClr3);
		_rom->storeColour(blocksHeat + ((j) * 0x13), baseBubble);
		_rom->storeColour(blocksHeat + ((j) * 0x13) + 2, baseTile);

		_rom->storeColour(otherHeat + ((j) * 0x10), baseClr1);
		_rom->storeColour(otherHeat + ((j) * 0x10) + 2, baseClr2);
		_rom->storeColour(otherHeat + ((j) * 0x10) + 4, baseClr3);
		_rom->storeColour(stoneHeat + ((j) * 0x10), baseStone);
		_rom->storeColour(stoneHeat + ((j) * 0x10) + 2, baseRock);

		_rom->storeColour(darkHeat + ((j) * 0x10), baseClr4);
		_rom->storeColour(darkHeat + ((j) * 0x10) + 2, baseClr5);
		_rom->storeColour(darkHeat + ((j) * 0x10) + 4, baseClr6);
		_rom->storeColour(darkStoneHeat + ((j) * 0x10), baseStoneDark);
		_rom->storeColour(darkStoneHeat + ((j) * 0x10) + 2, baseRockDark);

		baseClr1 = baseClr1.ChangeLightness(110);
		baseClr2 = baseClr2.ChangeLightness(110);
		baseClr3 = baseClr3.ChangeLightness(110);
		baseBubble = baseBubble.ChangeLightness(110);
		baseTile = baseTile.ChangeLightness(110);
		baseStone = baseStone.ChangeLightness(110);
		baseRock = baseRock.ChangeLightness(110);
		baseClr4 = baseClr4.ChangeLightness(110);
		baseClr5 = baseClr5.ChangeLightness(110);
		baseClr6 = baseClr6.ChangeLightness(110);
		baseStoneDark = baseStoneDark.ChangeLightness(110);
		baseRockDark = baseRockDark.ChangeLightness(110);
	}
}

void RandoFrame::processTilesetTemplate(wxColour *tilesets[29], wxVector<PalColour *> templates[29], int t) {
	// Now we need to define the material colours
	// Natural objects get the widest range of possible starting colours
	wxColour objNatural[16] = {wxColour(148,55,255), wxColour(255,38,0), wxColour(4,51,255), wxColour(255,251,0),
							   wxColour(255,147,0), wxColour(0,250,146), wxColour(142,250,0), wxColour(255,47,146),
							   wxColour(0,150,255), wxColour(122,129,255), wxColour(215,131,255), wxColour(255,138,216),
							   wxColour(255,252,121), wxColour(148,17,0), wxColour(255,126,121), wxColour(115,250,121)};

	// Metal objects get silvers, browns, reds
	wxColour objMetal[16] = {wxColour(189,192,197), wxColour(138,93,78), wxColour(172,173,180), wxColour(175,119,43),
							 wxColour(192,192,192), wxColour(121,121,121), wxColour(169,169,169), wxColour(235,235,235),
							 wxColour(140,140,140), wxColour(184,122,74), wxColour(117,106,96), wxColour(149,137,120),
							 wxColour(75,77,74), wxColour(111,98,80), wxColour(119,103,92), wxColour(112,73,51)};

	// Stone objects are greys and browns mostly
	wxColour objStone[24] = {wxColour(169,169,169), wxColour(94,94,94), wxColour(180,91,7), wxColour(148,82,0),
							 wxColour(83,27,147), wxColour(148,23,81), wxColour(255,147,0), wxColour(146,144,0),
							 wxColour(155,148,140), wxColour(86,59,36), wxColour(101,58,41), wxColour(212,209,204),
							 wxColour(181,150,128), wxColour(81,83,82), wxColour(80,65,67), wxColour(97,95,71),
							 wxColour(174,175,174), wxColour(159,162,161), wxColour(163,161,162), wxColour(147,151,155),
							 wxColour(110,87,84), wxColour(85,70,70), wxColour(145,145,138), wxColour(70,78,87)};

	// Glass/Ice objects get light colours like blue, white, yellow, etc.
	wxColour objGlass[6] = {wxColour(115,253,255), wxColour(115,252,214), wxColour(212,251,121), wxColour(255,255,255), wxColour(255,252,121), wxColour(255,126,121)};

	// Water get various kinds of blues
	wxColour objWater[8] = {wxColour(90,158,179), wxColour(99,161,188), wxColour(72,128,150), wxColour(160,232,222),
							wxColour(21,38,58),  wxColour(49,99,162), wxColour(56,115,167), wxColour(48,95,137)};

	wxVector<float> hues;

	// For use at various points
	unsigned char r;
	unsigned char g;
	unsigned char b;
	ok_color::RGB rgb;
	ok_color::HSV hsv;

	for (int i = 0; i < templates[t].size(); i++) {
		// Silhouette mode just colours in the background for a given tileset, not the foreground
		if ((_shuffleTilePalSilhouette->GetValue() == true) && templates[t][i]->background == false) {
			continue;
		}

		// If continuity is set to off, then we want to make sure to remove the reference from any colours that have continuity
		if ((templates[t][i]->tileset != -1) && (_shuffleTilePalContinuity->GetValue() == false)) {
			templates[t][i]->reference = -1;
			templates[t][i]->tileset = -1;
		}

		// First thing we need is a colour, which will be used and possibly changed
		wxColour clr;

		// If the object has a hard coded colour, we want to use that one
		if (templates[t][i]->hardCoded != nullptr) {
			clr = *templates[t][i]->hardCoded;

		} else if (templates[t][i]->relation != kRelNone) {
			// First we grab the reference colour
			clr = tilesets[t][templates[t][i]->reference];

			// Then we convert it into the ok_color space so we can use hsv
			rgb.r = clr.Red();
			rgb.g = clr.Green();
			rgb.b = clr.Blue();
			hsv = srgb_to_okhsv(rgb);

			// Then we apply the transformation
			switch (templates[t][i]->relation) {
				case kRelCompliment:
					// Complimentary colours are opposite from each other on the colour wheel
					hsv.h = std::fmod((hsv.h + (180.0 / 360.0)), 1.0);
					break;
				case kRelAnalogous:
					// Analogous colours are roughly 40 - 60 degrees apart
					hsv.h = std::fmod((hsv.h + (50.0 / 360.0)), 1.0);
					break;
				case kRelTriadic:
					// Triadic colours are 120 degrees apart (a triangle)
					hsv.h = std::fmod((hsv.h + (120.0 / 360.0)), 1.0);
					break;
				case kRelTetradic:
					// Tetradic colours are 90 degrees apart (a square)
					hsv.h = std::fmod((hsv.h + (90.0 / 360.0)), 1.0);
				default:
					break;
			}
			rgb = okhsv_to_srgb(hsv);
			r = rgb.r;
			g = rgb.g;
			b = rgb.b;
			clr = wxColour(r, g, b);

		// If the object is referencing another colour, we want to use that one
		} else if (templates[t][i]->reference != -1) {
			// If the colour references another tileset, use the number as the tileset to reference
			if (templates[t][i]->tileset != -1) {
				// And we want to change the brightness to whatever the object defines
				clr = tilesets[templates[t][i]->tileset][templates[t][i]->reference].ChangeLightness(templates[t][i]->brightnessS);
			
			} else {
				clr = tilesets[t][templates[t][i]->reference].ChangeLightness(templates[t][i]->brightnessS);	
			}

		// If the object has a material type, we want to pick from material colour pools
		} else if (templates[t][i]->object != kObjNone) {
			switch (templates[t][i]->object) {
				case kObjNatural:
					clr = objNatural[rand() % 16];
					break;
				case kObjMetal:
					clr = objMetal[rand() % 16];
					break;
				case kObjStone:
					clr = objStone[rand() % 24];
					break;
				case kObjGlass:
					clr = objGlass[rand() % 6];
					break;
				case kObjWater:
					clr = objWater[rand() % 8];
				default:
					break;
			}

			// We want any material type colour to be default 100 brightness
			clr.ChangeLightness(100 + (100 - clr.GetLuminance()));
			if ((templates[t][i]->brightnessS != 100) &&(templates[t][i]->gradient == 0)) {
				clr.ChangeLightness(templates[t][i]->brightnessS);
			}

		// otherwise it's just a completely random colour
		} else {
			// That being said, we want to make sure the colour is not really close to another colour we've already chosen
			bool closeToAnother = true;
			while (closeToAnother == true) {
				clr = wxColour(rand() % 255, rand() % 255, rand() % 255);
				// And we also want to make sure the colour is bright enough to start
				if (clr.GetLuminance() < 0.5f) {
					clr = clr.ChangeLightness(150);
				}
				rgb.r = clr.Red();
				rgb.g = clr.Green();
				rgb.b = clr.Blue();
				hsv = srgb_to_okhsv(rgb);

				closeToAnother = false;
				for (int i = 0; i < hues.size(); i++) {
					if ((hsv.h > (hues[i] - 0.1)) && (hsv.h < (hues[i] + 0.1))) {
						// Just in case we actually have a situation where we already have 0.1 - 1.0, let it use the colour
						if (i < (hues.size() - 1)) {
							closeToAnother = true;
						}
					}
				}
			}

			// Once we have a new colour, we add it to the hues vector
			hues.push_back(hsv.h);
		}

		// If the saturation level is less than 1, we convert to ok_color and change the saturation
		if (templates[t][i]->saturation != 1) {
			rgb.r = clr.Red();
			rgb.g = clr.Green();
			rgb.b = clr.Blue();
			hsv = srgb_to_okhsv(rgb);

			hsv.s = templates[t][i]->saturation;

			rgb = okhsv_to_srgb(hsv);
			r = rgb.r;
			g = rgb.g;
			b = rgb.b;
			clr = wxColour(r, g, b);
		}


		// If the object has a gradient, create one
		if (templates[t][i]->gradient != 0) {
			// Difference is the total difference in luminance between start and end
			int diff = templates[t][i]->brightnessS - templates[t][i]->brightnessE;

			// Step is the amount of change per step
			int step = diff / std::abs(templates[t][i]->gradient);
			int numSteps = std::abs(templates[t][i]->gradient);

			// Index is the starting position of the gradient
			int index = templates[t][i]->index;

			rgb.r = clr.Red();
			rgb.g = clr.Green();
			rgb.b = clr.Blue();
			hsv = srgb_to_okhsv(rgb);

			// We have the brightness step from the template, but we need the percentage of the colours actual brightness to use for stepping in the gradient
			float percent = hsv.v * ((float) step / 100.0);
			hsv.v = hsv.v * ((float) templates[t][i]->brightnessS / 100.0);

			while (numSteps > 0) {
				rgb = okhsv_to_srgb(hsv);
				r = rgb.r;
				g = rgb.g;
				b = rgb.b;
				clr = wxColour(r, g, b);

				// Now if we want greyscale, we just use makeGrey() on whatever colour we have
				if (_shuffleTilePalGreyscale->GetValue() == true) {
					r = clr.Red();
					g = clr.Green();
					b = clr.Blue();
					wxColour::MakeGrey(&r, &g, &b);
					tilesets[t][index] = wxColour(r, g, b);

				} else {
					tilesets[t][index] = clr;						
				}

				hsv.v -= percent;
				if (templates[t][i]->hueShift != 0) {
					hsv.h = std::fmod((hsv.h + (templates[t][i]->hueShift / 360.0)), 1.0);
				}

				// If the gradient is negative, it goes backwards
				if (templates[t][i]->gradient < 0) {
					index--;
				} else {
					index++;
				}

				numSteps--;
			}
		
		} else {
			if (_shuffleTilePalGreyscale->GetValue() == true) {
				r = clr.Red();
				g = clr.Green();
				b = clr.Blue();
				wxColour::MakeGrey(&r, &g, &b);
				tilesets[t][templates[t][i]->index] = wxColour(r, g, b);

			} else {
				tilesets[t][templates[t][i]->index] = clr;					
			}
		}
	}
}

void RandoFrame::writeLockedPaletteColours(wxColour *tilesets[29]) {
	// First we need to define the colours that don't get randomized, like the doors
	wxColour doorsP[3] = {wxColour(0xf8,0xc0,0x01), wxColour(0xd0,0x78,0x01), wxColour(0x80,0x20,0x01)};
	wxColour doorsS[3] = {wxColour(0x90,0xf8,0x20), wxColour(0x40,0xa8,0x20), wxColour(0x18,0x48,0x10)};
	wxColour doorsM[3] = {wxColour(0xe0,0xa8,0xe0), wxColour(0xd8,0x38,0x90), wxColour(0xb0,0x01,0x30)};
	wxColour doorsB[3] = {wxColour(0x90,0xa8,0xe0), wxColour(0x38,0x70,0xe0), wxColour(0x18,0x40,0x98)};

	// And the hud/door tubes
	wxColour hud[4] = {wxColour(0xd8,0xa8,0xb8), wxColour(0x98,0x68,0x78), wxColour(0x70,0x48,0x50), wxColour(0x30,0x20,0x28)};

	// The misc used in items and sprites
	wxColour misc[5] = {wxColour(0x88,0xe8,0x10), wxColour(0xd8,0x38,0x90), wxColour(0xf8,0xf8,0xf8), wxColour(0x1,0x1,0x1), wxColour(0xf8,0xf8,0xf8)};

	// And the ones that I forget the purpose of
	wxColour other[7] = {wxColour(0xc0,0xc0,0xc0), wxColour(0x38,0x38,0x38), wxColour(0x20,0x20,0x20), wxColour(0x1,0x1,0x1), wxColour(0xf8,0xf8,0xf8), wxColour(0xf8,0xb0,0x1), wxColour(0xf8,0x1,0x1)};

	// Fill out the predefined colours
	for (int i = 0; i < 29; i++) {
		if (tilesets[i] != nullptr) {
			for (int j = 0; j < 3; j++) {
				tilesets[i][0x1 + j] = doorsP[j];
			}

			for (int j = 0; j < 3; j++) {
				tilesets[i][0x11 + j] = doorsS[j];
			}

			for (int j = 0; j < 4; j++) {
				tilesets[i][0x14 + j] = hud[j];
			}

			for (int j = 0; j < 5; j++) {
				tilesets[i][0x8 + j] = misc[j];
			}

			for (int j = 0; j < 7; j++) {
				tilesets[i][0x18 + j] = other[j];
			}

			// In the ceres tilset, lines 0x2 and 0x3 are used for the tileset
			if ((i < 15) || (i > 20)) {
				for (int j = 0; j < 3; j++) {
					tilesets[i][0x21 + j] = doorsM[j];
				}

				for (int j = 0; j < 3; j++) {
					tilesets[i][0x31 + j] = doorsB[j];
				}
			}
			
			tilesets[i][0x0D] = _hudColourPicker2->GetColour();
			tilesets[i][0x0E] = _hudColourPicker->GetColour();
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

	_romType = new wxCheckBox(_mainPanel, wxID_ANY, "Vanilla Map", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, wxEmptyString);

	wxButton *buttonOpen = new wxButton(_mainPanel, wxID_ANY, "Load Rom", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, "Open");
	buttonOpen->SetBitmap(wxArtProvider::GetIcon(wxART_FOLDER, wxART_FRAME_ICON));
	buttonOpen->Bind(wxEVT_BUTTON, &RandoFrame::onOpen, this);

	topSizer->Add(diffText, 0, wxALIGN_CENTER_VERTICAL);
	topSizer->Add(difficulty, 0, wxGROW | wxLEFT, 5);
	topSizer->AddStretchSpacer();
	topSizer->Add(_romType, 0, wxGROW | wxLEFT, 5);
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
	pauseSizer->Add(_skipCeres, 0, wxGROW | wxLEFT, (kMacMargins * 10));
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

	wxStaticBoxSizer *tilesetBox = new wxStaticBoxSizer(wxVERTICAL, visualsPage, "Tileset Palette Settings");
	wxStaticBoxSizer *paletteBox = new wxStaticBoxSizer(wxVERTICAL, visualsPage, "Other Palette Settings");
	wxStaticBoxSizer *gfxBox = new wxStaticBoxSizer(wxVERTICAL, visualsPage, "Graphics Settings");

	wxBoxSizer *suitSizer = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer *hudColourSizer = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer *tilePalSizer = new wxBoxSizer(wxHORIZONTAL);

	// First the hud colour selector
	wxStaticText *hudColourText = new wxStaticText(tilesetBox->GetStaticBox(), wxID_ANY, "HUD Colour");
	_hudColourPicker = new wxColourPickerCtrl(tilesetBox->GetStaticBox(), wxID_ANY, wxColour(*wxWHITE), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, wxEmptyString);
	hudColourSizer->Add(hudColourText, 0, wxALIGN_CENTER_VERTICAL);
	hudColourSizer->Add(_hudColourPicker, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);

	wxStaticText *hudColourSecondText = new wxStaticText(tilesetBox->GetStaticBox(), wxID_ANY, "HUD Colour (Secondary)");
	_hudColourPicker2 = new wxColourPickerCtrl(tilesetBox->GetStaticBox(), wxID_ANY, wxColour(0x28,0x38,0x88), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, wxEmptyString);
	hudColourSizer->Add(hudColourSecondText, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
	hudColourSizer->Add(_hudColourPicker2, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);

	// Now the palette box
	_shuffleTilePal = new wxCheckBox(tilesetBox->GetStaticBox(), wxID_ANY, "Randomize Tileset Palettes", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, wxEmptyString);
	wxStaticText *shuffleTileText = new wxStaticText(tilesetBox->GetStaticBox(), wxID_ANY, "Options:");
	_shuffleTilePalGreyscale = new wxCheckBox(tilesetBox->GetStaticBox(), wxID_ANY, "Greyscale", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, wxEmptyString);
	_shuffleTilePalSilhouette = new wxCheckBox(tilesetBox->GetStaticBox(), wxID_ANY, "Silhouette", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, wxEmptyString);
	_shuffleTilePalContinuity = new wxCheckBox(tilesetBox->GetStaticBox(), wxID_ANY, "Continuity", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, wxEmptyString);
	_shuffleTilePalContinuity->SetValue(true);

	tilePalSizer->Add(_shuffleTilePalGreyscale);
	tilePalSizer->Add(_shuffleTilePalSilhouette, 0, wxLEFT, 5);
	tilePalSizer->Add(_shuffleTilePalContinuity, 0, wxLEFT, 5);

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

	tilesetBox->Add(_shuffleTilePal, 0, wxGROW | wxLEFT, 5);
	tilesetBox->Add(shuffleTileText, 0, wxGROW | wxLEFT | wxTOP, 5);
	tilesetBox->Add(tilePalSizer, 0, wxGROW | wxLEFT | wxTOP, 5);
	tilesetBox->Add(hudColourSizer, 0, wxGROW | wxLEFT | wxTOP, 5);

	//paletteBox->Add(_shuffleEnemyPal, 0, wxGROW | wxLEFT | wxTOP, 5);
	paletteBox->Add(_shuffleBeamPal, 0, wxGROW | wxLEFT, 5);
	paletteBox->Add(suitSizer, 0, wxGROW | wxLEFT | wxTOP, 5);

	// Now the graphics box
	//_shuffleBeamGfx = new wxCheckBox(gfxBox->GetStaticBox(), wxID_ANY, "Shuffle Beam Graphics", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, wxEmptyString);
	//_shuffleMorphGfx = new wxCheckBox(gfxBox->GetStaticBox(), wxID_ANY, "Shuffle Morph Graphics", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, wxEmptyString);
	_shuffleFontNGfx = new wxCheckBox(gfxBox->GetStaticBox(), wxID_ANY, "Shuffle Font Graphics (numbers)", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, wxEmptyString);
	_shuffleFontTGfx = new wxCheckBox(gfxBox->GetStaticBox(), wxID_ANY, "Shuffle Font Graphics (text)", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, wxEmptyString);
	_mysteryItemGfx = new wxCheckBox(gfxBox->GetStaticBox(), wxID_ANY, "Mystery Item Graphics", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, wxEmptyString);
	_shuffleFX1 = new wxCheckBox(gfxBox->GetStaticBox(), wxID_ANY, "Shuffle room FX (ie. fog)", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, wxEmptyString);

	//gfxBox->Add(_shuffleBeamGfx, 0, wxGROW | wxLEFT, 5);
	//gfxBox->Add(_shuffleMorphGfx, 0, wxGROW | wxLEFT | wxTOP, 5);
	gfxBox->Add(_shuffleFontNGfx, 0, wxGROW | wxLEFT, 5);
	gfxBox->Add(_shuffleFontTGfx, 0, wxGROW | wxLEFT | wxTOP, 5);
	gfxBox->Add(_mysteryItemGfx, 0, wxGROW | wxLEFT | wxTOP, 5);
	gfxBox->Add(_shuffleFX1, 0, wxGROW | wxLEFT | wxTOP, 5);

	visualsSizer->Add(tilesetBox, 0, wxGROW | wxLEFT | wxRIGHT, 5);
	visualsSizer->Add(paletteBox, 0, wxGROW | wxLEFT | wxRIGHT | wxTOP, 5);
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

	_floodMode = new wxCheckBox(logicOptionsBox->GetStaticBox(), wxID_ANY, "Flood Mode", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, wxEmptyString);

	logicOptionsBox->Add(_gravityHeat, 0, wxGROW | wxLEFT, 9);
	logicOptionsBox->Add(_majorMinor, 0, wxGROW | wxLEFT | wxTOP, 9);
	logicOptionsBox->Add(_partyRando, 0, wxGROW | wxLEFT | wxTOP, 9);
	logicOptionsBox->Add(_floodMode, 0, wxGROW | wxLEFT | wxTOP, 9);

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

	// And now temporarily, a palette page
	   wxPanel *palettePage = new wxPanel(optionsBook);
	   wxBoxSizer *paletteSizer = new wxBoxSizer(wxHORIZONTAL);

	_paletteGrid = new wxGrid(palettePage, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_DOUBLE, wxEmptyString);

	// We need to set up some properties of the grid
	_paletteGrid->SetDefaultCellAlignment(wxALIGN_CENTER, wxALIGN_CENTER);
	_paletteGrid->SetRowLabelSize(0);
	_paletteGrid->SetColLabelSize(0);
	_paletteGrid->DisableDragColMove();
	_paletteGrid->DisableDragRowMove();
	_paletteGrid->DisableDragColSize();
	_paletteGrid->DisableDragGridSize();
	_paletteGrid->DisableDragRowSize();
	_paletteGrid->EnableGridLines(false);
	_paletteGrid->DisableCellEditControl();
	_paletteGrid->EnableEditing(false);
	_paletteGrid->SetDefaultRowSize(grid->GetTextExtent("000").GetWidth());
	_paletteGrid->SetDefaultColSize(grid->GetTextExtent("000").GetWidth());
	_paletteGrid->EnableGridLines(false);

	// Now we can actually create the grid
	_paletteGrid->CreateGrid(8, 16, wxGrid::wxGridSelectionModes::wxGridSelectNone);

	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 16; j++) {
			_paletteGrid->SetCellValue(i, j, wxString::Format("00,00,00"));
		}
	}

	for (int i = 0; i < 16; i++) {
		wxGridCellAttr *cellAttr = _paletteGrid->GetOrCreateCellAttr(0, i);
		wxGridCellRenderer *renderer = new PaletteRenderer();
		cellAttr->SetRenderer(renderer);
		_paletteGrid->SetColAttr(i, cellAttr);
	}
	_paletteGrid->ForceRefresh();

	paletteSizer->Add(_paletteGrid, 0, wxGROW);
	palettePage->SetSizer(paletteSizer);
	palettePage->Hide();

	// We can add the pages to the notebook
	optionsBook->AddPage(optionsPage, "Options");
	optionsBook->AddPage(visualsPage, "Visuals");
	optionsBook->AddPage(logicPage, "Logic");
	optionsBook->AddPage(logPage, "Log");
	//optionsBook->AddPage(palettePage, "Palette");

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

// Render the data as a colour for any given cell of the grid
void PaletteRenderer::Draw(wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, const wxRect& rect, int row, int col, bool isSelected) {
	dc.SetBackgroundMode(wxBRUSHSTYLE_SOLID);

	wxColour clr;
	if (grid.IsThisEnabled()) {
		if (isSelected) {
			if (grid.HasFocus()) {
				clr = grid.GetSelectionBackground();

			} else {
				clr = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW);
			}
		
		} else {
			wxStringTokenizer colourTokenizer(grid.GetCellValue(row, col), ",");

			wxString inputRed   = colourTokenizer.GetNextToken();
			wxString inputGreen = colourTokenizer.GetNextToken();
			wxString inputBlue  = colourTokenizer.GetNextToken();

			int red;
			int green;
			int blue;

			sscanf(inputRed.c_str(),   "%2X", &red);
			sscanf(inputGreen.c_str(), "%2X", &green);
			sscanf(inputBlue.c_str(),  "%2X", &blue);

			clr = wxColour(red, green, blue);
		}
	
	} else {
		// grey out fields if the grid is disabled
		clr = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
	}

	dc.SetBrush(clr);
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.DrawRectangle(rect);
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





























