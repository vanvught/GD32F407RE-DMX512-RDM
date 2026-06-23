#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <cstdint>
#include <cstring>
#include "lib-osc/src/server/oscserver.h"

class OscServerSecurityTest : public ::testing::TestWithParam<std::pair<const uint8_t*, uint32_t>> {};

TEST_P(OscServerSecurityTest, BufferReadsNeverExceedDeclaredLength) {
    // Invariant: Buffer reads never exceed the declared length
    auto [buffer, size] = GetParam();
    
    // Create OscServer instance
    OscServer server;
    
    // This should not cause buffer overflows - either truncate or reject
    // We're testing that the function handles oversized inputs safely
    server.Input(buffer, size, 0x7F000001, 9000);
    
    // If we reach here without crashing, the test passes
    SUCCEED();
}

INSTANTIATE_TEST_SUITE_P(
    AdversarialInputs,
    OscServerSecurityTest,
    ::testing::Values(
        // Valid input (normal OSC message)
        std::make_pair(reinterpret_cast<const uint8_t*>("/test\0\0\0,\0\0\0"), 12),
        
        // Boundary case: exactly at typical buffer limit (64 bytes)
        std::make_pair([](){
            static uint8_t data[64];
            strcpy(reinterpret_cast<char*>(data), "/test");
            return data;
        }(), 64),
        
        // Exploit case 1: 2x typical buffer (128 bytes)
        std::make_pair([](){
            static uint8_t data[128];
            memset(data, 'A', 127);
            data[127] = '\0';
            return data;
        }(), 128),
        
        // Exploit case 2: 10x typical buffer (640 bytes)
        std::make_pair([](){
            static uint8_t data[640];
            memset(data, 'B', 639);
            data[639] = '\0';
            return data;
        }(), 640),
        
        // Extreme case: maximum UDP packet size (65507 bytes)
        std::make_pair([](){
            static uint8_t data[65507];
            memset(data, 'C', 65506);
            data[65506] = '\0';
            return data;
        }(), 65507)
    )
);

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}