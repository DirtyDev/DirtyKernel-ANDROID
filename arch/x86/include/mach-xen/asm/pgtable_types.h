#ifndef _ASM_X86_PGTABLE_DEFS_H
#define _ASM_X86_PGTABLE_DEFS_H

#include <linux/const.h>
#include <asm/page_types.h>

#define FIRST_USER_ADDRESS	0

#define _PAGE_BIT_PRESENT	0	/* is present */
#define _PAGE_BIT_RW		1	/* writeable */
#define _PAGE_BIT_USER		2	/* userspace addressable */
#define _PAGE_BIT_PWT		3	/* page write through */
#define _PAGE_BIT_PCD		4	/* page cache disabled */
#define _PAGE_BIT_ACCESSED	5	/* was accessed (raised by CPU) */
#define _PAGE_BIT_DIRTY		6	/* was written to (raised by CPU) */
#define _PAGE_BIT_PSE		7	/* 4 MB (or 2MB) page */
#define _PAGE_BIT_PAT		7	/* on 4KB pages */
#define _PAGE_BIT_GLOBAL	8	/* Global TLB entry PPro+ */
#define _PAGE_BIT_UNUSED1	9	/* available for programmer */
#define _PAGE_BIT_IOMAP		10	/* flag used to indicate IO mapping */
#define _PAGE_BIT_HIDDEN	11	/* hidden by kmemcheck */
#define _PAGE_BIT_PAT_LARGE	12	/* On 2MB or 1GB pages */
#define _PAGE_BIT_SPECIAL	_PAGE_BIT_UNUSED1
#define _PAGE_BIT_CPA_TEST	_PAGE_BIT_UNUSED1
#define _PAGE_BIT_SPLITTING	_PAGE_BIT_UNUSED1 /* only valid on a PSE pmd */
#define _PAGE_BIT_NX           63       /* No execute: only valid after cpuid check */

/* If _PAGE_BIT_PRESENT is clear, we use these: */
/* - if the user mapped it with PROT_NONE; pte_present gives true */
#define _PAGE_BIT_PROTNONE	_PAGE_BIT_GLOBAL
/* - set: nonlinear file mapping, saved PTE; unset:swap */
#define _PAGE_BIT_FILE		_PAGE_BIT_DIRTY

#define _PAGE_PRESENT	(_AT(pteval_t, 1) << _PAGE_BIT_PRESENT)
#define _PAGE_RW	(_AT(pteval_t, 1) << _PAGE_BIT_RW)
#define _PAGE_USER	(_AT(pteval_t, 1) << _PAGE_BIT_USER)
#define _PAGE_PWT	(_AT(pteval_t, 1) << _PAGE_BIT_PWT)
#define _PAGE_PCD	(_AT(pteval_t, 1) << _PAGE_BIT_PCD)
#define _PAGE_ACCESSED	(_AT(pteval_t, 1) << _PAGE_BIT_ACCESSED)
#define _PAGE_DIRTY	(_AT(pteval_t, 1) << _PAGE_BIT_DIRTY)
#define _PAGE_PSE	(_AT(pteval_t, 1) << _PAGE_BIT_PSE)
#define _PAGE_GLOBAL	(_AT(pteval_t, 1) << _PAGE_BIT_GLOBAL)
#define _PAGE_UNUSED1	(_AT(pteval_t, 1) << _PAGE_BIT_UNUSED1)
#define _PAGE_IOMAP	(_AT(pteval_t, 1) << _PAGE_BIT_IOMAP)
#define _PAGE_PAT	(_AT(pteval_t, 1) << _PAGE_BIT_PAT)
#define _PAGE_PAT_LARGE (_AT(pteval_t, 1) << _PAGE_BIT_PAT_LARGE)
#define _PAGE_SPECIAL	(_AT(pteval_t, 1) << _PAGE_BIT_SPECIAL)
#define _PAGE_CPA_TEST	(_AT(pteval_t, 1) << _PAGE_BIT_CPA_TEST)
#define _PAGE_SPLITTING	(_AT(pteval_t, 1) << _PAGE_BIT_SPLITTING)
#define __HAVE_ARCH_PTE_SPECIAL

#ifdef CONFIG_KMEMCHECK
#define _PAGE_HIDDEN	(_AT(pteval_t, 1) << _PAGE_BIT_HIDDEN)
#else
#define _PAGE_HIDDEN	(_AT(pteval_t, 0))
#endif

#if defined(CONFIG_X86_64) || defined(CONFIG_X86_PAE)
#define _PAGE_NX	(_AT(pteval_t, 1) << _PAGE_BIT_NX)
#else
#define _PAGE_NX	(_AT(pteval_t, 0))
#endif

#define _PAGE_FILE	(_AT(pteval_t, 1) << _PAGE_BIT_FILE)
#define _PAGE_PROTNONE	(_AT(pteval_t, 1) << _PAGE_BIT_PROTNONE)

#ifndef __ASSEMBLY__
#if defined(CONFIG_X86_64) && CONFIG_XEN_COMPAT <= 0x030002
extern unsigned int __kernel_page_user;
#else
#define __kernel_page_user 0
#endif
#endif

#define _PAGE_TABLE	(_PAGE_PRESENT | _PAGE_RW | _PAGE_USER |	\
			 _PAGE_ACCESSED | _PAGE_DIRTY)
#define _KERNPG_TABLE	(_PAGE_PRESENT | _PAGE_RW | _PAGE_ACCESSED |	\
			 _PAGE_DIRTY | __kernel_page_user)

/* Set of bits not changed in pte_modify */
#define _PAGE_CHG_MASK	(PTE_PFN_MASK | _PAGE_CACHE_MASK | _PAGE_IOMAP | \
			 _PAGE_SPECIAL | _PAGE_ACCESSED | _PAGE_DIRTY)
#define _HPAGE_CHG_MASK (_PAGE_CHG_MASK | _PAGE_PSE)

/*
 * PAT settings are part of the hypervisor interface, which sets the
 * MSR to 0x050100070406 (i.e. WB, WT, UC-, UC, WC, WP [, UC, UC]).
 */
#define _PAGE_CACHE_MASK	(_PAGE_PCD | _PAGE_PWT | _PAGE_PAT)
#define _PAGE_CACHE_WB		(0)
#define _PAGE_CACHE_WT		(_PAGE_PWT)
#define _PAGE_CACHE_WC		(_PAGE_PAT)
#define _PAGE_CACHE_WP		(_PAGE_PAT | _PAGE_PWT)
#define _PAGE_CACHE_UC_MINUS	(_PAGE_PCD)
#define _PAGE_CACHE_UC		(_PAGE_PCD | _PAGE_PWT)

#define PAGE_NONE	__pgprot(_PAGE_PROTNONE | _PAGE_ACCESSED)
#define PAGE_SHARED	__pgprot(_PAGE_PRESENT | _PAGE_RW | _PAGE_USER | \
				 _PAGE_ACCESSED | _PAGE_NX)

#define PAGE_SHARED_EXEC	__pgprot(_PAGE_PRESENT | _PAGE_RW |	\
					 _PAGE_USER | _PAGE_ACCESSED)
#define PAGE_COPY_NOEXEC	__pgprot(_PAGE_PRESENT | _PAGE_USER |	\
					 _PAGE_ACCESSED | _PAGE_NX)
#define PAGE_COPY_EXEC		__pgprot(_PAGE_PRESENT | _PAGE_USER |	\
					 _PAGE_ACCESSED)
#define PAGE_COPY		PAGE_COPY_NOEXEC
#define PAGE_READONLY		__pgprot(_PAGE_PRESENT | _PAGE_USER |	\
					 _PAGE_ACCESSED | _PAGE_NX)
#define PAGE_READONLY_EXEC	__pgprot(_PAGE_PRESENT | _PAGE_USER |	\
					 _PAGE_ACCESSED)

#define __PAGE_KERNEL_EXEC						\
	(_PAGE_PRESENT | _PAGE_RW | _PAGE_DIRTY | _PAGE_ACCESSED | __kernel_page_user)
#define __PAGE_KERNEL		(__PAGE_KERNEL_EXEC | _PAGE_NX)

#define __PAGE_KERNEL_RO		(__PAGE_KERNEL & ~_PAGE_RW)
#define __PAGE_KERNEL_RX		(__PAGE_KERNEL_EXEC & ~_PAGE_RW)
#define __PAGE_KERNEL_EXEC_NOCACHE	(__PAGE_KERNEL_EXEC | _PAGE_PCD | _PAGE_PWT)
#define __PAGE_KERNEL_WC		(__PAGE_KERNEL | _PAGE_CACHE_WC)
#define __PAGE_KERNEL_NOCACHE		(__PAGE_KERNEL | _PAGE_PCD | _PAGE_PWT)
#define __PAGE_KERNEL_UC_MINUS		(__PAGE_KERNEL | _PAGE_PCD)
#define __PAGE_KERNEL_VSYSCALL		(__PAGE_KERNEL_RX | _PAGE_USER)
#define __PAGE_KERNEL_VSYSCALL_NOCACHE	(__PAGE_KERNEL_VSYSCALL | _PAGE_PCD | _PAGE_PWT)
#define __PAGE_KERNEL_LARGE		(__PAGE_KERNEL | _PAGE_PSE)
#define __PAGE_KERNEL_LARGE_NOCACHE	(__PAGE_KERNEL | _PAGE_CACHE_UC | _PAGE_PSE)
#define __PAGE_KERNEL_LARGE_EXEC	(__PAGE_KERNEL_EXEC | _PAGE_PSE)

#define __PAGE_KERNEL_IO		(__PAGE_KERNEL | _PAGE_IOMAP)
#define __PAGE_KERNEL_IO_NOCACHE	(__PAGE_KERNEL_NOCACHE | _PAGE_IOMAP)
#define __PAGE_KERNEL_IO_UC_MINUS	(__PAGE_KERNEL_UC_MINUS | _PAGE_IOMAP)
#define __PAGE_KERNEL_IO_WC		(__PAGE_KERNEL_WC | _PAGE_IOMAP)

#define PAGE_KERNEL			__pgprot(__PAGE_KERNEL)
#define PAGE_KERNEL_RO			__pgprot(__PAGE_KERNEL_RO)
#define PAGE_KERNEL_EXEC		__pgprot(__PAGE_KERNEL_EXEC)
#define PAGE_KERNEL_RX			__pgprot(__PAGE_KERNEL_RX)
#define PAGE_KERNEL_WC			__pgprot(__PAGE_KERNEL_WC)
#define PAGE_KERNEL_NOCACHE		__pgprot(__PAGE_KERNEL_NOCACHE)
#define PAGE_KERNEL_UC_MINUS		__pgprot(__PAGE_KERNEL_UC_MINUS)
#define PAGE_KERNEL_EXEC_NOCACHE	__pgprot(__PAGE_KERNEL_EXEC_NOCACHE)
#define PAGE_KERNEL_LARGE		__pgprot(__PAGE_KERNEL_LARGE)
#define PAGE_KERNEL_LARGE_NOCACHE	__pgprot(__PAGE_KERNEL_LARGE_NOCACHE)
#define PAGE_KERNEL_LARGE_EXEC		__pgprot(__PAGE_KERNEL_LARGE_EXEC)
#define PAGE_KERNEL_VSYSCALL		__pgprot(__PAGE_KERNEL_VSYSCALL)
#define PAGE_KERNEL_VSYSCALL_NOCACHE	__pgprot(__PAGE_KERNEL_VSYSCALL_NOCACHE)

#define PAGE_KERNEL_IO			__pgprot(__PAGE_KERNEL_IO)
#define PAGE_KERNEL_IO_NOCACHE		__pgprot(__PAGE_KERNEL_IO_NOCACHE)
#define PAGE_KERNEL_IO_UC_MINUS		__pgprot(__PAGE_KERNEL_IO_UC_MINUS)
#define PAGE_KERNEL_IO_WC		__pgprot(__PAGE_KERNEL_IO_WC)

/*         xwr */
#define __P000	PAGE_NONE
#define __P001	PAGE_READONLY
#define __P010	PAGE_COPY
#define __P011	PAGE_COPY
#define __P100	PAGE_READONLY_EXEC
#define __P101	PAGE_READONLY_EXEC
#define __P110	PAGE_COPY_EXEC
#define __P111	PAGE_COPY_EXEC

#define __S000	PAGE_NONE
#define __S001	PAGE_READONLY
#define __S010	PAGE_SHARED
#define __S011	PAGE_SHARED
#define __S100	PAGE_READONLY_EXEC
#define __S101	PAGE_READONLY_EXEC
#define __S110	PAGE_SHARED_EXEC
#define __S111	PAGE_SHARED_EXEC

/*
 * early identity mapping  pte attrib macros.
 */
#ifdef CONFIG_X86_64
#define __PAGE_KERNEL_IDENT_LARGE_EXEC	__PAGE_KERNEL_LARGE_EXEC
#else
/*
 * For PDE_IDENT_ATTR include USER bit. As the PDE and PTE protection
 * bits are combined, this will alow user to access the high address mapped
 * VDSO in the presence of CONFIG_COMPAT_VDSO
 */
#define PTE_IDENT_ATTR	 0x003		/* PRESENT+RW */
#define PDE_IDENT_ATTR	 0x067		/* PRESENT+RW+USER+DIRTY+ACCESSED */
#define PGD_IDENT_ATTR	 0x001		/* PRESENT (no other attributes) */
#endif

#ifdef CONFIG_X86_32
# include <asm/pgtable_32_types.h>
#else
# include "pgtable_64_types.h"
#endif

#ifndef __ASSEMBLY__

#include <linux/types.h>

/* PTE_PFN_MASK extracts the PFN from a (pte|pmd|pud|pgd)val_t */
#define PTE_PFN_MASK		((pteval_t)PHYSICAL_PAGE_MASK)

/* PTE_FLAGS_MASK extracts the flags from a (pte|pmd|pud|pgd)val_t */
#define PTE_FLAGS_MASK		(~PTE_PFN_MASK)

typedef struct pgprot { pgprotval_t pgprot; } pgprot_t;

#include <asm/maddr.h>

typedef struct { pgdval_t pgd; } pgd_t;

#define __pgd_ma(x) ((pgd_t) { (x) } )
static inline pgd_t xen_make_pgd(pgdval_t val)
{
	if (likely(val & _PAGE_PRESENT))
		val = pte_phys_to_machine(val);
	return (pgd_t) { val };
}

#define __pgd_val(x) ((x).pgd)
static inline pgdval_t xen_pgd_val(pgd_t pgd)
{
	pgdval_t ret = __pgd_val(pgd);
#if PAGETABLE_LEVELS == 2 && CONFIG_XEN_COMPAT <= 0x030002
	if (likely(ret))
		ret = machine_to_phys(ret) | _PAGE_PRESENT;
#else
	if (likely(ret & _PAGE_PRESENT))
		ret = pte_machine_to_phys(ret);
#endif
	return ret;
}

static inline pgdval_t pgd_flags(pgd_t pgd)
{
	return __pgd_val(pgd) & PTE_FLAGS_MASK;
}

#if PAGETABLE_LEVELS > 3
typedef struct { pudval_t pud; } pud_t;

#define __pud_ma(x) ((pud_t) { (x) } )
static inline pud_t xen_make_pud(pudval_t val)
{
	if (likely(val & _PAGE_PRESENT))
		val = pte_phys_to_machine(val);
	return (pud_t) { val };
}

#define __pud_val(x) ((x).pud)
static inline pudval_t xen_pud_val(pud_t pud)
{
	pudval_t ret = __pud_val(pud);
	if (likely(ret & _PAGE_PRESENT))
		ret = pte_machine_to_phys(ret);
	return ret;
}
#else
#include <asm-generic/pgtable-nopud.h>

#define __pud_val(x) __pgd_val((x).pgd)
static inline pudval_t xen_pud_val(pud_t pud)
{
	return xen_pgd_val(pud.pgd);
}
#endif

#if PAGETABLE_LEVELS > 2
typedef struct { pmdval_t pmd; } pmd_t;

#define __pmd_ma(x)	((pmd_t) { (x) } )
static inline pmd_t xen_make_pmd(pmdval_t val)
{
	if (likely(val & _PAGE_PRESENT))
		val = pte_phys_to_machine(val);
	return (pmd_t) { val };
}

#define __pmd_val(x) ((x).pmd)
static inline pmdval_t xen_pmd_val(pmd_t pmd)
{
	pmdval_t ret = __pmd_val(pmd);
#if CONFIG_XEN_COMPAT <= 0x030002
	if (likely(ret))
		ret = pte_machine_to_phys(ret) | _PAGE_PRESENT;
#else
	if (likely(ret & _PAGE_PRESENT))
		ret = pte_machine_to_phys(ret);
#endif
	return ret;
}
#else
#include <asm-generic/pgtable-nopmd.h>

#define __pmd_ma(x) ((pmd_t) { .pud.pgd = __pgd_ma(x) } )
#define __pmd_val(x) __pgd_val((x).pud.pgd)
static inline pmdval_t xen_pmd_val(pmd_t pmd)
{
	return xen_pgd_val(pmd.pud.pgd);
}
#endif

static inline pudval_t pud_flags(pud_t pud)
{
	return __pud_val(pud) & PTE_FLAGS_MASK;
}

static inline pmdval_t pmd_flags(pmd_t pmd)
{
	return __pmd_val(pmd) & PTE_FLAGS_MASK;
}

#define __pte_ma(x) ((pte_t) { .pte = (x) } )
static inline pte_t xen_make_pte(pteval_t val)
{
	if (likely((val & (_PAGE_PRESENT|_PAGE_IOMAP)) == _PAGE_PRESENT))
		val = pte_phys_to_machine(val);
	return (pte_t) { .pte = val };
}

#define __pte_val(x) ((x).pte)
static inline pteval_t xen_pte_val(pte_t pte)
{
	pteval_t ret = __pte_val(pte);
	if (likely((pte.pte_low & (_PAGE_PRESENT|_PAGE_IOMAP)) == _PAGE_PRESENT))
		ret = pte_machine_to_phys(ret);
	return ret;
}

static inline pteval_t pte_flags(pte_t pte)
{
	return __pte_val(pte) & PTE_FLAGS_MASK;
}

#define pgprot_val(x)	((x).pgprot)
#define __pgprot(x)	((pgprot_t) { (x) } )


typedef struct page *pgtable_t;

extern pteval_t __supported_pte_mask;
extern void set_nx(void);
extern int nx_enabled;

#define pgprot_writecombine	pgprot_writecombine
extern pgprot_t pgprot_writecombine(pgprot_t prot);

#ifndef CONFIG_XEN
/* Indicate that x86 has its own track and untrack pfn vma functions */
#define __HAVE_PFNMAP_TRACKING
#endif

#define __HAVE_PHYS_MEM_ACCESS_PROT
struct file;
pgprot_t phys_mem_access_prot(struct file *file, unsigned long pfn,
                              unsigned long size, pgprot_t vma_prot);
int phys_mem_access_prot_allowed(struct file *file, unsigned long pfn,
                              unsigned long size, pgprot_t *vma_prot);

/* Install a pte for a particular vaddr in kernel space. */
void set_pte_vaddr(unsigned long vaddr, pte_t pte);

extern void xen_pagetable_reserve(u64 start, u64 end);

struct seq_file;
extern void arch_report_meminfo(struct seq_file *m);

enum {
	PG_LEVEL_NONE,
	PG_LEVEL_4K,
	PG_LEVEL_2M,
	PG_LEVEL_1G,
	PG_LEVEL_NUM
};

#ifdef CONFIG_PROC_FS
extern void update_page_count(int level, unsigned long pages);
#else
static inline void update_page_count(int level, unsigned long pages) { }
#endif

/*
 * Helper function that returns the kernel pagetable entry controlling
 * the virtual address 'address'. NULL means no pagetable entry present.
 * NOTE: the return type is pte_t but if the pmd is PSE then we return it
 * as a pte too.
 */
extern pte_t *lookup_address(unsigned long address, unsigned int *level);

#endif	/* !__ASSEMBLY__ */

#endif /* _ASM_X86_PGTABLE_DEFS_H */
