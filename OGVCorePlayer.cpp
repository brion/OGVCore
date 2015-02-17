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

#include "OGVCore.hpp"

#pragma mark - Declarations

class OGVCorePlayer::impl {
public:

    impl(OGVCorePlayerBackend *backend);
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
    std::unique_ptr<OGVCorePlayerBackend> backend;
};

#pragma mark - OGVCorePlayer pimpl bounce methods

OGVCorePlayer::OGVCorePlayer(OGVCorePlayerBackend *aBackend): pimpl(new impl(aBackend))
{
}

OGVCorePlayer::~OGVCorePlayer()
{
}

void OGVCorePlayer::load()
{
    pimpl->load();
}

void OGVCorePlayer::process()
{
    pimpl->process();
}

double OGVCorePlayer::getDuration()
{
    return pimpl->getDuration();
}

double OGVCorePlayer::getVideoWidth()
{
    return pimpl->getVideoWidth();
}

double OGVCorePlayer::getVideoHeight()
{
    return pimpl->getVideoHeight();
}

const char *OGVCorePlayer::getSourceURL()
{
    return pimpl->getSourceURL();
}

void OGVCorePlayer::setSourceURL(const char *aUrl)
{
    pimpl->setSourceURL(aUrl);
}

double OGVCorePlayer::getCurrentTime()
{
    return pimpl->getCurrentTime();
}

void OGVCorePlayer::setCurrentTime(double aTime)
{
    pimpl->setCurrentTime(aTime);
}

bool OGVCorePlayer::getPaused()
{
    return pimpl->getPaused();
}

void OGVCorePlayer::setPaused(bool aPaused)
{
    return pimpl->setPaused(aPaused);
}

bool OGVCorePlayer::getPlaying()
{
    return pimpl->getPlaying();
}

bool OGVCorePlayer::getSeeking()
{
    return pimpl->getSeeking();
}

#pragma mark - impl methods

OGVCorePlayer::impl::impl(OGVCorePlayerBackend *aBackend): backend(aBackend)
{
}

OGVCorePlayer::impl::~impl()
{
}

void OGVCorePlayer::impl::load()
{
    // TODO
}

void OGVCorePlayer::impl::process()
{
    // TODO
}

double OGVCorePlayer::impl::getDuration()
{
    return NAN; // TODO
}

double OGVCorePlayer::impl::getVideoWidth()
{
    return NAN; // TODO
}

double OGVCorePlayer::impl::getVideoHeight()
{
    return NAN; // TODO
}

const char *OGVCorePlayer::impl::getSourceURL()
{
    return NULL;
}

void OGVCorePlayer::impl::setSourceURL(const char *aUrl)
{
    // TODO
}

double OGVCorePlayer::impl::getCurrentTime()
{
    return NAN; // TODO
}

void OGVCorePlayer::impl::setCurrentTime(double aTime)
{
    // TODO
}

bool OGVCorePlayer::impl::getPaused()
{
    return false; // TODO
}

void OGVCorePlayer::impl::setPaused(bool aPaused)
{
    // TODO
}

bool OGVCorePlayer::impl::getPlaying()
{
    return false; // TODO
}

bool OGVCorePlayer::impl::getSeeking()
{
    return false; // TODO
}
