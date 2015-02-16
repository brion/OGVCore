//
// Platform-independent Ogg Vorbis/Theora/Opus decoder/player base
//
// Copyright (c) 2013-2015 Brion Vibber <brion@pobox.com>
// MIT-style license
// Please reuse and redistribute with the LICENSE notes intact.
//


struct OGVCoreFrameLayout {
	int pictureWidth;
	int pictureHeight;
	int frameWidth;
	int frameHeight;
	int pictureOffsetX;
	int pictureOffsetY;
	int horizontalDecimation;
	int verticalDecimation;
};

struct OGVCoreFrameBuffer {
	OGVCoreVideoInfo *layout;
	double timestamp;

	const char *bytesY;
	const char *bytesCb;
	const char *bytesCr;
	
	int strideY;
	int strideCb;
	int strideCr;
};


struct OGVCoreAudioLayout {
	int channelCount;
	int sampleRate;
}

struct OGVCoreAudioBuffer {
	OGVCoreAudioInfo *layout;
	int sampleCount;
	const float **samples;
};


// forward decl
class OGVCoreDecoderPrivate;

///
/// Platform-independent class for wrapping the decoder
///
class OGVCoreDecoder {
public:
	OGVCoreDecoder(OGVCorePlayerBackend *backend);
	~OGVCoreDecoder();

	void receiveInput(const char *buffer, int bufsize);
	void process();

	void decodeFrame();
	OGVCoreFrameBuffer *dequeueFrame();
	void discardFrame();

	void decodeAudio();
	OGVCoreAudioBuffer *dequeueAudio();
	void discardAudio();

	void flushBuffers();	

#ifdef SKELETON
	/**
	 * @return segment length in bytes
	 */
	long getSegmentLength();
	/**
	 * @return segment duration in seconds, or -1 if unknown
	 */
	double getDuration();
	long getKeypointOffset(long time_ms);
#endif

private:
	OGVCoreDecoderPrivate *impl;
};


///
/// Abstract class for JS, Cocoa, etc backends to implement
/// platform-specific behavior...
///
abstract class OGVCorePlayerBackend {
public:

	virtual void scheduleTimeout(double aDelay);
	
	virtual void drawFrame(OGVCoreFrameBuffer *aFrame);
	
	virtual void startAudio(OGVCoreAudioLayout *aLayout);
	virtual void bufferAudio(OGVCoreAudioBuffer *aBuffer);
	virtual void playAudio();
	virtual void endAudio();
	
	virtual void setDownloadURL(const char *aUrl);
	virtual void startDownload();
	virtual void seekDownload(long long aPos);
	virtual void continueDownload();
	virtual void endDownload();
};


///
/// Platform-independent parts of the frontend player widget.
/// Pair with a backend class to implement the drawing, audio,
/// timing, and network code.
///
class OGVCorePlayer {
public:

	OGVCorePlayer(OGVCorePlayerBackend *backend);
	~OGVCorePlayer();

	void load();
	void process();
	
	double getDuration();
	double getVideoWidth();
	double getVideoHeight();
	
	const char *getSourceURL();
	void setSourceURL(const char *aUrl);
	
	double getCurrentTime();
	void setCurrentTime(double aTime);
	
	bool getPaused();
	void setPaused(bool aPaused);
	
	bool getPlaying();
	bool getSeeking();

private:
	OGVCorePlayerPrivate *impl;
}

