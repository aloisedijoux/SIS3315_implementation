#pragma once
#include <vector>
#include <cstdint>

/**
 * @file Event.h
 * @brief Structure normalisée d'un événement ADC.
 *
 * POURQUOI ce fichier existe AVANT ADCModule.h ?
 *   ADCModule.h a besoin du type Event pour son callback.
 *   On définit d'abord le "quoi" (la donnée), puis le "comment" (l'interface).
 *
 * INDÉPENDANCE HARDWARE :
 *   Cette structure ne contient aucun type du SDK constructeur.
 *   Elle est identique que l'Event vienne d'un SIS3315 ou d'un SIS3316.
 *   C'est le format de sortie universel du système.
 */


class Event {
    public:
    //ctor 
    Event() = default; // constructeur par défaut
    Event(const Event&) = default; // constructeur de copie
};
