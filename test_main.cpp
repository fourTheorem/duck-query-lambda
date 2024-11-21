#include <iostream>
#include <duckdb.hpp>

using namespace std;
using namespace duckdb;

int main()
{
  DuckDB db(nullptr);
  Connection con(db);
  auto result = con.Query(
      "SELECT * FROM 'https://www.timestored.com/data/sample/iris.parquet'");
  result->Print();

  auto platform = con.Query("PRAGMA platform");
  platform->Print();

  return 0;
}
