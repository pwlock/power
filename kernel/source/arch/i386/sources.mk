KERNEL_ARCH_CFILES=arch/i386/gdt.c arch/i386/idt.c arch/i386/exceptions.c arch/i386/paging.c \
				   arch/i386/timer/hpet.c arch/i386/int/apic.c arch/i386/timer/secondary.c   \
				   arch/i386/timer/apict.c arch/i386/timer/pit.c
KERNEL_ARCH_ASFILES=arch/i386/memory.s arch/i386/gdt.s arch/i386/idt.s arch/i386/task_switch.s \
					arch/i386/fet.s arch/i386/syscall.s