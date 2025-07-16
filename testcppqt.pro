QT += core gui widgets

# -----------------------------------
# 编译配置：强制使用 Release 模式（避免 _ITERATOR_DEB11UG_LEVEL 冲突）
# -----------------------------------
CONFIG -= debug
CONFIG += release
QMAKE_CXXFLAGS += /std:c++17
QMAKE_CFLAGS_RELEASE += /MD
QMAKE_CXXFLAGS_RELEASE += /MD

# -----------------------------------
# 本地头文件路径
# -----------------------------------
INCLUDEPATH += $$PWD/include
INCLUDEPATH += $$OUT_PWD/../include

# -----------------------------------
# vcpkg 路径配置（请根据本地路径调整）
# -----------------------------------
VCPKG_INC = D:/vcpkg/vcpkg/installed/x64-windows/include
VCPKG_LIB = D:/vcpkg/vcpkg/installed/x64-windows/lib
INCLUDEPATH += $$VCPKG_INC
LIBS += \
    $$VCPKG_LIB/libmariadb.lib \
    $$VCPKG_LIB/libssl.lib \
    $$VCPKG_LIB/libcrypto.lib \
    $$VCPKG_LIB/zlib.lib \
    $$VCPKG_LIB/miniocpp.lib \
    $$VCPKG_LIB/libcurl.lib \
    $$VCPKG_LIB/curlpp.lib \
    $$VCPKG_LIB/pugixml.lib \
    $$VCPKG_LIB/inih.lib \
    $$VCPKG_LIB/INIReader.lib \
    ws2_32.lib

# -----------------------------------
# 宏定义（部分库会用到）
# -----------------------------------
DEFINES += _CRT_SECURE_NO_WARNINGS
DEFINES += VCPKG_APPLOCAL_DEPS

# -----------------------------------
# 源文件
# -----------------------------------
SOURCES += \
    main.cpp \
    mainwindow.cpp \
    src/AuthManager.cpp \
    src/DatabaseManager.cpp \
    src/RedisManager.cpp \
    src/MinioClient.cpp \
    src/CLIHandler.cpp \
    src/Logger.cpp \
    src/ConfigManager.cpp \
    src/ImportExportManager.cpp \
    src/Utils.cpp \
    src/linenoise.cpp \
    src/ConvertUTF.cpp \
    src/wcwidth.cpp \
    src/DocListDialog.cpp

# -----------------------------------
# 头文件
# -----------------------------------
HEADERS += \
    include/AuthManager.h \
    include/CLIHandler.h \
    include/Common.h \
    include/ConfigManager.h \
    include/DatabaseManager.h \
    include/ImportExportManager.h \
    include/Logger.h \
    include/MinioClient.h \
    include/RedisManager.h \
    include/linenoise.h \
    include/DocListDialog.h \
    mainwindow.h

# -----------------------------------
# UI 文件
# -----------------------------------
FORMS += mainwindow.ui


CONFIG_FILES += config/config.json

# -----------------------------------
# 安装路径（可忽略）
# -----------------------------------
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# -----------------------------------
# 配置文件打包
# -----------------------------------
CONFIG_FILES += config/config.json
