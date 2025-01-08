#pragma once
#include <string>
#include <unordered_map>
#include <ofParameter.h>

class ofxPresetsParametersBase {

public:

	// this map holds a list of all parameters and their names
	// its gonna be used to work with the presets
    std::unordered_map<std::string, ofAbstractParameter*> parameterMap;

	std::string groupName;

    virtual ~ofxPresetsParametersBase() = default;

	ofxPresetsParametersBase() = default;
};
