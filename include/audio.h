#ifndef AUDIO_H
#define AUDIO_H

#include <sndfile.h>
#include <portaudio.h>

#include <stdexcept>
#include <string>
#include <vector>
#include <map>
#include <cstring>

namespace Audio
{

namespace {

const int FRAMES_PER_BUFFER = 64;

static int instanceCallback(const void *input, void *output,
						 unsigned long framesPerBuffer,
						 const PaStreamCallbackTimeInfo* time,
						 PaStreamCallbackFlags statusFlags,
						 void *userData);

struct AudioData
{
	AudioData()
	{
		data = nullptr;
		sampleCount = 0;
	}
	AudioData(std::string filename)
	{
		SNDFILE* file = sf_open(filename.c_str(), SFM_READ, &this->info);
    	if (sf_error(file) != SF_ERR_NO_ERROR)
			throw std::runtime_error("failed to load audio data at " + filename);
		this->sampleCount = info.frames * info.channels;
		this->data = new float[sampleCount];
		if(sf_readf_float(file, this->data, info.frames) != info.frames)
		{
			sf_close(file);
			throw std::runtime_error("incorect num of sampels loaded from audio data at " + filename);
		}
		sf_close(file);

		this->filename = filename;
	}
	~AudioData()
	{
		delete[] data;
	}
	float* data;
	SF_INFO  info;
	sf_count_t sampleCount;
	std::string filename;
};

struct Instance
{
	Instance(AudioData* audio, bool loop, float volume)
	{
		this->current = 0;
		this->audio = audio;
		this->loop = loop;
		this->volume = volume;
		this->paused = false;
		PaError err = Pa_OpenDefaultStream(&stream,
											0,
											audio->info.channels,
											paFloat32,
											audio->info.samplerate,
											FRAMES_PER_BUFFER,
											instanceCallback,
											this);
		if(err != paNoError)
			throw std::runtime_error("failed to open default stream for file at " + this->audio->filename);
	}

	~Instance()
	{
		Pa_CloseStream(stream);
		//if(err != paNoError) throw std::runtime_error("failed to close stream");
	}

	void Play()
	{
		this->paused = false;
		PaError err = Pa_StartStream(stream);
		if(err != paNoError)
			throw std::runtime_error("failed to start stream at " + audio->filename + " Pa Error: " + std::string(Pa_GetErrorText(err)));
	}

	void Pause()
	{
		this->paused = true;
		PaError err = Pa_StopStream(stream);
		if(err != paNoError)
			throw std::runtime_error("failed to stop stream at " + audio->filename + " Pa Error: " + std::string(Pa_GetErrorText(err)));
	}

	bool Playing()
	{
		return Pa_IsStreamActive(stream);
	}

	AudioData* audio;
	int current;
	float volume;
	PaStream *stream;
	bool loop;
	bool paused;
};

static int instanceCallback(const void *input, void *output,
						 unsigned long framesPerBuffer,
						 const PaStreamCallbackTimeInfo* time,
						 PaStreamCallbackFlags statusFlags,
						 void *userData)
{
	Instance *data = (Instance*)userData;
	float *out = (float*)output;
	for (unsigned int frame = 0; frame < framesPerBuffer; frame++)
		for (int channel = 0; channel < data->audio->info.channels; channel++)
		{
			*out++ = data->audio->data[data->current++] * data->volume;
			if (data->current >= data->audio->sampleCount)
			{
				if(data->loop)
					data->current = 0;
				else
				{
					memset(out, 0, (framesPerBuffer - frame - 1) * data->audio->info.channels);
					return paComplete;
				}
			}
		}
	return paContinue;
}

} //unnamed end

class Manager
{
public:
	Manager()
	{
		if(sizeof(float) != 4)
			throw std::runtime_error("float must be 32bit");
		PaError err;
		err = Pa_Initialize();
		if(err != paNoError) throw std::runtime_error("Failed to initialize Port Audio\n Err: " + std::string(Pa_GetErrorText(err)));
	}
	~Manager()
	{
		StopAll();
		for(auto &audio : loaded)
		{
			delete audio.second;
		}
		PaError err = Pa_Terminate();
		//if(err != paNoError) throw std::runtime_error("failed to terminate Port Audio");
	}

	void RemovePlayed()
	{
		for (size_t i = 0; i < activeAudio.size(); i++)
			if(!activeAudio[i]->paused && !activeAudio[i]->Playing())
				activeAudio.erase(activeAudio.begin() + i--);
	}

//optional, can frontload loading to make Play() faster the first time on a file
	void LoadAudioFile(std::string filename)
	{
		loaded[filename] = new AudioData(filename);
	}

	void Play(std::string filename, bool loop, float volume)
	{
		if(loaded.count(filename) == 0)
			this->LoadAudioFile(filename);
		activeAudio.push_back(new Instance(loaded[filename], loop, volume));
		activeAudio.back()->Play();
	}

	void Resume(std::string filename)
	{
		for (size_t i = 0; i < activeAudio.size(); i++)
			if(activeAudio[i]->audio->filename == filename && activeAudio[i]->paused)
				activeAudio[i]->Play();
	}

	void Pause(std::string filename)
	{
		for (size_t i = 0; i < activeAudio.size(); i++)
			if(activeAudio[i]->audio->filename == filename && !activeAudio[i]->paused)
					activeAudio[i]->Pause();
	}

	void Stop(std::string filename)
	{
		for (size_t i = 0; i < activeAudio.size(); i++)
			if(activeAudio[i]->audio->filename == filename)
			{
				delete activeAudio[i];
				activeAudio.erase(activeAudio.begin() + i--);
			}
	}

	void StopAll()
	{
		for (size_t i = 0; i < activeAudio.size(); i++)
			delete activeAudio[i];
		activeAudio.clear();
	}

	void SetVolume(std::string filename, float volume)
	{
		for (size_t i = 0; i < activeAudio.size(); i++)
			if(activeAudio[i]->audio->filename == filename)
				activeAudio[i]->volume = volume;
	}

	bool Playing(std::string filename)
	{
		for (size_t i = 0; i < activeAudio.size(); i++)
			if(activeAudio[i]->audio->filename == filename && activeAudio[i]->Playing())
				return true;
		return false;
	}

private:
	std::map<std::string, AudioData*> loaded;
	std::vector<Instance*> activeAudio;
};

} //audio namespace end

#endif
