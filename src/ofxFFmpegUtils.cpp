//
//  ofxFFmpegUtils.cpp
//  BasicSketch
//
//  Created by Oriol Ferrer Mesià on 16/03/2018.
//
//

#include "ofxFFmpegUtils.h"

ofxFFmpegUtils::ofxFFmpegUtils(){
}

ofxFFmpegUtils::~ofxFFmpegUtils(){
	for(auto p : jobQueue){
		delete p.second.process;
	}
	jobQueue.clear();

	for(auto p : activeProcesses){
		p.second.process->kill();
		p.second.process->join();
		delete p.second.process;
	}
	activeProcesses.clear();
}

void ofxFFmpegUtils::setup(const string & ffmpegBinaryPath, const string & ffProbeBinaryPath){
	this->ffmpegBinaryPath = ffmpegBinaryPath;
	this->ffProbeBinaryPath = ffProbeBinaryPath;
}

void ofxFFmpegUtils::setMaxSimulatneousJobs(int max){
	maxSimultJobs = ofClamp(max, 1, INT_MAX);
}

void ofxFFmpegUtils::setMaxThreadsPerJob(int maxThr){
	maxThreadsPerJob = maxThr;
}

ofJson ofxFFmpegUtils::getVideoInfo(const string & filePath){
	//https://gist.github.com/nrk/2286511
	//ffprobe -v quiet -print_format json -show_format -show_streams
	string jsonString = ofSystem(ffProbeBinaryPath + " -v quiet -print_format json -show_format -show_streams \"" + filePath + "\"");
	return ofJson::parse(jsonString);
}


ofVec2f ofxFFmpegUtils::getVideoResolution(const string & movieFilePath){

	string videoSize = ofSystem(ffProbeBinaryPath + " -v error -show_entries stream=width,height -of default=noprint_wrappers=1 \"" + movieFilePath + "\"");
	auto lines = ofSplitString(videoSize, "\n");
	int w = 0;
	int h = 0;
	ofRectangle r;
	for(auto & s : lines){
		ofStringReplace(s, " ", "");
		if(ofStringTimesInString(s, "width=")){
			ofStringReplace(s, "width=", "");
			w = ofToInt(s);
		}
		if(ofStringTimesInString(s, "height=")){
			ofStringReplace(s, "height=", "");
			h = ofToInt(s);
		}
	}
	ofLogNotice("ofxFFmpegUtils") << "detected resolution for video " << movieFilePath << " : [" << w << "x" << h << "]";
	return ofVec2f(w,h);
}


float ofxFFmpegUtils::getVideoFramerate(const string & movieFilePath){
//ffprobe -v 0 -of csv=p=0 -select_streams 0 -show_entries stream=r_frame_rate infile

	//https://askubuntu.com/questions/110264/how-to-find-frames-per-second-of-any-video-file

	string framerate = ofSystem(ffProbeBinaryPath + " -v 0 -of csv=p=0 -select_streams 0 -show_entries stream=r_frame_rate \"" + movieFilePath + "\"");
	auto split = ofSplitString(framerate, "/");
	if (split.size() != 2){
		ofLogError("ofxFFmpegUtils") << "can't detect framerate for video " << movieFilePath;
		return 0;
	}else{
		int val1 = ofToInt(split[0]);
		int val2 = ofToInt(split[1]);
		float fr = float(val1) / float(val2);
		ofLogNotice("ofxFFmpegUtils") << "detected framerate for video " << movieFilePath << " : " << fr;
		return fr;
	}
}


size_t ofxFFmpegUtils::convertToImageSequence(const string & movieFile, const string & imgFileExtension, float jpegQuality/*[0..1]*/,
											  const string & outputFolder, bool convertToGrayscale, int numFilenameDigits,
											ofVec2f resizeBox, ofVec2f cropToAspectRatio, float cropBalance){

	size_t jobID = jobCounter;
	jobCounter++;

	ofxExternalProcess * proc = new ofxExternalProcess();
	vector<string> args;

	//beware - this overwrites
	if(ofDirectory::doesDirectoryExist(ofToDataPath(outputFolder, true))){
		ofDirectory::removeDirectory(ofToDataPath(outputFolder, true), true); //remove old
	}
	ofDirectory::createDirectory(ofToDataPath(outputFolder, true), true, true);

	string imgNameScheme = ofToDataPath(outputFolder, true) + "/" + "output_" + "%0" + ofToString(numFilenameDigits) + "d." + imgFileExtension;

	//ffmpeg -i "$inputMovie" -q:v $jpegQuality -coder "raw" -y  -loglevel 40 "$folderName/output_%06d.$format"
	args = {
		"-i", ofToDataPath(movieFile, true),
		"-q:v", ofToString((int)ofMap(0, 1, 31, 1, true)),
		"-coder", "raw",
		"-y", //overwrite
		//"-pix_fmt", "yuv420p",
		"-loglevel", "40", //verbose
		imgNameScheme
	};

	auto fps = getVideoFramerate(movieFile);

	auto res = getVideoResolution(movieFile);
	int w = res.x;
	int h = res.y;
	ofRectangle r = ofRectangle(0,0,w,h); //the final img pixel size (may or may not be resized) b4 cropping
	ofRectangle resizeTarget = ofRectangle(0,0,resizeBox.x, resizeBox.y);

	bool isResizing = resizeBox.x > 0 && resizeBox.y > 0;
	bool isCropping = cropToAspectRatio.x > 0 && cropToAspectRatio.y > 0;

	ofJson json;
	json["framerate"] = fps;
	json["resolution"]["x"] = res.x;
	json["resolution"]["y"] = res.y;
	json["originalFile"] = movieFile;
	json["imgExtension"] = imgFileExtension;
	json["jpegQuality"] = jpegQuality;
	json["numFilenameDigits"] = numFilenameDigits;
	json["cropBalance"] = cropBalance;
	json["convertToGrayscale"] = convertToGrayscale;
	ofSaveJson(ofToDataPath(outputFolder, true) + "/info.json", json);

	vector<string> vfArgs;
	bool needVF = false;

	if( isResizing && !isCropping ){ //only resize

		r.scaleTo(resizeTarget);

		if(r.width != 0 && r.height != 0){
			needVF = true;
			vfArgs.push_back( "scale=" + ofToString(r.width,0) + ":" + ofToString(r.height,0) );
			isResizing = true;
		}else{
			ofLogError("ofxFFmpegUtils") << "cant get video res! cant resize video! " << movieFile;
		}
	}

	if(cropBalance < 0.0) cropBalance = 0.5; //if no balance defined (-1), crop in middle

	if(isCropping && !isResizing){ //only crop

		ofRectangle crop = ofRectangle(0,0,cropToAspectRatio.x,cropToAspectRatio.y);
		crop.scaleTo(r, OF_SCALEMODE_FIT);

		string command;

		if( int(crop.width) == int(r.width) ){ //we are cropping vertically - we must remove pix from top or bottom

			int diff = r.height - crop.height;
			int cropOffset = ofMap(cropBalance, 0, 1, 0, diff);
			command = "crop=" + ofToString(crop.width,0) + ":" + ofToString(crop.height,0) + ":" + "0" + ":" + ofToString(cropOffset);

		}else{ //we are cropping horizontally - we must remove pixels from left or right

			int diff = r.width - crop.width;
			int cropOffset = ofMap(cropBalance, 0, 1, 0, diff);
			command = "crop=" + ofToString(crop.width,0) + ":" + ofToString(crop.height,0) + ":" + ofToString(cropOffset) + ":" + "0";
		}

		needVF = true;
		vfArgs.push_back( command );
	}

	if(isCropping && isResizing){ //resize & crop

		ofRectangle crop = ofRectangle(0,0,cropToAspectRatio.x,cropToAspectRatio.y);
		crop.scaleTo(r, OF_SCALEMODE_FIT);
		crop.scaleTo(resizeTarget);

		string command;
		r.scaleTo(crop, OF_SCALEMODE_FILL);

		if( int(crop.width) == int(r.width) ){ //we are cropping vertically - we must remove pix from top or bottom

			int diff = r.height - crop.height;
			int cropOffset = ofMap(cropBalance, 0, 1, 0, diff);
			command = "scale=" + ofToString(r.width,0) + ":" + ofToString(r.height,0) + ",crop=" +
			ofToString(crop.width,0) + ":" + ofToString(crop.height,0) + ":" + "0" + ":" + ofToString(cropOffset);

		}else{ //we are cropping horizontally - we must remove pixels from left or right

			int diff = r.width - crop.width;
			int cropOffset = ofMap(cropBalance, 0, 1, 0, diff);
			command = "scale=" + ofToString(r.width,0) + ":" + ofToString(r.height,0) + ",crop=" +
			ofToString(crop.width,0) + ":" + ofToString(crop.height,0) + ":" + ofToString(cropOffset) + ":" + "0";
		}

		needVF = true;
		vfArgs.push_back( command );
	}


	if(maxThreadsPerJob > 0){
		ofLogNotice("ofxFFmpegUtils") << "limiting ffmpeg job to " << maxThreadsPerJob << " threads.";
		args.insert(args.begin(), ofToString(maxThreadsPerJob));
		args.insert(args.begin(), "-threads");
	}

	if(convertToGrayscale){
		needVF = true;
	}

	if(needVF){
		if(convertToGrayscale){ //note that grayscale conversion comes last!
			vfArgs.push_back("format=gray");
		}

		string totalVF;
		int c = 0;
		for(auto arg : vfArgs){
			totalVF += arg;
			c++;
			if(c < vfArgs.size()) totalVF += ",";
		}
		args.insert(args.begin() + args.size() - 1, "-vf");
		args.insert(args.begin() + args.size() - 1, totalVF);
	}

	proc->setup(
				".", 				//working dir
				ffmpegBinaryPath, 	//command
				args 				//args (std::vector<string>)
	);

	proc->setLivePipeOutputDelay(0);
	proc->setLivePipe(ofxExternalProcess::STDOUT_AND_STDERR_PIPE);

	//proc->executeInThreadAndNotify();
	JobInfo jobInfo;
	jobInfo.originalFile = movieFile;
	jobInfo.destinationFolder = outputFolder;
	jobInfo.process = proc;
	jobQueue[jobID] = jobInfo; //enqueue job
	return jobID;
}


void ofxFFmpegUtils::update(float dt){

	vector<size_t> toDelete;

	//look for finished processes, put them on delete list and notify
	for(auto p : activeProcesses){
		if (!p.second.process->isRunning()){
			ofLogNotice("ofxFFmpegUtils") << "job \"" << p.first << "\" done!";
			//ofLogNotice("ofxFFmpegUtils") << p.second->getCombinedOutput();
			JobResult r;
			r.jobID = p.first;
			r.inputFilePath = p.second.originalFile;
			r.outputFolder = p.second.destinationFolder;
			r.results = p.second.process->getLastExecutionResult();
			r.ok = r.results.statusCode == 0;
			ofNotifyEvent(eventJobCompleted, r, this);

			delete p.second.process;
			toDelete.push_back(p.first);
		}
	}

	//delete completed processes
	for(auto p : toDelete){
		activeProcesses.erase(p);
	}

	//spawn pending jobs if any, but only up to "maxSimultJobs" can run at the same time
	vector<size_t> toTransfer;
	for(auto & p : jobQueue){
		if(activeProcesses.size() + toTransfer.size() < maxSimultJobs){
			toTransfer.push_back(p.first);
		}else{
			break;
		}
	}

	for(auto & t : toTransfer){
		activeProcesses[t] = jobQueue[t];
		activeProcesses[t].process->executeInThreadAndNotify();
		jobQueue.erase(t);
	}

}


void ofxFFmpegUtils::drawDebug(int x, int y){
	ofPushMatrix();
	ofTranslate(x,y);
		string msg = "#### ofxFFmpegUtils ################################\n";
		msg += " Pending: " + ofToString(jobQueue.size()) + " Active: " + ofToString(activeProcesses.size()) + "\n";

		for(auto & p : activeProcesses){
			auto out = p.second.process->getCombinedOutput();
			auto lines = ofSplitString(out, "\n");
			if(lines.size() > 0){
				auto frames = ofSplitString(lines.back(), "\r");
				if(frames.size() > 0){
					msg += "   -P" + ofToString(p.first) + ": " + frames.back() + "\n";
				}
			}
		}
		ofDrawBitmapString(msg, 0, 0);
	ofPopMatrix();
}
