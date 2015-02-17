//
// Platform-independent Ogg Vorbis/Theora/Opus decoder/player base
//
// Copyright (c) 2013-2015 Brion Vibber <brion@pobox.com>
// MIT-style license
// Please reuse and redistribute with the LICENSE notes intact.
//

#include <string.h>
#include <memory>

namespace OGVCore {

	struct FrameLayout {
		int frameWidth;
		int frameHeight;
		int pictureWidth;
		int pictureHeight;
		int pictureOffsetX;
		int pictureOffsetY;
		int horizontalDecimation;
		int verticalDecimation;
		double aspectRatio;
		double fps;
	};

	struct FrameBuffer {
		FrameLayout *layout;
		double timestamp;
		double keyframeTimestamp;

		const unsigned char *bytesY;
		const unsigned char *bytesCb;
		const unsigned char *bytesCr;
	
		int strideY;
		int strideCb;
		int strideCr;
	};


	struct AudioLayout {
		int channelCount;
		int sampleRate;
	};

	struct AudioBuffer {
		AudioLayout *layout;
		int sampleCount;
		const float **samples;
	
		AudioBuffer(AudioLayout *aLayout, int aSampleCount, const float **aSamples)
		{
			layout = aLayout;
			int n = layout->channelCount;
			sampleCount = aSampleCount;
			samples = new const float *[n];
			for (int i = 0; i < n; i++) {
				samples[i] = (const float *)new float[sampleCount];
				memcpy((void *)aSamples[i], (void *)samples[i], sampleCount * sizeof(float));
			}
		}
	};


	///
	/// Platform-independent class for wrapping the decoder
	///
	class Decoder {
	public:
		Decoder();
		~Decoder();
	
		bool hasAudio();
		bool hasVideo();
		bool isAudioReady();
		bool isFrameReady();
		AudioLayout *getAudioLayout();
		FrameLayout *getFrameLayout();

		void receiveInput(const char *buffer, int bufsize);
		bool process();

		bool decodeFrame();
		FrameBuffer *dequeueFrame();
		void discardFrame();

		bool decodeAudio();
		AudioBuffer *dequeueAudio();
		void discardAudio();

		void flush();	

		/**
		 * @return segment length in bytes
		 */
		long getSegmentLength();
		/**
		 * @return segment duration in seconds, or -1 if unknown
		 */
		double getDuration();
		long getKeypointOffset(long time_ms);

	private:
		class impl; std::unique_ptr<impl> pimpl;
	};


	///
	/// Abstract class for JS, Cocoa, etc backends to implement
	/// platform-specific behavior...
	///
	class PlayerBackend {
	public:

		virtual double getTimestamp() = 0;
		virtual void setTimeout(double aDelay) = 0;
	
		virtual void drawFrame(FrameBuffer *aFrame) = 0;
	
		virtual void startAudio(AudioLayout *aLayout) = 0;
		virtual void bufferAudio(AudioBuffer *aBuffer) = 0;
		virtual void playAudio() = 0;
		virtual void endAudio() = 0;
	
		virtual void setDownloadURL(const char *aUrl) = 0;
		virtual void startDownload() = 0;
		virtual void seekDownload(long long aPos) = 0;
		virtual void continueDownload() = 0;
		virtual void endDownload() = 0;
	};

	///
	/// Platform-independent parts of the frontend player widget.
	/// Pair with a backend class to implement the drawing, audio,
	/// timing, and network code.
	///
	class Player {
	public:

		Player(PlayerBackend *backend);
		~Player();

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
		class impl; std::unique_ptr<impl> pimpl;
	};

}
