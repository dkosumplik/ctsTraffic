/*

Copyright (c) Microsoft Corporation
All rights reserved.

Licensed under the Apache License, Version 2.0 (the ""License""); you may not use this file except in compliance with the License. You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

THIS CODE IS PROVIDED ON AN  *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.

See the Apache Version 2.0 License for specific language governing permissions and limitations under the License.

*/

#include <SDKDDKVer.h>
#include "CppUnitTest.h"

#include <Windows.h>

#include <ctLocks.hpp>
#include <ctVersionConversion.hpp>

#include "ctsConfig.h"
#include "ctsSocket.h"
#include "ctsSocketState.h"
#include "ctsSocketBroker.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std;

///
/// Fakes
///
namespace ctsTraffic {
    shared_ptr<ctsIOPattern> ctsIOPattern::MakeIOPattern()
    {
        Logger::WriteMessage(L"ctsIOPattern::MakeIOPattern\n");
        return nullptr;
    }

    namespace ctsConfig {
        ctsConfigSettings* Settings;

        void PrintDebug(_In_ LPCWSTR _text, ...) NOEXCEPT
        {
            va_list args;
            va_start(args, _text);

            auto formatted(ctl::ctString::format_string_va(_text, args));
            Logger::WriteMessage(ctl::ctString::format_string(L"ctsConfig::PrintDebug: %s\n", formatted.c_str()).c_str());

            va_end(args);
        }
        void PrintConnectionResults(const ctl::ctSockaddr& _local_addr, const ctl::ctSockaddr& _remote_addr, unsigned long _error) NOEXCEPT
        {
            Logger::WriteMessage(L"ctsConfig::PrintConnectionResults(error)\n");
        }
        void PrintConnectionResults(const ctl::ctSockaddr& _local_addr, const ctl::ctSockaddr& _remote_addr, unsigned long _error, const ctsTcpStatistics& _stats) NOEXCEPT
        {
            Logger::WriteMessage(L"ctsConfig::PrintConnectionResults(ctsTcpStatistics)\n");
        }
        void PrintConnectionResults(const ctl::ctSockaddr& _local_addr, const ctl::ctSockaddr& _remote_addr, unsigned long _error, const ctsUdpStatistics& _stats) NOEXCEPT
        {
            Logger::WriteMessage(L"ctsConfig::PrintConnectionResults(ctsUdpStatistics)\n");
        }
        bool IsListening() NOEXCEPT
        {
            return false;
        }
    }

    /// ctsSocketBroker stubs - when ctsSocketState calls out to update the broker
    void ctsSocketBroker::initiating_io() NOEXCEPT
    {
    }
    void ctsSocketBroker::closing(bool _was_active) NOEXCEPT
    {
    }
}
///
/// End of Fakes
///


using namespace ctsTraffic;

static long s_CallbackCount = 0L;

static DWORD s_CreateReturnCode = 0UL;
static DWORD s_ConnectReturnCode = 0UL;
static DWORD s_IOReturnCode = 0UL;
static DWORD s_ShouldNeverHitErrorCode = 0xffffffffUL;
void ResetStatics(DWORD _create = s_ShouldNeverHitErrorCode, DWORD _connect = s_ShouldNeverHitErrorCode, DWORD _io = s_ShouldNeverHitErrorCode)
{
    s_CallbackCount = 0L;
    s_CreateReturnCode = _create;
    s_ConnectReturnCode = _connect;
    s_IOReturnCode = _io;
}

void CreateFunctionHook(std::weak_ptr<ctsSocket> _socket) NOEXCEPT
{
    auto shared_socket(_socket.lock());
    Assert::IsNotNull(shared_socket.get());

    Assert::AreNotEqual(s_ShouldNeverHitErrorCode, s_CreateReturnCode);

    ctl::ctMemoryGuardIncrement(&s_CallbackCount);
    if (shared_socket) {
        shared_socket->complete_state(s_CreateReturnCode);
    }
}

void ConnectFunctionHook(std::weak_ptr<ctsSocket> _socket) NOEXCEPT
{
    auto shared_socket(_socket.lock());
    Assert::IsNotNull(shared_socket.get());

    Assert::AreNotEqual(s_ShouldNeverHitErrorCode, s_ConnectReturnCode);

    ctl::ctMemoryGuardIncrement(&s_CallbackCount);
    if (shared_socket) {
        shared_socket->complete_state(s_ConnectReturnCode);
    }
}

void IoFunctionHook(std::weak_ptr<ctsSocket> _socket) NOEXCEPT
{
    auto shared_socket(_socket.lock());
    Assert::IsNotNull(shared_socket.get());

    Assert::AreNotEqual(s_ShouldNeverHitErrorCode, s_IOReturnCode);

    ctl::ctMemoryGuardIncrement(&s_CallbackCount);
    if (shared_socket) {
        shared_socket->complete_state(s_IOReturnCode);
    }
}


namespace ctsUnitTest {
    TEST_CLASS(ctsSocketStateUnitTest)
    {
    public:
        TEST_CLASS_INITIALIZE(Setup)
        {
            ctsConfig::Settings = new ctsConfig::ctsConfigSettings;
            ctsConfig::Settings->CreateFunction = CreateFunctionHook;
            ctsConfig::Settings->ConnectFunction = ConnectFunctionHook;
            ctsConfig::Settings->IoFunction = IoFunctionHook;
        }
        TEST_CLASS_CLEANUP(Cleanup)
        {
            delete ctsConfig::Settings;
        }

        TEST_METHOD(AllIOSucceed)
        {
            // expect all to pass
            ResetStatics(0, 0, 0);

            std::shared_ptr<ctsSocketState> test(std::make_shared<ctsSocketState>(nullptr));
            test->start();

            do {
                ::Sleep(100);
            } while (ctsSocketState::InternalState::Closed != test->current_state());

            Assert::AreEqual(3L, ctl::ctMemoryGuardRead(&s_CallbackCount));
        }

        TEST_METHOD(CreateFails)
        {
            // create should fail, the others never invoked
            ResetStatics(1);

            std::shared_ptr<ctsSocketState> test(std::make_shared<ctsSocketState>(nullptr));
            test->start();

            do {
                ::Sleep(100);
            } while (ctsSocketState::InternalState::Closed != test->current_state());

            Assert::AreEqual(1L, ctl::ctMemoryGuardRead(&s_CallbackCount));
        }

        TEST_METHOD(ConnectFails)
        {
            // connect should fail, IO should never invoked
            ResetStatics(0, 1);

            std::shared_ptr<ctsSocketState> test(std::make_shared<ctsSocketState>(nullptr));
            test->start();

            do {
                ::Sleep(100);
            } while (ctsSocketState::InternalState::Closed != test->current_state());

            Assert::AreEqual(2L, ctl::ctMemoryGuardRead(&s_CallbackCount));
        }

        TEST_METHOD(IOFails)
        {
            // IO should fail
            ResetStatics(0, 0, 1);

            std::shared_ptr<ctsSocketState> test(std::make_shared<ctsSocketState>(nullptr));
            test->start();

            do {
                ::Sleep(100);
            } while (ctsSocketState::InternalState::Closed != test->current_state());

            Assert::AreEqual(3L, ctl::ctMemoryGuardRead(&s_CallbackCount));
        }
    };
}