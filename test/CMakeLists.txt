cmake_minimum_required(VERSION 3.20.0)
project(libstorage_tests)

enable_testing()
find_package(GTest REQUIRED)

include_directories(${LIBSTORAGE_HEADERS_DIR})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused")

macro(package_add_test TESTNAME)
    # create an executable in which the tests will be stored
    add_executable(${TESTNAME} ${ARGN}) # main.cpp
    # link the Google test infrastructure, mocking library, and a default main function to
    # the test executable.  Remove g_test_main if writing your own main function.
    target_link_libraries(${TESTNAME} GTest::GTest GTest::Main libstorage) #GTest::Main
    # gtest_discover_tests replaces gtest_add_tests,
    # see https://cmake.org/cmake/help/v3.10/module/GoogleTest.html for more options to pass to it
    gtest_discover_tests(${TESTNAME}
        # set a working directory so your project root so that you can find test data via paths relative to the project root
        WORKING_DIRECTORY ${PROJECT_DIR}
        PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_DIR}"
    )
    set_target_properties(${TESTNAME} PROPERTIES FOLDER tests)
endmacro()

package_add_test(IntervalMap src/IntervalMap.cpp)
package_add_test(StorageBuffer src/StorageBuffer.cpp)
package_add_test(Serialize src/Serialize.cpp)
package_add_test(RMA src/RMA.cpp)
package_add_test(VirtAddressMapping src/VirtAddressMapping.cpp)
package_add_test(SimpleStorage src/SimpleStorage.cpp)
package_add_test(VirtualFileCatalog src/VirtualFileCatalog.cpp)
package_add_test(UniqueID src/UniqueID.cpp)
