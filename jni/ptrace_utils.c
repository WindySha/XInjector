#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/elf.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>

#include "logger.h"
#include "ptrace_utils.h"
#include "utils.h"

// rest predefined
#define CPSR_T_MASK (1u << 5)

#define USE_CACHE 1


static void* cached_dlopen_addr = NULL;
static void* cached_dlsym_addr = NULL;
static void* cached_dlerror_addr = NULL;
static void* cached_mmap_addr = NULL;
static void* cached_libc_base_addr = NULL;


int xinjector_ptrace_attach(pid_t pid)
{
    int status = 0;
    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) < 0) {
        LOGE("ptraceAttach error, pid: %d, error: %s", pid, strerror(errno));
        return -1;
    }

    LOGI("ptraceAttach success");
    waitpid(pid, &status, WUNTRACED);

    return 0;
}

int xinjector_ptrace_continue(pid_t pid)
{
    if (ptrace(PTRACE_CONT, pid, NULL, NULL) < 0) {
        LOGE("ptraceContinue error, pid: %d, error: %s", pid, strerror(errno));
        return -1;
    }

    LOGI("ptraceContinue success");
    return 0;
}

int xinjector_ptrace_detach(pid_t pid)
{
    if (ptrace(PTRACE_DETACH, pid, NULL, 0) < 0) {
        LOGE("ptraceDetach error, pid: %d, err: %s", pid, strerror(errno));
        return -1;
    }

    LOGI("ptraceDetach success");
    return 0;
}

int xinjector_ptrace_getregs(pid_t pid, struct pt_regs* regs)
{
#if defined(__aarch64__)
    int regset = NT_PRSTATUS;
    struct iovec ioVec;

    ioVec.iov_base = regs;
    ioVec.iov_len = sizeof(*regs);
    if (ptrace(PTRACE_GETREGSET, pid, (void*)regset, &ioVec) < 0) {
        LOGE("ptrace_getregs: Can not get register values, io %llx, %d\n", ioVec.iov_base, ioVec.iov_len);
        return -1;
    }

    return 0;
#else
    if (ptrace(PTRACE_GETREGS, pid, NULL, regs) < 0) {
        LOGE("Get Ptrace regs error, error: %s\n", strerror(errno));
        return -1;
    }
#endif
    return 0;
}

int xinjector_ptrace_setregs(pid_t pid, struct pt_regs* regs)
{
#if defined(__aarch64__)
    int regset = NT_PRSTATUS;
    struct iovec ioVec;

    ioVec.iov_base = regs;
    ioVec.iov_len = sizeof(*regs);
    if (ptrace(PTRACE_SETREGSET, pid, (void*)regset, &ioVec) < 0) {
        LOGE("ptrace_setregs: Can not get register values");
        return -1;
    }

    return 0;
#else
    if (ptrace(PTRACE_SETREGS, pid, NULL, regs) < 0) {
        LOGE("Set Regs error, pid:%d, err:%s\n", pid, strerror(errno));
        return -1;
    }
#endif
    return 0;
}

long xinjector_ptrace_getret(struct pt_regs* regs)
{
    return regs->ARM_r0;
}

long xinjector_ptrace_getpc(struct pt_regs* regs)
{
    return regs->ARM_pc;
}

// get the dlopen address of the target process
void* xinjector_get_dlopen_addr(pid_t pid)
{
    if (USE_CACHE && cached_dlopen_addr)
        return cached_dlopen_addr;

    void* dlOpenAddress;

    if (xinjector_get_android_sdk_level() <= 23) { // Android 7
        dlOpenAddress = xinjector_get_remote_func_addr(pid, xinjector_get_linker_path(), (void*)dlopen);
    } else {
        dlOpenAddress = xinjector_get_remote_func_addr(pid, xinjector_get_libdl_path(), (void*)dlopen);
    }
    cached_dlopen_addr = dlOpenAddress;
    return dlOpenAddress;
}

//get the dlsym address of the target process
//can be used in the future to call a specific function in the injected process
void* xinjector_get_dlsym_addr(pid_t pid)
{
    if (USE_CACHE && cached_dlsym_addr)
        return cached_dlsym_addr;

    void* dlSymAddress;

    if (xinjector_get_android_sdk_level() <= 23) {
        dlSymAddress = xinjector_get_remote_func_addr(pid, xinjector_get_linker_path(), (void*)dlsym);
    } else {
        dlSymAddress = xinjector_get_remote_func_addr(pid, xinjector_get_libdl_path(), (void*)dlsym);
    }
    cached_dlsym_addr = dlSymAddress;
    return dlSymAddress;
}

//get the dlerror address of the target process
void* xinjector_get_dlerror_addr(pid_t pid)
{
    if (USE_CACHE && cached_dlerror_addr)
        return cached_dlerror_addr;

    void* dlErrorAddress;

    if (xinjector_get_android_sdk_level() <= 23) {
        dlErrorAddress = xinjector_get_remote_func_addr(pid, xinjector_get_linker_path(), (void*)dlerror);
    } else {
        dlErrorAddress = xinjector_get_remote_func_addr(pid, xinjector_get_libdl_path(), (void*)dlerror);
    }
    cached_dlerror_addr = dlErrorAddress;
    return dlErrorAddress;
}

void* xinjector_get_mmap_addr(pid_t pid)
{
    if (USE_CACHE && cached_mmap_addr)
        return cached_mmap_addr;

    cached_mmap_addr = xinjector_get_remote_func_addr(pid, xinjector_get_libc_path(), (void*)mmap);
    return cached_mmap_addr;
}

int xinjector_ptrace_read_data(pid_t pid, uint8_t* pSrcBuf, uint8_t* pDestBuf, size_t size)
{
    long nReadCount = 0;
    long nRemainCount = 0;
    uint8_t* pCurSrcBuf = pSrcBuf;
    uint8_t* pCurDestBuf = pDestBuf;
    long lTmpBuf = 0;
    long i = 0;

    nReadCount = size / sizeof(long);
    nRemainCount = size % sizeof(long);

    for (i = 0; i < nReadCount; i++) {
        lTmpBuf = ptrace(PTRACE_PEEKTEXT, pid, pCurSrcBuf, 0);
        memcpy(pCurDestBuf, (char*)(&lTmpBuf), sizeof(long));
        pCurSrcBuf += sizeof(long);
        pCurDestBuf += sizeof(long);
    }

    if (nRemainCount > 0) {
        lTmpBuf = ptrace(PTRACE_PEEKTEXT, pid, pCurSrcBuf, 0);
        memcpy(pCurDestBuf, (char*)(&lTmpBuf), nRemainCount);
    }

    return 0;
}

int xinjector_ptrace_write_data(pid_t pid, uint8_t* pWriteAddr, uint8_t* pWriteData, size_t size)
{
    long nWriteCount = 0;
    long nRemainCount = 0;
    uint8_t* pCurSrcBuf = pWriteData;
    uint8_t* pCurDestBuf = pWriteAddr;
    long lTmpBuf = 0;
    long i = 0;

    nWriteCount = size / sizeof(long);
    nRemainCount = size % sizeof(long);

    mprotect(pWriteAddr, size, PROT_READ | PROT_WRITE | PROT_EXEC);

    for (i = 0; i < nWriteCount; i++) {
        memcpy((void*)(&lTmpBuf), pCurSrcBuf, sizeof(long));
        if (ptrace(PTRACE_POKETEXT, pid, (void*)pCurDestBuf, (void*)lTmpBuf) < 0) {
            LOGE("Write Remote Memory error, MemoryAddr: 0x%lx, error:%s\n", (uintptr_t)pCurDestBuf, strerror(errno));
            return -1;
        }
        pCurSrcBuf += sizeof(long);
        pCurDestBuf += sizeof(long);
    }

    if (nRemainCount > 0) {
        lTmpBuf = ptrace(PTRACE_PEEKTEXT, pid, pCurDestBuf, NULL);
        memcpy((void*)(&lTmpBuf), pCurSrcBuf, nRemainCount);
        if (ptrace(PTRACE_POKETEXT, pid, pCurDestBuf, lTmpBuf) < 0) {
            LOGE("Write Remote Memory error, MemoryAddr: 0x%lx, err:%s\n", (uintptr_t)pCurDestBuf, strerror(errno));
            return -1;
        }
    }
    return 0;
}

static void* get_libc_base_address(pid_t pid)
{
    if (USE_CACHE && cached_libc_base_addr)
        return cached_libc_base_addr;

    void* libc_base_address = xinjector_get_module_base_addr(pid, xinjector_get_libc_path());
    cached_libc_base_addr = libc_base_address;
    return libc_base_address;
}

int xinjector_ptrace_call(pid_t pid, uintptr_t ExecuteAddr, long* parameters, long num_params, struct pt_regs* regs)
{
    int num_param_registers = 8;
#if defined(__arm__)
    num_param_registers = 4;
#endif
    int i = 0;
    for (i = 0; i < num_params && i < num_param_registers; i++) {
        regs->uregs[i] = parameters[i];
    }

    if (i < num_params) {
        regs->ARM_sp -= (num_params - i) * sizeof(long);
        if (xinjector_ptrace_write_data(pid, (uint8_t*)(regs->ARM_sp), (uint8_t*)&parameters[i], (num_params - i) * sizeof(long)) == -1)
            return -1;
    }

    regs->ARM_pc = ExecuteAddr;
    // 判断跳转的地址位[0]是否为1，如果为1，则将CPST寄存器的标志T置位，解释为Thumb代码
    if (regs->ARM_pc & 1) {
        regs->ARM_pc &= (~1u);
        regs->ARM_cpsr |= CPSR_T_MASK;
    } else {
        regs->ARM_cpsr &= ~CPSR_T_MASK;
    }

    long lr_val = 0;
    if (xinjector_get_android_sdk_level() <= 23) {
        lr_val = 0;
    } else { // Android 7.0
        static long start_ptr = 0;
        if (start_ptr == 0) {
            start_ptr = (long)get_libc_base_address(pid);
        }
        lr_val = start_ptr;
    }
    regs->ARM_lr = lr_val;

    //对于ptrace_continue运行的进程，他会在三种情况下进入暂停状态：1.下一次系统调用 2.子进程出现异常 3.子进程退出
    //将存放返回地址的lr寄存器设置为0，执行返回的时候就会发生错误，从子进程暂停
    if (xinjector_ptrace_setregs(pid, regs) == -1 || xinjector_ptrace_continue(pid) == -1) {
        return -1;
    }

    int stat = 0;
    //参数WUNTRACED表示当进程进入暂停状态后，立即返回
    waitpid(pid, &stat, WUNTRACED);

    //0xb7f表示子进程进入暂停状态
    while ((stat & 0xFF) != 0x7f) {
        if (xinjector_ptrace_continue(pid) == -1) {
            return -1;
        }
        waitpid(pid, &stat, WUNTRACED);
    }

    // 获取远程进程的寄存器值，获取call调用的返回值
    if (xinjector_ptrace_getregs(pid, regs) == -1) {
        return -1;
    }
    return 0;
}
