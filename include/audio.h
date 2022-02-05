#ifndef AUDIO_H
#define AUDIO_H


#if defined(_WIN32) || defined(WIN32)

#include <tchar.h>
#include <Windows.h>

#include <string>
#include <iostream>
#include <thread>
#include <cstring>

class Audio
{
public:
	Audio(std::string filename)
	{
		if (std::strcmp(filename.substr(filename.find_last_of('.'), 3).c_str(), "mp3"))
			isMp3 = true;
		this->filename = filename;
		std::string cmd = "open \"" + filename + "\" alias " + filename;
		mciSendStringA(cmd.c_str(), NULL, 0, 0);
	}
	Audio() {}
	~Audio()
	{
		std::string cmd = "close " + filename;
		mciSendStringA(cmd.c_str(), NULL, 0, 0);
	}

	void play()
	{
		std::string cmd = "play " + filename;

		mciSendStringA(cmd.c_str(), NULL, 0, 0);
		playing = true;
	}

	void pause()
	{
		std::string cmd = "pause " + filename;

		mciSendStringA(cmd.c_str(), NULL, 0, 0);
		playing = false;
	}

	void stop()
	{
		std::string cmd = "stop " + filename;

		mciSendStringA(cmd.c_str(), NULL, 0, 0);
		playing = false;
	}

	void loop()
	{
		if (!isMp3)
			throw std::runtime_error("non mp3 files do not support looping, convert " + filename + " to mp3!");
		std::string cmd = "play " + filename + " repeat";

		mciSendStringA(cmd.c_str(), NULL, 0, 0);
		playing = true;
	}

	void setVolume(float volume)
	{
		if (!isMp3)
			throw std::runtime_error("non mp3 files do not support volume setting, convert " + filename + " to mp3!");
		volume < 0 ? volume = 0 : volume > 1 ? volume = 1 : volume = volume;
		int vol = (int)(volume * 1000.0f);
		std::string msg = "setaudio " + filename + " volume to " + std::to_string(vol);
		mciSendStringA(msg.c_str(), NULL, 0, 0);
	}


	bool isplaying()
	{
		if(!playing)
			return playing;
		else
		{
			char* out = new char[8];
			std::string cmd = "status " + filename + " mode";
			mciSendStringA(cmd.c_str(), out, 8, 0);
			if (strncmp(out, "paused", 6) == 0 || strncmp(out, "stopped", 7) == 0)
				playing = false;
			delete[] out;
		}
	}

	void oneTime(std::string path, float volume)
	{
		std::string cmd = "open \"" + path + "\" alias " + path + std::to_string(count);
		mciSendStringA(cmd.c_str(), NULL, 0, 0);
		cmd = "play " + path + std::to_string(count);
		mciSendStringA(cmd.c_str(), NULL, 0, 0);
		volume < 0 ? volume = 0 : volume > 1 ? volume = 1 : volume = volume;
		int vol = volume * 1000;
		cmd = "setaudio " + path + std::to_string(count) + " volume to " + std::to_string(vol);
		mciSendStringA(cmd.c_str(), NULL, 0, 0);
		count++;
	}

private:
	int count = 0;
	std::string filename;
	bool playing = false;
	bool isMp3 = false;
};




#else

#include <iostream>

class Audio
{
public:
	Audio(std::string filename)
	{
		std::cout << "audio not supported on this OS" << std::endl;
	}
	Audio() {}
	~Audio()
	{

	}

	void play()
	{

	}

	void pause()
	{

	}

	void stop()
	{

	}

	void loop()
	{

	}

	void setVolume(float volume)
	{
;
	}


	bool isplaying()
	{

		return false;
	}

	void oneTime(std::string path, float volume)
	{

	}

private:
	std::string filename;
	bool playing = false;
	bool isMp3 = false;
};


#endif //os end

#endif //header end
