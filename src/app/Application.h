#pragma once

#include <iosfwd>

namespace ovtr {

class Application {
public:
    int runCliDiagnostics(std::ostream& output) const;
};

} // namespace ovtr

