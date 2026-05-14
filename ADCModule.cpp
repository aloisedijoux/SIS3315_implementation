#include "ADCModule.h"

#include <fstream>
#include <iostream>

// Correspondance enum -> string pour sérialisation
static const std::map<ADCParameterType, std::string> kParamNames = {
    { ADCParameterType::SamplingRate,     "SamplingRate"     },
    { ADCParameterType::VoltageRange,     "VoltageRange"     },
    { ADCParameterType::InputTermination, "InputTermination" },
    { ADCParameterType::SampleLength,     "SampleLength"     },
    { ADCParameterType::PreTriggerLength, "PreTriggerLength" },
    { ADCParameterType::TriggerThreshold, "TriggerThreshold" },
    { ADCParameterType::TriggerEnable,    "TriggerEnable"    },
    { ADCParameterType::AveragingMode,    "AveragingMode"    },
    { ADCParameterType::ClockSource,      "ClockSource"      },
};

bool ADCModule::SaveToDisk(const std::string& filepath) const {
    std::ofstream out(filepath);
    if (!out.is_open()) {
        std::cerr << "[ADCModule] Cannot open: " << filepath << "\n";
        return false;
    }

    // En-tête lisible
    out << "# " << GetModelName()
        << " @ " << GetIdentifier() << "\n";

    // Pour chaque paramètre connu : lire la valeur courante
    // et l'écrire dans le fichier
    for (const auto& [param, name] : kParamNames) {
        try {
            float val = GetParameter(param);
            // GetParameter() est virtuelle pure
            // : appelle SIS3316Module::GetParameter()
            // : lit le registre hardware réel
            out << name << "=" << val << "\n";
        } catch (...) {
            // paramètre non lisible  donc on l'ignore
            // pas de crash pour un paramètre optionnel
        }
    }
    return true;
}

bool ADCModule::LoadFromDisk(const std::string& filepath) {
    std::ifstream in(filepath);
    if (!in.is_open()) {
        std::cerr << "[ADCModule] Cannot open: " << filepath << "\n";
        return false;
    }

    // Table inverse : string : enum
    std::map<std::string, ADCParameterType> nameToParam;
    for (const auto& [param, name] : kParamNames)
        nameToParam[name] = param;

    std::string line;
    int applied = 0;
    int errors  = 0;

    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue; // commentaires

        auto sep = line.find('=');
        if (sep == std::string::npos) continue;

        std::string key = line.substr(0, sep);
        float       val = std::stof(line.substr(sep + 1));

        auto it = nameToParam.find(key);
        if (it == nameToParam.end()) {
            std::cerr << "[ADCModule] Unknown: " << key << "\n";
            ++errors;
            continue;
        }

        // SetParameter() est virtuelle pure
        // : appelle SIS3316Module::SetParameter()
        // : écrit dans le registre hardware réel
        if (SetParameter(it->second, val)) ++applied;
        else                               ++errors;
    }

    std::cout << "[ADCModule] Loaded " << applied
              << " params (" << errors << " errors)\n";
    return (errors == 0);
}