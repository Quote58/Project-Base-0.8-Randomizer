#ifndef RANDO_ROM_H
#define RANDO_ROM_H

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
 
#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

#include <wx/vector.h>
#include <wx/wfstream.h>
#include <wx/filename.h>

/* Hexer Rom handler
 * This class handles the actual I/O
 * for the rom being edited
 */
class Rom {
public:
	Rom(wxString path);
	~Rom() {
		delete _rom;
		delete _dataBuffer;
	}

	wxFile *_rom;										// The Rom itself
	wxByte *_dataBuffer;								// A mutable buffer of the rom data
	wxString _name;										// The name of the rom file

	void makeNewRom(wxString fileName);					// Creates a new rom from the data buffer
	wxByte getByte(int offset);							// Gets a single byte from the rom at offset
	void setByte(int offset, wxByte byte);				// Sets the byte at offset in the buffer to byte
	void setWord(int offset, uint16_t word);			// Sets each byte of a word at a given offset in the buffer
	void setBytes(int offset, wxVector<wxByte> bytes);	// Sets the bytes at offset in the buffer to bytes
	void applyPatch(wxByte *patch);						// Applies an ips patch supplied as a byte buffer to the rom buffer
	void mountUndoCodeRead();
};

#endif