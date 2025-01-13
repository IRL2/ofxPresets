#include "ofApp.h"
#include "ofxPresetsParametersBase.h"
#include "ofxPresets.h"
#include "ofxGui.h"

struct Params : public ofxPresetsParametersBase {
public:
	ofParameter<int> x;
	ofParameter<int> y;
	ofParameter<int> radius;
	ofParameter<ofColor> color;

	Params() {
		groupName = "params";

		parameterMap["x"] = &x.set("x pos", 0);
		parameterMap["y"] = &y;
		parameterMap["color"] = &color;
		// radius is not added to the map, so it won't be saved
	}
};
Params p;
std::vector<ofxPresetsParametersBase*> allParameters;

ofxPresets manager;

ofxPanel gui;
ofxLabel currPreset;
ofxLabel instructions;
ofxLabel internalSequence;
ofParameter<std::string> sequenceInput;

//--------------------------------------------------------------
void ofApp::setup(){
	//ofSetLogLevel(OF_LOG_VERBOSE);

	allParameters = { &p };
	manager.setup( allParameters );

	p.x.set("x pos", ofGetWidth() / 2, 0, ofGetWidth());
	p.y.set("y pos", ofGetHeight() / 2, 0, ofGetHeight());
	p.radius = 50;

	ofAddListener(manager.transitionFinished, this, &ofApp::onPresetChanged);

	gui.setup();
	gui.setWidthElements(300);
	gui.add(manager.interpolationDuration.set("Transition duration", 0.5f, 0.0f, 20.0f));
	gui.add(manager.sequencePresetDuration.set("Preset duration", 0.5f, 0.0f, 20.0f));
	gui.add(sequenceInput.set("Sequence", "1, 2*, ?-3, 8"));
	gui.add(currPreset.setup("Current preset", ""));
	gui.add(internalSequence.setup("Internal seq", ""));

	gui.add(p.color.set("Color", ofColor::white));
}

//--------------------------------------------------------------
void ofApp::update(){
	manager.update();
	currPreset = ofToString(manager.getCurrentPreset());
	internalSequence = ofToString(manager.sequence.get());
}

//--------------------------------------------------------------
void ofApp::draw(){
	ofSetColor(p.color);
	ofDrawCircle(p.x, p.y, p.radius);
	gui.draw();

	ofDrawBitmapStringHighlight("Press 1-9 to apply a preset\n<Shift> 1-9 to save into a preset\nS to start a sequence\nC to stop the sequence\nM to mutate", ofGetWidth() - 280, 38);
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
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
	p.x = x;
	p.y = y;
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
	p.x = x;
	p.y = y;
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
