#include "ofApp.h"
#include "ofxPresetsParametersBase.h"
#include "ofxPresets.h"
#include "ofxGui.h"

struct Params : public ofxPresetsParametersBase {
public:
	ofParameter<int> x;
	ofParameter<int> y;
	ofParameter<int> radius;

	Params() {
		groupName = "params";

		parameterMap["x"] = &x.set("x pos", 0);
		parameterMap["y"] = &y;
		// radius is not added to the map, so it won't be saved
	}
};
Params p;
std::vector<ofxPresetsParametersBase*> allParameters;

ofxPresets manager;

ofxPanel gui;
ofxLabel currPreset;
ofxLabel instructions;

//--------------------------------------------------------------
void ofApp::setup(){
	//ofSetLogLevel(OF_LOG_VERBOSE);

	allParameters = { &p };
	manager.setup( allParameters );

	p.x = ofGetWidth() / 2;
	p.y = ofGetHeight() / 2;
	p.radius = 50;

	ofAddListener(manager.transitionFinished, this, &ofApp::onPresetChanged);

	gui.setup();
	gui.add(manager.interpolationDuration.set("Transition duration", 0.5f, 0.0f, 20.0f));
	gui.add(manager.sequencePresetDuration.set("Preset duration", 0.5f, 0.0f, 20.0f));
	//gui.add()
	gui.add(currPreset.setup("Current preset", ""));
	gui.add(instructions.setup("Press 1-9 to apply a preset\n<Shift> 1-9 to save into a preset\nS to start a sequence\nC to stop the sequence", ""));
}

//--------------------------------------------------------------
void ofApp::update(){
	manager.update();
	currPreset = ofToString(manager.getCurrentPreset());
}

//--------------------------------------------------------------
void ofApp::draw(){
	ofDrawCircle(p.x, p.y, p.radius);
	gui.draw();
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
		manager.loadSequence("1, 2, ?-5, 8");
		manager.playSequence();
	}

	if (e.keycode == 'C') {
		manager.stop();
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
