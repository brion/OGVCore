//
// Platform-independent Ogg Vorbis/Theora/Opus decoder/player base
//
// Copyright (c) 2013-2015 Brion Vibber <brion@pobox.com>
// MIT-style license
// Please reuse and redistribute with the LICENSE notes intact.
//

// C++ awesome
#include <vector>
#include <functional>
#include <cmath>

// good ol' C library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Various Xiph headers!
#include <ogg/ogg.h>
#include <vorbis/codec.h>
#include <theora/theoradec.h>

#ifdef OPUS
#include <opus/opus_multistream.h>
#include "opus_header.h"
#include "opus_helper.h"
#endif

#include <skeleton/skeleton.h>


// And our own headers.

#include <OGVCore.h>

namespace OGVCore {

#pragma mark - Declarations

    class Decoder::impl {
    public:
        impl();
        ~impl();

		void setOnLoadedMetadata(const std::function<void()> &aCallback);

        bool hasAudio() const;
        bool hasVideo() const;
        bool audioReady() const;
        bool frameReady() const;
        double audioTimestamp() const;
        double frameTimestamp() const;
        double keyframeTimestamp() const;
        std::shared_ptr<AudioLayout> getAudioLayout() const;
        std::shared_ptr<FrameLayout> getFrameLayout() const;

        void receiveInput(std::vector<unsigned char> aBuffer);
        bool process();

        bool decodeFrame(std::function<void(FrameBuffer &aBuffer)> aCallback);
        void discardFrame();

        bool decodeAudio(std::function<void(AudioBuffer &aBuffer)> aCallback);
        void discardAudio();

        void flushBuffers();

        long getSegmentLength() const;
        double getDuration() const;
        long getKeypointOffset(double aTime);

    private:
        std::function<void()> onLoadedMetadata;

        void video_write(std::function<void(FrameBuffer &aBuffer)> aCallback);
        int queue_page(ogg_page *page);

        void processBegin();
        void processHeaders();
        void processDecoding();


        /* Ogg and codec state for demux/decode */
        ogg_sync_state    oggSyncState {};
        ogg_page          oggPage {};
        ogg_packet        oggPacket {};
        ogg_packet        audioPacket {};
        ogg_packet        videoPacket {};

        /* Video decode state */
        ogg_stream_state  theoraStreamState {};
        th_info           theoraInfo {};
        th_comment        theoraComment {};
        th_setup_info    *theoraSetupInfo = nullptr;
        th_dec_ctx       *theoraDecoderContext = nullptr;

        int               theoraHeaders = 0;
        int               theoraProcessingHeaders;
        int               frames = 0;

        /* single frame video buffering */
        int               videobufReady = 0;
        ogg_int64_t       videobufGranulepos = -1;  // @todo reset with TH_CTL_whatver on seek
        double            videobufTime = -1;         // time seen on actual decoded frame
        ogg_int64_t       keyframeGranulepos = -1;  //
        double            keyframeTime = -1;        // last-keyframe time seen on actual decoded frame

        int               audiobufReady = 0;
        ogg_int64_t       audiobufGranulepos = -1; /* time position of last sample */
        double            audiobufTime = -1;

        /* Audio decode state */
        int               vorbisHeaders = 0;
        int               vorbisProcessingHeaders = 0;
        ogg_stream_state  vorbisStreamState {};
        vorbis_info       vorbisInfo {};
        vorbis_dsp_state  vorbisDspState {};
        vorbis_block      vorbisBlock {};
        vorbis_comment    vorbisComment {};

#ifdef OPUS
        int               opusHeaders = 0;
        ogg_stream_state  opusStreamState {};
        OpusMSDecoder    *opusDecoder = nullptr;
        int               opusMappingFamily = 0;
        int               opusChannels = 0;
        int               opusPreskip = 0;
        ogg_int64_t       opusPrevPacketGranpos = 0L;
        float             opusGain = 0.0f;
        int               opusStreams = 0;
        /* 120ms at 48000 */
#define OPUS_MAX_FRAME_SIZE (960*6)
#endif

        OggSkeleton      *skeleton = nullptr;
        ogg_stream_state  skeletonStreamState {};
        int               skeletonHeaders = 0;
        int               skeletonProcessingHeaders = 0;
        int               skeletonDone = 0;

        int               processAudio = 1;
        int               processVideo = 1;

        enum AppState {
            OGVCORE_STATE_BEGIN,
            OGVCORE_STATE_HEADERS,
            OGVCORE_STATE_DECODING
        } appState;

        int needData = 1;
        int buffersReceived = 0;

        bool isFrameReady = false;
        std::shared_ptr<FrameLayout> frameLayout = nullptr;
        std::shared_ptr<FrameBuffer> queuedFrame = nullptr;

        bool isAudioReady = false;
        std::shared_ptr<AudioLayout> audioLayout = nullptr;
        std::shared_ptr<AudioBuffer> queuedAudio = nullptr;
    };

#pragma mark - Decoder methods

    Decoder::Decoder():
        pimpl(new impl())
    {}

    Decoder::~Decoder()
    {}

#pragma mark - public method pimpl bouncers

	void Decoder::setOnLoadedMetadata(const std::function<void()> &aCallback)
	{
		pimpl->setOnLoadedMetadata(aCallback);
	}

    bool Decoder::hasAudio() const
    {
        return pimpl->hasAudio();
    }

    bool Decoder::hasVideo() const
    {
        return pimpl->hasVideo();
    }

    bool Decoder::audioReady() const
    {
        return pimpl->audioReady();
    }

    bool Decoder::frameReady() const
    {
        return pimpl->frameReady();
    }

    double Decoder::audioTimestamp() const
    {
        return pimpl->audioTimestamp();
    }
    
    double Decoder::frameTimestamp() const
    {
        return pimpl->frameTimestamp();
    }
    
    double Decoder::keyframeTimestamp() const
    {
        return pimpl->keyframeTimestamp();
    }

    std::shared_ptr<AudioLayout> Decoder::getAudioLayout() const
    {
        return pimpl->getAudioLayout();
    }

    std::shared_ptr<FrameLayout> Decoder::getFrameLayout() const
    {
        return pimpl->getFrameLayout();
    }

    void Decoder::receiveInput(std::vector<unsigned char> aBuffer)
    {
        pimpl->receiveInput(aBuffer);
    }

    bool Decoder::process()
    {
        return pimpl->process();
    }

    bool Decoder::decodeFrame(std::function<void(FrameBuffer &aBuffer)> aCallback)
    {
        return pimpl->decodeFrame(aCallback);
    }

    void Decoder::discardFrame()
    {
        return pimpl->discardFrame();
    }

    bool Decoder::decodeAudio(std::function<void(AudioBuffer &aBuffer)> aCallback)
    {
        return pimpl->decodeAudio(aCallback);
    }

    void Decoder::discardAudio()
    {
        pimpl->discardAudio();
    }

    void Decoder::flush()
    {
        pimpl->flushBuffers();
        pimpl->discardFrame();
        pimpl->discardAudio();
    }

    long Decoder::getSegmentLength() const
    {
        return pimpl->getSegmentLength();
    }

    double Decoder::getDuration() const
    {
        return pimpl->getDuration();
    }

    long Decoder::getKeypointOffset(double aTime)
    {
        return pimpl->getKeypointOffset(aTime);
    }

#pragma mark - implementation methods

    Decoder::impl::impl() :
		onLoadedMetadata([](){})
    {
        appState = OGVCORE_STATE_BEGIN;

        /* start up Ogg stream synchronization layer */
        ogg_sync_init(&oggSyncState);

        /* init supporting Vorbis structures needed in header parsing */
        vorbis_info_init(&vorbisInfo);
        vorbis_comment_init(&vorbisComment);

        /* init supporting Theora structures needed in header parsing */
        th_comment_init(&theoraComment);
        th_info_init(&theoraInfo);

        skeleton = oggskel_new();
    }

    Decoder::impl::~impl()
    {
        if (theoraHeaders) {
            ogg_stream_clear(&theoraStreamState);
            th_decode_free(theoraDecoderContext);
            th_comment_clear(&theoraComment);
            th_info_clear(&theoraInfo);
        }

        if (vorbisHeaders) {
            ogg_stream_clear(&vorbisStreamState);
            vorbis_info_clear(&vorbisInfo);
            vorbis_dsp_clear(&vorbisDspState);
            vorbis_block_clear(&vorbisBlock);
            vorbis_comment_clear(&vorbisComment);
        }

#ifdef OPUS
        if (opusHeaders) {
            opus_multistream_decoder_destroy(opusDecoder);
        }
#endif

        if (skeletonHeaders) {
            ogg_stream_clear(&skeletonStreamState);
            oggskel_destroy(skeleton);
        }

        ogg_sync_clear(&oggSyncState);
    }

	void Decoder::impl::setOnLoadedMetadata(const std::function<void()> &aCallback)
	{
		onLoadedMetadata = aCallback;
	}

    bool Decoder::impl::hasAudio() const
    {
        return (audioLayout.get() != NULL);
    }

    bool Decoder::impl::hasVideo() const
    {
        return (frameLayout.get() != NULL);
    }

    bool Decoder::impl::audioReady() const
    {
        return isAudioReady;
    }

    bool Decoder::impl::frameReady() const
    {
        return isFrameReady;
    }

    double Decoder::impl::audioTimestamp() const
    {
        return audiobufTime;
    }
    
    double Decoder::impl::frameTimestamp() const
    {
        return videobufTime;
    }
    
    double Decoder::impl::keyframeTimestamp() const
    {
        return keyframeTime;
    }

    std::shared_ptr<AudioLayout> Decoder::impl::getAudioLayout() const
    {
        return audioLayout;
    }

    std::shared_ptr<FrameLayout> Decoder::impl::getFrameLayout() const
    {
        return frameLayout;
    }

    void Decoder::impl::video_write(std::function<void(FrameBuffer &aBuffer)> aCallback) {
        th_ycbcr_buffer ycbcr;
        th_decode_ycbcr_out(theoraDecoderContext, ycbcr);

        int hdec = !(theoraInfo.pixel_fmt & 1);
        int vdec = !(theoraInfo.pixel_fmt & 2);

        assert(queuedFrame.get() == NULL);
        queuedFrame.reset(new FrameBuffer(*frameLayout,
                                          videobufTime, keyframeTime,
                                          PlaneBuffer(ycbcr[0].data, ycbcr[0].stride, frameLayout->frame.height),
                                          PlaneBuffer(ycbcr[1].data, ycbcr[1].stride, frameLayout->frame.height >> frameLayout->subsampling.y),
                                          PlaneBuffer(ycbcr[2].data, ycbcr[2].stride, frameLayout->frame.height >> frameLayout->subsampling.y)));
        aCallback(*queuedFrame);
        queuedFrame.reset();
    }

    /* helper: push a page into the appropriate steam */
    /* this can be done blindly; a stream won't accept a page
                    that doesn't belong to it */
    int Decoder::impl::queue_page(ogg_page *page) {
        if (theoraHeaders) ogg_stream_pagein(&theoraStreamState, page);
        if (vorbisHeaders) ogg_stream_pagein(&vorbisStreamState, page);
#ifdef OPUS
        if (opusHeaders) ogg_stream_pagein(&opusStreamState, page);
#endif
        if (skeletonHeaders) ogg_stream_pagein(&skeletonStreamState, page);
        return 0;
    }

    void Decoder::impl::receiveInput(std::vector<unsigned char> aBuffer)
    {
        int bufsize = aBuffer.size();
        if (bufsize > 0) {
            const unsigned char *buffer = aBuffer.data();
            buffersReceived = 1;
            if (appState == OGVCORE_STATE_DECODING) {
                // queue ALL the pages!
                while (ogg_sync_pageout(&oggSyncState, &oggPage) > 0) {
                    queue_page(&oggPage);
                }
            }
            char *dest = ogg_sync_buffer(&oggSyncState, bufsize);
            memcpy(dest, buffer, bufsize);
            if (ogg_sync_wrote(&oggSyncState, bufsize) < 0) {
                printf("Horrible error in ogg_sync_wrote\n");
            }
        }
    }

    bool Decoder::impl::process()
    {
        if (!buffersReceived) {
            return 0;
        }
        if (needData) {
            int ret = ogg_sync_pageout(&oggSyncState, &oggPage);
            if (ret > 0) {
                // complete page retrieved
                queue_page(&oggPage);
            } else if (ret < 0) {
                // incomplete sync
                // continue on the next loop
                return 1;
            } else {
                // need moar data
                return 0;
            }
        }
        if (appState == OGVCORE_STATE_BEGIN) {
            processBegin();
        } else if (appState == OGVCORE_STATE_HEADERS) {
            processHeaders();
        } else if (appState == OGVCORE_STATE_DECODING) {
            processDecoding();
        } else {
            // uhhh...
            printf("Invalid appState in OgvJsProcess\n");
        }
        return 1;
    }

    void Decoder::impl::processBegin() {
        if (ogg_page_bos(&oggPage)) {
            printf("Packet is at start of a bitstream\n");
            int got_packet;

            // Initialize a stream state object...
            ogg_stream_state test;
            ogg_stream_init(&test, ogg_page_serialno(&oggPage));
            ogg_stream_pagein(&test, &oggPage);

            // Peek at the next packet, since th_decode_headerin() will otherwise
            // eat the first Theora video packet...
            got_packet = ogg_stream_packetpeek(&test, &oggPacket);
            if (!got_packet) {
                return;
            }

            /* identify the codec: try theora */
            if (processVideo && !theoraHeaders && (theoraProcessingHeaders = th_decode_headerin(&theoraInfo, &theoraComment, &theoraSetupInfo, &oggPacket)) >= 0) {
                /* it is theora -- save this stream state */
                printf("found theora stream!\n");
                memcpy(&theoraStreamState, &test, sizeof (test));
                theoraHeaders = 1;

                if (theoraProcessingHeaders == 0) {
                    printf("Saving first video packet for later!\n");
                } else {
                    ogg_stream_packetout(&theoraStreamState, NULL);
                }
            } else if (processAudio && !vorbisHeaders && (vorbisProcessingHeaders = vorbis_synthesis_headerin(&vorbisInfo, &vorbisComment, &oggPacket)) == 0) {
                // it's vorbis! save this as our audio stream...
                printf("found vorbis stream! %d\n", vorbisProcessingHeaders);
                memcpy(&vorbisStreamState, &test, sizeof (test));
                vorbisHeaders = 1;

                // ditch the processed packet...
                ogg_stream_packetout(&vorbisStreamState, NULL);
#ifdef OPUS
            } else if (processAudio && !opusHeaders && (opusDecoder = opus_process_header(&oggPacket, &opusMappingFamily, &opusChannels, &opusPreskip, &opusGain, &opusStreams)) != NULL) {
                printf("found Opus stream! (first of two headers)\n");
                memcpy(&opusStreamState, &test, sizeof (test));
                if (opusGain) {
                    opus_multistream_decoder_ctl(opusDecoder, OPUS_SET_GAIN(opusGain));
                }
                opusPrevPacketGranpos = 0;
                opusHeaders = 1;

                // ditch the processed packet...
                ogg_stream_packetout(&opusStreamState, NULL);
#endif
            } else if (!skeletonHeaders && (skeletonProcessingHeaders = oggskel_decode_header(skeleton, &oggPacket)) >= 0) {
                memcpy(&skeletonStreamState, &test, sizeof (test));
                skeletonHeaders = 1;
                skeletonDone = 0;

                // ditch the processed packet...
                ogg_stream_packetout(&skeletonStreamState, NULL);
            } else {
                printf("already have stream, or not theora or vorbis or opus packet\n");
                /* whatever it is, we don't care about it */
                ogg_stream_clear(&test);
            }
        } else {
            printf("Moving on to header decoding...\n");
            // Not a bitstream start -- move on to header decoding...
            appState = OGVCORE_STATE_HEADERS;
            //processHeaders();
        }
    }

    void Decoder::impl::processHeaders()
    {

        if ((theoraHeaders && theoraProcessingHeaders)
            || (vorbisHeaders && vorbisHeaders < 3)
#ifdef OPUS
            || (opusHeaders && opusHeaders < 2)
#endif
            || (skeletonHeaders && !skeletonDone)
        ) {
            printf("processHeaders pass... %d %d %d\n", theoraHeaders, theoraProcessingHeaders, vorbisHeaders);
            int ret;

            // Rest of the skeleton comes before everything else, so process it up!
            if (skeletonHeaders && !skeletonDone) {
                ret = ogg_stream_packetout(&skeletonStreamState, &oggPacket);
                if (ret < 0) {
                    printf("Error reading skeleton headers: %d.\n", ret);
                    exit(1);
                }
                if (ret > 0) {
                    printf("Checking another skeleton header packet...\n");
                    skeletonProcessingHeaders = oggskel_decode_header(skeleton, &oggPacket);
                    if (skeletonProcessingHeaders < 0) {
                        printf("Error processing skeleton header packet: %d\n", skeletonProcessingHeaders);
                    }
                    if (oggPacket.e_o_s) {
                        printf("Found the skeleton end of stream!\n");
                        skeletonDone = 1;
                    }
                }
                if (ret == 0) {
                    printf("No skeleton header packet...\n");
                }
            }

            /* look for further theora headers */
            if (theoraHeaders && theoraProcessingHeaders) {
                printf("checking theora headers...\n");

                ret = ogg_stream_packetpeek(&theoraStreamState, &oggPacket);
                if (ret < 0) {
                    printf("Error reading theora headers: %d.\n", ret);
                    exit(1);
                }
                if (ret > 0) {
                    printf("Checking another theora header packet...\n");
                    theoraProcessingHeaders = th_decode_headerin(&theoraInfo, &theoraComment, &theoraSetupInfo, &oggPacket);
                    if (theoraProcessingHeaders == 0) {
                        // We've completed the theora header
                        printf("Completed theora header. Saving video packet for later...\n");
                        theoraHeaders = 3;
                    } else if (theoraProcessingHeaders < 0) {
                        printf("Error parsing theora headers: %d.\n", theoraProcessingHeaders);
                    } else {
                        printf("Still parsing theora headers...\n");
                        ogg_stream_packetout(&theoraStreamState, NULL);
                    }
                }
                if (ret == 0) {
                    printf("No theora header packet...\n");
                }
            }

            if (vorbisHeaders && (vorbisHeaders < 3)) {
                printf("checking vorbis headers...\n");

                ret = ogg_stream_packetpeek(&vorbisStreamState, &oggPacket);
                if (ret < 0) {
                    printf("Error reading vorbis headers: %d.\n", ret);
                    exit(1);
                }
                if (ret > 0) {
                    printf("Checking another vorbis header packet...\n");
                    vorbisProcessingHeaders = vorbis_synthesis_headerin(&vorbisInfo, &vorbisComment, &oggPacket);
                    if (vorbisProcessingHeaders == 0) {
                        printf("Completed another vorbis header (of 3 total)...\n");
                        vorbisHeaders++;
                    } else {
                        printf("Invalid vorbis header?\n");
                        exit(1);
                    }
                    ogg_stream_packetout(&vorbisStreamState, NULL);
                }
                if (ret == 0) {
                    printf("No vorbis header packet...\n");
                }
            }
#ifdef OPUS
            if (opusHeaders && (opusHeaders < 2)) {
                printf("checking for opus headers...\n");

                ret = ogg_stream_packetpeek(&opusStreamState, &oggPacket);
                if (ret < 0) {
                    printf("Error reading Opus headers: %d.\n", ret);
                    exit(1);
                }
                // FIXME: perhaps actually *check* if this is a comment packet ;-)
                opusHeaders++;
                printf("discarding Opus comments...\n");
                ogg_stream_packetout(&opusStreamState, NULL);
            }
#endif

        } else {
            /* and now we have it all.  initialize decoders */
#ifdef OPUS
            printf("theoraHeaders is %d; vorbisHeaders is %d, opusHeaders is %d\n", theoraHeaders, vorbisHeaders, opusHeaders);
#else
            printf("theoraHeaders is %d; vorbisHeaders is %d\n", theoraHeaders, vorbisHeaders);
#endif
            if (theoraHeaders) {
                theoraDecoderContext = th_decode_alloc(&theoraInfo, theoraSetupInfo);

               frameLayout.reset(new FrameLayout(
                    Size(theoraInfo.frame_width, theoraInfo.frame_height),
                    Size(theoraInfo.pic_width, theoraInfo.pic_height),
                    Point(theoraInfo.pic_x, theoraInfo.pic_y),
                    Point(!(theoraInfo.pixel_fmt & 1), !(theoraInfo.pixel_fmt & 2)),
                    (double) theoraInfo.aspect_numerator / theoraInfo.aspect_denominator,
                    (double) theoraInfo.fps_numerator / theoraInfo.fps_denominator));
            }

#ifdef OPUS
            // If we have both Vorbis and Opus, prefer Opus
            if (opusHeaders) {
                // opusDecoder should already be initialized
                // Opus has a fixed internal sampling rate of 48000 Hz
                audioLayout.reset(new AudioLayout(opusChannels, 48000));
            } else
#endif
            if (vorbisHeaders) {
                vorbis_synthesis_init(&vorbisDspState, &vorbisInfo);
                vorbis_block_init(&vorbisDspState, &vorbisBlock);
                printf("Ogg logical stream %lx is Vorbis %d channel %ld Hz audio.\n",
                        vorbisStreamState.serialno, vorbisInfo.channels, vorbisInfo.rate);

                audioLayout.reset(new AudioLayout(vorbisInfo.channels, vorbisInfo.rate));
            }

            appState = OGVCORE_STATE_DECODING;
            printf("Done with headers step\n");
            onLoadedMetadata();
        }
    }

    void Decoder::impl::processDecoding()
    {
        needData = 0;
        if (theoraHeaders && !videobufReady) {
            /* theora is one in, one out... */
            if (ogg_stream_packetpeek(&theoraStreamState, &videoPacket) > 0) {
                videobufReady = 1;

                if (videoPacket.granulepos < 0) {
                    // granulepos is actually listed per-page, not per-packet,
                    // so not every packet lists a granulepos.
                    // Scary, huh?
                    if (videobufGranulepos < 0) {
                        // don't know our position yet
                    } else {
                        videobufGranulepos++;
                    }
                } else {
                    videobufGranulepos = videoPacket.granulepos;
                    th_decode_ctl(theoraDecoderContext, TH_DECCTL_SET_GRANPOS, &videobufGranulepos, sizeof(videobufGranulepos));
                }

                double videoPacketTime = -1;
                double packetKeyframeTime = -1;
                if (videobufGranulepos < 0) {
                    // Extract the previous-keyframe info from the granule pos. It might be handy.
                    keyframeGranulepos = (videobufGranulepos >> theoraInfo.keyframe_granule_shift) << theoraInfo.keyframe_granule_shift;

                    // Convert to precious, precious seconds. Yay linear units!
                    videobufTime = th_granule_time(theoraDecoderContext, videobufGranulepos);
                    keyframeTime = th_granule_time(theoraDecoderContext, keyframeGranulepos);

                    // Also, if we've just resynced a stream we need to feed this down to the decoder
                }
                //printf("packet granulepos: %llx; time %lf; offset %d\n",(unsigned long long)videoPacket.granulepos, (double)videoPacketTime, (int)theoraInfo.keyframe_granule_shift);

                //OgvJsOutputFrameReady(videobufTime, keyframeTime);
                isFrameReady = 1;
            } else {
                needData = 1;
            }
        }

        if (!audiobufReady) {
#ifdef OPUS
            if (opusHeaders) {
                if (ogg_stream_packetpeek(&opusStreamState, &audioPacket) > 0) {
                    audiobufReady = 1;
                    if (audioPacket.granulepos == -1) {
                        // we can't update the granulepos yet
                    } else {
                        audiobufGranulepos = audioPacket.granulepos;
                        audiobufTime = (double)audiobufGranulepos / audioLayout->sampleRate;
                    }
                    //OgvJsOutputAudioReady(audiobufTime);
                    isAudioReady = 1;
                } else {
                    needData = 1;
                }
            } else
#endif
            if (vorbisHeaders) {
                if (ogg_stream_packetpeek(&vorbisStreamState, &audioPacket) > 0) {
                    audiobufReady = 1;
                    if (audioPacket.granulepos == -1) {
                        // we can't update the granulepos yet
                    } else {
                        audiobufGranulepos = audioPacket.granulepos;
                        audiobufTime = vorbis_granule_time(&vorbisDspState, audiobufGranulepos);
                    }
                    //OgvJsOutputAudioReady(audiobufTime);
                    isAudioReady = 1;
                } else {
                    needData = 1;
                }
            }
        }
    }

    bool Decoder::impl::decodeFrame(std::function<void(FrameBuffer &aBuffer)> aCallback)
    {
        if (ogg_stream_packetout(&theoraStreamState, &videoPacket) <= 0) {
            printf("Theora packet didn't come out of stream\n");
            return 0;
        }
        videobufReady = 0;
        int ret = th_decode_packetin(theoraDecoderContext, &videoPacket, NULL);
        if (ret == 0) {
            double t = th_granule_time(theoraDecoderContext, videobufGranulepos);
            if (t > 0) {
                videobufTime = t;
            } else {
                // For some reason sometimes we get a bunch of 0s out of th_granule_time
                videobufTime += 1.0 / ((double) theoraInfo.fps_numerator / theoraInfo.fps_denominator);
            }

            //printf("granulepos: %llx; time %lf; offset %d\n",(unsigned long long)videobufGranulepos, (double)videobufTime, (int)theoraInfo.keyframe_granule_shift);

            frames++;
            video_write(aCallback);
            return 1;
        } else if (ret == TH_DUPFRAME) {
            // Duplicated frame, advance time
            videobufTime += 1.0 / ((double) theoraInfo.fps_numerator / theoraInfo.fps_denominator);
            //printf("dupe videobuf time %lf\n", (double)videobufTime);
            frames++;
            video_write(aCallback);
            return 1;
        } else {
            printf("Theora decoder failed mysteriously? %d\n", ret);
            return 0;
        }
    }

    void Decoder::impl::discardFrame()
    {
        if (videobufReady) {
            if (theoraHeaders) {
                ogg_stream_packetout(&theoraStreamState, &videoPacket);
            }
            videobufReady = 0;
        }
    }

    bool Decoder::impl::decodeAudio(std::function<void(AudioBuffer &aBuffer)> aCallback)
    {
        int packetRet = 0;
        audiobufReady = 0;
        int foundSome = 0;

#ifdef OPUS
        if (opusHeaders) {
            if (ogg_stream_packetout(&opusStreamState, &audioPacket) > 0) {
                float *output = (float *)malloc(sizeof (float)*OPUS_MAX_FRAME_SIZE * opusChannels);
                int sampleCount = opus_multistream_decode_float(opusDecoder, (unsigned char*) audioPacket.packet, audioPacket.bytes, output, OPUS_MAX_FRAME_SIZE, 0);
                if (sampleCount < 0) {
                    printf("Opus decoding error, code %d\n", sampleCount);
                } else {
                    int skip = opusPreskip;
                    if (audioPacket.granulepos != -1) {
                        if (audioPacket.granulepos <= opusPrevPacketGranpos) {
                            sampleCount = 0;
                        } else {
                            ogg_int64_t endSample = opusPrevPacketGranpos + sampleCount;
                            if (audioPacket.granulepos < endSample) {
                                sampleCount = (int) (endSample - audioPacket.granulepos);
                            }
                        }
                        opusPrevPacketGranpos = audioPacket.granulepos;
                    } else {
                        opusPrevPacketGranpos += sampleCount;
                    }
                    if (skip >= sampleCount) {
                        skip = sampleCount;
                    } else {
                        foundSome = 1;
                        // reorder Opus' interleaved samples into two-dimensional [channel][sample] form
                        float *pcm = (float *)malloc(sizeof (*pcm)*(sampleCount - skip) * opusChannels);
                        float **pcmp = (float **)malloc(sizeof (*pcmp) * opusChannels);
                        for (int c = 0; c < opusChannels; ++c) {
                            pcmp[c] = pcm + c * (sampleCount - skip);
                            for (int s = skip; s < sampleCount; ++s) {
                                pcmp[c][s - skip] = output[s * opusChannels + c];
                            }
                        }
                        if (audiobufGranulepos != -1) {
                            // keep track of how much time we've decodec
                            audiobufGranulepos += (sampleCount - skip);
                            audiobufTime = (double)audiobufGranulepos / audioLayout->sampleRate;
                        }
                        assert(queuedAudio.get() == NULL);
                        queuedAudio.reset(new AudioBuffer(*audioLayout, sampleCount, (const float **)pcmp));

                        free(pcmp);
                        free(pcm);
                    }
                    opusPreskip -= skip;
                }
                free(output);
            }
        } else
#endif
        if (vorbisHeaders) {
            if (ogg_stream_packetout(&vorbisStreamState, &audioPacket) > 0) {
                int ret = vorbis_synthesis(&vorbisBlock, &audioPacket);
                if (ret == 0) {
                    foundSome = 1;
                    vorbis_synthesis_blockin(&vorbisDspState, &vorbisBlock);

                    float **pcm;
                    int sampleCount = vorbis_synthesis_pcmout(&vorbisDspState, &pcm);
                    if (audiobufGranulepos != -1) {
                        // keep track of how much time we've decodec
                        audiobufGranulepos += sampleCount;
                        audiobufTime = vorbis_granule_time(&vorbisDspState, audiobufGranulepos);
                    }
                    //OgvJsOutputAudio(pcm, vorbisInfo.channels, sampleCount);

                    assert(queuedAudio.get() == NULL);
                    queuedAudio.reset(new AudioBuffer(*audioLayout, sampleCount, (const float **)pcm));

                    vorbis_synthesis_read(&vorbisDspState, sampleCount);
                } else {
                    printf("Vorbis decoder failed mysteriously? %d", ret);
                }
            }
        }
        
        if (foundSome) {
            aCallback(*queuedAudio);
            queuedAudio.reset();
        }

        return foundSome;
    }

    void Decoder::impl::discardAudio()
    {
        if (audiobufReady) {
            if (vorbisHeaders) {
                ogg_stream_packetout(&vorbisStreamState, &audioPacket);
            }
#ifdef OPUS
            if (opusHeaders) {
                ogg_stream_packetout(&opusStreamState, &audioPacket);
            }
#endif
            audiobufReady = 0;
        }
    }

    void Decoder::impl::flushBuffers()
    {
        // First, read out anything left in our input buffer
        while (ogg_sync_pageout(&oggSyncState, &oggPage) > 0) {
            queue_page(&oggPage);
        }

        // Then, dump all packets from the streams
        if (theoraHeaders) {
            while (ogg_stream_packetout(&theoraStreamState, &videoPacket)) {
                // flush!
            }
        }

        if (vorbisHeaders) {
            while (ogg_stream_packetout(&vorbisStreamState, &audioPacket)) {
                // flush!
            }
        }

#ifdef OPUS
        if (opusHeaders) {
            while (ogg_stream_packetout(&opusStreamState, &audioPacket)) {
                // flush!
            }
        }
#endif

        // And reset sync state for good measure.
        ogg_sync_reset(&oggSyncState);
        videobufReady = 0;
        audiobufReady = 0;
        videobufGranulepos = -1;
        videobufTime = -1;
        keyframeGranulepos = -1;
        keyframeTime = -1;
        audiobufGranulepos = -1;
        audiobufTime = -1;

        needData = 1;
    }

    long Decoder::impl::getSegmentLength() const
    {
        ogg_int64_t segment_len = -1;
        if (skeletonHeaders) {
            oggskel_get_segment_len(skeleton, &segment_len);
        }
        return (long)segment_len;
    }

    double Decoder::impl::getDuration() const
    {
        if (skeletonHeaders) {
            ogg_uint16_t ver_maj = -1, ver_min = -1;
            oggskel_get_ver_maj(skeleton, &ver_maj);
            oggskel_get_ver_min(skeleton, &ver_min);
            printf("ver_maj %d\n", ver_maj);
            printf("ver_min %d\n", ver_min);

            ogg_int32_t serial_nos[4];
            size_t nstreams = 0;
            if (theoraHeaders) {
                serial_nos[nstreams++] = theoraStreamState.serialno;
            }
#ifdef OPUS
            if (opusHeaders) {
                serial_nos[nstreams++] = opusStreamState.serialno;
            }
#endif
            if (vorbisHeaders) {
                serial_nos[nstreams++] = vorbisStreamState.serialno;
            }

            double firstSample = -1,
                   lastSample = -1;
            for (int i = 0; i < nstreams; i++) {
                ogg_int64_t first_sample_num = -1,
                            first_sample_denum = -1,
                            last_sample_num = -1,
                            last_sample_denum = -1;
                int ret;
                ret = oggskel_get_first_sample_num(skeleton, serial_nos[i], &first_sample_num);
                printf("%d\n", ret);
                ret = oggskel_get_first_sample_denum(skeleton, serial_nos[i], &first_sample_denum);
                printf("%d\n", ret);
                ret = oggskel_get_last_sample_num(skeleton, serial_nos[i], &last_sample_num);
                printf("%d\n", ret);
                ret = oggskel_get_last_sample_denum(skeleton, serial_nos[i], &last_sample_denum);
                printf("%d\n", ret);
                printf("%lld %lld %lld %lld\n", first_sample_num, first_sample_denum, last_sample_num, last_sample_denum);

                double firstStreamSample = (double)first_sample_num / (double)first_sample_denum;
                if (firstSample == -1 || firstStreamSample < firstSample) {
                    firstSample = firstStreamSample;
                }

                double lastStreamSample = (double)last_sample_num / (double)last_sample_denum;
                if (lastSample == -1 || lastStreamSample > lastSample) {
                    lastSample = lastStreamSample;
                }
            }

            return lastSample - firstSample;
        }
        return -1;
    }

    long Decoder::impl::getKeypointOffset(double aTime)
    {
        long time_ms = (long)(aTime * 1000.0);
        ogg_int64_t offset = -1;
        if (skeletonHeaders) {
            ogg_int32_t serial_nos[4];
            size_t nstreams = 0;
            if (theoraHeaders) {
                serial_nos[nstreams++] = theoraStreamState.serialno;
            } else {
#ifdef OPUS
                if (opusHeaders) {
                    serial_nos[nstreams++] = opusStreamState.serialno;
                }
#endif
                if (vorbisHeaders) {
                    serial_nos[nstreams++] = vorbisStreamState.serialno;
                }
            }
            oggskel_get_keypoint_offset(skeleton, serial_nos, nstreams, time_ms, &offset);
        }
        return (long)offset;
    }
}
