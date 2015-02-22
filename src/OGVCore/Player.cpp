//
// Platform-independent Ogg Vorbis/Theora/Opus decoder/player base
//
// Copyright (c) 2013-2015 Brion Vibber <brion@pobox.com>
// MIT-style license
// Please reuse and redistribute with the LICENSE notes intact.
//

// good ol' C library

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

// And our own headers.

#include <OGVCore.h>

#include "Bisector.h"

namespace OGVCore {

#pragma mark - Declarations

    class Player::impl {
    public:

        impl(Delegate *delegate);
        ~impl();

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
        std::unique_ptr<Delegate> delegate;
        std::unique_ptr<Timer> timer;
        std::unique_ptr<FrameSink> frameSink;
        std::unique_ptr<StreamFile> stream;
        std::unique_ptr<AudioFeeder> audioFeeder;

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
        
        int bytesTotal;

        double lastFrameTimestamp = 0.0;
        double frameEndTimestamp = 0.0;
        FrameBuffer *yCbCrBuffer = NULL;

        void processFrame();
        void drawFrame();
        void doFrameComplete();

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
        Bisector *seekBisector;

        void startBisection(double targetTime);
        void seek(double toTime);
        void continueSeekedPlayback();
        void doProcessLinearSeeking();
        void doProcessBisectionSeek();

        // Main stuff!
        void doProcessing();
        void pingProcessing(double delay);
        void startProcessingVideo();
    };

#pragma mark - Player pimpl bounce methods

    Player::Player(Delegate *aDelegate): pimpl(new impl(aDelegate))
    {
    }

    Player::~Player()
    {
    }

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

    const char *Player::getSourceURL()
    {
        return pimpl->getSourceURL();
    }

    void Player::setSourceURL(const char *aUrl)
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

#pragma mark - impl methods

    Player::impl::impl(Delegate *aDelegate): delegate(aDelegate)
    {
    }

    Player::impl::~impl()
    {
    }

    void Player::impl::load()
    {
        // TODO
    }

    void Player::impl::process()
    {
        // TODO
    }

    double Player::impl::getDuration()
    {
        return NAN; // TODO
    }

    double Player::impl::getVideoWidth()
    {
        return NAN; // TODO
    }

    double Player::impl::getVideoHeight()
    {
        return NAN; // TODO
    }

    const char *Player::impl::getSourceURL()
    {
        return NULL;
    }

    void Player::impl::setSourceURL(const char *aUrl)
    {
        // TODO
    }

    double Player::impl::getCurrentTime()
    {
        return NAN; // TODO
    }

    void Player::impl::setCurrentTime(double aTime)
    {
        // TODO
    }

    bool Player::impl::getPaused()
    {
        return false; // TODO
    }

    void Player::impl::setPaused(bool aPaused)
    {
        // TODO
    }

    bool Player::impl::getPlaying()
    {
        return false; // TODO
    }

    bool Player::impl::getSeeking()
    {
        return false; // TODO
    }


    void Player::impl::processFrame()
    {
        yCbCrBuffer = codec->dequeueFrame();
        frameEndTimestamp = yCbCrBuffer->timestamp;
    }

    void Player::impl::drawFrame()
    {
        frameSink->drawFrame(yCbCrBuffer);
        doFrameComplete();
    }

    void Player::impl::doFrameComplete()
    {
        lastFrameTimestamp = timer->getTimestamp();
    }

    void Player::impl::startBisection(double targetTime)
    {
		bisectTargetTime = targetTime;
		seekBisector = new Bisector(
			/* start */ 0,
			/* end */ bytesTotal - 1,
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
		);
		seekBisector->start();
    }

    void Player::impl::seek(double toTime)
    {
    }

    void Player::impl::continueSeekedPlayback()
    {
    }

    void Player::impl::doProcessLinearSeeking()
    {
    }

    void Player::impl::doProcessBisectionSeek()
    {
    }

    void Player::impl::doProcessing()
    {
    }

    void Player::impl::pingProcessing(double delay)
    {
    }

    void Player::impl::startProcessingVideo()
    {
    }

}
