/*

Copyright (c) Microsoft Corporation
All rights reserved.

Licensed under the Apache License, Version 2.0 (the ""License""); you may not use this file except in compliance with the License. You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

THIS CODE IS PROVIDED ON AN  *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.

See the Apache Version 2.0 License for specific language governing permissions and limitations under the License.

*/

#include <SDKDDKVer.h>
#include "CppUnitTest.h"

#include <memory>
#include <vector>
#include <algorithm>

#include <ctScopeGuard.hpp>
#include <ctString.hpp>

#include "ctsIOBuffers.hpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

ctsTraffic::ctsUnsignedLongLong s_TransferSize = 0ULL;
bool s_Listening = true;
///
/// Fakes
///
namespace ctsTraffic {
    namespace ctsConfig {
        ctsConfigSettings* Settings;

        void PrintConnectionResults(const ctl::ctSockaddr& _local_addr, const ctl::ctSockaddr& _remote_addr, unsigned long _error) throw()
        {
        }
        void PrintConnectionResults(const ctl::ctSockaddr& _local_addr, const ctl::ctSockaddr& _remote_addr, unsigned long _error, const ctsTcpStatistics& _stats) throw()
        {
        }
        void PrintConnectionResults(const ctl::ctSockaddr& _local_addr, const ctl::ctSockaddr& _remote_addr, unsigned long _error, const ctsUdpStatistics& _stats) throw()
        {
        }
        void PrintDebug(_In_z_ _Printf_format_string_ LPCWSTR _text, ...) throw()
        {
        }
        void PrintException(const std::exception& e) throw()
        {
        }
        void PrintJitterUpdate(long long _sequence_number, long long _sender_qpc, long long _sender_qpf, long long _recevier_qpc, long long _receiver_qpf) throw()
        {
        }
        void PrintErrorInfo(_In_z_ _Printf_format_string_ LPCWSTR _text, ...) throw()
        {
        }

        bool IsListening( ) throw()
        {
            return s_Listening;
        }

        ctsUnsignedLongLong GetTransferSize( ) throw()
        {
            return s_TransferSize;
        }
    }
}
///
/// End of Fakes
///

using namespace ctsTraffic; 
namespace ctsUnitTest
{		
    TEST_CLASS(ctsIOBuffersUnitTest_Server)
    {
    private:
        ctsTcpStatistics stats;

    public:
        TEST_CLASS_INITIALIZE(Setup)
        {
            ctsConfig::Settings = new ctsConfig::ctsConfigSettings;
            ctsConfig::Settings->Protocol = ctsConfig::ProtocolType::TCP;
            statics::ServerConnectionGrowthRate = 10; // changing the growth rate so that it's easier to test (we don't want to test growing every 4096)
        }

        TEST_CLASS_CLEANUP(Cleanup)
        {
            delete ctsConfig::Settings;
        }
        
        TEST_METHOD(RequestAndReturnOneConnection)
        {
            ctsIOTask test_task;
            ctlScopeGuard(return_test_task, { ctsIOBuffers::ReleaseConnectionIdBuffer(test_task); });

            test_task = ctsIOBuffers::NewConnectionIdBuffer(stats.connection_identifier);
            Assert::AreEqual(ctsStatistics::ConnectionIdLength, test_task.buffer_length);
            Assert::IsNotNull(test_task.buffer);
            Assert::AreEqual(0UL, test_task.buffer_offset);

            return_test_task.run_once( );

            ctsIOTask test_task_second;
            ctlScopeGuard(return_test_task_second, { ctsIOBuffers::ReleaseConnectionIdBuffer(test_task_second); });
            test_task_second = ctsIOBuffers::NewConnectionIdBuffer(stats.connection_identifier);
            Assert::AreEqual(test_task_second.buffer, test_task.buffer);

            // scope guard will return the buffers
        }

        TEST_METHOD(RequestAndReturnAllConnections)
        {
            std::vector<ctsIOTask> test_tasks;
            ctlScopeGuard(return_test_tasks, { 
                for (auto& task : test_tasks) {
                    ctsIOBuffers::ReleaseConnectionIdBuffer(task);
                }
            });

            for (unsigned long add_tasks = 0; add_tasks < statics::ServerConnectionGrowthRate; ++add_tasks) {
                test_tasks.push_back(ctsIOBuffers::NewConnectionIdBuffer(stats.connection_identifier));
            }
            for (auto& task : test_tasks) {
                Assert::AreEqual(ctsStatistics::ConnectionIdLength, task.buffer_length);
                Assert::IsNotNull(task.buffer);
                Assert::AreEqual(0UL, task.buffer_offset);
            }

            // return the buffers back via the scope guard
            return_test_tasks.run_once( );

            std::vector<ctsIOTask> test_tasks_second;
            ctlScopeGuard(return_test_tasks_second, {
                for (auto& task : test_tasks_second) {
                    ctsIOBuffers::ReleaseConnectionIdBuffer(task);
                }
            });

            for (unsigned long add_tasks = 0; add_tasks < statics::ServerConnectionGrowthRate; ++add_tasks) {
                test_tasks_second.push_back(ctsIOBuffers::NewConnectionIdBuffer(stats.connection_identifier));
            }

            // since I returned the first buffers in reverse order, they'll be reversed here
            auto iter_first_task = test_tasks.rbegin( );
            for (auto& task : test_tasks_second) {
                Assert::AreEqual(iter_first_task->buffer, task.buffer);
                ++iter_first_task;
            }

            // scope guard will return the buffers
        }

        TEST_METHOD(RequestAndReturnDoubleGrowthRate)
        {
            std::vector<ctsIOTask> test_tasks;
            ctlScopeGuard(return_test_tasks, {
                for (auto& task : test_tasks) {
                    ctsIOBuffers::ReleaseConnectionIdBuffer(task);
                }
            });

            for (unsigned long add_tasks = 0; add_tasks < statics::ServerConnectionGrowthRate; ++add_tasks) {
                test_tasks.push_back(ctsIOBuffers::NewConnectionIdBuffer(stats.connection_identifier));
            }
            for (auto& task : test_tasks) {
                Assert::AreEqual(ctsStatistics::ConnectionIdLength, task.buffer_length);
                Assert::IsNotNull(task.buffer);
                Assert::AreEqual(0UL, task.buffer_offset);
            }

            std::vector<ctsIOTask> test_tasks_second;
            ctlScopeGuard(return_test_tasks_second, {
                for (auto& task : test_tasks_second) {
                    ctsIOBuffers::ReleaseConnectionIdBuffer(task);
                }
            });

            for (unsigned long add_tasks = 0; add_tasks < statics::ServerConnectionGrowthRate; ++add_tasks) {
                test_tasks_second.push_back(ctsIOBuffers::NewConnectionIdBuffer(stats.connection_identifier));
            }
            for (auto& task : test_tasks) {
                Assert::AreEqual(ctsStatistics::ConnectionIdLength, task.buffer_length);
                Assert::IsNotNull(task.buffer);
                Assert::AreEqual(0UL, task.buffer_offset);
            }

            // scope guard will return the buffers
        }

        TEST_METHOD(RequestAndReturnOnePageOfBuffers)
        {
            ::SYSTEM_INFO sysInfo;         // Useful information about the system
            ::GetSystemInfo(&sysInfo);     // Initialize the structure.
            unsigned long buffersToAdd = (sysInfo.dwPageSize / ctsStatistics::ConnectionIdLength);

            std::vector<char*> test_buffers;
            std::vector<ctsIOTask> test_tasks;
            ctlScopeGuard(return_test_tasks, {
                for (auto& task : test_tasks) {
                    ctsIOBuffers::ReleaseConnectionIdBuffer(task);
                }
            });

            for (unsigned long add_tasks = 0; add_tasks < buffersToAdd; ++add_tasks) {
                ctsIOTask temp_task = ctsIOBuffers::NewConnectionIdBuffer(stats.connection_identifier);
                test_tasks.push_back(temp_task);
                test_buffers.push_back(temp_task.buffer);
            }
            for (auto& task : test_tasks) {
                Assert::AreEqual(ctsStatistics::ConnectionIdLength, task.buffer_length);
                Assert::IsNotNull(task.buffer);
                Assert::AreEqual(0UL, task.buffer_offset);
            }

            std::sort(test_buffers.begin( ), test_buffers.end( ));
            if (std::adjacent_find(test_buffers.begin( ), test_buffers.end( )) != test_buffers.end( )) {
                Assert::Fail(L"The same buffer was given to 2 different ctsIOTasks");
            }

            return_test_tasks.run_once( );
            test_tasks.clear( );
            test_buffers.clear( );


            ctlScopeGuard(return_test_tasks2, {
                for (auto& task : test_tasks) {
                    ctsIOBuffers::ReleaseConnectionIdBuffer(task);
                }
            });

            for (unsigned long add_tasks = 0; add_tasks < buffersToAdd; ++add_tasks) {
                ctsIOTask temp_task = ctsIOBuffers::NewConnectionIdBuffer(stats.connection_identifier);
                test_tasks.push_back(temp_task);
                test_buffers.push_back(temp_task.buffer);
            }
            for (auto& task : test_tasks) {
                Assert::AreEqual(ctsStatistics::ConnectionIdLength, task.buffer_length);
                Assert::IsNotNull(task.buffer);
                Assert::AreEqual(0UL, task.buffer_offset);
            }

            std::sort(test_buffers.begin( ), test_buffers.end( ));
            if (std::adjacent_find(test_buffers.begin( ), test_buffers.end( )) != test_buffers.end( )) {
                Assert::Fail(L"The same buffer was given to 2 different ctsIOTasks");
            }

            return_test_tasks2.run_once( );
        }

        TEST_METHOD(RequestAndReturnOverOnePageOfBuffers)
        {
            ::SYSTEM_INFO sysInfo;         // Useful information about the system
            ::GetSystemInfo(&sysInfo);     // Initialize the structure.
            unsigned long buffersToAdd = (sysInfo.dwPageSize / ctsStatistics::ConnectionIdLength) + 1;

            std::vector<char*> test_buffers;
            std::vector<ctsIOTask> test_tasks;
            ctlScopeGuard(return_test_tasks, {
                for (auto& task : test_tasks) {
                    ctsIOBuffers::ReleaseConnectionIdBuffer(task);
                }
            });

            for (unsigned long add_tasks = 0; add_tasks < buffersToAdd; ++add_tasks) {
                ctsIOTask temp_task = ctsIOBuffers::NewConnectionIdBuffer(stats.connection_identifier);
                test_tasks.push_back(temp_task);
                test_buffers.push_back(temp_task.buffer);
            }
            for (auto& task : test_tasks) {
                Assert::AreEqual(ctsStatistics::ConnectionIdLength, task.buffer_length);
                Assert::IsNotNull(task.buffer);
                Assert::AreEqual(0UL, task.buffer_offset);
            }

            std::sort(test_buffers.begin( ), test_buffers.end( ));
            if (std::adjacent_find(test_buffers.begin( ), test_buffers.end( )) != test_buffers.end( )) {
                Assert::Fail(L"The same buffer was given to 2 different ctsIOTasks");
            }

            return_test_tasks.run_once( );
            test_tasks.clear( );
            test_buffers.clear( );


            ctlScopeGuard(return_test_tasks2, {
                for (auto& task : test_tasks) {
                    ctsIOBuffers::ReleaseConnectionIdBuffer(task);
                }
            });

            for (unsigned long add_tasks = 0; add_tasks < buffersToAdd; ++add_tasks) {
                ctsIOTask temp_task = ctsIOBuffers::NewConnectionIdBuffer(stats.connection_identifier);
                test_tasks.push_back(temp_task);
                test_buffers.push_back(temp_task.buffer);
            }
            for (auto& task : test_tasks) {
                Assert::AreEqual(ctsStatistics::ConnectionIdLength, task.buffer_length);
                Assert::IsNotNull(task.buffer);
                Assert::AreEqual(0UL, task.buffer_offset);
            }

            std::sort(test_buffers.begin( ), test_buffers.end( ));
            if (std::adjacent_find(test_buffers.begin( ), test_buffers.end( )) != test_buffers.end( )) {
                Assert::Fail(L"The same buffer was given to 2 different ctsIOTasks");
            }

            return_test_tasks2.run_once( );
        }

        TEST_METHOD(RequestAndReturnOverTwoPagesOfBuffers)
        {
            ::SYSTEM_INFO sysInfo;         // Useful information about the system
            ::GetSystemInfo(&sysInfo);     // Initialize the structure.
            unsigned long buffersToAdd = ((sysInfo.dwPageSize / ctsStatistics::ConnectionIdLength) + 1) * 2;

            std::vector<char*> test_buffers;
            std::vector<ctsIOTask> test_tasks;
            ctlScopeGuard(return_test_tasks, {
                for (auto& task : test_tasks) {
                    ctsIOBuffers::ReleaseConnectionIdBuffer(task);
                }
            });

            for (unsigned long add_tasks = 0; add_tasks < buffersToAdd; ++add_tasks) {
                ctsIOTask temp_task = ctsIOBuffers::NewConnectionIdBuffer(stats.connection_identifier);
                test_tasks.push_back(temp_task);
                test_buffers.push_back(temp_task.buffer);
            }
            for (auto& task : test_tasks) {
                Assert::AreEqual(ctsStatistics::ConnectionIdLength, task.buffer_length);
                Assert::IsNotNull(task.buffer);
                Assert::AreEqual(0UL, task.buffer_offset);
            }

            std::sort(test_buffers.begin( ), test_buffers.end( ));
            if (std::adjacent_find(test_buffers.begin( ), test_buffers.end( )) != test_buffers.end( )) {
                Assert::Fail(L"The same buffer was given to 2 different ctsIOTasks");
            }

            return_test_tasks.run_once( );
            test_tasks.clear( );
            test_buffers.clear( );


            ctlScopeGuard(return_test_tasks2, {
                for (auto& task : test_tasks) {
                    ctsIOBuffers::ReleaseConnectionIdBuffer(task);
                }
            });

            for (unsigned long add_tasks = 0; add_tasks < buffersToAdd; ++add_tasks) {
                ctsIOTask temp_task = ctsIOBuffers::NewConnectionIdBuffer(stats.connection_identifier);
                test_tasks.push_back(temp_task);
                test_buffers.push_back(temp_task.buffer);
            }
            for (auto& task : test_tasks) {
                Assert::AreEqual(ctsStatistics::ConnectionIdLength, task.buffer_length);
                Assert::IsNotNull(task.buffer);
                Assert::AreEqual(0UL, task.buffer_offset);
            }

            std::sort(test_buffers.begin( ), test_buffers.end( ));
            if (std::adjacent_find(test_buffers.begin( ), test_buffers.end( )) != test_buffers.end( )) {
                Assert::Fail(L"The same buffer was given to 2 different ctsIOTasks");
            }

            return_test_tasks2.run_once( );
        }
    };
}