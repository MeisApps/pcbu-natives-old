#ifndef PAM_PCBIOUNLOCK_I18N_H
#define PAM_PCBIOUNLOCK_I18N_H

#include <string>
#include <map>
#include <locale>

#include "utils/Utils.h"
#include "Logger.h"

const std::map<std::string, std::string> m_EnLangMap = {
        {"initializing", "Initializing..."},
        {"password", "Password"},
        {"error_unknown", "Unknown Error."},
        {"error_pam", "Error: Could not get PAM info."},
        {"error_not_paired", "Error: Not paired."},
        {"error_invalid_user", "Error: Invalid user."},
        {"error_start_handler", "Error: Could not start socket."},
        {"error_connect", "Could not connect to phone."},
        {"error_password", "Invalid password."},
        {"enter_password", "Please enter your password."},
        {"wait_key_press", "Press any key or click."},
        {"wait_unlock", "Use phone to unlock..."},
        {"unlock_success", "Success."},
        {"unlock_canceled", "Canceled."},
        {"unlock_timeout", "Timeout."},
};
const std::map<std::string, std::string> m_DeLangMap = {
        {"initializing", "Initialisiere..."},
        {"password", "Passwort"},
        {"error_unknown", "Unbekannter Fehler."},
        {"error_pam", "Fehler: Konnte PAM Infos nicht holen."},
        {"error_not_paired", "Fehler: Nicht gepairt."},
        {"error_invalid_user", "Fehler: Ungültiger Benutzer."},
        {"error_start_handler", "Fehler: Konnte Socket nicht starten."},
        {"error_connect", "Verbindung zum Telefon fehlgeschlagen."},
        {"error_password", "Ungültiges Passwort."},
        {"enter_password", "Gib bitte dein Passwort ein."},
        {"wait_key_press", "Drücke eine beliebige Taste."},
        {"wait_unlock", "Verwende Telefon, um zu entsperren..."},
        {"unlock_success", "Erfolg."},
        {"unlock_canceled", "Abgebrochen."},
        {"unlock_timeout", "Timeout."},
};

class I18n {
public:
    static std::string Get(const std::string& key);

private:
    static void FindLocale();

    static int m_Locale;
    I18n() = default;
};

#endif //PAM_PCBIOUNLOCK_I18N_H
