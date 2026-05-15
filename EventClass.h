#pragma once
#include <vector>
#include <cstdint>
 

/*
The Hits/Events are stored to Bankx memory with the following data format:
31
 16
 15
 4 3
 0
Timestamp [47:32]
 Channel ID
 Format bits
Timestamp [31:16]
 Timestamp [15:0]
Index of Peakhigh value [15:0]
 Peakhigh value [15:0]
Information [7:0]
 Accumulator sum of Gate 1 [23:0]
"0000"
 Accumulator sum of Gate 2 [27:0]
"0000"
 Accumulator sum of Gate 3 [27:0]
"0000"
 Accumulator sum of Gate 4 [27:0]
"0000"
 Accumulator sum of Gate 5 [27:0]
"0000"
 Accumulator sum of Gate 6 [27:0]
"0000"
 Accumulator sum of Gate 7 [27:0]
"0000"
 Accumulator sum of Gate 8 [27:0]
"0000"
 MAW maximum value [27:0]
"0000"
 MAW value before Trigger [27:0]
"0000"
 MAW value after/with Trigger [27:0]
Start Energy value (Energy value from first value of Trigger Gate )
Max. Energy value (during Trigger Gate active )
If Format bit 0 = 1
If Format bit 1 = 1
If Format bit 2 = 1
If Format bit 3 = 1
31-28
 27
 26
MAW
0xE
 Test
 Status Flag
Flag
25-0
number of raw samples (x 2 samples, 32-bit words)
*/
struct Event {

    // fixed header
    /*     uint8_t formatBits; 

    If Format bit 0 = 1 : accumulator gates from 0 to 6
If Format bit 1 = 1 : accumulator gates 7 et 8
If Format bit 2 = 1 : MAW things
If Format bit 3 = 1 : energy valuues things 
    */
    uint8_t formatBits; 
    uint8_t channelId;
    uint64_t timestamp; // découpé en 3 parties dans le raw, mais on le reconstitue en 64-bit pour plus de commodité
    uint8_t  infoFlags;         // bit7=Overflow, bit6=Underflow, bit5=RePileup, bit4=Pileup
    uint8_t  statusFlag;        // Trigger source: Internal / External / Pileup
    uint16_t peakhighValue;     // Maximum ADC value in the gate window
    uint16_t peakhighIndex;     // Sample index of that maximum

    /* first options with formatBits & 0x1*/
    uint32_t accumulators[8];
/*second option with formatBits & 0x4*/
    uint32_t mawMaximum;        // MAW maximum value
    uint32_t mawBeforeTrigger;  // MAW value just before trigger
    uint32_t mawAfterTrigger;
/* third option with formatBits & 0x8*/
uint32_t energyStart;
uint32_t energyMax;

/* raw adc samples*/
    // Pre-allocated once (call reserveSamples() at startup).
    // numSamples tells how many are valid for this event.
    std::vector<uint32_t> rawSamples;  // 16 bit or 18bit values packed in 32-bit words
    uint32_t numSamples;               // nb of valid samples in rawSamples
 
    //  Reserve sample buffer once at startup to avoid
    //  repeated allocations during acquisition.
    //  maxSamples: maximum sample length you will ever use.
    void reserveSamples(size_t maxSamples)
    {
        rawSamples.resize(maxSamples);
        numSamples = 0;
    }
};


/*Callback type used by the ADC read-out loop. 
The Event reference is only valid for the duration of the calln copy if you need to keep the data.*/
using EventCallback = std::function<void(const Event&)>;
 
