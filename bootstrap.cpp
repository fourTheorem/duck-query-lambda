// Custom AWS Lambda Runtime for DuckDB queries 🦆🐑
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
using namespace Aws::Utils::Json;

/**
 * Base64 encoding utility to allow returning binary data from the Lambda function.
 * This allows the query result to be saved to a temporary file (JSON, CSV, Parquet) and returned as a base64 encoded string.
 */
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

/**
 * Lambda event handler
 */
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
    // The caller can specify a query with "COPY .. TO '<outputFile>'" so DuckDB
    // saves the result to a file. If they also specify this filename in the event JSON's `outputFile`
    // property, it will be base64 encoded and returned as the result. This temporary file is also deleted before
    // the function completes.
    auto outputFile = view.GetString("outputFile");
    if (view.KeyExists("outputFormat"))
    {
      auto outputFormat = view.GetString("outputFormat");
      if (outputFormat == "json")
      {
        // Read file as JSON string
        ifstream fileStream(outputFile);
        if (!fileStream)
        {
          throw std::runtime_error("Failed to open file");
        }
        std::string jsonString((std::istreambuf_iterator<char>(fileStream)), std::istreambuf_iterator<char>());
        fileStream.close();
        remove(outputFile.c_str());
        return invocation_response::success(jsonString, "application/json");
      }
    }
    // Vanilla base64 encoded response
    auto encodedOutput = b64encode(outputFile);
    // Delete the file after encoding
    remove(outputFile.c_str());
    return invocation_response::success(encodedOutput, "application/base64");
  }

  return invocation_response::success(result->ToString(), "text/plain");
}

int main()
{
  cout << "Running Lambda Handler " << __DATE__ << "," << __TIME__ << endl;

  // We use a single connection for all queries. This may be good for performance in some cases, but
  // is probably not ideal in the spirit of single execution isolation.
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