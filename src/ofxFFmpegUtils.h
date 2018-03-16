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

	ofVec2f getVideoResolution(string movieFilePath);

	void convertToImageSequence(string movieFilePath,
								string imgFileExtension, //"jpeg", "tiff", etc
								float jpegQuality/*[0..1]*/,
								string outputFolder,
								int numFilenameDigits = 6, // "output_00004.jpg" ctrl # of leading zeros
								ofVec2f resizeBox = ofVec2f(-1,-1), //if you supply a size, img sequence will be resized so that it fits in that size (keeping aspect ratio)
								ofVec2f cropToAspectRatio = ofVec2f(-1,-1),
								float cropBalance = -1 	//[0..1] if we are cropping, what's the crop mapping?
														//this is a loose param, works for horizontal and vertical crop
														// 0 would mean crop all the "right" (or bottom) pixels that dont fit in the A/R

								);

	void drawDebug(int x, int y);

protected:

	struct ProcessInfo{
	};

	map<ofxExternalProcess*, ProcessInfo> processes;

	string ffmpegBinaryPath;
	string ffProbeBinaryPath;
};

