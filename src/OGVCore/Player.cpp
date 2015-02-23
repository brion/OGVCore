//
// Platform-independent Ogg Vorbis/Theora/Opus decoder/player base
//
// Copyright (c) 2013-2015 Brion Vibber <brion@pobox.com>
// MIT-style license
// Please reuse and redistribute with the LICENSE notes intact.
//

// C++11
#include <string>
#include <iostream>
#include <cmath>

// good ol' C library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// And our own headers.
#include <OGVCore.h>
#include "Bisector.h"

namespace OGVCore {

#pragma mark - Declarations

    class Player::impl {
    public:

        impl(std::shared_ptr<Player::Delegate> aDelegate) :
            delegate(aDelegate),
            timer(delegate->timer())
        {}

        ~impl()
        {}

        void load()
        {
            if (stream.get() != NULL) {
                // already loaded.
                return;
            }

            started = false;
            stream = delegate->streamFile(getSourceURL(), std::shared_ptr<StreamFile::Delegate>(new StreamDelegate(this)));
        }

        void process()
        {
            // TODO
        }

        double getDuration()
        {
			if (codec && loadedMetadata) {
				if (!isnan(duration)) {
					return duration;
				} else {
					return INFINITY;
				}
			} else {
				return NAN;
			}
        }

        double getVideoWidth()
        {
            return NAN; // TODO
        }

        double getVideoHeight()
        {
            return NAN; // TODO
        }

        std::string getSourceURL()
        {
            return NULL; // TODO
        }

        void setSourceURL(std::string aUrl)
        {
        }

        double getCurrentTime()
        {
            return NAN; // TODO
        }
        
        void setCurrentTime(double aTime)
        {
            // TODO
        }

        bool getPaused()
        {
            return false; // TODO
        }
        void setPaused(bool aPaused)
        {
            // TODO
        }

        bool getPlaying()
        {
            return false; // TODO
        }
        
        bool getSeeking()
        {
            return false; // TODO
        }

    private:
        std::shared_ptr<Player::Delegate> delegate;
        std::shared_ptr<Timer> timer;
        std::shared_ptr<FrameSink> frameSink;

        std::shared_ptr<AudioFeeder> audioFeeder;
	    bool muted = false;
		double initialAudioPosition = 0.0;
		double initialAudioOffset = 0.0;

        class AudioDelegate : public AudioFeeder::Delegate {
            Player::impl *owner;
        public:
            AudioDelegate(Player::impl *aOwner) :
                owner(aOwner)
            {}

            void onStarved() {
                // If we're in a background tab, timers may be throttled.
                // When audio buffers run out, go decode some more stuff.
                owner->pingProcessing();
            }
        };

        void initAudioFeeder()
        {
            audioFeeder = delegate->audioFeeder(audioInfo, std::shared_ptr<AudioFeeder::Delegate>(new AudioDelegate(this)));
            if (muted) {
                audioFeeder->mute();
            }
        }
    
        void startAudio(double offset)
        {
            audioFeeder->start();
            initialAudioPosition = audioFeeder->getPlaybackPosition();
            if (offset >= 0) {
                initialAudioOffset = offset;
            }
        }
    
        void stopAudio() {
            initialAudioOffset = getAudioTime();
            audioFeeder->stop();
        }
    
        /**
         * Get audio playback time position in file's units
         *
         * @return {number} seconds since file start
         */
        double getAudioTime() {
            return (audioFeeder->getPlaybackPosition() - initialAudioPosition) + initialAudioOffset;
        }


        std::shared_ptr<StreamFile> stream;
        long byteLength;
        double duration;

        class StreamDelegate : public StreamFile::Delegate {
        private:
            Player::impl *owner;
        
        public:
            StreamDelegate(Player::impl *aOwner) :
                owner(aOwner)
            {}

            virtual void onStart()
            {
                // Fire off the read/decode/draw loop...
                owner->byteLength = owner->stream->bytesTotal();
    
                // If we get X-Content-Duration, that's as good as an explicit hint
                auto durationHeader = owner->stream->getResponseHeader("X-Content-Duration");
                if (durationHeader.length() > 0) {
                    owner->duration = std::atof(durationHeader.c_str());
                }
                owner->startProcessingVideo();
            }
            
            virtual void onBuffer()
            {}
            
            virtual void onRead(std::vector<unsigned char> data)
            {
                // Pass chunk into the codec's buffer
                owner->codec->receiveInput(data);

                // Continue the read/decode/draw loop...
                owner->pingProcessing();
            }
            
            virtual void onDone()
            {
                if (owner->state == STATE_SEEKING) {
                    owner->pingProcessing();
                } else if (owner->state == STATE_SEEKING_END) {
                    owner->pingProcessing();
                } else {
                    //throw new Error('wtf is this');
                    owner->stream.reset();
    
                    // Let the read/decode/draw loop know we're out!
                    owner->pingProcessing();
                }
            }
            
            virtual void onError(std::string err)
            {
                std::cout << "reading error: " << err;
            }
        };

        enum {
            STATE_INITIAL,
            STATE_SEEKING_END,
            STATE_LOADED,
            STATE_PLAYING,
            STATE_PAUSED,
            STATE_SEEKING,
            STATE_ENDED
        } state = STATE_INITIAL;

        std::unique_ptr<Decoder> codec;
        bool started = false;
		bool paused = true;
		bool ended = false;
		bool loadedMetadata = false;

        std::shared_ptr<FrameBuffer> yCbCrBuffer = NULL;
        double lastFrameTimestamp = 0.0;
        double frameEndTimestamp = 0.0;

        void processFrame()
        {
            yCbCrBuffer = codec->dequeueFrame();
            frameEndTimestamp = yCbCrBuffer->timestamp;
        }

        void drawFrame()
        {
            frameSink->drawFrame(yCbCrBuffer);
            doFrameComplete();
        }

        void doFrameComplete()
        {
            lastFrameTimestamp = timer->getTimestamp();
        }

        // Seeking
        enum {
            SEEKSTATE_NOT_SEEKING,
            SEEKSTATE_BISECT_TO_TARGET,
            SEEKSTATE_BISECT_TO_KEYPOINT,
            SEEKSTATE_LINEAR_TO_TARGET
        } seekState = SEEKSTATE_NOT_SEEKING;

        double seekTargetTime = 0.0;
        double seekTargetKeypoint = 0.0;
        double bisectTargetTime = 0.0;
        long lastSeekPosition = 0.0;
        bool lastFrameSkipped;
        std::unique_ptr<Bisector> seekBisector;

        void startBisection(double targetTime)
        {
            bisectTargetTime = targetTime;
            seekBisector.reset(new Bisector(
                /* start */ 0,
                /* end */ byteLength - 1,
                /* process */ [this] (int start, int end, int position) {
                    if (position == lastSeekPosition) {
                        return false;
                    } else {
                        lastSeekPosition = position;
                        lastFrameSkipped = false;
                        codec->flush();
                        stream->seek(position);
                        stream->readBytes();
                        return true;
                    }
                }
            ));
            seekBisector->start();
        }
        
        void seek(double toTime)
        {
            if (stream->bytesTotal() == 0) {
                std::cout << "Cannot bisect a non-seekable stream";
                return;
            }
            state = STATE_SEEKING;
            seekTargetTime = toTime;
            seekTargetKeypoint = -1;
            lastFrameSkipped = false;
            lastSeekPosition = -1;
            codec->flush();
        
            if (codec->hasAudio() && audioFeeder) {
                stopAudio();
            }
        
            long offset = codec->getKeypointOffset(toTime);
            if (offset > 0) {
                // This file has an index!
                //
                // Start at the keypoint, then decode forward to the desired time.
                //
                seekState = SEEKSTATE_LINEAR_TO_TARGET;
                stream->seek(offset);
                stream->readBytes();
            } else {
                // No index.
                //
                // Bisect through the file finding our target time, then we'll
                // have to do it again to reach the keypoint, and *then* we'll
                // have to decode forward back to the desired time.
                //
                seekState = SEEKSTATE_BISECT_TO_TARGET;
                startBisection(seekTargetTime);
            }
        }

        void continueSeekedPlayback();
        void doProcessLinearSeeking();
        void doProcessBisectionSeek();

        // Main stuff!
        void doProcessing()
        {
            // TODO
        }
        
        void pingProcessing(double delay = -1.0)
        {
            // TODO
        }
        
        std::shared_ptr<FrameLayout> videoInfo;
        std::shared_ptr<AudioLayout> audioInfo;

        void startProcessingVideo()
        {
            // TODO
        }
    };

#pragma mark - Player pimpl bounce methods

    Player::Player(std::shared_ptr<Player::Delegate> aDelegate):
        pimpl(new impl(aDelegate))
    {}

    Player::~Player()
    {}

    void Player::load()
    {
        pimpl->load();
    }

    void Player::process()
    {
        pimpl->process();
    }

    double Player::getDuration()
    {
        return pimpl->getDuration();
    }

    double Player::getVideoWidth()
    {
        return pimpl->getVideoWidth();
    }

    double Player::getVideoHeight()
    {
        return pimpl->getVideoHeight();
    }

    std::string Player::getSourceURL()
    {
        return pimpl->getSourceURL();
    }

    void Player::setSourceURL(std::string aUrl)
    {
        pimpl->setSourceURL(aUrl);
    }

    double Player::getCurrentTime()
    {
        return pimpl->getCurrentTime();
    }

    void Player::setCurrentTime(double aTime)
    {
        pimpl->setCurrentTime(aTime);
    }

    bool Player::getPaused()
    {
        return pimpl->getPaused();
    }

    void Player::setPaused(bool aPaused)
    {
        return pimpl->setPaused(aPaused);
    }

    bool Player::getPlaying()
    {
        return pimpl->getPlaying();
    }

    bool Player::getSeeking()
    {
        return pimpl->getSeeking();
    }

}
