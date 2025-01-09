#pragma once

#include <fstream>
#include "ofJson.h"
#include "ofLog.h"
#include "ofxPresetsParametersBase.h"
#include "ofxEasing.h"


/*
* the json file follows the same structure as the parameters, with the first level being the group name
{
    "simulation": {
        "particles": 1000,
        "radius": 3,
        ...
    },
    "render": {
        "param1": value,
        "param2": value,
        ...
    }
}
*/

const std::string DEFAULT_FOLDER_PATH = "data\\";
const float DEFAULT_SEQUENCE_PRESET_DURATION = 5.0f;
const float DEFAULT_INTERPOLATION_DURATION = 3.0f;
const int MAX_RANDOM_PRESET = 16;


// TODO: Use the map directly and store a common startTime, no need for a super struct
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
    static std::string removeInvalidCharacters(const std::string& input);
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

    void onPresetFinished();
    void onTransitionFinished();
    void onSequenceFinished();

    void updateParameters();
    void updateSequence();
    void advanceSequenceIndex();

public:
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

	ofParameter<std::vector<int>> sequence;
    int getCurrentPreset();

    bool isInterpolating() { return !interpolationDataMap.empty(); } // for when parameters are being interpolated
    bool isPlayingSequence() { return isPlaying; }

    bool presetExist(int id);

    void setFolderPath(const std::string& path);

    ofParameter<float> sequencePresetDuration = DEFAULT_SEQUENCE_PRESET_DURATION;
    ofParameter<float> interpolationDuration = DEFAULT_INTERPOLATION_DURATION;

    ofEvent<void> sequencePresetFinished;
    ofEvent<void> transitionFinished;
    ofEvent<void> sequenceFinished;
};


/// <summary>
/// Loads the parameterBase vector
/// </summary>
/// <param name="parameters"></param>
void ofxPresets::setup(std::vector<ofxPresetsParametersBase*>& parameters) {
    this->params = &parameters;
}



/// <summary>
/// Apply the values from a JSON file to the parameters
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
                            dynamic_cast<ofParameter<ofColor>*>(param)->set(color);
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
	if (id == -1) {
        id = getRandomPreset(1, MAX_RANDOM_PRESET);
	}

    std::string jsonFilePath = convertIDtoJSonFilename(id);
    if (fileExist(jsonFilePath)) {
        applyJsonToParameters(jsonFilePath, duration);
        lastAppliedPreset = id;
    }
    else {
        ofLog() << "ofxPresets::applyPreset:: No json file for preset " << id << " on " << jsonFilePath;
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
            }
        }
        currentParameterValues[paramGroup->groupName] = groupValues;
    }
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
		ofLog() << "ofxPresets::setFolderPath:: Creating folder " << folderPath;
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
        ofLog() << "ofxPresets::deletePreset" << "Preset " << id << " deleted";
    }
    //else {
    //	ofLog() << "ofxPresets::deletePreset:: No json file for preset " << idStr;
    //}
}


/// <summary>
/// Take the given parameters from a vector and saves them as a json file with the parameters and save it to the parameters
/// </summary>
/// <param name="jsonFilePath"></param>
/// <param name="parameterGroups"></param>
void ofxPresets::saveParametersToJson(const std::string& jsonFilePath) {
    ofLog() << "ofxPresets::saveParametersToJson:: Saving JSON to esencia parameters to " << jsonFilePath;

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

    for (auto& [group, interpolationData] : interpolationDataMap) {
        float elapsedTime = currentTime - interpolationData.startTime;
        float t = std::min(elapsedTime / interpolationDuration.get(), 1.0f);

        for (auto& [key, targetValue] : interpolationData.targetValues) {
            float startValue = currentParameterValues[group][key];
            float interpolatedValue = ofxeasing::map_clamp(t, 0.0f, 1.0f, startValue, targetValue, &ofxeasing::linear::easeInOut);

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
                }
            }
        }

        if (t >= 1.0f) { // it means (currentTime - interpolationData.startTime >= interpolationData.duration)
            interpolationDataMap.erase(group);
            if (interpolationDataMap.empty()) {
				onTransitionFinished();
                break;
            }
        }
    }
}

#pragma endregion



/// <summary>
/// Clone a preset to another preset
/// </summary>
void ofxPresets::clonePresetTo(int from, int to) {
	std::string fromJsonFilePath = convertIDtoJSonFilename(from);
    std::string toJsonFilePath = convertIDtoJSonFilename(to);

    if (fileExist(fromJsonFilePath)) {
        ofLog() << "ofxPresets::clonePresetTo:: Cloning preset " << from << " to " << to;
        std::ifstream src(fromJsonFilePath, std::ios::binary);
        std::ofstream dst(toJsonFilePath, std::ios::binary);
        dst << src.rdbuf();
        dst.close();
    }
    else {
        ofLog() << "ofxPresets::clonePresetTo:: No json file for source preset " << from << ". Looking for " << toJsonFilePath;
    }
}


/// <summary>
/// Load the given sequence string into the sequence vector
/// </summary>
/// <param name="seqString"></param>
void ofxPresets::loadSequence(const std::string& seqString) {
    this->sequenceString = seqString;

    sequence.set(parseSequence(this->sequenceString));
    sequenceIndex = 0;

    ofLog() << "ofxPresets::loadSequence:: Sequence loaded " << ofToString(sequence.get());
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
/// Update interpolations and sequence
/// </summary>
void ofxPresets::update() {
    updateParameters();
    updateSequence();
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
/// Event to notify that a transition has finished
/// </summary>
void ofxPresets::onSequenceFinished() {
    ofLogVerbose("ofxPresets::onSequenceFinished") << "Sequence finished";
    ofNotifyEvent(sequenceFinished, this);
}


/// <summary>
/// Returns the current preset item in the sequence
/// Will report the actual preset when -1 (random) is found
/// </summary>
/// <returns></returns>
int ofxPresets::getCurrentPreset() {
    return lastAppliedPreset;

	// TODO: Should be enough to return lastAppliedPreset, but need more testing if need to report the -1
    //if (sequence.get().size() > 0 && sequenceIndex <= sequence.get().size()) {
    //    if (sequence.get()[sequenceIndex] >= 0)
    //        return sequence.get()[sequenceIndex];
    //}
    //else {
    //    return lastAppliedPreset;
    //}
    //return 0;
}




/// <summary>
/// Parse an string sequence into a vector of integers
/// example: "1, 2, 3 - 6, 2" turns into [1, 2, 3, 4, 5, 6, 2]
/// example: "1, ? - 3, 2, ?" turns into [1, -1, -1, -1, 2, -1]
/// </summary>
/// <param name="input">allows: Numbers, commas, dashes and question mark</param>
std::vector<int> ofxPresets::parseSequence(std::string& input) {
    std::vector<int> s;

    input = removeInvalidCharacters(input);

    std::istringstream stream(input);
    std::string token;

    while (std::getline(stream, token, ',')) {

        // Handle ranges
        if (token.find('-') != std::string::npos) {
            std::vector<int> sr = unfoldRanges(token);
            for (auto& i : sr) {
                s.push_back(i);
            }
		}

        else if (token.find('?') != std::string::npos) {
            s.push_back(-1);
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
/// Using ?-N produces N times a random preset. i.e. "?-3" = {-1, -1, -1}
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
/// Convert a range with ? into a vector of N times -1
/// i.e. "?-3" = {-1, -1, -1}
/// i.e. "3-?" = {-1, -1, -1}
/// i.e. "?-?" = {-1}
/// </summary>
std::vector<int> ofxPresets::unfoldRandomRange(std::vector<std::string> randomRange) {
    std::vector<int> sr;

    bool firstIsRandom = randomRange[0].find('?') != std::string::npos;
    bool secondIsRandom = randomRange[1].find('?') != std::string::npos;

    // when both are random, return a single -1
    if (firstIsRandom && secondIsRandom) {
        randomRange[1] = "1";
    }
    // fill a range with N random presets
    if (firstIsRandom || secondIsRandom) {
        int rep = firstIsRandom ? std::stoi(randomRange[1]) : std::stoi(randomRange[0]);
        for (int i = 1; i <= rep; ++i) {
            sr.push_back(-1);
        }
    }

    return sr;
}


/// <summary>
/// Removes invalid characters but digits, comma, dash and question mark
/// </summary>
/// <param name="input"></param>
/// <returns></returns>
std::string ofxPresets::removeInvalidCharacters(const std::string& input) {
    std::string result;
    std::copy_if(input.begin(), input.end(), std::back_inserter(result), [](char c) {
        return std::isdigit(c) || c == ',' || c == '-' || c == '?';
        });
    return result;
}


