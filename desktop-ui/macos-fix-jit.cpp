// macos-fix-jit - Fix Ares JIT support on macOS with old linkers
//
// Ares JIT requires the data segment to be executable (see 
// https://github.com/ares-emulator/ares/pull/200 for details on why
// this is done instead of allocating a new segment via mmap).
//
// To do this, it uses mprotect() (bump_allocator::resize), but it also
// needs to tell the linker that the data segment can be upgraded to executable
// with -Wl,-segprot,__DATA,rwx,rw (nall/GNUmakefile). Unfortunately, for quite
// some time, the clang LD had a bug and didn't accept that option for the
// data segment. The bug was fixed in Xcode 11.4.1.
//
// The official CI has a newer linker without the bug, but for developers compiling
// ares with an old toolchain, this small tool will patch the binary to
// mark the data segment as potentially executable.

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define VM_PROT_R  1
#define VM_PROT_W  2
#define VM_PROT_X  4

bool is_64bit = false;
bool is_le = false;

uint32_t read32(FILE *f) {
	if (is_le) {	
		uint32_t x = fgetc(f);
		x |= fgetc(f) << 8;
		x |= fgetc(f) << 16;
		x |= fgetc(f) << 24;
		return x;
	} else {
		uint32_t x = fgetc(f) << 24;
		x |= fgetc(f) << 16;
		x |= fgetc(f) << 8;
		x |= fgetc(f);
		return x;
	}
}

void write32(FILE *f, uint32_t x) {
	if (is_le) {
		fputc(x, f); fputc(x>>8, f); fputc(x>>16, f); fputc(x>>24, f);
	} else {
		fputc(x>>24, f); fputc(x>>16, f); fputc(x>>8, f); fputc(x, f);
	}
}

int main(int argc, char *argv[]) {
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <ares-binary>\n", argv[0]);
		return 1;
	}

	FILE *f = fopen(argv[1], "r+w");
	if (!f) {
		fprintf(stderr, "cannot open file: %s\n", argv[1]);
		return 1;
	}

	uint32_t magic = read32(f);
	switch (magic) {
	case 4277009102: is_64bit = false; is_le = false; break;
	case 4277009103: is_64bit = true;  is_le = false; break;
	case 3472551422: is_64bit = false; is_le = true;  break;
	case 3489328638: is_64bit = true;  is_le = true;  break;
	case 3405691582: fprintf(stderr, "universal binary is not supported"); return 1;
	default: fprintf(stderr, "invalid MachO magic number: %u\n", magic); return 1;
	}

	fseek(f, 3*4, SEEK_CUR);
	uint32_t nlcs = read32(f);
	fseek(f, (2+is_64bit)*4, SEEK_CUR);

	if (nlcs > 1024) {
		fprintf(stderr, "too many MachO load commands, header corrupted? (%08x)", nlcs);
		return 1;
	}

	for (int i=0; i<nlcs; i++) {
		uint32_t cmd = read32(f);
		uint32_t sz = read32(f);
		int next = ftell(f) + sz - 8;
	
		// Search for the command LC_SEGMENT_64 on data segment
		if (cmd == 0x19) {
			char segname[17] = {0};
			fread(segname, 1, 16, f);
			if (!strcmp(segname, "__DATA")) {
				fseek(f, 4*(1+is_64bit)*4, SEEK_CUR);

				uint32_t maxprot = read32(f);
				switch (maxprot) {
				case VM_PROT_R | VM_PROT_W | VM_PROT_X:
					return 0; // binary already patched, nothing to do
				case VM_PROT_R | VM_PROT_W:
					// Tell the kernel that the binary can upgrade __DATA to +x
					maxprot |= VM_PROT_X;
					fseek(f, -4, SEEK_CUR);
					write32(f, maxprot);
					fclose(f);
					return 0;
				default:
					fprintf(stderr, "unexpected maxprot for __DATA segment: %x\n", maxprot);
					return 1;
				}
			}
		}

		fseek(f, next, SEEK_SET);
	}

	fprintf(stderr, "cannot find LC_SEGMENT_64 for __DATA segment\n");
	return 0;
}
