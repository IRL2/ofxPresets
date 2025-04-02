#pragma once

#include <fstream>
#include "ofJson.h"
#include "ofLog.h"
#include "ofxPresetsParametersBase.h"
#include "ofxSEasing.h"




const std::string DEFAULT_FOLDER_PATH = "data\\";
const float DEFAULT_SEQUENCE_PRESET_DURATION = 5.0f;
const float DEFAULT_INTERPOLATION_DURATION = 3.0f;
const float DEFAULT_MUTATION_PERCENTAGE = 0.1f;
const int MAX_RANDOM_PRESET = 16;


// This struct stores the target values for the interpolation,
// and a starting time
struct InterpolationData {
    float startTime = 0.0f;
    std::unordered_map<std::string, float> targetValues;
};


class ofxPresets {

private:
    void saveParametersToJson(const std::string& jsonFilePath);
    void applyJsonToParameters(const std::string& jsonFilePath, float interpolationDuration);

    std::string convertIDtoJSonFilename(int id);
    bool fileExist(const std::string& jsonFilePath);
	std::string folderPath = "data\\";

    std::string sequenceString;
    int sequenceIndex = 0;
    int lastAppliedPreset = 0;

    float lastUpdateTime = 0.0f;
    bool isTransitioning = false;  // flag to know if we are transitioning(interpolating) in the sequence
    bool isPlaying = false;

    std::unordered_map<std::string, InterpolationData> interpolationDataMap;
    std::unordered_map<std::string, std::unordered_map<std::string, float>> currentParameterValues;
    void storeCurrentValues();
    int getRandomPreset(int lowerPreset, int higherPreset);

    std::vector<ofxPresetsParametersBase*>* params; // local reference to the parameters

    std::vector<int> parseSequence(std::string& input);
    std::vector<std::string> splitString(const std::string& str, char delimiter) const;
    std::vector<int> unfoldRanges(std::string& str);
    std::vector<int> unfoldRandomRange(std::vector<std::string> randomRange);
    
    std::function<float(float)> easingFunction = ofxSEeasing::easeInOutCubic;

    void onPresetFinished();
    void onTransitionFinished();
    //void onSequenceFinished(); // not implemented

    void updateParameters();
    void updateSequence();
    void advanceSequenceIndex();

public:
    ofxPresets() {}

    ~ofxPresets() {
        stop();
        if (params) {
            delete params;
        }
        if (!interpolationDataMap.empty()) {
            interpolationDataMap.clear();
        }
        if (!currentParameterValues.empty()) {
            currentParameterValues.clear();
        }
    }

    void setup(ofParameterGroup& parameters);
    void setup(ofxPresetsParametersBase& parameters);
    void setup(std::vector<ofParameterGroup>& parameters);
    void setup(std::vector<ofxPresetsParametersBase*>& parameters);

    void update();

    void applyPreset(int id);
    void applyPreset(int id, float duration);
    void savePreset(int id);
    void deletePreset(int id);
    void clonePresetTo(int from, int to);

    void loadSequence(const std::string& sequenceString);
    void playSequence();
    void playSequence(float sequenceDuration, float transitionDuration);
    void stopSequence();
    void stopInterpolating();
    void stop();

    void mutate();
    void mutate(float percentage);

    void mutateFromPreset(int id, float percentage);

	ofParameter<std::vector<int>> sequence;
    int getCurrentPreset();
    static std::string removeInvalidCharacters(const std::string& input);

    bool isInterpolating() { return !interpolationDataMap.empty(); } // for when parameters are being interpolated
    bool isPlayingSequence() const { return isPlaying; }
    int getSequenceIndex() const { return sequenceIndex; }

    bool presetExist(int id);

    void setFolderPath(const std::string& path);

    ofParameter<float> sequencePresetDuration = DEFAULT_SEQUENCE_PRESET_DURATION;
    ofParameter<float> interpolationDuration = DEFAULT_INTERPOLATION_DURATION;
    ofParameter<float> mutationPercentage = DEFAULT_MUTATION_PERCENTAGE;

    void setEasingFunction(std::function<float(float)> func);

    ofEvent<void> sequencePresetFinished;
    ofEvent<void> transitionFinished;
    ofEvent<void> sequenceFinished;
	ofEvent<void> presetAppicationStarted;
};


/// <summary>
/// Set the easing function to be used for the interpolation
/// </summary>
/// <param name="func">Take one from the ofxSEasing or use yours</param>
void ofxPresets::setEasingFunction(std::function<float(float)> func) {
    easingFunction = func;
}


/// <summary>
/// Setup the preset manager from a vector containing the parameters structs
/// </summary>
/// <param name="parameters">i.e.
/// std::vector<ofxPresetsParametersBase*> allParameters;
/// allParameters = { &myParamStruct1, &myOtherParams };
/// </param>
void ofxPresets::setup(std::vector<ofxPresetsParametersBase*>& parameters) {
    this->params = &parameters;
}


/// <summary>
/// Setup the preset manager from a single parameter struct
/// </summary>
/// <param name="parameters"></param>
void ofxPresets::setup(ofxPresetsParametersBase& parameters) {
    this->params = new std::vector<ofxPresetsParametersBase*>{ &parameters };
}


/// <summary>
/// Setup the preset manager from an ofxParameterGroup
/// </summary>
/// <param name="parameters"></param>
void ofxPresets::setup(ofParameterGroup& parameters) {
    struct p : ofxPresetsParametersBase {
        p(ofParameterGroup& parameters) {
            groupName = parameters.getName();
            for (auto& param : parameters) {
                parameterMap[param->getName()] = param.get();
            }
        }
    };
    auto paramGroup = new p(parameters);

    this->params = new std::vector<ofxPresetsParametersBase*>{ paramGroup };
}


/// <summary>
/// Setup the preset manager from a vector of ofxParameterGroups
/// </summary>
/// <param name="parameters"></param>
void ofxPresets::setup(std::vector<ofParameterGroup>& parameters) {
    std::vector<ofxPresetsParametersBase*> allParameters;
    for (auto& paramGroup : parameters) {
        struct p : ofxPresetsParametersBase {
            p(ofParameterGroup& parameters) {
                groupName = parameters.getName();
                for (auto& param : parameters) {
                    parameterMap[param->getName()] = param.get();
                }
            }
        };
        auto param = new p(paramGroup);
        allParameters.push_back(param);
    }
    this->params = &allParameters;
}


/// <summary>
/// Update interpolations and sequence
/// </summary>
void ofxPresets::update() {
    updateParameters();
    updateSequence();
}



#pragma region ParameterHandling


/// <summary>
/// Apply the values from a JSON file into to the parameter interpolation map
/// </summary>
/// <param name="jsonFilePath">Full path to the json file</param>
void ofxPresets::applyJsonToParameters(const std::string& jsonFilePath, float duration) {
    ofLog(OF_LOG_NOTICE) << "ofxPresets::applyJsonToParameters:: Applying preset to parameters from " << jsonFilePath;

    // Read the JSON file
    std::ifstream file(jsonFilePath);
    if (!file.is_open()) {
        ofLogError("ofxPresets::applyJsonToParameters") << "Could not open JSON file";
        return;
    }

    // Parse the JSON file
    ofJson j;
    file >> j;

    interpolationDataMap.clear();

	storeCurrentValues(); // needed for interpolation

    interpolationDuration.set(duration);

    // Iterate over all items in the JSON
    for (auto& [group, v] : j.items()) {  // first level is the parameter group

        // Iterate over all parameter groups to find the corresponding 1st level set from the json
        for (auto& paramGroup : *params) {
            if (paramGroup->groupName == group) {

                InterpolationData interpolationData;
                interpolationData.startTime = ofGetElapsedTimef();

                // iterate over all items in the group
                for (auto& [key, value] : j[group].items()) {

                    // store actual param key and value
                    auto param = paramGroup->parameterMap[key];
                    if (param == nullptr) {
                        ofLog(OF_LOG_VERBOSE) << "ofxPresets::applyJsonToParameters::" << "Preset key " << key << " not found in " << group;
                        continue;
                    }

                    // cast to apply the value
                    const std::string paramTypeName = typeid(*param).name(); // get the type of the parameter to cast it

                    try {
                        if (paramTypeName == typeid(ofParameter<bool>).name()) {
                            dynamic_cast<ofParameter<bool>*>(param)->set(value.get<bool>());
                        }
                        else if (paramTypeName == typeid(ofParameter<int>).name()) {
                            //dynamic_cast<ofParameter<int>*>(param)->set(value.get<int>());
                            interpolationData.targetValues[key] = value.get<int>();
                        }
                        else if (paramTypeName == typeid(ofParameter<float>).name()) {
                            //dynamic_cast<ofParameter<float>*>(param)->set(value.get<float>());
                            interpolationData.targetValues[key] = value.get<float>();
                        }
                        else if (paramTypeName == typeid(ofParameter<ofColor>).name()) {
                            ofColor color;
                            color.setHex(value.get<int>());
                            if (j[group].contains(key + "_alpha")) {
                                color.a = j[group][key + "_alpha"].get<int>();
                            } else {
                                color.a = 255;
                            }
                            interpolationData.targetValues[key] = color.getHex();
                            interpolationData.targetValues[key + "_alpha"] = color.a;
                        }
                    }
                    catch (const std::exception& e) {
                        ofLogError("ofxPresets::applyJsonToParameters") << "Error applying value for key " << key << ": " << e.what();
                    }
                }

                interpolationDataMap[group] = interpolationData;
            }
        }
    }
}



/// <summary>
/// Save the current values of the parameters to use them as a reference for interpolation
/// </summary>
void ofxPresets::storeCurrentValues() {
    for (auto& paramGroup : *params) {
        std::unordered_map<std::string, float> groupValues;
        for (auto& [key, param] : paramGroup->parameterMap) {
            if (param) { // Check if the parameter is not null
                const std::string paramTypeName = typeid(*param).name();
                if (paramTypeName == typeid(ofParameter<int>).name()) {
                    groupValues[key] = dynamic_cast<ofParameter<int>*>(param)->get();
                }
                else if (paramTypeName == typeid(ofParameter<float>).name()) {
                    groupValues[key] = dynamic_cast<ofParameter<float>*>(param)->get();
				}
                else if (paramTypeName == typeid(ofParameter<ofColor>).name()) {
                    ofColor color = dynamic_cast<ofParameter<ofColor>*>(param)->get();
                    groupValues[key] = color.getHex();
                    groupValues[key + "_alpha"] = color.a;
                }
            }
        }
        currentParameterValues[paramGroup->groupName] = groupValues;
    }
}




/// <summary>
/// Mutate the parameters with a random value within the min and max range
/// Uses the global mutationPercentage (Default 0.1)
/// </summary>
void ofxPresets::mutate(){
	mutate(mutationPercentage);
}


/// <summary>
/// Mutate the parameters with a random value within the min and max range
/// </summary>
/// <param name="percentage">This will modify the public mutationPercentage</param>
void ofxPresets::mutate(float percentage) {
	ofLog(OF_LOG_VERBOSE) << "ofxPresets::mutate:: Mutating current parameter values with percentage " << percentage;

	mutationPercentage.set(percentage);

    storeCurrentValues(); // Store current values as a reference

    interpolationDataMap.clear(); // Clear any existing interpolation data

    for (auto& paramGroup : *params) {
        InterpolationData interpolationData;
        interpolationData.startTime = ofGetElapsedTimef();

        for (auto& [key, param] : paramGroup->parameterMap) {
            if (param) { // Check if the parameter is not null
                const std::string paramTypeName = typeid(*param).name();
                float currentValue = 0.0f;
                float minValue = 0.0f;
                float maxValue = 0.0f;

                if (paramTypeName == typeid(ofParameter<int>).name()) {
                    auto intParam = dynamic_cast<ofParameter<int>*>(param);
                    currentValue = intParam->get();
                    minValue = intParam->getMin();
                    maxValue = intParam->getMax();
                }
                else if (paramTypeName == typeid(ofParameter<float>).name()) {
                    auto floatParam = dynamic_cast<ofParameter<float>*>(param);
                    currentValue = floatParam->get();
                    minValue = floatParam->getMin();
                    maxValue = floatParam->getMax();
                }
				else if (paramTypeName == typeid(ofParameter<ofColor>).name()) {
					currentValue = dynamic_cast<ofParameter<ofColor>*>(param)->get().getHue();
					minValue = 0.0f;
					maxValue = 255.0f;
				}

                // Calculate the random mutation
                float range = maxValue - minValue;
                float mutation = ofRandomGaussian(0.0f, percentage / 4) * range;
				float mutatedValue = currentValue + mutation;  // mutation does use the current value

                // Clamp the mutated value within the min and max range
                mutatedValue = std::clamp(mutatedValue, minValue, maxValue);

                interpolationData.targetValues[key] = mutatedValue;

				// Special case for colors, mutate the brightness and hue
                if (paramTypeName == typeid(ofParameter<ofColor>).name()) {
                    ofColor targetColor = dynamic_cast<ofParameter<ofColor>*>(param)->get();
                    targetColor.setHue(mutatedValue);
                    targetColor.setBrightness(mutatedValue-currentValue+ targetColor.getBrightness());
                    targetColor.a = std::clamp(targetColor.a + ofRandomGaussian(0.0f, percentage / 4) * 255.0f, 0.0f, 255.0f);
                    interpolationData.targetValues[key] = targetColor.getHex();
                    interpolationData.targetValues[key + "_alpha"] = targetColor.a;
                }
            }
        }

        interpolationDataMap[paramGroup->groupName] = interpolationData;
    }
	// No need to start the interpolation, it will be done on the next update, since InterpolationDataMap is not empty
}


/// <summary>
/// Take an existent preset and mutate the values
/// </summary>
/// <param name="id"></param>
/// <param name="percentage"></param>
void ofxPresets::mutateFromPreset(int id, float percentage) {
    ofLog(OF_LOG_VERBOSE) << "ofxPresets::mutateFromPreset:: About to mutate values from the preset " << -id;

    std::string jsonFilePath = convertIDtoJSonFilename(-id);
    if (!fileExist(jsonFilePath)) {
        ofLogError("ofxPresets::mutateFromPreset") << "Preset file does not exist for ID: " << id;
        return;
    }

    // Apply the preset without interpolation
    applyJsonToParameters(jsonFilePath, interpolationDuration);

    // Store current values as a reference
    storeCurrentValues();

    // Mutate the parameters with the given percentage
    mutationPercentage.set(percentage);

	// TODO: this is repeated from the mutate() BUT using different sources, should be a common function
    for (auto& [group, interpolationData] : interpolationDataMap) {
        for (auto& [key, targetValue] : interpolationData.targetValues) {
            float currentValue = currentParameterValues[group][key];
            float minValue = 0.0f;
            float maxValue = 0.0f;

            // Determine the parameter type and get min/max values
            for (auto& paramGroup : *params) {
                if (paramGroup->groupName == group) {
                    auto param = paramGroup->parameterMap[key];
                    if (param) {
                        const std::string paramTypeName = typeid(*param).name();
                        if (paramTypeName == typeid(ofParameter<int>).name()) {
                            auto intParam = dynamic_cast<ofParameter<int>*>(param);
                            minValue = intParam->getMin();
                            maxValue = intParam->getMax();
                        }
                        else if (paramTypeName == typeid(ofParameter<float>).name()) {
                            auto floatParam = dynamic_cast<ofParameter<float>*>(param);
                            minValue = floatParam->getMin();
                            maxValue = floatParam->getMax();
						}
                        else if (paramTypeName == typeid(ofParameter<ofColor>).name()) {
                            targetValue = dynamic_cast<ofParameter<ofColor>*>(param)->get().getHue();
                            minValue = 0.0f;
                            maxValue = 255.0f;
                        }
                        // Calculate the random mutation
                        float range = maxValue - minValue;
                        float mutation = ofRandomGaussian(0.0f, percentage / 4) * range;
			            float mutatedValue = targetValue + mutation; // mutationFromPreset does use the target value instead of the current

                        // Clamp the mutated value within the min and max range
                        mutatedValue = std::clamp(mutatedValue, minValue, maxValue);

                        // Update the target value with the mutated value
                        interpolationData.targetValues[key] = mutatedValue;

                        // Special case for colors, mutate the brightness and hue
                        if (paramTypeName == typeid(ofParameter<ofColor>).name()) {
                            ofColor targetColor = dynamic_cast<ofParameter<ofColor>*>(param)->get();
                            
                            // change the hue
                            targetColor.setHue(mutatedValue);

							// repeat for brightness
                            mutation = ofRandomGaussian(0.0f, percentage / 4) * range;
                            mutatedValue = targetColor.getBrightness() + mutation;
                            mutatedValue = std::clamp(mutatedValue, 0.0f, 255.0f);
                            targetColor.setBrightness(mutatedValue);

                            // repeat for saturation
                            mutation = ofRandomGaussian(0.0f, percentage / 4) * range;
                            mutatedValue = targetColor.getSaturation() + mutation;
                            mutatedValue = std::clamp(mutatedValue, 0.0f, 255.0f);
                            targetColor.setSaturation(mutatedValue);

                            // alpha value
                            targetColor.a = std::clamp(targetColor.a + ofRandomGaussian(0.0f, percentage / 4) * 255.0f, 0.0f, 255.0f);

                            interpolationData.targetValues[key] = targetColor.getHex();
                            interpolationData.targetValues[key + "_alpha"] = targetColor.a;
                        }
                    }
                }
            }
        }
    }
}



#pragma endregion








/// <summary>
/// Apply a preset to the parameters
/// Uses the global interpolation duration
/// </summary>
/// <param name="id"></param>
void ofxPresets::applyPreset(int id) {
	applyPreset(id, interpolationDuration);
}

/// <summary>
/// Apply a preset to the parameters
/// </summary>
/// <param name="id"></param>
/// <param name="parameterGroups"></param>
/// <param name="duration">This will update the global interpolationDuration</param>
void ofxPresets::applyPreset(int id, float duration) {

    // mutation
    if (id < 0) {
        mutateFromPreset(id, mutationPercentage);
        lastAppliedPreset = id;
		presetAppicationStarted.notify();
        return;
    }

    // random preset
    if (id == 0) {
        id = getRandomPreset(1, MAX_RANDOM_PRESET);
	}

	// apply preset
    std::string jsonFilePath = convertIDtoJSonFilename(id);
    if (fileExist(jsonFilePath)) {
        applyJsonToParameters(jsonFilePath, duration);
        lastAppliedPreset = id;
        presetAppicationStarted.notify();
    }
    else {
        ofLog(OF_LOG_WARNING) << "ofxPresets::applyPreset:: No json file for preset " << id << " : " << jsonFilePath;
    }
}

/// <summary>
/// Find a valid random preset by looking for an existant json file
/// </summary>
/// <returns>After higherPreset^2 unlucky attempts, returns lowerPreset </returns>
int ofxPresets::getRandomPreset(int lowerPreset = 1, int higherPreset = 10) {
    int id = ofRandom(lowerPreset, higherPreset);
	int exitCounter = higherPreset * higherPreset;

    while (!presetExist(id) && exitCounter-- > 0) {
		id = ofRandom(lowerPreset, higherPreset);
	}

    if (exitCounter <= 0) {
        ofLogError("ofxPresets::getRandomPreset") << "Could not find valid random preset file";
        id = lowerPreset;
    }

	ofLog(OF_LOG_VERBOSE) << "ofxPresets::getRandomPreset:: Getting random preset " << id;
    return id;
}


/// <summary>
/// Returns the current preset item in the sequence
/// Will report the actual preset when 0 (random) is found
/// </summary>
/// <returns></returns>
int ofxPresets::getCurrentPreset() {
    return lastAppliedPreset;
}


/// <summary>
/// Stops the running interpolation
/// </summary>
void ofxPresets::stopInterpolating() {
    ofLog(OF_LOG_VERBOSE) << "ofxPresets::stopSequence:: Stopping interpolation";
    interpolationDataMap.clear();
}

/// <summary>
/// Stops the running sequence and interpolation
/// </summary>
void ofxPresets::stop() {
    stopInterpolating();
    stopSequence();
}



/// <summary>
/// public method to save the current parameters to a json file
/// </summary>
/// <param name="id">The preset ID (1-based)</param>
/// <param name="parameterGroups">all groups to be saved</param>
void ofxPresets::savePreset(int id) {
    std::string jsonFilePath = convertIDtoJSonFilename(id);
    saveParametersToJson(jsonFilePath);
}



#pragma region FileHandling


/// <summary>
/// Given an integer ID, convert it to a file path string
/// </summary>
/// <param name="id"></param>
/// <returns>filename is two digits. ie. 1 -> data\01.json </returns>
std::string ofxPresets::convertIDtoJSonFilename(int id) {
    std::string idStr = (id < 10 ? "0" : "") + std::to_string(id);
    std::string jsonFilePath = folderPath + idStr + ".json";
    return jsonFilePath;
}


/// <summary
/// Check if a file exists
/// </summary>
bool ofxPresets::fileExist(const std::string& jsonFilePath) {
    std::ifstream file(jsonFilePath);
    if (!file.is_open()) {
        return false;
    }
    return true;
}

/// <summary>
/// Sets the relative path to the folder where the presets will be saved and loaded
/// </summary>
/// <param name="path">example: "data\\presets\\"</param>
void ofxPresets::setFolderPath(const std::string& path) {
    folderPath = path;
	
    if (!std::filesystem::exists(folderPath)) {
		ofLog(OF_LOG_NOTICE) << "ofxPresets::setFolderPath:: Creating folder " << folderPath;
		std::filesystem::create_directory(folderPath);
	}
}


/// <summary>
/// Verify if an associated json file exists for a given preset ID
/// </summary>
/// <param name="id"></param>
/// <returns></returns>
bool ofxPresets::presetExist(int id) {
    auto file = convertIDtoJSonFilename(id);
    return fileExist(file);
}



/// <summary>
/// Delete a preset file
/// </summary>
/// <param name="id"></param>
void ofxPresets::deletePreset(int id) {
    std::string jsonFilePath = convertIDtoJSonFilename(id);
    if (fileExist(jsonFilePath)) {
        std::remove(jsonFilePath.c_str());
        ofLog(OF_LOG_VERBOSE) << "ofxPresets::deletePreset" << "Preset " << id << " deleted";
    }
}




/// <summary>
/// Clone a preset to another preset
/// </summary>
void ofxPresets::clonePresetTo(int from, int to) {
    std::string fromJsonFilePath = convertIDtoJSonFilename(from);
    std::string toJsonFilePath = convertIDtoJSonFilename(to);

    if (fileExist(fromJsonFilePath)) {
        ofLog(OF_LOG_VERBOSE) << "ofxPresets::clonePresetTo:: Cloning preset " << from << " to " << to;
        std::ifstream src(fromJsonFilePath, std::ios::binary);
        std::ofstream dst(toJsonFilePath, std::ios::binary);
        dst << src.rdbuf();
        dst.close();
    }
    else {
        ofLog(OF_LOG_ERROR) << "ofxPresets::clonePresetTo:: No json file for source preset " << from << ". Looking for " << toJsonFilePath;
    }
}



#pragma endregion



/// <summary>
/// Take the given parameters from a vector and saves them as a json file with the parameters and save it to the parameters
/// </summary>
/// <param name="jsonFilePath"></param>
/// <param name="parameterGroups"></param>
void ofxPresets::saveParametersToJson(const std::string& jsonFilePath) {
    ofLog(OF_LOG_NOTICE) << "ofxPresets::saveParametersToJson:: Saving JSON to esencia parameters to " << jsonFilePath;

    ofJson j;

    for (const auto& paramGroup : *params) {
        ofJson groupJson;
        for (const auto& [key, param] : paramGroup->parameterMap) {
            if (param != nullptr) {
                const std::string paramTypeName = typeid(*param).name();

                try {
                    if (paramTypeName == typeid(ofParameter<bool>).name()) {
                        groupJson[key] = dynamic_cast<ofParameter<bool>*>(param)->get();
                    }
                    else if (paramTypeName == typeid(ofParameter<int>).name()) {
                        groupJson[key] = dynamic_cast<ofParameter<int>*>(param)->get();
                    }
                    else if (paramTypeName == typeid(ofParameter<float>).name()) {
                        groupJson[key] = dynamic_cast<ofParameter<float>*>(param)->get();
                    }
                    else if (paramTypeName == typeid(ofParameter<ofColor>).name()) {
                        ofColor color = dynamic_cast<ofParameter<ofColor>*>(param)->get();
                        groupJson[key] = color.getHex();
                        groupJson[key + "_alpha"] = color.a;
                    }
                }
                catch (const std::exception& e) {
                    ofLogError() << "ofxPresets::saveParametersToJson:: Error saving value for key " << key << ": " << e.what();
                }
            }
        }
        j[paramGroup->groupName] = groupJson; // Assuming the first key is the group name
    }

    std::ofstream file(jsonFilePath);
    if (file.is_open()) {
        file << j.dump(4); // Pretty print with 4 spaces
        file.close();
    }
    else {
        ofLogError() << "ofxPresets::saveParametersToJson:: Could not open JSON file for writing";
    }
}


#pragma region updateParameters

/// <summary>
/// Update the parameters with the interpolation data towards the target values
/// </summary>
/// <param name="parameterGroups"></param>
void ofxPresets::updateParameters() {
    if (interpolationDataMap.empty()) {
        return;
    }

    float currentTime = ofGetElapsedTimef();
    float t;

    for (auto& [group, interpolationData] : interpolationDataMap) {
        float elapsedTime = currentTime - interpolationData.startTime;
        t = std::min(elapsedTime / interpolationDuration.get(), 1.0f); // t is the normalized time (value between 0 and 1)

        for (auto& [key, targetValue] : interpolationData.targetValues) {

            // skip _alpha keys, already covered by the color ones
            if (key.find("_alpha") != std::string::npos) {
                continue;
            }

            float startValue = currentParameterValues[group][key];
            float interpolatedValue = ofxSEeasing::map_clamp(t, 0.0f, 1.0f, startValue, targetValue, easingFunction(t));

            for (auto& paramGroup : *params) {
                if (paramGroup->groupName == group) {
                    auto param = paramGroup->parameterMap[key];
                    const std::string paramTypeName = typeid(*param).name();

                    if (paramTypeName == typeid(ofParameter<int>).name()) {
                        dynamic_cast<ofParameter<int>*>(param)->set(static_cast<int>(interpolatedValue));
                    }
                    else if (paramTypeName == typeid(ofParameter<float>).name()) {
                        dynamic_cast<ofParameter<float>*>(param)->set(interpolatedValue);
                    }
                    else if (paramTypeName == typeid(ofParameter<ofColor>).name()) {
                        ofColor color;
                        color.setHex(startValue);
						color.lerp(ofColor::fromHex(targetValue), t);
                        if (interpolationData.targetValues.find(key + "_alpha") != interpolationData.targetValues.end()) {
                            color.a = ofLerp(currentParameterValues[group][key + "_alpha"], interpolationData.targetValues[key + "_alpha"], t);
                        } else {
                            color.a = 255;
                        }
                        dynamic_cast<ofParameter<ofColor>*>(param)->set(color);
                    }
                }
            }
        }
    }
    if (t >= 1.0f) { // it means (currentTime - interpolationData.startTime >= interpolationData.duration)
        interpolationDataMap.clear();
        onTransitionFinished();
    }
}

#pragma endregion




#pragma region Sequencer


/// <summary>
/// Load the given sequence string into the sequence vector
/// </summary>
/// <param name="seqString"></param>
void ofxPresets::loadSequence(const std::string& seqString) {
    this->sequenceString = seqString;

    sequence.set(parseSequence(this->sequenceString));
    sequenceIndex = 0;

    ofLog(OF_LOG_NOTICE) << "ofxPresets::loadSequence:: Sequence loaded " << ofToString(sequence.get());
}


/// <summary>
/// Starts playing the loaded sequence
/// Uses sequencePresetDuration and interpolationDuration as default durations
/// </summary>
void ofxPresets::playSequence() {
	playSequence(sequencePresetDuration, interpolationDuration);
}


/// <summary>
/// Starts playing the loaded sequence
/// </summary>
/// <param name="presetDuration">This will update the global presetDuration value</param>
/// <param name="transitionDuration">This will update the global presetDuration value</param>
void ofxPresets::playSequence(float presetDuration, float transitionDuration) {
    ofLogNotice("ofxPresets::playSequence") << "Playing the loaded sequence with transition and preset durations: " << transitionDuration<< ", " << presetDuration;
    this->sequencePresetDuration.set(presetDuration);
    this->interpolationDuration.set(transitionDuration);
    this->isPlaying = true;

	// > to ensure the first preset is applied immediately (not waiting for the presetDuration to happen)
	this->isTransitioning = false;
	this->lastUpdateTime = ofGetElapsedTimef() - presetDuration;
	this->sequenceIndex = 0;

    if (sequence.get().size() == 0) {
        ofLogVerbose() << "ofxPresets::playSequence:: No sequence to play";
        return;
    }
}


/// <summary>
/// Stops the running sequence
/// </summary>
void ofxPresets::stopSequence() {
    ofLog(OF_LOG_VERBOSE) << "ofxPresets::stopSequence:: Stopping sequence";
    this->isPlaying = false;
    sequenceIndex = 0;
}



// Update the sequencer
void ofxPresets::updateSequence() {
    if (isPlayingSequence()) {
        float currentTime = ofGetElapsedTimef();

        if (isTransitioning) {
            if (currentTime - lastUpdateTime >= interpolationDuration.get()) {
                isTransitioning = false;
                lastUpdateTime = currentTime;
            }
        }
        else {
            if (currentTime - lastUpdateTime >= sequencePresetDuration.get()) {
                lastUpdateTime = currentTime;
                applyPreset(sequence.get()[sequenceIndex], interpolationDuration.get());
                advanceSequenceIndex();
                isTransitioning = true;
                onPresetFinished();
            }
        }
    }
}


/// <summary>
/// Move to the next step in the sequence. Restart if the end is reached
/// </summary>
void ofxPresets::advanceSequenceIndex() {
    sequenceIndex++;
    if (sequenceIndex >= sequence.get().size()) {
        sequenceIndex = 0;
    }
}


#pragma endregion




#pragma region Listeners

/// <summary>
/// Event to notify that a preset has finished
/// </summary>
void ofxPresets::onPresetFinished() {
    ofLogVerbose("ofxPresets::onPresetFinished") << "Sequence preset's waiting time finished";
    ofNotifyEvent(sequencePresetFinished, this);
}


/// <summary>
/// Event to notify that a transition interpolation has finished, happens for direct preset apply and sequence transitions
/// </summary>
void ofxPresets::onTransitionFinished() {
    ofLogVerbose("ofxPresets::onTransitionFinished") << "Preset transition finished";
    ofNotifyEvent(transitionFinished, this);
}


/// <summary>
/// NOT IN USE
/// Event to notify that a transition has finished
/// </summary>
//void ofxPresets::onSequenceFinished() {
//    ofLogVerbose("ofxPresets::onSequenceFinished") << "Sequence finished";
//    ofNotifyEvent(sequenceFinished, this);
//}


#pragma endregion




#pragma region Parsing

/// <summary>
/// Parse an string sequence into a vector of integers
/// example: "1, 2, 3 - 6, 2" turns into [1, 2, 3, 4, 5, 6, 2]
/// example: "1, ? - 3, 2, ?" turns into [1, 0, 0, 0, 2, 0]
/// </summary>
/// <param name="input">allows: Numbers, commas, dashes and question mark</param>
std::vector<int> ofxPresets::parseSequence(std::string& input) {
    std::vector<int> s;

    input = removeInvalidCharacters(input);

    std::istringstream stream(input);
    std::string token;

    // TODO: Better use regex to validate and organize parsing each section

    while (std::getline(stream, token, ',')) {

        // Handle ranges
        if (token.find('-') != std::string::npos) {
            std::vector<int> sr = unfoldRanges(token);
            for (auto& i : sr) {
                s.push_back(i);
            }
        }

        // Handle random presets
        else if (token.find('?') != std::string::npos) {
            s.push_back(0);
        }

        // Handle mutation
        else if (token.find('*') != std::string::npos) {
            int mutationPreset;
            if (token.find('*') == 0) {
				mutationPreset = std::stoi(token.substr(1, token.length()-1));
            }
            else {
				mutationPreset = std::stoi(token.substr(0, token.find('*')));
            }
            //mutationPreset = std::stoi(token.substr(1));
            s.push_back(-mutationPreset);
        }

        // Handle single numbers
        else {
            s.push_back(std::stoi(token));
        }
    }
    return s;
}

/// <summary>
/// Returns a vector of strings from a string separated by a delimiter
/// </summary>
/// <param name="str"></param>
/// <param name="delimiter">","</param>
std::vector<std::string> ofxPresets::splitString(const std::string& str, char delimiter) const {
    std::vector<std::string> parts;
    std::istringstream stream(str);
    std::string part;

    while (std::getline(stream, part, delimiter)) {
        parts.push_back(part);
    }
    return parts;
}


/// <summary>
/// Convert a "1-5" string into a vector of integers {1, 2, 3, 4, 5}
/// Using ?-N produces N times a random preset. i.e. "?-3" = {0, 0, 0}
/// </summary>
std::vector<int> ofxPresets::unfoldRanges(std::string& str) {
    std::vector<int> sr;
    int start = 1;
    int end = 1;

    auto rangeParts = splitString(str, '-');

    if (rangeParts.size() == 2) {

		// handle random presets
        if (rangeParts[0].find('?') != std::string::npos || rangeParts[0].find('?') != std::string::npos) {
			return unfoldRandomRange(rangeParts);
        }

        // regular range
        start = std::stoi(rangeParts[0]);
        end = std::stoi(rangeParts[1]);

        // Handle reversed ranges
        if (start > end) {
            std::swap(start, end);
        }
        
        for (int i = start; i <= end; ++i) {
            sr.push_back(i);
        }

        // Reverse back the reversed ranges
        if (std::stoi(rangeParts[0]) > std::stoi(rangeParts[1])) {
            std::reverse(sr.begin(), sr.end());
        }
    }

    return sr;
}



/// <summary>
/// Convert a range with ? into a vector of N times 0
/// i.e. "?-3" = {0, 0, 0}
/// i.e. "3-?" = {0, 0, 0}
/// i.e. "?-?" = {0}
/// </summary>
std::vector<int> ofxPresets::unfoldRandomRange(std::vector<std::string> randomRange) {
    std::vector<int> sr;

    bool firstIsRandom = randomRange[0].find('?') != std::string::npos;
    bool secondIsRandom = randomRange[1].find('?') != std::string::npos;

    // when both are random, return a single 0
    if (firstIsRandom && secondIsRandom) {
        randomRange[1] = "1";
    }
    // fill a range with N random presets
    if (firstIsRandom || secondIsRandom) {
        int rep = firstIsRandom ? std::stoi(randomRange[1]) : std::stoi(randomRange[0]);
        for (int i = 1; i <= rep; ++i) {
            sr.push_back(0);
        }
    }

    return sr;
}


/// <summary>
/// Removes invalid characters but digits, comma, dash, question mark, star
/// </summary>
/// <param name="input"></param>
/// <returns></returns>
std::string ofxPresets::removeInvalidCharacters(const std::string& input) {
    std::string result;
    std::copy_if(input.begin(), input.end(), std::back_inserter(result), [](char c) {
        return std::isdigit(c) || c == ',' || c == '-' || c == '?' || c == '*';
        });
    return result;
}


# pragma endregion