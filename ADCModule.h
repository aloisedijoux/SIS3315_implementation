#ifndef ADCMODULE_H
#define ADCMODULE_H

#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "EventClass.h"

using Callback = std::function<void(const Event&)>; // crée un alias pour un type, en l'occurrencestd::function<void(const Event&)> c'est un type qui peut stocker n'importe quelle fonction qui prend un Event et ne retourne rien

// à chaque paramètre ADC (clé), on associe la liste des valeurs autorisées (valeur). Chaque classe dérivée remplit cette map à partir de la datasheet du constructeur.
enum class ADCParameterType {
    SamplingRate,
    VoltageRange,
    InputTermination,
    SampleLength,
    PreTriggerLength,
    TriggerThreshold,
    TriggerEnable,
    TIPort,
    TOPort,
    AveragingMode,
    ClockSource
};

class ADCModule {
protected:
    std::map<ADCParameterType, std::vector<float>> supportedParameterValues_;
    Callback cb_; 

public:
    virtual ~ADCModule() = default;

    // channel == -1 signifie tous les canaux
    virtual bool  SetParameter(ADCParameterType param, float value, int channel = -1) = 0;
    virtual float GetParameter(ADCParameterType param, int channel = -1) const = 0;

    // Implémentée ici : même logique pour tous les chips, seules les données diffèrent
    std::vector<float> GetSupportedValues(ADCParameterType param) const {
        auto it = supportedParameterValues_.find(param);
        return (it != supportedParameterValues_.end()) ? it->second : std::vector<float>{};
    }

    virtual bool SaveToDisk(const std::string& filepath) const = 0;
    virtual bool LoadFromDisk(const std::string& filepath) = 0;

    virtual bool Connect() = 0;
    virtual void Disconnect() = 0;
    virtual bool IsConnected() const = 0;

    virtual int  SoftwareTrigger() = 0;
    virtual void StartAcquisition() = 0;
    virtual void StartReadoutLoop() = 0;
    virtual void StopReadoutLoop() = 0;

    virtual void ParseEvent(Event e) = 0;
    virtual void DispatchEvent(const Event& e) const = 0;

    void SetEventCallback(Callback cb) { cb_ = std::move(cb); }

    void log(const std::string& message) const {
        std::cout << "[ADCModule] " << message << std::endl;
    }
};

#endif // ADCMODULE_H
