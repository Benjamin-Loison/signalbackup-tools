/*
  Copyright (C) 2024  Selwin van Dijk

  This file is part of signalbackup-tools.

  signalbackup-tools is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  signalbackup-tools is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with signalbackup-tools.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "signalbackup.ih"

#if __cpp_lib_ranges >= 201911L
#include <ranges>
#endif

bool SignalBackup::updateRows(std::string const &table,
                              std::vector<std::pair<std::string, std::any>> data,
                              std::vector<std::pair<std::string, std::any>> whereclause,
                              std::string const &returnfield, std::any *returnvalue) const
{
  // check if columns exist...
  for (auto it = data.begin(); it != data.end();)
  {
    if (it->first.empty())
      it = data.erase(it);
    else if (!d_database.tableContainsColumn(table, it->first))
    {
      Logger::warning("Table '", table, "' does not contain any column '", it->first, "'. Removing");
      it = data.erase(it);
    }
    else
      ++it;
  }

  std::string query = "UPDATE " + table + " SET ";
  for (uint i = 0; i < data.size(); ++i)
    query += data[i].first + (i < data.size() -1 ? " = ?, " : " = ?");

  if (whereclause.size())
  {
    query += " WHERE ";
    for (uint i = 0; i < whereclause.size(); ++i)
      query += whereclause[i].first + (i < whereclause.size() -1 ? " = ? AND " : " = ?");
  }
  if (!returnfield.empty() && returnvalue)
    query += " RETURNING " + returnfield;

  SqliteDB::QueryResults res;

  // when concat_view gets implemented...
  // llvms feature test macro list does not have this value yet:
  // - https://en.cppreference.com/w/cpp/utility/feature_test
  // - https://libcxx.llvm.org/FeatureTestMacroTable.html)
#if false && __cpp_lib_ranges >= 201911L && __cpp_lib_ranges_concat >= 202403L
  bool ret = d_database.exec(query, std::ranges::views::values(std::ranges::views::concat(data, whereclause)), &res, d_verbose);
#else
  std::vector<std::any> values;
  std::transform(data.begin(), data.end(), std::back_inserter(values), [](auto const &pair){ return pair.second; });
  std::transform(whereclause.begin(), whereclause.end(), std::back_inserter(values), [](auto const &pair){ return pair.second; });
  bool ret = d_database.exec(query, values, &res, d_verbose);
#endif
  if (ret && !returnfield.empty() && returnvalue && res.rows() && res.columns())
  {
    if (res.rows() > 1 || res.columns() > 1)
      Logger::warning("Requested return of '", returnfield,
                      "', but query returned multiple results. Returning first.");
    *returnvalue = res.value(0, 0);
  }
  return ret;
}
