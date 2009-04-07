#ifdef WIN32

#include <windows.h>
#include <setupapi.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>

#include "libwbfs.h"

static int read_sector(void *_handle, u32 lba, u32 count, void *buf)
{
	HANDLE *handle = (HANDLE *)_handle;
	LARGE_INTEGER large;
	DWORD read;
	u64 offset = lba;
	
	offset *= 512ULL;
	large.QuadPart = offset;

	if (SetFilePointerEx(handle, large, NULL, FILE_BEGIN) == FALSE)
	{
		fprintf(stderr, "\n\n%lld %d %p\n", offset, count, _handle);
		wbfs_error("error seeking in hd sector (read)");
		return 1;
	}
	
	read = 0;
	if (ReadFile(handle, buf, count * 512ULL, &read, NULL) == FALSE)
	{
		wbfs_error("error reading hd sector");
		return 1;
	}
	
	return 0;
}

static int write_sector(void *_handle, u32 lba, u32 count, void *buf)
{
	HANDLE *handle = (HANDLE *)_handle;
	LARGE_INTEGER large;
	DWORD written;
	u64 offset = lba;

	offset *= 512ULL;
	large.QuadPart = offset;

	if (SetFilePointerEx(handle, large, NULL, FILE_BEGIN) == FALSE)
	{
		wbfs_error("error seeking in hd sector (write)");
		return 1;
	}
	
	written = 0;
	if (WriteFile(handle, buf, count * 512ULL, &written, NULL) == FALSE)
	{
		wbfs_error("error writing hd sector");
		return 1;
	}
	
	return 0;
  
}

static void close_handle(void *handle)
{
	CloseHandle((HANDLE *)handle);
}

static int get_capacity(char *fileName, u32 *sector_size, u32 *sector_count)
{
	DISK_GEOMETRY dg;
	PARTITION_INFORMATION pi;

	DWORD bytes;
	HANDLE *handle = CreateFile(fileName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);

	if (handle == INVALID_HANDLE_VALUE)
	{
		wbfs_error("could not open drive");
		return 0;
	}
	
	if (DeviceIoControl(handle, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, &dg, sizeof(DISK_GEOMETRY), &bytes, NULL) == FALSE)
	{
		CloseHandle(handle);
		wbfs_error("could not get drive geometry");
		return 0;
	}

	*sector_size = dg.BytesPerSector;

	if (DeviceIoControl(handle, IOCTL_DISK_GET_PARTITION_INFO, NULL, 0, &pi, sizeof(PARTITION_INFORMATION), &bytes, NULL) == FALSE)
	{
		CloseHandle(handle);
		wbfs_error("could not get partition info");
		return 0;
	}

	*sector_count = (u32)(pi.PartitionLength.QuadPart / dg.BytesPerSector);
	
	CloseHandle(handle);
	return 1;
}

wbfs_t *wbfs_try_open_hd(char *driveName, int reset)
{
	wbfs_error("no direct harddrive support");
	return 0;
}

wbfs_t *wbfs_try_open_partition(char *partitionLetter, int reset)
{
	HANDLE *handle;
	char drivePath[8] = "\\\\?\\Z:";
	
	u32 sector_size, sector_count;
	
	if (strlen(partitionLetter) != 1)
	{
		wbfs_error("bad drive name");
		return NULL;
	}

	drivePath[4] = partitionLetter[0];
	
	if (!get_capacity(drivePath, &sector_size, &sector_count))
	{
		return NULL;
	}
	
	handle = CreateFile(drivePath, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);
	
	if (handle == INVALID_HANDLE_VALUE)
	{
		return NULL;
	}
	
	return wbfs_open_partition(read_sector, write_sector, close_handle, handle, sector_size, sector_count, 0, reset);
}

wbfs_t *wbfs_try_open(char *disc, char *partition, int reset)
{
	wbfs_t *p = 0;
	
	if (partition)
	{
		p = wbfs_try_open_partition(partition,reset);
	}
	
	if (!p && !reset && disc)
	{
		p = 0;
	}
	else if(!p && !reset)
	{
		p = 0;
	}

	return p;
}

#endif

