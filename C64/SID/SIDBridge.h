/*!
 * @header      SIDBridge.h
 * @author      Dirk W. Hoffmann, www.dirkwhoffmann.de
 * @copyright   2011 - 2018 Dirk W. Hoffmann
 */
/*              This program is free software; you can redistribute it and/or modify
 *              it under the terms of the GNU General Public License as published by
 *              the Free Software Foundation; either version 2 of the License, or
 *              (at your option) any later version.
 *
 *              This program is distributed in the hope that it will be useful,
 *              but WITHOUT ANY WARRANTY; without even the implied warranty of
 *              MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *              GNU General Public License for more details.
 *
 *              You should have received a copy of the GNU General Public License
 *              along with this program; if not, write to the Free Software
 *              Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _SIDBRIDGE_H
#define _SIDBRIDGE_H

#include "VirtualComponent.h"
#include "FastSID.h"
#include "ReSID.h"
#include "SID_types.h"

class SIDBridge : public VirtualComponent {

    friend C64Memory;

private:

    //! @brief    FastSID (Adapted from VICE 3.1)
    FastSID fastsid;

    //! @brief    ReSID (Taken from VICE 3.1)
    ReSID resid;
   
    //! @brief    SID selector
    bool useReSID;
    
    //! @brief    Current clock cycle since power up
    uint64_t cycles;
    
private:
    
    //
    // Audio ringbuffer
    //
    
    //! @brief   Number of sound samples stored in ringbuffer
    static constexpr size_t bufferSize = 12288;
    
    /*! @brief   The audio sample ringbuffer.
     *  @details This ringbuffer serves as the data interface between the
     *           emulation code and the audio API (CoreAudio on Mac OS X).
     */
    float ringBuffer[bufferSize];
    
    /*! @brief   Scaling value for sound samples
     *  @details All sound samples produced by reSID are scaled by this
     *           value before they are written into the ringBuffer.
     */
    static constexpr float scale = 0.000005f;
    
    /*! @brief   Ring buffer read pointer
     */
    uint32_t readPtr;
    
    /*! @brief   Ring buffer write pointer
     */
    uint32_t writePtr;
    
    /*! @brief   Current volume
     *  @note    A value of 0 or below silences the audio playback.
     */
    int32_t volume;
    
    /*! @brief   Target volume
     *  @details Whenever an audio sample is written, the volume is
     *           increased or decreased by volumeDelta to make it reach
     *           the target volume eventually. This feature simulates a
     *           fading effect.
     */
    int32_t targetVolume;
    
    /*! @brief   Maximum volume
     */
    const static int32_t maxVolume = 100000;
    
    /*! @brief   Volume offset
     *  @details If the current volume does not match the target volume,
     *           it is increased or decreased by the specified amount. The
     *           increase or decrease takes place whenever an audio sample
     *           is generated.
     */
    int32_t volumeDelta;
    
public:
	
	//! @brief    Constructor
	SIDBridge();
	
	//! @brief    Destructor
	~SIDBridge();
			
    //! @brief    Method from VirtualComponent
    void reset();
    
    //! Load state
    void loadFromBuffer(uint8_t **buffer);
    
	//! @brief    Prints debug information
	void dumpState();
	
    
    //
	// Configuring the device
	//

    //! @brief    Configures the SID chip for being used in PAL machines.
    void setPAL();
    
    //! @brief    Configures the SID chip for being used in NTSC machines.
    void setNTSC();
    
    //! @brief    Returns true, whether ReSID or the old implementation should be used.
    bool getReSID() { return useReSID; }
    
    //! @brief    Enables or disables the ReSID library.
    void setReSID(bool enable);
    
    //! @brief    Returns the simulated chip model.
    SIDChipModel getChipModel();
    
    //! @brief    Sets chip model
    void setChipModel(SIDChipModel value);
    
    //! @brief    Returns true iff audio filters are enabled.
    bool getAudioFilter();
    
    //! @brief    Enables or disables filters of SID.
    void setAudioFilter(bool enable);
    
    //! @brief    Returns the sampling method.
    SamplingMethod getSamplingMethod();
    
    //! @brief    Sets the sampling method (ReSID only).
    void setSamplingMethod(SamplingMethod value);

    //! @brief    Returns the sample rate.
    uint32_t getSampleRate();
    
	//! @brief    Sets the samplerate of SID and it's 3 voices.
	void setSampleRate(uint32_t sr);
    
    //! @brief    Returns the clock frequency.
    uint32_t getClockFrequency();
    
	//! @brief    Sets the clock frequency.
	void setClockFrequency(uint32_t frequency);	

    //
    // Running the device
    //
    
    /*! @brief   Execute SID
     *  @details Runs reSID for the specified amount of CPU cycles and writes
     *           the generated sound samples into the internal ring buffer.
     */
    // virtual void execute(uint64_t cycles) = 0;
    
    //! Notifies the SID chip that the emulator has started
    void run();
    
    //! Notifies the SID chip that the emulator has started
    void halt();
    
    //
    // Volume control
    //
    
    /*! @brief Sets the current volume
     */
    void setVolume(int32_t vol) { volume = vol; }
    
    /*! @brief   Triggers volume ramp up phase
     *  @details Configures volume and targetVolume to simulate a smooth audio fade in
     */
    void rampUp() { targetVolume = maxVolume; volumeDelta = 3; }
    void rampUpFromZero() { volume = 0; targetVolume = maxVolume; volumeDelta = 3; }
    
    /*! @brief   Triggers volume ramp down phase
     *  @details Configures volume and targetVolume to simulate a quick audio fade out
     */
    void rampDown() { targetVolume = 0; volumeDelta = 50; }
    
    //
    // Ringbuffer handling
    //
    
    //! @brief  Clears the ringbuffer and resets the read and write pointer
    void clearRingbuffer();
    
    //! @brief  Reads a single audio sample from the ringbuffer
    float readData();
    
    /*! @brief   Reads a certain amount of samples from ringbuffer
     *  @details Samples are stored in a single mono stream
     */
    void readMonoSamples(float *target, size_t n);
    
    /*! @brief   Reads a certain amount of samples from ringbuffer
     *  @details Samples are stored in two seperate mono streams
     */
    void readStereoSamples(float *target1, float *target2, size_t n);
    
    /*! @brief   Reads a certain amount of samples from ringbuffer
     *  @details Samples are stored in an interleaved stereo stream
     */
    void readStereoSamplesInterleaved(float *target, size_t n);
    
    /*! @brief  Writes a certain number of audio samples into ringbuffer
     */
    void writeData(short *data, size_t count);
    
    /*! @brief   Handles a buffer underflow condition
     *  @details A buffer underflow occurs when the computer's audio device
     *           needs sound samples than SID hasn't produced, yet!
     */
    void handleBufferUnderflow();
    
    /*! @brief   Handles a buffer overflow condition
     *  @details A buffer overflow occurs when SID is producing more samples
     *           than the computer's audio device is able to consume
     */
    void handleBufferOverflow();
    
    //! @brief   Moves read pointer one position forward
    void advanceReadPtr() { readPtr = (readPtr + 1) % bufferSize; }
    
    //! @brief   Moves read pointer forward or backward
    void advanceReadPtr(int steps) { readPtr = (readPtr + bufferSize + steps) % bufferSize; }
    
    //! @brief   Moves write pointer one position forward
    void advanceWritePtr() { writePtr = (writePtr + 1) % bufferSize; }
    
    //! @brief   Moves write pointer forward or backward
    void advanceWritePtr(int steps) { writePtr = (writePtr + bufferSize + steps) % bufferSize; }
    
    //! @brief   Returns number of stored samples in ringbuffer
    unsigned samplesInBuffer() { return (writePtr + bufferSize - readPtr) % bufferSize; }
    
    //! @brief   Returns remaining storage capacity of ringbuffer
    unsigned bufferCapacity() { return (readPtr + bufferSize - writePtr) % bufferSize; }
    
    /*! @brief   Align write pointer
     *  @details This function puts the write pointer somewhat ahead of the read pointer.
     *           With a standard sample rate of 44100 Hz, 735 samples is 1/60 sec.
     */
    void alignWritePtr() { writePtr = (readPtr + (8 * 735)) % bufferSize; }
    
public:
    
    /*! @brief    Executes SID until a certain cycle is reached
     *  @param    cycle The target cycle
     */
    void executeUntil(uint64_t targetCycle);

    /*! @brief    Executes SID for a certain number of cycles
     *  @param    cycles Number of cycles to execute
     */
	void execute(uint64_t numCycles);

     
	//
	// Accessig device properties
	//
    
public:
    
	//! @brief    Special peek function for the I/O memory range.
	uint8_t peek(uint16_t addr);
	
    //! @brief    Same as peek, but without side effects.
    uint8_t spy(uint16_t addr);
    
	//! @brief    Special poke function for the I/O memory range.
	void poke(uint16_t addr, uint8_t value);
};

#endif
