#include "app/Application.h"

#include "recording/BinarySessionFormat.h"

#include <ostream>

namespace ovtr {

int Application::runCliDiagnostics(std::ostream& output) const
{
    output << "OpenVR Tracker Recorder core initialized\n";
    output << "Binary session format version: " << kBinaryFormatVersion << '\n';
    output << "Desktop UI target: configure with OVTR_BUILD_DESKTOP_APP=ON\n";
    return 0;
}

} // namespace ovtr

