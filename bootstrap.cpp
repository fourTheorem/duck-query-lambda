#include <aws/core/utils/HashingUtils.h>
#include <aws/core/utils/json/JsonSerializer.h>
#include <aws/core/utils/memory/stl/SimpleStringStream.h>
#include <aws/lambda-runtime/runtime.h>

#include <duckdb.hpp>
#include <fstream>
#include <iostream>

using namespace duckdb;
using namespace std;
using namespace aws::lambda_runtime;
// using namespace Aws::Utils::Logging;
using namespace Aws::Utils::Json;

std::string b64encode(const std::string &filename)
{
  ifstream fileStream(filename);
  if (!fileStream)
  {
    throw std::runtime_error("Failed to open file");
  }

  std::vector<unsigned char> fileContents;

  char fileBuffer[1024 * 4];
  while (fileStream.good())
  {
    fileStream.read(fileBuffer, sizeof(fileBuffer));
    auto bytesRead = fileStream.gcount();
    if (bytesRead > 0)
    {
      fileContents.insert(fileContents.end(), (unsigned char *)fileBuffer, (unsigned char *)fileBuffer + bytesRead);
    }
  }

  Aws::Utils::ByteBuffer bb(fileContents.data(), fileContents.size());
  return Aws::Utils::HashingUtils::Base64Encode(bb);
}

static invocation_response query_handler(invocation_request const &req, Connection &con)
{
  JsonValue event(req.payload);
  if (!event.WasParseSuccessful())
  {
    return invocation_response::failure("Failed to parse input JSON",
                                        "InvalidJSON");
  }

  auto view = event.View();
  if (!view.KeyExists("query"))
  {
    return invocation_response::failure("Missing 'query' key in input JSON",
                                        "InvalidJSON");
  }

  auto query = view.GetString("query");
  auto result = con.Query(query);
  if (result->HasError())
  {
    return invocation_response::failure(result->GetError(), "QueryError");
  }

  if (view.KeyExists("outputFile"))
  {
    auto outputFile = view.GetString("outputFile");
    auto encodedOutput = b64encode(outputFile);
    // Delete the file after encoding
    remove(outputFile.c_str());

    return invocation_response::success(encodedOutput,
                                        "application/base64");
  }

  return invocation_response::success(result->ToString(), "text/plain");
}

int main()
{
  cout << "Running Lambda Handler " << __DATE__ << "," << __TIME__ << endl;

  DuckDB db(nullptr);
  Connection con(db);
  auto result = con.Query("SET home_directory='/tmp'; SET extension_directory='/opt/duckdb_extensions';");
  if (result->HasError())
  {
    cerr << "Failed to set home directory: " << result->GetError() << endl;
    throw std::runtime_error(result->GetError());
  }

  auto platform = con.Query("PRAGMA platform");
  auto version = con.Query("PRAGMA version");
  cout << "DuckDB: " << platform->GetValue(0, 0) << " " << version->GetValue(0, 0) << endl;

  auto handler_fn = [&con](aws::lambda_runtime::invocation_request const &req)
  {
    return query_handler(req, con);
  };

  run_handler(handler_fn);

  return 0;
}
