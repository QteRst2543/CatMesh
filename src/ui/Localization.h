#pragma once

#include <string>
#include <unordered_map>

enum class Language {
    English,
    Russian
};

class Localization {
public:
    static Localization& Instance() {
        static Localization instance;
        return instance;
    }

    void SetLanguage(Language lang) {
        currentLanguage = lang;
    }

    Language GetLanguage() const {
        return currentLanguage;
    }

    const char* Get(const std::string& key) const {
        auto it = translations[static_cast<int>(currentLanguage)].find(key);
        if (it != translations[static_cast<int>(currentLanguage)].end()) {
            return it->second.c_str();
        }
        return key.c_str();
    }

private:
    Localization() : currentLanguage(Language::English) {
        InitializeTranslations();
    }

    void InitializeTranslations();
    
    Language currentLanguage;
    std::unordered_map<std::string, std::string> translations[2]; // 0 = English, 1 = Russian
};
