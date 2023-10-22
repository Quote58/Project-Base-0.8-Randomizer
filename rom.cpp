#include "rom.h"

Rom::Rom(wxString path) {
	// Make a new file object for the base rom
	_rom = new wxFile(path, wxFile::read_write);
	
	// If the file is opened (ie. the path was valid), then we make a new data buffer and give it the file contents
	if (_rom->IsOpened()) {
		_dataBuffer = (wxByte *)malloc(_rom->Length());
		_rom->Read(_dataBuffer, _rom->Length());

	// Otherwise just give a message and end
	} else {
		wxLogError("File could not be opened!");
		return;
	}

	// The name can be found from the fileName object with a given path
	wxFileName name(path);
	_name = name.GetName();
}

void Rom::makeNewRom(wxString fileName) {
	// Create a new file object that will be used for making the rom
	wxFile *output = new wxFile();

	// Create the actual rom file with the filename given
	output->Create(fileName, true);

	// Start at the beginning of the file, and write the data buffer to the new rom
	output->Seek(0);
	output->Write(_dataBuffer, _rom->Length());
}

wxByte Rom::getByte(uint64_t offset) {
	if (offset < _rom->Length()) {
		return _dataBuffer[offset];

	} else {
		std::cout << "invalid offset! Can't access offset, returning 0xFF instead " << offset << std::endl;
		return 0xFF;
	}
}

void Rom::setByte(uint64_t offset, wxByte byte) {
	if (offset >= _rom->Length()) {
		std::cout << "invalid offset! Can't access offset " << offset << std::endl;
		return;
	}

	// Set the byte at the offset in the data buffer to the given byte
	_dataBuffer[offset] = byte;
}

void Rom::setWord(uint64_t offset, uint16_t word) {
	if (offset >= _rom->Length()) {
		std::cout << "invalid offset! Can't access offset " << offset << std::endl;
		return;
	}

	// A word is a uint16, so we take each half and set it to the data buffer
	_dataBuffer[offset] = word & 0x00FF;
	_dataBuffer[offset + 1] = (word & 0xFF00) >> 8;
}

void Rom::setBytes(uint64_t offset, wxVector<wxByte> bytes) {
	if ((offset + bytes.size()) >= _rom->Length()) {
		std::cout << "invalid offset! Can't access offset and/or number of bytes " << offset << std::endl;
		return;
	}

	// Loop over the bytes in the vector and set each one in the data buffer at increasing position
	for (int i = 0; i < bytes.size(); i++) {
		_dataBuffer[offset + i] = bytes[i];
	}
}

void Rom::applyPatch(wxByte *patch) {
	if ((patch[0] == 'P') && (patch[1] == 'A') && (patch[2] == 'T') && (patch[3] == 'C') && (patch[4] == 'H')) {
		uint64_t index = 5;
		while (!((patch[index] == 'E') && (patch[index + 1] == 'O') && (patch[index + 2] == 'F'))) {
			uint64_t currentOffset = (((uint64_t) (patch[index]) << 16)) | (((uint64_t) (patch[index + 1]) << 8)) | (((uint64_t) patch[index + 2]));
			index += 3;
			uint16_t currentSize = ((uint16_t) (patch[index] << 8)) | ((uint16_t) patch[index + 1]);
			index += 2;
			if (currentSize == 0) {
				// RLE
				uint16_t rleSize = ((uint16_t) (patch[index] << 8)) | ((uint16_t) patch[index + 1]);
				index += 2;
				for (int i = 0; i < rleSize; i++) {
					setByte(currentOffset + i, patch[index]);
				}
				index++;
			
			} else {
				for (int i = 0; i < currentSize; i++) {
					setByte(currentOffset + i, patch[index]);
					index++;
				}
			}
		}

	} else {
		std::cout << "not a patch" << std::endl;
		return;
	}
}

void Rom::storeColour(uint64_t addr, wxColour clr) {
	// Convert the colour to 15bit
	int newR = trunc(float(clr.Red())   / float((1 << 8) - 1) * float((1 << 5) - 1) + 0.5f);
	int newG = trunc(float(clr.Green()) / float((1 << 8) - 1) * float((1 << 5) - 1) + 0.5f);
	int newB = trunc(float(clr.Blue())  / float((1 << 8) - 1) * float((1 << 5) - 1) + 0.5f);

	// Now we need to store the new colours into the palette buffer by compressing them into a single 15bit value across 2 bytes
	_dataBuffer[addr] = newR | (newG << 5);
	_dataBuffer[addr + 1] = (newG >> 3) | newB << 2;
}

/* This compression code is converted to c++ from https://github.com/DJuttmann/SM3E/blob/master/SM3E/Tools/Compression.cs
 * thank you so much DJuttmann! */
enum paletteSizes {
	kPalSize = 128,
	kPalBytes = 256
};

void Rom::decompressPalette(wxColour *palette, uint64_t offset) {
	int startPos = offset;
	wxByte copyByte1;
	wxByte copyByte2;
	int address;
	bool longLength;
	wxVector<wxByte> output;

	while (offset < (startPos + 0x8000)) {
		wxByte currentByte = getByte(offset);
		offset++;
		int command = currentByte >> 5;
		int length = (currentByte & 0b11111) + 1;
		
		if (currentByte == 0xFF) {
			offset = startPos + 0x8000; // tell the loop to end, we've reached the end of the compressed data
		
		} else {
			do {
				longLength = false;
				switch (command) {
					case 0b000:
						for (int i = 0; i < length; i++) {
							output.push_back(getByte(offset + i));
						}
						offset += length;
						break;
					case 0b001:
						copyByte1 = getByte(offset);
						offset++;
						for (int i = 0; i < length; i++) {
							output.push_back(copyByte1);
						}
						break;
					case 0b010:
						copyByte1 = getByte(offset);
						copyByte2 = getByte(offset + 1);
						offset += 2;
						for (int i = 0; i < length; i++) {
							output.push_back((i % 2) == 0 ? copyByte1 : copyByte2); 
						}
						break;
					case 0b011:
						copyByte1 = getByte(offset);
						offset++;
						for (int i = 0; i < length; i++) {
							output.push_back(copyByte1++);
						}
						break;
					case 0b100:
						address = getByte(offset) + (getByte(offset + 1) << 8);
						if (address >= output.size()) {
							std::cout << "address not within output range!" << std::endl;
							return;
						}
						offset += 2;
						for (int i = 0; i < length; i++) {
							output.push_back(output[address + i]);
						}
						break;
					case 0b101:	// same as 0b100 but the bits are flipped
						address = getByte(offset) + (getByte(offset + 1) << 8);
						if (address >= output.size()) {
							std::cout << "address not within output range!" << std::endl;
							return;
						}
						offset += 2;
						for (int i = 0; i < length; i++) {
							output.push_back( (wxByte) (output[address + i] ^ 0xFF));
						}
						break;
					case 0b110:
						address = output.size() - getByte(offset);
						if (address < 0) {
							std::cout << "address below 0!" << std::endl;
							return;
						}
						offset++;
						for (int i = 0; i < length; i++) {
							output.push_back(output[address + i]);
						}
						break;
					case 0b111:
						command = (currentByte >> 2) & 0b111;
						length = ((currentByte & 0b11) << 8) + getByte(offset) + 1;
						offset++;

						if (command == 0b111) {
							address = output.size() - getByte(offset);
							if (address < 0) {
								std::cout << "address below 0!" << std::endl;
								return;
							}
							offset++;
							for (int i = 0; i < length; i++) {
								output.push_back( (wxByte) (output[address + i] ^ 0xFF));
							}

						} else {
							longLength = true;
						}
						break;
				}
			} while (longLength);
		}
	}

	std::cout << output.size() << std::endl;
	
	for (int i = 0; i < output.size(); i += 2) {
		int r = output[i] & 0x1F;
		int g = ((output[i] & 0xE0) >> 5) | ((output[i + 1] & 0x3) << 3);
		int b = (output[i + 1] & 0x7C) >> 2;

		// Convert the colour to 24bit
		int newR = trunc(float(r)   / float((1 << 5) - 1) * float((1 << 8) - 1) + 0.5f);
		int newG = trunc(float(g) / float((1 << 5) - 1) * float((1 << 8) - 1) + 0.5f);
		int newB = trunc(float(b)  / float((1 << 5) - 1) * float((1 << 8) - 1) + 0.5f);

		palette[i / 2] = wxColour(newR, newG, newB);
	}

	if (output.size() < 256) {
		int remaining = (256 - output.size()) / 2;
		for (int i = 128 - remaining; i < 128; i++) {
			palette[i] = wxColour(0,0,0);
		}
	}
}

bool Rom::compressPalette(wxColour *palette, uint64_t offset) {
	// We start by turning the array of 24bit colours into a byte buffer of 15bit colours
	wxByte paletteBuffer[kPalBytes];
	for (int i = 0; i < kPalSize; i++) {

		// Convert the colour to 15bit
		int newR = trunc(float(palette[i].Red())   / float((1 << 8) - 1) * float((1 << 5) - 1) + 0.5f);
		int newG = trunc(float(palette[i].Green()) / float((1 << 8) - 1) * float((1 << 5) - 1) + 0.5f);
		int newB = trunc(float(palette[i].Blue())  / float((1 << 8) - 1) * float((1 << 5) - 1) + 0.5f);

		// Now we need to store the new colours into the palette buffer by compressing them into a single 15bit value across 2 bytes
		paletteBuffer[i * 2] = newR | (newG << 5);
		paletteBuffer[(i * 2) + 1] = (newG >> 3) | newB << 2;
	}

	// Next we actually compress the data into an output stream, which we then write into the rom at offset
	int byteFillLengths[kPalBytes];
	int wordFillLengths[kPalBytes];
	int byteIncrementLengths[kPalBytes];
	Interval copyLengths[kPalBytes];
	Interval xorCopyLengths[kPalBytes];

	wxVector<wxByte> output;

	int addresses[256][256];

	calcByteFill(byteFillLengths, paletteBuffer);
	calcWordFill(wordFillLengths, paletteBuffer);
	calcByteIncrement(byteIncrementLengths, paletteBuffer);
	calcCopy(copyLengths, paletteBuffer, addresses, wordFillLengths, byteFillLengths);

	int i = 0;
	while (i < kPalBytes) {
		auto length = std::max({byteFillLengths[i], wordFillLengths[i], byteIncrementLengths[i], copyLengths[i].Length});

		if (length < 3) {
			int j = i;
			while ((j < kPalBytes) && (length < 3)) {
				length = std::max({byteFillLengths[j], wordFillLengths[j], byteIncrementLengths[j], copyLengths[j].Length});
				j++;
			}
			length = (j == kPalBytes ? j - i : j - i - 1);
			writeUncompressed(output, i, length, paletteBuffer);

		} else if (length == byteFillLengths[i]) {
			length = std::min(length, 1024);
			writeByteFill(output, paletteBuffer[i], length);
		
		} else if (length == wordFillLengths[i]) {
			length = std::min(length, 1024);
			writeWordFill(output, paletteBuffer[i], paletteBuffer[i + 1], length);
		
		} else if (length == byteIncrementLengths[i]) {
			length = std::min(length, 1024);
			writeByteIncrement(output, paletteBuffer[i], length);
		
		} else if (length == copyLengths[i].Length) {
			length = std::min(length, 1024);
			if ((i - copyLengths[i].Address) < 32) {
				writeNegativeCopy(output, i - copyLengths[i].Address, length);
			
			} else {
				writeCopy(output, copyLengths[i].Address, length);
			}
		}
		i += length;
	}
	output.push_back(0xFF);

	if (output.size() < 0x200) {
		for (int i = 0; i < output.size(); i++) {
			setByte(offset + i, output[i]);
		}
		return true;

	} else {
		std::cout << "palette is too large, not overwriting" << std::endl;
		return false;
	}
}

void Rom::writeChunkHeader(wxVector<wxByte> &output, int type, int length) {
	length--;
	if (length < 32) {
		output.push_back((wxByte) ((type << 5) | length));
	
	} else {
		output.push_back((wxByte) (0b11100000 | (type << 2) | (length >> 8)));
		output.push_back((wxByte) (length & 0xFF));
	}
}

void Rom::writeUncompressed(wxVector<wxByte> &output, int index, int length, wxByte *paletteBuffer) {
	writeChunkHeader(output, 0b000, length);
	for (int i = 0; i < length; i++) {
		output.push_back(paletteBuffer[index + i]);
	}
}

void Rom::writeByteFill(wxVector<wxByte> &output, wxByte b, int length) {
	writeChunkHeader(output, 0b001, length);
	output.push_back(b);
}

void Rom::writeWordFill(wxVector<wxByte> &output, wxByte b1, wxByte b2, int length) {
	writeChunkHeader(output, 0b010, length);
	output.push_back(b1);
	output.push_back(b2);
}

void Rom::writeByteIncrement(wxVector<wxByte> &output, wxByte b, int length) {
	writeChunkHeader(output, 0b011, length);
	output.push_back(b);
}

void Rom::writeCopy(wxVector<wxByte> &output, int address, int length) {
	writeChunkHeader(output, 0b100, length);
	output.push_back((wxByte) (address & 0xFF));
	output.push_back((wxByte) (address >> 8));
}

void Rom::writeXorCopy(wxVector<wxByte> &output, int negOffset, int length) {
	writeChunkHeader(output, 0b101, length);
	output.push_back((wxByte) negOffset);
}

void Rom::writeNegativeCopy(wxVector<wxByte> &output, int address, int length) {
	writeChunkHeader(output, 0b110, length);
	output.push_back((wxByte) address);
}

void Rom::writeNegativeXorCopy(wxVector<wxByte> &output, int negOffset, int length) {
	writeChunkHeader(output, 0b111, length);
	output.push_back((wxByte) negOffset);
}

void Rom::calcByteFill(int *byteFillLengths, wxByte *paletteBuffer) {
	int carry = 0;
	for (int i = 0; i < kPalBytes; i++) {
		if (carry == 0) {
			wxByte value = paletteBuffer[i];
			while (((i + carry) < kPalBytes) && (paletteBuffer[i + carry] == value)) {
				carry++;
			}
		}
		byteFillLengths[i] = carry;
		carry--;
	}
}

void Rom::calcWordFill(int *wordFillLengths, wxByte *paletteBuffer) {
	int carry = 1;
	for (int i = 0; i < kPalBytes - 1; i++) {
		if (carry == 1) {
			wxByte value[2] = {paletteBuffer[i], paletteBuffer[i + 1]};
			while (((i + carry) < kPalBytes) && (paletteBuffer[i + carry] == value[carry & 1])) {
				carry++;
			}
		}
		wordFillLengths[i] = carry;
		carry--;
	}
}

void Rom::calcByteIncrement(int *byteIncrementLengths, wxByte *paletteBuffer) {
	int carry = 0;
	for (int i = 0; i < kPalBytes; i++) {
		if (carry == 0) {
			wxByte value = paletteBuffer[i];
			while (((i + carry) < kPalBytes) && (paletteBuffer[i + carry] == value)) {
				carry++;
				value++;
			}
		}
		byteIncrementLengths[i] = carry;
		carry--;
	}
}

void Rom::calcCopy(Interval *copyLengths, wxByte *paletteBuffer, int addresses[256][256], int *wordFillLengths, int *byteFillLengths) {
	int backReferences[kPalBytes];
	wxVector<Interval> matchLengths;

	for (int i = 0; i < 256; i++) {
		for (int j = 0; j < 256; j++) {
			addresses[i][j] = -1;
		}
	}

	int byte1 = 0;
	int byte2 = 0;

	for (int i = 0; i < kPalBytes - 1;) {
		int fillLength = wordFillLengths[i];
		if (fillLength % 2 == 0) {
			byte1 = paletteBuffer[i];
			byte2 = paletteBuffer[i + 1];
		
		} else {
			byte1 = paletteBuffer[i + 1];
			byte2 = paletteBuffer[i];
		}

		backReferences[i] = addresses[byte1][byte2];
		addresses[byte1][byte2] = i;

		matchLengths.clear();

		int prevAddress = backReferences[i];
		while (prevAddress >= 0) {
			int prevFill = wordFillLengths[prevAddress];
			int l = matchSubSequences(prevAddress + prevFill, i + fillLength, paletteBuffer, wordFillLengths, byteFillLengths);
			matchLengths.push_back(Interval(prevAddress, l));
			prevAddress = backReferences[prevAddress];
		}

		for (int j = 0; j < fillLength - 1; j++) {
			Interval bestMatch = Interval(0, 0);
			for (int n = 0; n < matchLengths.size(); n++) {
				Interval match = Interval(0, 0);
				int matchFill = wordFillLengths[matchLengths[n].Address];
				if (matchFill < fillLength - j) {
					match = Interval(0, 0);
				
				} else {
					match = Interval(matchLengths[n].Address + matchFill - fillLength + j, matchLengths[n].Length + fillLength - j);
				}

				if (match.Length > bestMatch.Length) {
					bestMatch = match;
				}
			}
			copyLengths[i + j] = bestMatch;
		}

		addresses[byte1][byte2] = i;
		i += fillLength - 1;
	}
}

int Rom::matchSubSequences(int a, int b, wxByte *paletteBuffer, int *wordFillLengths, int *byteFillLengths) {
	int bStart = b;

	if ((b >= kPalBytes) || (paletteBuffer[a] != paletteBuffer[b])) {
		return 0;
	}

	if (byteFillLengths[a] != byteFillLengths[b]) {
		return (std::min(byteFillLengths[a], byteFillLengths[b]));
	}

	if (((b + 1) >= kPalBytes) || (paletteBuffer[a + 1] != paletteBuffer[b + 1])) {
		return 1;
	}

	if (wordFillLengths[a] != wordFillLengths[b]) {
		return (std::min(wordFillLengths[a], wordFillLengths[b]));
	}

	int step = std::max(byteFillLengths[a], wordFillLengths[a]);
	a += step;
	b += step;
	while ((b < kPalBytes) && (paletteBuffer[a] == paletteBuffer[b])) {
		a++;
		b++;
	}
	return b - bStart;
}













