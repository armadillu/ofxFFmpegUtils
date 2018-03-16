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

void ofxFFmpegUtils::setup(string ffmpegBinaryPath, string ffProbeBinaryPath){
	this->ffmpegBinaryPath = ffmpegBinaryPath;
	this->ffProbeBinaryPath = ffProbeBinaryPath;
}


ofVec2f ofxFFmpegUtils::getVideoResolution(string movieFilePath){

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


void ofxFFmpegUtils::convertToImageSequence(string movieFile, string imgFileExtension,
											float jpegQuality/*[0..1]*/, string outputFolder, int numFilenameDigits,
											ofVec2f resizeBox, ofVec2f cropToAspectRatio, float cropBalance){

	ofxExternalProcess * proc = new ofxExternalProcess();
	vector<string> args;

	//beware - this overwrites
	ofDirectory::removeDirectory(ofToDataPath(outputFolder, true), true);
	ofDirectory::createDirectory(ofToDataPath(outputFolder, true), true, true);

	string imgNameScheme = ofToDataPath(outputFolder, true) + "/" + "output_" + "%0" + ofToString(numFilenameDigits) + "d." + imgFileExtension;

	//ffmpeg -i "$inputMovie" -q:v $jpegQuality -coder "raw" -y  -loglevel 40 "$folderName/output_%06d.$format"
	args = {
		"-i", ofToDataPath(movieFile, true),
		"-q:v", ofToString((int)ofMap(0, 1, 31, 1, true)),
		"-coder", "raw",
		"-y", //overwrite
		"-loglevel", "40", //verbose
		imgNameScheme
	};

	auto res = getVideoResolution(movieFile);
	int w = res.x;
	int h = res.y;
	ofRectangle r = ofRectangle(0,0,w,h); //the final img pixel size (may or may not be resized) b4 cropping
	ofRectangle resizeTarget = ofRectangle(0,0,resizeBox.x, resizeBox.y);

	bool isResizing = resizeBox.x > 0 && resizeBox.y > 0;
	bool isCropping = cropToAspectRatio.x > 0 && cropToAspectRatio.y > 0;

	if(isResizing && !isCropping ){ //user provided a resize box

		r.scaleTo(resizeTarget);

		if(r.width != 0 && r.height != 0){
			args.insert(args.begin() + args.size() - 1, "-vf");
			args.insert(args.begin() + args.size() - 1, "scale=" + ofToString(r.width,0) + ":" + ofToString(r.height,0));
			isResizing = true;
		}else{
			ofLogError("ofxFFmpegUtils") << "cant get video res! cant resize video! " << movieFile;
		}
	}

	if(isCropping && isResizing){ //user provided a crop res

		//r.scaleTo(ofRectangle(0,0,resizeBox.x, resizeBox.y));

		ofRectangle crop = ofRectangle(0,0,cropToAspectRatio.x,cropToAspectRatio.y);
		crop.scaleTo(r, OF_SCALEMODE_FIT);

		crop.scaleTo(resizeTarget);

		string command;

		//at this point, crop holds the final resolution we will output to.
		//but we still need to decide where to crop (ie left or right)

		if(cropBalance < 0.0) cropBalance = 0.5;
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

		args.insert(args.begin() + args.size() - 1, "-vf");
		args.insert(args.begin() + args.size() - 1, command);
	}

	proc->setup(
				".", 				//working dir
				ffmpegBinaryPath, 	//command
				args 				//args (std::vector<string>)
	);

	proc->setLivePipeOutputDelay(0);
	proc->setLivePipe(ofxExternalProcess::STDOUT_AND_STDERR_PIPE);

	proc->executeInThreadAndNotify();

	ProcessInfo info;
	processes[proc] = info;
}

void ofxFFmpegUtils::update(float dt){

	vector<ofxExternalProcess*> toDelete;
	for(auto p : processes){
		if (!p.first->isRunning()){
			ofLogNotice("ofxFFmpegUtils") << "ofxFFmpegUtils done!";
			ofLogNotice("ofxFFmpegUtils") << p.first->getCombinedOutput();
			toDelete.push_back(p.first);
		}
	}

	for(auto p : toDelete){
		processes.erase(p);
	}

}


void ofxFFmpegUtils::drawDebug(int x, int y){
	ofPushMatrix();
	ofTranslate(x,y);
	string msg;
	for(auto & p : processes){
		msg += p.first->getCombinedOutput();
		msg += "\n\n";
	}
		ofDrawBitmapString(msg, 0, 0);
	ofPopMatrix();
}