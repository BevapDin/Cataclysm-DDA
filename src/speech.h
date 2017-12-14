#pragma once
#ifndef SPEECH_H
#define SPEECH_H

#include "translatable_text.h"

#include <string>

class JsonObject;

struct SpeechBubble {
    translatable_text text;
    int volume;
};

void load_speech( JsonObject &jo );
void reset_speech();
const SpeechBubble &get_speech( const std::string label );

#endif
