#ifndef CF_PARSER_H
#define CF_PARSER_H

#include <string>
#include <optional>

namespace cfparser {

/**
 * @brief Estrae un codice fiscale italiano da una stringa di testo.
 *
 * Utilizza una regex completa che supporta anche i caratteri di omocodia
 * (dove le cifre possono essere sostituite da lettere).
 *
 * @param text La stringa in cui cercare il codice fiscale
 * @return Il codice fiscale trovato (in maiuscolo), o std::nullopt se non trovato
 */
std::optional<std::wstring> extractCodiceFiscale(const std::wstring& text);

/**
 * @brief Verifica se una stringa è un codice fiscale italiano valido.
 *
 * @param cf Il codice fiscale da verificare
 * @return true se il codice fiscale è valido, false altrimenti
 */
bool isValidCodiceFiscale(const std::wstring& cf);

/**
 * @brief Converte i caratteri omocodici in cifre.
 *
 * In caso di omocodia, alcune cifre del CF vengono sostituite con lettere:
 * L=0, M=1, N=2, P=3, Q=4, R=5, S=6, T=7, U=8, V=9
 *
 * @param cf Il codice fiscale con possibili caratteri omocodici
 * @return Il codice fiscale con le cifre ripristinate
 */
std::wstring normalizeOmocodia(const std::wstring& cf);

/**
 * @brief Calcola il carattere di controllo (CIN) del codice fiscale.
 *
 * @param cf I primi 15 caratteri del codice fiscale
 * @return Il carattere di controllo calcolato
 */
wchar_t calculateCIN(const std::wstring& cf);

/**
 * @brief Verifica il carattere di controllo del codice fiscale.
 *
 * @param cf Il codice fiscale completo (16 caratteri)
 * @return true se il CIN è corretto, false altrimenti
 */
bool verifyCIN(const std::wstring& cf);

} // namespace cfparser

#endif // CF_PARSER_H
