/*
Classe abstraite ADCModule dont hériteront SIS3316 et SIS3315 et qui utilise un enum class ADCParameterType 
*/

#ifndef ADCMODULE_H
#define ADCMODULE_H
#include <map>
#include <vector>
#include <string>
#include "EventClass.h"
//pour it :
#include <iterator>
//pour std::function 
#include <functional>
#include <iostream>

// Le transfert se fait via le callback enregistré par l'appelant
// ADCModule produit -> callback consomme
// Aucun buffer intermédiaire, aucune copie supplémentaire
using Callback = std::function<void(const Event&)>; // const Event& = référence constante = pas de copie à l'appel

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

	// Attributs protégés : supportedParameterValues map
	protected:

		std::map<ADCParameterType, std::vector<int>> supportedParameterValues;
    // protected : les dérivées le remplissent, l'extérieur ne le voit pas
    // Rempli dans le constructeur dérivé :
    // supported_[Param::SamplingRate] = {62.5f, 125.0f, 250.0f}; // SIS3316
    // supported_[Param::SamplingRate] = {100.0f, 200.0f};         // SIS3315

	/*
		Méthodes publiques:
		+SetParameter(type, value) : bool
		+GetParameter(type) : float
		+GetSupportedValues(type) : vector<int>
		+SaveToDisk(filepath) : bool
		+LoadFromDisk(filepath) : bool
		+SoftwareTrigger() : int
		+StartReadoutLoop() : void
		+StopReadoutLoop() : void
		+SetEventCallback(cb) : void
	*/
public:
	virtual bool SetParameter(ADCParameterType type, int value) = 0;
// Implémenté dans ADCModule car logique identique pour tous les chips
std::vector<float> GetSupportedValues(ADCParameterType type) const {
    auto it = supportedParameterValues.find(type);
    return (it != supportedParameterValues.end()) ? it->second : std::vector<float>{};
}
// Pas virtuelle pure : même comportement partout, données différentes seulement	virtual std::vector<int> GetSupportedValues(ADCParameterType type) = 0;
	virtual bool SaveToDisk(const std::string& filepath) = 0;
	virtual bool LoadFromDisk(const std::string& filepath) = 0;
	virtual int SoftwareTrigger() = 0;
	virtual void StartReadoutLoop() = 0;
	virtual void StopReadoutLoop() = 0;
	virtual void SetEventCallback(void (*cb)(const std::vector<int>&)) = 0;
	virtual bool Connect()    = 0;
	virtual void Disconnect() = 0;
	virtual bool IsConnected() const = 0;
	virtual void parsese(Event) = 0;
	virtual void DispatchEvent(const Event& e) const = 0; // implémenté dans chaque dérivée, pas de logique commune possible
	virtual void StartAcquisition() = 0;
	void log(const std::string& message) const {
	// Implémentation de base : log vers la console
	// Les dérivées peuvent redéfinir pour log vers fichier, réseau, etc.
	std::cout << "[ADCModule] " << message << std::endl;
	}
};

#endif // ADCMODULE_H