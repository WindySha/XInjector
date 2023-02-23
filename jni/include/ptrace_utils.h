#pragma once

#include <stdio.h>
#include <sys/ptrace.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__aarch64__) //x64
#define pt_regs user_pt_regs
#define uregs regs
#define ARM_pc pc
#define ARM_sp sp
#define ARM_cpsr pstate
#define ARM_lr regs[30]
#define ARM_r0 regs[0]
#define PTRACE_GETREGS PTRACE_GETREGSET
#define PTRACE_SETREGS PTRACE_SETREGSET
#endif

//Attach ptrace to process using the processID
int xinjector_ptrace_attach(pid_t pid);

//Used to the attached process doesn't stop
int xinjector_ptrace_continue(pid_t pid);

//Detach from the target process
int xinjector_ptrace_detach(pid_t pid);

//get the register value of the process
int xinjector_ptrace_getregs(pid_t pid, struct pt_regs* regs);

//set the register value of the process
int xinjector_ptrace_setregs(pid_t pid, struct pt_regs* regs);

//get the value stored in the corresponding register
long xinjector_ptrace_getret(struct pt_regs* regs);

// get the pc anddress
long xinjector_ptrace_getpc(struct pt_regs* regs);

// get remote dlopen function address
void* xinjector_get_dlopen_addr(pid_t pid);

// get remote dlsym function address
void* xinjector_get_dlsym_addr(pid_t pid);

// get remote dlerror function address
void* xinjector_get_dlerror_addr(pid_t pid);

//get the mmap address of the target process
void* xinjector_get_mmap_addr(pid_t pid);

//read data from the target process
int xinjector_ptrace_read_data(pid_t pid, uint8_t* pSrcBuf, uint8_t* pDestBuf, size_t size);

//write data to the target process
int xinjector_ptrace_write_data(pid_t pid, uint8_t* pWriteAddr, uint8_t* pWriteData, size_t size);

//call a function on the target process
int xinjector_ptrace_call(pid_t pid, uintptr_t ExecuteAddr, long* parameters, long num_params, struct pt_regs* regs);

#ifdef __cplusplus
}
#endif