#include "ofApp.h"
#include "ofxPresetsParametersBase.h"
#include "ofxPresets.h"



struct Params : public ofxPresetsParametersBase {
public:
	ofParameter<int> x;
	ofParameter<int> y;
	ofParameter<int> radius;

	Params() {
		groupName = "params";

		parameterMap["x"] = &x.set("x pos", 0);
		parameterMap["y"] = &y;
	}
};

ofParameterGroup g;

Params p;
std::vector<ofxPresetsParametersBase*> allParameters;

ofxPresets manager;

//--------------------------------------------------------------
void ofApp::setup(){
	//g.setName("params");
	//g.add(p.x.set("x pos", 0));
	//g.add(p.y.set("y pos", 0));

	//for (auto pp : g) {
	//	ofLog() << pp->getName();
	//}

	ofSetLogLevel(OF_LOG_VERBOSE);

	allParameters = { &p };
	manager.setup( allParameters );

	p.x = ofGetWidth() / 2;
	p.y = ofGetHeight() / 2;
	p.radius = 50;

	ofAddListener(manager.transitionFinished, this, &ofApp::onPresetChanged);
}

//--------------------------------------------------------------
void ofApp::update(){
	manager.update();
}

//--------------------------------------------------------------
void ofApp::draw(){
	ofDrawCircle(p.x, p.y, p.radius);
}

//--------------------------------------------------------------
void ofApp::keyReleased(ofKeyEventArgs& e) {
	if (e.keycode >= '1' && e.keycode <= '9') {
		int index = e.keycode - '1' + 1; // convert to int by removing the ascii offset

		if (e.hasModifier(OF_KEY_SHIFT)) {
			manager.savePreset(index);
		}
		else {
			manager.applyPreset(index, 5.0f);
		}
	}

	if (e.keycode == 'S') {
		manager.loadSequence("1-3");
		manager.playSequence();
	}

	if (e.keycode == 'R') {
		p.radius = ofRandom(5, 50);
	}

	if (e.keycode == 'C') {
		manager.stopSequence();
	}
}

void ofApp::onPresetChanged() {
	//ofLog() << "Preset changed";
}

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
