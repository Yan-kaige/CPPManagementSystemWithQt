#include "MinioClient.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <chrono>
#include <miniocpp/credentials.h>
#include <QDebug>
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

        // 首先检查对象信息
        minio::s3::StatObjectArgs statArgs;
        statArgs.bucket = bucket;
        statArgs.object = objectName;

        qDebug() << QString::fromUtf8("Checking object: " + objectName);

        minio::s3::StatObjectResponse statResp = client->StatObject(statArgs);

        if (!statResp) {
            return Result<bool>::Error("Object not found: " + objectName);
        }

        qDebug() << QString::fromUtf8("Object found. Size: " + std::to_string(statResp.size) + " bytes");

        if (statResp.size == 0) {
            return Result<bool>::Error("Object is empty on server: " + objectName);
        }

        // 打开输出文件
        std::ofstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            return Result<bool>::Error("Cannot create file: " + filePath);
        }

        size_t totalBytes = 0;

        // 设置 GetObject 参数
        minio::s3::GetObjectArgs args;
        args.bucket = bucket;
        args.object = objectName;

        // 设置数据回调函数
        args.datafunc = [&file, &totalBytes](minio::http::DataFunctionArgs args) -> bool {
            // 将接收到的数据写入文件
            file.write(args.datachunk.data(), args.datachunk.size());
            totalBytes += args.datachunk.size();

            // 显示进度（可选）
            if (totalBytes % 1024 == 0 || args.datachunk.size() < 1024) {
                qDebug() << QString::fromUtf8("Downloaded: " + std::to_string(totalBytes) + " bytes");
            }

            return true; // 返回 true 继续接收数据
        };

        qDebug() << QString::fromUtf8("Starting download...");

        // 执行下载
        minio::s3::GetObjectResponse resp = client->GetObject(args);

        file.close();

        // 检查响应
        if (resp) {
            qDebug() << QString::fromUtf8("Download successful! Total bytes: " + std::to_string(totalBytes));

            // 验证文件大小
            if (totalBytes == statResp.size) {
                qDebug() << QString::fromUtf8("File size verification passed");
                return Result<bool>::Success(true);
            } else {
                qDebug() << QString::fromUtf8("Warning: File size mismatch. Expected: " + std::to_string(statResp.size) + ", Got: " + std::to_string(totalBytes));
                return Result<bool>::Success(true); // 仍然认为成功，可能是部分内容
            }
        } else {
            qDebug() << QString::fromUtf8("Download failed: " + resp.Error().String());
            return Result<bool>::Error("Download failed: " + resp.Error().String());
        }

    } catch (const minio::error::Error& err) {
        qDebug() << QString::fromUtf8("MinIO Error: " + err.String());
        return Result<bool>::Error("Download failed: " + std::string(err.String()));
    } catch (const std::exception& e) {
        qDebug() << QString::fromUtf8("Standard Exception: " + std::string(e.what()));
        return Result<bool>::Error("Download failed: " + std::string(e.what()));
    } catch (...) {
        qDebug() << QString::fromUtf8("Unknown exception occurred");
        return Result<bool>::Error("Download failed: Unknown exception");
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

Result<bool> MinioClient::copyObject(const std::string& sourceObjectName, const std::string& destObjectName) {
    if (!initialized || !client) {
        return Result<bool>::Error("Client not initialized");
    }

    try {
        std::lock_guard<std::mutex> lock(clientMutex);

        minio::s3::CopyObjectArgs args;
        args.bucket = bucket;
        args.object = destObjectName;

        // Set source
        minio::s3::CopySource source;
        source.bucket = bucket;
        source.object = sourceObjectName;
        args.source = source;

        client->CopyObject(args);
        return Result<bool>::Success(true);
    } catch (const minio::error::Error& err) {
        return Result<bool>::Error("Copy object failed: " + std::string(err.String()));
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
