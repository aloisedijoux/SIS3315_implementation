/********************************************************************\

  Name:         abstract_class_midaspaqs.h
  Created by:   Aloise Dijoux

  Contents:     Common abstract interface for SIS3316 ADC modules,
              with MIDAS and PAQS concrete implementations.

\********************************************************************/


#ifndef ABSTRACT_CLASS_MIDASPAQS_H
#define ABSTRACT_CLASS_MIDASPAQS_H

#include <cstdint>
#include <array>
#include <vector>
#include <string>

//*********************************************/
//**abstract class common to both frameworks***/
//*********************************************/

class SIS3316Module {

protected:

    static constexpr int N_ADC_GROUPS = 4;
    static constexpr int N_CHANNELS   = 16;

    unsigned int module_address;
    std::array<uint32_t, N_ADC_GROUPS> event_length = {     };

public:

    SIS3316Module(unsigned int module_address) : module_address(module_address) {}
    virtual ~SIS3316Module() = default;

    //Initialisation
    // MIDAS : ReadModuleInformation() / ClearLinkErrorLatchBits() / ReadADCStatusRegisters()
    // PAQS  : InitSIS3316() (gather the three)
    virtual int ReadModuleInformation()    = 0;
    virtual int ReadBoardTemperature()     = 0;
    virtual int ClearLinkErrorLatchBits()  = 0;
    virtual int ReadADCStatusRegisters()   = 0;

    //Reset
    virtual int Reset() = 0;

    //clock
    // MIDAS : ConfigureClock()
    // PAQS  : setSamplingRateAndAllRegisters()
    virtual int ConfigureClock() = 0;

    //analogic front-end 
    // MIDAS : SetGainAndTermination()
    // PAQS  : setGainTerminationRegister() + SetVoltageRangeGlobal() + SetInputTerminationGlobal()
    virtual int SetGainAndTermination() = 0;
    // MIDAS : SetDACOffsets()
    // PAQS  : setOffsetRegister() + SetOffset()
    virtual int SetDACOffsets() = 0;

    //IO
    // MIDAS : SetNIMIO()
    // PAQS  : setIORegisters() + SetReactToTISignal()
    virtual int SetNIMIO() = 0;

    //channels
    // MIDAS : SetChannelHeader()
    // PAQS  : part of setMiscRegister()
    virtual int SetChannelHeader() = 0;

    //Trigger
    // MIDAS : SetTriggerConfiguration()
    // PAQS  : initTrigger() + setTriggerThresholdStateRegister() + SetTriggerState*() + SetTriggerThreshold*()
    virtual int SetTriggerConfiguration() = 0;
    // MIDAS : SetTriggerGateLength()
    // PAQS  : setTriggerGateWindowLengthRegister()
    virtual int SetTriggerGateLength() = 0;

    //Buffers
    // MIDAS : SetTraceBuffer() (also includes MAW, absent in PAQS)
    // PAQS  : setRawDataBufferConfigRegister()
    virtual int SetTraceBuffer() = 0;
    // MIDAS : SetPileUp()
    // PAQS  : setRepileupWindowRegister() + SetRepileupWindowGlobal()
    virtual int SetPileUp() = 0;

    //Data format
    // MIDAS : SetDataFormat()
    // PAQS  : setDataformatRegister()
    virtual int SetDataFormat() = 0;

    //common Getters
    unsigned int GetModuleAddress() const { return module_address; }
    std::array<uint32_t, N_ADC_GROUPS> GetEventLength() const { return event_length; }
};


//**********************************************************/
/************ MIDAS Implementation (frontends/) ************/
//**********************************************************/

#ifdef MIDAS_FRAMEWORK

#include "/home/aloiselkb/Documents/midpaqs/drivers/sis3316.h"
#include "/home/aloiselkb/Documents/midpaqs/drivers/sis3316_class.h"
#include "/home/aloiselkb/Documents/midpaqs/drivers/sis1100linux_vme_class.h"
#include "include/midas.h"
#include "include/SettingsHAL.h"

// Inherits from sis3316_adc to access driver methods

class SIS3316ModuleMIDAS : public SIS3316Module, public sis3316_adc {

public:

    SIS3316ModuleMIDAS(sis1100* crate, unsigned int baseaddr, int i_mod);

    //Overrides de la classe commune
    int ReadModuleInformation()    override;
    int ReadBoardTemperature()     override;
    int ClearLinkErrorLatchBits()  override;
    int ReadADCStatusRegisters()   override;
    int Reset()                    override;
    int ConfigureClock()           override;
    int SetGainAndTermination()    override;
    int SetDACOffsets()            override;
    int SetNIMIO()                 override;
    int SetChannelHeader()         override;
    int SetTriggerConfiguration()  override;
    int SetTriggerGateLength()     override;
    int SetTraceBuffer()           override;
    int SetPileUp()                override;
    int SetDataFormat()            override;

    //midas specific: absent de PAQS
    int ConnectODB();
    int SetFIREnergy();             
    int SetTriggerCoincidenceLogic(); 

    bool IsInitialized() const { return status == FE_SUCCESS; }
    int  GetStatus()     const { return status; }
    int  GetErrorCode()  const { return error_code; }

    std::array<uint32_t, N_ADC_GROUPS> GetRAWBufferLength() const { return raw_buffer_length; }
    std::array<uint32_t, N_ADC_GROUPS> GetMAWBufferLength() const { return maw_buffer_length; }

    std::vector<uint32_t> counts;
    void SetRates(std::vector<float>& values) { variables["Rate"]   = values; }
    void SetCounts()                          { variables["Counts"] = counts; }

private:

    Settings settings;
    Settings variables;
    int   status      = 0;
    int   error_code  = 1;
    int   i_module    = 0;
    float temperature = 0.f;

    std::array<uint32_t, N_ADC_GROUPS> raw_buffer_length = {};
    std::array<uint32_t, N_ADC_GROUPS> maw_buffer_length = {};
};

#endif // MIDAS_FRAMEWORK


//**********************************************************************/
/************ PAQS Implementation (PAQS_AcquisitionServer/) ************/
//**********************************************************************/
#ifdef PAQS_FRAMEWORK

#include "sis3316_class.h"
#include "sis1100w_vme_class.h"
#include "PAQSadcBasics.h"
#include "PAQSSignal.h"

// Composition instead of inheritance, sis3316_adc is a member pointer

class SIS3316ModulePAQS : public SIS3316Module {

public:

    SIS3316ModulePAQS(vme_interface_class* vme_crate, UINT module_address,
                      std::string name, bool isLVDSslave = false);
    ~SIS3316ModulePAQS();

    //Overrides de la classe commune
    // (mappés sur les méthodes privées set*/init* de PAQS)
    int ReadModuleInformation()    override; // part of InitSIS3316()
    int ReadBoardTemperature()     override;
    int ClearLinkErrorLatchBits()  override; // part of InitSIS3316()
    int ReadADCStatusRegisters()   override; // part of InitSIS3316()
    int Reset()                    override;
    int ConfigureClock()           override; // setSamplingRateAndAllRegisters()
    int SetGainAndTermination()    override; // setGainTerminationRegister()
    int SetDACOffsets()            override; // setOffsetRegister()
    int SetNIMIO()                 override; // setIORegisters()
    int SetChannelHeader()         override; // part of setMiscRegister()
    int SetTriggerConfiguration()  override; // initTrigger() + setTriggerThresholdStateRegister()
    int SetTriggerGateLength()     override; // setTriggerGateWindowLengthRegister()
    int SetTraceBuffer()           override; // setRawDataBufferConfigRegister()
    int SetPileUp()                override; // setRepileupWindowRegister()
    int SetDataFormat()            override; // setDataformatRegister()

//Specific to PAQS: acquisition cycle (absent from MIDAS, managed by the framework)
    UINT StartADC(bool timestampReset);
    UINT StopADC();
    UINT Update(UINT(*processSignal)(UINT, TIMESTAMP_ADC, Polarity, BYTE,
                                     const TRACE&, const TRACE&));
    void ForceTrigger();

    //Specific to PAQS: runtime configuration
    void SetReactToTISignal(bool enable);
    UINT SetTimeout(UINT ms);
    UINT SetSamplingRate(UINT rate);
    UINT SetPretriggerGlobal(UINT delay);
    UINT SetOversamplingGlobal(UINT flag);
    UINT SetSampleLengthGlobal(UINT length);
    UINT SetVoltageRangeGlobal(UINT flag);
    UINT SetInputTerminationGlobal(UINT flag);
    UINT SetRepileupWindowGlobal(UINT flag);
    UINT SetAdressThreshold(UINT noOfEvents);
    UINT SetOffset();
    UINT SetTriggerStateGlobal(UINT flag);
    UINT SetTriggerState(UINT chNo, UINT flag);
    UINT SetTriggerThresholdGlobal(UINT threshold);
    UINT SetTriggerThreshold(UINT chNo, UINT threshold);
    bool EnableAcquisitionOnFastClock(USHORT noOfSamples, USHORT noOfPretriggerSamples);
    void DisableAcquisitionOnFastClock();

private:

    vme_interface_class* vme_crate;
    sis3316_adc*         sis3316adc;  
    std::string          name;
    bool                 isLVDSslave = false;
    bool                 reactToTI   = true;
    UINT                 bank1_armed_flag;
    DWORD                timeOfLastReading;
    UINT                 timeout;

    TIMESTAMP_ADC lasttime[N_CHANNELS]    = {};
    UINT16        wrapcounter[N_CHANNELS] = {};

    UINT oversamplingFlag;
    UINT header_length;
    UINT inputtermination;
    UINT voltagerange;
    UINT samplingrate;
    UINT samplelength;
    UINT pretriggerDelayFlag;
    UINT repileupwindow;
    UINT adressthresholdNoOfEvents;
    UINT samplelengthFastAcquisition    = 0;
    UINT pretriggerDelayFastAcquisition = 0;

    std::array<UINT, N_CHANNELS> triggerStates     = {};
    std::array<UINT, N_CHANNELS> triggerThresholds = {};

    // Méthodes privées de bas niveau
    void InitSIS3316();
    void TryToRecoverFromADCFailure();
    void initTrigger();
    void RecalculateEventLength();

    UINT setSamplingRateAndAllRegisters();
    UINT setAllRegisters();
    UINT setIORegisters();
    UINT setMiscRegister();
    UINT setGainTerminationRegister();
    UINT setOffsetRegister();
    UINT setDataformatRegister();
    UINT setTriggerCounterRegister();
    UINT setTriggerThresholdStateRegister(UINT chNo);
    UINT setOversamplingSamplelengthPretriggerRegister();
    UINT setPretriggerRegister();
    UINT setTriggerGateWindowLengthRegister();
    UINT setRawDataBufferConfigRegister();
    UINT setRepileupWindowRegister();
    UINT setAdressThresholdRegister();

    Polarity DeterminePolarity(const TRACE& data, UINT triggerThreshold);
    USHORT   GetOversamplingFactorFromFlag(UINT flag) { return 2 << (flag >> 28); }
};

#endif // PAQS_FRAMEWORK

#endif // ABSTRACT_CLASS_MIDASPAQS_H
