#include "TestCases.h"
#include "TestSupport.h"

#include "data/SessionTypes.h"
#include "vr/MockVRProvider.h"

namespace ovtr::test {
void testMockVRProvider()
{
    ovtr::MockVRProvider provider;
    require(provider.initialize(), "mock provider failed to initialize");
    require(provider.isInitialized(), "mock provider should be initialized");

    const ovtr::DeviceDescriptor tracker = makeTestTracker("LHR-TEST");
    provider.setDevices({tracker});

    const auto devices = provider.enumerateDevices();
    require(devices.size() == 1, "mock provider device count mismatch");
    require(devices[0].serial == "LHR-TEST", "mock provider serial mismatch");
}
} // namespace ovtr::test
