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

	void setup(const string & ffmpegBinaryPath, const string & ffProbeBinaryPath);

	bool isFFMpegAvailable();

	void update(float dt);
	void setMaxSimulatneousJobs(int max); //enqueue jobs if more than N are already running
	void setMaxThreadsPerJob(int maxThr); //set it "-1" for auto (# of hw cores)

	//returns video res of a specific file (spaws an external process & blocks!)
	ofVec2f getVideoResolution(const string & movieFilePath);
	float getVideoFramerate(const string & movieFilePath);
	ofJson getVideoInfo(const string & movieFilePath); //returns a json object

	void setExtraArguments(vector<string> args){extraArguments = args;};
	void clearExtraArguments(){extraArguments.clear();}


	//returns a jobID
	size_t convertToImageSequence(const string & movieFilePath,
								const string & imgFileExtension, //"jpeg", "tiff", etc
								float jpegQuality/*[0..1]*/,
								const string & outputFolder,
								bool convertToGrayscale,
								int numFilenameDigits = 6, // "output_00004.jpg" ctrl # of leading zeros
								ofVec2f resizeBox = ofVec2f(-1,-1), //if you supply a size, img sequence will be resized so that it fits in that size (keeping aspect ratio)
								ofVec2f cropToAspectRatio = ofVec2f(-1,-1),
								float cropBalance = -1 	//[0..1] if we are cropping, what's the crop mapping?
														//this is a loose param, works for horizontal and vertical crop
														// 0 would mean crop all the "right" (or bottom) pixels that dont fit in the A/R

								);

	//returns a jobID
	size_t imgSequenceToMP4(	const string & imgFolder,
							float framerate,
							float compressQuality, /*0..1*/
						  	const string &filenameFormat, 		//ie frame_%08d
						  	const string & imgFileExtension, 	//ie tiff
						  	const string & outputMovieFilePath 		//result movie file path
						  );

	std::string getStatus();
	void drawDebug(int x, int y);
	

	enum JobType{
		MOVIE_TO_IMG_SEQ,
		IMG_SEQ_TO_MOVIE
	};

	struct JobResult{
		JobType type;
		size_t jobID = 0;
		string inputFilePath;
		string outputFolder;
		bool ok = false;
		ofxExternalProcess::Result results;
	};

	string getCurrentOutputForJob(size_t jobID);

	ofEvent<JobResult> eventJobCompleted;

	bool isBusy(){return activeProcesses.size() > 0 || jobQueue.size() > 0;}


protected:

	struct JobInfo{
		JobType type;
		string originalFile;
		string destinationFolder;
		ofxExternalProcess* process = nullptr;
	};

	map<size_t, JobInfo> jobQueue;
	map<size_t, JobInfo> activeProcesses;

	vector<string> extraArguments;

	string ffmpegBinaryPath;
	string ffProbeBinaryPath;

	size_t jobCounter = 0;
	int maxSimultJobs = 2;
	int maxThreadsPerJob = -1; //default to auto - use all
};
