#define DR_FLAC_IMPLEMENTATION
#include "extras/dr_flac.h"  /* Enables FLAC decoding. */
#define DR_MP3_IMPLEMENTATION
#include "extras/dr_mp3.h"   /* Enables MP3 decoding. */
#define DR_WAV_IMPLEMENTATION
#include "extras/dr_wav.h"   /* Enables WAV decoding. */

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include <atomic>
#include <chrono>
#include <thread>
#include <iostream>

std::atomic<bool> done{false};

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
	ma_decoder* pDecoder = (ma_decoder*)pDevice->pUserData;
	if (pDecoder == NULL) {
		return;
	}
	
	int len = ma_decoder_read_pcm_frames(pDecoder, pOutput, frameCount);
	if(len<=0){
		done = true;
	}
	
	(void)pInput;
}

int main(int argc, char** argv)
{
	ma_result result;
	ma_decoder decoder;
	ma_device_config deviceConfig;
	ma_device device;
	
	const char * filename = "/Users/hansi/Downloads/youtube-dl/boing.wav";
	result = ma_decoder_init_file(filename, NULL, &decoder);
	if (result != MA_SUCCESS) {
		return -2;
	}
	
	deviceConfig = ma_device_config_init(ma_device_type_playback);
	deviceConfig.playback.format   = decoder.outputFormat;
	deviceConfig.playback.channels = decoder.outputChannels;
	deviceConfig.sampleRate        = decoder.outputSampleRate;
	deviceConfig.dataCallback      = data_callback;
	deviceConfig.pUserData         = &decoder;
	
	if (ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS) {
		printf("Failed to open playback device.\n");
		ma_decoder_uninit(&decoder);
		return -3;
	}
	
	if (ma_device_start(&device) != MA_SUCCESS) {
		printf("Failed to start playback device.\n");
		ma_device_uninit(&device);
		ma_decoder_uninit(&decoder);
		return -4;
	}
	
	while(!done){
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	
	std::cout << "done" << std::endl;
	
	ma_device_uninit(&device);
	ma_decoder_uninit(&decoder);
	
	return 0;
}
