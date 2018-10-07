/*
Copyright (c) 2018 - 2018, Roland Bock
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this
   list of conditions and the following disclaimer in the documentation and/or
   other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <cfloat>
#include <iostream>

#include <sqlpp17/clause/create_table.h>
#include <sqlpp17/clause/drop_table.h>
#include <sqlpp17/clause/insert_into.h>
#include <sqlpp17/clause/select.h>
#include <sqlpp17/clause/truncate.h>
#include <sqlpp17/parameter.h>

#include <sqlpp17/postgresql/connection.h>

#include <sqlpp17_test/tables/TabFloat.h>

using test::tabFloat;

auto print_debug(std::string_view message)
{
  std::cout << "Debug: " << message << std::endl;
}

namespace postgresql = sqlpp::postgresql;
int main()
{
  auto config = postgresql::connection_config_t{};
  config.dbname = "sqlpp17_test";
  config.debug = print_debug;
  try
  {
    auto db = postgresql::connection_t<::sqlpp::debug::none>{config};
  }
  catch (const sqlpp::exception& e)
  {
    std::cerr << "For testing, you'll need to create a database sqlpp_test for user root (no password)" << std::endl;
    std::cerr << e.what() << std::endl;
    return 1;
  }
  try
  {
    auto db = postgresql::connection_t<::sqlpp::debug::allowed>{config};
    db(drop_table(tabFloat));
    db(create_table(tabFloat));

    std::cout << std::setprecision(std::numeric_limits<long double>::digits10 + 1);
    {
      db(truncate(tabFloat));
      [[maybe_unused]] auto id =
          db(insert_into(tabFloat).set(tabFloat.valueFloat = 1.2345678901234567890,
                                       tabFloat.valueDouble = 1.2345678901234567890, tabFloat.valueInt = 1234567890));
      for (const auto& row : db(select(all_of(tabFloat)).from(tabFloat).unconditionally()))
      {
        std::cout << row.valueFloat << "\n" << row.valueDouble << "\n" << row.valueInt << "\n";
      }
    }
    {
      std::cout << "-------------------------------------------\n";
      std::cout << "prepared insert\n";
      std::cout << "-------------------------------------------\n";
      db(truncate(tabFloat));
      auto preparedInsert = db.prepare(insert_into(tabFloat).set(
          tabFloat.valueFloat = ::sqlpp::parameter<float>(tabFloat.valueFloat),
          tabFloat.valueDouble = ::sqlpp::parameter<double>(tabFloat.valueDouble), tabFloat.valueInt = 1234567890));
      preparedInsert.parameters.valueFloat = 1.2345678901234567890;
      preparedInsert.parameters.valueDouble = 1.2345678901234567890;
      execute(preparedInsert);
      preparedInsert.parameters.valueFloat = DBL_MIN / 2.0;
      preparedInsert.parameters.valueDouble = INFINITY;
      execute(preparedInsert);
      preparedInsert.parameters.valueFloat = std::nanf("");
      preparedInsert.parameters.valueDouble = std::nan("");
      execute(preparedInsert);
      for (const auto& row : db(select(all_of(tabFloat)).from(tabFloat).unconditionally()))
      {
        std::cout << "char result: " << row.valueFloat << "\n" << row.valueDouble << "\n" << row.valueInt << "\n";
      }
      auto preparedSelect = db.prepare(select(all_of(tabFloat)).from(tabFloat).unconditionally());
      for (const auto& row : execute(preparedSelect))
      {
        std::cout << "bind result: " << row.valueFloat << "\n" << row.valueDouble << "\n" << row.valueInt << "\n";
      }
    }
    {
      std::cout << "-------------------------------------------\n";
      std::cout << "big number insert\n";
      std::cout << "-------------------------------------------\n";
      db(truncate(tabFloat));
      [[maybe_unused]] auto id =
          db(insert_into(tabFloat).set(tabFloat.valueFloat = 1234567890.1234567890,
                                       tabFloat.valueDouble = 1234567890.1234567890, tabFloat.valueInt = 1234567890));
      for (const auto& row : db(select(all_of(tabFloat)).from(tabFloat).unconditionally()))
      {
        std::cout << row.valueFloat << "\n" << row.valueDouble << "\n" << row.valueInt << "\n";
      }
    }
  }
  catch (const std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << std::endl;
    return 1;
  }
}

