/*
 * handle transition of Linux booting another kernel
 * Copyright (C) 2002-2005 Eric Biederman  <ebiederm@xmission.com>
 *
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <linux/mm.h>
#include <linux/kexec.h>
#include <linux/string.h>
#include <linux/gfp.h>
#include <linux/reboot.h>
#include <linux/numa.h>
#include <linux/ftrace.h>
#include <linux/io.h>
#include <linux/suspend.h>

#include <asm/pgtable.h>
#include <asm/tlbflush.h>
#include <asm/mmu_context.h>
#include <asm/debugreg.h>

static int init_one_level2_page(struct kimage *image, pgd_t *pgd,
				unsigned long addr)
{
	pud_t *pud;
	pmd_t *pmd;
	struct page *page;
	int result = -ENOMEM;

	addr &= PMD_MASK;
	pgd += pgd_index(addr);
	if (!pgd_present(*pgd)) {
		page = kimage_alloc_control_pages(image, 0);
		if (!page)
			goto out;
		pud = (pud_t *)page_address(page);
		clear_page(pud);
		set_pgd(pgd, __pgd(__pa(pud) | _KERNPG_TABLE));
	}
	pud = pud_offset(pgd, addr);
	if (!pud_present(*pud)) {
		page = kimage_alloc_control_pages(image, 0);
		if (!page)
			goto out;
		pmd = (pmd_t *)page_address(page);
		clear_page(pmd);
		set_pud(pud, __pud(__pa(pmd) | _KERNPG_TABLE));
	}
	pmd = pmd_offset(pud, addr);
	if (!pmd_present(*pmd))
		set_pmd(pmd, __pmd(addr | __PAGE_KERNEL_LARGE_EXEC));
	result = 0;
out:
	return result;
}

static void init_level2_page(pmd_t *level2p, unsigned long addr)
{
	unsigned long end_addr;

	addr &= PAGE_MASK;
	end_addr = addr + PUD_SIZE;
	while (addr < end_addr) {
		set_pmd(level2p++, __pmd(addr | __PAGE_KERNEL_LARGE_EXEC));
		addr += PMD_SIZE;
	}
}

static int init_level3_page(struct kimage *image, pud_t *level3p,
				unsigned long addr, unsigned long last_addr)
{
	unsigned long end_addr;
	int result;

	result = 0;
	addr &= PAGE_MASK;
	end_addr = addr + PGDIR_SIZE;
	while ((addr < last_addr) && (addr < end_addr)) {
		struct page *page;
		pmd_t *level2p;

		page = kimage_alloc_control_pages(image, 0);
		if (!page) {
			result = -ENOMEM;
			goto out;
		}
		level2p = (pmd_t *)page_address(page);
		init_level2_page(level2p, addr);
		set_pud(level3p++, __pud(__pa(level2p) | _KERNPG_TABLE));
		addr += PUD_SIZE;
	}
	/* clear the unused entries */
	while (addr < end_addr) {
		pud_clear(level3p++);
		addr += PUD_SIZE;
	}
out:
	return result;
}


static int init_level4_page(struct kimage *image, pgd_t *level4p,
				unsigned long addr, unsigned long last_addr)
{
	unsigned long end_addr;
	int result;

	result = 0;
	addr &= PAGE_MASK;
	end_addr = addr + (PTRS_PER_PGD * PGDIR_SIZE);
	while ((addr < last_addr) && (addr < end_addr)) {
		struct page *page;
		pud_t *level3p;

		page = kimage_alloc_control_pages(image, 0);
		if (!page) {
			result = -ENOMEM;
			goto out;
		}
		level3p = (pud_t *)page_address(page);
		result = init_level3_page(image, level3p, addr, last_addr);
		if (result)
			goto out;
		set_pgd(level4p++, __pgd(__pa(level3p) | _KERNPG_TABLE));
		addr += PGDIR_SIZE;
	}
	/* clear the unused entries */
	while (addr < end_addr) {
		pgd_clear(level4p++);
		addr += PGDIR_SIZE;
	}
out:
	return result;
}

#ifdef CONFIG_XEN

#include <xen/interface/kexec.h>
#include <xen/interface/memory.h>

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
	void *table_page;

	memset(xki->page_list, 0, sizeof(xki->page_list));

	control_page = page_address(image->control_code_page) + PAGE_SIZE;
	memcpy(control_page, relocate_kernel, PAGE_SIZE);

	table_page = page_address(image->control_code_page);

	xki->page_list[PA_CONTROL_PAGE] = __ma(control_page);
	xki->page_list[PA_TABLE_PAGE] = __ma(table_page);

	if (image->type == KEXEC_TYPE_DEFAULT)
		xki->page_list[PA_SWAP_PAGE] = page_to_phys(image->swap_page);
}

#include "machine_kexec_xen.c"

#endif /* CONFIG_XEN */

static void free_transition_pgtable(struct kimage *image)
{
	free_page((unsigned long)image->arch.pud);
	free_page((unsigned long)image->arch.pmd);
	free_page((unsigned long)image->arch.pte);
}

static int init_transition_pgtable(struct kimage *image, pgd_t *pgd)
{
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;
	unsigned long vaddr, paddr;
	int result = -ENOMEM;

	vaddr = (unsigned long)relocate_kernel;
	paddr = __pa(page_address(image->control_code_page)+PAGE_SIZE);
	pgd += pgd_index(vaddr);
	if (!pgd_present(*pgd)) {
		pud = (pud_t *)get_zeroed_page(GFP_KERNEL);
		if (!pud)
			goto err;
		image->arch.pud = pud;
		set_pgd(pgd, __pgd(__pa(pud) | _KERNPG_TABLE));
	}
	pud = pud_offset(pgd, vaddr);
	if (!pud_present(*pud)) {
		pmd = (pmd_t *)get_zeroed_page(GFP_KERNEL);
		if (!pmd)
			goto err;
		image->arch.pmd = pmd;
		set_pud(pud, __pud(__pa(pmd) | _KERNPG_TABLE));
	}
	pmd = pmd_offset(pud, vaddr);
	if (!pmd_present(*pmd)) {
		pte = (pte_t *)get_zeroed_page(GFP_KERNEL);
		if (!pte)
			goto err;
		image->arch.pte = pte;
		set_pmd(pmd, __pmd(__pa(pte) | _KERNPG_TABLE));
	}
	pte = pte_offset_kernel(pmd, vaddr);
	set_pte(pte, pfn_pte(paddr >> PAGE_SHIFT, PAGE_KERNEL_EXEC));
	return 0;
err:
	free_transition_pgtable(image);
	return result;
}


static int init_pgtable(struct kimage *image, unsigned long start_pgtable)
{
	pgd_t *level4p;
	int result;
	level4p = (pgd_t *)__va(start_pgtable);
	result = init_level4_page(image, level4p, 0, max_pfn << PAGE_SHIFT);
	if (result)
		return result;
	/*
	 * image->start may be outside 0 ~ max_pfn, for example when
	 * jump back to original kernel from kexeced kernel
	 */
	result = init_one_level2_page(image, level4p, image->start);
	if (result)
		return result;
	return init_transition_pgtable(image, level4p);
}

int machine_kexec_prepare(struct kimage *image)
{
	unsigned long start_pgtable;
	int result;

	/* Calculate the offsets */
	start_pgtable = page_to_pfn(image->control_code_page) << PAGE_SHIFT;

	/* Setup the identity mapped 64bit page table */
	result = init_pgtable(image, start_pgtable);
	if (result)
		return result;

	return 0;
}

void machine_kexec_cleanup(struct kimage *image)
{
	free_transition_pgtable(image);
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

	control_page = page_address(image->control_code_page) + PAGE_SIZE;
	memcpy(control_page, relocate_kernel, KEXEC_CONTROL_CODE_MAX_SIZE);

	page_list[PA_CONTROL_PAGE] = virt_to_phys(control_page);
	page_list[VA_CONTROL_PAGE] = (unsigned long)control_page;
	page_list[PA_TABLE_PAGE] =
	  (unsigned long)__pa(page_address(image->control_code_page));

	if (image->type == KEXEC_TYPE_DEFAULT)
		page_list[PA_SWAP_PAGE] = (page_to_pfn(image->swap_page)
						<< PAGE_SHIFT);

	/* now call it */
	image->start = relocate_kernel((unsigned long)image->head,
				       (unsigned long)page_list,
				       image->start,
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
#ifndef CONFIG_XEN /* could really be CONFIG_RELOCATABLE */
	VMCOREINFO_SYMBOL(phys_base);
#endif
	VMCOREINFO_SYMBOL(init_level4_pgt);

#ifdef CONFIG_NUMA
	VMCOREINFO_SYMBOL(node_data);
	VMCOREINFO_LENGTH(node_data, MAX_NUMNODES);
#endif
}

