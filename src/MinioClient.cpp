#include "MinioClient.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <chrono>
#include <miniocpp/credentials.h>

MinioClient::MinioClient()
        : initialized(false), useSSL(false) {
    // minio-cpp client will be initialized when needed
}

MinioClient::~MinioClient() {
    cleanup();
}

bool MinioClient::initialize(const std::string& endpoint, const std::string& accessKey,
                             const std::string& secretKey, const std::string& bucket, bool useSSL) {
    try {
        this->endpoint = endpoint;
        this->accessKey = accessKey;
        this->secretKey = secretKey;
        this->bucket = bucket;
        this->useSSL = useSSL;

        // Create minio client using the correct constructor
        // Create BaseUrl object with host and https flag
        minio::s3::BaseUrl baseUrl(endpoint, useSSL);

        // Create credentials provider - store as member to ensure proper lifetime
        credentialsProvider = std::make_unique<minio::creds::StaticProvider>(accessKey, secretKey);

        // Create the client with BaseUrl and credentials provider
        client = std::make_unique<minio::s3::Client>(baseUrl, credentialsProvider.get());

        initialized = true;

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize MinioClient: " << e.what() << std::endl;
        return false;
    }
}


void MinioClient::cleanup() {
    std::lock_guard<std::mutex> lock(clientMutex);
    client.reset();
    credentialsProvider.reset();
    initialized = false;
}

Result<bool> MinioClient::bucketExists() {
    if (!initialized || !client) {
        return Result<bool>::Error("Client not initialized");
    }

    try {
        std::lock_guard<std::mutex> lock(clientMutex);
        minio::s3::BucketExistsArgs args;
        args.bucket = bucket;

        minio::s3::BucketExistsResponse resp = client->BucketExists(args);
        return Result<bool>::Success(resp.exist);
    } catch (const minio::error::Error& err) {
        return Result<bool>::Error("Bucket exists check failed: " + std::string(err.String()));
    }
}

Result<bool> MinioClient::makeBucket() {
    if (!initialized || !client) {
        return Result<bool>::Error("Client not initialized");
    }

    try {
        std::lock_guard<std::mutex> lock(clientMutex);
        minio::s3::MakeBucketArgs args;
        args.bucket = bucket;

        client->MakeBucket(args);
        return Result<bool>::Success(true);
    } catch (const minio::error::Error& err) {
        return Result<bool>::Error("Make bucket failed: " + std::string(err.String()));
    }
}

Result<bool> MinioClient::removeBucket() {
    if (!initialized || !client) {
        return Result<bool>::Error("Client not initialized");
    }

    try {
        std::lock_guard<std::mutex> lock(clientMutex);
        minio::s3::RemoveBucketArgs args;
        args.bucket = bucket;

        client->RemoveBucket(args);
        return Result<bool>::Success(true);
    } catch (const minio::error::Error& err) {
        return Result<bool>::Error("Remove bucket failed: " + std::string(err.String()));
    }
}

Result<std::vector<std::string>> MinioClient::listBuckets() {
    if (!initialized || !client) {
        return Result<std::vector<std::string>>::Error("Client not initialized");
    }

    try {
        std::lock_guard<std::mutex> lock(clientMutex);
        minio::s3::ListBucketsArgs args;

        minio::s3::ListBucketsResponse resp = client->ListBuckets(args);
        std::vector<std::string> bucketNames;

        for (const auto& bucket : resp.buckets) {
            bucketNames.push_back(bucket.name);
        }

        return Result<std::vector<std::string>>::Success(bucketNames);
    } catch (const minio::error::Error& err) {
        return Result<std::vector<std::string>>::Error("List buckets failed: " + std::string(err.String()));
    }
}

Result<bool> MinioClient::putObject(const std::string& objectName, const std::string& filePath,
                                   const std::string& contentType,
                                   const std::map<std::string, std::string>& metadata) {
    if (!initialized || !client) {
        return Result<bool>::Error("Client not initialized");
    }

    try {
        // Read file into memory first to avoid stream lifetime issues
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            return Result<bool>::Error("Cannot open file: " + filePath);
        }

        // Read file content into vector
        file.seekg(0, std::ios::end);
        long fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<uint8_t> fileData(fileSize);
        file.read(reinterpret_cast<char*>(fileData.data()), fileSize);
        file.close();

        // Use the vector-based putObject method
        return putObject(objectName, fileData, contentType, metadata);
    } catch (const std::exception& e) {
        return Result<bool>::Error("File read failed: " + std::string(e.what()));
    }
}

Result<bool> MinioClient::putObject(const std::string& objectName, const std::vector<uint8_t>& data,
                                   const std::string& contentType,
                                   const std::map<std::string, std::string>& metadata) {
    if (!initialized || !client) {
        return Result<bool>::Error("Client not initialized");
    }

    try {
        std::lock_guard<std::mutex> lock(clientMutex);

        // Create stringstream from vector data - keep it alive during the operation
        std::string dataStr(data.begin(), data.end());
        auto dataStream = std::make_unique<std::istringstream>(dataStr);

        minio::s3::PutObjectArgs args(*dataStream, data.size(), 0);
        args.bucket = bucket;
        args.object = objectName;

        if (!contentType.empty()) {
            args.headers.Add("Content-Type", contentType);
        }

        // Add metadata
        for (const auto& [key, value] : metadata) {
            args.user_metadata.Add("x-amz-meta-" + key, value);
        }

        // Keep the stream alive during the PutObject call
        client->PutObject(args);
        return Result<bool>::Success(true);
    } catch (const minio::error::Error& err) {
        return Result<bool>::Error("Put object failed: " + std::string(err.String()));
    }
}

Result<bool> MinioClient::getObject(const std::string& objectName, const std::string& filePath) {
    if (!initialized || !client) {
        return Result<bool>::Error("Client not initialized");
    }

    try {
        std::lock_guard<std::mutex> lock(clientMutex);

        minio::s3::GetObjectArgs args;
        args.bucket = bucket;
        args.object = objectName;

        minio::s3::GetObjectResponse resp = client->GetObject(args);

        // Write to file
        std::ofstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            return Result<bool>::Error("Cannot create file: " + filePath);
        }

        file.write(resp.data.data(), resp.data.size());
        file.close();

        return Result<bool>::Success(true);
    } catch (const minio::error::Error& err) {
        return Result<bool>::Error("Get object failed: " + std::string(err.String()));
    }
}

Result<std::vector<uint8_t>> MinioClient::getObject(const std::string& objectName) {
    if (!initialized || !client) {
        return Result<std::vector<uint8_t>>::Error("Client not initialized");
    }

    try {
        std::lock_guard<std::mutex> lock(clientMutex);

        minio::s3::GetObjectArgs args;
        args.bucket = bucket;
        args.object = objectName;

        minio::s3::GetObjectResponse resp = client->GetObject(args);

        std::vector<uint8_t> data(resp.data.begin(), resp.data.end());
        return Result<std::vector<uint8_t>>::Success(data);
    } catch (const minio::error::Error& err) {
        return Result<std::vector<uint8_t>>::Error("Get object failed: " + std::string(err.String()));
    }
}

Result<bool> MinioClient::removeObject(const std::string& objectName) {
    if (!initialized || !client) {
        return Result<bool>::Error("Client not initialized");
    }

    try {
        std::lock_guard<std::mutex> lock(clientMutex);

        minio::s3::RemoveObjectArgs args;
        args.bucket = bucket;
        args.object = objectName;

        client->RemoveObject(args);
        return Result<bool>::Success(true);
    } catch (const minio::error::Error& err) {
        return Result<bool>::Error("Remove object failed: " + std::string(err.String()));
    }
}

Result<bool> MinioClient::objectExists(const std::string& objectName) {
    if (!initialized || !client) {
        return Result<bool>::Error("Client not initialized");
    }

    try {
        std::lock_guard<std::mutex> lock(clientMutex);

        minio::s3::StatObjectArgs args;
        args.bucket = bucket;
        args.object = objectName;

        client->StatObject(args);
        return Result<bool>::Success(true);
    } catch (const minio::error::Error& err) {
        // Simplified error handling - assume object doesn't exist if we get an error
        return Result<bool>::Success(false);
    }
}




minio::error::Error MinioClient::handleMinioError(const minio::error::Error& err) {
    return err;
}

FileInfo MinioClient::convertMinioObjectToFileInfo(const minio::s3::Item& item) {
    FileInfo fileInfo;
    fileInfo.key = item.name;
    fileInfo.etag = item.etag;
    fileInfo.size = item.size;
    fileInfo.lastModified = std::chrono::system_clock::now(); // Simplified for now
    return fileInfo;
}

std::map<std::string, std::string> MinioClient::convertMinioHeaders(const std::map<std::string, std::string>& headers) {
    return headers;
}

Result<std::vector<std::string>> MinioClient::listObjects(const std::string& prefix) {
    if (!initialized || !client) {
        return Result<std::vector<std::string>>::Error("Client not initialized");
    }

    try {
        std::lock_guard<std::mutex> lock(clientMutex);

        minio::s3::ListObjectsArgs args;
        args.bucket = bucket;
        if (!prefix.empty()) {
            args.prefix = prefix;
        }

        minio::s3::ListObjectsResult result = client->ListObjects(args);
        std::vector<std::string> objectNames;

        // 遍历ListObjectsResult
        for (; result; ++result) {
            objectNames.push_back((*result).name);
        }

        return Result<std::vector<std::string>>::Success(objectNames);
    } catch (const minio::error::Error& err) {
        return Result<std::vector<std::string>>::Error("List objects failed: " + std::string(err.String()));
    }
}
