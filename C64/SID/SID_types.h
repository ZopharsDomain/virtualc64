//
//  SID_types.h
//  V64
//
//  Created by Dirk Hoffmann on 15.04.18.
//

#ifndef SID_TYPES_H
#define SID_TYPES_H

/*! @brief    Sound chip models
 *  @details  This enum reflects enum "chip_model" used by reSID.
 */
typedef enum {
    MOS_6581,
    MOS_8580
} SIDChipModel;

/*! @brief    Sampling method
 *  @details  This enum reflects enum "sampling_method" used by reSID.
 */
typedef enum {
    SID_SAMPLE_FAST,
    SID_SAMPLE_INTERPOLATE,
    SID_SAMPLE_RESAMPLE,
    SID_SAMPLE_RESAMPLE_FASTMEM
} SamplingMethod;

/*! @brief    SID info
 *  @details  Used by SIDBridge::getInfo() to collect debug information
 */
typedef struct {
    struct {
        uint16_t frequency;
        uint16_t pulseWidth;
        uint8_t waveform;
        bool ringMod;
        bool hardSync;
        uint8_t attackRate;
        uint8_t decayRate;
        uint8_t sustainRate;
        uint8_t releaseRate;
        bool filterOn;
    } voice[3];
    uint8_t volume; 
    uint16_t filterCutoff;
    uint8_t filterType;
} SIDInfo;

#endif
