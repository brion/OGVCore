//
// Platform-independent Ogg Vorbis/Theora/Opus decoder/player base
//
// Copyright (c) 2013-2015 Brion Vibber <brion@pobox.com>
// MIT-style license
// Please reuse and redistribute with the LICENSE notes intact.
//

#include <string.h>
#include <memory>
#include <string>

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
	/// platform-specific audio behavior...
	///
	class AudioFeeder {
	public:
	
		class Delegate {
		public:
			virtual void onStarved() = 0;
		};

		virtual void start() = 0;
		virtual void stop() = 0;
		virtual void bufferData(AudioBuffer *aBuffer) = 0;
		virtual double getPlaybackPosition() = 0;
		virtual double getBufferedTime() = 0;
	};

	///
	/// Abstract class for JS, Cocoa, etc backends to implement
	/// platform-specific streaming download behavior...
	///
	class StreamFile {
	public:

		class Delegate {
		public:
			virtual void onStart() = 0;
			virtual void onBuffer() = 0;
			virtual void onRead() = 0;
			virtual void onDone() = 0;
			virtual void onError() = 0;
		};

		virtual void readBytes() = 0;
		virtual void abort() = 0;
		virtual void seek(long aBytePosition) = 0;

		virtual std::string getResponseHeader(std::string aHeaderName) = 0;
		virtual long bytesTotal() = 0;
		virtual long bytesBuffered() = 0;
		virtual long bytesRead() = 0;
		virtual bool isSeekable() = 0;
	};

	///
	/// Abstract class for JS, Cocoa, etc backends to implement
	/// platform-specific behavior...
	///
	class FrameSink {
	public:
		virtual void drawFrame(FrameBuffer *aFrame) = 0;
	};
	
	///
	/// Abstract class for JS, Cocoa, etc backends to implement
	/// platform-specific behavior...
	///
	class Timer {
	public:
		virtual double getTimestamp() = 0;
		virtual void setTimeout(double aDelay) = 0;
	};

	///
	/// Platform-independent parts of the frontend player widget.
	/// Pair with a backend class to implement the drawing, audio,
	/// timing, and network code.
	///
	class Player {
	public:
		class Delegate {
		public:
			virtual Timer *timer() = 0;
			virtual FrameSink *frameSink(FrameLayout *aLayout) = 0;
			virtual AudioFeeder *audioFeeder(AudioLayout *aLayout, AudioFeeder::Delegate *aDelegate) = 0;
			virtual StreamFile *streamFile(std::string aURL, StreamFile::Delegate *aDelegate) = 0;
		
			virtual void onLoadedMetadata() = 0;
			virtual void onPlay() = 0;
			virtual void onPause() = 0;
			virtual void onEnded() = 0;
		};


		Player(Delegate *delegate);
		~Player();

		void load();
		void process();
	
		double getDuration();
		double getVideoWidth();
		double getVideoHeight();
	
		std::string getSourceURL();
		void setSourceURL(std::string aUrl);
	
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
