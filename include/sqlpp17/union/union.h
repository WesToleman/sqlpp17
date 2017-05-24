#pragma once

/*
Copyright (c) 2016, Roland Bock
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

#include <vector>
#include <sqlpp17/clause_fwd.h>
#include <sqlpp17/type_traits.h>
#include <sqlpp17/wrapped_static_assert.h>

namespace sqlpp
{
  namespace clause
  {
    struct union_
    {
    };
  }

  template <typename Flag, typename LeftSelect, typename RightSelect>
  struct union_t
  {
    Flag _flag;
    LeftSelect _left;
    RightSelect _right;
  };

  template <typename Flag, typename LeftSelect, typename RightSelect>
  constexpr auto is_result_clause_v<union_t<Flag, LeftSelect, RightSelect>> = true;

  template <typename Flag, typename LeftSelect, typename RightSelect>
  constexpr auto clause_tag<union_t<Flag, LeftSelect, RightSelect>> = clause::union_{};

  template <typename Flag, typename LeftSelect, typename RightSelect, typename Statement>
  class clause_base<union_t<Flag, LeftSelect, RightSelect>, Statement>
  {
  public:
    template <typename OtherStatement>
    clause_base(const clause_base<union_t<Flag, LeftSelect, RightSelect>, OtherStatement>& s)
        : _left(s._left), _right(s._right)
    {
    }

    clause_base(const union_t<Flag, LeftSelect, RightSelect>& f) : _flag(f._flag), _left(f._left), _right(f._right)
    {
    }

    Flag _flag;
    LeftSelect _left;
    RightSelect _right;
  };

  template <typename LeftColumn, typename RightColumn>
  struct merge_column_spec
  {
    static_assert(wrong<merge_column_spec>, "Invalid arguments for merge_column_spec");
  };

  template <typename LeftAlias,
            typename LeftCppType,
            tag::type LeftTags,
            typename RightAlias,
            typename RightCppType,
            tag::type RightTags>
  struct merge_column_spec<column_spec<LeftAlias, LeftCppType, LeftTags>,
                           column_spec<RightAlias, RightCppType, RightTags>>
  {
    using type = column_spec<LeftAlias,
                             LeftCppType,
                             ((LeftTags ^ tag::null_is_trivial_value) | (RightTags ^ tag::null_is_trivial_value)) ^
                                 tag::null_is_trivial_value>;
  };

  template <typename LeftResultRow, typename RightResultRow>
  struct merge_result_row_specs
  {
    static_assert(wrong<merge_result_row_specs>, "Invalid arguments for merge_result_row_specs");
  };

  template <typename... LeftColumnSpecs, typename... RightColumnSpecs>
  struct merge_result_row_specs<result_row_t<LeftColumnSpecs...>, result_row_t<RightColumnSpecs...>>
  {
    using type = result_row_t<typename merge_column_spec<LeftColumnSpecs, RightColumnSpecs>::type...>;
  };

  template <typename Flag, typename LeftSelect, typename RightSelect, typename Statement>
  class result_base<union_t<Flag, LeftSelect, RightSelect>, Statement>
  {
  public:
    using result_row_t =
        typename merge_result_row_specs<typename LeftSelect::result_row_t, typename RightSelect::result_row_t>::type;

    template <typename Connection>
    [[nodiscard]] auto run(Connection& connection) const
    {
      constexpr auto check = check_statement_executable<Connection>(type_v<Statement>);

      if
        constexpr(check)
        {
          return connection.select(Statement::of(this), result_row_t{});
        }
      else
      {
        return ::sqlpp::bad_statement_t{check};
      }
    }
  };

  template <typename Context, typename Flag, typename LeftSelect, typename RightSelect, typename Statement>
  decltype(auto) operator<<(Context& context, const clause_base<union_t<Flag, LeftSelect, RightSelect>, Statement>& t)
  {
    return context << embrace(t._left) << " UNION " << embrace(t._right);
  }
}
