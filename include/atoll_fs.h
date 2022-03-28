#ifndef __atoll_fs_h
#define __atoll_fs_h

#include <Arduino.h>
#include "FS.h"

#include "atoll_serial.h"

namespace Atoll {

class Fs {
   public:
    bool mounted = false;

    virtual void setup() = 0;

    virtual fs::FS *fsp() = 0;

    void test() {
        testListDir("/", 0);
        testCreateDir("/testing");
        testListDir("/", 0);
        testRemoveDir("/testing");
        testListDir("/", 2);
        testWriteFile("/testing.txt", "Hello ");
        testAppendFile("/testing.txt", "World!");
        testReadFile("/testing.txt");
        testDeleteFile("/testing2.txt");
        testRenameFile("/testing.txt", "/testing2.txt");
        testReadFile("/testing2.txt");
        testFileIO("/testing2.txt");
        testDeleteFile("/testing2.txt");
    }

    void testListDir(const char *path, uint8_t levels) {
        log_i("Listing directory: %s", path);

        File root = fsp()->open(path);
        if (!root) {
            log_i("Failed to open directory");
            return;
        }
        if (!root.isDirectory()) {
            log_i("Not a directory");
            return;
        }

        File file = root.openNextFile();
        while (file) {
            if (file.isDirectory()) {
                log_i("Dir: %s", file.name());
                if (levels) {
                    testListDir(file.name(), levels - 1);
                }
            } else {
                log_i("File: %s, size: %d", file.name(), file.size());
            }
            file = root.openNextFile();
        }
        file.close();
        root.close();
    }

    void testCreateDir(const char *path) {
        log_i("Creating Dir: %s", path);
        if (fsp()->mkdir(path)) {
            log_i("Dir created");
        } else {
            log_i("mkdir failed");
        }
    }

    void testRemoveDir(const char *path) {
        log_i("Removing Dir: %s", path);
        if (fsp()->rmdir(path)) {
            log_i("Dir removed");
        } else {
            log_i("rmdir failed");
        }
    }

    void testReadFile(const char *path) {
        log_i("Reading file: %s", path);

        File file = fsp()->open(path);
        if (!file) {
            log_i("Failed to open file for reading");
            return;
        }

        log_i("Read from file:");
#ifdef FEATURE_SERIAL
        while (file.available()) {
            Serial.write(file.read());
        }
        Serial.println();
#else
        log_i("No FEATURE_SERIAL");
#endif
        file.close();
    }

    void testWriteFile(const char *path, const char *message) {
        log_i("Writing file: %s", path);

        File file = fsp()->open(path, FILE_WRITE);
        if (!file) {
            log_i("Failed to open file for writing");
            return;
        }
        if (file.print(message)) {
            log_i("File written");
        } else {
            log_i("Write failed");
        }
        file.close();
    }

    void testAppendFile(const char *path, const char *message) {
        log_i("Appending to file: %s", path);

        File file = fsp()->open(path, FILE_APPEND);
        if (!file) {
            log_i("Failed to open file for appending");
            return;
        }
        if (file.print(message)) {
            log_i("Message appended");
        } else {
            log_i("Append failed");
        }
        file.close();
    }

    void testRenameFile(const char *path1, const char *path2) {
        log_i("Renaming file %s to %s", path1, path2);
        if (fsp()->rename(path1, path2)) {
            log_i("File renamed");
        } else {
            log_i("Rename failed");
        }
    }

    void testDeleteFile(const char *path) {
        log_i("Deleting file: %s", path);
        if (fsp()->remove(path)) {
            log_i("File deleted");
        } else {
            log_i("Delete failed");
        }
    }

    void testFileIO(const char *path) {
        static uint8_t buf[512];
        size_t len = 0;
        ulong start = millis();

        File file = fsp()->open(path, FILE_WRITE);
        if (!file) {
            log_i("Failed to open file for writing");
            return;
        }
        for (size_t i = 0; i < 2048; i++) {
            file.write(buf, 512);
        }
        log_i("%u bytes written in %u ms", 2048 * 512, millis() - start);
        file.close();

        file = fsp()->open(path);
        if (!file) {
            log_i("Failed to open file for reading");
            return;
        }
        len = file.size();
        size_t flen = len;
        start = millis();
        while (len) {
            size_t toRead = len;
            if (toRead > 512) {
                toRead = 512;
            }
            file.read(buf, toRead);
            len -= toRead;
        }
        log_i("%u bytes read in %u ms", flen, millis() - start);
        file.close();
    }
};

}  // namespace Atoll

#endif