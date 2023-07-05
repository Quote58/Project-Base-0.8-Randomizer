// Logic
#include "rando.h"
#include "rom.h"

// Items as bits for logic expressions
const uint64_t kRMorphingBall = 0x1;
const uint64_t kRBomb 		  = 0x2;
const uint64_t kRChargeBeam   = 0x4;
const uint64_t kRSpazer	      = 0x8;
const uint64_t kRVariaSuit	  = 0x10;
const uint64_t kRHiJumpBoots  = 0x20;
const uint64_t kRSpeedBooster = 0x40;
const uint64_t kRWaveBeam	  = 0x80;
const uint64_t kRGrappleBeam  = 0x100;
const uint64_t kRGravitySuit  = 0x200;
const uint64_t kRSpaceJump    = 0x400;
const uint64_t kRSpringBall   = 0x800;
const uint64_t kRPlasmaBeam   = 0x1000;
const uint64_t kRIceBeam	  = 0x2000;
const uint64_t kRScrewAttack  = 0x4000;
const uint64_t kRXRayScope	  = 0x8000;
const uint64_t kRDashBall	  = 0x10000;
const uint64_t kRMissiles	  = 0x20000;
const uint64_t kRSupers	      = 0x40000;
const uint64_t kRPowerBombs	  = 0x80000;
const uint64_t kRPBLow 		  = 0x80002;
const uint64_t kRReserve      = 0x100000;
const uint64_t kREnergy		  = 0x200000;
const uint64_t kRMissilesAmmo = 0x400000;

// Tricks as bits
const uint64_t kRCanWallJump  = 0x1000000;
const uint64_t kRCanIBJ		  = 0x2000000;
const uint64_t kRCanHeatRun   = 0x4000000;
const uint64_t kRCanMochball  = 0x8000000;
const uint64_t kRCanSuperJump = 0x10000000;
const uint64_t kRCanWaterWJ   = 0x20000000;
const uint64_t kRCanBackflip  = 0x40000000;
const uint64_t kRGravityHeat  = 0x80000000;

/* Logic for the randomizer
 * ----
 * This randomizer works a little differently from what you
 * might expect, so I am going to give a quick run down of
 * how everything works.
 * ----
 * This randomizer uses a new algorithm I designed that moves
 * forward, but doesn't have the usual problems that come with
 * doing that. I'll explain why, but first I will explain
 * generally what happens.
 * The logic is made up of two parts. The logic bits, and the
 * logical expressions that use those bits.
 * The logical expressions are stored as strings, with the bits
 * inserted into them.
 * Ie. "1 & 2"
 * The expressions can become quite complex, with nested expressions.
 * Ie. "( 1 & 2 ) | ( 3 | ( 4 & 5 ) )"
 * The expressions format with the bits ahead of time, by referencing
 * the variable names for each bit.
 * Ie. wxString::Format("%d & %d", kRfirst, kRSecond)
 * These expressions are stored in a Location struct, of which there
 * is one for each item location in the game.
 * The rest of the location struct is pretty self explanitary, so
 * I will just explain the expression part.
 * The location struct takes in an expression as a string, but where
 * it gets more complicated is that it takes the string, and creates
 * an expression tree from it. An expression tree is a binary tree
 * that represents an expression. Each node in the tree represents an
 * operator or a bit. The bit being an item or other requirement like
 * a trick. So when the location struct is created, it creates an
 * expression tree out of the string expression, and stores the top node
 * in the struct. The reason for this expression tree, is that it is
 * very fast to evaluate. We don't need to parse anything to read the
 * expression, and we don't need bespoke functions for each requirement.
 * We just need a function that can read an expression tree quickly.
 * This also allows us to short circuit the expression where possible, saving
 * time.
 * ----
 * An expression tree is only half of the picture though. This allows us
 * to handle the first part of logic, which is deciding whether a location
 * is available. For this, the expression tree is all we need. We can check
 * if an expression tree returns true, and if so the location is available.
 * However this leads us to the second half, which is deciding what items to
 * place in the available locations. This is where things diverge more from
 * traditional randomizers.
 * ----
 * This randomizer uses a new system I designed for deciding on items. In
 * a traditional forward randomizer, the program will attempt to give the 'player'
 * an item, and see if progress is opened up. Then, from the items that are able to make
 * progression, it will randomly choose one to give. This has many issues, which
 * can be solved in various ways but ultimately the system itself is flawed. This
 * is why I designed a new system. One that does not rely on testing items, and therefor
 * does not need to waste time trying to find which items can make progress. Instead,
 * this system is designed around 'requirements'. The basic idea is that when we create
 * an expression tree out of an expression, we know what items will make progression already,
 * because they are the requirements in the expression. So instead of creating a list
 * of progression items, we just need to look at the requirements of locations that
 * are not yet accessable, as those are the items that are needed to make progression.
 * The way this is implemented currently, is that when making the location struct, we also
 * build a set of items that represent the requirements in a given expression. This may seem
 * odd, as you could always find a way to translate an item requirement into an item, and
 * then you don't need a separate set. However, we will see why this is needed in a minute.
 * With a set of items for each location, we now have a way to know what items are possibly
 * needed for making progression. Therefor, we can take all of the sets of items from
 * unavailable locations, and sort them by how many items they need for making progress, and
 * what items those are. This way we can find what the smallest size of item requirements is
 * at any given time, and apply all the items from that set. This allows for situations
 * where multiple items are needed for progression, not just one. This also allows us to
 * weigh the items themselves to make it more or less likely for powerful items to show up
 * early on.
 * ----
 * Now, a flaw you might be thinking of with this system comes in the form of OR statements.
 * If you make a set of all items from an expression, but some items are in the form of
 * OR statements, then when deciding on items, you have both instead of one or the other.
 * This is why we create the set of items instead of translating the bits into items.
 * What we can do, is process the OR statement in advance, by making the random choice of
 * item between any two items, when building the set itself. This means we are still choosing
 * a random item from the OR statement to give, we are just doing it before we look at the
 * location when processing the logic later on.
 * There is a lot more nuance to this, but that's the general idea.
 * ----
 * The structure is basically:
 * Item bits -> String expression -> Expression tree + item set -> Requirement set -> Placed items
 * Hopefully the rest is easy enough to understand from the comments
 */

// Generic Node struct representing an operator or bitset of an expression
struct Node {
	char op = '0';
	uint64_t bitset = 0;
	Node *left;
	Node *right;
	Node(char opChar) {
		op = opChar;
		left = nullptr;
		right = nullptr;
	}
	~Node() {
		delete left;
		delete right;
	}
};

// Locations have an address to write to, a name, whether they are hidden or not, major or minor, a weight, and an expression tree + item set
struct Location {
	long addr;
	wxString name;
	StorageType hidden;
	Major major;
	int weight = 0;
	Node *requirements;
	std::unordered_set<uint64_t, wxIntegerHash> items;

	Node *buildTree(wxString s, Player p) {
		wxStringTokenizer sTokenizer(s, " ");
		std::stack<Node *> nodes;
		char op = '0';

		while (sTokenizer.HasMoreTokens()) {
			wxString next = sTokenizer.GetNextToken();
			if (next[0] == '(') {
				wxString newString = "";
				int numParenthases = 1;
				while (numParenthases > 0) {
					next = sTokenizer.GetNextToken();
					
					if (next[0] == '(') {
						numParenthases++;
					
					} else if (next[0] == ')') {
						numParenthases--;
					}

					if (numParenthases > 0) {
						newString += next;
						newString += " ";
					}
				}

				if (newString != "") {
					nodes.push(buildTree(newString, p));
				}

			} else if ((next[0] == '&') || (next[0] == '|')) {
				op = next[0];
			
			} else {
				Node *node = new Node('0');
				next.ToULongLong(&node->bitset, 10);
				nodes.push(node);
			}

			if ((nodes.size() >= 2) && (op != '0')) {
				Node *node = new Node(op);
				node->right = nodes.top();
				nodes.pop();
				node->left = nodes.top();
				nodes.pop();
				if (op == '&') {
					if ((node->left->bitset >= kRCanWallJump) && ((node->left->bitset & p.collected) != node->left->bitset)) {
						node->op = '^';
					} else if ((node->right->bitset >= kRCanWallJump) && ((node->right->bitset & p.collected) != node->right->bitset)) {
						node->op = '^';
					}
				}
				nodes.push(node);
				op = '0';
			}
		}
		if (nodes.size() != 0) {
			return nodes.top();
		
		} else {
			return nullptr;
		}
	}

	void buildItems(Node *node, Player player) {
		// Morph ball has no requirements, so it ends up being null
		if (node != nullptr) {
			// Everything else has a valid pointer
			if (node->op == '&') {
				if (node->left->bitset >= kRCanWallJump) {
					if ((node->left->bitset & player.collected) == node->left->bitset) {
						// If the trick is collected, we want to use the item
						buildItems(node->right, player);
					}

				} else if (node->right->bitset >= kRCanWallJump) {
					if ((node->right->bitset & player.collected) == node->right->bitset) {
						// If the trick is collected, we want to use the item
						buildItems(node->left, player);
					}

				} else {
					buildItems(node->left, player);
					buildItems(node->right, player);
				}

			} else if (node->op == '|') {
				// If one side is a trick and not an item, check if the trick is collected
				if (node->left->op == '^') {
					buildItems(node->right, player);
				
				} else if (node->right->op == '^') {
					buildItems(node->left, player);
				
				} else if ((node->left->bitset >= kRCanWallJump) && ((node->left->bitset & player.collected) != node->left->bitset)) {
						// If the trick is not collected, we want to use the item
						buildItems(node->right, player);

				} else if ((node->right->bitset >= kRCanWallJump) && ((node->right->bitset & player.collected) != node->right->bitset)) {
						// If the trick is not collected, we want to use the item
						buildItems(node->left, player);

				// Else we want to randomly choose one of them
				} else {
					if ((rand() % 2) == 0) {
						buildItems(node->left, player);
				
					} else {
						buildItems(node->right, player);
					}
				}
			} else {
				if (node->bitset < kRCanWallJump) {
					items.insert(node->bitset);
				}
			}
		}
	}

	Location(Player p, long a, wxString n, StorageType h, Major m, wxString r) {
		name = n;
		addr = a;
		hidden = h;
		major = m;
		requirements = buildTree(r, p);
		buildItems(requirements, p);		
	}

	~Location() {
	}
};

// To be able to make a set of sets of requirements, we need to make a new hash type that combines them
struct RequirementHash {
	std::size_t operator()(const std::unordered_set<uint64_t, wxIntegerHash> &set) const {
		std::size_t hash = 0;
		// Really simple, for every set of requirements, combine the hash of each requirement to make a total hash of them all
		for (std::unordered_set<uint64_t, wxIntegerHash>::iterator it = set.begin(); it != set.end(); it++) {
			hash ^= std::hash<uint64_t>{}(*it);
		}
		// Then just return the total hash
		return hash;
	}
};

// Here's where the actual logic happens
void RandoFrame::logic() {
	// Locations
	wxVector<Location> locations;
	wxVector<Location> locationsMinor;
	wxVector<Location> availableLocations;

	// Items
	ItemDict itemPool;
	std::unordered_set<std::unordered_set<uint64_t, wxIntegerHash>, RequirementHash> requirementsList;
	wxVector<std::unordered_set<uint64_t, wxIntegerHash>> requirementsListFinal;
	std::unordered_set<uint64_t, wxIntegerHash> requirements;

	int locationWeight = 1;
	int allLocationWeights = 0;
	int allWeights = 0;

	/* --- Step 1 ---
	 * initialize the locations and item pool and
	 * build the expression trees and sets for each location
	 */
	resetLocationsAndItems(locations, locationsMinor, itemPool, allWeights);

	while (locations.size() > 0) {
		/* --- Step 2 ---
		 * Find all available locations with the current equipment and
		 * put them into a new vector. At the same time, find the lowest
		 * requirement size for the next step, as well as erasing the last
		 * item placed from the requirements set of each location
		 */
		int lowest = -1;
		requirementsList.clear();
		requirementsListFinal.clear();
		locationWeight *= 2;

		for (int i = 0; i < locations.size(); i++) {
			// Erase the last items left in requirements
			for (std::unordered_set<uint64_t, wxIntegerHash>::iterator it = requirements.begin(); it != requirements.end(); it++) {
				locations[i].items.erase(*it);
			}

			// Check the requirements for each location, and if it is available add it to the available locations
			if (checkRequirements(locations[i].requirements, itemPool) == true) {
				locations[i].weight = locationWeight;
				allLocationWeights += locationWeight;
				availableLocations.push_back(locations[i]);
				locations.erase(locations.begin() + i);
				// We just removed the location, so we need to adjust our position in the vector
				i--;
			
			// If the location is not available, add the requirements set to the requirements list
			} else {
				requirementsList.insert(locations[i].items);
				// To find the lowest, we start by making lowest the first one, and then checking if the next is lower
				if ((lowest == -1) || (locations[i].items.size() < lowest)) {
					lowest = locations[i].items.size();
				}
			}
		}

		/* --- Step 3 ---
		 * We know what the minimum requirements are, so build a list of
		 * all the sets with that minimum requirement size, then pull
		 * a random set from that list to use for items
		 */
		// If the requirementlist has nothing, it means we only have available rooms left
		if (requirementsList.size() > 0) {
			int weight = 0;

			for (std::unordered_set<std::unordered_set<uint64_t, wxIntegerHash>>::iterator it = requirementsList.begin(); it != requirementsList.end(); it++) {
				// And finally if it's one of the lowest, we add it to the list
				if (it->size() == lowest){
					// But not if it's a 2 power bomb check and we only have 1 location
					if (!((it->find(kRPBLow) != it->end()) && (availableLocations.size() < it->size() - 1 + (kRPBLow & 0xF)))) {
						for (std::unordered_set<uint64_t, wxIntegerHash>::iterator newIt = it->begin(); newIt != it->end(); newIt++) {
							if (*newIt == kRPBLow) {
								weight += itemPool[kRPowerBombs].weight;
							} else if (*newIt >= kREnergy) {
								weight += itemPool[kREnergy].weight;
							} else {
								weight += itemPool[*newIt].weight;
							}
						}
						requirementsListFinal.push_back(*it);
					}
				}
			}

			int randReq = rand() % weight;
			for (int i = 0; i < requirementsListFinal.size(); i++) {
				int itemWeight = 0;

				for (std::unordered_set<uint64_t, wxIntegerHash>::iterator it = requirementsListFinal[i].begin(); it != requirementsListFinal[i].end(); it++) {
					if (*it == kRPBLow) {
						itemWeight += itemPool[kRPowerBombs].weight;
					} else if (*it >= kREnergy) {
						itemWeight += itemPool[kREnergy].weight;
					} else {
						itemWeight += itemPool[*it].weight;
					}
				}

				if (randReq < itemWeight) {
					requirements = requirementsListFinal[i];
					break;

				} else {
					randReq -= itemWeight;
				}
			}

			/* --- Step 4 ---
			 * Use the requirement set to distribute requirements as items in
			 * random locations across the currently available locations
			 */
			for (std::unordered_set<uint64_t, wxIntegerHash>::iterator it = requirements.begin(); it != requirements.end(); it++) {
				int pos = 0;
				if (*it == kRPBLow) {
					int powerAmt = (*it & 0xF) - (10 - itemPool[kRPowerBombs].number);
					if (powerAmt > 0) {
						if (availableLocations.size() < powerAmt) {
							std::cout << "not enough locations for setting power bombs" << std::endl;
							return;
						}
						for (int i = 0; i < powerAmt; i++) {
							pos = rand() % availableLocations.size();
							setItem(availableLocations, pos, kRPowerBombs, itemPool, allWeights);
						}
						// We need to also include power bombs in the requirements to remove, because we just collected some
						// *** Remember to do this with missiles as well, when missile requirements get added ***
						requirements.insert(kRPowerBombs);
					}

				} else if (*it >= kREnergy) {
					int energyAmt = (*it & 0xF) - (14 - itemPool[kREnergy].number);
					if (energyAmt > 0) {
						if (availableLocations.size() < energyAmt) {
							std::cout << "not enough locations for setting energy" << std::endl;
							return;
						}
						for (int i = 0; i < energyAmt; i++) {
							pos = rand() % availableLocations.size();
							setItem(availableLocations, pos, kREnergy, itemPool, allWeights);
						}
					}
				
				} else {
					int randomLocation = rand() % allLocationWeights;
					for (int i = 0; i < availableLocations.size(); i++) {
						if (randomLocation < availableLocations[i].weight) {
							pos = i;
							break;
						} else {
							randomLocation -= availableLocations[i].weight;
						}
					}
					setItem(availableLocations, pos, *it, itemPool, allWeights);
				}
			}
		}
	}

	/* --- Step 5 ---
	 * For the remaining items, start by distributing
	 * the remaining non progression items
	 */
	_log += "\nNot progression:\n";
	for (int i = 0; i < availableLocations.size(); i++) {
		ItemDict::iterator it = itemPool.begin();
		do {
			int randNum = rand() % allWeights;
			if (itemPool.size() > 0) {
				it = itemPool.begin();
				while ((it != itemPool.end()) && (randNum >= itemPool[it->first].weight)) {
					randNum -= itemPool[it->first].weight;
					it++;
				}
			}			
		} while ((_player.majorMinor == true) && (itemPool[it->first].major == kMinor));
		std::cout << "setting item " << it->first << std::endl;
		setItem(availableLocations, i, it->first, itemPool, allWeights);
		i--;
	}

	// And if we are doing major/minor, then we will have locations left in the minor array
	// So distribute the remaining items into those locations
	if (locationsMinor.size() != 0) {
		std::cout << "setting minors now" << std::endl;
		for (int i = 0; i < locationsMinor.size(); i++) {
			int randNum = rand() % allWeights;
			if (itemPool.size() > 0) {
				ItemDict::iterator it = itemPool.begin();
				while ((it != itemPool.end()) && (randNum >= itemPool[it->first].weight)) {
					randNum -= itemPool[it->first].weight;
					it++;
				}
				std::cout << "setting item " << it->first << std::endl;
				setItem(locationsMinor, i, it->first, itemPool, allWeights);
				i--;
			}
		}
	}

}

// This function just sets a given item into a given location, and changes the item weight and item pool accordingly
void RandoFrame::setItem(wxVector<Location> &locations, int pos, uint64_t item, ItemDict &itemPool, int &allWeights) {
	// We start by actually writing the item into the rom
	if (itemPool[item].value == 0xEFD8) {
		// Item is dash ball, which has no chozo/hidden version, so we set it to open
		_rom->setWord(locations[pos].addr, itemPool[item].value);
	
	} else {
		// All other items get hidden or not hidden based on the location
		// Each item header is 4 bytes, and there are 21 items
		_rom->setWord(locations[pos].addr, itemPool[item].value + ((locations[pos].hidden * 21) * 4));
	}

	if (item == kRPowerBombs) {
		std::cout << "Setting a power bomb" << std::endl;
		std::cout << _player.collected << std::endl;
	}

	// And adding the entry to the log
	_log += wxString::Format("Item: %s | Location: %s\n", itemPool[item].name, locations[pos].name);

	// Now that we've used the location, we can erase it from the available locations
	locations.erase(locations.begin() + pos);

	// Next we need an iterator for the dictionary to be able to erase items from it
	ItemDict::iterator itemIt = itemPool.find(item);
	itemIt->second.number--;
	if (itemIt->second.number == 0) {
		allWeights -= itemIt->second.weight;
		itemPool.erase(itemIt);
	}
	_player.collected |= item;
}

// This is the main function for checking the expression tree requirements
bool RandoFrame::checkRequirements(Node *node, ItemDict itemPool) {
	// Morph ball has no requirements, so it ends up being null
	if (node == nullptr) {
		return true;
	}

	// Everything else has a valid pointer
	if (node->op == '&') {
		return checkRequirements(node->left, itemPool) && checkRequirements(node->right, itemPool);

	} else if (node->op == '|') {
		return checkRequirements(node->left, itemPool) || checkRequirements(node->right, itemPool);

	} else if (node->op == '^') {
		return false;

	} else {
		// If the requirement is energy or ammo, we check how much we have vs the last nyble of the requirement
		if ((node->bitset >= kREnergy) && (node->bitset <= kRMissilesAmmo)) {
			if (_player.majorMinor == true) {
				// In major minor, for now just ignore health requirements
				return true;
			}
			return (14 - itemPool[kREnergy].number) >= (node->bitset & 0xF);

		} else if (node->bitset == kRPBLow) {
			return (10 - itemPool[kRPowerBombs].number) >= (node->bitset & 0xF);
		
		} else {
			return (node->bitset & _player.collected) == node->bitset;
		}
	}
}

void RandoFrame::resetLocationsAndItems(wxVector<Location> &locations, wxVector<Location> &locationsMinor, ItemDict &itemPool, int &allWeights) {
	// The rest of these are changeable by the user
	uint64_t kREnergyLow  = kREnergy + _player.energyLow;
	uint64_t kREnergyMed  = kREnergy + _player.energyMed;
	uint64_t kREnergyHigh = kREnergy + _player.energyHigh;

	// No idea how much energy you actually need for a lower norfair heatrun tbh
	uint64_t kREnergyHeatLow = kREnergy + 5;
	uint64_t kREnergyHeatHigh = kREnergy + 8;

	// Including these missile ammo ones
	uint64_t kRMissilesLow  = kRMissilesAmmo + _player.missilesLow;
	uint64_t kRMissilesMed  = kRMissilesAmmo + _player.missilesMed;
	uint64_t kRMissilesHigh = kRMissilesAmmo + _player.missilesHigh;

	wxString kRCanUsePB          = wxString::Format("( %ld & %ld )", kRMorphingBall, kRPowerBombs);
	wxString kRCanAccessRB       = kRCanUsePB;	// This one seems to be as simple as having PBs...
	wxString kRCanUseBomb        = wxString::Format("( %ld & %ld )", kRMorphingBall, kRBomb);
	wxString kRCanSpeedball      = wxString::Format("( %ld & %ld )", kRMorphingBall, kRSpeedBooster);
	wxString kRCanUse2PB         = wxString::Format("( %ld & %ld )", kRMorphingBall, kRPBLow);
	wxString kRCanOpenMissile    = wxString::Format("( %ld | %ld )", kRMissiles, kRSupers);
	if (_player.majorMinor == true) {
		// This is the most jank way to do this but :shrug:
		kRCanOpenMissile		 = wxString::Format("( %ld )", kRMissiles);
	}
	wxString kRCanAccessWS       = wxString::Format("( %ld & %s & %ld & ( %ld | %ld | %ld ) )", kRSupers, kRCanUsePB, kREnergyMed, kRSpaceJump, kRGrappleBeam, kRSpeedBooster);
	wxString kRGauntlet          = wxString::Format("( ( %ld | %ld ) & ( %ld | %s | %s | %ld ) )", kRHiJumpBoots, kRCanWallJump, kRSpeedBooster, kRCanUseBomb, kRCanUse2PB, kRScrewAttack);
	wxString kRCanOpenPassages   = wxString::Format("( %s | %s | %ld )", kRCanUseBomb, kRCanUsePB, kRScrewAttack);
	wxString kRCanEnterPassages  = wxString::Format("( %s | %s )", kRCanUseBomb, kRCanUsePB);
	wxString kRCanDestroyBomb    = wxString::Format("( %s | %ld | %ld )", kRCanEnterPassages, kRScrewAttack, kRSpeedBooster);
	wxString kRCanAccessGB       = wxString::Format("( %s | ( %s & %ld ) | ( %s & %s ) )", kRCanEnterPassages, kRCanUsePB, kRSupers, kRCanOpenMissile, kRCanUsePB);
	wxString kRCanAccessPB       = wxString::Format("( %s & %ld | ( %s & %s ) | %s )", kRCanEnterPassages, kRSupers, kRCanAccessGB, kRCanOpenMissile, kRCanAccessRB);
	wxString kRCanDefeatKraid    = wxString::Format("( %s & %s & %s & %ld )", kRCanAccessRB, kRCanEnterPassages, kRCanOpenMissile, kREnergyLow);
	wxString kRCanAccessNorfair  = wxString::Format("( %s & ( %ld | ( %ld & %ld ) | ( %ld & %ld ) ) )", kRCanAccessRB, kRVariaSuit, kRGravitySuit, kRGravityHeat, kRCanHeatRun, kREnergyHeatLow);
	wxString kRCanAccessLN       = wxString::Format("( %s & %ld & ( %ld | ( %ld & %ld ) | ( %ld & %ld ) ) & ( ( %ld & %ld & %ld ) | %ld ) )", kRCanAccessNorfair, kREnergyHigh, kRVariaSuit, kRGravitySuit, kRGravityHeat, kRCanHeatRun, kREnergyHeatHigh, kRGravitySuit, kRSpaceJump, kRPowerBombs, kRCanBackflip);
	wxString kRCanAccessIce      = wxString::Format("( %s & %s )", kRCanAccessNorfair, kRCanUsePB);
	wxString kRCanAccessCroc     = wxString::Format("( %s & %ld )", kRCanAccessNorfair, kRSupers);
	wxString kRCanAccessMaridia  = wxString::Format("( %s & ( %ld | %ld ) & %s & %ld )", kRCanAccessRB, kRGravitySuit, kRCanWaterWJ, kRCanUsePB, kREnergyHigh);
	wxString kRCanAccessAquaduct = wxString::Format("( %s & ( %ld | %ld | %ld ) )", kRCanAccessMaridia, kRSpeedBooster, kRSpaceJump, kRGrappleBeam);
	wxString kRCanMeetEtecoons   = wxString::Format("( %s & %s )", kRCanAccessGB, kRCanUsePB);

	// Then the PB locations
	Location locationsArrayPB[] = {
	Location(_player, 0x781D4, "Power Bomb (Crateria surface)", 				 kNormal, kMinor, wxString::Format("%s & ( %ld | %ld | ( %s & %ld ) )", kRCanUsePB, kRSpeedBooster, kRSpaceJump, kRCanUseBomb, kRCanIBJ)),
	Location(_player, 0x7EEAC, "Missile (Underwater outside Wrecked Ship)", 	 kNormal, kMinor, kRCanAccessWS),
	Location(_player, 0x7EEB2, "Missile (outside Wrecked Ship top)", 			 kNormal, kMinor, kRCanAccessWS),
	Location(_player, 0x7EEB8, "Missile (outside WS under super block)", 		 kNormal, kMinor, kRCanAccessWS),
	Location(_player, 0x78248, "Missile (Crateria moat)", 						 kNormal, kMinor, kRCanAccessWS),
	Location(_player, 0x7846A, "Energy Tank (Crateria gauntlet)", 				 kNormal, kMajor, kRGauntlet),
	Location(_player, 0x78404, "Bomb", 											 kNormal, kMajor, wxString::Format("%s & %ld", kRCanOpenMissile, kRMorphingBall)),
	Location(_player, 0x78432, "Energy Tank (Crateria terminator)", 			 kNormal, kMajor, kRCanEnterPassages),
	Location(_player, 0x7F39E, "Missile (Crateria old MB missiles)", 			 kNormal, kMinor, wxString::Format("%ld & ( %ld | %ld | %ld )", kRMorphingBall, kRBomb, kRPowerBombs, kRScrewAttack)),
	Location(_player, 0x78464, "Missile (Crateria 2st gauntlet missiles)", 		 kNormal, kMinor, kRGauntlet),
	Location(_player, 0x7825E, "Missile (Crateria 1st gauntlet missiles)", 		 kNormal, kMinor, kRGauntlet),
	Location(_player, 0x78478, "Super Missile (Crateria supers)", 				 kNormal, kMinor, wxString::Format("%s & %ld", kRCanUsePB, kRSpeedBooster)),
	Location(_player, 0x78486, "Missile (Crateria Final Missiles)", 			 kNormal, kMinor, kRCanEnterPassages),
	Location(_player, 0x78444, "Missile (Crateria Map Station Missiles)", 		 kNormal, kMinor, wxString::Format("%s & %ld", kRCanOpenMissile, kRMorphingBall)),
	Location(_player, 0x784AC, "Power Bomb (Brinstar Etecoons)", 				 kNormal, kMinor, kRCanMeetEtecoons),
	Location(_player, 0x7F31C, "Super Missile (Spospo Supers)", 				 kChozo,  kMajor, wxString::Format("%s & %ld", kRCanAccessPB, kRSupers)),
	Location(_player, 0x7850C, "Missile (Brinstar below Early Supers)", 		 kNormal, kMinor, wxString::Format("%s & %s & %ld", kRCanAccessGB, kRCanOpenMissile, kRMorphingBall)),
	Location(_player, 0x78512, "Super Missile (Brinstar Early Supers)", 		 kNormal, kMinor, wxString::Format("%s & %s & ( %s | ( %ld & %ld ) | ( %ld & %ld ) )", kRCanAccessGB, kRCanOpenMissile, kRCanSpeedball, kRMorphingBall, kRCanMochball, kRMorphingBall, kRDashBall)),
	Location(_player, 0x7852C, "Reserve Tank (Brinstar Reserve)", 				 kChozo,  kMajor, wxString::Format("%s & %s & ( %s | ( %ld & %ld ) | ( %ld & %ld ) )", kRCanAccessGB, kRCanOpenMissile, kRCanSpeedball, kRMorphingBall, kRCanMochball, kRMorphingBall, kRDashBall)),
	Location(_player, 0x78532, "Missile (Brinstar Reserve Ron Popeil Missiles)", kHidden, kMinor, wxString::Format("%s & %s & %s & ( %s | ( %ld & %ld ) | ( %ld & %ld ) )", kRCanAccessGB, kRCanOpenMissile, kRCanEnterPassages, kRCanSpeedball, kRMorphingBall, kRCanMochball, kRMorphingBall, kRDashBall)),
	Location(_player, 0x78538, "Missile (Brinstar Reserve Missiles)", 			 kNormal, kMinor, wxString::Format("%s & %s & ( %s | ( %ld & %ld ) | ( %ld & %ld ) )", kRCanAccessGB, kRCanOpenMissile, kRCanSpeedball, kRMorphingBall, kRCanMochball, kRMorphingBall, kRDashBall)),
	Location(_player, 0x78608, "Missile (Big Pink Brinstar top)", 				 kNormal, kMinor, wxString::Format("%s & ( %ld | %ld | %ld | %ld )", kRCanAccessPB, kRGrappleBeam, kRSpaceJump, kRSpeedBooster, kRCanWallJump)),
	Location(_player, 0x7860E, "Missile (Big Pink Brinstar Charge Missiles)", 	 kNormal, kMinor, kRCanAccessPB),
	Location(_player, 0x78614, "Charge Beam", 									 kNormal, kMajor, kRCanAccessPB),
	Location(_player, 0x7F232, "Power Bomb (Pink Brinstar Power Bombs)", 		 kNormal, kMinor, wxString::Format("%s & %s & ( %ld | %ld | %ld | %ld )", kRCanAccessPB, kRCanUsePB, kRGrappleBeam, kRSpaceJump, kRSpeedBooster, kRCanWallJump)),
	Location(_player, 0x78670, "Missile (Brinstar Pipe Missiles)", 				 kNormal, kMinor, wxString::Format("%s | %s", kRCanAccessPB, kRCanEnterPassages)),
	Location(_player, 0x7867E, "Morphing Ball", 								 kNormal, kMajor, ""),
	Location(_player, 0x78684, "Power Bomb (Blue Brinstar)", 					 kNormal, kMinor, kRCanUsePB),
	Location(_player, 0x7EBB8, "Missile (Blue Brinstar middle)", 				 kNormal, kMinor, wxString::Format("%ld & %s", kRMorphingBall, kRCanOpenMissile)),
	Location(_player, 0x7EBBE, "Energy Tank (Blue Brinstar)", 					 kNormal, kMajor, kRCanOpenMissile),
	Location(_player, 0x7F4F2, "Energy Tank (Brinstar near Etecoons E-tank)", 	 kNormal, kMajor, kRCanMeetEtecoons),
	Location(_player, 0x7F9CC, "Super Missile (Brinstar near Etecoons)", 		 kNormal, kMinor, kRCanMeetEtecoons),
	Location(_player, 0x787EE, "Energy Tank (Pink Brinstar under Charge Beam)",  kNormal, kMajor, wxString::Format("%s & %s & %ld", kRCanAccessPB, kRCanUsePB, kRSpeedBooster)),
	Location(_player, 0x7EAC8, "Missile (Blue Brinstar First Missiles)", 		 kChozo,  kMajor, wxString::Format("%ld", kRMorphingBall)),
	Location(_player, 0x78824, "Energy Tank (Pink Brinstar Hoppers Room)", 		 kNormal, kMajor, wxString::Format("%s & %s", kRCanAccessPB, kRCanUsePB)),
	Location(_player, 0x7EBC4, "Dash Ball (Blue Brinstar upper Billy Mays)", 	 kNormal, kMajor, wxString::Format("%s & %ld & ( %ld | %ld | ( %ld & %ld ) )", kRCanOpenMissile, kRMorphingBall, kRSpaceJump, kRCanWallJump, kRBomb, kRCanIBJ)),
	Location(_player, 0x7EBCA, "Missile (Blue Brinstar lower Billy Mays)", 		 kNormal, kMinor, wxString::Format("%s & %ld", kRCanOpenMissile, kRMorphingBall)),
	Location(_player, 0x7EBDC, "Missile (Blue Brinstar Ceiling tile)",			 kHidden, kMinor, wxString::Format("%s & ( %ld | %ld | %ld | ( %s & %ld ) )", kRCanOpenMissile, kRHiJumpBoots, kRSpaceJump, kRSpeedBooster, kRCanUseBomb, kRCanIBJ)),
	Location(_player, 0x7FFD1, "X-Ray Visor", 									 kNormal, kMajor, wxString::Format("( %s & %ld & %ld & %ld ) | ( %ld & %s & %s )", kRCanAccessPB, kRSupers, kRMorphingBall, kRGrappleBeam, kRGrappleBeam, kRCanAccessRB, kRCanUsePB)),
	Location(_player, 0x788C4, "Power Bomb (Beta Power Bomb Room)", 			 kNormal, kMinor, wxString::Format("%s & %ld & %s", kRCanAccessRB, kRSupers, kRCanUsePB)),
	Location(_player, 0x788DE, "Power Bomb (Alpha Power Bomb Room)", 			 kChozo,  kMajor, wxString::Format("%s & %ld", kRCanAccessRB, kRSupers)),
	Location(_player, 0x788E4, "Missile (Alpha Power Bomb Room)", 				 kNormal, kMinor, wxString::Format("%s & %s", kRCanAccessRB, kRCanDestroyBomb)),
	Location(_player, 0x7896E, "Spazer", 										 kNormal, kMajor, wxString::Format("%s & %ld", kRCanAccessRB, kRSupers)),
	Location(_player, 0x7FC7E, "Energy Tank (Kraid)", 							 kChozo,  kMajor, wxString::Format("%s & %s", kRCanAccessRB, kRCanDefeatKraid)),
	Location(_player, 0x7FC06, "Missile (Kraid)", 								 kChozo,  kMinor, wxString::Format("%s & %s", kRCanAccessRB, kRCanDefeatKraid)),
	Location(_player, 0x7F150, "Varia Suit", 									 kNormal, kMajor, wxString::Format("%s & %s", kRCanAccessRB, kRCanDefeatKraid)),
	Location(_player, 0x7F9C6, "Missile (Brinstar Map Station Missiles)", 		 kNormal, kMinor, wxString::Format("%s & %s", kRCanAccessGB, kRCanOpenMissile)),
	Location(_player, 0x7F600, "Missile (Cathedral Missiles)", 					 kHidden, kMinor, kRCanAccessNorfair),
	Location(_player, 0x78B24, "Ice Beam", 										 kNormal, kMajor, kRCanAccessIce),
	Location(_player, 0x7EFA6, "Missile (Left Grapple Missiles)", 				 kNormal, kMinor, wxString::Format("%s | ( %s & %s )", kRCanAccessRB, kRCanAccessCroc, kRCanEnterPassages)),
	Location(_player, 0x7F5D6, "Energy Tank (Crocomire)", 						 kNormal, kMajor, wxString::Format("%s | ( %s & %s )", kRCanAccessCroc, kRCanAccessRB, kRCanEnterPassages)),
	Location(_player, 0x78BAC, "Hi-Jump Boots", 								 kNormal, kMajor, wxString::Format("%s & %s", kRCanAccessRB, kRCanEnterPassages)),
	Location(_player, 0x7FE1A, "Missile (Crocomire Escape)", 					 kNormal, kMinor, kRCanAccessRB),
	Location(_player, 0x78BE6, "Missile (Hi-Jump Boots)", 						 kNormal, kMinor, wxString::Format("%s & %s", kRCanAccessRB, kRCanEnterPassages)),
	Location(_player, 0x78BEC, "Energy Tank (Hi-Jump Boots)", 					 kNormal, kMajor, wxString::Format("%s & %s", kRCanAccessRB, kRCanEnterPassages)),
	Location(_player, 0x7FCC6, "Power Bomb (Grapple Power Bombs)", 				 kNormal, kMinor, wxString::Format("( %ld | %ld | %ld ) & ( %s & %s ) | %s", kRSpeedBooster, kRGrappleBeam, kRSpaceJump, kRCanAccessIce, kRCanEnterPassages, kRCanAccessCroc)),
	Location(_player, 0x78BA4, "Missile (Crocomire)", 							 kNormal, kMinor, wxString::Format("%s & ( %ld | %ld | %ld )", kRCanAccessCroc, kRSpeedBooster, kRGrappleBeam, kRSpaceJump)),
	Location(_player, 0x7FCC0, "Missile (Right Grapple Missiles)", 				 kNormal, kMinor, wxString::Format("( %ld | %ld | %ld ) & ( %s & %s ) | %s", kRSpeedBooster, kRGrappleBeam, kRSpaceJump, kRCanAccessIce, kRCanEnterPassages, kRCanAccessCroc)),
	Location(_player, 0x7EFA0, "Grapple Beam", 									 kNormal, kMajor, wxString::Format("( %s & %s ) | %s", kRCanAccessRB, kRCanEnterPassages, kRCanAccessCroc)),
	Location(_player, 0x7F642, "Reserve Tank (Norfair Reserves)", 				 kChozo,  kMajor, wxString::Format("%s & %ld & ( %ld | %ld | ( %s & %ld ) )", kRCanAccessNorfair, kRSupers, kRGrappleBeam, kRCanWallJump, kRCanUseBomb, kRCanIBJ)),
	Location(_player, 0x7F63C, "Missile (Norfair Reserves 2nd Missiles)", 		 kNormal, kMinor, wxString::Format("%s & %ld & ( %ld | %ld | ( %s & %ld ) )", kRCanAccessNorfair, kRSupers, kRGrappleBeam, kRCanWallJump, kRCanUseBomb, kRCanIBJ)),
	Location(_player, 0x7F636, "Missile (Norfair Reserves 1st Missiles)", 		 kNormal, kMinor, wxString::Format("%s & %ld & ( %ld | %ld | ( %s & %ld ) )", kRCanAccessNorfair, kRSupers, kRGrappleBeam, kRCanWallJump, kRCanUseBomb, kRCanIBJ)),
	Location(_player, 0x78166, "Missile (Bubble Mountain)", 					 kNormal, kMinor, kRCanAccessNorfair),
	Location(_player, 0x78C74, "Missile (Speed Booster)", 						 kHidden, kMinor, kRCanAccessNorfair),
	Location(_player, 0x78C82, "Speed Booster", 								 kNormal, kMajor, wxString::Format("( %s & %s ) | ( %s & %s )", kRCanAccessNorfair, kRCanOpenMissile, kRCanAccessNorfair, kRCanUsePB)),
	Location(_player, 0x7816C, "Missile (Wave/LN Escape Missiles)", 			 kNormal, kMinor, wxString::Format("%s & ( %ld | %ld )", kRCanAccessNorfair, kRHiJumpBoots, kRCanWallJump)),
	Location(_player, 0x78172, "Wave Beam", 									 kNormal, kMajor, wxString::Format("%s & %s & ( %ld | %ld | %ld | ( %s & %ld ) )", kRCanAccessNorfair, kRCanOpenMissile, kRSpeedBooster, kRGrappleBeam, kRSpaceJump, kRCanUseBomb, kRCanIBJ)),
	Location(_player, 0x78E4A, "Missile (Gold Torizo)", 						 kNormal, kMinor, wxString::Format("%s & %s & %ld", kRCanAccessNorfair, kRCanUsePB, kRChargeBeam)),
	Location(_player, 0x78E50, "Super Missile (Gold Torizo)", 					 kNormal, kMinor, wxString::Format("%s & %s & %ld", kRCanAccessNorfair, kRCanUsePB, kRChargeBeam)),
	Location(_player, 0x78F00, "Missile (Mickey Mouse)", 						 kNormal, kMinor, wxString::Format("%s & %s", kRCanAccessLN, kRCanEnterPassages)),
	Location(_player, 0x78F8E, "Missile (Spring Ball Maze)", 					 kNormal, kMinor, wxString::Format("%s & %s", kRCanAccessLN, kRCanEnterPassages)),
	Location(_player, 0x78FD2, "Power Bomb (Spring Ball Maze)", 				 kNormal, kMinor, wxString::Format("%s & %s", kRCanAccessLN, kRCanEnterPassages)),
	Location(_player, 0x79072, "Power Bomb (Power Bombs of Shame)", 			 kNormal, kMinor, wxString::Format("%s & %s", kRCanAccessLN, kRCanEnterPassages)),
	Location(_player, 0x790E2, "Missile (FrankerZ)", 							 kNormal, kMinor, wxString::Format("%s & %s", kRCanAccessLN, kRCanEnterPassages)),
	Location(_player, 0x7EA78, "Energy Tank (Ridley)", 							 kNormal, kMajor, wxString::Format("%s & %s & %ld", kRCanAccessLN, kRCanUsePB, kRSupers)),
	Location(_player, 0x79110, "Screw Attack", 									 kNormal, kMajor, wxString::Format("%s & %s", kRCanAccessLN, kRCanUsePB)),
	Location(_player, 0x79136, "Energy Tank (Lower Norfair Fire Flea E-tank)", 	 kNormal, kMajor, wxString::Format("%s & %ld & ( %ld | %ld )", kRCanAccessLN, kRSupers, kRSpaceJump, kRCanWallJump)),
	Location(_player, 0x7C265, "Missile (Wrecked Ship Spike Room)", 			 kNormal, kMinor, kRCanAccessWS),
	Location(_player, 0x78A9E, "Reserve Tank (Wrecked Ship Reserves)", 			 kChozo,  kMajor, wxString::Format("%s & ( %ld | %ld | %ld )", kRCanAccessWS, kRSpeedBooster, kRGrappleBeam, kRSpaceJump)),
	Location(_player, 0x78AA4, "Missile (Wrecked Ship Reserves)", 				 kNormal, kMinor, wxString::Format("%s & ( %ld | %ld | %ld )", kRCanAccessWS, kRSpeedBooster, kRGrappleBeam, kRSpaceJump)),
	Location(_player, 0x7C231, "Missile (Wrecked Ship Attic)", 					 kNormal, kMinor, kRCanAccessWS),
	Location(_player, 0x7C337, "Energy Tank (Wrecked Ship)", 					 kNormal, kMajor, wxString::Format("%s & ( %ld | %ld | %ld )", kRCanAccessWS, kRGrappleBeam, kRSpaceJump, kRCanWallJump)),
	Location(_player, 0x7C357, "Super Missile (Wrecked Ship left)", 			 kNormal, kMinor, kRCanAccessWS),
	Location(_player, 0x7C365, "Super Missile (Wrecked Ship right)", 			 kNormal, kMinor, kRCanAccessWS),
	Location(_player, 0x78A86, "Gravity Suit", 									 kNormal, kMajor, wxString::Format("%s & ( %ld | %ld | %ld )", kRCanAccessWS, kRSpeedBooster, kRGrappleBeam, kRSpaceJump)),
	Location(_player, 0x7C34D, "Missile (Wrecked Ship Map Station Missiles)", 	 kHidden, kMinor, wxString::Format("%s & ( %ld | %ld | %ld )", kRCanAccessWS, kRIceBeam, kRHiJumpBoots, kRSpaceJump)),
	Location(_player, 0x7C437, "Missile (Maridia Main Street)", 				 kNormal, kMinor, wxString::Format("%s & %ld & %ld", kRCanAccessRB, kRGravitySuit, kRSpeedBooster)),
	Location(_player, 0x7C43D, "Super Missile (Maridia Crab Supers)", 			 kNormal, kMinor, kRCanAccessMaridia),
	Location(_player, 0x7C47D, "Energy Tank (Maridia Turtles)", 				 kNormal, kMajor, wxString::Format("%s & ( %ld | %ld | %ld )", kRCanAccessMaridia, kRSpaceJump, kRGrappleBeam, kRSpeedBooster)),
	Location(_player, 0x7C483, "Missile (Maridia Turtles)", 					 kNormal, kMinor, kRCanAccessMaridia),
	Location(_player, 0x7F196, "Super Missile (Maridia Watering Hole Supers)", 	 kNormal, kMinor, wxString::Format("%s & %ld", kRCanAccessMaridia, kRHiJumpBoots)),
	Location(_player, 0x7F19C, "Missile (Maridia Watering Hole Missiles)", 		 kNormal, kMinor, wxString::Format("%s & %ld", kRCanAccessMaridia, kRHiJumpBoots)),
	Location(_player, 0x7C509, "Missile (Maridia Beach Missiles)", 				 kNormal, kMinor, wxString::Format("%s & ( %ld | %ld | %ld | ( %ld & %s ) | ( %ld & %ld ) )", kRCanAccessMaridia, kRHiJumpBoots, kRSpaceJump, kRGravitySuit, kRCanIBJ, kRCanUseBomb, kRCanSuperJump, kRSupers)),
	Location(_player, 0x7C553, "Plasma Beam", 									 kNormal, kMajor, kRCanAccessMaridia),
	Location(_player, 0x7FECB, "Missile (Maridia left sand pit room)", 			 kNormal, kMinor, kRCanAccessAquaduct),
	Location(_player, 0x7FEC5, "Reserve Tank (Maridia Reserves)", 				 kChozo,  kMajor, kRCanAccessAquaduct),
	Location(_player, 0x7FED1, "Missile (Maridia right sand pit room)", 		 kNormal, kMinor, kRCanAccessAquaduct),
	Location(_player, 0x7FED7, "Power Bomb (Maridia right sand pit room)", 		 kNormal, kMinor, kRCanAccessAquaduct),
	Location(_player, 0x7FEB9, "Missile (Aqueduct Missiles)", 					 kNormal, kMinor, wxString::Format("%s & %ld & %ld", kRCanAccessAquaduct, kRSpeedBooster, kRGravitySuit)),
	Location(_player, 0x7FEBF, "Super Missile (Aqueduct Supers)", 				 kNormal, kMinor, wxString::Format("%s & %ld & %ld", kRCanAccessAquaduct, kRSpeedBooster, kRGravitySuit)),
	Location(_player, 0x7C6E5, "Spring Ball", 									 kNormal, kMajor, wxString::Format("%s & ( %ld | %ld )", kRCanAccessMaridia, kRSpaceJump, kRCanWallJump)),
	Location(_player, 0x780D2, "Missile (Draygon)",								 kHidden, kMinor, wxString::Format("%s & ( %ld | %ld | %ld )", kRCanAccessAquaduct, kRGrappleBeam, kRSpaceJump, kRCanWallJump)),
	Location(_player, 0x7F1F6, "Energy Tank (Botwoon)", 						 kNormal, kMajor, wxString::Format("%s & %s", kRCanAccessAquaduct, kRCanEnterPassages)),
	Location(_player, 0x7C791, "Space Jump", 									 kNormal, kMajor, wxString::Format("%s & ( %ld | %ld | %ld )", kRCanAccessAquaduct, kRGrappleBeam, kRSpaceJump, kRCanWallJump)),
	Location(_player, 0x7C4B1, "Missile (Maridia Map Station Missiles Corridor)",kNormal, kMinor, kRCanAccessRB)};

	// If the player chose major/minor, we want to section off the major locations from the minors
	if (_player.majorMinor == true) {
		for (int i = 0; i < 105; i++) {
			if (locationsArrayPB[i].major == kMajor) {
				locations.push_back(locationsArrayPB[i]);

			} else {
				locationsMinor.push_back(locationsArrayPB[i]);
			}
		}
	
	} else {
		for (int i = 0; i < 105; i++) {
			locations.push_back(locationsArrayPB[i]);
		}
	}

	// Now the item pool
	itemPool[kRMorphingBall] = Item("Morph Ball",    0xEF23, 10);
	itemPool[kRBomb]         = Item("Bombs", 	     0xEEE7, 10);
	itemPool[kRChargeBeam]   = Item("Charge Beam",   0xEEEB, 20);
	itemPool[kRSpazer]       = Item("Spazer", 	     0xEEFF, 20);
	itemPool[kRVariaSuit]    = Item("Varia Suit",    0xEF07, 10);
	itemPool[kRHiJumpBoots]  = Item("Hijump Boots",  0xEEF3, 20);
	itemPool[kRSpeedBooster] = Item("Speed Booster", 0xEEF7, 10);
	itemPool[kRWaveBeam]     = Item("Wave Beam",	 0xEEFB, 20);
	itemPool[kRGrappleBeam]  = Item("Grapple Beam",	 0xEF17, 20);
	itemPool[kRGravitySuit]  = Item("Gravity Suit",	 0xEF0B, 1);
	itemPool[kRSpaceJump]    = Item("Space Jump",	 0xEF1B, 5);
	itemPool[kRSpringBall]   = Item("Spring Ball",	 0xEF03, 20);
	itemPool[kRPlasmaBeam]   = Item("Plasma Beam",	 0xEF13, 5);
	itemPool[kRIceBeam]      = Item("Ice Beam",		 0xEEEF, 20);
	itemPool[kRScrewAttack]  = Item("Screw Attack",	 0xEF1F, 5);
	itemPool[kRXRayScope]    = Item("Xray Scope",	 0xEF0F, 40);
	itemPool[kRDashBall]     = Item("Dash Ball",     0xEFD8, 40);
	
	// There are max 4 reserve tanks
	itemPool[kRReserve] = Item("Reserve Tank", 0xEF27, 40);
	itemPool[kRReserve].number = 4;

	// There are max 14 Energy tanks (we can use the energyLow instead of a dedicated one because it'll be a special case anyway)
	itemPool[kREnergy] = Item("Energy Tank", 0xEED7, 60);
	itemPool[kREnergy].number = 14;

	// There are max 50 Missiles
	itemPool[kRMissiles] = Item("Missile", 0xEEDB, 150);
	itemPool[kRMissiles].number = 50;
	itemPool[kRMissiles].major = kMinor;

	// And max 10 Supers/Power Bombs
	itemPool[kRSupers] = Item("Super Missile", 0xEEDF, 80);
	itemPool[kRSupers].number = 10;
	itemPool[kRSupers].major = kMinor;

	itemPool[kRPowerBombs] = Item("Power Bomb", 0xEEE3, 80);
	itemPool[kRPowerBombs].number = 10;
	itemPool[kRPowerBombs].major = kMinor;

	// We also want to know the total weight, so we just add up the weight of all of them (this could be hard coded, but I am assuming I will allow user changeable weights in the future)
	for (ItemDict::iterator it = itemPool.begin(); it != itemPool.end(); it++) {
		allWeights += itemPool[it->first].weight;
	}

}












