//
//  ofxFFmpegUtils.h
//  BasicSketch
//
//  Created by Oriol Ferrer Mesià on 16/03/2018.
//
//

#pragma once
#include "ofMain.h"
#include "ofxExternalProcess.h"

class ofxFFmpegUtils{

public:
	
	ofxFFmpegUtils();

	void setup(string ffmpegBinaryPath, string ffProbeBinaryPath);
	void update(float dt);

	void convertToImageSequence(string movieFilePath,
								string imgFileExtension, //"jpeg", "tiff", etc
								float jpegQuality/*[0..1]*/,
								string outputFolder,
								ofVec2f resizeBox = ofVec2f(), //if you supply a size, img sequence will be resized so that it fits in that size (keeping aspect ratio)
								int numFilenameDigits = 6 // "output_00004.jpg" ctrl # of leading zeros
								);

	void drawDebug(int x, int y);

protected:

	struct ProcessInfo{
	};

	map<ofxExternalProcess*, ProcessInfo> processes;

	string ffmpegBinaryPath;
	string ffProbeBinaryPath;
};

