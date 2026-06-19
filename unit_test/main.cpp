#include "progress.h"

static const char CONFIG_FILE[] = "noise_config.txt";

int main() {
    try {
        SIS3315Manager module("192.168.0.100");
        module.open();

        module.readFirmware();
        module.readSerialNumber();
        module.resetModule();

        ClockConfig     clk_cfg  = {};
        TriggerConfig   trig_cfg = {};
        AcquisitionConfig acq   = {};

        // --- Recharger la config sauvegardée si elle existe ---
        bool use_saved = false;
        if (loadConfigs(CONFIG_FILE, clk_cfg, trig_cfg, acq)) {
            std::cout << "\nConfig sauvegardée trouvée dans '" << CONFIG_FILE << "'." << std::endl;
            std::cout << "  Horloge   : source=" << static_cast<int>(clk_cfg.source)
                      << "  freq=" << clk_cfg.internal_freq_mhz << " MHz" << std::endl;
            std::cout << "  Trigger   : source=" << static_cast<int>(trig_cfg.source) << std::endl;
            std::cout << "  Acq       : " << acq.nof_samples << " samples, "
                      << acq.channels.size() << " canaux" << std::endl;
            std::cout << "Utiliser cette config ? (0=non – reconfigurer, 1=oui) : ";
            int choice;
            std::cin >> choice;
            use_saved = (choice == 1);
        }

        if (use_saved) {
            // Application directe sans menus interactifs
            if (module.setClockParameter(clk_cfg) != 0) {
                std::cerr << "Erreur : horloge, abandon" << std::endl; return 1;
            }
            if (module.setTriggerParameter(trig_cfg) != 0) {
                std::cerr << "Erreur : trigger, abandon" << std::endl; return 1;
            }
            if (module.setAcquisitionParameter(acq) != 0) {
                std::cerr << "Erreur : acquisition, abandon" << std::endl; return 1;
            }
        } else {
            // Configuration interactive complète
            module.readClockSources();
            clk_cfg = askClockConfig();
            if (module.setClockParameter(clk_cfg) != 0) {
                std::cerr << "Erreur : configuration horloge échouée, abandon" << std::endl;
                return 1;
            }   

            module.readTriggerSources();

            std::cout << "\nLancer le test trigger interne (canal 1) ? (0=non, 1=oui) : ";
            int do_test;
            std::cin >> do_test;
            if (do_test == 1) {
                if (module.testInternalTrigger() != 0)
                    std::cerr << "Avertissement : test trigger interne échoué" << std::endl;
            }

            trig_cfg = SIS3315Manager::askTriggerConfig();
            if (module.setTriggerParameter(trig_cfg) != 0) {
                std::cerr << "Erreur : configuration trigger échouée" << std::endl;
                return 1;
            }

            acq = ask_acquisition_config();
            if (module.setAcquisitionParameter(acq) != 0) {
                std::cerr << "Erreur : configuration acquisition échouée" << std::endl;
                return 1;
            }

            // Sauvegarder pour les prochains lancements
            if (saveConfigs(CONFIG_FILE, clk_cfg, trig_cfg, acq))
                std::cout << "Config sauvegardée dans '" << CONFIG_FILE << "'" << std::endl;
            else
                std::cerr << "Avertissement : impossible de sauvegarder la config" << std::endl;
        }

        // --- Acquisition bruit ---
        std::cout << "\nLancer l'acquisition bruit sur les 16 canaux ? (0=non, 1=oui) : ";
        int do_noise;
        std::cin >> do_noise;
        if (do_noise == 1) {
            if (module.acquireNoise(acq.nof_samples) != 0)
                std::cerr << "Avertissement : acquisition bruit échouée" << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "Erreur fatale : " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
