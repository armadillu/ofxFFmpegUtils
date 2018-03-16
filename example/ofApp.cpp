#include "ofApp.h"


void ofApp::setup(){

	ofBackground(22);
	ffmpeg.setup("/usr/local/bin/ffmpeg", "/usr/local/bin/ffprobe");
}


void ofApp::update(){

	float dt = 1./60.;
	ffmpeg.update(dt);
}


void ofApp::draw(){

	ofDrawBitmapString("drag movie files into this window to create image sequences ", 20, 20);

	ffmpeg.drawDebug(20, 40);
}


void ofApp::keyPressed(int key){

}


void ofApp::keyReleased(int key){

}


void ofApp::mouseMoved(int x, int y ){

}


void ofApp::mouseDragged(int x, int y, int button){

}


void ofApp::mousePressed(int x, int y, int button){

}


void ofApp::mouseReleased(int x, int y, int button){

}


void ofApp::windowResized(int w, int h){

}


void ofApp::gotMessage(ofMessage msg){

}


void ofApp::dragEvent(ofDragInfo dragInfo){

	for (auto f : dragInfo.files){

		string movie = f;
		string targetDir = ofFilePath::getEnclosingDirectory(movie);
		string movieFileName = ofFilePath::getBaseName(movie);

		ffmpeg.convertToImageSequence(movie, //file
									  "jpg", //img format
									  1.0, //img compression [0..1]
									  targetDir + "/" + movieFileName, //folder to output to
									  6, //num digits in img sequence file naming
									  ofVec2f(100,100), //resize to fit this box
									  ofVec2f(4,1), //crop to match this aspect ratio
									  1 //if cropping, crop balance factor: where to we crop?
									  );
	}
}


