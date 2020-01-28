#include "args.hxx"
#include <iostream>
#include "play_rec.h"

bool is_alphanum(const char * name);

int main(int argc, char** argv)
{
	args::ArgumentParser parser("play-rec", "Tiny tool to play back and record an audio file (wav, mp3 or flac)");
	args::HelpFlag help(parser, "help", "Display help menu and the device list", {'h', "help"});
	args::ValueFlag<std::string> in_dev(parser, "in-dev", "Input device (either by index, or by device name", {"in-dev"});
	args::ValueFlag<std::string> out_dev(parser, "out-dev", "Output device (either by index, or by device name", {"out-dev"});
	args::ValueFlag<std::string> in_file(parser, "in-file", "Input file (wav, mp3 or flac)",{"in-file"});
		args::ValueFlag<std::string> out_file(parser, "out-file", "Output file (wav)",{"out-file"});
	args::Flag list(parser, "list", "List audio devices", {"list"});
	args::CompletionFlag completion(parser, {"complete"});

	try
	{
		parser.ParseCLI(argc, argv);
	}
	catch (const args::Completion& e)
	{
		std::cout << e.what();
		return 0;
	}
	catch (const args::Help&)
	{
		std::cout << parser;
		return 0;
	}
	catch (const args::ParseError& e)
	{
		std::cerr << e.what() << std::endl;
		std::cerr << parser;
		return 1;
	}
	catch (args::ValidationError e)
	{
		std::cerr << e.what() << std::endl;
		std::cerr << parser;
		return 1;
	}
	
	if(list){
		list_devices();
	}
	
	
	int in_idx = 0;
	int out_idx = 0;
	
	if(in_dev){
		const char * name = in_dev.Get().c_str();
		if(is_alphanum(name)){
			in_idx = atoi(name);
		}
		else{
			in_idx = find_in_dev(name);
			if(!out_dev) out_idx = find_out_dev(name);
		}
	}
	
	if(out_dev){
		const char * name = out_dev.Get().c_str();
		if(is_alphanum(name)){
			out_idx = atoi(name);
		}
		else{
			out_idx = find_out_dev(name);
			if(!in_dev) in_idx = find_in_dev(name);
		}
	}
	
	
	if(!in_file || !out_file){
		std::cerr << parser << std::endl;
		std::cerr << std::endl;
		
		if(!in_file) std::cerr << "Input file (--in) is required" << std::endl;
		if(!out_file) std::cerr << "Output file (--out) is required" << std::endl;
		play_rec("/Users/hansi/Downloads/youtube-dl/boing.wav", "out.wav", in_idx, out_idx);
		return 1;
	}
	
	play_rec(in_file.Get().c_str(), out_file.Get().c_str(), in_idx, out_idx);
	
	return 0;
}

bool is_alphanum(const char * name){
	for(int idx = 0; idx < 1024; idx++){
		if(name[idx] == 0) return true;
		if(name[idx] < '0' || name[idx] > '9') return false;
	}
	// wtf
	return false;
}
