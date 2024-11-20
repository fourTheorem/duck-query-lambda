# duck-query-lambda

`duck-query-lambda` is a custom AWS Lambda Layer that provides a runtime for executing DuckDB queries programmatically.

Since the [S3 Select feature has ended](https://aws.amazon.com/blogs/storage/how-to-optimize-querying-your-data-in-amazon-s3/), this function
provides an alternative for querying data stored in S3 (any other locations) using DuckDB.

This layer allows you to run DuckDB queries without writing any code or deploying anything other than a simple function based on this runtime. This makes it ideal for use in things like AWS Step Functions for tasks such as querying, data transformation, and more.

## Current status
- 🧪 Experimental
- 📑 Limited documentation
- 💃 Working under tests to date

## Features

- **Custom Runtime**: Provides a custom runtime for AWS Lambda to execute DuckDB queries.
- **Data Transformation**: Perform complex data transformations using SQL queries.
- **Integration with Step Functions**: Easily integrate with AWS Step Functions for orchestrating data workflows.

## Usage

### Prerequisites

- AWS CLI
- AWS SAM CLI
- Docker (for building the Lambda Layer)

### Building the Lambda Layer

1. Clone the repository:
    ```sh
    git clone https://github.com/fourTheorem/duck-query-lambda.git
    cd duck-query-lambda
    ```

2. Build the Lambda Layer:
    ```sh
    sam build
    ```

3. Package the Lambda Layer:
    ```sh
    sam package --output-template-file packaged.yaml --s3-bucket <your-s3-bucket>
    ```

4. Deploy the Lambda Layer:
    ```sh
    sam deploy --template-file packaged.yaml --stack-name duck-query-lambda --capabilities CAPABILITY_IAM
    ```

### Using the Lambda Layer

1. Add the Lambda Layer to your Lambda function (AWS SAM example):
    ```yaml
    Resources:
      MyLambdaFunction:
        Type: AWS::Serverless::Function
        Properties:
          Handler: bootstrap
          Runtime: provided
          Architecture: arm64
          Layers:
            - arn:aws:lambda:<region>:<account-id>:layer:duck-query-lambda:<version>
    ```

2. Invoke the Lambda function with a query:
    ```json
    {
      "query": "SELECT * FROM 's3://bucket/table.parquet'"
    }
    ```

### Example

Here is an example of how to use the Lambda Layer in an AWS Step Function:

```json
{
  "StartAt": "Convert Parquet to JSON",
  "States": {
    "RunQuery": {
      "Type": "Task",
      "Resource": "arn:aws:lambda:<region>:<account-id>:function:MyLambdaFunction",
      "Parameters": {
        "query": "COPY (SELECT * FROM 's3://bucket/table.parquet) TO 's3://bucket/output/result.json' (ARRAY)'",
      },
      "End": true
    }
  }
}