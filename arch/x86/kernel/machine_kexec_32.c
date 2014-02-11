/*
 * handle transition of Linux booting another kernel
 * Copyright (C) 2002-2005 Eric Biederman  <ebiederm@xmission.com>
 *
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <linux/mm.h>
#include <linux/kexec.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/numa.h>
#include <linux/ftrace.h>
#include <linux/suspend.h>
#include <linux/gfp.h>
#include <linux/io.h>

#include <asm/pgtable.h>
#include <asm/pgalloc.h>
#include <asm/tlbflush.h>
#include <asm/mmu_context.h>
#include <asm/apic.h>
#include <asm/cpufeature.h>
#include <asm/desc.h>
#include <asm/system.h>
#include <asm/cacheflush.h>
#include <asm/debugreg.h>

#ifdef CONFIG_XEN
#include <xen/interface/kexec.h>
#endif

static void machine_kexec_free_page_tables(struct kimage *image)
{
	free_page((unsigned long)image->arch.pgd);
#ifdef CONFIG_X86_PAE
	free_page((unsigned long)image->arch.pmd0);
	free_page((unsigned long)image->arch.pmd1);
#endif
	free_page((unsigned long)image->arch.pte0);
	free_page((unsigned long)image->arch.pte1);
}

static int machine_kexec_alloc_page_tables(struct kimage *image)
{
	image->arch.pgd = (pgd_t *)get_zeroed_page(GFP_KERNEL);
#ifdef CONFIG_X86_PAE
#ifdef CONFIG_XEN /* machine address must fit into xki->page_list[PA_PGD] */
	if (image->arch.pgd) {
		struct page *pg = virt_to_page(image->arch.pgd);

		if (xen_limit_pages_to_max_mfn(pg, 0, BITS_PER_LONG) < 0) {
			image->arch.pgd = NULL;
			__free_page(pg);
			return -ENOMEM;
		}
	}
#endif
	image->arch.pmd0 = (pmd_t *)get_zeroed_page(GFP_KERNEL);
	image->arch.pmd1 = (pmd_t *)get_zeroed_page(GFP_KERNEL);
#endif
	image->arch.pte0 = (pte_t *)get_zeroed_page(GFP_KERNEL);
	image->arch.pte1 = (pte_t *)get_zeroed_page(GFP_KERNEL);
	if (!image->arch.pgd ||
#ifdef CONFIG_X86_PAE
	    !image->arch.pmd0 || !image->arch.pmd1 ||
#endif
	    !image->arch.pte0 || !image->arch.pte1) {
		machine_kexec_free_page_tables(image);
		return -ENOMEM;
	}
	return 0;
}

static void machine_kexec_page_table_set_one(
	pgd_t *pgd, pmd_t *pmd, pte_t *pte,
	unsigned long vaddr, unsigned long paddr)
{
	pud_t *pud;

	pgd += pgd_index(vaddr);
#ifdef CONFIG_X86_PAE
	if (!(pgd_val(*pgd) & _PAGE_PRESENT))
		set_pgd(pgd, __pgd(__pa(pmd) | _PAGE_PRESENT));
#endif
	pud = pud_offset(pgd, vaddr);
	pmd = pmd_offset(pud, vaddr);
	if (!(pmd_val(*pmd) & _PAGE_PRESENT))
		set_pmd(pmd, __pmd(__pa(pte) | _PAGE_TABLE));
	pte = pte_offset_kernel(pmd, vaddr);
	set_pte(pte, pfn_pte(paddr >> PAGE_SHIFT, PAGE_KERNEL_EXEC));
}

static void machine_kexec_prepare_page_tables(struct kimage *image)
{
	void *control_page;
	pmd_t *pmd = NULL;

	control_page = page_address(image->control_code_page);
#ifdef CONFIG_X86_PAE
	pmd = image->arch.pmd0;
#endif
	machine_kexec_page_table_set_one(
		image->arch.pgd, pmd, image->arch.pte0,
		(unsigned long)control_page, __pa(control_page));
#ifdef CONFIG_X86_PAE
	pmd = image->arch.pmd1;
#endif
	machine_kexec_page_table_set_one(
		image->arch.pgd, pmd, image->arch.pte1,
		__pa(control_page), __pa(control_page));
}

#ifdef CONFIG_XEN

#define __ma(x) (pfn_to_mfn(__pa((x)) >> PAGE_SHIFT) << PAGE_SHIFT)

#if PAGES_NR > KEXEC_XEN_NO_PAGES
#error PAGES_NR is greater than KEXEC_XEN_NO_PAGES - Xen support will break
#endif

#if PA_CONTROL_PAGE != 0
#error PA_CONTROL_PAGE is non zero - Xen support will break
#endif

void machine_kexec_setup_load_arg(xen_kexec_image_t *xki, struct kimage *image)
{
	void *control_page;

	memset(xki->page_list, 0, sizeof(xki->page_list));

	control_page = page_address(image->control_code_page);
	memcpy(control_page, relocate_kernel, PAGE_SIZE);

	xki->page_list[PA_CONTROL_PAGE] = __ma(control_page);
	xki->page_list[PA_PGD] = __ma(image->arch.pgd);

	if (image->type == KEXEC_TYPE_DEFAULT)
		xki->page_list[PA_SWAP_PAGE] = page_to_phys(image->swap_page);
}

int __init machine_kexec_setup_resources(struct resource *hypervisor,
					 struct resource *phys_cpus,
					 int nr_phys_cpus)
{
	int k;

	/* The per-cpu crash note resources belong to the hypervisor resource */
	for (k = 0; k < nr_phys_cpus; k++)
		request_resource(hypervisor, phys_cpus + k);

	return 0;
}

void machine_kexec_register_resources(struct resource *res) { ; }

#endif /* CONFIG_XEN */

/*
 * A architecture hook called to validate the
 * proposed image and prepare the control pages
 * as needed.  The pages for KEXEC_CONTROL_PAGE_SIZE
 * have been allocated, but the segments have yet
 * been copied into the kernel.
 *
 * Do what every setup is needed on image and the
 * reboot code buffer to allow us to avoid allocations
 * later.
 *
 * - Make control page executable.
 * - Allocate page tables
 * - Setup page tables
 */
int machine_kexec_prepare(struct kimage *image)
{
	int error;

	set_pages_x(image->control_code_page, 1);
	error = machine_kexec_alloc_page_tables(image);
	if (error)
		return error;
	machine_kexec_prepare_page_tables(image);
	return 0;
}

/*
 * Undo anything leftover by machine_kexec_prepare
 * when an image is freed.
 */
void machine_kexec_cleanup(struct kimage *image)
{
	set_pages_nx(image->control_code_page, 1);
	machine_kexec_free_page_tables(image);
}

#ifndef CONFIG_XEN
/*
 * Do not allocate memory (or fail in any way) in machine_kexec().
 * We are past the point of no return, committed to rebooting now.
 */
void machine_kexec(struct kimage *image)
{
	unsigned long page_list[PAGES_NR];
	void *control_page;
	int save_ftrace_enabled;
	asmlinkage unsigned long
		(*relocate_kernel_ptr)(unsigned long indirection_page,
				       unsigned long control_page,
				       unsigned long start_address,
				       unsigned int has_pae,
				       unsigned int preserve_context);

#ifdef CONFIG_KEXEC_JUMP
	if (image->preserve_context)
		save_processor_state();
#endif

	save_ftrace_enabled = __ftrace_enabled_save();

	/* Interrupts aren't acceptable while we reboot */
	local_irq_disable();
	hw_breakpoint_disable();

	if (image->preserve_context) {
#ifdef CONFIG_X86_IO_APIC
		/*
		 * We need to put APICs in legacy mode so that we can
		 * get timer interrupts in second kernel. kexec/kdump
		 * paths already have calls to disable_IO_APIC() in
		 * one form or other. kexec jump path also need
		 * one.
		 */
		disable_IO_APIC();
#endif
	}

	control_page = page_address(image->control_code_page);
	memcpy(control_page, relocate_kernel, KEXEC_CONTROL_CODE_MAX_SIZE);

	relocate_kernel_ptr = control_page;
	page_list[PA_CONTROL_PAGE] = __pa(control_page);
	page_list[VA_CONTROL_PAGE] = (unsigned long)control_page;
	page_list[PA_PGD] = __pa(image->arch.pgd);

	if (image->type == KEXEC_TYPE_DEFAULT)
		page_list[PA_SWAP_PAGE] = (page_to_pfn(image->swap_page)
						<< PAGE_SHIFT);

	/* now call it */
	image->start = relocate_kernel_ptr((unsigned long)image->head,
					   (unsigned long)page_list,
					   image->start, cpu_has_pae,
					   image->preserve_context);

#ifdef CONFIG_KEXEC_JUMP
	if (image->preserve_context)
		restore_processor_state();
#endif

	__ftrace_enabled_restore(save_ftrace_enabled);
}
#endif

void arch_crash_save_vmcoreinfo(void)
{
#ifdef CONFIG_NUMA
	VMCOREINFO_SYMBOL(node_data);
	VMCOREINFO_LENGTH(node_data, MAX_NUMNODES);
#endif
#ifdef CONFIG_X86_PAE
	VMCOREINFO_CONFIG(X86_PAE);
#endif
}

