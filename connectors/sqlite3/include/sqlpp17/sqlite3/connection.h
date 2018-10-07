#pragma once

/*
Copyright (c) 2017 - 2018, Roland Bock
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

#include <functional>
#include <type_traits>

#include <sqlpp17/connection_base.h>
#include <sqlpp17/exception.h>
#include <sqlpp17/result.h>
#include <sqlpp17/statement.h>
#include <sqlpp17/clause/command.h>

#include <sqlpp17/sqlite3/clause.h>
#include <sqlpp17/sqlite3/connection_config.h>
#include <sqlpp17/sqlite3/context.h>
#include <sqlpp17/sqlite3/parameter.h>
#include <sqlpp17/sqlite3/prepared_statement.h>
#include <sqlpp17/sqlite3/prepared_statement_result.h>

namespace sqlpp::sqlite3
{
  template <typename Pool, ::sqlpp::debug Debug>
  class base_connection;

  template <::sqlpp::debug Debug = ::sqlpp::debug::allowed>
  using connection_t = base_connection<no_pool, Debug>;

};  // namespace sqlpp::sqlite3

namespace sqlpp::sqlite3::detail
{
  struct connection_cleanup_t
  {
  public:
    auto operator()(::sqlite3* handle) -> void
    {
      auto rc = sqlite3_close(handle);
      if (rc != SQLITE_OK)
      {
        throw sqlpp::exception(std::string("Sqlite3 error: Can't close database: ") + sqlite3_errmsg(handle));
      }
    }
  };
  using unique_connection_ptr = std::unique_ptr<::sqlite3, detail::connection_cleanup_t>;

}  // namespace sqlpp::sqlite3::detail

namespace sqlpp::sqlite3
{
  template<typename Pool, ::sqlpp::debug Debug>
  class base_connection : public ::sqlpp::connection_base
  {
    detail::unique_connection_ptr _handle;
    Pool* _connection_pool = nullptr;
    bool _transaction_active = false;
    std::function<void(std::string_view)> _debug;

    template <typename... Clauses>
    friend class ::sqlpp::statement;

    template <typename Clause, typename Statement>
    friend class ::sqlpp::clause_base;

    friend Pool;

    base_connection(const connection_config_t& config,
                 detail::unique_connection_ptr&& handle,
                 Pool* connection_pool)
        : _handle(std::move(handle)), _connection_pool(connection_pool), _debug(config.debug)
    {
    }

    base_connection(const connection_config_t& config, Pool* connection_pool) : base_connection(config)
    {
      _connection_pool = connection_pool;
    }

  public:
    base_connection() = delete;
    base_connection(const connection_config_t& config) : _handle(nullptr, {}), _debug(config.debug)
    {
      ::sqlite3* connection_ptr = nullptr;
      const auto rc = sqlite3_open_v2(config.path_to_database.c_str(), &connection_ptr, config.flags,
                                      config.vfs.empty() ? nullptr : config.vfs.c_str());
      _handle.reset(connection_ptr);

      if (rc != SQLITE_OK)
      {
        throw sqlpp::exception("Sqlite3: Can't open database: " + std::string(sqlite3_errmsg(connection_ptr)));
      }

#ifdef SQLITE_HAS_CODEC
      if (config.password.size() > 0)
      {
        const auto ret = sqlite3_key(sqlite, config.password.data(), config.password.size());
        if (ret != SQLITE_OK)
        {
          const std::string msg = sqlite3_errmsg(sqlite);
          throw sqlpp::exception("Sqlite3: Can't set password for database: " +
                                 std::string(sqlite3_errmsg(connection_ptr)));
        }
      }
#endif
    }

#warning : There are a bunch of additional functions in the sqlpp11 connector
    base_connection(const base_connection&) = delete;
    base_connection(base_connection&&) = default;
    base_connection& operator=(const base_connection&) = delete;
    base_connection& operator=(base_connection&&) = default;
    ~base_connection()
    {
      if constexpr (not std::is_same_v<Pool, ::sqlpp::no_pool>)
      {
        if (_connection_pool)
          _connection_pool->put(std::move(_handle));
      }
    }

    template <typename... Clauses>
    auto operator()(const ::sqlpp::statement<Clauses...>& statement)
    {
      using Statement = ::sqlpp::statement<Clauses...>;
      if constexpr (constexpr auto _check = check_statement_executable<base_connection>(type_v<Statement>); _check)
      {
        using ResultType = result_type_of_t<Statement>;
        if constexpr (std::is_same_v<ResultType, insert_result>)
        {
          return insert(statement);
        }
        else if constexpr (std::is_same_v<ResultType, delete_result>)
        {
          return delete_from(statement);
        }
        else if constexpr (std::is_same_v<ResultType, update_result>)
        {
          return update(statement);
        }
        else if constexpr (std::is_same_v<ResultType, select_result>)
        {
          return select(statement);
        }
        else if constexpr (std::is_same_v<ResultType, execute_result>)
        {
          return execute(statement);
        }
        else
        {
          static_assert(wrong<Statement>, "Unknown statement type");
        }
      }
      else
      {
        return ::sqlpp::bad_expression_t{_check};
      }
    }

    template <typename... Clauses>
    auto prepare(const ::sqlpp::statement<Clauses...>& statement)
    {
      using Statement = ::sqlpp::statement<Clauses...>;
      if constexpr (constexpr auto _check = check_statement_preparable<base_connection>(type_v<Statement>); _check)
      {
#warning: Need to have an enum for result owns statement
        return ::sqlpp::sqlite3::prepared_statement_t{*this, statement, false};
      }
      else
      {
        return ::sqlpp::bad_expression_t{_check};
      }
    }

    auto start_transaction() -> void
    {
      if (_transaction_active)
      {
        throw sqlpp::exception("Sqlite3: Cannot have more than one open transaction per connection");
      }

      auto prepared_statement = prepared_statement_t{*this, ::sqlpp::command("BEGIN TRANSACTION"), true};
      prepared_statement.execute();
      _transaction_active = true;
    }

    auto commit() -> void
    {
      if (not _transaction_active)
      {
        throw sqlpp::exception("Sqlite3: Cannot commit without active transaction");
      }

      auto prepared_statement = prepared_statement_t{*this, ::sqlpp::command("COMMIT"), true};
      prepared_statement.execute();
#warning: Need to check other connectors, if they have the order of things correct, here.
      _transaction_active = false;
    }

    auto rollback() -> void
    {
      if (not _transaction_active)
      {
        throw sqlpp::exception("Sqlite3: Cannot rollback without active transaction");
      }

      auto prepared_statement = prepared_statement_t{*this, ::sqlpp::command("ROLLBACK"), true};
      prepared_statement.execute();
      _transaction_active = false;
    }

    auto destroy_transaction() noexcept -> void
    {
      try
      {
        if (debug())
          debug()("Auto rollback!");

        rollback();
      }
      catch (...)
      {
        // This is called in ~transaction().
        // We must not throw
      }
    }

    auto debug() const
    {
      return _debug;
    }

    auto* get() const
    {
      return _handle.get();
    }

    auto is_alive() -> bool;

  private:
    template <typename... Clauses>
    auto execute(const ::sqlpp::statement<Clauses...>& statement)
    {
      auto prepared_statement = prepare(statement);
      prepared_statement.execute();
    }

    template <typename Statement>
    auto insert(const Statement& statement)
    {
      auto prepared_statement = prepare(statement);
      return prepared_statement.execute();
    }

    template <typename Statement>
    auto update(const Statement& statement)
    {
      auto prepared_statement = prepare(statement);
      return prepared_statement.execute();
    }

    template <typename Statement>
    auto delete_from(const Statement& statement)
    {
      auto prepared_statement = prepare(statement);
      return prepared_statement.execute();
    }

    template <typename Statement>
    [[nodiscard]] auto select(const Statement& statement)
    {
      auto prepared_statement = prepare(statement);
      return prepared_statement.execute();
    }

  };

}  // namespace sqlpp::sqlite3
