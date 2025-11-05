#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/kvm.h>
#include <stdint.h>
#include <time.h>
#include <linux/vfio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>

#define VFIO_DEVICE "/dev/vfio/vfio"
#define PCI_CONFIG_OFFSET_COMMAND 0x04
#define DEV_PATH_MAX 64

#define KVM_DEVICE "/dev/kvm"
#define RAM_SIZE (128UL << 30)
#define CODE_START 0x1000

#define SYSFS_PCI_DEVICES "/sys/bus/pci/devices"
#define MAX_PATH 1024

#define CHECK_ERR(x, msg) do { \
    if ((x) < 0) { \
        perror((msg)); \
        goto out; \
    } \
} while(0)

struct pci_device {
    char bdf[16];
    char vendor_id[8];
    char device_id[8];
    char iommu_group[64];
    char device_name[128];
};

int vfio_fd;
void *guest_mem = NULL;
int verbose = 0;

/* 
 * 1. bind vfio driver
 *    echo "cabc 05xx" > /sys/bus/pci/drivers/vfio-pci/new_id
 * 
 * 2. manual enable bus master
 *    for i in `lspci -d cabc: | cut -d ' ' -f 1`; do  
 *      sudo setpci -s $i 0x4.B=0x46;
 *    done  
 * 
 * 3. find iommu_group
 *    for i in $x;do 
 *      echo -n $i=; 
 *      readlink /sys/bus/pci/devices/0000\:$i/iommu_group | cut -d '/' -f 9; 
 *    done
 * 
 * char *pci_devs[8][2] = { 
 *     {"0000:0f:00.0","26"}, {"0000:34:00.0","45"}, {"0000:48:00.0","59"}, 
 *     {"0000:5a:00.0","71"}, {"0000:87:00.0","261"}, {"0000:ae:00.0","280"}, 
 *     {"0000:c2:00.0","292"}, {"0000:d7:00.0","307"}
 * };
 */

unsigned char guest_code[] = {
    0xba, 0xf8, 0x03, /* mov $0x3f8, %dx */  // 串口端口
    0xb0, 'H',        /* mov $'H', %al */
    0xee,             /* out %al, %dx */
    0xb0, 'e',        /* mov $'e', %al */
    0xee,             /* out %al, %dx */
    0xb0, 'l',        /* mov $'l', %al */
    0xee,             /* out %al, %dx */
    0xb0, 'l',        /* mov $'l', %al */
    0xee,             /* out %al, %dx */
    0xb0, 'o',        /* mov $'o', %al */
    0xee,             /* out %al, %dx */
    0xb0, ' ',        /* mov $' ', %al */
    0xee,             /* out %al, %dx */
    0xb0, 'K',        /* mov $'K', %al */
    0xee,             /* out %al, %dx */
    0xb0, 'V',        /* mov $'V', %al */
    0xee,             /* out %al, %dx */
    0xb0, 'M',        /* mov $'M', %al */
    0xee,             /* out %al, %dx */
    0xb0, '!',        /* mov $'!', %al */
    0xee,             /* out %al, %dx */
    0xb0, '\n',       /* mov $'\n', %al */
    0xee,             /* out %al, %dx */
    0xf4,             /* hlt - 挂起 CPU */
};

int check_kvm_version(int kvm_fd) {
    int ret = ioctl(kvm_fd, KVM_GET_API_VERSION, 0);
    if (ret == -1) {
        perror("KVM_GET_API_VERSION");
        return -1;
    }
    
    if (ret != 12) {
        fprintf(stderr, "KVM API version mismatch: expected 12, got %d\n", ret);
        return -1;
    }
    
    printf("KVM API version: %d\n", ret);
    return 0;
}

int create_vm(int kvm_fd) {
    int vm_fd = ioctl(kvm_fd, KVM_CREATE_VM, 0);
    if (vm_fd == -1) {
        perror("KVM_CREATE_VM");
        return -1;
    }
    
    printf("VM created successfully\n");
    return vm_fd;
}

int setup_guest_memory(int vm_fd, void **guest_mem) {
    *guest_mem = mmap(NULL, RAM_SIZE, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (*guest_mem == MAP_FAILED) {
        perror("mmap guest memory");
        return -1;
    }
    
    struct kvm_userspace_memory_region mem_region = {
        .slot = 0,
        .flags = 0,
        .guest_phys_addr = 0,
        .memory_size = RAM_SIZE,
        .userspace_addr = (unsigned long)*guest_mem,
    };
    
    if (ioctl(vm_fd, KVM_SET_USER_MEMORY_REGION, &mem_region) == -1) {
        perror("KVM_SET_USER_MEMORY_REGION");
        return -1;
    }
    
    printf("Guest memory setup: %ld bytes at %p\n", RAM_SIZE, *guest_mem);
    return 0;
}

int create_vcpu(int kvm_fd, int vm_fd, struct kvm_run **kvm_run) {
    int vcpu_fd = ioctl(vm_fd, KVM_CREATE_VCPU, 0);
    if (vcpu_fd == -1) {
        perror("KVM_CREATE_VCPU");
        return -1;
    }
    
    int run_size = ioctl(kvm_fd, KVM_GET_VCPU_MMAP_SIZE, 0);
    if (run_size == -1) {
        perror("KVM_GET_VCPU_MMAP_SIZE");
        return -1;
    }
    
    *kvm_run = mmap(NULL, run_size, PROT_READ | PROT_WRITE, MAP_SHARED, vcpu_fd, 0);
    if (*kvm_run == MAP_FAILED) {
        perror("mmap kvm_run");
        return -1;
    }
    
    printf("VCPU created successfully, run_size: %d\n", run_size);
    return vcpu_fd;
}

int setup_vcpu_registers(int vcpu_fd, void *guest_mem) {
    struct kvm_regs regs;
    struct kvm_sregs sregs;
    
    if (ioctl(vcpu_fd, KVM_GET_REGS, &regs) == -1) {
        perror("KVM_GET_REGS");
        return -1;
    }
    
    if (ioctl(vcpu_fd, KVM_GET_SREGS, &sregs) == -1) {
        perror("KVM_GET_SREGS");
        return -1;
    }
    
    // 设置代码段为实模式
    sregs.cs.selector = 0;
    sregs.cs.base = 0;
    
    // 设置通用寄存器
    regs.rip = CODE_START;  // 指令指针指向代码开始
    regs.rflags = 0x2;      // 设置标志位
    regs.rax = 0;
    regs.rbx = 0;
    regs.rcx = 0;
    regs.rdx = 0;
    
    if (ioctl(vcpu_fd, KVM_SET_SREGS, &sregs) == -1) {
        perror("KVM_SET_SREGS");
        return -1;
    }
    
    if (ioctl(vcpu_fd, KVM_SET_REGS, &regs) == -1) {
        perror("KVM_SET_REGS");
        return -1;
    }
    
    printf("VCPU registers initialized (RIP=0x%llx)\n", regs.rip);
    return 0;
}

void load_guest_code(void *guest_mem) {
    // memcpy(guest_mem + CODE_START, guest_code, sizeof(guest_code));
    memset(guest_mem, 0, sizeof(RAM_SIZE));
    printf("Guest code loaded at 0x%x (%zu bytes)\n", CODE_START, sizeof(guest_code));
}

void run_vm(int vcpu_fd, struct kvm_run *kvm_run) {
    printf("Starting VM execution...\n");
    
    while (1) {
        if (ioctl(vcpu_fd, KVM_RUN, 0) == -1) {
            perror("KVM_RUN");
            break;
        }
        
        switch (kvm_run->exit_reason) {
            case KVM_EXIT_HLT:
                printf("VM halted gracefully\n");
                return;
            
            case KVM_EXIT_IO:
                if (kvm_run->io.direction == KVM_EXIT_IO_OUT) {
                    printf("VM I/O output: ");
                    for (int i = 0; i < kvm_run->io.size; i++) {
                        char c = *(char*)((char*)kvm_run + kvm_run->io.data_offset + i);
                        if (c >= 32 && c <= 126) {
                            printf("%c", c);
                        }
                    }
                    printf("\n");
                }
                break;
            
            case KVM_EXIT_SHUTDOWN:
                printf("VM shutdown\n");
                return;
            
            default:
                printf("Unexpected VM exit reason: %d\n", kvm_run->exit_reason);
                return;
        }
    }
}

void cleanup(int kvm_fd, int vm_fd, int vcpu_fd,
             struct kvm_run *kvm_run, void *guest_mem) {
    if (kvm_run) {
        int run_size = ioctl(vcpu_fd, KVM_GET_VCPU_MMAP_SIZE, 0);
        if (run_size > 0) {
            munmap(kvm_run, run_size);
        }
    }
    
    if (guest_mem) {
        munmap(guest_mem, RAM_SIZE);
    }
    
    if (vcpu_fd != -1) close(vcpu_fd);
    if (vm_fd != -1) close(vm_fd);
    if (kvm_fd != -1) close(kvm_fd);
}

int read_file_content(const char *filename, char *buffer, size_t size) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        return -1;
    }
    
    if (fgets(buffer, size, file) == NULL) {
        fclose(file);
        return -1;
    }
    
    buffer[strcspn(buffer, "\n")] = '\0';
    fclose(file);
    return 0;
}

int get_pci_device_info(const char *bdf, struct pci_device *device) {
    char path[MAX_PATH];
    
    strncpy(device->bdf, bdf, sizeof(device->bdf) - 1);
    
    snprintf(path, sizeof(path), "%s/%s/vendor", SYSFS_PCI_DEVICES, bdf);
    if (read_file_content(path, device->vendor_id, sizeof(device->vendor_id)) != 0) {
        return -1;
    }
    
    if (strncmp(device->vendor_id, "0x", 2) == 0) {
        memmove(device->vendor_id, device->vendor_id + 2, strlen(device->vendor_id) - 1);
    }
    
    snprintf(path, sizeof(path), "%s/%s/device", SYSFS_PCI_DEVICES, bdf);
    if (read_file_content(path, device->device_id, sizeof(device->device_id)) != 0) {
        return -1;
    }
    
    if (strncmp(device->device_id, "0x", 2) == 0) {
        memmove(device->device_id, device->device_id + 2, strlen(device->device_id) - 1);
    }
    
    snprintf(path, sizeof(path), "%s/%s/iommu_group", SYSFS_PCI_DEVICES, bdf);
    char group_link[MAX_PATH];
    ssize_t len = readlink(path, group_link, sizeof(group_link) - 1);
    if (len != -1) {
        group_link[len] = '\0';
        char *group_name = strrchr(group_link, '/');
        if (group_name) {
            strncpy(device->iommu_group, group_name + 1, sizeof(device->iommu_group) - 1);
        } else {
            strncpy(device->iommu_group, group_link, sizeof(device->iommu_group) - 1);
        }
    } else {
        strcpy(device->iommu_group, "N/A");
    }
    
    char command[256];
    snprintf(command, sizeof(command), "lspci -s %s 2>/dev/null", bdf);
    FILE *lspci = popen(command, "r");
    if (lspci) {
        if (fgets(device->device_name, sizeof(device->device_name), lspci) != NULL) {
            device->device_name[strcspn(device->device_name, "\n")] = '\0';
        }
        pclose(lspci);
    } else {
        strcpy(device->device_name, "Unknown");
    }
    
    return 0;
}

int find_pci_devices_by_vendor(const char *target_vendor, struct pci_device **devices, int *count) {
    DIR *dir;
    struct dirent *entry;
    struct pci_device *found_devices = NULL;
    int capacity = 10;
    int found_count = 0;
    
    found_devices = malloc(capacity * sizeof(struct pci_device));
    if (!found_devices) {
        return -1;
    }
    
    dir = opendir(SYSFS_PCI_DEVICES);
    if (!dir) {
        free(found_devices);
        return -1;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        if (strlen(entry->d_name) < 7) {
            continue;
        }
        
        struct pci_device device;
        if (get_pci_device_info(entry->d_name, &device) == 0) {
            if (strcasecmp(device.vendor_id, target_vendor) == 0) {
                if (found_count >= capacity) {
                    capacity *= 2;
                    struct pci_device *new_devices = realloc(found_devices, capacity * sizeof(struct pci_device));
                    if (!new_devices) {
                        closedir(dir);
                        free(found_devices);
                        return -1;
                    }
                    found_devices = new_devices;
                }
                
                found_devices[found_count] = device;
                found_count++;
            }
        }
    }
    
    closedir(dir);
    
    *devices = found_devices;
    *count = found_count;
    return 0;
}

void print_device_info(const struct pci_device *device) {
    printf("BDF: %s\n", device->bdf);
    printf("  Vendor ID: %s\n", device->vendor_id);
    printf("  Device ID: %s\n", device->device_id);
    printf("  IOMMU Group: %s\n", device->iommu_group);
    printf("  Device: %s\n", device->device_name);
    printf("\n");
}

void print_iommu_group_devices(const char *group_id) {
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "/sys/kernel/iommu_groups/%s/devices", group_id);
    
    DIR *dir = opendir(path);
    if (!dir) {
        printf("  Cannot access IOMMU group devices\n");
        return;
    }
    
    printf("  Devices in IOMMU Group %s:\n", group_id);
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        printf("    - %s\n", entry->d_name);
    }
    
    closedir(dir);
}

int vfio_get_iommu_info(int container_fd) {
    struct vfio_iommu_type1_info *info;
    size_t argsz;
    
    argsz = sizeof(*info);
    info = malloc(argsz);
    if (!info) {
        perror("malloc failed");
        return -1;
    }
    memset(info, 0, argsz);
    info->argsz = argsz;
    
    if (ioctl(container_fd, VFIO_IOMMU_GET_INFO, info) == -1) {
        perror("VFIO_IOMMU_GET_INFO failed");
        free(info);
        return -1;
    }
    
    if (info->argsz > argsz) {
        argsz = info->argsz;
        struct vfio_iommu_type1_info *new_info = realloc(info, argsz);
        if (!new_info) {
            perror("realloc failed");
            free(info);
            return -1;
        }
        info = new_info;
        memset(info, 0, argsz);
        info->argsz = argsz;
        
        if (ioctl(container_fd, VFIO_IOMMU_GET_INFO, info) == -1) {
            perror("VFIO_IOMMU_GET_INFO failed after realloc");
            free(info);
            return -1;
        }
    }
    
    printf("IOMMU size: %u, flags: 0x%x, cap_offset: %u\n", 
           info->argsz, info->flags, info->cap_offset);
    
    /*for(int i = 0; i < info->argsz - info->cap_offset; i++) {
        printf("%02x ", *((unsigned char *)hdr + i)); 
        if (i %16 ==15) printf("\n");
    }*/
    
    if (info->flags & VFIO_IOMMU_INFO_CAPS) {
        struct vfio_info_cap_header *hdr = (struct vfio_info_cap_header *)((char *)info + info->cap_offset);
        while (hdr) {
            struct vfio_iommu_type1_info_cap_iova_range *iova_cap = (struct vfio_iommu_type1_info_cap_iova_range *)hdr;
            struct vfio_iommu_type1_info_cap_migration *cap = (struct vfio_iommu_type1_info_cap_migration *)hdr;
            struct vfio_iommu_type1_info_dma_avail *dma_cap = (struct vfio_iommu_type1_info_dma_avail *)hdr;
            
            printf("Capability ID: 0x%x, Version: %u, Next: 0x%x\n", 
                   hdr->id, hdr->version, hdr->next);
            
            switch (hdr->id) {
                case VFIO_IOMMU_TYPE1_INFO_CAP_IOVA_RANGE:
                    printf("  IOVA Capability Number of IOVA ranges: %u\n", iova_cap->nr_iovas);
                    for (int i = 0; i < iova_cap->nr_iovas; i++) {
                        printf("    Supported IOVA range %d: [0x%llx - 0x%llx]\n", 
                               i, iova_cap->iova_ranges[i].start, iova_cap->iova_ranges[i].end);
                    }
                    break;
                
                case VFIO_IOMMU_TYPE1_INFO_CAP_MIGRATION:
                    printf("  Migration Capability:\n");
                    printf("    Flags: 0x%x\n", cap->flags);
                    printf("    Page Size Bitmap: 0x%llx\n", cap->pgsize_bitmap);
                    printf("    Max Dirty Bitmap Size: 0x%llx\n", cap->max_dirty_bitmap_size);
                    break;
                
                case VFIO_IOMMU_TYPE1_INFO_DMA_AVAIL:
                    printf("  DMA Capability:\n");
                    printf("    DMA avail: %u\n", dma_cap->avail);
                    break;
                
                default:
                    printf("    Unknown capability ID: 0x%x\n", hdr->id);
                    printf("    Raw data (first 32 bytes): ");
                    for (int i = 0; i < 32 && i < hdr->next; i++) {
                        printf("%02x ", ((unsigned char *)hdr)[i]);
                    }
                    printf("\n");
                    break;
            }
            
            hdr = hdr->next ? (struct vfio_info_cap_header *)((char *)info + hdr->next) : NULL;
        }
    }
    
    free(info);
    return 0;
}

int open_pci(char *iommu_group, char *pci_bdf) {
    int device_fd = -1, group_fd = -1;
    int container_fd = -1;
    void *device_map = MAP_FAILED;
    uint8_t buf[256];
    ssize_t ret;
    struct vfio_group_status group_status = { .argsz = sizeof(group_status) };
    struct vfio_device_info device_info = { .argsz = sizeof(device_info) };
    struct vfio_region_info config_region = { .argsz = sizeof(config_region), .index = 0 };
    
    vfio_fd = open(VFIO_DEVICE, O_RDWR);
    CHECK_ERR(vfio_fd, "Failed to open VFIO container");
    
    char group_path[DEV_PATH_MAX];
    snprintf(group_path, sizeof(group_path), "/dev/vfio/%s", iommu_group);
    group_fd = open(group_path, O_RDWR);
    CHECK_ERR(group_fd, "Failed to open IOMMU group");
    
    CHECK_ERR(ioctl(group_fd, VFIO_GROUP_GET_STATUS, &group_status), "Failed to get group status");
    if (!(group_status.flags & VFIO_GROUP_FLAGS_VIABLE)) {
        fprintf(stderr, "IOMMU group is not viable\n");
        return -1;
    }
    CHECK_ERR(ioctl(group_fd, VFIO_GROUP_SET_CONTAINER, &vfio_fd), "Failed to set group container");
    
    CHECK_ERR(ioctl(vfio_fd, VFIO_SET_IOMMU, VFIO_TYPE1v2_IOMMU), "Failed to set IOMMU type");
    
    device_fd = ioctl(group_fd, VFIO_GROUP_GET_DEVICE_FD, pci_bdf);
    CHECK_ERR(device_fd, "Failed to get device fd");
    
    CHECK_ERR(ioctl(device_fd, VFIO_DEVICE_GET_INFO, &device_info), "Failed to get device info");
    if (verbose) 
        printf("Device found. Number of regions: %d. Number of irq: %d\n", 
               device_info.num_regions, device_info.num_irqs);
    
    // Supported IOVA range 0: [0x0 - 0xfedfffff]
    // Supported IOVA range 1: [0xfef00000 - 0x7fffffffffff]
    if (verbose) 
        vfio_get_iommu_info(vfio_fd);
    
    printf("Mmap iova [0x0 - 0xfee00000] + [0x100000000 - 0x%lx]  ...\n", RAM_SIZE);
    
    struct vfio_iommu_type1_dma_map map = { 
        .argsz = sizeof(map),
        .flags = VFIO_DMA_MAP_FLAG_READ | VFIO_DMA_MAP_FLAG_WRITE, 
        .vaddr = (unsigned long)guest_mem, 
        .iova = 0, 
        .size = 0xfee00000,
    };
    CHECK_ERR(ioctl(vfio_fd, VFIO_IOMMU_MAP_DMA, &map), "Failed to mmap iova");
    
    for (unsigned long j = 0x100000000; j < RAM_SIZE; j += 1024 * 1024 * 1024) {
        struct vfio_iommu_type1_dma_map map = {
            .argsz = sizeof(map),
            .flags = VFIO_DMA_MAP_FLAG_READ | VFIO_DMA_MAP_FLAG_WRITE,
            .vaddr = (unsigned long)guest_mem + j,
            .iova = j,
            .size = 1024 * 1024 * 1024,
        };
        CHECK_ERR(ioctl(vfio_fd, VFIO_IOMMU_MAP_DMA, &map), "Failed to mmap iova");
    }
    
    // 获取配置空间偏移（Region 7 通常是 PCI配置空间）。不能mmap
    if (device_info.num_regions > 0) {
        memset(&config_region, 0, sizeof(struct vfio_region_info) + sizeof(struct vfio_region_info_cap_sparse_mmap));
        config_region.argsz = sizeof(struct vfio_region_info) + sizeof(struct vfio_region_info_cap_sparse_mmap);
        config_region.index = 7;
        
        CHECK_ERR(ioctl(device_fd, VFIO_DEVICE_GET_REGION_INFO, &config_region), "Failed to get config region info");
        
        if (verbose)
            printf("PCI Config space mapped size: 0x%llx, offset: 0x%llx\n", 
                   (unsigned long long)config_region.size, (unsigned long long)config_region.offset);
        
        ret = pread(device_fd, buf, 16, (unsigned long long)config_region.offset);
        buf[4] = 0x46;
        ret = pwrite(device_fd, &buf[4], 1, (unsigned long long)config_region.offset + 4);
        
        if (ret != 1) {
            printf("Error: pci write master bus byte failed\n");
            for (int i = 0; i < 16; i++) {
                printf("%02x ", buf[i]);
            }
            printf("\n");
            goto out;
        }
        
        ret = pread(device_fd, buf, 16, (unsigned long long)config_region.offset);
        if (buf[4] != 0x46) {
            printf("Error: enable master bus failed\n");
            for (int i = 0; i < 16; i++) {
                printf("%02x ", buf[i]);
            }
            printf("\n");
        }
    }
    
    return 0;
    
out:
    return -1;
}

int main() {
    int kvm_fd = -1, vm_fd = -1, vcpu_fd = -1;
    struct kvm_run *kvm_run = NULL;
    const char *target_vendor = "cabc";
    struct pci_device *devices = NULL;
    int count = 0;
    
    printf("KVM Userspace VMM\n");
    
    kvm_fd = open(KVM_DEVICE, O_RDWR);
    if (kvm_fd == -1) {
        perror("open " KVM_DEVICE);
        fprintf(stderr, "Make sure KVM is enabled and you have permissions\n");
        return 1;
    }
    
    if (check_kvm_version(kvm_fd) != 0) {
        goto error;
    }
    
    if (ioctl(kvm_fd, KVM_CHECK_EXTENSION, KVM_CAP_USER_MEMORY) != 1) {
        fprintf(stderr, "KVM does not support user memory\n");
        goto error;
    }
    
    vm_fd = create_vm(kvm_fd);
    if (vm_fd == -1) {
        goto error;
    }
    
    if (setup_guest_memory(vm_fd, &guest_mem) != 0) {
        goto error;
    }
    
    load_guest_code(guest_mem);
    
    vcpu_fd = create_vcpu(kvm_fd, vm_fd, &kvm_run);
    if (vcpu_fd == -1) {
        goto error;
    }
    
    if (setup_vcpu_registers(vcpu_fd, guest_mem) != 0) {
        goto error;
    }
    
    printf("Searching for PCI devices with Vendor ID: %s\n", target_vendor);
    if (find_pci_devices_by_vendor(target_vendor, &devices, &count) != 0) {
        printf("Error: Failed to search for PCI devices\n");
        goto out;
    }
    
    if (count == 0) {
        printf("No PCI devices found with Vendor ID: %s\n", target_vendor);
        free(devices);
        goto out;
    }
    
    printf("Found %d device(s) with Vendor ID %s:\n", count, target_vendor);
    
    for (int j = 0; j < count; j++) {
        if (verbose) {
            print_device_info(&devices[j]);
            if (strcmp(devices[j].iommu_group, "N/A") != 0) {
                print_iommu_group_devices(devices[j].iommu_group);
            }
        }
        
        printf("Process dev%d iommu_group:%s device:%s\n", 
               j, devices[j].iommu_group, devices[j].bdf);
        open_pci(devices[j].iommu_group, devices[j].bdf);
        printf("----------------------------------------\n");
    }
    
    free(devices);
    
    printf("VFIO device enable operation completed. wait for...\n");
    // sleep(90);
    // getchar();
    
    // run_vm(vcpu_fd, kvm_run);
    printf("Checking Mem...\n");
    
    int have_fail = 0;
    long *ptr = (long *)guest_mem;
    time_t currentTime;
    struct tm *localTime;
    char tmbuf[80];
    
    while (1) {
        time(&currentTime);
        localTime = localtime(&currentTime);
        strftime(tmbuf, 80, "%Y/%m/%d_%H:%M:%S", localTime);
        printf("%s\n", tmbuf);
        
        for (long i = 0; i < RAM_SIZE / sizeof(long); i++) {
            if (*(ptr + i) != 0) {
                have_fail = 1;
                printf("Error at %lx = %lx\n", i * sizeof(long), *(ptr + i));
            }
        }
        
        if (have_fail) {
            printf("Done with MEMERROR\n");
            goto out;
        }
        
        sleep(1);
    }
    
    printf("Done.\n");
    
    cleanup(kvm_fd, vm_fd, vcpu_fd, kvm_run, guest_mem);
    return 0;
    
out:
error:
    cleanup(kvm_fd, vm_fd, vcpu_fd, kvm_run, guest_mem);
    return -1;
}
