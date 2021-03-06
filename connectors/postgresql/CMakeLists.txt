# Copyright (c) 2017 - 2018, Roland Bock
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
#   Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
#
#   Redistributions in binary form must reproduce the above copyright notice, this
#   list of conditions and the following disclaimer in the documentation and/or
#   other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

find_package(Threads REQUIRED)

find_package(PostgreSQL)
set_package_properties(PostgreSQL PROPERTIES URL "https://www.postgresql.org/" DESCRIPTION "PostgresSQL support" TYPE OPTIONAL)

if (POSTGRESQL_FOUND)
    add_library(sqlpp17-connector-postgresql INTERFACE)

   target_include_directories(sqlpp17-connector-postgresql INTERFACE
        ${PostgreSQL_INCLUDE_DIRS}
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )

    target_link_libraries(sqlpp17-connector-postgresql INTERFACE sqlpp17)
    target_link_libraries(sqlpp17-connector-postgresql INTERFACE "${PostgreSQL_LIBRARIES}")

    add_subdirectory(tests)

    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/sqlpp17"
            DESTINATION include
            FILES_MATCHING PATTERN "*.h")
endif()
