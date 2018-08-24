#include <taihen.h>
#include <vitasdkkern.h>
#include <string.h>

int ReadFile(const char *file, void *buf, int size) {
	SceUID fd = ksceIoOpen(file, SCE_O_RDONLY, 0);
	if (fd < 0)
	return fd;
	int read = ksceIoRead(fd, buf, size);
	ksceIoClose(fd);
	return read;
}

int WriteFile(const char *file, void *buf, int size) {
	SceUID fd = ksceIoOpen(file, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	if (fd < 0)
	return fd;
	int written = ksceIoWrite(fd, buf, size);
	ksceIoClose(fd);
	return written;
}

unsigned int pa2va(unsigned int pa) {
	unsigned int va;
	unsigned int vaddr;
	unsigned int paddr;
	unsigned int i;
	va = 0;
	for (i = 0; i < 0x100000; i++) {
		vaddr = i << 12;
		__asm__("mcr p15,0,%1,c7,c8,0\n\t"
		"mrc p15,0,%0,c7,c4,0\n\t" : "=r" (paddr) : "r" (vaddr));
		if ((pa & 0xFFFFF000) == (paddr & 0xFFFFF000)) {
			va = vaddr + (pa & 0xFFF);
			break;
		}
	}
	return va;
}