/*
    SmolFmXmlParser - XML format parser for .smolfm files.
*/

#pragma once

#include "ISmolFmParser.h"

namespace smolfm
{

class SmolFmXmlParser : public ISmolFmParser
{
public:
    bool parse (const juce::File& file, SmolFmData& data) override;
    bool canParse (const juce::File& file) override;
};

} // namespace smolfm