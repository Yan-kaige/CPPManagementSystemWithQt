#pragma once

#include "Common.h"
#include <miniocpp/client.h>
#include <miniocpp/error.h>

struct FileInfo {
    std::string key;
    std::string etag;
    size_t size;
    std::string contentType;
    std::chrono::system_clock::time_point lastModified;
    std::map<std::string, std::string> metadata;
};

struct UploadProgress {
    size_t bytesUploaded;
    size_t totalBytes;
    double percentage;
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point currentTime;
};

// MinIO S3-compatible client using minio-cpp
class MinioClient {
private:
    std::string endpoint;
    std::string accessKey;
    std::string secretKey;
    std::string bucket;
    bool useSSL;

    std::unique_ptr<minio::s3::Client> client;
    std::unique_ptr<minio::creds::StaticProvider> credentialsProvider;
    bool initialized;
    std::mutex clientMutex;

    // Progress callback function
    std::function<void(const UploadProgress&)> progressCallback;

public:
    MinioClient();
    ~MinioClient();

    bool initialize(const std::string& endpoint, const std::string& accessKey,
                    const std::string& secretKey, const std::string& bucket, bool useSSL = false);
    void cleanup();
    bool isInitialized() const { return initialized; }

    // Bucket operations
    Result<bool> bucketExists();
    Result<bool> makeBucket();
    Result<bool> removeBucket();
    Result<std::vector<std::string>> listBuckets();

    // Object operations
    Result<bool> putObject(const std::string& objectName, const std::string& filePath,
                           const std::string& contentType = "application/octet-stream",
                           const std::map<std::string, std::string>& metadata = {});
    Result<bool> putObject(const std::string& objectName, const std::vector<uint8_t>& data,
                           const std::string& contentType = "application/octet-stream",
                           const std::map<std::string, std::string>& metadata = {});
    Result<bool> getObject(const std::string& objectName, const std::string& filePath);
    Result<std::vector<uint8_t>> getObject(const std::string& objectName);
    Result<bool> removeObject(const std::string& objectName);
    Result<bool> objectExists(const std::string& objectName);
    Result<std::vector<std::string>> listObjects(const std::string& prefix = "");

private:
    // Helper methods for minio-cpp integration
    minio::error::Error handleMinioError(const minio::error::Error& err);
    FileInfo convertMinioObjectToFileInfo(const minio::s3::Item& item);
    std::map<std::string, std::string> convertMinioHeaders(const std::map<std::string, std::string>& headers);
};