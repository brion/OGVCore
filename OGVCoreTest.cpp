//
// Platform-independent Ogg Vorbis/Theora/Opus decoder/player base
//
// Copyright (c) 2013-2015 Brion Vibber <brion@pobox.com>
// MIT-style license
// Please reuse and redistribute with the LICENSE notes intact.
//

#include <stdio.h>

#include "OGVCore.hpp"

int main() {
	auto decoder = new OGVCoreDecoder();
	printf("Hello! %p\n", decoder);
	delete decoder;

	return 0;
}
