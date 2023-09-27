/*
* Copyright The Solar Author.
* This file is part of the Patch C++ Library.
*
* The Patch C++ Library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public License as
* published by the Free Software Foundation; either version 2.1 of the
* License, or (at your option) any later version.
*
* The Patch C++ Library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
*
* libpatch.cpp
* 9/14/2023
* Updated 9/27/2023
*/

#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ptrace.h>

#include <vector>
#include <cerrno>
#include <string>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <sstream>
#include <iostream>

using namespace std;

vector<unsigned char> buffer;

class ArmWriter
{
public:
    ArmWriter(pid_t pid, uintptr_t base);

    void putBytes(uintptr_t offset, const char* bytes);
    void putStaticBytes(const char* libpath, uintptr_t offset, const char* bytes);
    void putRet(uintptr_t offset);
    void putNop(uintptr_t offset);
    void putByte(uintptr_t offset, uint8_t byte);
    void putWord(uintptr_t offset, int16_t word);
    void putDword(uintptr_t offset, int32_t dword);
    void putLittleDword(uintptr_t offset, int32_t val);
    void putDoubleDword(uintptr_t offset, int32_t loDword, int32_t hiDword);
    void putQword(uintptr_t offset, int64_t val);
    void putString(uintptr_t offset, string s);
    void protect(uintptr_t offset, size_t size, const char* perms);
private:
    pid_t PID;
    uintptr_t libBase;
};

ArmWriter::ArmWriter(pid_t pid, uintptr_t base) : PID(pid), libBase(base)
{
}

void ArmWriter::putStaticBytes(const char* libpath, uintptr_t offset, const char* bytes)
{
    int fd = open(libpath, O_RDWR);
    struct stat libstat;
    fstat(fd, &libstat);
    void* mappaddr = mmap(NULL, libstat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mappaddr == MAP_FAILED)
    {
        perror("[!] Failed to mmap library file\n");
        close(fd);
        return;
    }
    uintptr_t start_address = (uintptr_t)mappaddr + offset;
    uintptr_t page_start = start_address & ~(4095UL);
    size_t size = (start_address - page_start) + 4;
    protect(page_start, size, "rw");
    for (int i = 0; i < 4; i++)
    {
        *(unsigned char*)(start_address + i) = strtol(bytes + (i * 2), NULL, 16);
    }
    protect(page_start, size, "rx");
    msync(mappaddr, libstat.st_size, MS_SYNC);
    munmap(mappaddr, libstat.st_size);
    close(fd);
}

void ArmWriter::putBytes(uintptr_t offset, const char* bytes)
{
    uintptr_t patchAddress = libBase + offset;
    ptrace(PTRACE_POKETEXT, PID, (void*)patchAddress, (void*)strtoul(bytes, NULL, 16));
    buffer.clear(); // Clear the vector buffer
}

void ArmWriter::putRet(uintptr_t offset)
{
    putBytes(offset, "1E FF 2F E1"); // BX LR (Arm)
}

void ArmWriter::putNop(uintptr_t offset)
{
    putBytes(offset, "00 00 00 EA"); // NOP (Arm)
}

void ArmWriter::putByte(uintptr_t offset, uint8_t val)
{
	uint8_t byte = val;
	buffer.push_back(byte);
    putBytes(offset, (const char*)buffer.data());
}

void ArmWriter::putWord(uintptr_t offset, int16_t val)
{
	for (int i = 1; i >= 0; i--) {
		int16_t word = (val >> (i * 8)) & 0xFF;
		buffer.push_back(word);
	}
    putBytes(offset, (const char*)buffer.data());
}

void ArmWriter::putDword(uintptr_t offset, int32_t val)
{
    for (int i = 3; i >= 0; i--) {
		int dword = (val >> (i * 8)) & 0xFF;
		buffer.push_back(dword);
	}
    putBytes(offset, (const char*)buffer.data());
}

void ArmWriter::putLittleDword(uintptr_t offset, int32_t val) // little-endian bytes order
{
    for (int i = 0; i <= 3; i++) {
		int32_t dword = (val >> (i * 8)) & 0xFF;
		buffer.push_back(dword);
	}
    putBytes(offset, (const char*)buffer.data());
}

void ArmWriter::putDoubleDword(uintptr_t offset, int32_t loDword, int32_t hiDword) // 2-int long, in right 4 bits we put high word, in left 4 bits we put low word
{
    putDword(offset, loDword);
    putDword(offset, hiDword + 4); // goto 4 left bits
    putBytes(offset, (const char*)buffer.data());
}

void ArmWriter::putQword(uintptr_t offset, int64_t val)
{
    for (int i = 7; i >= 0; i--) {
		int64_t qword = (val >> (i * 8)) & 0xFF;
		buffer.push_back(qword);
	}
    putBytes(offset, (const char*)buffer.data());
}

void ArmWriter::putString(uintptr_t offset, string s)
{
    if (s.empty()) {
        putDword(offset, -1);
    } else {
        for (char c : s) {
            buffer.push_back(c);
        }
        buffer.push_back('\0');
        putBytes(offset, (const char*)buffer.data());
    }
}

void ArmWriter::protect(uintptr_t offset, size_t size, const char* perms)
{
    int prot = 0;
    if (strcmp(perms, "rwx") == 0 || strcmp(perms, "777") == 0)
    {
        prot = PROT_READ | PROT_WRITE | PROT_EXEC;
    }
    else if (strcmp(perms, "rw") == 0 || strcmp(perms, "766") == 0)
    {
        prot = PROT_READ | PROT_WRITE;
    }
    else if (strcmp(perms, "rx") == 0 || strcmp(perms, "755") == 0)
    {
        prot = PROT_READ | PROT_EXEC;
    }
    else if (strcmp(perms, "r") == 0 || strcmp(perms, "444") == 0)
    {
        prot = PROT_READ;
    }
    else
    {
        cerr << "[!] ArmWriter::protect() ERROR: provide 'rwx' (777), 'rw' (766), 'rx' (755) or 'r' (444)" << endl;
        return;
    }
    mprotect((void*)offset, size, prot);
}

class ArmReader
{
public:
    ArmReader(pid_t pid, uintptr_t base);

    uint8_t readByte(uintptr_t offset);
    int16_t readWord(uintptr_t offset);
    int32_t readDword(uintptr_t offset);
    int32_t readLittleDword(uintptr_t offset);
    pair<int32_t, int32_t> readDoubleDword(uintptr_t offset);
    int64_t readQword(uintptr_t offset);
    char* readBytes(uintptr_t offset, size_t size);
    string readString(uintptr_t offset);
private:
    pid_t PID;
    uintptr_t libBase;
};

ArmReader::ArmReader(pid_t pid, uintptr_t base) : PID(pid), libBase(base)
{
}

uint8_t ArmReader::readByte(uintptr_t offset)
{
    uint8_t byte = ptrace(PTRACE_PEEKTEXT, PID, (void*)(libBase + offset), nullptr);
    return byte;
}

int16_t ArmReader::readWord(uintptr_t offset)
{
    int16_t word = 0;

    for (int i = 0; i <= 1; i++)
    {
        word |= (readByte(offset) << (8 * (3 - i)));
    }
    return word;
}

int32_t ArmReader::readDword(uintptr_t offset)
{
    int32_t dword = 0;

    for (int i = 0; i <= 3; i++)
    {
        dword |= (readByte(offset) << (8 * (3 - i)));
    }
    return dword;
}

int32_t ArmReader::readLittleDword(uintptr_t offset) // little-endian bytes order
{
    int32_t dword = 0;

    for (int i = 0; i <= 3; i++)
    {
        dword |= (readByte(offset) << (i * 8));
    }
    return dword;
}

pair<int32_t, int32_t> ArmReader::readDoubleDword(uintptr_t offset) // 2-int long, from right 4 bits we read high word, from left 4 bits we read low wor
{
    int32_t loDword = ArmReader::readDword(offset);
    int32_t hiDword = ArmReader::readDword(offset + 4); // goto 4 left bits
    return make_pair(hiDword, loDword);
}

int64_t ArmReader::readQword(uintptr_t offset)
{
    int64_t qword = 0;

    for (int i = 0; i <= 7; i++)
    {
        qword |= (readByte(offset) << (8 * (3 - i)));
    }
    return qword;
}

char* ArmReader::readBytes(uintptr_t offset, size_t size)
{
    char* data = new char[size];
    for (size_t i = 0; i < size; i++)
    {
        long byte = ptrace(PTRACE_PEEKTEXT, PID, (void*)(libBase + offset + i), nullptr);
        data[i] = (char)(byte);
    }
    return data;
}

string ArmReader::readString(uintptr_t offset)
{
    ostringstream oss;
    char* buffer = readBytes(offset, 1);
    while (*buffer != '\0')
    {
        oss << *buffer;
        offset++;
        buffer = readBytes(offset, 1);
    }
    delete[] buffer;
    return oss.str();
}

int getAllMaps(char* buf, size_t bufSize)
{
    int fd = open("/proc/self/maps", O_RDONLY);
    ssize_t bytesRead = read(fd, buf, bufSize - 1);
    close(fd);

    if (bytesRead >= 0) {
        buf[bytesRead] = '\0';
    }
    return bytesRead;
}

void getMapByName(const char* lib)
{
    char buf[4096];
    ssize_t bytesRead = getAllMaps(buf, sizeof(buf));
    char* line = buf;
    while (*line != '\0')
    {
        if (strstr(line, lib) != nullptr)
        {
            cout << "[*] Lib loaded in /proc/self/maps" << endl;
            break;
        }
        line = strchr(line, '\n');
        if (line == nullptr)
        {
            break;
        }
        line++;
    }
}

void getProcesses()
{
    DIR* dir = opendir("/data/app");
    dirent* entry;
    while ((entry = readdir(dir)))
    {
        cout << "[*] Base apk process: " << entry->d_name << endl;
    }
    closedir(dir);
}

int getLoginUid()
{
    int fd = open("/proc/self/loginuid", O_RDONLY);
    char buf[4096];
    ssize_t bytesRead = read(fd, buf, sizeof(buf) - 1);
    buf[bytesRead] = '\0';
    close(fd);
    return atoi(buf);
}

int getPidFromPkgName(const char* PACKAGE)
{
    char path[512];
    DIR* dir = opendir("/proc");
    struct dirent* ent;
    while ((ent = readdir(dir)) != nullptr)
    {
        if (isdigit(*ent->d_name))
        {
            snprintf(path, sizeof(path), "/proc/%s/cmdline", ent->d_name);
            FILE* cmdline = fopen(path, "r");
            if (cmdline)
            {
                char cmdline_content[512];
                if (fgets(cmdline_content, sizeof(cmdline_content), cmdline))
                {
                    cmdline_content[strcspn(cmdline_content, "\n")] = '\0';
                    if (strcmp(cmdline_content, PACKAGE) == 0)
                    {
                        fclose(cmdline);
                        closedir(dir);
                        return atoi(ent->d_name);
                    }
                }
                fclose(cmdline);
            }
        }
    }
    closedir(dir);
    return -1;
}

int attachTo(const char* PKG, const char* baselib)
{
    int PID = getPidFromPkgName(PKG);
    uintptr_t libBase = 0;
    char libPath[512];
    if (PID != -1)
    {
        cout << "[*] Package Name: " << PKG << endl << "[*] PID: " << PID << endl;
    }
    else
    {
        cout << "[*] Package not found: " << PKG << endl;
    }
    ptrace(PTRACE_ATTACH, PID, nullptr, nullptr);
    cout << "[*] Attached to process " << PKG << endl;
    waitpid(PID, nullptr, 0);
    memset(libPath, 0, sizeof(libPath));
    snprintf(libPath, sizeof(libPath), "/proc/self/maps");
    FILE* mapsFile = fopen(libPath, "r");
    if (mapsFile)
    {
        char line[256];
        while (fgets(line, sizeof(line), mapsFile))
        {
            if (strstr(line, baselib))
            {
                char* dash = strchr(line, '-');
                if (dash)
                {
                    *dash = '\0';
                    libBase = strtoull(line, nullptr, 16);
                    break;
                }
            }
        }
        fclose(mapsFile);
    }
    return PID;
}

int detachFrom(int PID)
{
    ptrace(PTRACE_DETACH, PID, nullptr, nullptr);
    cout << "[*] Detached from process" << endl;
    return 0;
}