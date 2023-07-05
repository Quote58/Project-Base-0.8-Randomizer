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

wxByte Rom::getByte(int offset) {
	if (offset < _rom->Length()) {
		return _dataBuffer[offset];

	} else {
		std::cout << "invalid offset! Can't access offset, returning 0xFF instead " << offset << std::endl;
		return 0xFF;
	}
}

void Rom::setByte(int offset, wxByte byte) {
	if (offset >= _rom->Length()) {
		std::cout << "invalid offset! Can't access offset " << offset << std::endl;
		return;
	}

	// Set the byte at the offset in the data buffer to the given byte
	_dataBuffer[offset] = byte;
}

void Rom::setWord(int offset, uint16_t word) {
	if (offset >= _rom->Length()) {
		std::cout << "invalid offset! Can't access offset " << offset << std::endl;
		return;
	}

	// A word is a uint16, so we take each half and set it to the data buffer
	_dataBuffer[offset] = word & 0x00FF;
	_dataBuffer[offset + 1] = (word & 0xFF00) >> 8;
}

void Rom::setBytes(int offset, wxVector<wxByte> bytes) {
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
		int index = 5;
		while ((patch[index] != 'E') && (patch[index + 1] != 'O') && (patch[index + 2] != 'F')) {
			int currentOffset = ((int) (patch[index] << 16)) | ((int) (patch[index + 1] << 8)) | ((int) patch[index + 2]);
			index += 3;
			int currentSize = ((uint16_t) (patch[index] << 8)) | ((uint16_t) patch[index + 1]);
			index += 2;
			if (currentSize == 0) {
				// RLE
				int rleSize = ((uint16_t) (patch[index] << 8)) | ((uint16_t) patch[index + 1]);
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

void Rom::mountUndoCodeRead() {
	wxLogError(wxString("This is a great name for a function"));
}
















