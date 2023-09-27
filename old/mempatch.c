#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <dlfcn.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/syscall.h>
#include "clibpatch.h"

int mempatch(const char* PACKAGE, uintptr_t OFFSETS[] = 0, unsigned long PATCHBYTES[] = 0) {
    int PID = getPidFromPkgName(PACKAGE);

    uintptr_t libBase = 0;

    char libPath[512];

    if (PID != -1) {
        printf("[*] Package Name: %s\n[*] PID: %d\n", PACKAGE, PID);
    } else {
        printf("[*] Package not found: %s\n", PACKAGE);
    }

    ptrace(PTRACE_ATTACH, PID, NULL, NULL);
    printf("[*] Attached to process\n");

    waitpid(PID, NULL, 0);
    memset(libPath, 0, sizeof(libPath));

    snprintf(libPath, sizeof(libPath), "/proc/%d/maps", PID);
    FILE* mapsFile = fopen(libPath, "r");

    if (mapsFile) {
        char line[256];

        while (fgets(line, sizeof(line), mapsFile)) {
            if (strstr(line, "libg.so")) {
                char* dash = strchr(line, '-');
                if (dash) {
                    *dash = '\0';
                    libBase = strtoull(line, NULL, 16);
                    break;
                }
            }
        }
        fclose(mapsFile);
    }

    // size_t numPatches = sizeof(OFFSETS) / sizeof(OFFSETS[0]);

    void patchBytes() {
        for (size_t i = 0; i < sizeof(OFFSETS) / sizeof(OFFSETS[0]); i++) {
            uintptr_t patchAddress = libBase + OFFSETS[i];

            ptrace(PTRACE_POKETEXT, PID, (void*)patchAddress, (void*)PATCHBYTES[i]);
        }
        printf("[*] Successfully patched\n");
    }

    unsigned long armHex(const char* hexString) {
        char* cleanHexString = malloc(strlen(hexString) + 1);
        int cleanIndex = 0;

        for (int i = 0; hexString[i] != '\0'; i++) {
            if (hexString[i] != ' ') {
                cleanHexString[cleanIndex++] = hexString[i];
            }
        }
        cleanHexString[cleanIndex] = '\0';
        INSTRUCTION_SET hexValue = strtoul(cleanHexString, NULL, 16);

        free(cleanHexString);
        return hexValue;
    }

    ptrace(PTRACE_DETACH, PID, NULL, NULL);
    printf("[*] Detached from process\n");
    return 0;
}