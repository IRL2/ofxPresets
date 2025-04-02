#include "ofApp.h"
#include "ofxPresetsParametersBase.h"
#include "ofxPresets.h"
#include "ofxGui.h"

// use ofxPresetsParametersBase for a more flexible way of setting up a group of parameters
// 
//struct Params : public ofxPresetsParametersBase {
//public:
//	ofParameter<int> x;
//	ofParameter<int> y;
//	ofParameter<int> radius;
//	ofParameter<ofColor> color;
//
//	Params() {
//		groupName = "params";
//
//		parameterMap["x"] = &x.set("x pos", ofGetWidth() / 2);
//		parameterMap["y"] = &y;
//		parameterMap["color"] = &color;
//		// radius is not added to the map, so it won't be saved
//	}
//};
//Params p;
//std::vector<ofxPresetsParametersBase*> allParameters;  // when using more than one group, wrap them on a vector

ofParameter<int> x;
ofParameter<int> y;
ofParameter<int> radius;
ofParameter<ofColor> color;

ofParameterGroup params;

ofxPresets manager;

ofxPanel gui;
ofxPanel guiParams;
ofxLabel currentPreset;
ofxLabel instructions;
ofxLabel internalSequence;
ofxLabel playing;
ofParameter<std::string> sequenceInput;

//--------------------------------------------------------------
void ofApp::setup(){
	//ofSetLogLevel(OF_LOG_VERBOSE);

	// Different ways of setting up the presets

	// 1. using ofParameterGroup
    params.setName("params");
	params.add(x.set("x", ofGetWidth() / 2, 0, ofGetWidth()));
	params.add(y.set("y", ofGetHeight() / 2, 0, ofGetHeight()));
	params.add(radius.set("radius", 5, 5, 80));
	params.add(color.set("color", ofColor::white));
    manager.setup(params);

    // 2. Use the ofxPresetsParametersBase struct as defined above
	//manager.setup(p);
	// 
	// or various in a vector
	//allParameters = { &p, &q };
	//manager.setup( allParameters );

	ofAddListener(manager.transitionFinished, this, &ofApp::onPresetChanged);

	gui.setup("sequencer", ofxPanelDefaultFilename, 10, 10);
	gui.setPosition(10, 0);
	gui.setWidthElements(230);
	gui.add(manager.interpolationDuration.set("Transition duration", 2.5f, 0.0f, 20.0f));
	gui.add(manager.sequencePresetDuration.set("Preset duration", 0.5f, 0.0f, 20.0f));
	gui.add(sequenceInput.set("Sequence", "1, 2*, ?-2"));
	gui.add(internalSequence.setup("Internal seq", ""));
	gui.add(currentPreset.setup("Current preset", ""));
    gui.add(playing.setup("Sequencer playing", ""));

	guiParams.setup("params");
	guiParams.setPosition(10, ofGetHeight()-260);
	guiParams.setWidthElements(100);
    guiParams.add(x);
	guiParams.add(y);
	guiParams.add(radius);
	guiParams.add(color);
}

//--------------------------------------------------------------
void ofApp::update(){
	manager.update();
	currentPreset = ofToString(manager.getCurrentPreset());
	internalSequence = ofToString(manager.sequence.get());
    playing = ofToString(manager.isPlayingSequence() ? "true" : "false");
}

//--------------------------------------------------------------
void ofApp::draw(){
	ofBackground(ofColor::gray);

	ofSetColor(color);
	ofDrawCircle(x, y, radius);

	ofDrawBitmapStringHighlight("Press 1-9 to apply a preset\n<Shift> 1-9 to save into a preset\nS to start a sequence\nC to stop the sequence\nM to mutate", ofGetWidth() - 280, 38);

	gui.draw();
    guiParams.draw();
}

//--------------------------------------------------------------
void ofApp::keyReleased(ofKeyEventArgs& e) {
	if (e.keycode >= '1' && e.keycode <= '9') {
		int index = e.keycode - '1' + 1; // convert to int by removing the ascii offset

		if (e.hasModifier(OF_KEY_SHIFT)) {
			manager.savePreset(index);
		}
		else {
			manager.applyPreset(index);
		}
	}

	if (e.keycode == 'S') {
		manager.loadSequence(ofToString(sequenceInput.get()));
		manager.playSequence();
	}

	if (e.keycode == 'C') {
		manager.stop();
	}

	if (e.keycode == 'M') {
		manager.mutate();
	}
}

//--------------------------------------------------------------
void ofApp::onPresetChanged() {
	ofLog() << "Recieving preset changed event";
}


//--------------------------------------------------------------
void ofApp::mouseDragged(int _x, int _y, int button){
	x = _x;
	y = _y;
}

//--------------------------------------------------------------
void ofApp::mousePressed(int _x, int _y, int button){
	x = _x;
	y = _y;
}

