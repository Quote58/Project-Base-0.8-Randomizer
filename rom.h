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

struct Interval
{
    int Address;
    int Length;

    Interval(int address, int length) : Address(address), Length(length) {}
    Interval() {
    	Address = 0;
    	Length = 0;
    }

    void Reset()
    {
        Address = 0;
        Length = 0;
    }
};

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

	wxFile *_rom;											// The Rom itself
	wxByte *_dataBuffer;									// A mutable buffer of the rom data
	wxString _name;											// The name of the rom file

	void makeNewRom(wxString fileName);						// Creates a new rom from the data buffer
	wxByte getByte(uint64_t offset);						// Gets a single byte from the rom at offset
	void setByte(uint64_t offset, wxByte byte);				// Sets the byte at offset in the buffer to byte
	void setWord(uint64_t offset, uint16_t word);			// Sets each byte of a word at a given offset in the buffer
	void setBytes(uint64_t offset, wxVector<wxByte> bytes);	// Sets the bytes at offset in the buffer to bytes
	void applyPatch(wxByte *patch);							// Applies an ips patch supplied as a byte buffer to the rom buffer
	void storeColour(uint64_t addr, wxColour clr);

	void decompressPalette(wxColour *palette, uint64_t offset); // Decompresses a given 15bit palette into a 24 bit palette
	bool compressPalette(wxColour *palette, uint64_t offset); 	// Compresses a given 24bit palette into the rom at a given address
	void writeChunkHeader(wxVector<wxByte> &output, int type, int length);
	void writeUncompressed(wxVector<wxByte> &output, int index, int length, wxByte *paletteBuffer);
	void writeByteFill(wxVector<wxByte> &output, wxByte b, int length);
	void writeWordFill(wxVector<wxByte> &output, wxByte b1, wxByte b2, int length);
	void writeByteIncrement(wxVector<wxByte> &output, wxByte b, int length);
	void writeCopy(wxVector<wxByte> &output, int address, int length);
	void writeXorCopy(wxVector<wxByte> &output, int negOffset, int length);
	void writeNegativeCopy(wxVector<wxByte> &output, int address, int length);
	void writeNegativeXorCopy(wxVector<wxByte> &output, int negOffset, int length);
	void calcByteFill(int *byteFillLengths, wxByte *paletteBuffer);
	void calcWordFill(int *wordFillLengths, wxByte *paletteBuffer);
	void calcByteIncrement(int *byteIncrementLengths, wxByte *paletteBuffer);
	void calcCopy(Interval *copyLengths, wxByte *paletteBuffer, int addresses[256][256], int *wordFillLengths, int *byteFillLengths);
	int matchSubSequences(int a, int b, wxByte *paletteBuffer, int *wordFillLengths, int *byteFillLengths);
};

#endif