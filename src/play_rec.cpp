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
#include <vector>
#include "scope_guard.h"
#include <cstring>

#ifdef _MSC_VER 
extern "C" {
#include "util/evil_string.h"
}
#endif

std::atomic<bool> done{false};
std::atomic<bool> input_ready{false};

struct OutputData{
	drwav* ch1;
	drwav* ch2;
	std::vector<int16_t> temp_buffer;
	
	// do the float->double conversion (we really really
	// want to use f64 as resulting wav format),
	// and we want to split the two channels
	int16_t * fill_temp(float * buffer, int stride, int frameCount){
		if(temp_buffer.size()<frameCount){
			temp_buffer.resize(frameCount);
		}
		for(int i = 0; i < frameCount; i++){
			temp_buffer[i] = buffer[i*stride]*32767;
		}
		return temp_buffer.data();
	}
};

std::vector<float> outbuff;

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
	if(pOutput != 0 && input_ready){
		ma_decoder* decoder = (ma_decoder*)pDevice->pUserData;
		if (decoder == NULL) {
			return;
		}
		
		if (outbuff.size() < frameCount) outbuff.resize(frameCount);

		// play mono out...
		size_t len = ma_decoder_read_pcm_frames(decoder, outbuff.data(), frameCount);
		// ...and make it stereo
		float * dest = (float*)pOutput;
		for (int i = 0; i < frameCount; i++) {
			dest[2 * i + 0] = outbuff[i];
			dest[2 * i + 1] = outbuff[i];
		}

		if(len<=0){
			done = true;
		}
	}
	if(pInput != 0){
		input_ready = true;
		OutputData * out_d = (OutputData*)pDevice->pUserData;
		int stride = pDevice->capture.channels;
		float * in = (float*)pInput;
		drwav_write_pcm_frames(
							   out_d->ch1,
							   frameCount,
							   out_d->fill_temp(in, stride, frameCount)
							   );
		drwav_write_pcm_frames(
							   out_d->ch2,
							   frameCount,
							   out_d->fill_temp(in+1, stride, frameCount)
							   );
	}
}


int play_rec(
			 const char * in_file,
			 const char * out_file_1,
			 const char * out_file_2,
			 uint32_t in_dev_idx,
			 uint32_t out_dev_idx
			  ){
	
	int sample_rate = 192000;
	
	ma_context context;
	ma_result result;
	ma_decoder decoder;
	ma_device_config out_config;
	ma_device_config in_config;
	ma_device out_device;
	ma_device in_device;
	scope_guard scope_exit;

	ma_decoder_config decoder_config;
	decoder_config.channels = 1;
	decoder_config.format = ma_format_f32;
	decoder_config.sampleRate = sample_rate;
	result = ma_decoder_init_file(in_file, &decoder_config, &decoder);
	if (result != MA_SUCCESS) {
		return -2;
	}
	scope_exit += [&](){ ma_decoder_uninit(&decoder);};
	
	drwav_data_format encoder_format;
	encoder_format.container     = drwav_container_riff;
	encoder_format.format        = DR_WAVE_FORMAT_PCM;
	encoder_format.channels      = 1;
	encoder_format.sampleRate    = sample_rate;
	encoder_format.bitsPerSample = 16;
	
	drwav capture_wav_1;
	if (drwav_init_file_write(&capture_wav_1, out_file_1, &encoder_format, NULL) == DRWAV_FALSE) {
		printf("Failed to initialize output file (ch 1).\n");
		return -1;
	}
	scope_exit += [&](){ drwav_uninit(&capture_wav_1); };
	
	drwav capture_wav_2;
	if (drwav_init_file_write(&capture_wav_2, out_file_2, &encoder_format, NULL) == DRWAV_FALSE) {
		printf("Failed to initialize output file (ch2).\n");
		return -1;
	}
	scope_exit += [&](){ drwav_uninit(&capture_wav_2); };
	
	if (ma_context_init(NULL, 0, NULL, &context) != MA_SUCCESS) {
		printf("Failed to initialize context.\n");
		return -2;
	}
	scope_exit += [&](){ ma_context_uninit(&context);};
	
	ma_device_info* pPlaybackDeviceInfos;
	ma_uint32 playbackDeviceCount;
	ma_device_info* pCaptureDeviceInfos;
	ma_uint32 captureDeviceCount;
	result = ma_context_get_devices(&context, &pPlaybackDeviceInfos, &playbackDeviceCount, &pCaptureDeviceInfos, &captureDeviceCount);
	if (result != MA_SUCCESS) {
		printf("Failed to retrieve device information.\n");
		return -3;
	}
	
	if(captureDeviceCount==0){
		printf("No capture device found\n");
		return 1;
	}
	if(playbackDeviceCount==0){
		printf("No playback device found\n");
		return 1;
	}
	
	if(in_dev_idx>=captureDeviceCount){
		printf("Ignoring capture device choice\n");
		in_dev_idx = 0;
	}
	
	if(out_dev_idx>=playbackDeviceCount){
		printf("Ignoring playback device choice\n");
		out_dev_idx = 0;
	}

	printf("Using %s as playback device\n", pPlaybackDeviceInfos[out_dev_idx].name);
	printf("Using %s as capture device\n", pCaptureDeviceInfos[in_dev_idx].name);

	

	out_config = ma_device_config_init(ma_device_type_playback);
	out_config.playback.pDeviceID = &pPlaybackDeviceInfos[out_dev_idx].id;
	out_config.playback.format   = ma_format_f32;
	out_config.playback.channels = 2;
	out_config.sampleRate        = sample_rate;
	out_config.dataCallback      = data_callback;
	out_config.pUserData         = &decoder;
	if (ma_device_init(NULL, &out_config, &out_device) != MA_SUCCESS) {
		printf("Failed to open playback device.\n");
		return -3;
	}
	scope_exit += [&](){ ma_device_uninit(&out_device);};
	
	in_config = ma_device_config_init(ma_device_type_capture);
	in_config.playback.pDeviceID = &pCaptureDeviceInfos[in_dev_idx].id;
	in_config.capture.format = ma_format_f32;
	in_config.capture.channels = 2;
	in_config.sampleRate        = sample_rate;
	in_config.dataCallback      = data_callback;
	OutputData out_user_data{&capture_wav_1, &capture_wav_2};
	in_config.pUserData         = &out_user_data;
	if (ma_device_init(NULL, &in_config, &in_device) != MA_SUCCESS) {
		printf("Failed to open capture device.\n");
		return -3;
	}
	scope_exit += [&](){ ma_device_uninit(&in_device);};
	
	if (ma_device_start(&out_device) != MA_SUCCESS) {
		printf("Failed to start playback device.\n");
		return -4;
	}
	
	if (ma_device_start(&in_device) != MA_SUCCESS) {
		printf("Failed to start capture device.\n");
		return -4;
	}
	

	while(!done){
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	
	std::cout << "playback + recording complete" << std::endl;
	std::cout << "wrote ch1 to " << out_file_1 << std::endl;
	std::cout << "wrote ch2 to " << out_file_2 << std::endl;

	// this is now taken care of by the scope guard:
	//drwav_uninit(&capture_wav);
	//ma_device_uninit(&in_device);
	//ma_device_uninit(&out_device);
	//ma_decoder_uninit(&decoder);
	//ma_context_uninit(&context);

	return 0;
}


int list_devices(){
	std::cout << "Device list: " << std::endl;

	ma_result result;
	ma_context context;
	ma_device_info* pPlaybackDeviceInfos;
	ma_uint32 playbackDeviceCount;
	ma_device_info* pCaptureDeviceInfos;
	ma_uint32 captureDeviceCount;
	ma_uint32 iDevice;
	
	if (ma_context_init(NULL, 0, NULL, &context) != MA_SUCCESS) {
		printf("Failed to initialize context.\n");
		return -2;
	}
	
	result = ma_context_get_devices(&context, &pPlaybackDeviceInfos, &playbackDeviceCount, &pCaptureDeviceInfos, &captureDeviceCount);
	if (result != MA_SUCCESS) {
		printf("Failed to retrieve device information.\n");
		return -3;
	}
	
	printf("Playback Devices\n");
	for (iDevice = 0; iDevice < playbackDeviceCount; ++iDevice) {
		printf("    %u: %s\n", iDevice, pPlaybackDeviceInfos[iDevice].name);
	}
	
	printf("\n");
	
	printf("Capture Devices\n");
	for (iDevice = 0; iDevice < captureDeviceCount; ++iDevice) {
		printf("    %u: %s\n", iDevice, pCaptureDeviceInfos[iDevice].name);
	}
	
	
	ma_context_uninit(&context);
	return 0;
}

int find_in_dev(const char * search_name){
	ma_result result;
	ma_context context;
	ma_device_info* pPlaybackDeviceInfos;
	ma_uint32 playbackDeviceCount;
	ma_device_info* pCaptureDeviceInfos;
	ma_uint32 captureDeviceCount;
	ma_uint32 iDevice;
	
	if (ma_context_init(NULL, 0, NULL, &context) != MA_SUCCESS) {
		printf("Failed to initialize context.\n");
		return -2;
	}
	
	result = ma_context_get_devices(&context, &pPlaybackDeviceInfos, &playbackDeviceCount, &pCaptureDeviceInfos, &captureDeviceCount);
	if (result != MA_SUCCESS) {
		printf("Failed to retrieve device information.\n");
		return -3;
	}
	
	for (iDevice = 0; iDevice < captureDeviceCount; ++iDevice) {
		const char * name = pCaptureDeviceInfos[iDevice].name;
		if(strcasecmp(search_name, name) == 0){
			return iDevice;
		}
	}
	
	for (iDevice = 0; iDevice < captureDeviceCount; ++iDevice) {
		const char * name = pCaptureDeviceInfos[iDevice].name;
		if(strcasestr(name, search_name) != NULL){
			return iDevice;
		}
	}
	

	ma_context_uninit(&context);
	return 0;
}

int find_out_dev(const char * search_name){
	ma_result result;
	ma_context context;
	ma_device_info* pPlaybackDeviceInfos;
	ma_uint32 playbackDeviceCount;
	ma_device_info* pCaptureDeviceInfos;
	ma_uint32 captureDeviceCount;
	ma_uint32 iDevice;
	
	if (ma_context_init(NULL, 0, NULL, &context) != MA_SUCCESS) {
		printf("Failed to initialize context.\n");
		return -2;
	}
	
	result = ma_context_get_devices(&context, &pPlaybackDeviceInfos, &playbackDeviceCount, &pCaptureDeviceInfos, &captureDeviceCount);
	if (result != MA_SUCCESS) {
		printf("Failed to retrieve device information.\n");
		return -3;
	}
	
	for (iDevice = 0; iDevice < playbackDeviceCount; ++iDevice) {
		const char * name = pPlaybackDeviceInfos[iDevice].name;
		if(strcasecmp(search_name, name) == 0){
			return iDevice;
		}
	}
	
	for (iDevice = 0; iDevice < playbackDeviceCount; ++iDevice) {
		const char * name = pPlaybackDeviceInfos[iDevice].name;
		if(strcasestr(name, search_name) != NULL){
			return iDevice;
		}
	}
	
	
	ma_context_uninit(&context);
	return 0;
}
