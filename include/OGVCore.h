//
// Platform-independent Ogg Vorbis/Theora/Opus decoder/player base
//
// Copyright (c) 2013-2015 Brion Vibber <brion@pobox.com>
// MIT-style license
// Please reuse and redistribute with the LICENSE notes intact.
//

#include <memory>
#include <string>
#include <vector>

namespace OGVCore {

	struct Point {
		int x;
		int y;

		Point(int aX, int aY) :
			x(aX),
			y(aY)
		{}
	};
	
	struct Size {
		int width;
		int height;

		Size(int aWidth, int aHeight) :
			width(aWidth),
			height(aHeight)
		{}
	};
	
	struct FrameLayout {
		const Size frame;
		const Size picture;
		const Point offset;
		const Point subsampling;
		double aspectRatio;
		double fps;
		
		FrameLayout(Size aFrame, Size aPicture, Point aOffset, Point aSubsampling, double aAspectRatio, double aFps) :
			frame(aFrame),
			picture(aPicture),
			offset(aOffset),
			subsampling(aSubsampling),
			aspectRatio(aAspectRatio),
			fps(aFps)
		{}
	};

	//
	// Currently wraps a raw pointer and does NOT copy data.
	//
	struct PlaneBuffer {
		const unsigned char *bytes;
		const int stride;
		
		PlaneBuffer(const unsigned char *aBytes, int aStride) :
			bytes(aBytes),
			stride(aStride)
		{}
	};

	struct FrameBuffer {
		std::shared_ptr<FrameLayout> layout;
		const double timestamp;
		const double keyframeTimestamp;
		PlaneBuffer Y;
		PlaneBuffer Cb;
		PlaneBuffer Cr;

		FrameBuffer(std::shared_ptr<FrameLayout> aLayout,
		            double aTimestamp, double aKeyframeTimestamp,
		            PlaneBuffer aY, PlaneBuffer aCb, PlaneBuffer aCr) :
			layout(aLayout),
			timestamp(aTimestamp),
			keyframeTimestamp(aKeyframeTimestamp),
			Y(aY),
			Cb(aCb),
			Cr(aCr)
		{};
	};


	struct AudioLayout {
		const int channelCount;
		const int sampleRate;
		
		AudioLayout(int aChannelCount, int aSampleRate) :
			channelCount(aChannelCount),
			sampleRate(aSampleRate)
		{}
	};

	struct AudioBuffer {
		std::shared_ptr<AudioLayout> layout;
		const int sampleCount;
		std::vector<std::vector<float>> samples;
	
		// Convenience constructor for the C library output
		AudioBuffer(std::shared_ptr<AudioLayout> aLayout, int aSampleCount, const float **aSamples) :
			layout(aLayout),
			sampleCount(aSampleCount),
			samples(layout->channelCount)
		{
			int n = layout->channelCount;
			for (int i = 0; i < n; i++) {
				samples.push_back(std::vector<float>(aSamples[n], aSamples[n] + aSampleCount));
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
		std::shared_ptr<AudioLayout> getAudioLayout();
		std::shared_ptr<FrameLayout> getFrameLayout();

		void receiveInput(std::vector<unsigned char> aBuffer);
		bool process();

		bool decodeFrame();
		std::shared_ptr<FrameBuffer> dequeueFrame();
		void discardFrame();

		bool decodeAudio();
		std::shared_ptr<AudioBuffer> dequeueAudio();
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
		virtual void bufferData(std::shared_ptr<AudioBuffer> aBuffer) = 0;
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
			virtual void onRead(std::vector<unsigned char> data) = 0;
			virtual void onDone() = 0;
			virtual void onError(std::string err) = 0;
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
		virtual void drawFrame(std::shared_ptr<FrameBuffer> aFrame) = 0;
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
			virtual std::shared_ptr<Timer> timer() = 0;
			virtual std::shared_ptr<FrameSink> frameSink(FrameLayout aLayout) = 0;
			virtual std::shared_ptr<AudioFeeder> audioFeeder(AudioLayout aLayout, std::shared_ptr<AudioFeeder::Delegate> aDelegate) = 0;
			virtual std::shared_ptr<StreamFile> streamFile(std::string aURL, std::shared_ptr<StreamFile::Delegate> aDelegate) = 0;
		
			virtual void onLoadedMetadata() = 0;
			virtual void onPlay() = 0;
			virtual void onPause() = 0;
			virtual void onEnded() = 0;
		};


		Player(std::shared_ptr<Delegate> delegate);
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
