#pragma once

#include "dumpDescriptor.h"

#include <string>

/**
 * @brief Open the file and create metadata.
 *
 * Detects the directional kind by substring-matching the filename. Does not parse the dump.
 * The returned descriptor is then used by @ref dispatchLoad to pick the right loader.
 *
 * @param fn Path to the dump file.
 * @return A populated descriptor with @c valid==true on success, else descriptor with @c valid==false .
 */
DumpDescriptor probeDump(const std::string& fn);
