/*
 * This file belongs to the FastSID implementation of VirtualC64,
 * an adaption of the code used in VICE 3.1, the Versatile Commodore Emulator.
 *
 * Original code written by
 *  Teemu Rantanen <tvr@cs.hut.fi>
 *  Michael Schwendt <sidplay@geocities.com>
 *  Ettore Perazzoli <ettore@comm2000.it>
 *
 * Adapted for VirtualC64 by
 *  Dirk Hoffmann
 */
/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "fastsid.h"
#include "waves.h"

// ADSR state
#define ATTACK   0
#define DECAY    1
#define SUSTAIN  2
#define RELEASE  3
#define IDLE     4

// Noise magic
#define NSHIFT(v, n) \
    (((v) << (n))    \
     | ((((v) >> (23 - (n))) ^ (v >> (18 - (n)))) & ((1 << (n)) - 1)))

#define NVALUE(v)                                   \
    (noiseLSB[v & 0xff] | noiseMID[(v >> 8) & 0xff] \
     | noiseMSB[(v >> 16) & 0xff])

#define NSEED 0x7ffff8

// Wave tables
static uint16_t wavetable00[2];
static uint16_t wavetable10[4096];
static uint16_t wavetable20[4096];
static uint16_t wavetable30[4096];
static uint16_t wavetable40[8192];
static uint16_t wavetable50[8192];
static uint16_t wavetable60[8192];
static uint16_t wavetable70[8192];

// Noise tables
static uint8_t noiseMSB[256];
static uint8_t noiseMID[256];
static uint8_t noiseLSB[256];

// Table for internal ADSR counter step calculations
static uint16_t adrtable[16] =
{
    1, 4, 8, 12, 19, 28, 34, 40, 50, 125, 250, 400, 500, 1500, 2500, 4000
};

// Table for pseudo-exponential ADSR calculations
static uint32_t exptable[6] =
{
    0x30000000, 0x1c000000, 0x0e000000, 0x08000000, 0x04000000, 0x00000000
};

// Clockcycles for each dropping bit when write-only register read is done
static uint32_t sidreadclocks[9];

static float lowPassParam[0x800];
static float bandPassParam[0x800];
static float filterResTable[16];
static const float filterRefFreq = 44100.0;
static signed char ampMod1x8[256];

/* manage temporary buffers. if the requested size is smaller or equal to the
 * size of the already allocated buffer, reuse it.  */
int16_t *buf = NULL;
int blen = 0;

static int16_t *getbuf(int len)
{
    if ((buf == NULL) || (blen < len)) {
        if (buf) {
            free(buf);
        }
        blen = len;
        buf = (int16_t *)calloc(len, 1);
    }
    return buf;
}

inline static void dofilter(voice_t *pVoice)
{
    if (!pVoice->filter) {
        return;
    }

    if (pVoice->s->filterType) {
        if (pVoice->s->filterType == 0x20) {
            pVoice->filtLow += pVoice->filtRef * pVoice->s->filterDy;
            pVoice->filtRef +=
                (pVoice->filtIO - pVoice->filtLow -
                          (pVoice->filtRef * pVoice->s->filterResDy)) *
                          pVoice->s->filterDy;
            pVoice->filtIO = (signed char)(pVoice->filtRef - pVoice->filtLow / 4);
        } else if (pVoice->s->filterType == 0x40) {
            float sample;
            pVoice->filtLow += (float)((pVoice->filtRef *
                                              pVoice->s->filterDy) * 0.1);
            pVoice->filtRef += (pVoice->filtIO - pVoice->filtLow -
                          (pVoice->filtRef * pVoice->s->filterResDy)) *
                          pVoice->s->filterDy;
            sample = pVoice->filtRef - (pVoice->filtIO / 8);
            if (sample < -128) {
                sample = -128;
            }
            if (sample > 127) {
                sample = 127;
            }
            pVoice->filtIO = (signed char)sample;
        } else {
            int tmp;
            float sample, sample2;
            pVoice->filtLow += pVoice->filtRef * pVoice->s->filterDy;
            sample = pVoice->filtIO;
            sample2 = sample - pVoice->filtLow;
            tmp = (int)sample2;
            sample2 -= pVoice->filtRef * pVoice->s->filterResDy;
            pVoice->filtRef += sample2 * pVoice->s->filterDy;

            pVoice->filtIO = pVoice->s->filterType == 0x10
                             ? (signed char)pVoice->filtLow :
                             (pVoice->s->filterType == 0x30
                              ? (signed char)pVoice->filtLow :
                              (pVoice->s->filterType == 0x50
                                   ? (signed char)
                                   ((int)(sample) - (tmp >> 1)) :
                                   (pVoice->s->filterType == 0x60
                                   ? (signed char)
                                   tmp :
                                   (pVoice->s->filterType == 0x70
                                   ? (signed char)
                                   ((int)(sample) - (tmp >> 1)) : 0))));
        }
    } else { /* filterType == 0x00 */
        pVoice->filtIO = 0;
    }
}

/* 15-bit oscillator value */
inline static uint32_t doosc(voice_t *pv)
{
    if (pv->noise) {
        return ((uint32_t)NVALUE(NSHIFT(pv->rv, pv->f >> 28))) << 7;
    }
    return pv->wt[(pv->f + pv->wtpf) >> pv->wtl] ^ pv->wtr[pv->vprev->f >> 31];
}

/* change ADSR state and all related variables */
static void set_adsr(voice_t *pv, uint8_t fm)
{
    int i;

    switch (fm) {
        case ATTACK:
            pv->adsrs = pv->s->adrs[pv->attack];
            pv->adsrz = 0;
            break;
        case DECAY:
            /* XXX: fix this */
            if (pv->adsr <= pv->s->sz[pv->sustain]) {
                set_adsr(pv, SUSTAIN);
                return;
            }
            for (i = 0; pv->adsr < exptable[i]; i++) {}
            pv->adsrs = -pv->s->adrs[pv->decay] >> i;
            pv->adsrz = pv->s->sz[pv->sustain];
            if (exptable[i] > pv->adsrz) {
                pv->adsrz = exptable[i];
            }
            break;
        case SUSTAIN:
            if (pv->adsr > pv->s->sz[pv->sustain]) {
                set_adsr(pv, DECAY);
                return;
            }
            pv->adsrs = 0;
            pv->adsrz = 0;
            break;
        case RELEASE:
            if (!pv->adsr) {
                set_adsr(pv, IDLE);
                return;
            }
            for (i = 0; pv->adsr < exptable[i]; i++) {}
            pv->adsrs = -pv->s->adrs[pv->release] >> i;
            pv->adsrz = exptable[i];
            break;
        case IDLE:
            pv->adsrs = 0;
            pv->adsrz = 0;
            break;
    }
    pv->adsrm = fm;
}

/* ADSR counter triggered state change */
static void trigger_adsr(voice_t *pv)
{
    switch (pv->adsrm) {
        case ATTACK:
            pv->adsr = 0x7fffffff;
            set_adsr(pv, DECAY);
            break;
        case DECAY:
        case RELEASE:
            if (pv->adsr >= 0x80000000) {
                pv->adsr = 0;
            }
            set_adsr(pv, pv->adsrm);
            break;
    }
}

/* update SID structure */
inline static void setup_sid(sound_t *psid)
{
    if (!psid->update) {
        return;
    }

    psid->vol = psid->d[0x18] & 0x0f;
    psid->has3 = ((psid->d[0x18] & 0x80) && !(psid->d[0x17] & 0x04)) ? 0 : 1;

    if (psid->emulatefilter) {
        psid->v[0].filter = psid->d[0x17] & 0x01 ? 1 : 0;
        psid->v[1].filter = psid->d[0x17] & 0x02 ? 1 : 0;
        psid->v[2].filter = psid->d[0x17] & 0x04 ? 1 : 0;
        psid->filterType = psid->d[0x18] & 0x70;
        if (psid->filterType != psid->filterCurType) {
            psid->filterCurType = psid->filterType;
            psid->v[0].filtLow = 0;
            psid->v[0].filtRef = 0;
            psid->v[1].filtLow = 0;
            psid->v[1].filtRef = 0;
            psid->v[2].filtLow = 0;
            psid->v[2].filtRef = 0;
        }
        psid->filterValue = 0x7ff & ((psid->d[0x15] & 7) | ((uint16_t)psid->d[0x16]) << 3);
        if (psid->filterType == 0x20) {
            psid->filterDy = bandPassParam[psid->filterValue];
        } else {
            psid->filterDy = lowPassParam[psid->filterValue];
        }
        psid->filterResDy = filterResTable[psid->d[0x17] >> 4]
                            - psid->filterDy;
        if (psid->filterResDy < 1.0) {
            psid->filterResDy = 1.0;
        }
    } else {
        psid->v[0].filter = 0;
        psid->v[1].filter = 0;
        psid->v[2].filter = 0;
    }
    psid->update = 0;
}

/* update voice structure */
inline static void setup_voice(voice_t *pv)
{
    if (!pv->update) {
        return;
    }

    pv->attack = pv->d[5] / 0x10;
    pv->decay = pv->d[5] & 0x0f;
    pv->sustain = pv->d[6] / 0x10;
    pv->release = pv->d[6] & 0x0f;
    pv->sync = pv->d[4] & 0x02 ? 1 : 0;
    pv->fs = pv->s->speed1 * (pv->d[0] + pv->d[1] * 0x100);

    if (pv->d[4] & 0x08) {
        pv->f = pv->fs = 0;
        pv->rv = NSEED;
    }
    pv->noise = 0;
    pv->wtl = 20;
    pv->wtpf = 0;
    pv->wtr[1] = 0;

    switch ((pv->d[4] & 0xf0) >> 4) {
        case 0:
            pv->wt = wavetable00;
            pv->wtl = 31;
            break;
        case 1:
            pv->wt = wavetable10;
            if (pv->d[4] & 0x04) {
                pv->wtr[1] = 0x7fff;
            }
            break;
        case 2:
            pv->wt = wavetable20;
            break;
        case 3:
            pv->wt = wavetable30;
            if (pv->d[4] & 0x04) {
                pv->wtr[1] = 0x7fff;
            }
            break;
        case 4:
            if (pv->d[4] & 0x08) {
                pv->wt = &wavetable40[4096];
            } else {
                pv->wt = &wavetable40[4096 - (pv->d[2]
                                              + (pv->d[3] & 0x0f) * 0x100)];
            }
            break;
        case 5:
            pv->wt = &wavetable50[pv->wtpf = 4096 - (pv->d[2]
                                                     + (pv->d[3] & 0x0f) * 0x100)];
            pv->wtpf <<= 20;
            if (pv->d[4] & 0x04) {
                pv->wtr[1] = 0x7fff;
            }
            break;
        case 6:
            pv->wt = &wavetable60[pv->wtpf = 4096 - (pv->d[2]
                                                     + (pv->d[3] & 0x0f) * 0x100)];
            pv->wtpf <<= 20;
            break;
        case 7:
            pv->wt = &wavetable70[pv->wtpf = 4096 - (pv->d[2]
                                                     + (pv->d[3] & 0x0f) * 0x100)];
            pv->wtpf <<= 20;
            if (pv->d[4] & 0x04 && pv->s->newsid) {
                pv->wtr[1] = 0x7fff;
            }
            break;
        case 8:
            pv->noise = 1;
            pv->wt = NULL;
            pv->wtl = 0;
            break;
        default:
            /* XXX: noise locking correct? */
            pv->rv = 0;
            pv->wt = wavetable00;
            pv->wtl = 31;
    }

    switch (pv->adsrm) {
        case ATTACK:
        case DECAY:
        case SUSTAIN:
            if (pv->d[4] & 0x01) {
                set_adsr(pv, (uint8_t)(pv->gateflip ? ATTACK : pv->adsrm));
            } else {
                set_adsr(pv, RELEASE);
            }
            break;
        case RELEASE:
        case IDLE:
            if (pv->d[4] & 0x01) {
                set_adsr(pv, ATTACK);
            } else {
                set_adsr(pv, pv->adsrm);
            }
            break;
    }
    pv->update = 0;
    pv->gateflip = 0;
}

static int16_t fastsid_calculate_single_sample(sound_t *psid, int i)
{
    uint32_t o0, o1, o2;
    int dosync1, dosync2;
    voice_t *v0, *v1, *v2;

    setup_sid(psid);
    v0 = &psid->v[0];
    setup_voice(v0);
    v1 = &psid->v[1];
    setup_voice(v1);
    v2 = &psid->v[2];
    setup_voice(v2);

    /* addfptrs, noise & hard sync test */
    dosync1 = 0;
    if ((v0->f += v0->fs) < v0->fs) {
        v0->rv = NSHIFT(v0->rv, 16);
        if (v1->sync) {
            dosync1 = 1;
        }
    }
    dosync2 = 0;
    if ((v1->f += v1->fs) < v1->fs) {
        v1->rv = NSHIFT(v1->rv, 16);
        if (v2->sync) {
            dosync2 = 1;
        }
    }
    if ((v2->f += v2->fs) < v2->fs) {
        v2->rv = NSHIFT(v2->rv, 16);
        if (v0->sync) {
            /* hard sync */
            v0->rv = NSHIFT(v0->rv, v0->f >> 28);
            v0->f = 0;
        }
    }

    /* hard sync */
    if (dosync2) {
        v2->rv = NSHIFT(v2->rv, v2->f >> 28);
        v2->f = 0;
    }
    if (dosync1) {
        v1->rv = NSHIFT(v1->rv, v1->f >> 28);
        v1->f = 0;
    }

    /* do adsr */
    if ((v0->adsr += v0->adsrs) + 0x80000000 < v0->adsrz + 0x80000000) {
        trigger_adsr(v0);
    }
    if ((v1->adsr += v1->adsrs) + 0x80000000 < v1->adsrz + 0x80000000) {
        trigger_adsr(v1);
    }
    if ((v2->adsr += v2->adsrs) + 0x80000000 < v2->adsrz + 0x80000000) {
        trigger_adsr(v2);
    }

    /* oscillators */
    o0 = v0->adsr >> 16;
    o1 = v1->adsr >> 16;
    o2 = v2->adsr >> 16;
    if (o0) {
        o0 *= doosc(v0);
    }
    if (o1) {
        o1 *= doosc(v1);
    }
    if (psid->has3 && o2) {
        o2 *= doosc(v2);
    } else {
        o2 = 0;
    }
    /* sample */
    if (psid->emulatefilter) {
        v0->filtIO = ampMod1x8[(o0 >> 22)];
        dofilter(v0);
        o0 = ((uint32_t)(v0->filtIO) + 0x80) << (7 + 15);
        v1->filtIO = ampMod1x8[(o1 >> 22)];
        dofilter(v1);
        o1 = ((uint32_t)(v1->filtIO) + 0x80) << (7 + 15);
        v2->filtIO = ampMod1x8[(o2 >> 22)];
        dofilter(v2);
        o2 = ((uint32_t)(v2->filtIO) + 0x80) << (7 + 15);
    }

    return (int16_t)(((int32_t)((o0 + o1 + o2) >> 20) - 0x600) * psid->vol);
}

int fastsid_calculate_samples(sound_t *psid, int16_t *pbuf, int nr,
                                     int interleave, int *delta_t)
{
    int i;
    int16_t *tmp_buf;

    if (psid->factor == 1000) {
        for (i = 0; i < nr; i++) {
            pbuf[i * interleave] = fastsid_calculate_single_sample(psid, i);
        }
        return nr;
    }
    tmp_buf = getbuf(2 * nr * psid->factor / 1000);
    for (i = 0; i < (nr * psid->factor / 1000); i++) {
        tmp_buf[i * interleave] = fastsid_calculate_single_sample(psid, i);
    }
    memcpy(pbuf, tmp_buf, 2 * nr);
    return nr;
}

static void init_filter(sound_t *psid, int freq)
{
    uint16_t uk;
    float rk;
    long int si;

    float yMax = 1.0;
    float yMin = (float)0.01;
    float resDyMax = 1.0;
    float resDyMin = 2.0;
    float resDy = resDyMin;

    float yAdd, yTmp;

    float filterFs = 400.0;
    float filterFm = 60.0;
    float filterFt = (float)0.05;

    float filterAmpl = 1.0;

    psid->filterValue = 0;
    psid->filterType = 0;
    psid->filterCurType = 0;
    psid->filterDy = 0;
    psid->filterResDy = 0;

    for (uk = 0, rk = 0; rk < 0x800; rk++, uk++) {
        float h;

        h = (float)((((exp(rk / 2048 * log(filterFs)) / filterFm) + filterFt) * filterRefFreq) / freq);
        if (h < yMin) {
            h = yMin;
        }
        if (h > yMax) {
            h = yMax;
        }
        lowPassParam[uk] = h;
    }

    yMax = (float)0.22;
    yMin = (float)0.002;
    yAdd = (float)((yMax - yMin) / 2048.0);
    yTmp = yMin;

    for (uk = 0, rk = 0; rk < 0x800; rk++, uk++) {
        bandPassParam[uk] = (yTmp * filterRefFreq) / freq;
        yTmp += yAdd;
    }

    for (uk = 0; uk < 16; uk++) {
        filterResTable[uk] = resDy;
        resDy -= ((resDyMin - resDyMax ) / 15);
    }

    filterResTable[0] = resDyMin;
    filterResTable[15] = resDyMax;

    /* XXX: if psid->emulatefilter = 0, ampMod1x8 is never referenced */
    if (psid->emulatefilter) {
        filterAmpl = (float)0.7;
    } else {
        filterAmpl = (float)1.0;
    }

    for (uk = 0, si = 0; si < 256; si++, uk++) {
        ampMod1x8[uk] = (signed char)((si - 0x80) * filterAmpl);
    }
}


int fastsid_init(sound_t *psid, int speed, int cycles_per_sec, int factor)
{
    uint32_t i;
    int sid_model;

    psid->factor = factor;

    psid->speed1 = (cycles_per_sec << 8) / speed;
    for (i = 0; i < 16; i++) {
        psid->adrs[i] = 500 * 8 * psid->speed1 / adrtable[i];
        psid->sz[i] = 0x8888888 * i;
    }
    psid->update = 1;

    /*
    if (resources_get_int("SidFilters", &(psid->emulatefilter)) < 0) {
        return 0;
    }
    */
    
    init_filter(psid, speed);
    setup_sid(psid);
    for (i = 0; i < 3; i++) {
        psid->v[i].vprev = &psid->v[(i + 2) % 3];
        psid->v[i].vnext = &psid->v[(i + 1) % 3];
        psid->v[i].nr = i;
        psid->v[i].d = psid->d + i * 7;
        psid->v[i].s = psid;
        psid->v[i].rv = NSEED;
        psid->v[i].filtLow = 0;
        psid->v[i].filtRef = 0;
        psid->v[i].filtIO = 0;
        psid->v[i].update = 1;
        setup_voice(&psid->v[i]);
    }

    /*
    if (resources_get_int("SidModel", &sid_model) < 0) {
        return 0;
    }
    */
    sid_model = 0;
    psid->newsid = 0;
    switch (sid_model) {
        default:
        case 0: /* 6581 */
        case 3: /* 6581R4 */
        case 4: /* DTVSID */
            psid->newsid = 0;
            break;
        case 1: /* 8580 */
        case 2: /* 8580 + digi boost */
            psid->newsid = 1;
            break;
    }

    for (i = 0; i < 4096; i++) {
        wavetable10[i] = (uint16_t)(i < 2048 ? i << 4 : 0xffff - (i << 4));
        wavetable20[i] = (uint16_t)(i << 3);
        wavetable30[i] = waveform30_8580[i] << 7;
        wavetable40[i + 4096] = 0x7fff;
        if (psid->newsid) {
            wavetable50[i + 4096] = waveform50_8580[i] << 7;
            wavetable60[i + 4096] = waveform60_8580[i] << 7;
            wavetable70[i + 4096] = waveform70_8580[i] << 7;
        } else {
            wavetable50[i + 4096] = waveform50_6581[i >> 3] << 7;
            wavetable60[i + 4096] = 0;
            wavetable70[i + 4096] = 0;
        }
    }

    for (i = 0; i < 256; i++) {
        noiseLSB[i] = (uint8_t)((((i >> (7 - 2)) & 0x04) | ((i >> (4 - 1)) & 0x02)
                              | ((i >> (2 - 0)) & 0x01)));
        noiseMID[i] = (uint8_t)((((i >> (13 - 8 - 4)) & 0x10)
                              | ((i << (3 - (11 - 8))) & 0x08)));
        noiseMSB[i] = (uint8_t)((((i << (7 - (22 - 16))) & 0x80)
                              | ((i << (6 - (20 - 16))) & 0x40)
                              | ((i << (5 - (16 - 16))) & 0x20)));
    }
    for (i = 0; i < 9; i++) {
        sidreadclocks[i] = 13;
    }

    return 1;
}

uint8_t fastsid_read(sound_t *psid, uint16_t addr)
{
    uint8_t ret;
    uint16_t ffix;
    uint32_t rvstore;
    // uint64_t tmp;

    switch (addr) {
        case 0x19:
            /* pot/x */
            ret = 0xff;
            break;
        case 0x1a:
            /* pot/y */
            ret = 0xff;
            break;
        case 0x1b:
            /* osc3 / random */
            ffix = (uint16_t)(42 * psid->v[2].fs);
            rvstore = psid->v[2].rv;
            if (psid->v[2].noise && psid->v[2].f + ffix < psid->v[2].f) {
                psid->v[2].rv = NSHIFT(psid->v[2].rv, 16);
            }
            psid->v[2].f += ffix;
            ret = (uint8_t)(doosc(&psid->v[2]) >> 7);
            psid->v[2].f -= ffix;
            psid->v[2].rv = rvstore;
            break;
        case 0x1c:
            ret = (uint8_t)(psid->v[2].adsr >> 23);
            break;
        default:
            /*
            while ((tmp = psid->laststorebit) &&
                   (tmp = psid->laststoreclk + sidreadclocks[tmp]) < maincpu_clk) {
                psid->laststoreclk = tmp;
                psid->laststore &= 0xfeff >> psid->laststorebit--;
            }
            */
            ret = psid->laststore;
    }

    return ret;
}

void fastsid_store(sound_t *psid, uint16_t addr, uint8_t byte)
{
    switch (addr) {
        case 4:
            if ((psid->d[addr] ^ byte) & 1) {
                psid->v[0].gateflip = 1;
            }
        case 0:
        case 1:
        case 2:
        case 3:
        case 5:
        case 6:
            psid->v[0].update = 1;
            break;
        case 11:
            if ((psid->d[addr] ^ byte) & 1) {
                psid->v[1].gateflip = 1;
            }
        case 7:
        case 8:
        case 9:
        case 10:
        case 12:
        case 13:
            psid->v[1].update = 1;
            break;
        case 18:
            if ((psid->d[addr] ^ byte) & 1) {
                psid->v[2].gateflip = 1;
            }
        case 14:
        case 15:
        case 16:
        case 17:
        case 19:
        case 20:
            psid->v[2].update = 1;
            break;
        default:
            psid->update = 1;
    }

    psid->d[addr] = byte;
    psid->laststore = byte;
    psid->laststorebit = 8;
}

/* ---------------------------------------------------------------------*/

/*
void fastsid_state_read(struct sound_s *psid, struct sid_fastsid_snapshot_state_s *sid_state)
{
    int i;

    sid_state->factor = (uint32_t)psid->factor;

    for (i = 0; i < 32; ++i) {
        sid_state->d[i] = psid->d[i];
    }

    sid_state->has3 = psid->has3;
    sid_state->vol = psid->vol;

    for (i = 0; i < 16; ++i) {
        sid_state->adrs[i] = psid->adrs[i];
        sid_state->sz[i] = psid->sz[i];
    }

    sid_state->speed1 = psid->speed1;
    sid_state->update = psid->update;
    sid_state->newsid = psid->newsid;
    sid_state->laststore = psid->laststore;
    sid_state->laststorebit = psid->laststorebit;
    sid_state->laststoreclk = (uint32_t)psid->laststoreclk;
    sid_state->emulatefilter = (uint32_t)psid->emulatefilter;
    sid_state->filterDy = (float)psid->filterDy;
    sid_state->filterResDy = (float)psid->filterResDy;
    sid_state->filterType = psid->filterType;
    sid_state->filterCurType = psid->filterCurType;
    sid_state->filterValue = psid->filterValue;

    for (i = 0; i < 3; ++i) {
        sid_state->v_nr[i] = (uint32_t)psid->v[i].nr;
        sid_state->v_f[i] = psid->v[i].f;
        sid_state->v_fs[i] = psid->v[i].fs;
        sid_state->v_noise[i] = psid->v[i].noise;
        sid_state->v_adsr[i] = psid->v[i].adsr;
        sid_state->v_adsrs[i] = psid->v[i].adsrs;
        sid_state->v_adsrz[i] = psid->v[i].adsrz;
        sid_state->v_sync[i] = psid->v[i].sync;
        sid_state->v_filter[i] = psid->v[i].filter;
        sid_state->v_update[i] = psid->v[i].update;
        sid_state->v_gateflip[i] = psid->v[i].gateflip;
        sid_state->v_adsrm[i] = psid->v[i].adsrm;
        sid_state->v_attack[i] = psid->v[i].attack;
        sid_state->v_decay[i] = psid->v[i].decay;
        sid_state->v_sustain[i] = psid->v[i].sustain;
        sid_state->v_release[i] = psid->v[i].release;
        sid_state->v_rv[i] = psid->v[i].rv;

        if (psid->v[i].wt >= &wavetable00[0] && psid->v[i].wt <= &wavetable00[1]) {
            sid_state->v_wt[i] = 0;
            sid_state->v_wt_offset[i] = psid->v[i].wt - &wavetable00[0];
        } else if (psid->v[i].wt >= &wavetable10[0] && psid->v[i].wt <= &wavetable10[4095]) {
            sid_state->v_wt[i] = 1;
            sid_state->v_wt_offset[i] = psid->v[i].wt - &wavetable10[0];
        } else if (psid->v[i].wt >= &wavetable20[0] && psid->v[i].wt <= &wavetable20[4095]) {
            sid_state->v_wt[i] = 2;
            sid_state->v_wt_offset[i] = psid->v[i].wt - &wavetable20[0];
        } else if (psid->v[i].wt >= &wavetable30[0] && psid->v[i].wt <= &wavetable30[4095]) {
            sid_state->v_wt[i] = 3;
            sid_state->v_wt_offset[i] = psid->v[i].wt - &wavetable30[0];
        } else if (psid->v[i].wt >= &wavetable40[0] && psid->v[i].wt <= &wavetable40[8191]) {
            sid_state->v_wt[i] = 4;
            sid_state->v_wt_offset[i] = psid->v[i].wt - &wavetable40[0];
        } else if (psid->v[i].wt >= &wavetable50[0] && psid->v[i].wt <= &wavetable50[8191]) {
            sid_state->v_wt[i] = 5;
            sid_state->v_wt_offset[i] = psid->v[i].wt - &wavetable50[0];
        } else if (psid->v[i].wt >= &wavetable60[0] && psid->v[i].wt <= &wavetable60[8191]) {
            sid_state->v_wt[i] = 6;
            sid_state->v_wt_offset[i] = psid->v[i].wt - &wavetable60[0];
        } else if (psid->v[i].wt >= &wavetable70[0] && psid->v[i].wt <= &wavetable70[8191]) {
            sid_state->v_wt[i] = 7;
            sid_state->v_wt_offset[i] = psid->v[i].wt - &wavetable70[0];
        } else {
            sid_state->v_wt[i] = 0;
            sid_state->v_wt_offset[i] = 0;
        }

        sid_state->v_wtpf[i] = psid->v[i].wtpf;
        sid_state->v_wtl[i] = psid->v[i].wtl;
        sid_state->v_wtr[0][i] = psid->v[i].wtr[0];
        sid_state->v_wtr[1][i] = psid->v[i].wtr[1];
        sid_state->v_filtIO[i] = (BYTE)psid->v[i].filtIO;
        sid_state->v_filtLow[i] = (float)psid->v[i].filtLow;
        sid_state->v_filtRef[i] = (float)psid->v[i].filtRef;
    }
}

void fastsid_state_write(struct sound_s *psid, struct sid_fastsid_snapshot_state_s *sid_state)
{
    int i;

    psid->factor = (int)sid_state->factor;

    for (i = 0; i < 32; ++i) {
        psid->d[i] = sid_state->d[i];
    }

    psid->has3 = sid_state->has3;
    psid->vol = sid_state->vol;

    for (i = 0; i < 16; ++i) {
        psid->adrs[i] = sid_state->adrs[i];
        psid->sz[i] = sid_state->sz[i];
    }

    psid->speed1 = sid_state->speed1;
    psid->update = sid_state->update;
    psid->newsid = sid_state->newsid;
    psid->laststore = sid_state->laststore;
    psid->laststorebit = sid_state->laststorebit;
    psid->laststoreclk = (CLOCK)sid_state->laststoreclk;
    psid->emulatefilter = (int)sid_state->emulatefilter;
    psid->filterDy = (float)sid_state->filterDy;
    psid->filterResDy = (float)sid_state->filterResDy;
    psid->filterType = psid->filterType;
    psid->filterCurType = sid_state->filterCurType;
    psid->filterValue = sid_state->filterValue;

    for (i = 0; i < 3; ++i) {
        psid->v[i].nr = (int)sid_state->v_nr[i];
        psid->v[i].f = sid_state->v_f[i];
        psid->v[i].fs = sid_state->v_fs[i];
        psid->v[i].noise = sid_state->v_noise[i];
        psid->v[i].adsr = sid_state->v_adsr[i];
        psid->v[i].adsrs = sid_state->v_adsrs[i];
        psid->v[i].adsrz = sid_state->v_adsrz[i];
        psid->v[i].sync = sid_state->v_sync[i];
        psid->v[i].filter = sid_state->v_filter[i];
        psid->v[i].update = sid_state->v_update[i];
        psid->v[i].gateflip = sid_state->v_gateflip[i];
        psid->v[i].adsrm = sid_state->v_adsrm[i];
        psid->v[i].attack = sid_state->v_attack[i];
        psid->v[i].decay = sid_state->v_decay[i];
        psid->v[i].sustain = sid_state->v_sustain[i];
        psid->v[i].release = sid_state->v_release[i];
        psid->v[i].rv = sid_state->v_rv[i];

        switch ((int)sid_state->v_wt[i]) {
            case 0:
                psid->v[i].wt = &wavetable00[sid_state->v_wt_offset[i]];
                break;
            case 1:
                psid->v[i].wt = &wavetable10[sid_state->v_wt_offset[i]];
                break;
            case 2:
                psid->v[i].wt = &wavetable20[sid_state->v_wt_offset[i]];
                break;
            case 3:
                psid->v[i].wt = &wavetable30[sid_state->v_wt_offset[i]];
                break;
            case 4:
                psid->v[i].wt = &wavetable40[sid_state->v_wt_offset[i]];
                break;
            case 5:
                psid->v[i].wt = &wavetable50[sid_state->v_wt_offset[i]];
                break;
            case 6:
                psid->v[i].wt = &wavetable60[sid_state->v_wt_offset[i]];
                break;
            case 7:
                psid->v[i].wt = &wavetable70[sid_state->v_wt_offset[i]];
                break;
        }

        psid->v[i].wtpf = sid_state->v_wtpf[i];
        psid->v[i].wtl = sid_state->v_wtl[i];
        psid->v[i].wtr[0] = sid_state->v_wtr[0][i];
        psid->v[i].wtr[1] = sid_state->v_wtr[1][i];
        psid->v[i].filtIO = (signed char)sid_state->v_filtIO[i];
        psid->v[i].filtLow = (float)sid_state->v_filtLow[i];
        psid->v[i].filtRef = (float)sid_state->v_filtRef[i];
    }
}
*/

