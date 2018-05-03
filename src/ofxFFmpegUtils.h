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
	~ofxFFmpegUtils();

	void setup(string ffmpegBinaryPath, string ffProbeBinaryPath);
	void update(float dt);
	void setMaxSimulatneousJobs(int max); //enqueue jobs if more than N are already running
	void setMaxThreadsPerJob(int maxThr); //set it "-1" for auto (# of hw cores)

	//returns video res of a specific file (spaws an external process & blocks!)
	ofVec2f getVideoResolution(string movieFilePath);

	//returns a jobID
	size_t convertToImageSequence(string movieFilePath,
								string imgFileExtension, //"jpeg", "tiff", etc
								float jpegQuality/*[0..1]*/,
								string outputFolder,
								bool convertToGrayscale,
								int numFilenameDigits = 6, // "output_00004.jpg" ctrl # of leading zeros
								ofVec2f resizeBox = ofVec2f(-1,-1), //if you supply a size, img sequence will be resized so that it fits in that size (keeping aspect ratio)
								ofVec2f cropToAspectRatio = ofVec2f(-1,-1),
								float cropBalance = -1 	//[0..1] if we are cropping, what's the crop mapping?
														//this is a loose param, works for horizontal and vertical crop
														// 0 would mean crop all the "right" (or bottom) pixels that dont fit in the A/R

								);

	void drawDebug(int x, int y);

	struct JobResult{
		size_t jobID = 0;
		string inputFilePath;
		string outputFolder;
		bool ok = false;
		ofxExternalProcess::Result results;
	};

	ofEvent<JobResult> eventJobCompleted;

protected:

	struct JobInfo{
		string originalFile;
		string destinationFolder;
		ofxExternalProcess* process = nullptr;
	};

	map<size_t, JobInfo> jobQueue;
	map<size_t, JobInfo> activeProcesses;

	string ffmpegBinaryPath;
	string ffProbeBinaryPath;

	size_t jobCounter = 0;
	int maxSimultJobs = 2;
	int maxThreadsPerJob = -1; //default to auto - use all
};
