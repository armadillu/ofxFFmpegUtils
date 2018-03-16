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


void ofxFFmpegUtils::convertToImageSequence(string movieFile, string imgFileExtension,
												   float jpegQuality/*[0..1]*/, string outputFolder, ofVec2f resizeBox, int numFilenameDigits){

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

	if(resizeBox.x != 0){
		//find out video res
		string videoSize = ofSystem(ffProbeBinaryPath + " -v error -show_entries stream=width,height -of default=noprint_wrappers=1 \"" + movieFile + "\"");
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
		ofLogNotice() << "detected resolution: " << w << " x " << h;
		r = ofRectangle(0,0,w,h);
		r.scaleTo(ofRectangle(0,0,resizeBox.x, resizeBox.y));

		if(r.width != 0 && r.height != 0){
			args.insert(args.begin() + args.size() - 1, "-vf");
			args.insert(args.begin() + args.size() - 1, "scale=" + ofToString(r.width,0) + ":" + ofToString(r.height,0));
		}else{
			ofLogError() << "cant get video res! cant resize video! " << movieFile;
		}
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
			ofLogNotice() << "ofxFFmpegUtils done!";
			ofLogNotice() << p.first->getCombinedOutput();
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