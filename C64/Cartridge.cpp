/*
 * (C) 2009 A. Carl Douglas. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <ctype.h> // for printReadable only
#include "Cartridge.h"

static void printReadable(const void *data, int length)
{
	int i;
	for(i = 0; i < length; i++) {
		char ch = ((char*)(data))[i];
		if (isascii(ch)) {
			printf("%02x %c ", ch, ch);
		} else {
			printf("%02x ? ", ch);
		}
		if (i > 0 && i % 16 == 0) {
			printf("\n");
		}
	}
}

//! Constructor
Cartridge::Cartridge()
{
	path = NULL;
	data = NULL;
	size = 0;
	
	numberOfChips = 0;
	for(int i = 0; i < MAX_CHIPS; i++) {
		chip[i] = NULL;
	}
}

//! Destructor
Cartridge::~Cartridge()
{
	if (path) free(path);
	if (data) free(data);
}

bool Cartridge::load(uint8_t **buffer)
{	
#if 0
	for (unsigned i = 0; i < sizeof(watchpoint); i++)
		watchpoint[i] = read8(buffer);	
	for (unsigned i = 0; i < sizeof(watchValue); i++) 
		watchValue[i] = read8(buffer);	
#endif
	return true;
}

bool Cartridge::save(uint8_t **buffer) 
{
#if 0
	for (unsigned i = 0; i < sizeof(watchpoint); i++)
		write8(buffer, watchpoint[i]);
	for (unsigned i = 0; i < sizeof(watchValue); i++) 
		write8(buffer, watchValue[i]);
#endif
	return true;
}

void Cartridge::reset()
{
	
}

//! Check file type
/*! Returns true, iff the specifies file is a valid cartridge file. */
bool Cartridge::fileIsValid(const char *filename)
{
	int magic_bytes[] = 
		{'C','6','4',' ','C','A','R','T','R','I','D','G','E',' ',' ',' ',EOF};
	
	assert(filename != NULL);
	
	if (!checkFileSuffix(filename, ".CRT") && !checkFileSuffix(filename, ".crt"))
		return false;
	
	if (!checkFileSize(filename, 0x40, -1))
		return false;
	
	if (!checkFileHeader(filename, magic_bytes))
		return false;
	
	return true;
}

bool Cartridge::loadFile(const char *filename)
{
	struct stat fileProperties;
	FILE *file = NULL;
	int c = 0;
	unsigned i = 0;
	
	assert (filename != NULL);
	
	// Check file type
	if (!fileIsValid(filename)) 
		return false;
	
	// Get file properties
    if (stat(filename, &fileProperties) != 0) {
		// Could not open file...
		return false;
	}
	
	// Open file
	if (!(file = fopen(filename, "r"))) {
		// Can't open for read (Huh?)
		return false;
	}
	
	// Allocate memory
	if ((data = (uint8_t *)malloc(fileProperties.st_size)) == NULL) {
		return false;
	}
	
	// Read data
	c = fgetc(file);
	while(c != EOF) {
		data[size++] = c;
		c = fgetc(file);
	}
	
	assert(size == fileProperties.st_size);
	
	fclose(file);
	
	// C64 CARTRIDGE
	uint8_t magic_cartridge[] = { 'C', '6', '4', ' ', 'C', 'A', 'R', 'T', 'R', 'I', 'D', 'G', 'E', ' ', ' ', ' ' };
	if (memcmp(magic_cartridge, &data[0], 16) != 0) {
		printf("Bad cartridge signature. Expected 'C64  CARTRIDGE  ', got ...\n");
		printReadable(&data[0], 16);
		return false;
	}
	
	// Cartridge header size
	uint16_t hi = LO_HI(data[0x0011], data[0x0010]);
	uint16_t lo  = LO_HI(data[0x0013], data[0x0012]);
	uint32_t headerSize = (uint32_t)((hi << 16) | (lo));

	version = LO_HI(data[0x0015], data[0x0014]);
	type = LO_HI(data[0x0017], data[0x0016]);

	printf ("Header size is: 0x%08X (normally 0x40)\n", headerSize);
	printf("Cartridge: %s, type: %d\n", &data[0x0020], type);
	printf("EXROM = %d, GAME = %d\n", data[0x0019], data[0x0018]);
	
	path = strdup(filename);
	
	// Load chip packets
	for (i = headerSize; i < size; ) {

		uint8_t magic_chip[] = { 'C', 'H', 'I', 'P' };
		if (memcmp(magic_chip, &data[i], 4) != 0) {
			printf("Unexpected data in cartridge, expected 'CHIP'\n");
			printReadable(&data[i], 4);
			return false;
		}
		
		// Check the size of the rom contained in the chip packet
		unsigned int romSize = LO_HI(data[i+0x000F], data[i+0x000E]);
		unsigned int chipType = LO_HI(data[i+0x0009], data[i+0x0008]);
		unsigned int bank = LO_HI(data[i+0x000B], data[i+0x000A]);
		unsigned int addr = LO_HI(data[i+0x000D], data[i+0x000C]);
		
		printf("Chip ROM size: 0x%04x (%d)  type: %d  Bank: 0x%04x  Address: 0x%04x\n", 
			   romSize, romSize, chipType, bank, addr );
		
		Cartridge::Chip * chipPacket = new Cartridge::Chip();
		chipPacket->loadAddress = addr;
		chipPacket->size = romSize;
		
		if (romSize > sizeof(chipPacket->rom)) {
			printf("Chip Rom too large\n");
			return false;
		}
		memcpy(chipPacket->rom, &data[i+0x0010], romSize);
		
		chip[numberOfChips++] = chipPacket;
		
		i += 0x0010 + romSize;
	}
	
	if(numberOfChips > 0) {
		switchBank(0);
	}
	
	printf("CRT container imported successfully (%d chips)\n", numberOfChips);
	
	return true;
}

//! The GAME line status
bool Cartridge::gameIsHigh()
{
	//	0019 - Cartridge port GAME line status
	//	0 - inactive
	//	1 - active
	return data[0x19] != 0;
}

//! The EXROM line status
bool Cartridge::exromIsHigh()
{
	//	0018 - Cartridge port EXROM line status
	//	0 - inactive
	//	1 - active
	return data[0x18] != 0;
}

void Cartridge::switchBank(int bankNumber)
{
	printf("Switching to bank %d ... ", bankNumber);
	Cartridge::Chip *chip = getChip(bankNumber);
	if (chip != NULL) {
		memcpy(&rom[chip->loadAddress], chip->rom, chip->size);
		printf("Banked %d bytes to 0x%04x", chip->size, chip->loadAddress);
	} else {
		printf("Bank %d does not exist\n", bankNumber);
	}
	printf("\n");
}

void Cartridge::poke(uint16_t addr, uint8_t value)             
{
	printf("Cartridge poke %04X %02x (%d)\n", addr, value, value);
	// 0xDE00 - 0xDEFF (I/O area 1)
	// 0xDF00 - 0xDFFF (I/O area 2) 
	if (addr >= 0xDE00 && addr <= 0xDFFF) {
		// For some cartridges (e.g. Ocean .crt type 5):
		// Bank switching is done by writing to $DE00. The lower six bits give the
		// bank number (ranging from 0-63). Bit 8 in this selection word is always
		// set.
		// When this occurs, the cartridge will present the selected bank
		// at the specified ROM locations.
		rom[addr] = value;
		
		Cartridge::Type type = getType();
		if (type == Normal_Cartridge) {
			
		} else if (type == Simons_Basic && addr == 0xDE00) {
			printf("Switching to bank %d (%02X) ... ", 1, value);
			// Simon banks the second chip into $A000-BFFF
			if (value == 0x01) {
				switchBank(1);
			} else {
				// $A000-BFFF is additional RAM
			}
		} else if (type == C64_Game_System_System_3) {
			uint8_t bankNumber = addr - 0xDE00;
			switchBank(bankNumber);
		} else if (type == Ocean_type_1) {
			uint8_t bankNumber = value & 0x3F;
			switchBank(bankNumber);
		} else if (type == Fun_Play_Power_Play) {
				
		} else {
			printf("Unsupported cartridge\n");
		}
	}
}

uint8_t Cartridge::peek(uint16_t addr)
{
	return rom[addr];
} 

bool Cartridge::isRomAddr(uint16_t addr)
{
	if (0x8000 <= addr && addr <= 0x9FFF) {
		return is8k() || is16k();
	} else if (0xA000 <= addr && addr <= 0xBFFF) {
		return is16k();
	} else if (0xE000 <= addr) {
		return isUltimax();
	}
	return false;
}

unsigned int Cartridge::getVersion()
{
	return version;
}

Cartridge::Type Cartridge::getType()
{
	return (Cartridge::Type)type;
}

// --------------------------------------------------------------------------------
//                       Cartridge Chip methods
// --------------------------------------------------------------------------------

Cartridge::Chip *Cartridge::getChip(unsigned index)
{
	if (index >= 0 && index < numberOfChips) {
		return chip[index];
	}
	return NULL;
}

Cartridge::Chip::Chip()
{
	size = 0;
}

Cartridge::Chip::~Chip()
{
	
}

