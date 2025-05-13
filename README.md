# duck-query-lambda

`duck-query-lambda` is a custom AWS Lambda Layer that provides a runtime for executing DuckDB queries programmatically.

Since the [S3 Select feature has ended](https://aws.amazon.com/blogs/storage/how-to-optimize-querying-your-data-in-amazon-s3/), this function
provides an alternative for querying data stored in S3 (any other locations) using DuckDB.

This layer allows you to run DuckDB queries without writing any code or deploying anything other than a simple function based on this runtime. This makes it ideal for use in things like AWS Step Functions for tasks such as querying, data transformation, and more.

## Current status
- 🧪 Experimental
- 📱 Arm64 only

## Features

- **Custom Runtime**: Provides a custom runtime for AWS Lambda to execute DuckDB queries.
- **Data Transformation**: Perform complex data transformations using SQL queries.
- **Integration with Step Functions**: Easily integrate with AWS Step Functions for orchestrating data workflows.

## Getting Started

### Adding the Lambda Layer to your AWS account

The Lambda Layer for this DuckDB runtime is available in the AWS Serverless Application Repository. You can deploy it directly from the AWS Management Console or using the AWS CLI.

- Install from the AWS Console: https://serverlessrepo.aws.amazon.com/applications/eu-west-1/949339270388/duck-query-lambda
- Install using AWS SAM or CloudFormation:
```yaml
  duckquerylambda:
    Type: AWS::Serverless::Application
    Properties:
      Location:
        ApplicationId: arn:aws:serverlessrepo:eu-west-1:949339270388:applications/duck-query-lambda
        SemanticVersion: 0.1.1   # x-release-please-version
```  

- Install using the AWS CDK:
```typescript
import * as sam from "aws-cdk-lib/aws-sam";
...

  new sam.CfnApplication(this, "DuckQueryRuntimeLayer", {
    location: {
      applicationId: "arn:aws:serverlessrepo:eu-west-1:949339270388:applications/duck-query-lambda",
      semanticVersion: "0.1.1",   // x-release-please-version
    },
  });
```

An example SAM project can be found in the [`examples/`](./examples/) directory.

### Creating a Lambda function using the DuckDB runtime

You don't need to write any code to use the DuckDB runtime. You can create a Lambda function that uses the runtime, give it some IAM permissions and then invoke it with a query.


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

## Examples

### Using the Lambda Layer in an AWS Step Function
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
```

### Invoking the Lambda Function and getting the query results back synchronously

By default, the Lambda function will not return the query results. This is because it's not trivial to convert all results back to JSON in a way that every user expects. If you do want to get the results back synchronously, you can write them to a temporary file in the Lambda and the function will then return the contents of that file. This data is base64 encoded by default, but you can also return the raw JSON from the output file using the `outputFormat` parameter.

Here is an example of how to do this:

```json
{
  "query": "COPY (SELECT * FROM 'https://github.com/Teradata/kylo/raw/refs/heads/master/samples/sample-data/parquet/userdata1.parquet' LIMIT 10) TO '/tmp/output.json'",
  "outputFile": "/tmp/output.json",
  "outputFormat": "json"
}
```
