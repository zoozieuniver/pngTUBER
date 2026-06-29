#include "localization.h"
#include "config.h"

const char* tr(const char* eng, const char* ukr) {
    if (globalSettings.language == 1) {
        return ukr;
    }
    return eng;
}
