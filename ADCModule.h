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

	/*
	On l'intègre ici et pas dans un fichier séparé car il n'existe que pour SetParameter() et GetParameter(). 
	Le séparer créerait une dépendance inutile. 
	Tout fichier qui inclut ADCModule.h obtient automatiquement l'enum.
	*/
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
	

    std::map<ADCParameterType,
             std::vector<float>> supportedParameterValues_;
		Callback cb_;
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

	void SetEventCallback(Callback cb) { cb_ = std::move(cb); }
/**********"setting and reading of parameters in a generalized way" ***********************/
	// "generalized way" = pas de méthode setSamplingRate(), setVoltageRange()...
// Une seule méthode qui couvre tous les paramètres via l'enum
// Le type du paramètre est passé en argument, pas encodé dans le nom

// // Ce qu'on évite :
// void setSamplingRate(float v);   // une méthode par paramètre
// void setVoltageRange(float v);   // si 9 paramètres -> 18 méthodes
// void setInputTermination(float v);// si nouveau chip -> tout réécrire

// Ce qu'on fait à la place :
bool SetParameter(ADCParameterType p, float value, int channel = -1);
float GetParameter(ADCParameterType p, int channel = -1) const;
// 2 méthodes pour tous les paramètres présents et futurs


/**********"virtual public bool SetParameter(ADCParameterType, float value) = 0"*******************/
virtual bool SetParameter(ADCParameterType param,
                           float value,
                           int channel = -1) = 0;
//                                     
//                        int channel 
//                        mais nécessaire : TriggerThreshold
//                        se configure par canal individuellement
//                        -1 = tous les canaux


/**********"virtual public ADCParameterType GetParameter(float value) = 0"***************/
// GetParameter(float value) n'a pas de sens :
// on passe une valeur pour obtenir un type de paramètre ?
// C'est l'inverse :on passe le type, on obtient la valeur
virtual float GetParameter(ADCParameterType param,
                            int channel = -1) const = 0; // retourne float (valeur courante), ADCParameterType est le param qu'on veut lire, channel est le canal concerné (si applicable), const : ne modifie pas l'état du module

/*********"public List<float> GetSupportedValues(ADCParameterType parameter)"***************/	
 // "List<float>" : std::vector<float> en C++
// Pas virtuelle pure -> implémentée directement dans ADCModule
// Même logique pour tous les chips -> pas besoin de surcharge

std::vector<float> GetSupportedValues(
    ADCParameterType param) const {
    auto it = supportedParameterValues_.find(param);
    if (it == supportedParameterValues_.end())
        return {}; // paramètre inconnu -> liste vide
    return it->second;
}						
/****************"Virtual public bool SaveToDisk(string filepath)"*****************/

virtual bool SaveToDisk(const std::string& filepath) const; // const& = pas de copie du string à l'appel, const = ne modifie pas l'état du module


};

#endif // ADCMODULE_H