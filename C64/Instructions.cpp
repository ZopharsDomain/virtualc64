/*
 * (C) 2006 - 2018 Dirk W. Hoffmann. All rights reserved.
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

#include "C64.h"

void
CPU::adc(uint8_t op)
{
    if (getD())
        adc_bcd(op);
    else
        adc_binary(op);
}

void
CPU::adc_binary(uint8_t op)
{
    uint16_t sum = A + op + (getC() ? 1 : 0);
    
    setC(sum > 255);
    setV(!((A ^ op) & 0x80) && ((A ^ sum) & 0x80));
    loadA((uint8_t)sum);
}

void
CPU::adc_bcd(uint8_t op)
{
    uint16_t sum       = A + op + (getC() ? 1 : 0);
    uint8_t  highDigit = (A >> 4) + (op >> 4);
    uint8_t  lowDigit  = (A & 0x0F) + (op & 0x0F) + (getC() ? 1 : 0);
    
    // Check for overflow conditions
    // If an overflow occurs on a BCD digit, it needs to be fixed by adding the pseudo-tetrade 0110 (=6)
    if (lowDigit > 9) {
        lowDigit = lowDigit + 6;
    }
    if (lowDigit > 0x0F) {
        highDigit++;
    }
    
    setZ((sum & 0xFF) == 0);
    setN(highDigit & 0x08);
    setV((((highDigit << 4) ^ A) & 0x80) && !((A ^ op) & 0x80));
    
    if (highDigit > 9) {
        highDigit = (highDigit + 6);
    }
    if (highDigit > 0x0F) {
        setC(1);
    } else {
        setC(0);
    }
    
    lowDigit &= 0x0F;
    A = (uint8_t)((highDigit << 4) | lowDigit);
}

void
CPU::cmp(uint8_t op1, uint8_t op2)
{
    uint8_t tmp = op1 - op2;
    
    setC(op1 >= op2);
    setN(tmp & 128);
    setZ(tmp == 0);
}

void
CPU::sbc(uint8_t op)
{
    if (getD())
        sbc_bcd(op);
    else
        sbc_binary(op);
}

void
CPU::sbc_binary(uint8_t op)
{
    uint16_t sum = A - op - (getC() ? 0 : 1);
    
    setC(sum <= 255);
    setV(((A ^ sum) & 0x80) && ((A ^ op) & 0x80));
    loadA((uint8_t)sum);
}

void
CPU::sbc_bcd(uint8_t op)
{
    uint16_t sum       = A - op - (getC() ? 0 : 1);
    uint8_t  highDigit = (A >> 4) - (op >> 4);
    uint8_t  lowDigit  = (A & 0x0F) - (op & 0x0F) - (getC() ? 0 : 1);
    
    // Check for underflow conditions
    // If an overflow occurs on a BCD digit, it needs to be fixed by subtracting the pseudo-tetrade 0110 (=6)
    if (lowDigit & 0x10) {
        lowDigit = lowDigit - 6;
        highDigit--;
    }
    if (highDigit & 0x10) {
        highDigit = highDigit - 6;
    }
    
    setC(sum < 0x100);
    setV(((A ^ sum) & 0x80) && ((A ^ op) & 0x80));
    setZ((sum & 0xFF) == 0);
    setN(sum & 0x80);
    
    A = (uint8_t)((highDigit << 4) | (lowDigit & 0x0f));
}

void 
CPU::registerCallback(uint8_t opcode, MicroInstruction mInstr)
{
	registerCallback(opcode, "???", ADDR_IMPLIED, mInstr);
}

void 
CPU::registerCallback(uint8_t opcode, const char *mnc,
                      AddressingMode mode, MicroInstruction mInstr)
{
	// table is write once!
	if (mInstr != JAM)
		assert(actionFunc[opcode] == JAM);
	
    mnemonic[opcode] = mnc;
    addressingMode[opcode] = mode;
	actionFunc[opcode] = mInstr;
}

void 
CPU::registerIllegalInstructions()
{
	registerCallback(0x93, "SHA*", ADDR_INDIRECT_Y, SHA_indirect_y);
	registerCallback(0x9F, "SHA*", ADDR_ABSOLUTE_Y, SHA_absolute_y);

	registerCallback(0x4B, "ALR*", ADDR_IMMEDIATE, ALR_immediate);

	registerCallback(0x0B, "ANC*", ADDR_IMMEDIATE, ANC_immediate);
	registerCallback(0x2B, "ANC*", ADDR_IMMEDIATE, ANC_immediate);
	
	registerCallback(0x8B, "ANE*", ADDR_IMMEDIATE, ANE_immediate);

	registerCallback(0x6B, "ARR*", ADDR_IMMEDIATE, ARR_immediate);
	registerCallback(0xCB, "AXS*", ADDR_IMMEDIATE, AXS_immediate);

	registerCallback(0xC7, "DCP*", ADDR_ZERO_PAGE, DCP_zero_page);
	registerCallback(0xD7, "DCP*", ADDR_ZERO_PAGE_X, DCP_zero_page_x);
	registerCallback(0xC3, "DCP*", ADDR_INDIRECT_X, DCP_indirect_x);
	registerCallback(0xD3, "DCP*", ADDR_INDIRECT_Y, DCP_indirect_y);
	registerCallback(0xCF, "DCP*", ADDR_ABSOLUTE, DCP_absolute);
	registerCallback(0xDF, "DCP*", ADDR_ABSOLUTE_X, DCP_absolute_x);
	registerCallback(0xDB, "DCP*", ADDR_ABSOLUTE_Y, DCP_absolute_y);

	registerCallback(0xE7, "ISC*", ADDR_ZERO_PAGE, ISC_zero_page);
	registerCallback(0xF7, "ISC*", ADDR_ZERO_PAGE_X, ISC_zero_page_x);
	registerCallback(0xE3, "ISC*", ADDR_INDIRECT_X, ISC_indirect_x);
	registerCallback(0xF3, "ISC*", ADDR_INDIRECT_Y, ISC_indirect_y);
	registerCallback(0xEF, "ISC*", ADDR_ABSOLUTE, ISC_absolute);
	registerCallback(0xFF, "ISC*", ADDR_ABSOLUTE_X, ISC_absolute_x);
	registerCallback(0xFB, "ISC*", ADDR_ABSOLUTE_Y, ISC_absolute_y);

	registerCallback(0xBB, "LAS*", ADDR_ABSOLUTE_Y, LAS_absolute_y);

	registerCallback(0xA7, "LAX*", ADDR_ZERO_PAGE, LAX_zero_page);
	registerCallback(0xB7, "LAX*", ADDR_ZERO_PAGE_Y, LAX_zero_page_y);
	registerCallback(0xA3, "LAX*", ADDR_INDIRECT_X, LAX_indirect_x);
	registerCallback(0xB3, "LAX*", ADDR_INDIRECT_Y, LAX_indirect_y);
	registerCallback(0xAF, "LAX*", ADDR_ABSOLUTE, LAX_absolute);
	registerCallback(0xBF, "LAX*", ADDR_ABSOLUTE_Y, LAX_absolute_y);

	registerCallback(0xAB, "LXA*", ADDR_IMMEDIATE, LXA_immediate);

	registerCallback(0x80, "NOP*", ADDR_IMMEDIATE, NOP_immediate);
	registerCallback(0x82, "NOP*", ADDR_IMMEDIATE, NOP_immediate);
	registerCallback(0x89, "NOP*", ADDR_IMMEDIATE, NOP_immediate);
	registerCallback(0xC2, "NOP*", ADDR_IMMEDIATE, NOP_immediate);
	registerCallback(0xE2, "NOP*", ADDR_IMMEDIATE, NOP_immediate);
	registerCallback(0x1A, "NOP*", ADDR_IMPLIED, NOP);
	registerCallback(0x3A, "NOP*", ADDR_IMPLIED, NOP);
	registerCallback(0x5A, "NOP*", ADDR_IMPLIED, NOP);
	registerCallback(0x7A, "NOP*", ADDR_IMPLIED, NOP);
	registerCallback(0xDA, "NOP*", ADDR_IMPLIED, NOP);
	registerCallback(0xFA, "NOP*", ADDR_IMPLIED, NOP);
	registerCallback(0x04, "NOP*", ADDR_ZERO_PAGE, NOP_zero_page);
	registerCallback(0x44, "NOP*", ADDR_ZERO_PAGE, NOP_zero_page);
	registerCallback(0x64, "NOP*", ADDR_ZERO_PAGE, NOP_zero_page);
	registerCallback(0x0C, "NOP*", ADDR_ABSOLUTE, NOP_absolute);
	registerCallback(0x14, "NOP*", ADDR_ZERO_PAGE_X, NOP_zero_page_x);
	registerCallback(0x34, "NOP*", ADDR_ZERO_PAGE_X, NOP_zero_page_x);
	registerCallback(0x54, "NOP*", ADDR_ZERO_PAGE_X, NOP_zero_page_x);
	registerCallback(0x74, "NOP*", ADDR_ZERO_PAGE_X, NOP_zero_page_x);
	registerCallback(0xD4, "NOP*", ADDR_ZERO_PAGE_X, NOP_zero_page_x);
	registerCallback(0xF4, "NOP*", ADDR_ZERO_PAGE_X, NOP_zero_page_x);
	registerCallback(0x1C, "NOP*", ADDR_ABSOLUTE_X, NOP_absolute_x);
	registerCallback(0x3C, "NOP*", ADDR_ABSOLUTE_X, NOP_absolute_x);
	registerCallback(0x5C, "NOP*", ADDR_ABSOLUTE_X, NOP_absolute_x);
	registerCallback(0x7C, "NOP*", ADDR_ABSOLUTE_X, NOP_absolute_x);
	registerCallback(0xDC, "NOP*", ADDR_ABSOLUTE_X, NOP_absolute_x);
	registerCallback(0xFC, "NOP*", ADDR_ABSOLUTE_X, NOP_absolute_x);

	registerCallback(0x27, "RLA*", ADDR_ZERO_PAGE, RLA_zero_page);
	registerCallback(0x37, "RLA*", ADDR_ZERO_PAGE_X, RLA_zero_page_x);
	registerCallback(0x23, "RLA*", ADDR_INDIRECT_X, RLA_indirect_x);
	registerCallback(0x33, "RLA*", ADDR_INDIRECT_Y, RLA_indirect_y);
	registerCallback(0x2F, "RLA*", ADDR_ABSOLUTE, RLA_absolute);
	registerCallback(0x3F, "RLA*", ADDR_ABSOLUTE_X, RLA_absolute_x);
	registerCallback(0x3B, "RLA*", ADDR_ABSOLUTE_Y, RLA_absolute_y);

	registerCallback(0x67, "RRA*", ADDR_ZERO_PAGE, RRA_zero_page);
	registerCallback(0x77, "RRA*", ADDR_ZERO_PAGE_X, RRA_zero_page_x);
	registerCallback(0x63, "RRA*", ADDR_INDIRECT_X, RRA_indirect_x);
	registerCallback(0x73, "RRA*", ADDR_INDIRECT_Y, RRA_indirect_y);
	registerCallback(0x6F, "RRA*", ADDR_ABSOLUTE, RRA_absolute);
	registerCallback(0x7F, "RRA*", ADDR_ABSOLUTE_X, RRA_absolute_x);
	registerCallback(0x7B, "RRA*", ADDR_ABSOLUTE_Y, RRA_absolute_y);

	registerCallback(0x87, "SAX*", ADDR_ZERO_PAGE, SAX_zero_page);
	registerCallback(0x97, "SAX*", ADDR_ZERO_PAGE_Y, SAX_zero_page_y);
	registerCallback(0x83, "SAX*", ADDR_INDIRECT_X, SAX_indirect_x);
	registerCallback(0x8F, "SAX*", ADDR_ABSOLUTE, SAX_absolute);

	registerCallback(0xEB, "SBC*", ADDR_IMMEDIATE, SBC_immediate);

	registerCallback(0x9E, "SHX*", ADDR_ABSOLUTE_Y, SHX_absolute_y);
	registerCallback(0x9C, "SHY*", ADDR_ABSOLUTE_X, SHY_absolute_x);

	registerCallback(0x07, "SLO*", ADDR_ZERO_PAGE, SLO_zero_page);
	registerCallback(0x17, "SLO*", ADDR_ZERO_PAGE_X, SLO_zero_page_x);
	registerCallback(0x03, "SLO*", ADDR_INDIRECT_X, SLO_indirect_x);
	registerCallback(0x13, "SLO*", ADDR_INDIRECT_Y, SLO_indirect_y);
	registerCallback(0x0F, "SLO*", ADDR_ABSOLUTE, SLO_absolute);
	registerCallback(0x1F, "SLO*", ADDR_ABSOLUTE_X, SLO_absolute_x);
	registerCallback(0x1B, "SLO*", ADDR_ABSOLUTE_Y, SLO_absolute_y);

	registerCallback(0x47, "SRE*", ADDR_ZERO_PAGE, SRE_zero_page);
	registerCallback(0x57, "SRE*", ADDR_ZERO_PAGE_X, SRE_zero_page_x);
	registerCallback(0x43, "SRE*", ADDR_INDIRECT_X, SRE_indirect_x);
	registerCallback(0x53, "SRE*", ADDR_INDIRECT_Y, SRE_indirect_y);
	registerCallback(0x4F, "SRE*", ADDR_ABSOLUTE, SRE_absolute);
	registerCallback(0x5F, "SRE*", ADDR_ABSOLUTE_X, SRE_absolute_x);
	registerCallback(0x5B, "SRE*", ADDR_ABSOLUTE_Y, SRE_absolute_y);
	
	registerCallback(0x9B, "TAS*", ADDR_ABSOLUTE_Y, TAS_absolute_y);
}

	
void CPU::registerInstructions()
{
	for (int i=0; i<256; i++)
		registerCallback(i, JAM);

	registerCallback(0x69, "ADC", ADDR_IMMEDIATE, ADC_immediate);
	registerCallback(0x65, "ADC", ADDR_ZERO_PAGE, ADC_zero_page);
	registerCallback(0x75, "ADC", ADDR_ZERO_PAGE_X, ADC_zero_page_x);
	registerCallback(0x6D, "ADC", ADDR_ABSOLUTE, ADC_absolute);
	registerCallback(0x7D, "ADC", ADDR_ABSOLUTE_X, ADC_absolute_x);
	registerCallback(0x79, "ADC", ADDR_ABSOLUTE_Y, ADC_absolute_y);
	registerCallback(0x61, "ADC", ADDR_INDIRECT_X, ADC_indirect_x);
	registerCallback(0x71, "ADC", ADDR_INDIRECT_Y, ADC_indirect_y);

	registerCallback(0x29, "AND", ADDR_IMMEDIATE, AND_immediate);
	registerCallback(0x25, "AND", ADDR_ZERO_PAGE, AND_zero_page);
	registerCallback(0x35, "AND", ADDR_ZERO_PAGE_X, AND_zero_page_x);
	registerCallback(0x2D, "AND", ADDR_ABSOLUTE, AND_absolute);
	registerCallback(0x3D, "AND", ADDR_ABSOLUTE_X, AND_absolute_x);
	registerCallback(0x39, "AND", ADDR_ABSOLUTE_Y, AND_absolute_y);
	registerCallback(0x21, "AND", ADDR_INDIRECT_X, AND_indirect_x);
	registerCallback(0x31, "AND", ADDR_INDIRECT_Y, AND_indirect_y);
	
	registerCallback(0x0A, "ASL", ADDR_ACCUMULATOR, ASL_accumulator);
	registerCallback(0x06, "ASL", ADDR_ZERO_PAGE, ASL_zero_page);
	registerCallback(0x16, "ASL", ADDR_ZERO_PAGE_X, ASL_zero_page_x);
	registerCallback(0x0E, "ASL", ADDR_ABSOLUTE, ASL_absolute);
	registerCallback(0x1E, "ASL", ADDR_ABSOLUTE_X, ASL_absolute_x);
	
	registerCallback(0x90, "BCC", ADDR_RELATIVE, BCC_relative);
	registerCallback(0xB0, "BCS", ADDR_RELATIVE, BCS_relative);
	registerCallback(0xF0, "BEQ", ADDR_RELATIVE, BEQ_relative);

	registerCallback(0x24, "BIT", ADDR_ZERO_PAGE, BIT_zero_page);
	registerCallback(0x2C, "BIT", ADDR_ABSOLUTE, BIT_absolute);
	
	registerCallback(0x30, "BMI", ADDR_RELATIVE, BMI_relative);
	registerCallback(0xD0, "BNE", ADDR_RELATIVE, BNE_relative);
	registerCallback(0x10, "BPL", ADDR_RELATIVE, BPL_relative);
	registerCallback(0x00, "BRK", ADDR_IMPLIED, BRK);
	registerCallback(0x50, "BVC", ADDR_RELATIVE, BVC_relative);
	registerCallback(0x70, "BVS", ADDR_RELATIVE, BVS_relative);

	registerCallback(0x18, "CLC", ADDR_IMPLIED, CLC);
	registerCallback(0xD8, "CLD", ADDR_IMPLIED, CLD);
	registerCallback(0x58, "CLI", ADDR_IMPLIED, CLI);
	registerCallback(0xB8, "CLV", ADDR_IMPLIED, CLV);

	registerCallback(0xC9, "CMP", ADDR_IMMEDIATE, CMP_immediate);
	registerCallback(0xC5, "CMP", ADDR_ZERO_PAGE, CMP_zero_page);
	registerCallback(0xD5, "CMP", ADDR_ZERO_PAGE_X, CMP_zero_page_x);
	registerCallback(0xCD, "CMP", ADDR_ABSOLUTE, CMP_absolute);
	registerCallback(0xDD, "CMP", ADDR_ABSOLUTE_X, CMP_absolute_x);
	registerCallback(0xD9, "CMP", ADDR_ABSOLUTE_Y, CMP_absolute_y);
	registerCallback(0xC1, "CMP", ADDR_INDIRECT_X, CMP_indirect_x);
	registerCallback(0xD1, "CMP", ADDR_INDIRECT_Y, CMP_indirect_y);

	registerCallback(0xE0, "CPX", ADDR_IMMEDIATE, CPX_immediate);
	registerCallback(0xE4, "CPX", ADDR_ZERO_PAGE, CPX_zero_page);
	registerCallback(0xEC, "CPX", ADDR_ABSOLUTE, CPX_absolute);

	registerCallback(0xC0, "CPY", ADDR_IMMEDIATE, CPY_immediate);
	registerCallback(0xC4, "CPY", ADDR_ZERO_PAGE, CPY_zero_page);
	registerCallback(0xCC, "CPY", ADDR_ABSOLUTE, CPY_absolute);

	registerCallback(0xC6, "DEC", ADDR_ZERO_PAGE, DEC_zero_page);
	registerCallback(0xD6, "DEC", ADDR_ZERO_PAGE_X, DEC_zero_page_x);
	registerCallback(0xCE, "DEC", ADDR_ABSOLUTE, DEC_absolute);
	registerCallback(0xDE, "DEC", ADDR_ABSOLUTE_X, DEC_absolute_x);

	registerCallback(0xCA, "DEX", ADDR_IMPLIED, DEX);
	registerCallback(0x88, "DEY", ADDR_IMPLIED, DEY);
	
	registerCallback(0x49, "EOR", ADDR_IMMEDIATE, EOR_immediate);
	registerCallback(0x45, "EOR", ADDR_ZERO_PAGE, EOR_zero_page);
	registerCallback(0x55, "EOR", ADDR_ZERO_PAGE_X, EOR_zero_page_x);
	registerCallback(0x4D, "EOR", ADDR_ABSOLUTE, EOR_absolute);
	registerCallback(0x5D, "EOR", ADDR_ABSOLUTE_X, EOR_absolute_x);
	registerCallback(0x59, "EOR", ADDR_ABSOLUTE_Y, EOR_absolute_y);
	registerCallback(0x41, "EOR", ADDR_INDIRECT_X, EOR_indirect_x);
	registerCallback(0x51, "EOR", ADDR_INDIRECT_Y, EOR_indirect_y);

	registerCallback(0xE6, "INC", ADDR_ZERO_PAGE, INC_zero_page);
	registerCallback(0xF6, "INC", ADDR_ZERO_PAGE_X, INC_zero_page_x);
	registerCallback(0xEE, "INC", ADDR_ABSOLUTE, INC_absolute);
	registerCallback(0xFE, "INC", ADDR_ABSOLUTE_X, INC_absolute_x);
	
	registerCallback(0xE8, "INX", ADDR_IMPLIED, INX);
	registerCallback(0xC8, "INY", ADDR_IMPLIED, INY);

	registerCallback(0x4C, "JMP", ADDR_DIRECT, JMP_absolute);
	registerCallback(0x6C, "JMP", ADDR_INDIRECT, JMP_absolute_indirect);

	registerCallback(0x20, "JSR", ADDR_DIRECT, JSR);

	registerCallback(0xA9, "LDA", ADDR_IMMEDIATE, LDA_immediate);
	registerCallback(0xA5, "LDA", ADDR_ZERO_PAGE, LDA_zero_page);
	registerCallback(0xB5, "LDA", ADDR_ZERO_PAGE_X, LDA_zero_page_x);
	registerCallback(0xAD, "LDA", ADDR_ABSOLUTE, LDA_absolute);
	registerCallback(0xBD, "LDA", ADDR_ABSOLUTE_X, LDA_absolute_x);
	registerCallback(0xB9, "LDA", ADDR_ABSOLUTE_Y, LDA_absolute_y);
	registerCallback(0xA1, "LDA", ADDR_INDIRECT_X, LDA_indirect_x);
	registerCallback(0xB1, "LDA", ADDR_INDIRECT_Y, LDA_indirect_y);

	registerCallback(0xA2, "LDX", ADDR_IMMEDIATE, LDX_immediate);
	registerCallback(0xA6, "LDX", ADDR_ZERO_PAGE, LDX_zero_page);
	registerCallback(0xB6, "LDX", ADDR_ZERO_PAGE_Y,LDX_zero_page_y);
	registerCallback(0xAE, "LDX", ADDR_ABSOLUTE, LDX_absolute);
	registerCallback(0xBE, "LDX", ADDR_ABSOLUTE_Y, LDX_absolute_y);

	registerCallback(0xA0, "LDY", ADDR_IMMEDIATE, LDY_immediate);
	registerCallback(0xA4, "LDY", ADDR_ZERO_PAGE, LDY_zero_page);
	registerCallback(0xB4, "LDY", ADDR_ZERO_PAGE_X, LDY_zero_page_x);
	registerCallback(0xAC, "LDY", ADDR_ABSOLUTE, LDY_absolute);
	registerCallback(0xBC, "LDY", ADDR_ABSOLUTE_X, LDY_absolute_x);
	
	registerCallback(0x4A, "LSR", ADDR_ACCUMULATOR, LSR_accumulator);
	registerCallback(0x46, "LSR", ADDR_ZERO_PAGE, LSR_zero_page);
	registerCallback(0x56, "LSR", ADDR_ZERO_PAGE_X, LSR_zero_page_x);
	registerCallback(0x4E, "LSR", ADDR_ABSOLUTE, LSR_absolute);
	registerCallback(0x5E, "LSR", ADDR_ABSOLUTE_X, LSR_absolute_x);

	registerCallback(0xEA, "NOP", ADDR_IMPLIED, NOP);
	
	registerCallback(0x09, "ORA", ADDR_IMMEDIATE, ORA_immediate);
	registerCallback(0x05, "ORA", ADDR_ZERO_PAGE, ORA_zero_page);
	registerCallback(0x15, "ORA", ADDR_ZERO_PAGE_X, ORA_zero_page_x);
	registerCallback(0x0D, "ORA", ADDR_ABSOLUTE, ORA_absolute);
	registerCallback(0x1D, "ORA", ADDR_ABSOLUTE_X, ORA_absolute_x);
	registerCallback(0x19, "ORA", ADDR_ABSOLUTE_Y, ORA_absolute_y);
	registerCallback(0x01, "ORA", ADDR_INDIRECT_X, ORA_indirect_x);
	registerCallback(0x11, "ORA", ADDR_INDIRECT_Y, ORA_indirect_y);

	registerCallback(0x48, "PHA", ADDR_IMPLIED, PHA);
	registerCallback(0x08, "PHP", ADDR_IMPLIED, PHP);
	registerCallback(0x68, "PLA", ADDR_IMPLIED, PLA);
	registerCallback(0x28, "PLP", ADDR_IMPLIED, PLP);

	registerCallback(0x2A, "ROL", ADDR_ACCUMULATOR, ROL_accumulator);
	registerCallback(0x26, "ROL", ADDR_ZERO_PAGE, ROL_zero_page);
	registerCallback(0x36, "ROL", ADDR_ZERO_PAGE_X, ROL_zero_page_x);
	registerCallback(0x2E, "ROL", ADDR_ABSOLUTE, ROL_absolute);
	registerCallback(0x3E, "ROL", ADDR_ABSOLUTE_X, ROL_absolute_x);

	registerCallback(0x6A, "ROR", ADDR_ACCUMULATOR, ROR_accumulator);
	registerCallback(0x66, "ROR", ADDR_ZERO_PAGE, ROR_zero_page);
	registerCallback(0x76, "ROR", ADDR_ZERO_PAGE_X, ROR_zero_page_x);
	registerCallback(0x6E, "ROR", ADDR_ABSOLUTE, ROR_absolute);
	registerCallback(0x7E, "ROR", ADDR_ABSOLUTE_X, ROR_absolute_x);
	
	registerCallback(0x40, "RTI", ADDR_IMPLIED, RTI);
	registerCallback(0x60, "RTS", ADDR_IMPLIED, RTS);

	registerCallback(0xE9, "SBC", ADDR_IMMEDIATE, SBC_immediate);
	registerCallback(0xE5, "SBC", ADDR_ZERO_PAGE, SBC_zero_page);
	registerCallback(0xF5, "SBC", ADDR_ZERO_PAGE_X, SBC_zero_page_x);
	registerCallback(0xED, "SBC", ADDR_ABSOLUTE, SBC_absolute);
	registerCallback(0xFD, "SBC", ADDR_ABSOLUTE_X, SBC_absolute_x);
	registerCallback(0xF9, "SBC", ADDR_ABSOLUTE_Y, SBC_absolute_y);
	registerCallback(0xE1, "SBC", ADDR_INDIRECT_X, SBC_indirect_x);
	registerCallback(0xF1, "SBC", ADDR_INDIRECT_Y, SBC_indirect_y);

	registerCallback(0x38, "SEC", ADDR_IMPLIED, SEC);
	registerCallback(0xF8, "SED", ADDR_IMPLIED, SED);
	registerCallback(0x78, "SEI", ADDR_IMPLIED, SEI);

	registerCallback(0x85, "STA", ADDR_ZERO_PAGE, STA_zero_page);
	registerCallback(0x95, "STA", ADDR_ZERO_PAGE_X, STA_zero_page_x);
	registerCallback(0x8D, "STA", ADDR_ABSOLUTE, STA_absolute);
	registerCallback(0x9D, "STA", ADDR_ABSOLUTE_X, STA_absolute_x);
	registerCallback(0x99, "STA", ADDR_ABSOLUTE_Y, STA_absolute_y);
	registerCallback(0x81, "STA", ADDR_INDIRECT_X, STA_indirect_x);
	registerCallback(0x91, "STA", ADDR_INDIRECT_Y, STA_indirect_y);

	registerCallback(0x86, "STX", ADDR_ZERO_PAGE, STX_zero_page);
	registerCallback(0x96, "STX", ADDR_ZERO_PAGE_Y, STX_zero_page_y);
	registerCallback(0x8E, "STX", ADDR_ABSOLUTE, STX_absolute);

	registerCallback(0x84, "STY", ADDR_ZERO_PAGE, STY_zero_page);
	registerCallback(0x94, "STY", ADDR_ZERO_PAGE_X, STY_zero_page_x);
	registerCallback(0x8C, "STY", ADDR_ABSOLUTE, STY_absolute);

	registerCallback(0xAA, "TAX", ADDR_IMPLIED, TAX);
	registerCallback(0xA8, "TAY", ADDR_IMPLIED, TAY);
	registerCallback(0xBA, "TSX", ADDR_IMPLIED, TSX);
	registerCallback(0x8A, "TXA", ADDR_IMPLIED, TXA);
	registerCallback(0x9A, "TXS", ADDR_IMPLIED, TXS);
	registerCallback(0x98, "TYA", ADDR_IMPLIED, TYA);

	// Register illegal instructions
	registerIllegalInstructions();	
}

void
CPU::executeMicroInstruction()
{
    switch (next) {
            
        case fetch:
            
            /*
             if (PC == 2073) {
             startSilentTracing();
             }
             */
            
            PC_at_cycle_0 = PC;
            
            // Check interrupt lines
            if (doNmi) {
                
                if (tracingEnabled()) trace("NMI (source = %02X)\n", nmiLine);
                nmiEdge = false;
                clear8_delayed(edgeDetector);
                next = nmi_2;
                doNmi = false;
                doIrq = false; // NMI wins
                return;
                
            } else if (doIrq) {
                
                if (tracingEnabled()) trace("IRQ (source = %02X)\n", irqLine);
                next = irq_2;
                doIrq = false;
                return;
            }
            
            // Execute fetch phase
            FETCH_OPCODE
            next = actionFunc[opcode];
            
            // Disassemble command if requested
            if (tracingEnabled()) {
                trace("%s\n", disassemble());
            }
            
            // Check breakpoint tag
            if (breakpoint[PC_at_cycle_0] != NO_BREAKPOINT) {
                if (breakpoint[PC_at_cycle_0] & SOFT_BREAKPOINT) {
                    breakpoint[PC_at_cycle_0] &= ~SOFT_BREAKPOINT; // Soft breakpoints get deleted when reached
                    setErrorState(CPU_SOFT_BREAKPOINT_REACHED);
                } else {
                    setErrorState(CPU_HARD_BREAKPOINT_REACHED);
                }
                debug(1, "Breakpoint reached\n");
            }
            return;
            
        // -------------------------------------------------------------------------------
        // Illegal instructions
        // -------------------------------------------------------------------------------
            
        case JAM:
            
            setErrorState(CPU_ILLEGAL_INSTRUCTION);
            CONTINUE

        case JAM_2:
            DONE

        // -------------------------------------------------------------------------------
        // IRQ handling
        // -------------------------------------------------------------------------------

        case irq:
            
            IDLE_READ_IMPLIED
            CONTINUE
            
        case irq_2:
            
            IDLE_READ_IMPLIED
            CONTINUE
            
        case irq_3:
            
            mem->poke(0x100+(SP--), HI_BYTE(PC));
            CONTINUE
            
        case irq_4:
            
            mem->poke(0x100+(SP--), LO_BYTE(PC));
            CONTINUE
            
        case irq_5:
            
            mem->poke(0x100+(SP--), getPWithClearedB());
            setI(1);
            CONTINUE
            
        case irq_6:
            
            data = mem->peek(0xFFFE);
            CONTINUE
            
        case irq_7:
            
            setPCL(data);
            setPCH(mem->peek(0xFFFF));
            DONE_NO_POLL
            
        // -------------------------------------------------------------------------------
        // NMI handling
        // -------------------------------------------------------------------------------
            
        case nmi:
            
            IDLE_READ_IMPLIED
            CONTINUE
            
        case nmi_2:

            IDLE_READ_IMPLIED
            CONTINUE
            
        case nmi_3:
            
            mem->poke(0x100+(SP--), HI_BYTE(PC));
            CONTINUE
            
        case nmi_4:
            
            mem->poke(0x100+(SP--), LO_BYTE(PC));
            CONTINUE
            
        case nmi_5:
            
            mem->poke(0x100+(SP--), getPWithClearedB());
            setI(1);
            CONTINUE
            
        case nmi_6:
            
            data = mem->peek(0xFFFA);
            CONTINUE
            
        case nmi_7:
            
            setPCL(data);
            setPCH(mem->peek(0xFFFB));
            DONE_NO_POLL

        // -------------------------------------------------------------------------------
        // Instruction: ADC
        //
        // Operation:   A,C := A+M+C
        //
        // Flags:       N Z C I D V
        //              / / / - - /
        // -------------------------------------------------------------------------------

        // -------------------------------------------------------------------------------
        case ADC_immediate:

            READ_IMMEDIATE
            adc(data);
            DONE

        // -------------------------------------------------------------------------------
        case ADC_zero_page:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case ADC_zero_page_2:
            
            READ_FROM_ZERO_PAGE
            adc(data);
            DONE

        // -------------------------------------------------------------------------------
        case ADC_zero_page_x:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case ADC_zero_page_x_2:
            
            READ_FROM_ZERO_PAGE
            ADD_INDEX_X
            CONTINUE
            
        case ADC_zero_page_x_3:
            
            READ_FROM_ZERO_PAGE
            adc(data);
            DONE

            // -------------------------------------------------------------------------------
        case ADC_absolute:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case ADC_absolute_2:
            
            FETCH_ADDR_HI
            CONTINUE
            
        case ADC_absolute_3:
            
            READ_FROM_ADDRESS
            adc(data);
            DONE

        // -------------------------------------------------------------------------------
        case ADC_absolute_x:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case ADC_absolute_x_2:
            
            FETCH_ADDR_HI
            ADD_INDEX_X
            CONTINUE
            
        case ADC_absolute_x_3:
            
            READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) {
                FIX_ADDR_HI
                CONTINUE
            } else {
                adc(data);
                DONE
            }
            
        case ADC_absolute_x_4:
            
            READ_FROM_ADDRESS
            adc(data);
            DONE

        // -------------------------------------------------------------------------------
        case ADC_absolute_y:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case ADC_absolute_y_2:
            
            FETCH_ADDR_HI
            ADD_INDEX_Y
            CONTINUE
            
        case ADC_absolute_y_3:
            
            READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) {
                FIX_ADDR_HI
                CONTINUE
            } else {
                adc(data);
                DONE
            }
            
        case ADC_absolute_y_4:
            
            READ_FROM_ADDRESS
            adc(data);
            DONE

        // -------------------------------------------------------------------------------
        case ADC_indirect_x:
            
            FETCH_POINTER_ADDR
            CONTINUE
            
        case ADC_indirect_x_2:
            
            IDLE_READ_FROM_ADDRESS_INDIRECT
            ADD_INDEX_X_INDIRECT
            CONTINUE
            
        case ADC_indirect_x_3:
            
            FETCH_ADDR_LO_INDIRECT
            CONTINUE
            
        case ADC_indirect_x_4:
            
            FETCH_ADDR_HI_INDIRECT
            CONTINUE
            
        case ADC_indirect_x_5:
            
            READ_FROM_ADDRESS
            adc(data);
            DONE

        // -------------------------------------------------------------------------------
        case ADC_indirect_y:
            
            FETCH_POINTER_ADDR
            CONTINUE
            
        case ADC_indirect_y_2:
            
            FETCH_ADDR_LO_INDIRECT
            CONTINUE
            
        case ADC_indirect_y_3:
            
            FETCH_ADDR_HI_INDIRECT
            ADD_INDEX_Y;
            CONTINUE
            
        case ADC_indirect_y_4:
            
            READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) {
                FIX_ADDR_HI
                CONTINUE
            } else {
                adc(data);
                DONE
            }
            
        case ADC_indirect_y_5:
            
            READ_FROM_ADDRESS
            adc(data);
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: AND
        //
        // Operation:   A := A AND M
        //
        // Flags:       N Z C I D V
        //              / / - - - -
        // -------------------------------------------------------------------------------

        case AND_immediate:
            
            READ_IMMEDIATE
            loadA(A & data);
            DONE

        // -------------------------------------------------------------------------------
        case AND_absolute:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case AND_absolute_2:
            
            FETCH_ADDR_HI
            CONTINUE
            
        case AND_absolute_3:
            READ_FROM_ADDRESS
            loadA(A & data);
            DONE

        // -------------------------------------------------------------------------------
        case AND_zero_page:

            FETCH_ADDR_LO
            CONTINUE

        case AND_zero_page_2:

            READ_FROM_ZERO_PAGE
            loadA(A & data);
            DONE

        // -------------------------------------------------------------------------------
        case AND_zero_page_x:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case AND_zero_page_x_2:
            
            READ_FROM_ZERO_PAGE
            ADD_INDEX_X
            CONTINUE
            
        case AND_zero_page_x_3:
            
            READ_FROM_ZERO_PAGE
            loadA(A & data);
            DONE

        // -------------------------------------------------------------------------------
        case AND_absolute_x:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case AND_absolute_x_2:
            
            FETCH_ADDR_HI
            ADD_INDEX_X
            CONTINUE
            
        case AND_absolute_x_3:
            
            READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) {
                FIX_ADDR_HI
                CONTINUE
            } else {
                loadA(A & data);
                DONE
            }
            
        case AND_absolute_x_4:
            
            READ_FROM_ADDRESS
            loadA(A & data);
            DONE

        // -------------------------------------------------------------------------------
        case AND_absolute_y:
            
            FETCH_ADDR_LO;
            CONTINUE
            
        case AND_absolute_y_2:
            
            FETCH_ADDR_HI
            ADD_INDEX_Y
            CONTINUE
            
        case AND_absolute_y_3:
            
            READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) {
                FIX_ADDR_HI
                CONTINUE
            } else {
                loadA(A & data);
                DONE
            }
            
        case AND_absolute_y_4:
        
            READ_FROM_ADDRESS
            loadA(A & data);
            DONE
        
        // -------------------------------------------------------------------------------
        case AND_indirect_x:
            
            FETCH_POINTER_ADDR
            CONTINUE
            
        case AND_indirect_x_2:
            
            IDLE_READ_FROM_ADDRESS_INDIRECT
            ADD_INDEX_X_INDIRECT
            CONTINUE
            
        case AND_indirect_x_3:
            
            FETCH_ADDR_LO_INDIRECT
            CONTINUE
            
        case AND_indirect_x_4:
            
            FETCH_ADDR_HI_INDIRECT
            CONTINUE
            
        case AND_indirect_x_5:
            
            READ_FROM_ADDRESS
            loadA(A & data);
            DONE
            
        // -------------------------------------------------------------------------------
        case AND_indirect_y:
            
            FETCH_POINTER_ADDR
            CONTINUE
            
        case AND_indirect_y_2:
            
            FETCH_ADDR_LO_INDIRECT
            CONTINUE
            
        case AND_indirect_y_3:
            
            FETCH_ADDR_HI_INDIRECT
            ADD_INDEX_Y
            CONTINUE
            
        case AND_indirect_y_4:
            
            READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) {
                FIX_ADDR_HI
                CONTINUE
            } else {
                loadA(A & data);
                DONE
            }
            
        case AND_indirect_y_5:
            
            READ_FROM_ADDRESS
            loadA(A & data);
            DONE
            
        // -------------------------------------------------------------------------------
        // Instruction: ASL
        //
        // Operation:   C <- (A|M << 1) <- 0
        //
        // Flags:       N Z C I D V
        //              / / / - - -
        // -------------------------------------------------------------------------------
    
        #define DO_ASL setC(data & 128); data = data << 1;

        // -------------------------------------------------------------------------------
        case ASL_accumulator:
            
            IDLE_READ_IMPLIED
            setC(A & 128); loadA(A << 1);
            DONE

        // -------------------------------------------------------------------------------
        case ASL_zero_page:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case ASL_zero_page_2:
            
            READ_FROM_ZERO_PAGE
            CONTINUE
            
        case ASL_zero_page_3:
            
            WRITE_TO_ZERO_PAGE
            DO_ASL
            CONTINUE
            
        case ASL_zero_page_4:
            
            WRITE_TO_ZERO_PAGE_AND_SET_FLAGS
            DONE

        // -------------------------------------------------------------------------------
        case ASL_zero_page_x:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case ASL_zero_page_x_2:
            
            READ_FROM_ZERO_PAGE
            ADD_INDEX_X
            CONTINUE
            
        case ASL_zero_page_x_3:
            
            READ_FROM_ZERO_PAGE
            CONTINUE
            
        case ASL_zero_page_x_4:
            
            WRITE_TO_ZERO_PAGE
            DO_ASL
            CONTINUE
            
        case ASL_zero_page_x_5:
            
            WRITE_TO_ZERO_PAGE_AND_SET_FLAGS
            DONE
            
        // -------------------------------------------------------------------------------
        case ASL_absolute:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case ASL_absolute_2:
            
            FETCH_ADDR_HI
            CONTINUE
            
        case ASL_absolute_3:
            
            READ_FROM_ADDRESS
            CONTINUE
            
        case ASL_absolute_4:
            
            WRITE_TO_ADDRESS
            DO_ASL
            CONTINUE
            
        case ASL_absolute_5:
            
            WRITE_TO_ADDRESS_AND_SET_FLAGS
            DONE

        // -------------------------------------------------------------------------------
        case ASL_absolute_x:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case ASL_absolute_x_2:
            
            FETCH_ADDR_HI
            ADD_INDEX_X
            CONTINUE
            
        case ASL_absolute_x_3:
            
            READ_FROM_ADDRESS;
            if (PAGE_BOUNDARY_CROSSED) { FIX_ADDR_HI }
            CONTINUE
            
        case ASL_absolute_x_4:
            
            READ_FROM_ADDRESS
            CONTINUE
            
        case ASL_absolute_x_5:
            
            WRITE_TO_ADDRESS
            DO_ASL
            CONTINUE
            
        case ASL_absolute_x_6:
            
            WRITE_TO_ADDRESS_AND_SET_FLAGS
            DONE

        // -------------------------------------------------------------------------------
        case ASL_indirect_x:
            
            FETCH_POINTER_ADDR
            CONTINUE
            
        case ASL_indirect_x_2:
            
            IDLE_READ_FROM_ADDRESS_INDIRECT
            ADD_INDEX_X_INDIRECT
            CONTINUE
            
        case ASL_indirect_x_3:
            
            FETCH_ADDR_LO_INDIRECT
            CONTINUE
            
        case ASL_indirect_x_4:
            
            FETCH_ADDR_HI_INDIRECT
            CONTINUE
            
        case ASL_indirect_x_5:
            
            READ_FROM_ADDRESS
            CONTINUE
            
        case ASL_indirect_x_6:
            
            WRITE_TO_ADDRESS
            DO_ASL
            CONTINUE
            
        case ASL_indirect_x_7:
            
            WRITE_TO_ADDRESS_AND_SET_FLAGS
            DONE
            
        // -------------------------------------------------------------------------------
        // Instruction: BCC
        //
        // Operation:   Branch on C = 0
        //
        // Flags:       N Z C I D V
        //              - - - - - -
        // -------------------------------------------------------------------------------

        // void CPU::branch(int8_t offset) { PC += offset; }
            
        case branch_3_underflow:
            
            IDLE_READ_FROM(PC + 0x100)
            POLL_IRQ_AND_NMI_AGAIN
            DONE_NO_POLL;
            
        case branch_3_overflow:
            
            IDLE_READ_FROM(PC - 0x100)
            POLL_IRQ_AND_NMI_AGAIN
            DONE_NO_POLL

        // ------------------------------------------------------------------------------
        case BCC_relative:
            
            READ_IMMEDIATE
            POLL_IRQ_AND_NMI
            
            if (!getC()) {
                CONTINUE
            } else {
                DONE_NO_POLL
            }
            
        case BCC_relative_2:
        {
            IDLE_READ_IMPLIED
            uint8_t pc_hi = HI_BYTE(PC);
            PC += (int8_t)data;
            
            if (pc_hi != HI_BYTE(PC)) {
                next = (data & 0x80) ? branch_3_underflow : branch_3_overflow;
                return;
            } else {
                nextPossibleIrqCycle++; // Delay IRQs by one cycle
                nextPossibleNmiCycle++;
                DONE_NO_POLL
            }
        }
            
        // -------------------------------------------------------------------------------
        // Instruction: BCS
        //
        // Operation:   Branch on C = 1
        //
        // Flags:       N Z C I D V
        //              - - - - - -
        // -------------------------------------------------------------------------------

        case BCS_relative:
            
            READ_IMMEDIATE
            POLL_IRQ_AND_NMI
            
            if (getC()) {
                CONTINUE
            } else {
                DONE_NO_POLL
            }
            
        case BCS_relative_2:
        {
            IDLE_READ_IMPLIED
            uint8_t pc_hi = HI_BYTE(PC);
            PC += (int8_t)data;
            
            if (pc_hi != HI_BYTE(PC)) {
                next = (data & 0x80) ? branch_3_underflow : branch_3_overflow;
                return;
            } else {
                nextPossibleIrqCycle++; // Delay IRQs by one cycle
                nextPossibleNmiCycle++;
                DONE_NO_POLL
            }
        }
            
        // -------------------------------------------------------------------------------
        // Instruction: BEQ
        //
        // Operation:   Branch on Z = 1
        //
        // Flags:       N Z C I D V
        //              - - - - - -
        // -------------------------------------------------------------------------------
            
        case BEQ_relative:
            
            READ_IMMEDIATE
            POLL_IRQ_AND_NMI
            
            if (getZ()) {
                CONTINUE
            } else {
                DONE_NO_POLL
            }
            
        case BEQ_relative_2:
        {
            IDLE_READ_IMPLIED
            uint8_t pc_hi = HI_BYTE(PC);
            PC += (int8_t)data;
            
            if (pc_hi != HI_BYTE(PC)) {
                next = (data & 0x80) ? branch_3_underflow : branch_3_overflow;
                return;
            } else {
                nextPossibleIrqCycle++; // Delay IRQs by one cycle
                nextPossibleNmiCycle++;
                DONE_NO_POLL
            }
        }
            
        // -------------------------------------------------------------------------------
        // Instruction: BIT
        //
        // Operation:   A AND M, N := M7, V := M6
        //
        // Flags:       N Z C I D V
        //              / / - - - /
        // -------------------------------------------------------------------------------

        case BIT_zero_page:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case BIT_zero_page_2:
            
            READ_FROM_ZERO_PAGE
            setN(data & 128);
            setV(data & 64);
            setZ((data & A) == 0);
            DONE

        // -------------------------------------------------------------------------------
        case BIT_absolute:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case BIT_absolute_2:
            
            FETCH_ADDR_HI;
            CONTINUE
            
        case BIT_absolute_3:
            
            READ_FROM_ADDRESS
            setN(data & 128);
            setV(data & 64);
            setZ((data & A) == 0);
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: BMI
        //
        // Operation:   Branch on N = 1
        //
        // Flags:       N Z C I D V
        //              - - - - - -
        // -------------------------------------------------------------------------------

        case BMI_relative:
            
            READ_IMMEDIATE
            POLL_IRQ_AND_NMI
            
            if (getN()) {
                CONTINUE
            } else {
                DONE_NO_POLL
            }
            
        case BMI_relative_2:
        {
            IDLE_READ_IMPLIED
            uint8_t pc_hi = HI_BYTE(PC);
            PC += (int8_t)data;
            
            if (pc_hi != HI_BYTE(PC)) {
                next = (data & 0x80) ? branch_3_underflow : branch_3_overflow;
                return;
            } else {
                nextPossibleIrqCycle++; // Delay IRQs by one cycle
                nextPossibleNmiCycle++;
                DONE_NO_POLL
            }
        }
            
        // -------------------------------------------------------------------------------
        // Instruction: BNE
        //
        // Operation:   Branch on Z = 0
        //
        // Flags:       N Z C I D V
        //              - - - - - -
        // -------------------------------------------------------------------------------
            
        case BNE_relative:
            
            READ_IMMEDIATE
            POLL_IRQ_AND_NMI
            
            if (!getZ()) {
                CONTINUE
            } else {
                DONE_NO_POLL
            }
            
        case BNE_relative_2:
        {
            IDLE_READ_IMPLIED
            uint8_t pc_hi = HI_BYTE(PC);
            PC += (int8_t)data;
            
            if (pc_hi != HI_BYTE(PC)) {
                next = (data & 0x80) ? branch_3_underflow : branch_3_overflow;
                return;
            } else {
                nextPossibleIrqCycle++; // Delay IRQs by one cycle
                nextPossibleNmiCycle++;
                DONE_NO_POLL
            }
        }

        // -------------------------------------------------------------------------------
        // Instruction: BPL
        //
        // Operation:   Branch on N = 0
        //
        // Flags:       N Z C I D V
        //              - - - - - -
        // -------------------------------------------------------------------------------

        case BPL_relative:
            
            READ_IMMEDIATE
            POLL_IRQ_AND_NMI
            
            if (!getN()) {
                CONTINUE
            } else {
                DONE_NO_POLL
            }
            
        case BPL_relative_2:
        {
            IDLE_READ_IMPLIED
            uint8_t pc_hi = HI_BYTE(PC);
            PC += (int8_t)data;
            
            if (pc_hi != HI_BYTE(PC)) {
                next = (data & 0x80) ? branch_3_underflow : branch_3_overflow;
                return;
            } else {
                nextPossibleIrqCycle++; // Delay IRQs by one cycle
                nextPossibleNmiCycle++;
                DONE_NO_POLL
            }
        }

        // -------------------------------------------------------------------------------
        // Instruction: BRK
        //
        // Operation:   Forced Interrupt (Break)
        //
        // Flags:       N Z C I D V    B
        //              - - - 1 - -    1
        // -------------------------------------------------------------------------------
            
        case BRK:
            
            IDLE_READ_IMMEDIATE
            CONTINUE
            
        case BRK_2:
            
            setB(1);
            PUSH_PCH
            CONTINUE
            
        case BRK_3:
            
            PUSH_PCL
            
            // "The official NMOS 65xx documentation claims that the BRK instruction could only cause a jump to the IRQ
            //  vector ($FFFE). However, if an NMI interrupt occurs while executing a BRK instruction, the processor will
            //  jump to the NMI vector ($FFFA), and the P register will be pushed on the stack with the B flag set."
            if (nmiEdge) {
                nmiEdge = false;
                clear8_delayed(edgeDetector);
                next = BRK_nmi_4;
            } else {
                next = BRK_4;
            }
            return;
            
        case BRK_4:
            
            PUSH_P
            CONTINUE
            
        case BRK_5:
            
            data = mem->peek(0xFFFE);
            CONTINUE
            
        case BRK_6:
            
            setPCL(data);
            setPCH(mem->peek(0xFFFF));
            setI(1);
            
            // "Ein NMI darf nicht sofort nach einem BRK oder IRQ ausgef�hrt werden, sondern erst mit 1 Cycle Delay."
            // [http://www.c64-wiki.de/index.php/Micro64]
            if (nextPossibleNmiCycle < c64->getCycles() + 2)
                nextPossibleNmiCycle = c64->getCycles() + 2;
            
            // HOW DO WE HANDLE THIS???
            POLL_IRQ
            doNmi = false;
            DONE_NO_POLL
            
        case BRK_nmi_4:
            
            PUSH_P
            CONTINUE
            
        case BRK_nmi_5:
            
            data = mem->peek(0xFFFA);
            CONTINUE
            
        case BRK_nmi_6:
            
            setPCL(data);
            setPCH(mem->peek(0xFFFB));
            setI(1);
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: BVC
        //
        // Operation:   Branch on V = 0
        //
        // Flags:       N Z C I D V
        //              - - - - - -
        // -------------------------------------------------------------------------------

        case BVC_relative:
            
            READ_IMMEDIATE
            POLL_IRQ_AND_NMI
            
            if (chipModel == MOS_6502 /* Drive CPU */ && !c64->floppy.getBitAccuracy()) {
                
                // Special handling for the VC1541 CPU. Taken from Frodo
                if (!((c64->floppy.via2.io[12] & 0x0E) == 0x0E || getV())) {
                    CONTINUE
                } else {
                    DONE_NO_POLL
                }
                
            } else {
                
                // Standard CPU behavior
                if (!getV()) {
                    CONTINUE
                } else {
                    DONE_NO_POLL
                }
            }
            
        case BVC_relative_2:
        {
            IDLE_READ_IMPLIED
            uint8_t pc_hi = HI_BYTE(PC);
            PC += (int8_t)data;
            
            if (pc_hi != HI_BYTE(PC)) {
                next = (data & 0x80) ? branch_3_underflow : branch_3_overflow;
                return;
            } else {
                nextPossibleIrqCycle++; // Delay IRQs by one cycle
                nextPossibleNmiCycle++;
                DONE_NO_POLL
            }
        }

        // -------------------------------------------------------------------------------
        // Instruction: BVS
        //
        // Operation:   Branch on V = 1
        //
        // Flags:       N Z C I D V
        //              - - - - - -
        // -------------------------------------------------------------------------------

        case BVS_relative:
            
            READ_IMMEDIATE
            POLL_IRQ_AND_NMI
            
            if (chipModel == MOS_6502 /* Drive CPU */ && !c64->floppy.getBitAccuracy()) {
                
                // Special handling for the VC1541 CPU. Taken from Frodo
                if ((c64->floppy.via2.io[12] & 0x0E) == 0x0E || getV()) {
                    CONTINUE
                } else {
                    DONE_NO_POLL
                }
                
            } else {
                
                // Standard CPU behavior
                if (getV()) {
                    CONTINUE
                } else {
                    DONE_NO_POLL
                }
            }
            
        case BVS_relative_2:
        {
            IDLE_READ_IMPLIED
            uint8_t pc_hi = HI_BYTE(PC);
            PC += (int8_t)data;
            
            if (pc_hi != HI_BYTE(PC)) {
                next = (data & 0x80) ? branch_3_underflow : branch_3_overflow;
                return;
            } else {
                nextPossibleIrqCycle++; // Delay IRQs by one cycle
                nextPossibleNmiCycle++;
                DONE_NO_POLL
            }
        }

        // -------------------------------------------------------------------------------
        // Instruction: CLC
        //
        // Operation:   C := 0
        //
        // Flags:       N Z C I D V
        //              - - 0 - - -
        // -------------------------------------------------------------------------------

        case CLC:
            
            IDLE_READ_IMPLIED
            setC(0);
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: CLD
        //
        // Operation:   D := 0
        //
        // Flags:       N Z C I D V
        //              - - - - 0 -
        // -------------------------------------------------------------------------------

        case CLD:
            
            IDLE_READ_IMPLIED
            setD(0);
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: CLI
        //
        // Operation:   I := 0
        //
        // Flags:       N Z C I D V
        //              - - - 0 - -
        // -------------------------------------------------------------------------------

        case CLI:
            
            IDLE_READ_IMPLIED
            POLL_IRQ_AND_NMI
            
            oldI = I;
            setI(0);
            
            DONE_NO_POLL

        // -------------------------------------------------------------------------------
        // Instruction: CLV
        //
        // Operation:   V := 0
        //
        // Flags:       N Z C I D V
        //              - - - - - 0
        // -------------------------------------------------------------------------------

        case CLV:
            
            IDLE_READ_IMPLIED
            setV(0);
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: CMP
        //
        // Operation:   A-M
        //
        // Flags:       N Z C I D V
        //              / / / - - -
        // -------------------------------------------------------------------------------

        // -------------------------------------------------------------------------------
        case CMP_immediate:
            
            READ_IMMEDIATE
            cmp(A, data);
            DONE

        // -------------------------------------------------------------------------------
        case CMP_absolute:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case CMP_absolute_2:
            
            FETCH_ADDR_HI
            CONTINUE
            
        case CMP_absolute_3:
            
            READ_FROM_ADDRESS
            cmp(A, data);
            DONE

        // -------------------------------------------------------------------------------
        case CMP_zero_page:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case CMP_zero_page_2:
            
            READ_FROM_ZERO_PAGE
            cmp(A, data);
            DONE

        // -------------------------------------------------------------------------------
        case CMP_zero_page_x:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case CMP_zero_page_x_2:
            
            READ_FROM_ZERO_PAGE
            ADD_INDEX_X
            CONTINUE
            
        case CMP_zero_page_x_3:
            
            READ_FROM_ZERO_PAGE
            cmp(A, data);
            DONE

        // -------------------------------------------------------------------------------
        case CMP_absolute_x:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case CMP_absolute_x_2:
            
            FETCH_ADDR_HI
            ADD_INDEX_X
            CONTINUE
            
        case CMP_absolute_x_3:
            
            READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) {
                FIX_ADDR_HI
                CONTINUE
            } else {
                cmp(A, data);
                DONE
            }
            
        case CMP_absolute_x_4:
            
            READ_FROM_ADDRESS
            cmp(A, data);
            DONE

        // -------------------------------------------------------------------------------
        case CMP_absolute_y:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case CMP_absolute_y_2:
            
            FETCH_ADDR_HI
            ADD_INDEX_Y
            CONTINUE
            
        case CMP_absolute_y_3:
            
            READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) {
                FIX_ADDR_HI
                CONTINUE
            } else {
                cmp(A, data);
                DONE
            }
            
        case CMP_absolute_y_4:
            
            READ_FROM_ADDRESS
            cmp(A, data);
            DONE

        // -------------------------------------------------------------------------------
        case CMP_indirect_x:
            
            FETCH_POINTER_ADDR
            CONTINUE
            
        case CMP_indirect_x_2:
            
            IDLE_READ_FROM_ADDRESS_INDIRECT
            ADD_INDEX_X_INDIRECT
            CONTINUE
            
        case CMP_indirect_x_3:
            
            FETCH_ADDR_LO_INDIRECT
            CONTINUE
            
        case CMP_indirect_x_4:
            
            FETCH_ADDR_HI_INDIRECT
            CONTINUE
            
        case CMP_indirect_x_5:
            
            READ_FROM_ADDRESS
            cmp(A, data);
            DONE

        // -------------------------------------------------------------------------------
        case CMP_indirect_y:
            
            FETCH_POINTER_ADDR
            CONTINUE
            
        case CMP_indirect_y_2:
            
            FETCH_ADDR_LO_INDIRECT
            CONTINUE
            
        case CMP_indirect_y_3:
            
            FETCH_ADDR_HI_INDIRECT
            ADD_INDEX_Y
            CONTINUE
            
        case CMP_indirect_y_4:
            
            READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) {
                FIX_ADDR_HI
                CONTINUE
            } else {
                cmp(A, data);
                DONE
            }
            
        case CMP_indirect_y_5:
            
            READ_FROM_ADDRESS
            cmp(A, data);
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: CPX
        //
        // Operation:   X-M
        //
        // Flags:       N Z C I D V
        //              / / / - - -
        // -------------------------------------------------------------------------------

        case CPX_immediate:
            
            READ_IMMEDIATE
            cmp(X, data);
            DONE

        // -------------------------------------------------------------------------------
        case CPX_zero_page:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case CPX_zero_page_2:
            
            READ_FROM_ZERO_PAGE
            cmp(X, data);
            DONE

        // -------------------------------------------------------------------------------
        case CPX_absolute:
            FETCH_ADDR_LO
            CONTINUE
            
        case CPX_absolute_2:
            FETCH_ADDR_HI
            CONTINUE
            
        case CPX_absolute_3:
            READ_FROM_ADDRESS
            cmp(X, data);
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: CPY
        //
        // Operation:   Y-M
        //
        // Flags:       N Z C I D V
        //              / / / - - -
        // -------------------------------------------------------------------------------

        case CPY_immediate:
            
            READ_IMMEDIATE
            cmp(Y, data);
            DONE

        // -------------------------------------------------------------------------------
        case CPY_zero_page:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case CPY_zero_page_2:
            
            READ_FROM_ZERO_PAGE
            cmp(Y, data);
            DONE

        // -------------------------------------------------------------------------------
        case CPY_absolute:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case CPY_absolute_2:
            
            FETCH_ADDR_HI
            CONTINUE
            
        case CPY_absolute_3:
            
            READ_FROM_ADDRESS
            cmp(Y, data);
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: DEC
        //
        // Operation:   M := : M - 1
        //
        // Flags:       N Z C I D V
        //              / / - - - -
        // -------------------------------------------------------------------------------

        #define DO_DEC data--;

        // -------------------------------------------------------------------------------
        case DEC_zero_page:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case DEC_zero_page_2:
            
            READ_FROM_ZERO_PAGE
            CONTINUE
            
        case DEC_zero_page_3:
            
            WRITE_TO_ZERO_PAGE
            DO_DEC
            CONTINUE
            
        case DEC_zero_page_4:
            
            WRITE_TO_ZERO_PAGE_AND_SET_FLAGS
            DONE

        // -------------------------------------------------------------------------------
        case DEC_zero_page_x:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case DEC_zero_page_x_2:
            
            READ_FROM_ZERO_PAGE
            ADD_INDEX_X
            CONTINUE
            
        case DEC_zero_page_x_3:
            
            READ_FROM_ZERO_PAGE
            CONTINUE
            
        case DEC_zero_page_x_4:
            
            WRITE_TO_ZERO_PAGE
            DO_DEC
            CONTINUE
            
        case DEC_zero_page_x_5:
            
            WRITE_TO_ZERO_PAGE_AND_SET_FLAGS
            DONE

        // -------------------------------------------------------------------------------
        case DEC_absolute:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case DEC_absolute_2:
            
            FETCH_ADDR_HI
            CONTINUE
            
        case DEC_absolute_3:
            
            READ_FROM_ADDRESS
            CONTINUE
            
        case DEC_absolute_4:
            
            WRITE_TO_ADDRESS
            DO_DEC
            CONTINUE
            
        case DEC_absolute_5:
            
            WRITE_TO_ADDRESS_AND_SET_FLAGS
            DONE

        // -------------------------------------------------------------------------------
        case DEC_absolute_x:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case DEC_absolute_x_2:
            
            FETCH_ADDR_HI
            ADD_INDEX_X
            CONTINUE
            
        case DEC_absolute_x_3:
            
            READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) { FIX_ADDR_HI }
            CONTINUE
            
        case DEC_absolute_x_4:
            
            READ_FROM_ADDRESS
            CONTINUE
            
        case DEC_absolute_x_5:
            
            WRITE_TO_ADDRESS
            DO_DEC
            CONTINUE
            
        case DEC_absolute_x_6:
            
            WRITE_TO_ADDRESS_AND_SET_FLAGS
            DONE

        // -------------------------------------------------------------------------------
        case DEC_indirect_x:
            
            FETCH_POINTER_ADDR
            CONTINUE
            
        case DEC_indirect_x_2:
            
            IDLE_READ_FROM_ADDRESS_INDIRECT
            ADD_INDEX_X_INDIRECT
            CONTINUE
            
        case DEC_indirect_x_3:
            
            FETCH_ADDR_LO_INDIRECT
            CONTINUE
            
        case DEC_indirect_x_4:
            
            FETCH_ADDR_HI_INDIRECT
            CONTINUE
            
        case DEC_indirect_x_5:
            
            READ_FROM_ADDRESS
            CONTINUE
            
        case DEC_indirect_x_6:
            
            WRITE_TO_ADDRESS
            DO_DEC
            CONTINUE
            
        case DEC_indirect_x_7:
            
            WRITE_TO_ADDRESS_AND_SET_FLAGS
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: DEX
        //
        // Operation:   X := X - 1
        //
        // Flags:       N Z C I D V
        //              / / - - - -
        // -------------------------------------------------------------------------------

        case DEX:
            
            IDLE_READ_IMPLIED
            loadX(getX()-1);
            DONE
            
        // -------------------------------------------------------------------------------
        // Instruction: DEY
        //
        // Operation:   Y := Y - 1
        //
        // Flags:       N Z C I D V
        //              / / - - - -
        // -------------------------------------------------------------------------------

        case DEY:
            
            IDLE_READ_IMPLIED
            loadY(getY()-1);
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: EOR
        //
        // Operation:   A := A XOR M
        //
        // Flags:       N Z C I D V
        //              / / - - - -
        // -------------------------------------------------------------------------------

        case EOR_immediate:
            
            READ_IMMEDIATE
            loadA(A ^ data);
            DONE

        // -------------------------------------------------------------------------------
        case EOR_absolute:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case EOR_absolute_2:
            
            FETCH_ADDR_HI
            CONTINUE
            
        case EOR_absolute_3:
            
            READ_FROM_ADDRESS
            loadA(A ^ data);
            DONE

        // -------------------------------------------------------------------------------
        case EOR_zero_page:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case EOR_zero_page_2:
            
            READ_FROM_ZERO_PAGE
            loadA(A ^ data);
            DONE

        // -------------------------------------------------------------------------------
        case EOR_zero_page_x:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case EOR_zero_page_x_2:
            
            READ_FROM_ZERO_PAGE
            ADD_INDEX_X
            CONTINUE
            
        case EOR_zero_page_x_3:
            
            READ_FROM_ZERO_PAGE
            loadA(A ^ data);
            DONE

        // -------------------------------------------------------------------------------
        case EOR_absolute_x:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case EOR_absolute_x_2:
            
            FETCH_ADDR_HI
            ADD_INDEX_X
            CONTINUE
            
        case EOR_absolute_x_3:
            
            READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) {
                FIX_ADDR_HI;
                CONTINUE
            } else {
                loadA(A ^ data);
                DONE
            }
            
        case EOR_absolute_x_4:
            
            READ_FROM_ADDRESS
            loadA(A ^ data);
            DONE

        // -------------------------------------------------------------------------------
        case EOR_absolute_y:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case EOR_absolute_y_2:
            
            FETCH_ADDR_HI
            ADD_INDEX_Y
            CONTINUE
            
        case EOR_absolute_y_3:
            
            READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) {
                FIX_ADDR_HI
                CONTINUE
            } else {
                loadA(A ^ data);
                DONE
            }
            
        case EOR_absolute_y_4:
            
            READ_FROM_ADDRESS
            loadA(A ^ data);
            DONE

        // -------------------------------------------------------------------------------
        case EOR_indirect_x:
            
            FETCH_POINTER_ADDR
            CONTINUE
            
        case EOR_indirect_x_2:
            
            IDLE_READ_FROM_ADDRESS_INDIRECT
            ADD_INDEX_X_INDIRECT
            CONTINUE
            
        case EOR_indirect_x_3:
            
            FETCH_ADDR_LO_INDIRECT
            CONTINUE
            
        case EOR_indirect_x_4:
            
            FETCH_ADDR_HI_INDIRECT
            CONTINUE
            
        case EOR_indirect_x_5:
            
            READ_FROM_ADDRESS
            loadA(A ^ data);
            DONE

        // -------------------------------------------------------------------------------
        case EOR_indirect_y:
            
            FETCH_POINTER_ADDR
            CONTINUE
            
        case EOR_indirect_y_2:
            
            FETCH_ADDR_LO_INDIRECT
            CONTINUE
            
        case EOR_indirect_y_3:
            
            FETCH_ADDR_HI_INDIRECT
            ADD_INDEX_Y
            CONTINUE
            
        case EOR_indirect_y_4:
            
            READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) {
                FIX_ADDR_HI
                CONTINUE
            } else {
                loadA(A ^ data);
                DONE
            }
            
        case EOR_indirect_y_5:
            
            READ_FROM_ADDRESS
            loadA(A ^ data);
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: INC
        //
        // Operation:   M := M + 1
        //
        // Flags:       N Z C I D V
        //              / / - - - -
        // -------------------------------------------------------------------------------

        #define DO_INC data++;

        // -------------------------------------------------------------------------------
        case INC_zero_page:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case INC_zero_page_2:
            
            READ_FROM_ZERO_PAGE
            CONTINUE
            
        case INC_zero_page_3:
            
            WRITE_TO_ZERO_PAGE
            DO_INC
            CONTINUE
            
        case INC_zero_page_4:
            
            WRITE_TO_ZERO_PAGE_AND_SET_FLAGS
            DONE

        // -------------------------------------------------------------------------------
        case INC_zero_page_x:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case INC_zero_page_x_2:
            
            READ_FROM_ZERO_PAGE
            ADD_INDEX_X
            CONTINUE
            
        case INC_zero_page_x_3:
            
            READ_FROM_ZERO_PAGE
            CONTINUE
            
        case INC_zero_page_x_4:
            
            WRITE_TO_ZERO_PAGE
            DO_INC
            CONTINUE
            
        case INC_zero_page_x_5:
            
            WRITE_TO_ZERO_PAGE_AND_SET_FLAGS
            DONE

        // -------------------------------------------------------------------------------
        case INC_absolute:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case INC_absolute_2:
            
            FETCH_ADDR_HI
            CONTINUE
            
        case INC_absolute_3:
            
            READ_FROM_ADDRESS
            CONTINUE
            
        case INC_absolute_4:
            
            WRITE_TO_ADDRESS
            DO_INC
            CONTINUE
            
        case INC_absolute_5:
            
            WRITE_TO_ADDRESS_AND_SET_FLAGS
            DONE

        // -------------------------------------------------------------------------------
        case INC_absolute_x:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case INC_absolute_x_2:
            
            FETCH_ADDR_HI
            ADD_INDEX_X
            CONTINUE
            
        case INC_absolute_x_3:
            
            READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) { FIX_ADDR_HI }
            CONTINUE
            
        case INC_absolute_x_4:
            
            READ_FROM_ADDRESS
            CONTINUE
            
        case INC_absolute_x_5:
            
            WRITE_TO_ADDRESS
            DO_INC
            CONTINUE
            
        case INC_absolute_x_6:
            
            WRITE_TO_ADDRESS_AND_SET_FLAGS
            DONE

        // -------------------------------------------------------------------------------
        case INC_indirect_x:
            
            FETCH_POINTER_ADDR
            CONTINUE
            
        case INC_indirect_x_2:
            
            IDLE_READ_FROM_ADDRESS_INDIRECT
            ADD_INDEX_X_INDIRECT
            CONTINUE
            
        case INC_indirect_x_3:
            
            FETCH_ADDR_LO_INDIRECT
            CONTINUE
            
        case INC_indirect_x_4:
            
            FETCH_ADDR_HI_INDIRECT
            CONTINUE
            
        case INC_indirect_x_5:
            
            READ_FROM_ADDRESS
            CONTINUE
            
        case INC_indirect_x_6:
            
            WRITE_TO_ADDRESS
            DO_INC
            CONTINUE
            
        case INC_indirect_x_7:
            
            WRITE_TO_ADDRESS_AND_SET_FLAGS
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: INX
        //
        // Operation:   X := X + 1
        //
        // Flags:       N Z C I D V
        //              / / - - - -
        // -------------------------------------------------------------------------------

        case INX:
            
            IDLE_READ_IMPLIED
            loadX(getX()+1);
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: INY
        //
        // Operation:   Y := Y + 1
        //
        // Flags:       N Z C I D V
        //              / / - - - -
        // -------------------------------------------------------------------------------

        case INY:
            
            IDLE_READ_IMPLIED
            loadY(getY()+1);
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: JMP
        //
        // Operation:   PC := Operand
        //
        // Flags:       N Z C I D V
        //              - - - - - -
        // -------------------------------------------------------------------------------

        case JMP_absolute:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case JMP_absolute_2:
            
            FETCH_ADDR_HI
            setPC(LO_HI(addr_lo, addr_hi));
            DONE

        // -------------------------------------------------------------------------------
        case JMP_absolute_indirect:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case JMP_absolute_indirect_2:
            
            FETCH_ADDR_HI
            CONTINUE
            
        case JMP_absolute_indirect_3:
            
            READ_FROM_ADDRESS
            CONTINUE
            
        case JMP_absolute_indirect_4:
            
            setPCL(data);
            setPCH(mem->peek(addr_lo+1, addr_hi));
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: JSR
        //
        // Operation:   PC to stack, PC := Operand
        //
        // Flags:       N Z C I D V
        //              - - - - - -
        // -------------------------------------------------------------------------------
            
        case JSR:
            
            FETCH_ADDR_LO
            callStack[callStackPointer++] = PC;
            CONTINUE
            
        case JSR_2:
            
            CONTINUE
            
        case JSR_3:
            
            PUSH_PCH
            CONTINUE
            
        case JSR_4:
            
            PUSH_PCL
            CONTINUE
            
        case JSR_5:
            
            FETCH_ADDR_HI
            setPC(LO_HI(addr_lo, addr_hi));
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: LDA
        //
        // Operation:   A := M
        //
        // Flags:       N Z C I D V
        //              / / - - - -
        // -------------------------------------------------------------------------------

        // -------------------------------------------------------------------------------
        case LDA_immediate:
            
            READ_IMMEDIATE
            loadA(data);
            DONE

        // -------------------------------------------------------------------------------
        case LDA_zero_page:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case LDA_zero_page_2:
            
            READ_FROM_ZERO_PAGE
            loadA(data);
            DONE

        // -------------------------------------------------------------------------------
        case LDA_zero_page_x:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case LDA_zero_page_x_2:
            
            READ_FROM_ZERO_PAGE
            ADD_INDEX_X
            CONTINUE
            
        case LDA_zero_page_x_3:
            
            READ_FROM_ZERO_PAGE
            loadA(data);
            DONE

        // -------------------------------------------------------------------------------
        case LDA_absolute:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case LDA_absolute_2:
            
            FETCH_ADDR_HI
            CONTINUE
            
        case LDA_absolute_3:
            
            READ_FROM_ADDRESS
            loadA(data);
            DONE

        // -------------------------------------------------------------------------------
        case LDA_absolute_x:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case LDA_absolute_x_2:
            
            FETCH_ADDR_HI
            ADD_INDEX_X
            CONTINUE
            
        case LDA_absolute_x_3:
            
            READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) {
                FIX_ADDR_HI
                CONTINUE
            } else {
                loadA(data);
                DONE
            }
            
        case LDA_absolute_x_4:
            
            READ_FROM_ADDRESS
            loadA(data);
            DONE
            
        // -------------------------------------------------------------------------------
        case LDA_absolute_y:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case LDA_absolute_y_2:
            
            FETCH_ADDR_HI
            ADD_INDEX_Y
            CONTINUE
            
        case LDA_absolute_y_3:
            
            READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) {
                FIX_ADDR_HI
                CONTINUE
            } else {
                loadA(data);
                DONE
            }
            
        case LDA_absolute_y_4:
            
            READ_FROM_ADDRESS
            loadA(data);
            DONE

        // -------------------------------------------------------------------------------
        case LDA_indirect_x:
            
            FETCH_POINTER_ADDR
            CONTINUE
            
        case LDA_indirect_x_2:

            IDLE_READ_FROM_ADDRESS_INDIRECT
            ADD_INDEX_X_INDIRECT
            CONTINUE
            
        case LDA_indirect_x_3:
            
            FETCH_ADDR_LO_INDIRECT
            CONTINUE
            
        case LDA_indirect_x_4:
            
            FETCH_ADDR_HI_INDIRECT
            CONTINUE
            
        case LDA_indirect_x_5:
            
            READ_FROM_ADDRESS
            loadA(data);
            DONE

        // -------------------------------------------------------------------------------
        case LDA_indirect_y:

            FETCH_POINTER_ADDR
            CONTINUE
            
        case LDA_indirect_y_2:
            
            FETCH_ADDR_LO_INDIRECT
            CONTINUE
            
        case LDA_indirect_y_3:
            
            FETCH_ADDR_HI_INDIRECT
            ADD_INDEX_Y
            CONTINUE
            
        case LDA_indirect_y_4:
            
            READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) {
                FIX_ADDR_HI
                CONTINUE
            } else {
                loadA(data);
                DONE
            }
            
        case LDA_indirect_y_5:
            
            READ_FROM_ADDRESS
            loadA(data);
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: LDX
        //
        // Operation:   X := M
        //
        // Flags:       N Z C I D V
        //              / / - - - -
        // -------------------------------------------------------------------------------

        // -------------------------------------------------------------------------------
        case LDX_immediate:
            
            READ_IMMEDIATE
            loadX(data);
            DONE

        // -------------------------------------------------------------------------------
        case LDX_zero_page:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case LDX_zero_page_2:
            
            READ_FROM_ZERO_PAGE
            loadX(data);
            DONE

        // -------------------------------------------------------------------------------
        case LDX_zero_page_y:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case LDX_zero_page_y_2:
            
            READ_FROM_ZERO_PAGE
            ADD_INDEX_Y
            CONTINUE
            
        case LDX_zero_page_y_3:
            
            READ_FROM_ZERO_PAGE
            loadX(data);
            DONE

        // -------------------------------------------------------------------------------
        case LDX_absolute:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case LDX_absolute_2:
            
            FETCH_ADDR_HI
            CONTINUE
            
        case LDX_absolute_3:
            
            READ_FROM_ADDRESS
            loadX(data);
            DONE

        // -------------------------------------------------------------------------------
        case LDX_absolute_y:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case LDX_absolute_y_2:
            
            FETCH_ADDR_HI
            ADD_INDEX_Y
            CONTINUE
            
        case LDX_absolute_y_3:
            
            READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) {
                FIX_ADDR_HI
                CONTINUE
            } else {
                loadX(data);
                DONE
            }
            
        case LDX_absolute_y_4:
            
            READ_FROM_ADDRESS
            loadX(data);
            DONE

        // -------------------------------------------------------------------------------
        case LDX_indirect_x:
            
            FETCH_POINTER_ADDR
            CONTINUE
            
        case LDX_indirect_x_2:
            
            IDLE_READ_FROM_ADDRESS_INDIRECT
            ADD_INDEX_X_INDIRECT
            CONTINUE
            
        case LDX_indirect_x_3:
            
            FETCH_ADDR_LO_INDIRECT
            CONTINUE
            
        case LDX_indirect_x_4:
            
            FETCH_ADDR_HI_INDIRECT
            CONTINUE
            
        case LDX_indirect_x_5:
            
            READ_FROM_ADDRESS
            loadX(data);
            DONE

        // -------------------------------------------------------------------------------
        case LDX_indirect_y:
            
            FETCH_POINTER_ADDR
            CONTINUE
            
        case LDX_indirect_y_2:
            
            FETCH_ADDR_LO_INDIRECT
            CONTINUE
            
        case LDX_indirect_y_3:
            
            FETCH_ADDR_HI_INDIRECT
            ADD_INDEX_Y
            CONTINUE
            
        case LDX_indirect_y_4:
            
            READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) {
                FIX_ADDR_HI
                CONTINUE
            } else {
                loadX(data);
                DONE
            }
            
        case LDX_indirect_y_5:
            
            READ_FROM_ADDRESS
            loadX(data);
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: LDY
        //
        // Operation:   Y := M
        //
        // Flags:       N Z C I D V
        //              / / - - - -
        // -------------------------------------------------------------------------------

        // -------------------------------------------------------------------------------
        case LDY_immediate:

	READ_IMMEDIATE
	loadY(data);
	DONE

// -------------------------------------------------------------------------------
        case LDY_zero_page:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case LDY_zero_page_2:
            
            READ_FROM_ZERO_PAGE
            loadY(data);
            DONE

        // -------------------------------------------------------------------------------
        case LDY_zero_page_x:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case LDY_zero_page_x_2:
            
            READ_FROM_ZERO_PAGE
            ADD_INDEX_X
            CONTINUE
            
        case LDY_zero_page_x_3:
            
            READ_FROM_ZERO_PAGE
            loadY(data);
            DONE

        // -------------------------------------------------------------------------------
        case LDY_absolute:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case LDY_absolute_2:
            
            FETCH_ADDR_HI
            CONTINUE
            
        case LDY_absolute_3:
            
            READ_FROM_ADDRESS;
            loadY(data);
            DONE

        // -------------------------------------------------------------------------------
        case LDY_absolute_x:
            
            FETCH_ADDR_LO
            CONTINUE

        case LDY_absolute_x_2:
            
            FETCH_ADDR_HI
            ADD_INDEX_X
            CONTINUE
            
        case LDY_absolute_x_3:
            
            READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) {
                FIX_ADDR_HI
                CONTINUE
            } else {
                loadY(data);
                DONE
            }
            
        case LDY_absolute_x_4:
            
            READ_FROM_ADDRESS
            loadY(data);
            DONE

        // -------------------------------------------------------------------------------
        case LDY_indirect_x:
            
            FETCH_POINTER_ADDR
            CONTINUE
            
        case LDY_indirect_x_2:
            
            IDLE_READ_FROM_ADDRESS_INDIRECT
            ADD_INDEX_X_INDIRECT
            CONTINUE
            
        case LDY_indirect_x_3:
            
            FETCH_ADDR_LO_INDIRECT
            CONTINUE
            
        case LDY_indirect_x_4:
            
            FETCH_ADDR_HI_INDIRECT
            CONTINUE
            
        case LDY_indirect_x_5:
            
            READ_FROM_ADDRESS
            loadY(data);
            DONE

        // -------------------------------------------------------------------------------
        case LDY_indirect_y:
            
            FETCH_POINTER_ADDR
            CONTINUE
            
        case LDY_indirect_y_2:
            
            FETCH_ADDR_LO_INDIRECT
            CONTINUE
            
        case LDY_indirect_y_3:
            
            FETCH_ADDR_HI_INDIRECT
            ADD_INDEX_Y
            CONTINUE
            
        case LDY_indirect_y_4:
            
            READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) {
                FIX_ADDR_HI
                CONTINUE
            } else {
                loadY(data);
                DONE
            }
            
        case LDY_indirect_y_5:
            
            READ_FROM_ADDRESS
            loadY(data);
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: LSR
        //
        // Operation:   0 -> (A|M >> 1) -> C
        //
        // Flags:       N Z C I D V
        //              0 / / - - -
        // -------------------------------------------------------------------------------

        #define DO_LSR setC(data & 1); data = data >> 1;

        // -------------------------------------------------------------------------------
        case LSR_accumulator:
            
            IDLE_READ_IMPLIED
            setC(A & 1); loadA(A >> 1);
            DONE

        // -------------------------------------------------------------------------------
        case LSR_zero_page:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case LSR_zero_page_2:
            
            READ_FROM_ZERO_PAGE
            CONTINUE
            
        case LSR_zero_page_3:
            
            WRITE_TO_ZERO_PAGE
            DO_LSR
            CONTINUE
            
        case LSR_zero_page_4:
            
            WRITE_TO_ZERO_PAGE_AND_SET_FLAGS
            DONE
            
        // -------------------------------------------------------------------------------
        case LSR_zero_page_x:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case LSR_zero_page_x_2:
            
            READ_FROM_ZERO_PAGE
            ADD_INDEX_X
            CONTINUE
            
        case LSR_zero_page_x_3:
            
            READ_FROM_ZERO_PAGE
            CONTINUE
            
        case LSR_zero_page_x_4:
            
            WRITE_TO_ZERO_PAGE
            DO_LSR
            CONTINUE
            
        case LSR_zero_page_x_5:
            
            WRITE_TO_ZERO_PAGE_AND_SET_FLAGS
            DONE
            
        // -------------------------------------------------------------------------------
        case LSR_absolute:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case LSR_absolute_2:
            
            FETCH_ADDR_HI
            CONTINUE
            
        case LSR_absolute_3:
            
            READ_FROM_ADDRESS
            CONTINUE
            
        case LSR_absolute_4:
            
            WRITE_TO_ADDRESS
            DO_LSR
            CONTINUE
            
        case LSR_absolute_5:
            
            WRITE_TO_ADDRESS_AND_SET_FLAGS
            DONE
            
        // -------------------------------------------------------------------------------
        case LSR_absolute_x:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case LSR_absolute_x_2:
            
            FETCH_ADDR_HI
            ADD_INDEX_X
            CONTINUE
            
        case LSR_absolute_x_3:
            
            READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) {
                FIX_ADDR_HI
            }
            CONTINUE
            
        case LSR_absolute_x_4:
            
            READ_FROM_ADDRESS
            CONTINUE
            
        case LSR_absolute_x_5:
            
            WRITE_TO_ADDRESS
            DO_LSR
            CONTINUE
            
        case LSR_absolute_x_6:
            
            WRITE_TO_ADDRESS_AND_SET_FLAGS
            DONE

        // -------------------------------------------------------------------------------
        case LSR_absolute_y:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case LSR_absolute_y_2:
            
            FETCH_ADDR_HI
            ADD_INDEX_X
            CONTINUE
            
        case LSR_absolute_y_3:
            
            READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) {
                FIX_ADDR_HI
            }
            CONTINUE
            
        case LSR_absolute_y_4:
            
            READ_FROM_ADDRESS
            CONTINUE
            
        case LSR_absolute_y_5:
            
            WRITE_TO_ADDRESS
            DO_LSR
            CONTINUE
            
        case LSR_absolute_y_6:
            
            WRITE_TO_ADDRESS_AND_SET_FLAGS
            DONE

            // -------------------------------------------------------------------------------
        case LSR_indirect_x:
            
            FETCH_POINTER_ADDR
            CONTINUE
            
        case LSR_indirect_x_2:
            
            IDLE_READ_FROM_ADDRESS_INDIRECT
            ADD_INDEX_X_INDIRECT
            CONTINUE
            
        case LSR_indirect_x_3:
            
            FETCH_ADDR_LO_INDIRECT
            CONTINUE
            
        case LSR_indirect_x_4:
            
            FETCH_ADDR_HI_INDIRECT
            CONTINUE
            
        case LSR_indirect_x_5:
            
            READ_FROM_ADDRESS
            CONTINUE
            
        case LSR_indirect_x_6:
            
            WRITE_TO_ADDRESS
            DO_LSR
            CONTINUE
            
        case LSR_indirect_x_7:
            
            WRITE_TO_ADDRESS_AND_SET_FLAGS
            DONE

        // -------------------------------------------------------------------------------
        case LSR_indirect_y:
            
            FETCH_POINTER_ADDR
            CONTINUE
            
        case LSR_indirect_y_2:
            
            FETCH_ADDR_LO_INDIRECT
            CONTINUE
            
        case LSR_indirect_y_3:
            
            FETCH_ADDR_HI_INDIRECT
            ADD_INDEX_Y
            CONTINUE
            
        case LSR_indirect_y_4:
            
            READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) {
                FIX_ADDR_HI
            }
            CONTINUE
            
        case LSR_indirect_y_5:
            
            READ_FROM_ADDRESS
            CONTINUE
            
        case LSR_indirect_y_6:
            
            WRITE_TO_ADDRESS
            DO_LSR
            CONTINUE
            
        case LSR_indirect_y_7:
            
            WRITE_TO_ADDRESS_AND_SET_FLAGS
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: NOP
        //
        // Operation:   No operation
        //
        // Flags:       N Z C I D V
        //              - - - - - -
        // -------------------------------------------------------------------------------

        case NOP:
            
            IDLE_READ_IMPLIED
            DONE

        // -------------------------------------------------------------------------------
        case NOP_immediate:
            
            IDLE_READ_IMMEDIATE
            DONE

        // -------------------------------------------------------------------------------
        case NOP_zero_page:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case NOP_zero_page_2:
            
            READ_FROM_ZERO_PAGE
            DONE

        // -------------------------------------------------------------------------------
        case NOP_zero_page_x:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case NOP_zero_page_x_2:
            
            READ_FROM_ZERO_PAGE
            ADD_INDEX_X
            CONTINUE
            
        case NOP_zero_page_x_3:
            
            READ_FROM_ZERO_PAGE
            DONE

        // -------------------------------------------------------------------------------
        case NOP_absolute:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case NOP_absolute_2:
            
            FETCH_ADDR_HI
            CONTINUE
            
        case NOP_absolute_3:
            
            READ_FROM_ADDRESS
            DONE

        // -------------------------------------------------------------------------------
        case NOP_absolute_x:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case NOP_absolute_x_2:
            
            FETCH_ADDR_HI
            ADD_INDEX_X
            CONTINUE
            
        case NOP_absolute_x_3:
            
            READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) {
                FIX_ADDR_HI
                CONTINUE
            } else {
                DONE
            }
            
        case NOP_absolute_x_4:
            
            READ_FROM_ADDRESS
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: ORA
        //
        // Operation:   A := A v M
        //
        // Flags:       N Z C I D V
        //              / / - - - -
        // -------------------------------------------------------------------------------

        case ORA_immediate:
            
            READ_IMMEDIATE
            loadA(A | data);
            DONE

        // -------------------------------------------------------------------------------
        case ORA_absolute:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case ORA_absolute_2:
            FETCH_ADDR_HI
            CONTINUE
            
        case ORA_absolute_3:
            READ_FROM_ADDRESS
            loadA(A | data);
            DONE

        // -------------------------------------------------------------------------------
        case ORA_zero_page:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case ORA_zero_page_2:
            
            READ_FROM_ZERO_PAGE
            loadA(A | data);
            DONE

        // -------------------------------------------------------------------------------
        case ORA_zero_page_x:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case ORA_zero_page_x_2:
            
            READ_FROM_ZERO_PAGE
            ADD_INDEX_X
            CONTINUE
            
        case ORA_zero_page_x_3:
            
            READ_FROM_ZERO_PAGE
            loadA(A | data);
            DONE

        // -------------------------------------------------------------------------------
        case ORA_absolute_x:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case ORA_absolute_x_2:
            
            FETCH_ADDR_HI
            ADD_INDEX_X
            CONTINUE
            
        case ORA_absolute_x_3:
            
            READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) {
                FIX_ADDR_HI
                CONTINUE
            } else {
                loadA(A | data);
                DONE
            }
            
        case ORA_absolute_x_4:
            
            READ_FROM_ADDRESS
            loadA(A | data);
            DONE

        // -------------------------------------------------------------------------------
        case ORA_absolute_y:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case ORA_absolute_y_2:
            
            FETCH_ADDR_HI
            ADD_INDEX_Y
            CONTINUE
            
        case ORA_absolute_y_3:
            
            READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) {
                FIX_ADDR_HI
                CONTINUE
            } else {
                loadA(A | data);
                DONE
            }
            
        case ORA_absolute_y_4:
            
            READ_FROM_ADDRESS
            loadA(A | data);
            DONE

        // -------------------------------------------------------------------------------
        case ORA_indirect_x:
            
            FETCH_POINTER_ADDR
            CONTINUE
            
        case ORA_indirect_x_2:
            
            IDLE_READ_FROM_ADDRESS_INDIRECT
            ADD_INDEX_X_INDIRECT
            CONTINUE
            
        case ORA_indirect_x_3:
            
            FETCH_ADDR_LO_INDIRECT
            CONTINUE;
            
        case ORA_indirect_x_4:
            
            FETCH_ADDR_HI_INDIRECT
            CONTINUE
            
        case ORA_indirect_x_5:
            
            READ_FROM_ADDRESS
            loadA(A | data);
            DONE

        // -------------------------------------------------------------------------------
        case ORA_indirect_y:
            
            FETCH_POINTER_ADDR
            CONTINUE
            
        case ORA_indirect_y_2:
            
            FETCH_ADDR_LO_INDIRECT
            CONTINUE
            
        case ORA_indirect_y_3:
            
            FETCH_ADDR_HI_INDIRECT
            ADD_INDEX_Y
            CONTINUE
            
        case ORA_indirect_y_4:
            
            READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) {
                FIX_ADDR_HI
                CONTINUE
            } else {
                loadA(A | data);
                DONE
            }
            
        case ORA_indirect_y_5:
            
            READ_FROM_ADDRESS
            loadA(A | data);
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: PHA
        //
        // Operation:   A to stack
        //
        // Flags:       N Z C I D V
        //              - - - - - -
        // -------------------------------------------------------------------------------

        case PHA:
            
            IDLE_READ_IMPLIED
            CONTINUE
            
        case PHA_2:
            
            PUSH_A
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: PHA
        //
        // Operation:   P to stack
        //
        // Flags:       N Z C I D V
        //              - - - - - -
        // -------------------------------------------------------------------------------
            
        case PHP:
            
            IDLE_READ_IMPLIED
            CONTINUE
            
        case PHP_2:
            
            PUSH_P
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: PLA
        //
        // Operation:   Stack to A
        //
        // Flags:       N Z C I D V
        //              - - - - - -
        // -------------------------------------------------------------------------------

        case PLA:
            
            IDLE_READ_IMPLIED
            CONTINUE
            
        case PLA_2:
            
            SP++;
            CONTINUE
            
        case PLA_3:
            
            PULL_A
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: PLP
        //
        // Operation:   Stack to p
        //
        // Flags:       N Z C I D V
        //              / / / / / /
        // -------------------------------------------------------------------------------

        case PLP:
            
            IDLE_READ_IMPLIED
            CONTINUE
            
        case PLP_2:
            
            SP++;
            CONTINUE
            
        case PLP_3:
            
            // TODO: According to the doc, interrupts are polled first
            // POLL_IRQ_AND_NMI
            // PULL_P
            // DONE_NO_POLL
            PULL_P
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: ROL
        //
        //              -----------------------
        //              |                     |
        // Operation:   ---(A|M << 1) <- C <---
        //
        // Flags:       N Z C I D V
        //              / / / - - -
        // -------------------------------------------------------------------------------

            /*
inline uint8_t CPU::rol(uint8_t op)
{
	uint8_t bit8    = (op & 128);
	uint8_t shifted = (op << 1) + (getC() ? 1 : 0); 
	setC(bit8);
	return shifted;
}	
*/
            
        #define DO_ROL if (getC()) { setC(data & 128); data = (data << 1) + 1; } else { setC(data & 128); data = (data << 1); }

        // -------------------------------------------------------------------------------
        case ROL_accumulator:
            
            IDLE_READ_IMPLIED
            if (getC()) {
                setC(A & 128);
                loadA((A << 1) + 1);
            } else {
                setC(A & 128);
                loadA(A << 1);
            }
            DONE

        // -------------------------------------------------------------------------------
        case ROL_zero_page:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case ROL_zero_page_2:
            
            READ_FROM_ZERO_PAGE
            CONTINUE
            
        case ROL_zero_page_3:
            
            WRITE_TO_ZERO_PAGE
            DO_ROL
            CONTINUE
            
        case ROL_zero_page_4:
            
            WRITE_TO_ZERO_PAGE_AND_SET_FLAGS
            DONE

        // -------------------------------------------------------------------------------
        case ROL_zero_page_x:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case ROL_zero_page_x_2:
            
            READ_FROM_ZERO_PAGE
            ADD_INDEX_X
            CONTINUE
            
        case ROL_zero_page_x_3:
            
            READ_FROM_ZERO_PAGE
            CONTINUE
            
        case ROL_zero_page_x_4:
            
            WRITE_TO_ZERO_PAGE
            DO_ROL
            CONTINUE
            
        case ROL_zero_page_x_5:
            
            WRITE_TO_ZERO_PAGE_AND_SET_FLAGS
            DONE

        // -------------------------------------------------------------------------------
        case ROL_absolute:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case ROL_absolute_2:
            
            FETCH_ADDR_HI
            CONTINUE
            
        case ROL_absolute_3:
            
            READ_FROM_ADDRESS
            CONTINUE
            
        case ROL_absolute_4:
            
            WRITE_TO_ADDRESS
            DO_ROL
            CONTINUE
            
        case ROL_absolute_5:
            
            WRITE_TO_ADDRESS_AND_SET_FLAGS
            DONE

        // -------------------------------------------------------------------------------
        case ROL_absolute_x:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case ROL_absolute_x_2:
            
            FETCH_ADDR_HI
            ADD_INDEX_X
            CONTINUE
            
        case ROL_absolute_x_3:
            
            READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) { FIX_ADDR_HI }
            CONTINUE
            
        case ROL_absolute_x_4:
            
            READ_FROM_ADDRESS
            CONTINUE
            
        case ROL_absolute_x_5:
            
            WRITE_TO_ADDRESS
            DO_ROL
            CONTINUE
            
        case ROL_absolute_x_6:
            
            WRITE_TO_ADDRESS_AND_SET_FLAGS
            DONE

        // -------------------------------------------------------------------------------
        case ROL_indirect_x:
            
            FETCH_POINTER_ADDR
            CONTINUE
            
        case ROL_indirect_x_2:
            
            IDLE_READ_FROM_ADDRESS_INDIRECT
            ADD_INDEX_X_INDIRECT
            CONTINUE
            
        case ROL_indirect_x_3:
            
            FETCH_ADDR_LO_INDIRECT
            CONTINUE
            
        case ROL_indirect_x_4:
            
            FETCH_ADDR_HI_INDIRECT
            CONTINUE
            
        case ROL_indirect_x_5:
            
            READ_FROM_ADDRESS
            CONTINUE
            
        case ROL_indirect_x_6:
            
            WRITE_TO_ADDRESS
            DO_ROL
            CONTINUE
            
        case ROL_indirect_x_7:
            
            WRITE_TO_ADDRESS_AND_SET_FLAGS
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: ROR
        //
        //              -----------------------
        //              |                     |
        // Operation:   --->(A|M >> 1) -> C ---
        //
        // Flags:       N Z C I D V
        //              / / / - - -
        // -------------------------------------------------------------------------------

    /*
inline uint8_t CPU::ror(uint8_t op)
{
	uint8_t bit1    = (op & 1);
	uint8_t shifted = (op >> 1) + (getC() ? 128 : 0); 
	setC(bit1);
	return shifted;
}	
 */
    
        #define DO_ROR if (getC()) { setC(data & 1); data = (data >> 1) + 128; } else { setC(data & 1); data = (data >> 1); }

        // -------------------------------------------------------------------------------
        case ROR_accumulator:
            
            IDLE_READ_IMPLIED
            if (getC()) {
                setC(A & 1);
                loadA((A >> 1) + 128);
            } else {
                setC(A & 1);
                loadA(A >> 1);
            }
            DONE

        // -------------------------------------------------------------------------------
        case ROR_zero_page:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case ROR_zero_page_2:
            
            READ_FROM_ZERO_PAGE
            CONTINUE
            
        case ROR_zero_page_3:
            
            WRITE_TO_ZERO_PAGE
            DO_ROR
            CONTINUE
            
        case ROR_zero_page_4:
            
            WRITE_TO_ZERO_PAGE_AND_SET_FLAGS
            DONE

        // -------------------------------------------------------------------------------
        case ROR_zero_page_x:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case ROR_zero_page_x_2:
            
            READ_FROM_ZERO_PAGE
            ADD_INDEX_X
            CONTINUE
            
        case ROR_zero_page_x_3:
            
            READ_FROM_ZERO_PAGE
            CONTINUE
            
        case ROR_zero_page_x_4:
            
            WRITE_TO_ZERO_PAGE
            DO_ROR
            CONTINUE
            
        case ROR_zero_page_x_5:
            
            WRITE_TO_ZERO_PAGE_AND_SET_FLAGS
            DONE

        // -------------------------------------------------------------------------------
        case ROR_absolute:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case ROR_absolute_2:
            
            FETCH_ADDR_HI
            CONTINUE
            
        case ROR_absolute_3:
            
            READ_FROM_ADDRESS
            CONTINUE
            
        case ROR_absolute_4:
            
            WRITE_TO_ADDRESS
            DO_ROR
            CONTINUE
            
        case ROR_absolute_5:
            
            WRITE_TO_ADDRESS_AND_SET_FLAGS
            DONE

        // -------------------------------------------------------------------------------
        case ROR_absolute_x:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case ROR_absolute_x_2:
            
            FETCH_ADDR_HI
            ADD_INDEX_X
            CONTINUE
            
        case ROR_absolute_x_3:
            
            READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) {
                FIX_ADDR_HI
            }
            CONTINUE
            
        case ROR_absolute_x_4:
            
            READ_FROM_ADDRESS
            CONTINUE
            
        case ROR_absolute_x_5:
            
            WRITE_TO_ADDRESS
            DO_ROR
            CONTINUE
            
        case ROR_absolute_x_6:
            
            WRITE_TO_ADDRESS_AND_SET_FLAGS
            DONE

        // -------------------------------------------------------------------------------
        case ROR_indirect_x:
            
            FETCH_POINTER_ADDR
            CONTINUE
            
        case ROR_indirect_x_2:
            
            IDLE_READ_FROM_ADDRESS_INDIRECT
            ADD_INDEX_X_INDIRECT
            CONTINUE
            
        case ROR_indirect_x_3:
            
            FETCH_ADDR_LO_INDIRECT
            CONTINUE
            
        case ROR_indirect_x_4:
            
            FETCH_ADDR_HI_INDIRECT
            CONTINUE
            
        case ROR_indirect_x_5:
            
            READ_FROM_ADDRESS;
            CONTINUE
            
        case ROR_indirect_x_6:
            
            WRITE_TO_ADDRESS;
            DO_ROR;
            CONTINUE
            
        case ROR_indirect_x_7:
            
            WRITE_TO_ADDRESS_AND_SET_FLAGS
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: RTI
        //
        // Operation:   P from Stack, PC from Stack
        //
        // Flags:       N Z C I D V
        //              / / / / / /
        // -------------------------------------------------------------------------------

        case RTI:
            
            IDLE_READ_IMMEDIATE;
            CONTINUE
            
        case RTI_2:
            
            SP++;
            CONTINUE
            
        case RTI_3:
            
            PULL_P
            SP++;
            CONTINUE
            
        case RTI_4:
            
            PULL_PCL
            SP++;
            CONTINUE
            
        case RTI_5:
            
            PULL_PCH
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: RTS
        //
        // Operation:   PC from Stack
        //
        // Flags:       N Z C I D V
        //              - - - - - -
        // -------------------------------------------------------------------------------
            
        case RTS:
            
            IDLE_READ_IMMEDIATE
            CONTINUE
            
        case RTS_2:
            
            IDLE_READ_IMMEDIATE_SP
            CONTINUE
            
        case RTS_3:
            
            PULL_PCL
            SP++;
            CONTINUE
            
        case RTS_4:
            
            PULL_PCH
            CONTINUE
            
        case RTS_5:
            
            IDLE_READ_IMMEDIATE
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: SBC
        //
        // Operation:   A := A - M - (~C)
        //
        // Flags:       N Z C I D V
        //              / / / - - /
        // -------------------------------------------------------------------------------

        // -------------------------------------------------------------------------------
        case SBC_immediate:
            
            READ_IMMEDIATE
            sbc(data);
            DONE

        // -------------------------------------------------------------------------------
        case SBC_zero_page:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case SBC_zero_page_2:
            
            READ_FROM_ZERO_PAGE
            sbc(data);
            DONE

        // -------------------------------------------------------------------------------
        case SBC_zero_page_x:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case SBC_zero_page_x_2:
            
            READ_FROM_ZERO_PAGE
            ADD_INDEX_X
            CONTINUE
            
        case SBC_zero_page_x_3:
            
            READ_FROM_ZERO_PAGE
            sbc(data);
            DONE

        // -------------------------------------------------------------------------------
        case SBC_absolute:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case SBC_absolute_2:
            
            FETCH_ADDR_HI
            CONTINUE
            
        case SBC_absolute_3:
            
            READ_FROM_ADDRESS;
            sbc(data);
            DONE

        // -------------------------------------------------------------------------------
        case SBC_absolute_x:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case SBC_absolute_x_2:
            
            FETCH_ADDR_HI
            ADD_INDEX_X
            CONTINUE
            
        case SBC_absolute_x_3:
            
            READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) {
                FIX_ADDR_HI
                CONTINUE
            } else {
                sbc(data);
                DONE
            }
            
        case SBC_absolute_x_4:
            
            READ_FROM_ADDRESS
            sbc(data);
            DONE

        // -------------------------------------------------------------------------------
        case SBC_absolute_y:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case SBC_absolute_y_2:
            
            FETCH_ADDR_HI
            ADD_INDEX_Y
            CONTINUE
            
        case SBC_absolute_y_3:
            
            READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) {
                FIX_ADDR_HI
                CONTINUE
            } else {
                sbc(data);
                DONE
            }
            
        case SBC_absolute_y_4:
            
            READ_FROM_ADDRESS
            sbc(data);
            DONE

        // -------------------------------------------------------------------------------
        case SBC_indirect_x:
            
            FETCH_POINTER_ADDR
            CONTINUE
            
        case SBC_indirect_x_2:
            
            IDLE_READ_FROM_ADDRESS_INDIRECT
            ADD_INDEX_X_INDIRECT
            CONTINUE
            
        case SBC_indirect_x_3:
            
            FETCH_ADDR_LO_INDIRECT
            CONTINUE
            
        case SBC_indirect_x_4:
            
            FETCH_ADDR_HI_INDIRECT
            CONTINUE
            
        case SBC_indirect_x_5:
            
            READ_FROM_ADDRESS
            sbc(data);
            DONE

        // -------------------------------------------------------------------------------
        case SBC_indirect_y:
            
            FETCH_POINTER_ADDR
            CONTINUE
            
        case SBC_indirect_y_2:
            
            FETCH_ADDR_LO_INDIRECT
            CONTINUE
            
        case SBC_indirect_y_3:
            
            FETCH_ADDR_HI_INDIRECT
            ADD_INDEX_Y
            CONTINUE
            
        case SBC_indirect_y_4:
            
            READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) {
                FIX_ADDR_HI
                CONTINUE
            } else {
                sbc(data);
                DONE
            }
            
        case SBC_indirect_y_5:
            
            READ_FROM_ADDRESS
            sbc(data);
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: SEC
        //
        // Operation:   C := 1
        //
        // Flags:       N Z C I D V
        //              - - 1 - - -
        // -------------------------------------------------------------------------------

        case SEC:
            
            IDLE_READ_IMPLIED
            setC(1);
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: SED
        //
        // Operation:   D := 1
        //
        // Flags:       N Z C I D V
        //              - - - - 1 -
        // -------------------------------------------------------------------------------

        case SED:
            
            IDLE_READ_IMPLIED
            setD(1);
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: SEI
        //
        // Operation:   I := 1
        //
        // Flags:       N Z C I D V
        //              - - - 1 - -
        // -------------------------------------------------------------------------------

        case SEI:
            
            IDLE_READ_IMPLIED
            POLL_IRQ_AND_NMI
            
            oldI = I;
            setI(1);
            
            DONE_NO_POLL

        // -------------------------------------------------------------------------------
        // Instruction: STA
        //
        // Operation:   M := A
        //
        // Flags:       N Z C I D V
        //              - - - - - -
        // -------------------------------------------------------------------------------

        case STA_zero_page:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case STA_zero_page_2:
            
            data = A;
            WRITE_TO_ZERO_PAGE
            DONE

        // -------------------------------------------------------------------------------
        case STA_zero_page_x:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case STA_zero_page_x_2:
            
            IDLE_READ_FROM_ZERO_PAGE
            ADD_INDEX_X
            CONTINUE
            
        case STA_zero_page_x_3:
            
            data = A;
            WRITE_TO_ZERO_PAGE
            DONE

        // -------------------------------------------------------------------------------
        case STA_absolute:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case STA_absolute_2:
            
            FETCH_ADDR_HI
            CONTINUE
            
        case STA_absolute_3:
            
            data = A;
            WRITE_TO_ADDRESS
            DONE

        // -------------------------------------------------------------------------------
        case STA_absolute_x:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case STA_absolute_x_2:
            
            FETCH_ADDR_HI
            ADD_INDEX_X
            CONTINUE
            
        case STA_absolute_x_3:
            
            IDLE_READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) { FIX_ADDR_HI }
            CONTINUE
            
        case STA_absolute_x_4:
            
            data = A;
            WRITE_TO_ADDRESS
            DONE

        // -------------------------------------------------------------------------------
        case STA_absolute_y:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case STA_absolute_y_2:
            
            FETCH_ADDR_HI
            ADD_INDEX_Y
            CONTINUE
            
        case STA_absolute_y_3:
            
            IDLE_READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED)
                FIX_ADDR_HI
                CONTINUE
                
                case STA_absolute_y_4:
                
                data = A;
            WRITE_TO_ADDRESS
            DONE

        // -------------------------------------------------------------------------------
        case STA_indirect_x:
            
            FETCH_POINTER_ADDR
            CONTINUE
            
        case STA_indirect_x_2:
            
            IDLE_READ_FROM_ADDRESS_INDIRECT
            ADD_INDEX_X_INDIRECT
            CONTINUE
            
        case STA_indirect_x_3:
            
            FETCH_ADDR_LO_INDIRECT
            CONTINUE
            
        case STA_indirect_x_4:
            
            FETCH_ADDR_HI_INDIRECT
            CONTINUE
            
        case STA_indirect_x_5:
            
            data = A;
            WRITE_TO_ADDRESS
            DONE

        // -------------------------------------------------------------------------------
        case STA_indirect_y:
            
            FETCH_POINTER_ADDR
            CONTINUE
            
        case STA_indirect_y_2:
            
            FETCH_ADDR_LO_INDIRECT
            CONTINUE
            
        case STA_indirect_y_3:
            
            FETCH_ADDR_HI_INDIRECT
            ADD_INDEX_Y
            CONTINUE
            
        case STA_indirect_y_4:
            
            IDLE_READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) { FIX_ADDR_HI }
            CONTINUE
            
        case STA_indirect_y_5:
            
            data = A;
            WRITE_TO_ADDRESS
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: STX
        //
        // Operation:   M := X
        //
        // Flags:       N Z C I D V
        //              - - - - - -
        // -------------------------------------------------------------------------------

        case STX_zero_page:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case STX_zero_page_2:
            
            data = X;
            WRITE_TO_ZERO_PAGE
            DONE

        // -------------------------------------------------------------------------------
        case STX_zero_page_y:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case STX_zero_page_y_2:
            
            IDLE_READ_FROM_ZERO_PAGE
            ADD_INDEX_Y
            CONTINUE
            
        case STX_zero_page_y_3:
            
            data = X;
            WRITE_TO_ZERO_PAGE
            DONE

        // -------------------------------------------------------------------------------
        case STX_absolute:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case STX_absolute_2:
            
            FETCH_ADDR_HI
            CONTINUE
            
        case STX_absolute_3:
            
            data = X;
            WRITE_TO_ADDRESS
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: STY
        //
        // Operation:   M := Y
        //
        // Flags:       N Z C I D V
        //              - - - - - -
        // -------------------------------------------------------------------------------

        case STY_zero_page:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case STY_zero_page_2:
            
            data = Y;
            WRITE_TO_ZERO_PAGE
            DONE

        // -------------------------------------------------------------------------------
        case STY_zero_page_x:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case STY_zero_page_x_2:
            
            IDLE_READ_FROM_ZERO_PAGE
            ADD_INDEX_X
            CONTINUE
            
        case STY_zero_page_x_3:
            
            data = Y;
            WRITE_TO_ZERO_PAGE
            DONE

        // -------------------------------------------------------------------------------
        case STY_absolute:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case STY_absolute_2:
            
            FETCH_ADDR_HI
            CONTINUE
            
        case STY_absolute_3:
            
            data = Y;
            WRITE_TO_ADDRESS
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: TAX
        //
        // Operation:   X := A
        //
        // Flags:       N Z C I D V
        //              / / - - - -
        // -------------------------------------------------------------------------------

        case TAX:
            
            IDLE_READ_IMPLIED
            loadX(A);
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: TAY
        //
        // Operation:   Y := A
        //
        // Flags:       N Z C I D V
        //              / / - - - -
        // -------------------------------------------------------------------------------

        case TAY:
            
            IDLE_READ_IMPLIED
            loadY(A);
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: TSX
        //
        // Operation:   X := Stack pointer
        //
        // Flags:       N Z C I D V
        //              / / - - - -
        // -------------------------------------------------------------------------------

        case TSX:
            
            IDLE_READ_IMPLIED
            loadX(SP);
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: TXA
        //
        // Operation:   A := X
        //
        // Flags:       N Z C I D V
        //              / / - - - -
        // -------------------------------------------------------------------------------

        case TXA:
            
            IDLE_READ_IMPLIED
            loadA(X);
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: TXS
        //
        // Operation:   Stack pointer := X
        //
        // Flags:       N Z C I D V
        //              - - - - - -
        // -------------------------------------------------------------------------------

        case TXS:
            
            IDLE_READ_IMPLIED
            SP = X;
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: TYA
        //
        // Operation:   A := Y
        //
        // Flags:       N Z C I D V
        //              / / - - - -
        // -------------------------------------------------------------------------------

        case TYA:
            
            IDLE_READ_IMPLIED
            loadA(Y);
            DONE

        // -------------------------------------------------------------------------------
        // Illegal instructions
        // -------------------------------------------------------------------------------
        

        // -------------------------------------------------------------------------------
        // Instruction: ALR
        //
        // Operation:   AND, followed by LSR
        //
        // Flags:       N Z C I D V
        //              / / / - - -
        // -------------------------------------------------------------------------------

        case ALR_immediate:
            
            READ_IMMEDIATE
            A = A & data;
            setC(A & 1);
            loadA(A >> 1);
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: ANC
        //
        // Operation:   A := A & op,   N flag is copied to C
        //
        // Flags:       N Z C I D V
        //              / / / - - -
        // -------------------------------------------------------------------------------

        case ANC_immediate:
            
            READ_IMMEDIATE
            loadA(A & data);
            setC(getN());
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: ARR
        //
        // Operation:   AND, followed by ROR
        //
        // Flags:       N Z C I D V
        //              / / / - - /
        // -------------------------------------------------------------------------------

        case ARR_immediate:
        {
            READ_IMMEDIATE
            
            uint8_t tmp2 = A & data;
            
            // Taken from Frodo...
            A = (getC() ? (tmp2 >> 1) | 0x80 : tmp2 >> 1);
            if (!getD()) {
                setN(A & 0x80);
                setZ(A == 0);
                setC(A & 0x40);
                setV((A & 0x40) ^ ((A & 0x20) << 1));
            } else {
                int c_flag;
                
                setN(getC());
                setZ(A == 0);
                setV((tmp2 ^ A) & 0x40);
                if ((tmp2 & 0x0f) + (tmp2 & 0x01) > 5)
                    A = (A & 0xf0) | ((A + 6) & 0x0f);
                c_flag = (tmp2 + (tmp2 & 0x10)) & 0x1f0;
                if (c_flag > 0x50) {
                    setC(1);
                    A += 0x60;
                } else {
                    setC(0);
                }
            }
            DONE
        }

        // -------------------------------------------------------------------------------
        // Instruction: AXS
        //
        // Operation:   X = (A & X) - op
        //
        // Flags:       N Z C I D V
        //              / / / - - -
        // -------------------------------------------------------------------------------

        case AXS_immediate:
        {
            READ_IMMEDIATE
            
            uint8_t op2  = A & X;
            uint8_t tmp = op2 - data;
            
            setC(op2 >= data);
            loadX(tmp);
            DONE
        }

        // -------------------------------------------------------------------------------
        // Instruction: DCP
        //
        // Operation:   DEC followed by CMP
        //
        // Flags:       N Z C I D V
        //              / / / - - -
        // -------------------------------------------------------------------------------
            
        case DCP_zero_page:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case DCP_zero_page_2:
            
            READ_FROM_ZERO_PAGE
            CONTINUE
            
        case DCP_zero_page_3:
            
            WRITE_TO_ZERO_PAGE
            DO_DEC
            CONTINUE
            
        case DCP_zero_page_4:
            
            WRITE_TO_ZERO_PAGE_AND_SET_FLAGS
            cmp(A, data);
            DONE

        // -------------------------------------------------------------------------------
        case DCP_zero_page_x:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case DCP_zero_page_x_2:
            
            READ_FROM_ZERO_PAGE
            ADD_INDEX_X
            CONTINUE
            
        case DCP_zero_page_x_3:
            
            READ_FROM_ZERO_PAGE
            CONTINUE
            
        case DCP_zero_page_x_4:
            
            WRITE_TO_ZERO_PAGE
            DO_DEC
            CONTINUE
            
        case DCP_zero_page_x_5:
            
            WRITE_TO_ZERO_PAGE_AND_SET_FLAGS
            cmp(A, data);
            DONE

        // -------------------------------------------------------------------------------
        case DCP_absolute:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case DCP_absolute_2:
            
            FETCH_ADDR_HI
            CONTINUE
            
        case DCP_absolute_3:
            
            READ_FROM_ADDRESS
            CONTINUE
            
        case DCP_absolute_4:
            
            WRITE_TO_ADDRESS
            DO_DEC;
            CONTINUE
            
        case DCP_absolute_5:
            
            WRITE_TO_ADDRESS_AND_SET_FLAGS
            cmp(A, data);
            DONE

        // -------------------------------------------------------------------------------
        case DCP_absolute_x:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case DCP_absolute_x_2:
            
            FETCH_ADDR_HI
            ADD_INDEX_X
            CONTINUE
            
        case DCP_absolute_x_3:
            
            READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) { FIX_ADDR_HI }
            CONTINUE
            
        case DCP_absolute_x_4:
            
            READ_FROM_ADDRESS
            CONTINUE
            
        case DCP_absolute_x_5:
            
            WRITE_TO_ADDRESS
            DO_DEC
            CONTINUE
            
        case DCP_absolute_x_6:
            
            WRITE_TO_ADDRESS_AND_SET_FLAGS
            cmp(A, data);
            DONE

        // -------------------------------------------------------------------------------
        case DCP_absolute_y:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case DCP_absolute_y_2:
            
            FETCH_ADDR_HI
            ADD_INDEX_Y
            CONTINUE
            
        case DCP_absolute_y_3:
            
            READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) { FIX_ADDR_HI }
            CONTINUE
            
        case DCP_absolute_y_4:
            
            READ_FROM_ADDRESS
            CONTINUE
            
        case DCP_absolute_y_5:
            
            WRITE_TO_ADDRESS
            DO_DEC
            CONTINUE
            
        case DCP_absolute_y_6:
            
            WRITE_TO_ADDRESS_AND_SET_FLAGS
            cmp(A, data);
            DONE

        // -------------------------------------------------------------------------------
        case DCP_indirect_x:
            
            FETCH_POINTER_ADDR
            CONTINUE
            
        case DCP_indirect_x_2:
            
            IDLE_READ_FROM_ADDRESS_INDIRECT
            ADD_INDEX_X_INDIRECT
            CONTINUE
            
        case DCP_indirect_x_3:
            
            FETCH_ADDR_LO_INDIRECT
            CONTINUE
            
        case DCP_indirect_x_4:
            
            FETCH_ADDR_HI_INDIRECT
            CONTINUE
            
        case DCP_indirect_x_5:
            
            READ_FROM_ADDRESS
            CONTINUE
            
        case DCP_indirect_x_6:
            
            WRITE_TO_ADDRESS
            DO_DEC
            CONTINUE
            
        case DCP_indirect_x_7:
            
            WRITE_TO_ADDRESS_AND_SET_FLAGS
            cmp(A, data);
            DONE;

        // -------------------------------------------------------------------------------
        case DCP_indirect_y:
            
            FETCH_POINTER_ADDR
            CONTINUE
            
        case DCP_indirect_y_2:
            
            FETCH_ADDR_LO_INDIRECT
            CONTINUE
            
        case DCP_indirect_y_3:
            
            FETCH_ADDR_HI_INDIRECT
            ADD_INDEX_Y
            CONTINUE
            
        case DCP_indirect_y_4:
            
            READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) { FIX_ADDR_HI }
            CONTINUE
            
        case DCP_indirect_y_5:
            
            READ_FROM_ADDRESS
            CONTINUE
            
        case DCP_indirect_y_6:
            
            WRITE_TO_ADDRESS
            DO_DEC
            CONTINUE
            
        case DCP_indirect_y_7:
            
            WRITE_TO_ADDRESS_AND_SET_FLAGS
            cmp(A, data);
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: ISC
        //
        // Operation:   INC followed by SBC
        //
        // Flags:       N Z C I D V
        //              / / / - - /
        // -------------------------------------------------------------------------------

        case ISC_zero_page:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case ISC_zero_page_2:
            
            READ_FROM_ZERO_PAGE
            CONTINUE
            
        case ISC_zero_page_3:
            
            WRITE_TO_ZERO_PAGE
            DO_INC
            CONTINUE
            
        case ISC_zero_page_4:
            
            WRITE_TO_ZERO_PAGE_AND_SET_FLAGS
            sbc(data);
            DONE

        // -------------------------------------------------------------------------------
        case ISC_zero_page_x:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case ISC_zero_page_x_2:
            
            READ_FROM_ZERO_PAGE
            ADD_INDEX_X
            CONTINUE
            
        case ISC_zero_page_x_3:
            
            READ_FROM_ZERO_PAGE
            CONTINUE
            
        case ISC_zero_page_x_4:
            
            WRITE_TO_ZERO_PAGE
            DO_INC
            CONTINUE
            
        case ISC_zero_page_x_5:
            
            WRITE_TO_ZERO_PAGE_AND_SET_FLAGS
            sbc(data);
            DONE

        // -------------------------------------------------------------------------------
        case ISC_absolute:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case ISC_absolute_2:
            
            FETCH_ADDR_HI
            CONTINUE
            
        case ISC_absolute_3:
            
            READ_FROM_ADDRESS
            CONTINUE
            
        case ISC_absolute_4:
            
            WRITE_TO_ADDRESS
            DO_INC
            CONTINUE
            
        case ISC_absolute_5:
            
            WRITE_TO_ADDRESS_AND_SET_FLAGS
            sbc(data);
            DONE

        // -------------------------------------------------------------------------------
        case ISC_absolute_x:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case ISC_absolute_x_2:
            
            FETCH_ADDR_HI
            ADD_INDEX_X
            CONTINUE
            
        case ISC_absolute_x_3:
            
            READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) { FIX_ADDR_HI }
            CONTINUE
            
        case ISC_absolute_x_4:
            
            READ_FROM_ADDRESS
            CONTINUE
            
        case ISC_absolute_x_5:
            
            WRITE_TO_ADDRESS
            DO_INC
            CONTINUE
            
        case ISC_absolute_x_6:
            
            WRITE_TO_ADDRESS_AND_SET_FLAGS
            sbc(data);
            DONE

        // -------------------------------------------------------------------------------
        case ISC_absolute_y:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case ISC_absolute_y_2:
            
            FETCH_ADDR_HI
            ADD_INDEX_Y
            CONTINUE
            
        case ISC_absolute_y_3:
            
            READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) { FIX_ADDR_HI }
            CONTINUE
            
        case ISC_absolute_y_4:
            
            READ_FROM_ADDRESS
            CONTINUE
            
        case ISC_absolute_y_5:
            
            WRITE_TO_ADDRESS
            DO_INC
            CONTINUE
            
        case ISC_absolute_y_6:
            
            WRITE_TO_ADDRESS_AND_SET_FLAGS
            sbc(data);
            DONE

        // -------------------------------------------------------------------------------
        case ISC_indirect_x:
            
            FETCH_POINTER_ADDR
            CONTINUE
            
        case ISC_indirect_x_2:
            
            IDLE_READ_FROM_ADDRESS_INDIRECT
            ADD_INDEX_X_INDIRECT
            CONTINUE
            
        case ISC_indirect_x_3:
            
            FETCH_ADDR_LO_INDIRECT
            CONTINUE
            
        case ISC_indirect_x_4:
            
            FETCH_ADDR_HI_INDIRECT
            CONTINUE
            
        case ISC_indirect_x_5:
            
            READ_FROM_ADDRESS
            CONTINUE
            
        case ISC_indirect_x_6:
            
            WRITE_TO_ADDRESS
            DO_INC
            CONTINUE
            
        case ISC_indirect_x_7:
            
            WRITE_TO_ADDRESS_AND_SET_FLAGS
            sbc(data);
            DONE

        // -------------------------------------------------------------------------------
        case ISC_indirect_y:
            
            FETCH_POINTER_ADDR
            CONTINUE
            
        case ISC_indirect_y_2:
            
            FETCH_ADDR_LO_INDIRECT
            CONTINUE
            
        case ISC_indirect_y_3:
            
            FETCH_ADDR_HI_INDIRECT
            ADD_INDEX_Y
            CONTINUE
            
        case ISC_indirect_y_4:
            
            READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) { FIX_ADDR_HI }
            CONTINUE
            
        case ISC_indirect_y_5:
            
            READ_FROM_ADDRESS
            CONTINUE
            
        case ISC_indirect_y_6:
            
            WRITE_TO_ADDRESS
            DO_INC
            CONTINUE
            
        case ISC_indirect_y_7:
            
            WRITE_TO_ADDRESS_AND_SET_FLAGS
            sbc(data);
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: LAS
        //
        // Operation:   SP,X,A = op & SP
        //
        // Flags:       N Z C I D V
        //              / / - - - -
        // -------------------------------------------------------------------------------

        case LAS_absolute_y:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case LAS_absolute_y_2:
            
            FETCH_ADDR_HI
            ADD_INDEX_Y
            CONTINUE
            
        case LAS_absolute_y_3:
            
            READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) {
                FIX_ADDR_HI
                CONTINUE
            } else {
                data &= SP;
                SP = data;
                X = data;
                loadA(data);
                DONE
            }
            
        case LAS_absolute_y_4:
            
            READ_FROM_ADDRESS
            data &= SP;
            SP = data;
            X = data;
            loadA(data);
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: LAX
        //
        // Operation:   LDA, followed by LDX
        //
        // Flags:       N Z C I D V
        //              / / - - - -
        // -------------------------------------------------------------------------------

        case LAX_zero_page:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case LAX_zero_page_2:
            
            READ_FROM_ZERO_PAGE
            loadA(data);
            loadX(data);
            DONE

        // -------------------------------------------------------------------------------
        case LAX_zero_page_y:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case LAX_zero_page_y_2:
            
            READ_FROM_ZERO_PAGE
            ADD_INDEX_Y
            CONTINUE
            
        case LAX_zero_page_y_3:
            
            READ_FROM_ZERO_PAGE
            loadA(data);
            loadX(data);
            DONE

        // -------------------------------------------------------------------------------
        case LAX_absolute:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case LAX_absolute_2:
            
            FETCH_ADDR_HI
            CONTINUE
            
        case LAX_absolute_3:
            
            READ_FROM_ADDRESS;
            loadA(data);
            loadX(data);
            DONE

        // -------------------------------------------------------------------------------
        case LAX_absolute_y:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case LAX_absolute_y_2:
            
            FETCH_ADDR_HI
            ADD_INDEX_Y
            CONTINUE
            
        case LAX_absolute_y_3:
            
            READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) {
                FIX_ADDR_HI
                CONTINUE
            } else {
                loadA(data);
                loadX(data);
                DONE
            }
            
        case LAX_absolute_y_4:
            
            READ_FROM_ADDRESS
            loadA(data);
            loadX(data);
            DONE

        // -------------------------------------------------------------------------------
        case LAX_indirect_x:
            
            FETCH_POINTER_ADDR
            CONTINUE
            
        case LAX_indirect_x_2:
            
            IDLE_READ_FROM_ADDRESS_INDIRECT
            ADD_INDEX_X_INDIRECT
            CONTINUE
            
        case LAX_indirect_x_3:
            
            FETCH_ADDR_LO_INDIRECT
            CONTINUE
            
        case LAX_indirect_x_4:
            
            FETCH_ADDR_HI_INDIRECT
            CONTINUE
            
        case LAX_indirect_x_5:
            
            READ_FROM_ADDRESS
            loadA(data);
            loadX(data);
            DONE

        // -------------------------------------------------------------------------------
        case LAX_indirect_y:
            
            FETCH_POINTER_ADDR
            CONTINUE
            
        case LAX_indirect_y_2:
            
            FETCH_ADDR_LO_INDIRECT
            CONTINUE
            
        case LAX_indirect_y_3:
            
            FETCH_ADDR_HI_INDIRECT
            ADD_INDEX_Y
            CONTINUE
            
        case LAX_indirect_y_4:
            
            READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) {
                FIX_ADDR_HI
                CONTINUE
            } else {
                loadA(data);
                loadX(data);
                DONE
            }
            
        case LAX_indirect_y_5:
            
            READ_FROM_ADDRESS
            loadA(data);
            loadX(data);
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: RLA
        //
        // Operation:   ROL, followed by AND
        //
        // Flags:       N Z C I D V
        //              / / / - - -
        // -------------------------------------------------------------------------------

        // -------------------------------------------------------------------------------
        case RLA_zero_page:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case RLA_zero_page_2:
            
            READ_FROM_ZERO_PAGE
            CONTINUE
            
        case RLA_zero_page_3:
            
            WRITE_TO_ZERO_PAGE
            DO_ROL
            CONTINUE
            
        case RLA_zero_page_4:
            
            WRITE_TO_ZERO_PAGE
            loadA(A & data);
            DONE

        // -------------------------------------------------------------------------------
        case RLA_zero_page_x:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case RLA_zero_page_x_2:
            
            READ_FROM_ZERO_PAGE
            ADD_INDEX_X
            CONTINUE
            
        case RLA_zero_page_x_3:
            
            READ_FROM_ZERO_PAGE
            CONTINUE
            
        case RLA_zero_page_x_4:
            
            WRITE_TO_ZERO_PAGE
            DO_ROL
            CONTINUE
            
        case RLA_zero_page_x_5:
            
            WRITE_TO_ZERO_PAGE
            loadA(A & data);
            DONE

        // -------------------------------------------------------------------------------
        case RLA_absolute:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case RLA_absolute_2:
            
            FETCH_ADDR_HI
            CONTINUE
            
        case RLA_absolute_3:
            
            READ_FROM_ADDRESS
            CONTINUE
            
        case RLA_absolute_4:
            
            WRITE_TO_ADDRESS
            DO_ROL
            CONTINUE
            
        case RLA_absolute_5:
            
            WRITE_TO_ADDRESS
            loadA(A & data);
            DONE

        // -------------------------------------------------------------------------------
        case RLA_absolute_x:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case RLA_absolute_x_2:
            
            FETCH_ADDR_HI
            ADD_INDEX_X
            CONTINUE
            
        case RLA_absolute_x_3:
            
            READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) { FIX_ADDR_HI }
            CONTINUE
            
        case RLA_absolute_x_4:
            
            READ_FROM_ADDRESS
            CONTINUE
            
        case RLA_absolute_x_5:
            
            WRITE_TO_ADDRESS
            DO_ROL
            CONTINUE
            
        case RLA_absolute_x_6:
            
            WRITE_TO_ADDRESS
            loadA(A & data);
            DONE

        // -------------------------------------------------------------------------------
        case RLA_absolute_y:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case RLA_absolute_y_2:
            
            FETCH_ADDR_HI
            ADD_INDEX_Y
            CONTINUE
            
        case RLA_absolute_y_3:
            
            READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) { FIX_ADDR_HI }
            CONTINUE
            
        case RLA_absolute_y_4:
            
            READ_FROM_ADDRESS
            CONTINUE
            
        case RLA_absolute_y_5:
            
            WRITE_TO_ADDRESS
            DO_ROL
            CONTINUE
            
        case RLA_absolute_y_6:
            
            WRITE_TO_ADDRESS
            loadA(A & data);
            DONE

        // -------------------------------------------------------------------------------
        case RLA_indirect_x:
            
            FETCH_POINTER_ADDR
            CONTINUE
            
        case RLA_indirect_x_2:
            
            IDLE_READ_FROM_ADDRESS_INDIRECT
            ADD_INDEX_X_INDIRECT
            CONTINUE
            
        case RLA_indirect_x_3:
            
            FETCH_ADDR_LO_INDIRECT
            CONTINUE
            
        case RLA_indirect_x_4:
            
            FETCH_ADDR_HI_INDIRECT
            CONTINUE
            
        case RLA_indirect_x_5:
            
            READ_FROM_ADDRESS
            CONTINUE
            
        case RLA_indirect_x_6:
            
            WRITE_TO_ADDRESS
            DO_ROL
            CONTINUE
            
        case RLA_indirect_x_7:
            
            WRITE_TO_ADDRESS
            loadA(A & data);
            DONE

        // -------------------------------------------------------------------------------
        case RLA_indirect_y:
            
            FETCH_POINTER_ADDR
            CONTINUE
            
        case RLA_indirect_y_2:
            
            FETCH_ADDR_LO_INDIRECT
            CONTINUE
            
        case RLA_indirect_y_3:
            
            FETCH_ADDR_HI_INDIRECT
            ADD_INDEX_Y
            CONTINUE
            
        case RLA_indirect_y_4:
            
            READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) { FIX_ADDR_HI }
            CONTINUE
            
        case RLA_indirect_y_5:
            
            READ_FROM_ADDRESS
            CONTINUE
            
        case RLA_indirect_y_6:
            
            WRITE_TO_ADDRESS
            DO_ROL
            CONTINUE
            
        case RLA_indirect_y_7:
            
            WRITE_TO_ADDRESS
            loadA(A & data);
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: RRA
        //
        // Operation:   ROR, followed by ADC
        //
        // Flags:       N Z C I D V
        //              / / / - - /
        // -------------------------------------------------------------------------------

        // -------------------------------------------------------------------------------
        case RRA_zero_page:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case RRA_zero_page_2:
            
            READ_FROM_ZERO_PAGE
            CONTINUE
            
        case RRA_zero_page_3:
            
            WRITE_TO_ZERO_PAGE
            DO_ROR
            CONTINUE
            
        case RRA_zero_page_4:
            
            WRITE_TO_ZERO_PAGE
            adc(data);
            DONE

        // -------------------------------------------------------------------------------
        case RRA_zero_page_x:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case RRA_zero_page_x_2:
            
            READ_FROM_ZERO_PAGE
            ADD_INDEX_X
            CONTINUE
            
        case RRA_zero_page_x_3:
            
            READ_FROM_ZERO_PAGE
            CONTINUE
            
        case RRA_zero_page_x_4:
            
            WRITE_TO_ZERO_PAGE
            DO_ROR
            CONTINUE
            
        case RRA_zero_page_x_5:
            
            WRITE_TO_ZERO_PAGE
            adc(data);
            DONE

        // -------------------------------------------------------------------------------
        case RRA_absolute:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case RRA_absolute_2:
            
            FETCH_ADDR_HI
            CONTINUE
            
        case RRA_absolute_3:
            
            READ_FROM_ADDRESS
            CONTINUE
            
        case RRA_absolute_4:
            
            WRITE_TO_ADDRESS
            DO_ROR
            CONTINUE
            
        case RRA_absolute_5:
            
            WRITE_TO_ADDRESS
            adc(data);
            DONE

        // -------------------------------------------------------------------------------
        case RRA_absolute_x:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case RRA_absolute_x_2:
            
            FETCH_ADDR_HI
            ADD_INDEX_X
            CONTINUE
            
        case RRA_absolute_x_3:
            
            READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) { FIX_ADDR_HI }
            CONTINUE
            
        case RRA_absolute_x_4:
            
            READ_FROM_ADDRESS
            CONTINUE
            
        case RRA_absolute_x_5:
            
            WRITE_TO_ADDRESS
            DO_ROR
            CONTINUE
            
        case RRA_absolute_x_6:
            
            WRITE_TO_ADDRESS
            adc(data);
            DONE

        // -------------------------------------------------------------------------------
        case RRA_absolute_y:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case RRA_absolute_y_2:
            
            FETCH_ADDR_HI
            ADD_INDEX_Y
            CONTINUE
            
        case RRA_absolute_y_3:
            
            READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) { FIX_ADDR_HI }
            CONTINUE
            
        case RRA_absolute_y_4:
            
            READ_FROM_ADDRESS
            CONTINUE
            
        case RRA_absolute_y_5:
            
            WRITE_TO_ADDRESS
            DO_ROR
            CONTINUE
            
        case RRA_absolute_y_6:
            
            WRITE_TO_ADDRESS
            adc(data);
            DONE

        // -------------------------------------------------------------------------------
        case RRA_indirect_x:
            
            FETCH_POINTER_ADDR
            CONTINUE
            
        case RRA_indirect_x_2:
            
            IDLE_READ_FROM_ADDRESS_INDIRECT
            ADD_INDEX_X_INDIRECT
            CONTINUE
            
        case RRA_indirect_x_3:
            
            FETCH_ADDR_LO_INDIRECT
            CONTINUE
            
        case RRA_indirect_x_4:
            
            FETCH_ADDR_HI_INDIRECT
            CONTINUE
            
        case RRA_indirect_x_5:
            
            READ_FROM_ADDRESS
            CONTINUE
            
        case RRA_indirect_x_6:
            
            WRITE_TO_ADDRESS
            DO_ROR
            CONTINUE
            
        case RRA_indirect_x_7:
            
            WRITE_TO_ADDRESS
            adc(data);
            DONE

        // -------------------------------------------------------------------------------
        case RRA_indirect_y:
            
            FETCH_POINTER_ADDR
            CONTINUE
            
        case RRA_indirect_y_2:
            
            FETCH_ADDR_LO_INDIRECT
            CONTINUE
            
        case RRA_indirect_y_3:
            
            FETCH_ADDR_HI_INDIRECT
            ADD_INDEX_Y
            CONTINUE
            
        case RRA_indirect_y_4:
            
            READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) { FIX_ADDR_HI }
            CONTINUE
            
        case RRA_indirect_y_5:
            
            READ_FROM_ADDRESS
            CONTINUE
            
        case RRA_indirect_y_6:
            
            WRITE_TO_ADDRESS
            DO_ROR
            CONTINUE
            
        case RRA_indirect_y_7:
            
            WRITE_TO_ADDRESS
            adc(data);
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: SAX
        //
        // Operation:   Mem := A & X
        //
        // Flags:       N Z C I D V
        //              - - - - - -
        // -------------------------------------------------------------------------------

        case SAX_zero_page:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case SAX_zero_page_2:
            
            data = A & X;
            WRITE_TO_ZERO_PAGE
            DONE

        // -------------------------------------------------------------------------------
        case SAX_zero_page_y:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case SAX_zero_page_y_2:
            
            IDLE_READ_FROM_ZERO_PAGE
            ADD_INDEX_Y
            CONTINUE
            
        case SAX_zero_page_y_3:
            
            data = A & X;
            WRITE_TO_ZERO_PAGE
            DONE

        // -------------------------------------------------------------------------------
        case SAX_absolute:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case SAX_absolute_2:
            
            FETCH_ADDR_HI
            CONTINUE
            
        case SAX_absolute_3:
            
            data = A & X;
            WRITE_TO_ADDRESS
            DONE

        // -------------------------------------------------------------------------------
        case SAX_indirect_x:
            
            FETCH_POINTER_ADDR
            CONTINUE
            
        case SAX_indirect_x_2:
            
            IDLE_READ_FROM_ADDRESS_INDIRECT
            ADD_INDEX_X_INDIRECT
            CONTINUE
            
        case SAX_indirect_x_3:
            
            FETCH_ADDR_LO_INDIRECT
            CONTINUE
            
        case SAX_indirect_x_4:
            
            FETCH_ADDR_HI_INDIRECT
            CONTINUE
            
        case SAX_indirect_x_5:
            
            data = A & X;
            WRITE_TO_ADDRESS
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: SHA
        //
        // Operation:   Mem := A & X & (M + 1)
        //
        // Flags:       N Z C I D V
        //              - - - - - -
        // -------------------------------------------------------------------------------

        case SHA_absolute_y:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case SHA_absolute_y_2:
            
            FETCH_ADDR_HI
            ADD_INDEX_Y
            CONTINUE
            
        case SHA_absolute_y_3:
            
            IDLE_READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) { FIX_ADDR_HI }
            CONTINUE
            
        case SHA_absolute_y_4:
            
            data = A & X & (addr_hi + 1);
            WRITE_TO_ADDRESS
            DONE

        // -------------------------------------------------------------------------------
        case SHA_indirect_y:
            
            FETCH_POINTER_ADDR
            CONTINUE
            
        case SHA_indirect_y_2:
            
            FETCH_ADDR_LO_INDIRECT
            CONTINUE
            
        case SHA_indirect_y_3:
            
            FETCH_ADDR_HI_INDIRECT
            ADD_INDEX_Y
            CONTINUE
            
        case SHA_indirect_y_4:
            
            IDLE_READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) { FIX_ADDR_HI }
            CONTINUE
            
        case SHA_indirect_y_5:
            
            data = A & X & (addr_hi + 1);
            WRITE_TO_ADDRESS
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: SHX
        //
        // Operation:   Mem := X & (HI_BYTE(op) + 1)
        //
        // Flags:       N Z C I D V
        //              - - - - - -
        // -------------------------------------------------------------------------------

        case SHX_absolute_y:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case SHX_absolute_y_2:
            
            FETCH_ADDR_HI
            ADD_INDEX_Y
            CONTINUE
            
        case SHX_absolute_y_3:
            
            IDLE_READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) { FIX_ADDR_HI }
            CONTINUE
            
        case SHX_absolute_y_4:
            
            data = X & (addr_hi + 1);
            WRITE_TO_ADDRESS
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: SHY
        //
        // Operation:   Mem := Y & (HI_BYTE(op) + 1)
        //
        // Flags:       N Z C I D V
        //              - - - - - -
        // -------------------------------------------------------------------------------

        case SHY_absolute_x:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case SHY_absolute_x_2:
            
            FETCH_ADDR_HI
            ADD_INDEX_X
            CONTINUE
            
        case SHY_absolute_x_3:
            
            IDLE_READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) { FIX_ADDR_HI }
            CONTINUE
            
        case SHY_absolute_x_4:
            
            data = 	data = Y & (addr_hi + 1);
            WRITE_TO_ADDRESS
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: SLO (ASO)
        //
        // Operation:   ASL memory location, followed by OR on accumulator
        //
        // Flags:       N Z C I D V
        //              / / / - - -
        // -------------------------------------------------------------------------------

        #define DO_SLO setC(data & 128); data <<= 1;

        // -------------------------------------------------------------------------------
        case SLO_zero_page:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case SLO_zero_page_2:
            
            READ_FROM_ZERO_PAGE
            CONTINUE
            
        case SLO_zero_page_3:
            
            WRITE_TO_ZERO_PAGE
            DO_SLO
            CONTINUE
            
        case SLO_zero_page_4:
            
            WRITE_TO_ZERO_PAGE
            loadA(A | data);
            DONE

        // -------------------------------------------------------------------------------
        case SLO_zero_page_x:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case SLO_zero_page_x_2:
            
            READ_FROM_ZERO_PAGE
            ADD_INDEX_X
            CONTINUE
            
        case SLO_zero_page_x_3:
            
            READ_FROM_ZERO_PAGE
            CONTINUE
            
        case SLO_zero_page_x_4:
            
            WRITE_TO_ZERO_PAGE
            DO_SLO
            CONTINUE
            
        case SLO_zero_page_x_5:
            
            WRITE_TO_ZERO_PAGE
            loadA(A | data);
            DONE

        // -------------------------------------------------------------------------------
        case SLO_absolute:
            FETCH_ADDR_LO
            CONTINUE
            
        case SLO_absolute_2:
            
            FETCH_ADDR_HI
            CONTINUE
            
        case SLO_absolute_3:
            
            READ_FROM_ADDRESS
            CONTINUE
            
        case SLO_absolute_4:
            
            WRITE_TO_ADDRESS
            DO_SLO
            CONTINUE
            
        case SLO_absolute_5:
            
            WRITE_TO_ADDRESS
            loadA(A | data);
            DONE

        // -------------------------------------------------------------------------------
        case SLO_absolute_x:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case SLO_absolute_x_2:
            
            FETCH_ADDR_HI
            ADD_INDEX_X
            CONTINUE
            
        case SLO_absolute_x_3:
            
            READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) { FIX_ADDR_HI }
            CONTINUE
            
        case SLO_absolute_x_4:
            
            READ_FROM_ADDRESS
            CONTINUE
            
        case SLO_absolute_x_5:
            
            WRITE_TO_ADDRESS
            DO_SLO
            CONTINUE
            
        case SLO_absolute_x_6:
            
            WRITE_TO_ADDRESS
            loadA(A | data);
            DONE

        // -------------------------------------------------------------------------------
        case SLO_absolute_y:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case SLO_absolute_y_2:
            
            FETCH_ADDR_HI
            ADD_INDEX_Y
            CONTINUE
            
        case SLO_absolute_y_3:
            
            READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) { FIX_ADDR_HI }
            CONTINUE
            
        case SLO_absolute_y_4:
            
            READ_FROM_ADDRESS
            CONTINUE
            
        case SLO_absolute_y_5:
            
            WRITE_TO_ADDRESS
            DO_SLO
            CONTINUE
            
        case SLO_absolute_y_6:
            
            WRITE_TO_ADDRESS
            loadA(A | data);
            DONE

        // -------------------------------------------------------------------------------
        case SLO_indirect_x:
            
            FETCH_POINTER_ADDR
            CONTINUE
            
        case SLO_indirect_x_2:
            
            IDLE_READ_FROM_ADDRESS_INDIRECT
            ADD_INDEX_X_INDIRECT
            CONTINUE
            
        case SLO_indirect_x_3:
            
            FETCH_ADDR_LO_INDIRECT
            CONTINUE
            
        case SLO_indirect_x_4:
            
            FETCH_ADDR_HI_INDIRECT
            CONTINUE
            
        case SLO_indirect_x_5:
            
            READ_FROM_ADDRESS
            CONTINUE
            
        case SLO_indirect_x_6:
            
            WRITE_TO_ADDRESS
            DO_SLO
            CONTINUE
            
        case SLO_indirect_x_7:
            
            WRITE_TO_ADDRESS
            loadA(A | data);
            DONE

        // -------------------------------------------------------------------------------
        case SLO_indirect_y:
            
            FETCH_POINTER_ADDR
            CONTINUE
            
        case SLO_indirect_y_2:
            
            FETCH_ADDR_LO_INDIRECT
            CONTINUE
            
        case SLO_indirect_y_3:
            
            FETCH_ADDR_HI_INDIRECT
            ADD_INDEX_Y
            CONTINUE
            
        case SLO_indirect_y_4:
            
            READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) { FIX_ADDR_HI }
            CONTINUE
            
        case SLO_indirect_y_5:
            
            READ_FROM_ADDRESS
            CONTINUE
            
        case SLO_indirect_y_6:
            
            WRITE_TO_ADDRESS
            DO_SLO
            CONTINUE
            
        case SLO_indirect_y_7:
            WRITE_TO_ADDRESS
            loadA(A | data);
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: SRE (LSE)
        //
        // Operation:   LSR, followed by EOR
        //
        // Flags:       N Z C I D V
        //              / / / - - -
        // -------------------------------------------------------------------------------

        #define DO_SRE setC(data & 1); data >>= 1;

        // -------------------------------------------------------------------------------
        case SRE_zero_page:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case SRE_zero_page_2:
            
            READ_FROM_ZERO_PAGE
            CONTINUE
            
        case SRE_zero_page_3:
            
            WRITE_TO_ZERO_PAGE
            DO_SRE
            CONTINUE
            
        case SRE_zero_page_4:
            
            WRITE_TO_ZERO_PAGE
            loadA(A ^ data);
            DONE

        // -------------------------------------------------------------------------------
        case SRE_zero_page_x:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case SRE_zero_page_x_2:
            
            READ_FROM_ZERO_PAGE
            ADD_INDEX_X
            CONTINUE
            
        case SRE_zero_page_x_3:
            
            READ_FROM_ZERO_PAGE
            CONTINUE
            
        case SRE_zero_page_x_4:
            
            WRITE_TO_ZERO_PAGE
            DO_SRE
            CONTINUE
            
        case SRE_zero_page_x_5:
            
            WRITE_TO_ZERO_PAGE
            loadA(A ^ data);
            DONE

        // -------------------------------------------------------------------------------
        case SRE_absolute:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case SRE_absolute_2:
            
            FETCH_ADDR_HI
            CONTINUE
            
        case SRE_absolute_3:
            
            READ_FROM_ADDRESS
            CONTINUE
            
        case SRE_absolute_4:
            
            WRITE_TO_ADDRESS
            DO_SRE
            CONTINUE
            
        case SRE_absolute_5:
            
            WRITE_TO_ADDRESS
            loadA(A ^ data);
            DONE

        // -------------------------------------------------------------------------------
        case SRE_absolute_x:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case SRE_absolute_x_2:
            
            FETCH_ADDR_HI
            ADD_INDEX_X
            CONTINUE
            
        case SRE_absolute_x_3:
            
            READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) { FIX_ADDR_HI }
            CONTINUE
            
        case SRE_absolute_x_4:
            
            READ_FROM_ADDRESS
            CONTINUE
            
        case SRE_absolute_x_5:
            
            WRITE_TO_ADDRESS
            DO_SRE
            CONTINUE
            
        case SRE_absolute_x_6:
            
            WRITE_TO_ADDRESS
            loadA(A ^ data);
            DONE

        // -------------------------------------------------------------------------------
        case SRE_absolute_y:
            
            FETCH_ADDR_LO
            CONTINUE
            
        case SRE_absolute_y_2:
            
            FETCH_ADDR_HI
            ADD_INDEX_Y
            CONTINUE
            
        case SRE_absolute_y_3:
            
            READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) { FIX_ADDR_HI }
            CONTINUE
            
        case SRE_absolute_y_4:
            
            READ_FROM_ADDRESS
            CONTINUE
            
        case SRE_absolute_y_5:
            
            WRITE_TO_ADDRESS
            DO_SRE
            CONTINUE
            
        case SRE_absolute_y_6:
            
            WRITE_TO_ADDRESS
            loadA(A ^ data);
            DONE

        // -------------------------------------------------------------------------------
        case SRE_indirect_x:
            
            FETCH_POINTER_ADDR
            CONTINUE
            
        case SRE_indirect_x_2:
            
            IDLE_READ_FROM_ADDRESS_INDIRECT
            ADD_INDEX_X_INDIRECT
            CONTINUE
            
        case SRE_indirect_x_3:
            
            FETCH_ADDR_LO_INDIRECT
            CONTINUE
            
        case SRE_indirect_x_4:
            
            FETCH_ADDR_HI_INDIRECT
            CONTINUE
            
        case SRE_indirect_x_5:
            
            READ_FROM_ADDRESS
            CONTINUE
            
        case SRE_indirect_x_6:
            
            WRITE_TO_ADDRESS
            DO_SRE
            CONTINUE
            
        case SRE_indirect_x_7:
            
            WRITE_TO_ADDRESS
            loadA(A ^ data);
            DONE

        // -------------------------------------------------------------------------------
        case SRE_indirect_y:
            
            FETCH_POINTER_ADDR
            CONTINUE
            
        case SRE_indirect_y_2:
            
            FETCH_ADDR_LO_INDIRECT
            CONTINUE
            
        case SRE_indirect_y_3:
            
            FETCH_ADDR_HI_INDIRECT
            ADD_INDEX_Y
            CONTINUE
            
        case SRE_indirect_y_4:
            
            READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) { FIX_ADDR_HI }
            CONTINUE
            
        case SRE_indirect_y_5:
            
            READ_FROM_ADDRESS
            CONTINUE
            
        case SRE_indirect_y_6:
            
            WRITE_TO_ADDRESS
            DO_SRE
            CONTINUE
            
        case SRE_indirect_y_7:
            
            WRITE_TO_ADDRESS
            loadA(A ^ data);
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: TAS (SHS)
        //
        // Operation:   SP := A & X,  Mem := SP & (HI_BYTE(op) + 1)
        //
        // Flags:       N Z C I D V
        //              - - - - - -
        //
        // TODO: THIS IS MOST LIKELY IMPLEMENTED WRONG
        // -------------------------------------------------------------------------------

        case TAS_absolute_y:
            
            data = mem->peek(PC + 1) + 1;
            FETCH_ADDR_LO
            CONTINUE
            
        case TAS_absolute_y_2:
            
            FETCH_ADDR_HI;
            ADD_INDEX_Y;
            CONTINUE
            
        case TAS_absolute_y_3:
            
            IDLE_READ_FROM_ADDRESS
            if (PAGE_BOUNDARY_CROSSED) {
                FIX_ADDR_HI
            }
            // We always perform an extra cycle here, even if page boundary is not crossed.
            // Otherwise, the CPUTIMING test fails.
            CONTINUE
            
        case TAS_absolute_y_4:
            
            IDLE_READ_FROM_ADDRESS;
            SP = A & X;
            data &= SP;
            WRITE_TO_ADDRESS;
            DONE;

        // -------------------------------------------------------------------------------
        // Instruction: ANE
        //
        // Operation:   A = X & op & (A | 0xEE) (taken from Frodo)
        //
        // Flags:       N Z C I D V
        //              / / - - - -
        // -------------------------------------------------------------------------------

        case ANE_immediate:
            
            READ_IMMEDIATE
            loadA(X & data & (A | 0xEE));
            DONE

        // -------------------------------------------------------------------------------
        // Instruction: LXA
        //
        // Operation:   A = X = op & (A | 0xEE) (taken from Frodo)
        //
        // Flags:       N Z C I D V
        //              / / - - - -
        // -------------------------------------------------------------------------------

        case LXA_immediate:
            
            READ_IMMEDIATE
            X = data & (A | 0xEE);
            loadA(X);
            DONE

        default:
            debug("ERROR: UNIMPLEMENTED OPCODE: %d (%02X)\n", next, next);
    }
}

