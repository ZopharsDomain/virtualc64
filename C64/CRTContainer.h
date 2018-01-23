/*!
 * @header      CRTContainer.h
 * @author      Written by Dirk Hoffmann based on the original code by A. Carl Douglas.
 * @copyright   All rights reserved.
 */
/*
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

/* For details about the .CRT format,
 * see: http://ist.uwaterloo.ca/~schepers/formats/CRT.TXT
 *
 * As well read the Commodore 64 Programmers Reference Guide pages 260-267.
 */

#ifndef _CRTContainer_H
#define _CRTContainer_H

#include "Container.h"

//! @brief    Cartridge types
/*! @seealso  Cartridge::isSupportedType
 */
enum CartridgeType : uint16_t {
    CRT_NORMAL = 0,
    CRT_ACTION_REPLAY = 1,
    CRT_KCS_POWER_CARTRIDGE = 2,
    CRT_FINAL_CARTRIDGE_III = 3,
    CRT_SIMONS_BASIC = 4,
    CRT_OCEAN_TYPE_1 = 5,
    CRT_EXPORT_CARTRIDGE = 6,
    CRT_FUN_PLAY_POWER_PLAY = 7,
    CRT_SUPER_GAMES = 8,
    CRT_ATOMIC_POWER = 9,
    CRT_EPYX_FASTLOAD = 10,
    CRT_WESTERMANN_LEARNING = 11,
    CRT_REX_UTILITY = 12,
    CRT_FINAL_CARTRIDGE_I = 13,
    CRT_MAGIC_FORMEL = 14,
    CRT_C64_GAME_SYSTEM_SYSTEM_3 = 15,
    CRT_WARPSPEED = 16,
    CRT_DINAMIC = 17,
    CRT_ZAXXON_SUPER_ZAXXON = 18,
    CRT_MAGIC_DESK_DOMARK_HES_AUSTRALIA = 19,
    CRT_SUPER_SNAPSHOT_5 = 20,
    CRT_COMAL = 21,
    CRT_STRUCTURE_BASIC = 22,
    CRT_ROSS = 23,
    CRT_DELA_EP64 = 24,
    CRT_DELA_EP7x8 = 25,
    CRT_DELA_EP256 = 26,
    CRT_REX_EP256 = 27,
    CRT_NONE = 255
};

/*! @class    CRTContainer
 *  @brief    The CRTContainer class declares the programmatic interface for a file
 *            of the CRT format type.
 */
class CRTContainer : public Container {
    
private:
    
    //! @brief    Raw data of CRT container file
    uint8_t *data;
    
    //! @brief    Number of chips contained in cartridge file
    unsigned int numberOfChips;
    
    //! @brief    Indicates where each chip section starts
    uint8_t *chips[64];

public:
    
    //! @brief    Constructor
    CRTContainer();
    
    //! @brief    Destructor
    ~CRTContainer();
    
    //! @brief    Frees the allocated memory.
    void dealloc();
    
    //! @brief    Returns the container type
    ContainerType getType() { return CRT_CONTAINER; }
    
    //! @brief    Type of container in plain text
    const char *getTypeAsString() { return "CRT"; }
    
    //! Returns true of filename points to a valid file of that type
    static bool isCRTFile(const char *filename);
    
    //! Check file type
    bool fileIsValid(const char *filename) { return CRTContainer::isCRTFile(filename); }

    //! Factory method
    static CRTContainer *containerFromCRTFile(const char *filename);
    
    //! Read container data from memory buffer
    bool readFromBuffer(const uint8_t *buffer, unsigned length);

    //
    // Cartridge information
    //
    
    //! Return logical cartridge name
    char *getCartridgeName() { return (char *)&data[0x20]; }
    
    //! Return cartridge version number
    uint16_t getCartridgeVersion() { return LO_HI(data[0x15], data[0x14]); }
    
    //! Return cartridge type
    CartridgeType getCartridgeType() { return CartridgeType(LO_HI(data[0x17], data[0x16])); }

    //! Return cartridge type
    const char *getCartridgeTypeName();

    //! Return exrom line
    bool getExromLine() { return data[0x18] != 0; }

    //! Return game line
    bool getGameLine() { return data[0x19] != 0; }
    

    //
    // Chip information
    //
    
    //! Return how many chips are contained in this cartridge
    uint8_t getNumberOfChips() { return numberOfChips; }
    
    //! Return start address of chip data
    uint8_t *getChipData(unsigned nr) { return chips[nr]+0x10; }
    
    //! Return size of chip (8 KB or 16 KB)
    uint16_t getChipSize(unsigned nr) { return LO_HI(chips[nr][0xF], chips[nr][0xE]); }
    
    //! Return type of chip
    uint16_t getChipType(unsigned nr) { return LO_HI(chips[nr][0x9], chips[nr][0x8]); }
    
    //! Return bank information
    uint16_t getChipBank(unsigned nr) { return LO_HI(chips[nr][0xB], chips[nr][0xA]); }
    
    //! Returns start of chip rom in address space
    uint16_t getChipAddr(unsigned nr) { return LO_HI(chips[nr][0xD], chips[nr][0xC]); }
};

#endif